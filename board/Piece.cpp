//
// Created by tludwig on 10.09.26.
//
#include "Piece.h"

std::ostream& operator<<(std::ostream& os, Piece const& piece) {
    if (piece.color() == WHITE) {
        switch (piece.type()) {
            case PAWN: os << "♙"; break;
            case KNIGHT: os << "♘"; break;
            case BISHOP: os << "♗"; break;
            case ROOK: os << "♖"; break;
            case QUEEN: os << "♕"; break;
            case KING: os << "♔"; break;
        }
    } else if (piece.color() == BLACK) {
        switch (piece.type()) {
            case PAWN: os << "♟"; break;
            case KNIGHT: os << "♞"; break;
            case BISHOP: os << "♝"; break;
            case ROOK: os << "♜"; break;
            case QUEEN: os << "♛"; break;
            case KING: os << "♚"; break;
        }
    } else {
        os << ' ';
    }
    return os;
}
