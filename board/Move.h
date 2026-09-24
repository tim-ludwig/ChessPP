//
// Created by tludwig on 08.09.26.
//

#ifndef CHESSPP_MOVE_H
#define CHESSPP_MOVE_H

#include <cstdint>

#include "Piece.h"
#include "Square.h"

struct Board;
// 16-bit move:
//
//  15 14 13 12 11         6 5        0
// +--+--+-----+------------+----------+
// | P| C|  PP |     TO     |   FROM   |
// +--+--+-----+------------+----------+
//
// P  = promotion flag
// C  = capture flag
// PP = promotion piece
//      00 knight
//      01 bishop
//      10 rook
//      11 queen
//
// Special flags:
// 0000 quiet
// 0001 double pawn push
// 0010 castle
// x1xx capture
// 0101 en passant
// 1xxx promotion
struct Move {
    static constexpr uint16_t FROM_MASK       = 0x003F;
    static constexpr uint16_t TO_MASK         = 0x0FC0;
    // special move flags
    static constexpr uint16_t FLAG_MASK       = 0xF000;
    static constexpr uint16_t QUIET           = 0x0000;
    static constexpr uint16_t DOUBLE_PAWN     = 0x1000;
    static constexpr uint16_t CASTLE          = 0x2000;
    static constexpr uint16_t CAPTURE_FLAG    = 0x4000;
    static constexpr uint16_t EN_PASSANT      = 0x5000;
    static constexpr uint16_t PROMOTION_FLAG  = 0x8000;
    static constexpr uint16_t PROMOTION_PIECE = 0x3000;

    uint16_t data;

    Move() = default;
    constexpr Move(const Square from, const Square to, const uint16_t flags) : data{static_cast<uint16_t>(flags | to << 6 | from)} {}

    [[nodiscard]] constexpr uint16_t flags() const { return data & FLAG_MASK; }

    bool operator==(Move const& move) const = default;

public:
    [[nodiscard]] static constexpr Move null() { return {0, 0, QUIET}; }
    [[nodiscard]] static constexpr Move quiet(Square from, Square to)       { return {from, to, QUIET}; }
    [[nodiscard]] static constexpr Move double_pawn(Square from, Square to) { return {from, to, DOUBLE_PAWN}; }
    [[nodiscard]] static constexpr Move castle(Square from, Square to)      { return {from, to, CASTLE}; }
    [[nodiscard]] static constexpr Move capture(Square from, Square to)     { return {from, to, CAPTURE_FLAG}; }
    [[nodiscard]] static constexpr Move en_passant(Square from, Square to)  { return {from, to, EN_PASSANT}; }

    [[nodiscard]] static constexpr Move promote(Square from, Square to, PieceType promotionPiece, bool capture) {
        uint16_t special = PROMOTION_FLAG | (promotionPiece - 1) << 12;
        if (capture) special |= CAPTURE_FLAG;
        return {from, to, special};
    }

    [[nodiscard]]
    constexpr Square from() const {
        return data & FROM_MASK;
    }

    [[nodiscard]]
    constexpr Square to() const {
        return (data & TO_MASK) >> 6;
    }

    [[nodiscard]]
    constexpr bool is_quiet() const { return flags() == QUIET; }

    [[nodiscard]]
    constexpr bool is_double_pawn() const { return flags() == DOUBLE_PAWN; }

    [[nodiscard]]
    constexpr bool is_castle() const { return flags() == CASTLE; }

    [[nodiscard]]
    constexpr bool is_capture() const { return flags() & CAPTURE_FLAG; }

    [[nodiscard]]
    constexpr bool is_en_passant() const { return flags() == EN_PASSANT; }

    [[nodiscard]]
    constexpr bool is_promotion() const { return flags() & PROMOTION_FLAG; }

    [[nodiscard]]
    constexpr PieceType promotion_piece() const {
        return 1 + ((flags() & PROMOTION_PIECE) >> 12);
    }

    [[nodiscard]]
    std::string coordinate_notation() const {
        std::string result = square_name(from()) + square_name(to());
        if (is_promotion()) {
            switch (promotion_piece()) {
                case KNIGHT: result += 'n'; break;
                case BISHOP: result += 'b'; break;
                case ROOK:   result += 'r'; break;
                case QUEEN:  result += 'q'; break;
            }
        }
        return result;
    }

    [[nodiscard]]
    std::string algebraic_notation(Board const& b) const;

    static Move from_coordinate_notation(std::string_view notation, Board const& b);
    static Move from_algebraic_notation(std::string_view notation, Board const& b);
};

#endif // CHESSPP_MOVE_H
