//
// Created by tludwig on 09.09.26.
//
#include "Board.h"

#include <iostream>
#include <ostream>

#include <ranges>

#include "../precompute/zobrist.h"


Piece Board::remove_piece(const Square square) {
    const BitBoard bb = bit(square);

    const Piece to_remove = pieces[square];
    pieces[square] = Piece();
    if (to_remove.color() != EMPTY) {
        bitboards[to_remove.color()][to_remove.type()] &= ~bb;
        occupied[to_remove.color()] &= ~bb;
        occupied[BOTH] &= ~bb;
        zhash_stack.back() ^= zobrist.piece_pos[to_remove.color()][to_remove.type()][square];
    }

    return to_remove;
}


void Board::put_piece(const Square square, const Piece piece) {
    const BitBoard bb = bit(square);

    pieces[square] = piece;
    bitboards[piece.color()][piece.type()] |= bb;
    occupied[piece.color()] |= bb;
    occupied[BOTH] |= bb;
    zhash_stack.back() ^= zobrist.piece_pos[piece.color()][piece.type()][square];
}

Piece Board::move_piece(const Square from, const Square to) {
    const Piece captured = remove_piece(to);
    put_piece(to, remove_piece(from));
    return captured;
}

void Board::make_move(const Move move) {
    Color us = to_move;
    Color them = 1 - us;

    zhash_stack.push_back(zhash_stack.back());
    uint64_t& zhash = zhash_stack.back();

    const Piece moved = pieces[move.from()];
    const Piece captured = move_piece(move.from(), move.to());

    // update additional state
    state_stack.push_back(state_stack.back());
    State& state = state_stack.back();
    state.captured = captured;
    if (state.en_passant_square != NONE) {
        zhash ^= zobrist.en_passant[state.en_passant_square % 8];
        state.en_passant_square = NONE;
    }

    // update fifty move rule counter
    if (moved.type() == PAWN || move.is_capture()) {
        state.fifty_move_counter = 0;
    } else {
        state.fifty_move_counter++;
    }

    // update castling
    zhash ^= zobrist.castling_rights[state.castling_ability];
    if (move.from() == A1 || move.to() == A1 || move.from() == E1)
        state.castling_ability &= ~CASTLING_WHITE_QUEEN;
    if (move.from() == H1 || move.to() == H1 || move.from() == E1)
        state.castling_ability &= ~CASTLING_WHITE_KING;
    if (move.from() == A8 || move.to() == A8 || move.from() == E8)
        state.castling_ability &= ~CASTLING_BLACK_QUEEN;
    if (move.from() == H8 || move.to() == H8 || move.from() == E8)
        state.castling_ability &= ~CASTLING_BLACK_KING;
    zhash ^= zobrist.castling_rights[state.castling_ability];

    // update side to move and move counter
    to_move = them;
    zhash ^= zobrist.black_to_move;
    if (them == WHITE) total_move_counter++;

    // special moves
    if (move.is_double_pawn()) {
        state.en_passant_square = us == WHITE ?  move.to() - 8 : move.to() + 8;
        zhash ^= zobrist.en_passant[move.to() % 8];
    } else if (move.is_en_passant()) {
        state.captured = remove_piece(us == WHITE ?  move.to() - 8 : move.to() + 8);
    } else if (move.is_castle()) {
        const uint8_t to_file = move.to() % 8;
        const Square rook_from = to_file == 2 ? move.to() - 2 : move.to() + 1;
        const Square rook_to = to_file == 2 ? move.to() + 1 : move.to() - 1;
        move_piece(rook_from, rook_to);
    } else if (move.is_promotion()) {
        remove_piece(move.to());
        put_piece(move.to(), Piece(us, move.promotion_piece()));
    }

    assert(zhash == compute_zobrist_hash());
}

void Board::unmake_move(const Move move) {
    Color them = to_move;
    Color us = 1 - them;

    to_move = us;
    if (them == WHITE) total_move_counter--;

    State const& state = state_stack.back();

    Piece captured = state.captured;
    Square captureSquare = move.to();
    Piece moved = remove_piece(move.to());


    if (move.is_promotion()) {
        moved = Piece(us, PAWN);
    } else if (move.is_en_passant()) {
        captureSquare = us == WHITE ? captureSquare - 8 : captureSquare + 8;
    } else if (move.is_castle()) {
        const uint8_t to_file = move.to() % 8;
        const Square rook_from = to_file == 2 ? move.to() - 2 : move.to() + 1;
        const Square rook_to = to_file == 2 ? move.to() + 1 : move.to() - 1;
        move_piece(rook_to, rook_from);
    }

    if (move.is_capture()) put_piece(captureSquare, captured);
    put_piece(move.from(), moved);


    state_stack.pop_back();
    zhash_stack.pop_back();

    assert(zhash_stack.back() == compute_zobrist_hash());
}

Board Board::setup_from_fen(std::span<const std::string> const& fen) {
    Board board;
    board.state_stack.push_back({
        .castling_ability = 0,
        .en_passant_square = NONE,
        .fifty_move_counter = 0,
        .captured = Piece()
    });
    board.zhash_stack.push_back(0);
    uint64_t& zhash = board.zhash_stack.back();
    State& state = board.state_stack.back();

    std::fill_n(board.pieces, 64, Piece());
    std::fill_n(board.bitboards[WHITE], 6, 0);
    std::fill_n(board.bitboards[BLACK], 6, 0);
    std::fill_n(board.occupied, 3, 0);

    int rank = 7;
    int file = 0;
    for (char c : fen[0]) {
        if (c == '/') {
            rank--;
            file = 0;
        } else if (std::isdigit(c)) {
            file += c - '0';
        } else {
            Color color = std::isupper(c) ? WHITE : BLACK;
            PieceType type;
            switch (std::tolower(c)) {
                case 'p': type = PAWN; break;
                case 'n': type = KNIGHT; break;
                case 'b': type = BISHOP; break;
                case 'r': type = ROOK; break;
                case 'q': type = QUEEN; break;
                case 'k': type = KING; break;
                default: throw std::invalid_argument("Invalid FEN character");
            }
            Square square = rank * 8 + file;
            board.put_piece(square, Piece(color, type));
            file++;
        }
    }

    board.to_move = fen[1] == "w" ? WHITE : BLACK;
    if (board.to_move == BLACK) zhash ^= zobrist.black_to_move;

    std::string const& castling_ability = fen[2];
    if (castling_ability.contains('K')) state.castling_ability |= CASTLING_WHITE_KING;
    if (castling_ability.contains('Q')) state.castling_ability |= CASTLING_WHITE_QUEEN;
    if (castling_ability.contains('k')) state.castling_ability |= CASTLING_BLACK_KING;
    if (castling_ability.contains('q')) state.castling_ability |= CASTLING_BLACK_QUEEN;

    zhash ^= zobrist.castling_rights[state.castling_ability];

    std::string const& en_passant_square = fen[3];
    if (en_passant_square != "-") {
        const char file = en_passant_square[0];
        const char rank = en_passant_square[1];
        state.en_passant_square = (rank - '1') * 8 + (file - 'a');
        zhash ^= zobrist.en_passant[state.en_passant_square % 8];
    }

    state.fifty_move_counter = std::stoi(fen[4]);
    board.total_move_counter = std::stoi(fen[5]);

    assert(zhash == board.compute_zobrist_hash());

    return board;
}

bool Board::is_insufficient_material() const {
    if (bitboards[WHITE][PAWN] | bitboards[BLACK][PAWN]
        | bitboards[WHITE][ROOK] | bitboards[BLACK][ROOK]
        | bitboards[WHITE][QUEEN] | bitboards[BLACK][QUEEN]) {
        return false;
    }

    if (std::popcount(bitboards[WHITE][BISHOP] | bitboards[WHITE][KNIGHT]) > 1) return false;
    if (std::popcount(bitboards[BLACK][BISHOP] | bitboards[BLACK][KNIGHT]) > 1) return false;

    if (bitboards[WHITE][KNIGHT] && bitboards[BLACK][KNIGHT]) return false;
    if (bitboards[WHITE][KNIGHT] && bitboards[BLACK][BISHOP]) return false;
    if (bitboards[WHITE][BISHOP] && bitboards[BLACK][KNIGHT]) return false;
    if (bitboards[WHITE][BISHOP] && bitboards[BLACK][BISHOP]) {
        Square white_bishop_square = get_lsb(bitboards[WHITE][BISHOP]);
        Square black_bishop_square = get_lsb(bitboards[BLACK][BISHOP]);
        bool white_bishop_color = (white_bishop_square / 8 + white_bishop_square % 8) % 2;
        bool black_bishop_color = (black_bishop_square / 8 + black_bishop_square % 8) % 2;
        if (white_bishop_color != black_bishop_color) return false;
    }

    return true;
}

int Board::repetition_count() const {
    int count = 1;
    uint64_t zhash = zhash_stack.back();
    for (int i = 4; i <= state_stack.back().fifty_move_counter && i < zhash_stack.size(); i += 2) {
        if (zhash_stack[zhash_stack.size() - 1 - i] == zhash) {
            count++;
        }
    }
    return count;
}

bool Board::is_draw() const {
    if (state_stack.back().fifty_move_counter >= 100) return true;
    if (repetition_count() >= 2) return true;
    if (is_insufficient_material()) return true;
    return false;
}

uint64_t Board::compute_zobrist_hash() const {
    uint64_t hash = 0;
    for (int square = 0; square < 64; square++) {
        const Piece piece = pieces[square];
        if (piece.color() != EMPTY) {
            hash ^= zobrist.piece_pos[piece.color()][piece.type()][square];
        }
    }

    State const& state = state_stack.back();
    if (to_move == BLACK) hash ^= zobrist.black_to_move;
    hash ^= zobrist.castling_rights[state.castling_ability];
    if (state.en_passant_square != NONE) hash ^= zobrist.en_passant[state.en_passant_square % 8];

    return hash;
}

void Board::print(Color perspective) const {
    std::cout << "┏━━━┯━━━┯━━━┯━━━┯━━━┯━━━┯━━━┯━━━┓" << std::endl;

    if (perspective == BOTH) perspective = to_move;
    int rank = perspective == WHITE ? 7 : 0;
    while (true) {
        std::cout << "┃";

        for (int file = 0; ; ) {
            const Square square = rank * 8 + file;
            std::cout << ' ' << pieces[square] << ' ';

            if (++file == 8) break;

            std::cout << "│";
        }

        std::cout << "┃" << std::endl;

        if (perspective == WHITE) rank--;
        else rank++;
        if (rank < 0 || rank > 7) break;

        std::cout << "┠───┼───┼───┼───┼───┼───┼───┼───┨" << std::endl;
    }
    std::cout << "┗━━━┷━━━┷━━━┷━━━┷━━━┷━━━┷━━━┷━━━┛" << std::endl;
}
