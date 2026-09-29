cmake_minimum_required(VERSION 4.3)
project(ChessPP)

set(CMAKE_CXX_STANDARD 26)

add_executable(ChessPP main.cpp
        board/Move.h
        board/Move.cpp
        board/Board.h
        board/Board.cpp
        board/Square.h
        board/Piece.h
        board/Piece.cpp
        board/BitBoard.h
        move_gen/MoveGen.h
        move_gen/MoveGen.cpp
        move_gen/perft.h
        move_gen/perft.cpp
        search/evaluation.h
        search/evaluation.cpp
        search/search.h
        search/search.cpp
        uci/UCI.cpp
        uci/UCI.h
        search/MoveOrdering.h
        precompute/zobrist.h
        precompute/zobrist.cpp
        precompute/movement.h
        precompute/movement.cpp
        search/TranspositionTable.h
        search/MoveOrdering.cpp)
