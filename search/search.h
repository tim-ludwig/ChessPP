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
    std::vector<Move> pv;
  };

  using Info = struct {
    int depth;
    std::size_t nodes;
  };

  using Options = struct {
    std::optional<int> max_depth;
    std::optional<std::chrono::steady_clock::time_point> deadline;
  };

  using NodeType = enum {
    PVNode,
    NonPVNode
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
  using PV = struct {
    std::size_t len;
    Move *moves;
  };

  Score qsearch(int ply, Score alpha, Score beta);
  template<NodeType node_type>
  Score search(int depth, int ply, Score alpha, Score beta, PV& pv_buffer, std::vector<Move> const& prev_pv, bool play_from_prev_pv);
  bool depth_allowed(int depth) const;
};




#endif //CHESSPP_SEARCH_H
