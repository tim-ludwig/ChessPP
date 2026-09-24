//
// Created by tludwig on 10.09.26.
//

#ifndef CHESSPP_MOVEGEN_H
#define CHESSPP_MOVEGEN_H

#include <vector>

#include "../board/Move.h"
#include "../board/Board.h"
#include "../precompute/movement.h"

BitBoard pawn_captures(BitBoard pawns, Color by);

struct LegalityInfo {
    BitBoard king_danger;
    BitBoard attacked;
    BitBoard checkers;
    BitBoard capture_mask;
    BitBoard push_mask;
    BitBoard pinned;
    BitBoard pin_rays[64];
};

class MoveList {
    LegalityInfo l;
    int n = 0;
    Move moves[256];

public:
    LegalityInfo& legality() { return l; }
    LegalityInfo const& legality() const { return l; }
    int size() const { return n; }
    void push_back(Move mov) { moves[n++] = mov; }
    Move& operator[](int index) { return moves[index]; }
    Move const& operator[](int index) const { return moves[index]; }
};

MoveList legal_moves(Board const& board, bool quiescence=false);

#endif //CHESSPP_MOVEGEN_H
