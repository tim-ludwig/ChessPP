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

#define MAXPLY 256
#define MAX_HISTORY 900
#define NULL_MOVE_MIN_DEPTH 6
#define NULL_MOVE_REDUCTION 2

class History {
public:
  Score history[2][64][64] = {0};

  void update(Color to_move, Move m, int bonus) {
    bonus = std::clamp(bonus, -MAX_HISTORY, MAX_HISTORY);
    history[to_move][m.from()][m.to()] += bonus - (history[to_move][m.from()][m.to()] * std::abs(bonus) / MAX_HISTORY);
  }

  Score get(Color to_move, Move m) const {
    return history[to_move][m.from()][m.to()];
  }
};

class Killers {
public:
  Move killers[2][MAXPLY] = {Move::null()};

  void clear() {
    for (int i = 0; i < MAXPLY; i++) {
      killers[0][i] = Move::null();
      killers[1][i] = Move::null();
    }
  }

  void update(int ply, Move m) {
    if (m == killers[0][ply]) return;
    killers[1][ply] = killers[0][ply];
    killers[0][ply] = m;
  }

  Move get(int ply, int i) const {
    return killers[i][ply];
  }
};

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
    std::size_t avg_cutoff_move = 0;
    std::size_t pv_researches = 0;
    std::size_t null_move_cuts = 0;
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

  explicit Search(Board& board) : board(board) {}

  std::stop_token stop;
  Result run(std::stop_token const& token);

private:
  int pv_length[MAXPLY];
  Move pv_moves[MAXPLY][MAXPLY];
  History history;
  Killers killers;

  Score qsearch(int ply, Score alpha, Score beta);
  template<NodeType node_type>
  Score search(int depth, int ply, Score alpha, Score beta, bool is_null_child, std::vector<Move> const& prev_pv, bool play_from_prev_pv);

  void build_pv_from_tt(int depth, int ply, int i, TranspositionTable& tt);

  bool depth_allowed(int depth) const {
    return (!options.max_depth || depth <= options.max_depth.value()) && depth < MAXPLY;
  }
  bool search_stopped() const {
    return stop.stop_requested() || (options.start_time && options.time_budget && std::chrono::steady_clock::now() >= options.start_time.value() + options.time_budget.value());
  }
};




#endif //CHESSPP_SEARCH_H
