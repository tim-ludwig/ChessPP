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

  using Info = struct Info {
    int depth = 0;
    std::size_t nodes = 0;
    std::size_t tt_cuts = 0;
    std::size_t beta_cuts = 0;
    std::size_t first_move_cuts = 0;
    std::size_t pv_researches = 0;
  };

  using Options = struct {
    std::optional<int> max_depth;
    bool pondering;
    std::optional<std::chrono::steady_clock::time_point> start_time;
    std::optional<std::chrono::steady_clock::duration> time_budget;
  };

  using NodeType = enum {
    PVNode,
    NonPVNode
  };

public:
  Board& board;
  Options options;
  Info info;
  TranspositionTable search_tt{1 << 20};
  TranspositionTable qsearch_tt{1 << 20};
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

  void build_pv_from_tt(int depth, TranspositionTable& tt, PV& pv);

  bool depth_allowed(int depth) const {
    return !options.max_depth || depth <= options.max_depth.value();
  }
  bool search_stopped() const {
    return stop.stop_requested() || (options.start_time && options.time_budget && std::chrono::steady_clock::now() >= options.start_time.value() + options.time_budget.value());
  }
};




#endif //CHESSPP_SEARCH_H
