//
// Created by tludwig on 10.09.26.
//

#include "MoveGen.h"


#include "../precompute/movement.h"

BitBoard pawn_captures(BitBoard pawns, Color by) {
    return by == WHITE ? (pawns & ~FILE_A) << 7 | (pawns & ~FILE_H) << 9
                       : (pawns & ~FILE_A) >> 9 | (pawns & ~FILE_H) >> 7;
}

void gen_legality_info(Board const& b, LegalityInfo& legality) {
    Color us = b.to_move;
    Color them = 1 - us;

    // attacked and king danger squares
    BitBoard attacked = pawn_captures(b.bitboards[them][PAWN], them);

    BitBoard knights = b.bitboards[them][KNIGHT];
    while (knights) attacked |= knight_table[pop_lsb(knights)];

    attacked |= king_table[get_lsb(b.bitboards[them][KING])];

    BitBoard king_danger = attacked;
    BitBoard occupied = b.occupied[BOTH];
    // mask out our king from occupation, so he doesn't block a check along the way he moves
    BitBoard occupied_without_king = occupied & ~b.bitboards[us][KING];

    BitBoard bishop_like = b.bitboards[them][BISHOP] | b.bitboards[them][QUEEN];
    while (bishop_like) {
        Square from = pop_lsb(bishop_like);
        attacked |= bishop_lookup(from, occupied);
        king_danger |= bishop_lookup(from, occupied_without_king);
    }

    BitBoard rook_like = b.bitboards[them][ROOK] | b.bitboards[them][QUEEN];
    while (rook_like) {
        Square from = pop_lsb(rook_like);
        attacked |= rook_lookup(from, occupied);
        king_danger |= rook_lookup(from, occupied_without_king);
    }

    legality.attacked = attacked;
    legality.king_danger = king_danger;

    // checkers
    BitBoard checkers = 0;
    BitBoard our_king = b.bitboards[us][KING];
    Square king_square = get_lsb(our_king);

    checkers |= pawn_captures(our_king, us) & b.bitboards[them][PAWN];
    checkers |= knight_table[king_square] & b.bitboards[them][KNIGHT];
    checkers |= bishop_lookup(king_square, occupied) & (b.bitboards[them][BISHOP] | b.bitboards[them][QUEEN]);
    checkers |= rook_lookup(king_square, occupied) & (b.bitboards[them][ROOK] | b.bitboards[them][QUEEN]);

    int n_checkers = std::popcount(checkers);
    if (n_checkers == 0) {
        legality.capture_mask = ~0ull;
        legality.push_mask    = ~0ull;
    } else if (std::popcount(checkers) == 1) {
        legality.capture_mask = checkers;
        if (checkers & (b.bitboards[them][BISHOP] | b.bitboards[them][ROOK] | b.bitboards[them][QUEEN])) {
            legality.push_mask = between[king_square][get_lsb(checkers)];
        } else {
            legality.push_mask = 0;
        }
    } else {
        legality.capture_mask = 0ull;
        legality.push_mask    = 0ull;
    }

    legality.checkers = checkers;

    // pins (by xray and intersection with "between")
    BitBoard rook_pinners = rook_lookup(king_square, b.occupied[them]) & (b.bitboards[them][ROOK] | b.bitboards[them][QUEEN]);
    BitBoard bishop_pinners = bishop_lookup(king_square, b.occupied[them]) & (b.bitboards[them][BISHOP] | b.bitboards[them][QUEEN]);
    BitBoard pinners = rook_pinners | bishop_pinners;
    legality.pinned = 0;
    while (pinners) {
        Square pinner = pop_lsb(pinners);
        BitBoard blocker = b.occupied[us] & between[king_square][pinner];
        if (std::popcount(blocker) == 1) {
            legality.pinned |= blocker;
            legality.pin_rays[get_lsb(blocker)] = between[king_square][pinner] | bit(pinner);
        }
    }
}

static void add_pawn_move(MoveList& moves, Square from, Square to, bool capture, bool quiescence) {
    int rank = to / 8;
    if (rank == 7 || rank == 0) {
        // promotion
        moves.push_back(Move::promote(from, to, QUEEN, capture));
        moves.push_back(Move::promote(from, to, ROOK, capture));
        moves.push_back(Move::promote(from, to, BISHOP, capture));
        moves.push_back(Move::promote(from, to, KNIGHT, capture));
    } else {
        // no promotion
        if (capture) {
            moves.push_back(Move::capture(from, to));
        } else if (!quiescence) {
            moves.push_back(Move::quiet(from, to));
        }
    }
}

static void add_pawn_moves(Board const& board, MoveList& moves, bool quiescence) {
    LegalityInfo const& legality_info = moves.legality();
    Color us = board.to_move;
    Color them = 1 - us;

    BitBoard unpinned_pawns = board.bitboards[us][PAWN] & ~legality_info.pinned;
    BitBoard pinned_pawns = board.bitboards[us][PAWN] & legality_info.pinned;

    BitBoard capture_targets = board.occupied[them];
    BitBoard occupied = board.occupied[BOTH];

    // single moves
    BitBoard single = us == WHITE ? unpinned_pawns << 8 : unpinned_pawns >> 8;
    single &= ~occupied;

    // double moves
    BitBoard dbl = us == WHITE ? (single & RANK_3) << 8 : (single & RANK_6) >> 8;
    dbl &= ~occupied;

    single &= legality_info.push_mask;
    dbl &= legality_info.push_mask;

    while (single) {
        Square to = pop_lsb(single);
        Square from = us == WHITE ? to - 8 : to + 8;
        add_pawn_move(moves, from, to, false, quiescence);
    }

    if (!quiescence) {
        while (dbl) {
            Square to = pop_lsb(dbl);
            Square from = us == WHITE ? to - 16 : to + 16;
            moves.push_back(Move::double_pawn(from, to));
        }
    }

    // capture left
    BitBoard capture_left = us == WHITE ? (unpinned_pawns & ~FILE_A) << 7 : (unpinned_pawns & ~FILE_A) >> 9;
    capture_left &= capture_targets;
    capture_left &= legality_info.capture_mask;

    // capture right
    BitBoard capture_right = us == WHITE ? (unpinned_pawns & ~FILE_H) << 9 : (unpinned_pawns & ~FILE_H) >> 7;
    capture_right &= capture_targets;
    capture_right &= legality_info.capture_mask;

    while (capture_left) {
        Square to = pop_lsb(capture_left);
        Square from = us == WHITE ? to - 7 : to + 9;
        add_pawn_move(moves, from, to, true, quiescence);
    }

    while (capture_right) {
        Square to = pop_lsb(capture_right);
        Square from = us == WHITE ? to - 9 : to + 7;
        add_pawn_move(moves, from, to, true, quiescence);
    }

    while (pinned_pawns) {
        Square pinned_square = pop_lsb(pinned_pawns);
        BitBoard pinned_pawn = bit(pinned_square);

        // single moves
        BitBoard single = us == WHITE ? pinned_pawn << 8 : pinned_pawn >> 8;
        single &= ~occupied & legality_info.pin_rays[pinned_square];


        // double moves
        BitBoard dbl = us == WHITE ? (single & RANK_3) << 8 : (single & RANK_6) >> 8;
        dbl &= ~occupied & legality_info.pin_rays[pinned_square];

        single &= legality_info.push_mask;
        dbl &= legality_info.push_mask;

        if (single) add_pawn_move(moves, pinned_square, get_lsb(single), false, quiescence);
        if (dbl) moves.push_back(Move::double_pawn(pinned_square, get_lsb(dbl)));

        // capture left
        BitBoard capture_left = us == WHITE ? (pinned_pawn & ~FILE_A) << 7 : (pinned_pawn & ~FILE_A) >> 9;
        capture_left &= capture_targets & legality_info.capture_mask & legality_info.pin_rays[pinned_square];
        if (capture_left) add_pawn_move(moves, pinned_square, get_lsb(capture_left), true, quiescence);

        // capture right
        BitBoard capture_right = us == WHITE ? (pinned_pawn & ~FILE_H) << 9 : (pinned_pawn & ~FILE_H) >> 7;
        capture_right &= capture_targets & legality_info.capture_mask & legality_info.pin_rays[pinned_square];
        if (capture_right) add_pawn_move(moves, pinned_square, get_lsb(capture_right), true, quiescence);
    }

    // en passant
    Square ep_square = board.state_stack.back().en_passant_square;
    if (ep_square != NONE) {
        Square captured = ep_square + (us == WHITE ? -8 : 8);
        if (bit(captured) & legality_info.capture_mask || bit(ep_square) & legality_info.push_mask) {
            int target_file = ep_square % 8;
            pinned_pawns = board.bitboards[us][PAWN] & legality_info.pinned;

            // capture left
            if (target_file != 7) {
                Square from = us == WHITE ? ep_square - 7 : ep_square + 9;
                BitBoard discovered_attackers = rook_lookup(get_lsb(board.bitboards[us][KING]), board.occupied[BOTH] & ~bit(from) & ~bit(captured) | bit(ep_square));
                if (!(discovered_attackers & (board.bitboards[them][ROOK] | board.bitboards[them][QUEEN]))) {
                    if (unpinned_pawns & bit(from) || pinned_pawns & bit(from) && bit(ep_square) & legality_info.pin_rays[from]) {
                        moves.push_back(Move::en_passant(from, ep_square));
                    }
                }
            }

            // capture right
            if (ep_square % 8 != 0) {
                Square from = us == WHITE ? ep_square - 9 : ep_square + 7;
                BitBoard discovered_attackers = rook_lookup(get_lsb(board.bitboards[us][KING]), board.occupied[BOTH] & ~bit(from) & ~bit(captured) | bit(ep_square));
                if (!(discovered_attackers & (board.bitboards[them][ROOK] | board.bitboards[them][QUEEN]))) {
                    if (unpinned_pawns & bit(from) || pinned_pawns & bit(from) && bit(ep_square) & legality_info.pin_rays[from]) {
                        moves.push_back(Move::en_passant(from, ep_square));
                    }
                }
            }
        }
    }
}

static void add_knight_moves(Board const& board, MoveList& moves, bool quiescence) {
    LegalityInfo const& legality_info = moves.legality();
    Color us = board.to_move;
    Color them = 1 - us;

    BitBoard knights = board.bitboards[us][KNIGHT] & ~legality_info.pinned;

    while (knights) {
        Square from = pop_lsb(knights);
        BitBoard candidates = knight_table[from];

        BitBoard capture = candidates & board.occupied[them] & legality_info.capture_mask;
        while (capture) moves.push_back(Move::capture(from, pop_lsb(capture)));

        if (quiescence) continue;

        BitBoard quiet = candidates & ~board.occupied[BOTH] & legality_info.push_mask;
        while (quiet) moves.push_back(Move::quiet(from, pop_lsb(quiet)));
    }
}

static void add_king_moves(Board const& board, MoveList& moves, bool quiescence) {
    LegalityInfo const& legality_info = moves.legality();
    Color us = board.to_move;
    Color them = 1 - us;

    Square from = get_lsb(board.bitboards[us][KING]);
    BitBoard candidates = king_table[from] & ~legality_info.king_danger;

    BitBoard capture = candidates & board.occupied[them];
    while (capture) moves.push_back(Move::capture(from, pop_lsb(capture)));

    if (quiescence) return;
    BitBoard occupied = board.occupied[BOTH];
    BitBoard quiet = candidates & ~occupied;
    while (quiet) moves.push_back(Move::quiet(from, pop_lsb(quiet)));

    // castling
    uint8_t castling_ability = board.state_stack.back().castling_ability;
    if (us == WHITE) {
        if (castling_ability & CASTLING_WHITE_KING) {
            if (!(occupied & (bit(F1) | bit(G1))) &&
                !(legality_info.attacked & (bit(E1) | bit(F1) | bit(G1)))) {
                moves.push_back(Move::castle(E1, G1));
            }
        }
        if (castling_ability & CASTLING_WHITE_QUEEN) {
            if (!(occupied & (bit(B1) | bit(C1) | bit(D1))) &&
                !(legality_info.attacked & (bit(C1) | bit(D1) | bit(E1)))) {
                moves.push_back(Move::castle(E1, C1));
            }
        }
    } else {
        if (castling_ability & CASTLING_BLACK_KING) {
            if (!(occupied & (bit(F8) | bit(G8))) &&
                !(legality_info.attacked & (bit(E8) | bit(F8) | bit(G8)))) {
                moves.push_back(Move::castle(E8, G8));
            }
        }
        if (castling_ability & CASTLING_BLACK_QUEEN) {
            if (!(occupied & (bit(B8) | bit(C8) | bit(D8))) &&
                !(legality_info.attacked & (bit(C8) | bit(D8) | bit(E8)))) {
                moves.push_back(Move::castle(E8, C8));
            }
        }
    }
}

static void add_rook_moves(Board const& board, MoveList& moves, bool quiescence) {
    LegalityInfo const& legality_info = moves.legality();
    Color us = board.to_move;
    Color them = 1 - us;

    BitBoard rooks = board.bitboards[us][ROOK];
    while (rooks) {
        Square from = pop_lsb(rooks);
        BitBoard candidates = rook_lookup(from, board.occupied[BOTH]);

        if (bit(from) & legality_info.pinned) candidates &= legality_info.pin_rays[from];

        BitBoard capture = candidates & board.occupied[them] & legality_info.capture_mask;
        while (capture) moves.push_back(Move::capture(from, pop_lsb(capture)));

        if (quiescence) continue;

        BitBoard quiet = candidates & ~board.occupied[BOTH] & legality_info.push_mask;
        while (quiet) moves.push_back(Move::quiet(from, pop_lsb(quiet)));
    }
}

static void add_bishop_moves(Board const& board, MoveList& moves, bool quiescence) {
    LegalityInfo const& legality_info = moves.legality();
    Color us = board.to_move;
    Color them = 1 - us;

    BitBoard bishops = board.bitboards[us][BISHOP];
    while (bishops) {
        Square from = pop_lsb(bishops);
        BitBoard candidates = bishop_lookup(from, board.occupied[BOTH]);

        if (bit(from) & legality_info.pinned) candidates &= legality_info.pin_rays[from];

        BitBoard capture = candidates & board.occupied[them] & legality_info.capture_mask;
        while (capture) moves.push_back(Move::capture(from, pop_lsb(capture)));

        if (quiescence) continue;

        BitBoard quiet = candidates & ~board.occupied[BOTH] & legality_info.push_mask;
        while (quiet) moves.push_back(Move::quiet(from, pop_lsb(quiet)));
    }
}

static void add_queen_moves(Board const& board, MoveList& moves, bool quiescence) {
    LegalityInfo const& legality_info = moves.legality();
    Color us = board.to_move;
    Color them = 1 - us;

    BitBoard queens = board.bitboards[us][QUEEN];
    while (queens) {
        Square from = pop_lsb(queens);
        BitBoard candidates = rook_lookup(from, board.occupied[BOTH])
                            | bishop_lookup(from, board.occupied[BOTH]);

        if (bit(from) & legality_info.pinned) candidates &= legality_info.pin_rays[from];

        BitBoard capture = candidates & board.occupied[them] & legality_info.capture_mask;
        while (capture) moves.push_back(Move::capture(from, pop_lsb(capture)));

        if (quiescence) continue;

        BitBoard quiet = candidates & ~board.occupied[BOTH] & legality_info.push_mask;
        while (quiet) moves.push_back(Move::quiet(from, pop_lsb(quiet)));
    }
}

MoveList legal_moves(Board const& board, bool quiescence) {
    MoveList moves;
    gen_legality_info(board, moves.legality());

    int num_checkers = std::popcount(moves.legality().checkers);

    if (num_checkers <= 1) {
        add_pawn_moves(board, moves, quiescence);
        add_knight_moves(board, moves, quiescence);
        add_bishop_moves(board, moves, quiescence);
        add_rook_moves(board, moves, quiescence);
        add_queen_moves(board, moves, quiescence);
    }
    add_king_moves(board, moves, quiescence);

    return moves;
}
