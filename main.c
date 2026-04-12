#include <stdio.h>
#include "functions/attacks/AttackMasks.h"
#include "functions/hash/ZobristHash.h"
#include "data/TranspositionTable.h"
#include "uci/Uci.h"

TranspositionTable TT;

int main() {
    initialize_attack_masks();
    init_line_between_bb();
    zobristHash_init();
    tt_init(&TT, 64);

    uci_loop();

    free_attack_tables();
    tt_free(&TT);
    return 0;
}
