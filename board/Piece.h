//
// Created by tludwig on 09.09.26.
//

#ifndef CHESSPP_PIECE_H
#define CHESSPP_PIECE_H

#include <iostream>

#include <cstdint>

using Color = uint8_t;
constexpr Color WHITE = 0;
constexpr Color BLACK = 1;
constexpr Color BOTH = 2;
constexpr Color EMPTY = 3;

using PieceType = uint8_t;
constexpr PieceType PAWN = 0;
constexpr PieceType KNIGHT = 1;
constexpr PieceType BISHOP = 2;
constexpr PieceType ROOK = 3;
constexpr PieceType QUEEN = 4;
constexpr PieceType KING = 5;

struct Piece {
    uint8_t data;

    Piece() : Piece(EMPTY, EMPTY) {}
    Piece(const Color color, const PieceType type) : data{static_cast<uint8_t>(color << 3 | type)} {}

    [[nodiscard]] Color color() const { return static_cast<Color>(data >> 3); }
    [[nodiscard]] PieceType type() const { return static_cast<PieceType>(data & 0b111); }
};

std::ostream& operator<<(std::ostream& os, Piece const& piece);

#endif //CHESSPP_PIECE_H
