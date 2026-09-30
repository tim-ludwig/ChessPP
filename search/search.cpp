//
// Created by tludwig on 15.09.26.
//

#include "search.h"

#include <cstring>

#include "MoveOrdering.h"

#include "../move_gen/MoveGen.h"
#include "../uci/UCI.h"

Score Search::qsearch(int ply, Score alpha, Score beta) {
    if (search_stopped()) return 0;

    info.nodes++;

    if (board.is_draw()) return 0;

    uint64_t zhash = board.zhash_stack.back();
    auto* entry = qsearch_tt.lookup(zhash);
    Move tt_move = Move::null();
    if (entry != nullptr) {
        Score tt_score = score_from_tt(entry->score, ply);
        tt_move = entry->best_move;
        switch (entry->bound) {
            case TranspositionTable::Entry::EXACT:
                return tt_score;
            case TranspositionTable::Entry::LOWER:
                if (tt_score >= beta) return tt_score;
                break;
            case TranspositionTable::Entry::UPPER:
                if (tt_score <= alpha) return tt_score;
                break;
        }
    }

    MoveList check_moves = legal_moves(board);
    bool in_check = check_moves.legality().checkers != 0;
    Score original_alpha = alpha;
    Score best = -INF;
    Move best_move = Move::null();

    if (!in_check) {
        best = eval(board);
        if (best >= beta) {
            qsearch_tt.store({
                .key = board.zhash_stack.back(),
                .generation = qsearch_tt.generation,
                .depth = 0,
                .best_move = best_move,
                .score = score_to_tt(best, ply),
                .bound = TranspositionTable::Entry::LOWER
            });
            return best;
        }
        if (best > alpha) alpha = best;
    }

    MoveList moves = in_check ? check_moves : legal_moves(board, true);
    if (moves.size() == 0) {
        best = in_check ? -MATE + ply : alpha;
    } else {
        MoveOrdering move_ordering(board, moves, Move::null(), tt_move);
        for (int i = 0; i < moves.size(); i++) {
            Move move = move_ordering.getMove();
            board.make_move(move);
            Score score = -qsearch(ply + 1, -beta, -alpha);
            board.unmake_move(move);

            if (search_stopped()) return 0;
            if (score > best) {
                best = score;
                best_move = move;
            }
            if (score > alpha) alpha = score;
            if (alpha >= beta) break;
        }
    }

    TranspositionTable::Entry::Bound bound;
    if (alpha <= original_alpha)
        bound = TranspositionTable::Entry::UPPER;
    else if (alpha >= beta)
        bound = TranspositionTable::Entry::LOWER;
    else
        bound = TranspositionTable::Entry::EXACT;
    qsearch_tt.store({
        .key = board.zhash_stack.back(),
        .generation = qsearch_tt.generation,
        .depth = 0,
        .best_move = best_move,
        .score = score_to_tt(best, ply),
        .bound = bound
    });

    return best;
}

void Search::build_pv_from_tt(int depth, TranspositionTable& tt, PV& pv) {
    if (depth == 0) return;
    if (board.is_draw()) return;

    auto* entry = tt.lookup(board.zhash_stack.back());
    if (entry == nullptr || entry->best_move == Move::null() || entry->depth < depth) return;

    MoveList moves = legal_moves(board);
    bool found = false;
    for (int i = 0; i < moves.size(); i++) {
        if (moves[i] == entry->best_move) {
            found = true;
            break;
        }
    }
    if (!found) return;

    pv.moves[pv.len++] = entry->best_move;
    board.make_move(entry->best_move);
    build_pv_from_tt(depth - 1, tt, pv);
    board.unmake_move(entry->best_move);
}

template<Search::NodeType node_type>
Score Search::search(int depth, int ply, Score alpha, Score beta, PV& pv, std::vector<Move> const& prev_pv, bool play_from_prev_pv) {
    constexpr bool is_pv_node = node_type == PVNode;
    if (search_stopped()) return 0;

    if (board.is_draw()) return 0;

    auto* entry = search_tt.lookup(board.zhash_stack.back());
    Move tt_move = Move::null();
    if (entry != nullptr) {
        tt_move = entry->best_move;

        if (entry->depth >= depth) {
            Score tt_score = score_from_tt(entry->score, ply);
            switch (entry->bound) {
                case TranspositionTable::Entry::EXACT:
                    if constexpr (is_pv_node) {
                        pv.len = 0;
                        build_pv_from_tt(depth, search_tt, pv);
                    }
                    info.tt_cuts++;
                    return tt_score;

                case TranspositionTable::Entry::LOWER:
                    if (tt_score >= beta) {
                        info.tt_cuts++;
                        return tt_score;
                    }
                    break;

                case TranspositionTable::Entry::UPPER:
                    if (tt_score <= alpha) {
                        info.tt_cuts++;
                        return tt_score;
                    }
                    break;
            }
        }
    }

    if (depth == 0) {
        info.nodes--;
        return qsearch(ply, alpha, beta);
    }

    info.nodes++;

    MoveList moves = legal_moves(board);
    if (moves.size() == 0) {
        return moves.legality().checkers ? -MATE + ply : 0;
    }

    Move pv_buffer[depth];
    PV child_pv = {
        .len = 0,
        .moves = pv_buffer
    };

    Move pv_move = play_from_prev_pv && ply < prev_pv.size() ? prev_pv[ply] : Move::null();
    MoveOrdering move_ordering(board, moves, pv_move, tt_move);

    Score original_alpha = alpha;
    Move best_move = Move::null();
    for (int i = 0; i < moves.size(); i++) {
        Move move = move_ordering.getMove();

        Score score;
        board.make_move(move);
        if (i == 0) {
            score = -search<node_type>(depth - 1, ply + 1,
                -beta, -alpha,
                child_pv, prev_pv, play_from_prev_pv);
        } else {
            score = -search<NonPVNode>(depth - 1, ply + 1,
                -alpha - 1, -alpha,
                child_pv, prev_pv, false);
            if constexpr (is_pv_node) {
                if (alpha < score && score < beta) {
                    info.pv_researches++;
                    score = -search<PVNode>(depth - 1, ply + 1,
                        -beta, -alpha,
                        child_pv, prev_pv, false);
                }
            }
        }
        board.unmake_move(move);

        if (search_stopped()) return 0;

        if (score > alpha) {
            alpha = score;
            best_move = move;
            if constexpr (is_pv_node) {
                pv.moves[0] = move;
                memcpy(pv.moves + 1, child_pv.moves, child_pv.len * sizeof(Move));
                pv.len = child_pv.len + 1;
            }
        }
        if (alpha >= beta) {
            info.beta_cuts++;
            if (i == 0) info.first_move_cuts++;
            break;
        }
    }

    TranspositionTable::Entry::Bound bound;
    if (alpha <= original_alpha)
        bound = TranspositionTable::Entry::UPPER;
    else if (alpha >= beta)
        bound = TranspositionTable::Entry::LOWER;
    else
        bound = TranspositionTable::Entry::EXACT;

    search_tt.store({
        .key = board.zhash_stack.back(),
        .generation = search_tt.generation,
        .depth = depth,
        .best_move = best_move,
        .score = score_to_tt(alpha, ply),
        .bound = bound
    });

    return alpha;
}

Search::Result Search::run(std::stop_token const& token) {
    stop = token;

    MoveList moves = legal_moves(board);
    Result result {
        .score = -INF,
        .pv = {moves[0]}
    };
    for (int d = 1; depth_allowed(d); d++) {
        info.depth = d;
        info.nodes = 0;
        info.beta_cuts = 0;
        info.first_move_cuts = 0;
        info.pv_researches = 0;
        info.tt_cuts = 0;


        Move pv_buffer[d];
        PV child_pv = {
            .len = 0,
            .moves = pv_buffer
        };
        auto depth_start =  std::chrono::steady_clock::now();
        Score score = search<PVNode>(d, 0, -INF, INF, child_pv, result.pv, d != 1);
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - depth_start).count();
        if (child_pv.len > 0) result.pv.assign(child_pv.moves, child_pv.moves + child_pv.len);

        if (search_stopped()) break;
        result.score = score;

        std::cout << "info depth " << d;
        if (result.score > MATE_THRESHOLD || result.score < -MATE_THRESHOLD) {
            std::cout << " score mate " << score_to_mate_moves(result.score);
        } else {
            std::cout << " score cp " << result.score;
        }
        std::cout << " nodes " << info.nodes;
        uint64_t nps = elapsed_ms > 0
            ? info.nodes * 1000 / elapsed_ms
            : 0;
        std::cout << " nps " << nps;
        std::cout << " hashfull " << search_tt.hashfull();
        std::cout << " pv";
        for (Move move : result.pv) {
            std::cout << " " << move.coordinate_notation();
        }
        std::cout << std::endl;

        std::cout << "info string beta_cuts " << info.beta_cuts << std::endl;
        std::cout << "info string first_move_cuts " << info.first_move_cuts << std::endl;
        std::cout << "info string pv_researches " << info.pv_researches << std::endl;
        std::cout << "info string tt_cuts " << info.tt_cuts << std::endl;
        if (!options.pondering && (result.score > MATE_THRESHOLD || result.score < -MATE_THRESHOLD)) break;
    }
    search_tt.new_generation();
    qsearch_tt.new_generation();
    return result;
}
