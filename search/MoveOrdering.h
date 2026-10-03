//
// Created by tludwig on 19.09.26.
//

#ifndef CHESSPP_MOVEORDERING_H
#define CHESSPP_MOVEORDERING_H

#include <vector>
#include <optional>

#include "evaluation.h"
#include "search.h"
#include "../board/Board.h"
#include "../board/Move.h"
#include "../move_gen/MoveGen.h"



class MoveOrdering {
    Board const& board;
    int ply;
    MoveList& moves;
    std::vector<Score> scores;

    int ready = 0;

    int index = 0;
    History const& history;
    Killers const& killers;

public:
    MoveOrdering(Board const& b, int ply, MoveList& m, Move pv_move, Move tt_move, History const& history, Killers const& killers) : board(b), ply(ply), moves(m), scores(m.size(), 0), history(history), killers(killers) {
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
                    scores[i] = 10 * see(moves[i]);
                } else {
                    scores[i] = history.get(board.to_move, moves[i]);
                    if (moves[i] == killers.get(ply, 0)) {
                        scores[i] += 975;
                    } else if (moves[i] == killers.get(ply, 1)) {
                        scores[i] += 950;
                    }
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
    Score mvvlva(Move move);
};


#endif //CHESSPP_MOVEORDERING_H
