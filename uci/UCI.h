//
// Created by tludwig on 17.09.26.
//

#ifndef CHESSPP_UCI_H
#define CHESSPP_UCI_H

#include <thread>

#include "../search/search.h"
#include "../board/Board.h"

class UCI {
private:
    std::jthread worker;
    Board board;
    Search search{
        .board = board
    };

public:
    void repl();

private:
    void handle_position(std::vector<std::string> const& tokens);
    void handle_go(std::vector<std::string> const& tokens);
    void handle_debug(std::vector<std::string> const& tokens);
};


inline int score_to_mate_moves(Score score) {
    int moves_from_mate = (MATE - std::abs(score) + 1) / 2;
    if (score < 0) moves_from_mate = -moves_from_mate;
    return moves_from_mate;
}


#endif //CHESSPP_UCI_H
