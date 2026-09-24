//
// Created by tludwig on 12.09.26.
//

#ifndef CHESSPP_PERFT_H
#define CHESSPP_PERFT_H

#include "../board/Board.h"
#include <stop_token>

void perft(Board& board, int depth, std::stop_token const& stop);
void interactive_perft(Board& board, int depth, std::stop_token const& stop);

#endif //CHESSPP_PERFT_H
