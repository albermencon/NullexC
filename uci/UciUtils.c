#include "UciUtils.h"
#include <stdio.h>
#include "core/types.h"

Square uci_to_sq(const char* s) {
    if (s[0] == '-') return -1;
    int file = s[0] - 'a';
    int rank = s[1] - '1';
    return (Square)(rank * 8 + file);
}

void print_uci_move(Move move) {
    if (move == move_null()) {
        printf("0000");
        return;
    }
    
    int f = move_from(move);
    int t = move_to(move);
    printf("%c%c%c%c", 'a' + (f % 8), '1' + (f / 8), 'a' + (t % 8), '1' + (t / 8));

    if (move_special_flag(move) == PROMOTION) {
        PieceType pt = get_promotion_piece_type(move);
        const char* promo_chars = " nbrq"; 
        printf("%c", promo_chars[pt]);
    }
}
