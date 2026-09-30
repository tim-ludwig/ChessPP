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



class MoveOrdering {
    Board const& board;
    MoveList& moves;
    std::vector<Score> scores;

    int ready = 0;

    int index = 0;

public:
    MoveOrdering(Board const& b, MoveList& m, Move pv_move, Move tt_move) : board(b), moves(m), scores(m.size(), 0) {
        for (int i = 0; i < moves.size(); i++) {
            if (moves[i] == pv_move) {
                scores[i] = std::numeric_limits<Score>::max();
                ready++;
            } else if (moves[i] == tt_move) {
                scores[i] = std::numeric_limits<Score>::max() - 1;
                ready++;
            }
            if (ready == 2) break;
        }
    }

    Move getMove() {
        assert(index < moves.size());

        if (index >= ready) {
            for (int i = index; i < moves.size(); i++) {
                if (moves[i].is_capture() || moves[i].is_promotion()) {
                    scores[i] = see(moves[i]);
                }
            }
            ready = moves.size();
        }

        auto best = std::max_element(scores.begin() + index, scores.end()) - scores.begin();
        std::swap(moves[index], moves[best]);
        std::swap(scores[index], scores[best]);
        return moves[index++];
    }

private:
    Score see(Move move);
};


#endif //CHESSPP_MOVEORDERING_H
