//
// Created by tludwig on 15.09.26.
//

#ifndef CHESSPP_EVALUATION_H
#define CHESSPP_EVALUATION_H
#include "../board/Board.h"

using Score = int;

constexpr Score INF = 32000;
constexpr Score MATE = 30000;
constexpr Score MATE_THRESHOLD = MATE - 1000;

// does not check for checkmate or stalemate, that happens in the search function
Score eval(Board const& b);

#endif //CHESSPP_EVALUATION_H
