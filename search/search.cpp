//
// Created by tludwig on 15.09.26.
//

#include "search.h"

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

Score Search::negamax(int depth, int ply, Score alpha, Score beta) {
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

    MoveList moves = legal_moves(board);
    if (moves.size() == 0) {
        return moves.legality().checkers ? -MATE + ply : 0;
    }
    Score original_alpha = alpha;
    Move best_move = Move::null();
    MoveOrdering move_ordering(board, moves, tt_move);
    for (int i = 0; i < moves.size(); i++) {
        Move move = move_ordering.getMove();

        board.make_move(move);
        Score score = -negamax(depth - 1, ply + 1, -beta, -alpha);
        board.unmake_move(move);

        if (SEARCH_STOPPED) return 0;

        if (score > alpha) {
            alpha = score;
            best_move = move;
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

Search::Result Search::search_root(int depth, Move prev_best) {
    info.nodes++;

    MoveList moves = legal_moves(board);

    Result result {
        .score = -INF,
        .best_move = prev_best != Move::null() ? prev_best : moves[0]
    };

    MoveOrdering move_ordering(board, moves, prev_best);
    Score alpha = -INF;
    for (int i = 0; i < moves.size(); i++) {
        Move move = move_ordering.getMove();

        board.make_move(move);
        auto score = -negamax(depth - 1, 1, -INF, -alpha);
        board.unmake_move(move);

        if (SEARCH_STOPPED) return result;

        if (score > alpha) {
            alpha = score;
            result.score = score;
            result.best_move = move;
        }
    }

    return result;
}

Search::Result Search::run(std::stop_token const& token) {
    stop = token;

    Result result {
        .score = -INF,
        .best_move = Move::null()
    };
    for (int d = 1; depth_allowed(d); d++) {
        info.nodes = 0;
        Result r = search_root(d, result.best_move);

        result.best_move = r.best_move;
        if (SEARCH_STOPPED) break;
        result.score = r.score;

        info.depth = d;
        std::cout << "info depth " << d << " nodes " << info.nodes << " hashfull " << search_tt.hashfull() << std::endl;
        if (result.score > MATE_THRESHOLD || result.score < -MATE_THRESHOLD) break;
    }

    return result;
}

bool Search::depth_allowed(int depth) const {
    return !options.max_depth || depth <= options.max_depth.value();
}
