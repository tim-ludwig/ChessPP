//
// Created by tludwig on 09.09.26.
//

#ifndef CHESSPP_BOARD_H
#define CHESSPP_BOARD_H

#include <cstdint>
#include <vector>

#include "BitBoard.h"
#include "Move.h"
#include "Square.h"


static constexpr uint8_t CASTLING_WHITE_KING  = 0b1000;
static constexpr uint8_t CASTLING_WHITE_QUEEN = 0b0100;
static constexpr uint8_t CASTLING_BLACK_KING  = 0b0010;
static constexpr uint8_t CASTLING_BLACK_QUEEN = 0b0001;

struct State {
    uint8_t castling_ability;
    Square en_passant_square;
    int fifty_move_counter;
    Piece captured;
};

struct Board {
    BitBoard bitboards[2][6];
    BitBoard occupied[3];
    Piece pieces[64];

    Color to_move;
    int total_move_counter;

    std::vector<State> state_stack;
    std::vector<uint64_t> zhash_stack;

    static Board setup_from_fen(std::span<const std::string> const& fen);

    void make_move(Move move);
    void unmake_move(Move move);

    void print(Color perspective=WHITE) const;

    int repetiton_count() const;
    bool is_draw() const;

private:
    Piece remove_piece(Square square);
    // only use on empty target squares
    void put_piece(Square square, Piece piece);
    Piece move_piece(Square from, Square to);

    uint64_t compute_zobrist_hash() const;
    bool is_insufficient_material() const;
};

#endif //CHESSPP_BOARD_H
