import random

RANKS = [
    0x00000000000000FF,
    0x000000000000FF00,
    0x0000000000FF0000,
    0x00000000FF000000,
    0x000000FF00000000,
    0x0000FF0000000000,
    0x00FF000000000000,
    0xFF00000000000000,
]

FILES = [
    0x0101010101010101,
    0x0202020202020202,
    0x0404040404040404,
    0x0808080808080808,
    0x1010101010101010,
    0x2020202020202020,
    0x4040404040404040,
    0x8080808080808080,
]

MASK64 = 0xFFFFFFFFFFFFFFFF

def pop_lsb(x):
    lsb = x & -x
    # lsb.bit_length() - 1,
    return lsb,  x & (x - 1)

def gen_between():
    between = []
    for fr in range(64):
        rankf = fr  // 8
        filef = fr % 8
        l = []
        for to in range(64):
            if fr == to:
                l.append(0)
                continue
            rankt = to // 8
            filet = to % 8
            if  rankf != rankt and filef != filet and abs(rankf - rankt) != abs(filef - filet):
                l.append(0)
                continue

            step = (0 if rankf == rankt else (8 if rankf < rankt else -8)) + (0 if filef == filet else (1 if filef < filet else -1))

            mask = 0
            sq = fr + step
            while sq != to:
                mask |= 1 << sq
                sq += step
            l.append(mask)
        between.append(l)
    return between

# knight table
def gen_knight_attacks():
    knight_attacks = []
    for i in range(64):
        rank = i // 8
        file = i % 8
        move = 0
        if file < 7 and rank < 6: move |= 1 << (i + 17);
        if file < 6 and rank < 7: move |= 1 << (i + 10);
        if file < 6 and rank > 0: move |= 1 << (i - 6);
        if file < 7 and rank > 1: move |= 1 << (i - 15);
        if file > 0 and rank > 1: move |= 1 << (i - 17);
        if file > 1 and rank > 0: move |= 1 << (i - 10);
        if file > 1 and rank < 7: move |= 1 << (i + 6);
        if file > 0 and rank < 6: move |= 1 << (i + 15);
        knight_attacks.append(move)
    return knight_attacks

# king table
def gen_king_attacks():
    king_attacks = []
    for i in range(64):
        rank = i // 8
        file = i % 8
        move = 0
        if file > 0 and rank > 0: move |= 1 << (i - 9)
        if              rank > 0: move |= 1 << (i - 8)
        if file < 7 and rank > 0: move |= 1 << (i - 7)
        if file > 0:              move |= 1 << (i - 1)
        if file < 7:              move |= 1 << (i + 1)
        if file > 0 and rank < 7: move |= 1 << (i + 7)
        if              rank < 7: move |= 1 << (i + 8)
        if file < 7 and rank < 7: move |= 1 << (i + 9)
        king_attacks.append(move)
    return king_attacks


def enumerate_occupancies(mask):
    n = 1 << mask.bit_count()
    for j in range(n):
        occupancy = 0
        remaining = mask
        k = 0
        while remaining:
            lsb, remaining = pop_lsb(remaining)
            if j & (1 << k):
                occupancy |= lsb
            k += 1
        yield occupancy


def test_magic(magic, shift, attack_map):
    table_size = 1 << (64 - shift)
    table = [None] * table_size

    for occupancy, attacks in attack_map.items():
        index = ((occupancy * magic) & MASK64) >> shift
        if table[index] is not None and table[index] != attacks:
            return None
        table[index] = attacks
    return table


def gen_magic(make_mask, make_attacks):
    masks = []
    magics = []
    shifts = []
    tables = []

    for square in range(64):
        mask = make_mask(square)

        attack_map = {}
        for occupancy in enumerate_occupancies(mask):
            attack_map[occupancy] = make_attacks(occupancy, square)

        shift = 64 - mask.bit_count()
        while True:
            magic = random.getrandbits(64) & random.getrandbits(64) & random.getrandbits(64)
            table = test_magic(magic, shift, attack_map)
            if table: break

        print(square, end=', ')
        masks.append(mask)
        magics.append(magic)
        shifts.append(shift)
        tables.append(table)
    print()
    return masks, magics, shifts, tables

def gen_rook_magic():
    def make_mask(square):
        rank = square // 8
        file = square % 8

        return(
                (RANKS[rank] & ~FILES[0] & ~FILES[7])
                | (FILES[file] & ~RANKS[0] & ~RANKS[7])
        ) & ~(1 << square)
    def rook_on_the_fly(occupancy, square):
        rank = square // 8
        file = square % 8

        attacks = 0

        # South
        for r in range(rank - 1, -1, -1):
            target = 1 << (8 * r + file)
            attacks |= target
            if occupancy & target:
                break

        # North
        for r in range(rank + 1, 8):
            target = 1 << (8 * r + file)
            attacks |= target
            if occupancy & target:
                break

        # West
        for f in range(file - 1, -1, -1):
            target = 1 << (8 * rank + f)
            attacks |= target
            if occupancy & target:
                break

        # East
        for f in range(file + 1, 8):
            target = 1 << (8 * rank + f)
            attacks |= target
            if occupancy & target:
                break

        return attacks

    return gen_magic(make_mask, rook_on_the_fly)


def gen_bishop_magic():
    def make_mask(square):
        rank = square // 8
        file = square % 8

        mask = 0

        # Southwest
        r, f = rank - 1, file - 1
        while r > 0 and f > 0:
            mask |= 1 << (8 * r + f)
            r -= 1
            f -= 1

        # Northwest
        r, f = rank + 1, file - 1
        while r < 7 and f > 0:
            mask |= 1 << (8 * r + f)
            r += 1
            f -= 1

        # Southeast
        r, f = rank - 1, file + 1
        while r > 0 and f < 7:
            mask |= 1 << (8 * r + f)
            r -= 1
            f += 1

        # Northeast
        r, f = rank + 1, file + 1
        while r < 7 and f < 7:
            mask |= 1 << (8 * r + f)
            r += 1
            f += 1

        return mask
    def bishop_on_the_fly(occupancy, square):
        rank = square // 8
        file = square % 8

        attacks = 0

        # Southwest
        r, f = rank - 1, file - 1
        while r >= 0 and f >= 0:
            target = 1 << (8 * r + f)
            attacks |= target
            if occupancy & target:
                break
            r -= 1
            f -= 1

        # Northwest
        r, f = rank + 1, file - 1
        while r < 8 and f >= 0:
            target = 1 << (8 * r + f)
            attacks |= target
            if occupancy & target:
                break
            r += 1
            f -= 1

        # Southeast
        r, f = rank - 1, file + 1
        while r >= 0 and f < 8:
            target = 1 << (8 * r + f)
            attacks |= target
            if occupancy & target:
                break
            r -= 1
            f += 1

        # Northeast
        r, f = rank + 1, file + 1
        while r < 8 and f < 8:
            target = 1 << (8 * r + f)
            attacks |= target
            if occupancy & target:
                break
            r += 1
            f += 1
        return attacks
    return gen_magic(make_mask, bishop_on_the_fly)

between = gen_between()
knight_attacks = gen_knight_attacks()
king_attacks = gen_king_attacks()
rook_masks, rook_magics, rook_shifts, rook_table = gen_rook_magic()
bishop_masks, bishop_magics, bishop_shifts, bishop_table = gen_bishop_magic()

with open('movement.h', 'w') as f:
    f.write('#ifndef CHESSPP_MOVEMENT_H\n')
    f.write('#define CHESSPP_MOVEMENT_H\n')
    f.write('// This file is generated by movement.py\n\n')

    f.write('#include "../board/Square.h"\n')
    f.write('#include "../board/BitBoard.h"\n\n')

    f.write('extern const BitBoard between[64][64];\n')
    f.write('extern const BitBoard knight_table[64];\n')
    f.write('extern const BitBoard king_table[64];\n')
    f.write('BitBoard rook_lookup(Square s, BitBoard occupied);\n')
    f.write('BitBoard bishop_lookup(Square s, BitBoard occupied);\n\n')

    f.write('#endif // CHESSPP_MOVEMENT_H\n')

with open('movement.cpp', 'w') as f:
    f.write('// This file is generated by movement.py\n')
    f.write('#include "movement.h"\n\n')

    #between
    f.write('const BitBoard between[64][64] = {\n')
    for fr in range(64):
        f.write('{')
        for to in range(64):
            f.write(f'0x{between[fr][to]:x},')
        f.write('},')
    f.write('\n};\n')

    # knight
    f.write('const BitBoard knight_table[64] = {\n')
    for move in knight_attacks:
        f.write(f'0x{move:x},')
    f.write('\n};\n')

    # king
    f.write('const BitBoard king_table[64] = {\n')
    for move in king_attacks:
        f.write(f'0x{move:x},')
    f.write('\n};\n')
    f.write('struct Magic {\n')
    f.write('    BitBoard const* table;\n')
    f.write('    BitBoard mask;\n')
    f.write('    BitBoard magic;\n')
    f.write('    int shift;\n')
    f.write('};\n')

    # rook magic
    f.write('static constexpr BitBoard rook_table[] = {\n')
    for square in range(64):
        for attacks in rook_table[square]:
            if attacks is None: attacks = 0
            f.write(f'0x{attacks:x},')
    f.write('\n};\n')

    f.write('static const Magic rook_magics[64] = {\n')
    table_offset = 0
    for square in range(64):
        f.write('{' + f'&rook_table[{table_offset}],0x{rook_masks[square]:x},0x{rook_magics[square]:x},{rook_shifts[square]}' + '},')
        table_offset += len(rook_table[square])
    f.write('\n};\n')

    # bishop magic
    f.write('static constexpr BitBoard bishop_table[] = {\n')
    for square in range(64):
        for attacks in bishop_table[square]:
            if attacks is None: attacks = 0
            f.write(f'0x{attacks:x},')
    f.write('\n};\n')

    f.write('static const Magic bishop_magics[64] = {\n')
    table_offset = 0
    for square in range(64):
        f.write('{' + f'&bishop_table[{table_offset}],0x{bishop_masks[square]:x},0x{bishop_magics[square]:x},{bishop_shifts[square]}' + '},')
        table_offset += len(bishop_table[square])
    f.write('\n};\n\n')

    f.write('BitBoard rook_lookup(Square s, BitBoard occupied) {\n')
    f.write('    Magic const& magic = rook_magics[s];\n')
    f.write('    return magic.table[((occupied & magic.mask) * magic.magic) >> magic.shift];\n')
    f.write('}\n\n')

    f.write('BitBoard bishop_lookup(Square s, BitBoard occupied) {\n')
    f.write('    Magic const& magic = bishop_magics[s];\n')
    f.write('    return magic.table[((occupied & magic.mask) * magic.magic) >> magic.shift];\n')
    f.write('}\n\n')