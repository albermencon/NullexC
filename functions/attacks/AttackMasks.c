#include "AttackMasks.h"
#ifdef USE_BMI2
#include <immintrin.h>
#endif
#include "stdlib.h"
#include "string.h"
#include <stdint.h>

// Auxiliar functions
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

uint64_t KNIGHT_ATTACKS[64];
uint64_t KING_ATTACKS[64];
uint64_t PAWN_ATTACKS[2][64];

uint64_t ROOK_MASKS[64];
int ROOK_BITS[64];
uint64_t* ROOK_ATTACKS[64];

uint64_t BISHOP_MASKS[64];
int BISHOP_BITS[64];
uint64_t* BISHOP_ATTACKS[64];

uint64_t ROOK_MAGICS[64] = {
    0x0080011040008020ULL, 0x0440002008100040ULL, 0x0080200010001880ULL, 0x0080048010008800ULL,
    0x0200020010042008ULL, 0x0180010200040080ULL, 0x0080060008800100ULL, 0x0080022100004080ULL,
    0x0010800420804000ULL, 0x0400401000200440ULL, 0x0003001040200100ULL, 0x0000800800823000ULL,
    0x4040808004000800ULL, 0x0200808044000200ULL, 0x0005000200010084ULL, 0x0208800040800100ULL,
    0x0480008020804000ULL, 0x0400808040002004ULL, 0x0003010020004010ULL, 0x0090004040080400ULL,
    0x1008008008040080ULL, 0x3000808004000200ULL, 0x0042004080400100ULL, 0x0000020000408124ULL,
    0x0001400080208002ULL, 0x0000400080200182ULL, 0x0000100480200480ULL, 0x0040090100100020ULL,
    0x0008110100040800ULL, 0x0084010040400200ULL, 0x0401000100020014ULL, 0x0003000100004082ULL,
    0x0000864000800020ULL, 0x0040008040802004ULL, 0x0000904101002000ULL, 0x0001080084801000ULL,
    0x8000040280800800ULL, 0x0011000803000400ULL, 0x0001000C21000200ULL, 0x0200802040800100ULL,
    0x0080209040008000ULL, 0x0010002010C04000ULL, 0x0020200041010010ULL, 0x000010200A020040ULL,
    0x0000900801010004ULL, 0x2000140002008080ULL, 0x0000A20004010100ULL, 0x0001000440810002ULL,
    0x4280004000200040ULL, 0x0011804000200080ULL, 0x0000421100200100ULL, 0x0800880080100080ULL,
    0x0800040080880080ULL, 0x1001040080020080ULL, 0x0001004200044100ULL, 0x0400800041000280ULL,
    0x0000800020410011ULL, 0x0000400080102101ULL, 0x0000410010200009ULL, 0x0000100100200805ULL,
    0x0002001004200802ULL, 0x0002000410080102ULL, 0x0001000200208401ULL, 0x0000002040810402ULL
};

uint64_t BISHOP_MAGICS[64] = {
    0x0008200400404100ULL, 0x000C040404002000ULL, 0x0008080500201000ULL, 0x0004050208000400ULL,
    0x0022021000000180ULL, 0x8001042004200000ULL, 0x4001041004040000ULL, 0x0000804410010800ULL,
    0x0080100410040040ULL, 0x0000020202022200ULL, 0x0000100106006000ULL, 0x2001024081000000ULL,
    0x0000020210002402ULL, 0x0000430420040000ULL, 0x00000A0801041000ULL, 0x0000002088041040ULL,
    0x0040000410042100ULL, 0x0020000401060200ULL, 0x1008001000801010ULL, 0x0008000401202010ULL,
    0x0804001080A00000ULL, 0x0041000080414000ULL, 0x0004040082011000ULL, 0x0000800042080140ULL,
    0x00044000A0020400ULL, 0x0008080020018100ULL, 0x0000221010008200ULL, 0x0024004004010002ULL,
    0x0800840000802002ULL, 0x0008104002010080ULL, 0x0001011004008800ULL, 0x0004208001004100ULL,
    0x0008041010042000ULL, 0x0008012400080800ULL, 0x0040802080040800ULL, 0x0000401808008200ULL,
    0x0800408020020200ULL, 0x00008101000A1000ULL, 0x0005040080040200ULL, 0x2008010020004200ULL,
    0x0002022020000408ULL, 0x0082020202002000ULL, 0x0000104028001004ULL, 0x0000804012001040ULL,
    0x8000400081200200ULL, 0x0802200041000080ULL, 0x0004010401000401ULL, 0x0010014200200080ULL,
    0x0001011010040020ULL, 0x0000208808084000ULL, 0x0400008400880010ULL, 0x0402000084040100ULL,
    0x0006201002020000ULL, 0x0200082008008010ULL, 0x00200801010C0000ULL, 0x0208100082004000ULL,
    0x0000820801010800ULL, 0x0080002404040400ULL, 0x0000000084208820ULL, 0x0000002005420200ULL,
    0x0000000008912400ULL, 0x0000001042101100ULL, 0x0002200802080040ULL, 0x0010200080820040ULL
};

// Helper functions
static void initialize_knight_table(void) {
    for (int square = 0; square < 64; square++)
    {
        uint64_t attacks = 0ULL;
        int rank = square / 8;
        int file = square % 8;

        int offsets[8][2] = {{2,1}, {1,2}, {-1,2}, {-2,1},{-2,-1}, {-1,-2}, {1,-2}, {2,-1}};
        for (int i = 0; i < 8; i++)
        {
            int r = rank + offsets[i][0];
            int f = file + offsets[i][1];
            if (r >= 0 && r < 8 && f >= 0 && f < 8)
            {
                attacks |= 1ULL << (r * 8 + f);
            }
        }
        KNIGHT_ATTACKS[square] = attacks;
    }
}

static void initialize_king_attacks(void) {
    for (int square = 0; square < 64; square++) {
        uint64_t attacks = 0ULL;
        int rank = square / 8;
        int file = square % 8;

        for (int r = MAX(0, rank - 1); r <= MIN(7, rank + 1); r++) {
            for (int f = MAX(0, file - 1); f <= MIN(7, file + 1); f++) {
                if (r != rank || f != file) {
                    attacks |= 1ULL << (r * 8 + f);
                }
            }
        }
        KING_ATTACKS[square] = attacks;
    }
}

static void initialize_pawn_attacks(void) {
    for (int square = 0; square < 64; square++)
    {
        int rank = square / 8;
        int file = square % 8;

        // White pawns
        uint64_t white_attacks = 0ULL;
        if (rank < 7)
        {
                if (file > 0) white_attacks |= 1ULL << ((rank + 1) * 8 + (file - 1));
                if (file < 7) white_attacks |= 1ULL << ((rank + 1) * 8 + (file + 1));
        }
        PAWN_ATTACKS[0][square] = white_attacks;

        // Black pawns 
        uint64_t black_attacks = 0ULL;
        if (rank > 0)
        {
                if (file > 0) black_attacks |= 1ULL << ((rank - 1) * 8 + (file - 1));
                if (file < 7) black_attacks |= 1ULL << ((rank - 1) * 8 + (file + 1));
        }
        PAWN_ATTACKS[1][square] = black_attacks;
    }
}

uint64_t createRookMask(int square) {
    uint64_t mask = 0ULL;
    int rank = square / 8;
    int file = square % 8;

    // Horizontal
    for (int f = 1; f < 7; f++) {
        if (f != file)
            mask |= 1ULL << (rank * 8 + f);
    }
    //Vertical
    for (int r = 1; r < 7; r++) {
        if (r != rank)
            mask |= 1ULL << (r * 8 + file);
    }
    return mask;
}

uint64_t createBishopMask(int square) {
    uint64_t mask = 0ULL;
    int rank = square / 8;
    int file = square % 8;

    int directions[4][2] = {{1,1}, {1,-1}, {-1,1}, {-1,-1}};
    for (int d=0; d<4; d++) {
        int r = rank + directions[d][0];
        int f = file + directions[d][1];
        while (r >= 1 && r <= 6 && f >= 1 && f <= 6) {
            mask |= 1ULL << (r * 8 + f);
            r += directions[d][0];
            f += directions[d][1];
        }
    }
    return mask;
}

uint64_t createOccupancyFromIndex(int index, uint64_t mask) {
    uint64_t occupancy = 0ULL;
    int bitIndex = 0;

    while (mask != 0) {
        int square = lsb(mask);
        if (index & (1 << bitIndex))
            occupancy |= 1ULL << square;
        mask &= mask - 1;
        bitIndex++;
    }
    return occupancy;
}

uint64_t createRookAttacks(int square, uint64_t occupied) {
    uint64_t attacks = 0ULL;
    int rank = square / 8;
    int file = square % 8;

    int directions[4][2] = {{0,1},{0,-1},{1,0},{-1,0}};

    for (int d=0; d<4; d++) {
        int r = rank + directions[d][0];
        int f = file + directions[d][1];
        while (r >= 0 && r < 8 && f >= 0 && f < 8) {
            int targetSquare = r*8 + f;
            attacks |= 1ULL << targetSquare;

            if (occupied & (1ULL << targetSquare))
                break;
            r += directions[d][0];
            f += directions[d][1];
        }
    }
    return attacks;
}

uint64_t createBishopAttacks(int square, uint64_t occupied) {
    uint64_t attacks = 0ULL;
    int rank = square / 8;
    int file = square % 8;

    int directions[4][2] = {{1,1}, {1,-1}, {-1,1}, {-1,-1}};

    for (int d=0; d<4; d++) {
        int r = rank + directions[d][0];
        int f = file + directions[d][1];
        while (r >= 0 && r < 8 && f >= 0 && f < 8) {
            int targetSquare = r*8 + f;
            attacks |= 1ULL << targetSquare;

            if (occupied & (1ULL << targetSquare))
                break;
            r += directions[d][0];
            f += directions[d][1];
        }
    }
    return attacks;
}

void initSliderAttacks(int square, int isRook) {
    uint64_t mask = isRook == 1 ? ROOK_MASKS[square] : BISHOP_MASKS[square];
    int bits = isRook == 1 ? ROOK_BITS[square] : BISHOP_BITS[square];
    uint64_t magic = isRook == 1 ? ROOK_MAGICS[square] : BISHOP_MAGICS[square];

    int size = 1 << bits;
    uint64_t *attackTable = (uint64_t *) malloc(size * sizeof(uint64_t));
    if (!attackTable) {
        exit(1); // malloc errors
    }

    // Initialize all entries to 0
    memset(attackTable, 0, size * sizeof(uint64_t));

    for (int i=0; i < size; i++) {
        uint64_t occupied = createOccupancyFromIndex(i, mask);
        uint64_t attacks = isRook ? createRookAttacks(square, occupied) : createBishopAttacks(square, occupied);

#ifdef USE_BMI2
        // PEXT index: directly extract bits at mask positions
        int index = (int)_pext_u64(occupied, mask);
#else
        // Magic index
        int index = (int)((occupied * magic) >> (64 - bits));
#endif
        attackTable[index] = attacks;
    }

    if (isRook == 1)
        ROOK_ATTACKS[square] = attackTable;
    else
        BISHOP_ATTACKS[square] = attackTable;
}

void initializeMagicBitboards() {
    for (int square=0; square < 64; square++) {
        ROOK_MASKS[square] = createRookMask(square);
        BISHOP_MASKS[square] = createBishopMask(square);
        ROOK_BITS[square] = popcount64(ROOK_MASKS[square]);
        BISHOP_BITS[square] = popcount64(BISHOP_MASKS[square]);

        initSliderAttacks(square, 1); // rook
        initSliderAttacks(square, 0); // bishop
    }
}

void initialize_attack_masks(void) {
    initialize_knight_table();
    initialize_king_attacks();
    initialize_pawn_attacks();
    initializeMagicBitboards();
}

// Free memory
void free_attack_tables(void) {
    for (int sq = 0; sq < 64; sq++) {
        if (ROOK_ATTACKS[sq] != NULL) {
            free(ROOK_ATTACKS[sq]);
            ROOK_ATTACKS[sq] = NULL;
        }
        if (BISHOP_ATTACKS[sq] != NULL) {
            free(BISHOP_ATTACKS[sq]);
            BISHOP_ATTACKS[sq] = NULL;
        }
    }
}