#include "MoveOrdering.h"

BitBoard attacks_to(Square to, const Board& board, BitBoard moved) {
    BitBoard attackers = 0;

    attackers |= rook_lookup(to, board.occupied[BOTH] & ~moved)
    & (board.bitboards[WHITE][ROOK] | board.bitboards[BLACK][ROOK]
        | board.bitboards[WHITE][QUEEN] | board.bitboards[BLACK][QUEEN]);

    attackers |= bishop_lookup(to, board.occupied[BOTH] & ~moved)
    & (board.bitboards[WHITE][BISHOP] | board.bitboards[BLACK][BISHOP]
        | board.bitboards[WHITE][QUEEN] | board.bitboards[BLACK][QUEEN]);

    attackers |= knight_table[to] & (board.bitboards[WHITE][KNIGHT] | board.bitboards[BLACK][KNIGHT]);

    attackers |= king_table[to] & (board.bitboards[WHITE][KING] | board.bitboards[BLACK][KING]);

    attackers |= pawn_captures(bit(to), BLACK) & board.bitboards[WHITE][PAWN];
    attackers |= pawn_captures(bit(to), WHITE) & board.bitboards[BLACK][PAWN];

    return attackers & ~moved;
}

BitBoard least_valuable_attackers(BitBoard attackers, const Board& board, Color to_move) {
    for (int piece_type = PAWN; piece_type <= KING; piece_type++) {
        BitBoard lva = attackers & board.bitboards[to_move][piece_type];
        if (lva) return lva;
    }
    return 0;
}

Score MoveOrdering::see(Move move) {
    Square sq = move.to();

    Score gain[32];
    Score trophy = 0;
    gain[0] = 0;

    BitBoard moved = bit(move.from());
    if (move.is_en_passant()) {
        gain[0] += PIECE_VALUE[PAWN];
        moved |= bit(board.to_move == WHITE ? sq - 8 : sq + 8);
    } else if (move.is_capture()) {
        gain[0] += PIECE_VALUE[board.pieces[sq].type()];
    }
    if (move.is_promotion()) {
        trophy = PIECE_VALUE[move.promotion_piece()];
        gain[0] += trophy - PIECE_VALUE[PAWN];
    } else {
        trophy = PIECE_VALUE[board.pieces[move.from()].type()];
    }

    Color to_move = 1 - board.to_move;

    int i = 0;
    for (;;) {
        BitBoard attackers = attacks_to(sq, board, moved);
        BitBoard lva = least_valuable_attackers(attackers, board, to_move);
        if (!lva) break;
        Square from = get_lsb(lva);
        PieceType attacker = board.pieces[from].type();

        if (attacker == KING && attackers & board.occupied[1 - to_move]) break;
        i++;
        gain[i] = trophy;
        if (attacker == PAWN && (sq < 8 || sq > 55)) {
            gain[i] += PIECE_VALUE[QUEEN] - PIECE_VALUE[PAWN];
            trophy = PIECE_VALUE[QUEEN];
        } else {
            trophy = PIECE_VALUE[attacker];
        }

        moved |= bit(from);
        to_move = 1 - to_move;
    }

    for (; i > 0; i--) {
        gain[i - 1] -= gain[i];
        if (i != 1) {
            gain[i - 1] = std::max(0, gain[i - 1]);
        }
    }

    return gain[0];
}