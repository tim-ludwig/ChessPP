//
// Created by tludwig on 15.09.26.
//

#include "search.h"

#include <cstring>
#include <oneapi/tbb/partitioner.h>

#include "MoveOrdering.h"

#include "../move_gen/MoveGen.h"

#define SEARCH_STOPPED (stop.stop_requested() || (options.deadline && std::chrono::steady_clock::now() >= options.deadline.value()))

Score Search::qsearch(int ply, Score alpha, Score beta) {
    if (SEARCH_STOPPED) return 0;

    info.nodes++;

    if (board.is_draw()) return 0;

    uint64_t zhash = board.zhash_stack.back();
    auto* entry = qsearch_tt.lookup(zhash);
    if (entry != nullptr) {
        Score tt_score = score_from_tt(entry->score, ply);
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
                .best_move = best_move,
                .score = score_to_tt(best, ply),
                .depth = 0,
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
        MoveOrdering move_ordering(board, moves, entry != nullptr ? entry->best_move : Move::null());
        for (int i = 0; i < moves.size(); i++) {
            Move move = move_ordering.getMove();
            board.make_move(move);
            Score score = -qsearch(ply + 1, -beta, -alpha);
            board.unmake_move(move);

            if (SEARCH_STOPPED) return 0;
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
        .best_move = best_move,
        .score = score_to_tt(best, ply),
        .depth = 0,
        .bound = bound
    });

    return best;
}

template<Search::NodeType node_type>
Score Search::search(int depth, int ply, Score alpha, Score beta, PV& pv, PV const* prev_pv) {
    if (SEARCH_STOPPED) return 0;

    info.nodes++;

    if (board.is_draw()) return 0;

    auto* entry = search_tt.lookup(board.zhash_stack.back());
    Move tt_move = Move::null();
    if (entry != nullptr) {
        tt_move = entry->best_move;

        if (entry->depth >= depth) {
            Score tt_score = score_from_tt(entry->score, ply);
            switch (entry->bound) {
                case TranspositionTable::Entry::EXACT:
                    pv.len = 1;
                    pv.moves[0] = tt_move;
                    return tt_score;

                case TranspositionTable::Entry::LOWER:
                    if (tt_score >= beta) {
                        return tt_score;
                    }
                    break;

                case TranspositionTable::Entry::UPPER:
                    if (tt_score <= alpha) {
                        return tt_score;
                    }
                    break;
            }
        }
    }

    if (depth == 0) {
        return qsearch(ply, alpha, beta);
    }

    Move pv_buffer[depth];
    PV child_pv = {
        .len = 0,
        .moves = pv_buffer
    };

    MoveList moves = legal_moves(board);
    if (moves.size() == 0) {
        return moves.legality().checkers ? -MATE + ply : 0;
    }

    Move first_move = prev_pv && prev_pv->len > ply ? prev_pv->moves[ply] : tt_move;
    MoveOrdering move_ordering(board, moves, first_move);

    Score original_alpha = alpha;
    Move best_move = Move::null();
    constexpr bool is_pv_node = node_type == PVNode;
    for (int i = 0; i < moves.size(); i++) {
        Move move = move_ordering.getMove();

        Score score;
        board.make_move(move);
        if (i == 0) {
            score = -search<node_type>(depth - 1, ply + 1,
                -beta, -alpha,
                child_pv, i == 0 ? prev_pv : nullptr);
        } else {
            score = -search<NonPVNode>(depth - 1, ply + 1,
                -alpha - 1, -alpha,
                child_pv, nullptr);
            if (is_pv_node && score > alpha) {
                score = -search<PVNode>(depth - 1, ply + 1,
                    -beta, -alpha,
                    child_pv, nullptr);
            }
        }
        board.unmake_move(move);

        if (SEARCH_STOPPED) return 0;

        if (score > alpha) {
            alpha = score;
            best_move = move;
            memcpy(pv.moves + 1, child_pv.moves, child_pv.len * sizeof(Move));
            pv.moves[0] = move;
            pv.len = child_pv.len + 1;
        }
        if (alpha >= beta) {
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
        .best_move = best_move,
        .score = score_to_tt(alpha, ply),
        .depth = depth,
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
        info.nodes = 0;

        Move pv_buffer[d];
        PV child_pv = {
            .len = 0,
            .moves = pv_buffer
        };
        PV prev_pv = {
            .len = result.pv.size(),
            .moves = result.pv.data()
        };

        Score score = search<PVNode>(d, 0, -INF, INF, child_pv, &prev_pv);

        result.pv.assign(pv_buffer, pv_buffer + child_pv.len);
        if (SEARCH_STOPPED) break;
        result.score = score;

        info.depth = d;
        std::cout << "info depth " << d << " nodes " << info.nodes << " pv";
        for (Move move : result.pv) {
            std::cout << " " << move.coordinate_notation();
        }
        std::cout << std::endl;

        std::cout << "hashfull " << search_tt.hashfull() << std::endl;
        if (result.score > MATE_THRESHOLD || result.score < -MATE_THRESHOLD) break;
    }

    return result;
}

bool Search::depth_allowed(int depth) const {
    return !options.max_depth || depth <= options.max_depth.value();
}
