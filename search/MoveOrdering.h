//
// Created by tludwig on 19.09.26.
//

#ifndef CHESSPP_MOVEORDERING_H
#define CHESSPP_MOVEORDERING_H

#include <vector>
#include <optional>

#include "evaluation.h"
#include "../board/Board.h"
#include "../board/Move.h"
#include "../move_gen/MoveGen.h"

constexpr int PIECE_VALUE[6] = {
    100,  // pawn
    320,  // knight
    330,  // bishop
    500,  // rook
    900,  // queen
    1000  // king
};

class MoveOrdering {
    Board const& board;
    MoveList& moves;
    std::vector<Score> scores;

    int index = 0;

public:
    MoveOrdering(Board const& b, MoveList& m) : board(b), moves(m) {
        scores.resize(m.size());

        for (int i = 0; i < moves.size(); i++) {
            int score = 0;

            Move move = moves[i];
            if (move.is_promotion()) {
                score += 10000 + PIECE_VALUE[move.promotion_piece()];
            }

            if (move.is_capture()) {
                Piece victim = board.pieces[move.to()];
                Piece attacker = board.pieces[move.from()];

                score += 5000
                      + 10 * PIECE_VALUE[victim.type()]
                      - PIECE_VALUE[attacker.type()];
            }

            scores[i] = score;
        }
    }

    MoveOrdering(Board const& b, MoveList& m, Move prev_best) : MoveOrdering(b, m) {
        if (prev_best == Move::null()) return;

        for (int i = 0; i < moves.size(); i++) {
            if (moves[i] == prev_best) {
                std::swap(moves[i], moves[0]);
                std::swap(scores[i], scores[0]);
                scores[0] = std::numeric_limits<Score>::max();
                break;
            }
        }
    }

    Move getMove() {
        if (index >= moves.size()) throw std::out_of_range("No more moves to order");
        if (scores[index] == std::numeric_limits<Score>::max()) return moves[index++];

        int best_index = index;
        for (int i = index + 1; i < moves.size(); i++) {
            if (scores[i] > scores[best_index]) {
                best_index = i;
            }
        }
        std::swap(scores[best_index], scores[index]);
        std::swap(moves[best_index], moves[index]);
        return moves[index++];
    }
};


#endif //CHESSPP_MOVEORDERING_H
