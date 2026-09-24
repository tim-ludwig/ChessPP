//
// Created by tludwig on 17.09.26.
//

#ifndef CHESSPP_UCI_H
#define CHESSPP_UCI_H

#include <iostream>
#include <thread>

#include "../board/Board.h"

class UCI {
private:
    std::jthread worker;
    Board board;

public:
    void repl();

private:
    void handle_position(std::vector<std::string> const& tokens);
    void handle_go(std::vector<std::string> const& tokens);
};



#endif //CHESSPP_UCI_H
