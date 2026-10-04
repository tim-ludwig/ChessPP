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
    auto* entry = search_tt.lookup(zhash);
    if (entry == nullptr)
        entry = qsearch_tt.lookup(zhash);
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
        MoveOrdering move_ordering(board, ply, moves, Move::null(), tt_move, history, killers);
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

void Search::build_pv_from_tt(int depth, int ply, int i, TranspositionTable& tt) {
    if (depth == 0) return;
    if (board.is_draw()) return;

    auto* entry = tt.lookup(board.zhash_stack.back());
    if (entry == nullptr || entry->best_move == Move::null() || entry->depth < depth) return;
    Move move = entry->best_move;

    MoveList moves = legal_moves(board);
    bool found = false;
    for (int j = 0; j < moves.size(); j++) {
        if (moves[j] == move) {
            found = true;
            break;
        }
    }
    if (!found) return;

    pv_length[ply] = i + 1;
    pv_moves[ply][i] = move;
    board.make_move(move);
    build_pv_from_tt(depth - 1, ply, i + 1, tt);
    board.unmake_move(move);
}

template<Search::NodeType node_type>
Score Search::search(int depth, int ply, Score alpha, Score beta, bool is_null_child, std::vector<Move> const& prev_pv, bool play_from_prev_pv) {
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
                        build_pv_from_tt(depth, ply, 0, search_tt);
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

    MoveList moves = legal_moves(board);
    if (moves.size() == 0) {
        return moves.legality().checkers ? -MATE + ply : 0;
    }

    if (depth == 0) {
        return qsearch(ply, alpha, beta);
    }

    if constexpr (!is_pv_node) {
        if (!is_null_child && depth >= NULL_MOVE_MIN_DEPTH && !moves.legality().checkers && (
                board.bitboards[board.to_move][KNIGHT]
                | board.bitboards[board.to_move][BISHOP]
                | board.bitboards[board.to_move][ROOK]
                | board.bitboards[board.to_move][QUEEN]
            )) {
            board.make_null_move();
            Score score = -search<NonPVNode>(depth - 1 - NULL_MOVE_REDUCTION, ply + 1,
                -beta - 1, -beta, true,
                prev_pv, false);
            board.unmake_null_move();

            if (search_stopped()) return 0;

            if (score >= beta && -MATE_THRESHOLD < score && score < MATE_THRESHOLD) {
                info.null_move_cuts++;
                return beta;
            }
        }
    }

    info.nodes++;

    Move pv_move = play_from_prev_pv && ply < prev_pv.size() ? prev_pv[ply] : Move::null();
    MoveOrdering move_ordering(board, ply, moves, pv_move, tt_move, history, killers);

    Score original_alpha = alpha;
    Move best_move = Move::null();
    for (int i = 0; i < moves.size(); i++) {
        Move move = move_ordering.getMove();

        pv_length[ply + 1] = 0;
        Score score;
        board.make_move(move);
        if (i == 0) {
            score = -search<node_type>(depth - 1, ply + 1,
                -beta, -alpha, false,
                prev_pv, play_from_prev_pv);
        } else {
            score = -search<NonPVNode>(depth - 1, ply + 1,
                -alpha - 1, -alpha, false,
                prev_pv, false);
            if constexpr (is_pv_node) {
                if (alpha < score && score < beta) {
                    info.pv_researches++;
                    score = -search<PVNode>(depth - 1, ply + 1,
                        -beta, -alpha, false,
                        prev_pv, false);
                }
            }
        }
        board.unmake_move(move);

        if (search_stopped()) return 0;

        if (score > alpha) {
            alpha = score;
            best_move = move;
            if constexpr (is_pv_node) {
                pv_moves[ply][0] = move;
                memcpy(&pv_moves[ply][1], &pv_moves[ply + 1][0], pv_length[ply + 1] * sizeof(Move));
                pv_length[ply] = 1 + pv_length[ply + 1];
            }
        }
        if (alpha >= beta) {
            info.beta_cuts++;
            if (i == 0) info.first_move_cuts++;
            info.avg_cutoff_move += i + 1;

            if (move.is_quiet()) {
                history.update(board.to_move, move, depth * depth);
                killers.update(ply, move);
            }
            for (int j = 0; j < i; j++) {
                if (moves[j].is_quiet()) {
                    history.update(board.to_move, moves[j], -(depth * depth));
                }
            }
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

    killers.clear();

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
        info.avg_cutoff_move = 0;
        info.pv_researches = 0;
        info.tt_cuts = 0;
        info.null_move_cuts = 0;

        auto depth_start =  std::chrono::steady_clock::now();
        pv_length[0] = 0;
        Score score = search<PVNode>(d, 0, -INF, INF, false, result.pv, d != 1);
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - depth_start).count();
        if (pv_length[0] > 0) result.pv.assign(&pv_moves[0][0], &pv_moves[0][pv_length[0]]);

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
        std::cout << "info string avg_cutoff_move " << (info.beta_cuts > 0 ? (double)info.avg_cutoff_move / info.beta_cuts : 0) << std::endl;
        std::cout << "info string tt_cuts " << info.tt_cuts << std::endl;
        std::cout << "info string null_move_cuts " << info.null_move_cuts << std::endl;
        std::cout << "info string pv_researches " << info.pv_researches << std::endl;
        if (!options.pondering && (result.score > MATE_THRESHOLD || result.score < -MATE_THRESHOLD)) break;
    }
    search_tt.new_generation();
    qsearch_tt.new_generation();
    return result;
}
