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
    MoveOrdering(Board const& b, MoveList& m, Move pv_move, Move tt_move) : board(b), moves(m), scores(m.size()) {
        for (int i = 0; i < moves.size(); i++) {
            if (moves[i] == pv_move) {
                moves[i] = moves[0];
                scores[i] = scores[0];
                moves[0] = pv_move;
                scores[0] = std::numeric_limits<Score>::max();
            } else if (moves[i] == tt_move) {
                int tt_index = pv_move == Move::null() ? 0 : 1;
                moves[i] = moves[tt_index];
                scores[i] = scores[tt_index];
                moves[tt_index] = tt_move;
                scores[tt_index] = std::numeric_limits<Score>::max();
            } else {
                Move move = moves[i];
                Score score = 0;

                if (move.is_capture() || move.is_promotion()) {
                    score += see(move);
                }

                scores[i] = score;
            }
        }
    }

    Move getMove() {
        if (index >= moves.size()) throw std::out_of_range("No more moves to order");

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

private:
    Score see(Move move);
};


#endif //CHESSPP_MOVEORDERING_H
