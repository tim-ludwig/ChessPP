//
// Created by tludwig on 17.09.26.
//

#include "UCI.h"

#include <climits>
#include <map>
#include <sstream>
#include <thread>
#include <vector>

#include "../board/Board.h"
#include "../move_gen/MoveGen.h"
#include "../move_gen/perft.h"
#include "../search/search.h"


void split(std::string const& s, std::vector<std::string>& tokens) {
    tokens.clear();

    std::istringstream ss(s);
    std::string token;

    while (ss >> token) tokens.push_back(token);
}

const std::string start_pos[] = {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR", "w", "KQkq", "-", "0", "1"};

void UCI::handle_position(std::vector<std::string> const& tokens) {
    bool from_startpos = tokens[1] == "startpos";
    size_t next_token;
    if (from_startpos) {
        board = Board::setup_from_fen(start_pos);
        next_token = 2;
    } else {
        board = Board::setup_from_fen(std::span(tokens.begin() + 2, tokens.begin() + 8));
        next_token = 8;
    }

    if (tokens.size() > next_token && tokens[next_token] == "moves") {
        for (size_t i = next_token + 1; i < tokens.size(); i++) {
            MoveList moves = legal_moves(board);

            Move m = Move::from_coordinate_notation(tokens[i], board);
            bool found = false;
            for (int i = 0; i < moves.size(); i++) {
                if (moves[i] == m) {
                    found = true;
                    break;
                }
            }
            if (!found) throw std::logic_error("Illegal move " + tokens[i]);

            board.make_move(m);
        }
    }
}

void UCI::handle_go(std::vector<std::string> const& tokens) {
    std::map<std::string, std::string> options;
    std::span<const std::string> search_moves;

    for (int i = 1; i < tokens.size(); i++) {
        if (tokens[i] == "infinite" || tokens[i] == "ponder") {
            options[tokens[i]] = "true";
            continue;
        }
        if (tokens[i] == "searchmoves") {
            options[tokens[i]] = "true";
            search_moves = std::span(tokens.begin() + i + 1, tokens.end());
            break;
        }

        options[tokens[i]] = tokens[i + 1];
        i++;
    }

    if (options.contains("perft")) {
        int depth = std::stoi(options["perft"]);
        worker = std::jthread([this, depth](std::stop_token const& stop) {
            perft(board, depth, stop);
        });
        return;
    }

    if (options.contains("depth")) {
        search.options.max_depth = std::stoi(options["depth"]);
    } else {
        search.options.max_depth = std::nullopt;
    }

    std::string time_option = board.to_move == WHITE ? "wtime" : "btime";
    std::string inc_option = board.to_move == WHITE ? "winc" : "binc";
    if (options.contains("movetime")) {
        int movetime = std::stoi(options["movetime"]);
        search.options.time_budget = std::chrono::milliseconds(movetime - 100);
    } else if (options.contains(time_option)) {
        int time = std::stoi(options[time_option]);
        int inc = options.contains(inc_option) ? std::stoi(options[inc_option]) : 0;
        int movestogo = options.contains("movestogo") ? std::stoi(options["movestogo"]) : 20;
        if (movestogo == 0) movestogo = 1;
        int budget = time / movestogo + inc / 2;
        budget = std::min(budget, time - 100);
        search.options.time_budget = std::chrono::milliseconds(budget);
    } else {
        search.options.time_budget = std::nullopt;
    }

    if (options.contains("ponder")) {
        search.options.start_time = std::nullopt;
        search.options.pondering = true;
    } else {
        search.options.start_time = std::chrono::steady_clock::now();
        search.options.pondering = false;
    }

    worker = std::jthread([this](std::stop_token const& stop) {
        Search::Result result = search.run(stop);

        if (result.score == INF || result.score == -INF) {
            // silently ignore this, as it means the search was stopped before a meaningful score was found
        } else if (result.score > MATE_THRESHOLD || result.score < -MATE_THRESHOLD) {
            std::cout << "info score mate " << score_to_mate_moves(result.score) << std::endl;
        } else {
            std::cout << "info score cp " << result.score << std::endl;
        }

        std::cout << "bestmove " << result.pv[0].coordinate_notation();
        if (result.pv.size() > 1) {
            std::cout << " ponder " << result.pv[1].coordinate_notation();
        }
        std::cout << std::endl;
    });
}

void UCI::handle_debug(const std::vector<std::string>& tokens) {
    if (tokens[1] == "print") {
        board.print();
    } else if (tokens[1] == "zhash") {
        for (int i = 0; i < board.zhash_stack.size(); i++) {
            std::cout << std::hex << board.zhash_stack[i] << std::dec << std::endl;
        }
        std::cout << "repetition count: " << board.repetition_count() << std::endl;
    }
}

void UCI::repl() {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::vector<std::string> tokens;
        split(line, tokens);

        if (tokens.empty()) continue;
        if (tokens[0] == "quit") break;

        if (tokens[0] == "uci") {
            std::cout << "id name Chess++\n";
            std::cout << "id author Tim Ludwig\n";
            std::cout << "uciok" << std::endl;
        } else if (tokens[0] == "isready") {
            std::cout << "readyok" << std::endl;
        } else if (tokens[0] == "position") {
            if (worker.joinable()) {
                worker.request_stop();
                worker.join();
            }
            handle_position(tokens);
        } else if (tokens[0] == "go") {
            if (worker.joinable()) {
                worker.request_stop();
                worker.join();
            }
            handle_go(tokens);
        } else if (tokens[0] == "ponderhit") {
            search.options.start_time = std::chrono::steady_clock::now();
            search.options.pondering = false;
        } else if (tokens[0] == "stop") {
            worker.request_stop();
            if (worker.joinable()) {
                worker.join();
            }
        } else if (tokens[0] == "debug") {
            handle_debug(tokens);
        }
    }

    if (worker.joinable()) {
        worker.request_stop();
        worker.join();
    }
}
