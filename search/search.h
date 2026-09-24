//
// Created by tludwig on 15.09.26.
//

#ifndef CHESSPP_SEARCH_H
#define CHESSPP_SEARCH_H

#include <optional>
#include <stop_token>

#include "evaluation.h"
#include "TranspositionTable.h"
#include "../board/Move.h"

class Search {
public:
  using Result = struct {
    Score score;
    Move best_move;
  };

  using Info = struct {
    int depth;
    int nodes;
  };

  using Options = struct {
    std::optional<int> max_depth;
    std::optional<std::chrono::steady_clock::time_point> deadline;
  };

public:
  Board& board;
  Options options;
  Info info;
  TranspositionTable search_tt;
  TranspositionTable qsearch_tt;
  std::stop_token stop;

  Result run(std::stop_token const& token);

private:
  Score qsearch(int ply, Score alpha, Score beta);
  Score negamax(int depth, int ply, Score alpha, Score beta);
  Result search_root(int depth, Move previous_best);
  bool depth_allowed(int depth) const;
};




#endif //CHESSPP_SEARCH_H
