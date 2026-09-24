//
// Created by tludwig on 12.09.26.
//

#include "perft.h"

#include "MoveGen.h"

static size_t perft_(Board& board, int depth, std::stop_token const& stop) {
    if (depth == 0) return 1;

    MoveList moves = legal_moves(board);

    if (depth == 1) return moves.size();

    size_t nodes = 0;
    for (int i = 0; i < moves.size(); i++) {
        if (stop.stop_requested()) break;

        board.make_move(moves[i]);
        nodes += perft_(board, depth - 1, stop);
        board.unmake_move(moves[i]);
    }
    return nodes;
}

void perft(Board& board, int depth, std::stop_token const& stop) {
    MoveList moves = legal_moves(board);

    size_t nodes = 0;
    for (int i = 0; i < moves.size(); i++) {
        if (stop.stop_requested()) break;

        board.make_move(moves[i]);
        size_t local_nodes = perft_(board, depth - 1, stop);
        board.unmake_move(moves[i]);
        nodes += local_nodes;

        std::cout << moves[i].coordinate_notation() << ": " << local_nodes << std::endl;
    }
    std::cout << "\nNodes searched: " << nodes << std::endl;
}
