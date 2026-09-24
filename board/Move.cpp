//
// Created by tludwig on 13.09.26.
//
#include "Move.h"

#include "Board.h"
#include "../move_gen/MoveGen.h"


std::string Move::algebraic_notation(Board const& b) const {
    Piece moved = b.pieces[from()];
    BitBoard conflicts = 0;

    std::string result;
    switch (moved.type()) {
        case PAWN:
            if (is_capture()) {
                conflicts = pawn_captures(bit(to()), 1 - b.to_move);
                result += square_name(from());
            }
            break;

        case KING:
            if (is_castle()) {
                if (to() % 8 == 6) return "0-0";
                return "0-0-0";
            }

            result += 'K';
            break;

        case KNIGHT: result += 'N'; conflicts = knight_table[to()]; break;
        case BISHOP: result += 'B'; conflicts = bishop_lookup(to(), b.occupied[BOTH]); break;
        case ROOK:   result += 'R'; conflicts = rook_lookup(to(), b.occupied[BOTH]); break;
        case QUEEN:  result += 'Q'; conflicts = bishop_lookup(to(), b.occupied[BOTH]) | rook_lookup(to(), b.occupied[BOTH]); break;
    }

    conflicts &= b.bitboards[moved.color()][moved.type()];
    conflicts &= ~bit(from());
    if (conflicts) {
        int rank = from() / 8;
        int file = from() % 8;
        // disambiguate using file, then rank, then square
        if (!(conflicts & FILES[file])) {
            result += static_cast<char>('a' + file);
        } else if (!(conflicts & RANKS[rank])) {
            result += static_cast<char>('1' + rank);
        } else {
            result += square_name(from());
        }
    }

    if (is_capture()) result += 'x';
    result += square_name(to());

    if (is_promotion()) {
        switch (promotion_piece()) {
            case KNIGHT: result += "=N"; break;
            case BISHOP: result += "=B"; break;
            case ROOK:   result += "=R"; break;
            case QUEEN:  result += "=Q"; break;
        }
    }
    return result;
}

Move Move::from_coordinate_notation(std::string_view notation, Board const& b) {
    Square from = (notation[0] - 'a') + (notation[1] - '1') * 8;
    Square to = (notation[2] - 'a') + (notation[3] - '1') * 8;

    Piece moved = b.pieces[from];
    Piece captured = b.pieces[to];
    bool is_promotion = notation.length() == 5;

    if (moved.type() == KING) {
        int from_file = from % 8;
        int to_file = to % 8;
        if (from_file == 4 && (to_file == 6 || to_file == 2)) {
            return Move::castle(from, to);
        }
    } else if (moved.type() == PAWN) {
        if (to == b.state_stack.back().en_passant_square) {
            return Move::en_passant(from, to);
        } else if (is_promotion) {
            PieceType promotion_piece;
            switch (notation[4]) {
                case 'n': promotion_piece = KNIGHT; break;
                case 'b': promotion_piece = BISHOP; break;
                case 'r': promotion_piece = ROOK; break;
                case 'q': promotion_piece = QUEEN; break;
                default: throw std::invalid_argument("Invalid promotion piece");
            }
            return Move::promote(from, to, promotion_piece, captured.color() != EMPTY);
        } else if (abs(from - to) == 16) {
            return Move::double_pawn(from, to);
        }
    }
    return captured.color() != EMPTY ? Move::capture(from, to) : Move::quiet(from, to);
}
