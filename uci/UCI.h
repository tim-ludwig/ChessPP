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
};



#endif //CHESSPP_UCI_H
