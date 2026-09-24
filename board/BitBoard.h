//
// Created by tludwig on 11.09.26.
//

#ifndef CHESSPP_BITBOARD_H
#define CHESSPP_BITBOARD_H

#include <bit>
#include <cassert>
#include <cstdint>

#include "Square.h"

using BitBoard = uint64_t;

constexpr BitBoard bit(const Square square) {
    return 1ULL << square;
}

constexpr Square get_lsb(const BitBoard board) {
    assert(board != 0);
    return std::countr_zero(board);
}

constexpr Square pop_lsb(BitBoard& board) {
    const Square s = get_lsb(board);
    board &= board - 1;
    return s;
}

constexpr BitBoard FILE_A = 0x0101010101010101ull;
constexpr BitBoard FILE_B = 0x0202020202020202ull;
constexpr BitBoard FILE_C = 0x0404040404040404ull;
constexpr BitBoard FILE_D = 0x0808080808080808ull;
constexpr BitBoard FILE_E = 0x1010101010101010ull;
constexpr BitBoard FILE_F = 0x2020202020202020ull;
constexpr BitBoard FILE_G = 0x4040404040404040ull;
constexpr BitBoard FILE_H = 0x8080808080808080ull;
constexpr BitBoard FILES[] = { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H };

constexpr BitBoard RANK_1 = 0x00000000000000FFull;
constexpr BitBoard RANK_2 = 0x000000000000FF00ull;
constexpr BitBoard RANK_3 = 0x0000000000FF0000ull;
constexpr BitBoard RANK_4 = 0x00000000FF000000ull;
constexpr BitBoard RANK_5 = 0x000000FF00000000ull;
constexpr BitBoard RANK_6 = 0x0000FF0000000000ull;
constexpr BitBoard RANK_7 = 0x00FF000000000000ull;
constexpr BitBoard RANK_8 = 0xFF00000000000000ull;
constexpr BitBoard RANKS[] = { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8 };

constexpr BitBoard shift(BitBoard board, int offset) {
    return offset >= 0 ? board << offset : board >> -offset;
}

#endif //CHESSPP_BITBOARD_H
