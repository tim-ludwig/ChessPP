//
// Created by tludwig on 21.09.26.
//

#ifndef CHESSPP_TRANSPOSITIONTABLE_H
#define CHESSPP_TRANSPOSITIONTABLE_H

#include "../board/Move.h"
#include "evaluation.h"


constexpr size_t TT_DEFAULT_SIZE = 1 << 20;

class TranspositionTable {
public:
    using Entry = struct Entry {
        uint64_t key = 0;
        Move best_move = Move::null();
        Score score = 0;
        int depth = -1;
        enum Bound { EXACT, LOWER, UPPER } bound = EXACT;
    };

    explicit TranspositionTable(std::size_t s=TT_DEFAULT_SIZE) : size(s), entries(s) {}

    Entry* lookup(uint64_t key) {
        size_t index = key % size;
        Entry& entry = entries[index];
        if (entry.key == key && entry.depth >= 0) {
            return &entry;
        }
        return nullptr;
    }

    void store(Entry const& entry) {
        std::size_t index = entry.key % size;
        if (entries[index].key == entry.key || entries[index].depth < entry.depth) {
            if (entries[index].depth < 0) used_entries++;
            entries[index] = entry;
        }
    }

    std::size_t hashfull() const {
        return used_entries * 1000 / size;
    }

private:
    std::size_t size;
    std::size_t used_entries = 0;
    std::vector<Entry> entries;
};

inline Score score_to_tt(Score score, int ply) {
    if (score > MATE_THRESHOLD)
        return score + ply;

    if (score < -MATE_THRESHOLD)
        return score - ply;

    return score;
}

inline Score score_from_tt(Score score, int ply) {
    if (score > MATE_THRESHOLD)
        return score - ply;

    if (score < -MATE_THRESHOLD)
        return score + ply;

    return score;
}


#endif //CHESSPP_TRANSPOSITIONTABLE_H
