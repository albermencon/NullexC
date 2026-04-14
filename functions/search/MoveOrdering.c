#include "MoveOrdering.h"
#include "core/types.h"
#include "data/Position.h"
#include "functions/evaluation/Evaluate.h"
#include "functions/evaluation/SSE.h"

#define SCORE_TT_MOVE   10000000
#define SCORE_CAPTURE    5000000
#define SCORE_PROMOTION  4000000
#define SCORE_KILLER_1   3000000
#define SCORE_KILLER_2   2000000

static const int VICTIM_SCORES[12] = {
    1000, 3000, 3000, 5000, 9000, 0, 
    1000, 3000, 3000, 5000, 9000, 0
};

static const int ATTACKER_SCORES[12] = {
    100, 300, 300, 500, 900, 0, 
    100, 300, 300, 500, 900, 0
};

int score_move(const Position* pos, Move move, Move ttMove, int ply);

void order_moves(const Position* pos, Move ttMove, Move* moves, int count, int ply) 
{
    int scores[count];
    for (int i = 0; i < count; i++) {
        scores[i] = score_move(pos, moves[i], ttMove, ply);
    }

    // Insertion sort (descending order)
    for (int i = 1; i < count; i++) {
        int keyScore = scores[i];
        Move keyMove = moves[i];

        int j = i - 1;

        while (j >= 0 && scores[j] < keyScore) {
            scores[j + 1] = scores[j];
            moves[j + 1] = moves[j];
            j--;
        }

        scores[j + 1] = keyScore;
        moves[j + 1] = keyMove;
    }
}

int score_move(const Position* pos, Move move, Move ttMove, int ply) 
{
    if (move == ttMove) return SCORE_TT_MOVE;

    int to = move_to(move);
    int from = move_from(move);
    int flag = move_special_flag(move);
    
    int attacker_piece = Position_PieceAt(pos, from);
    int captured_piece = Position_PieceAt(pos, to);

    // Tactical Moves
    if (captured_piece != -1)
    {
        int mvv = VICTIM_SCORES[captured_piece] - ATTACKER_SCORES[attacker_piece];

        // Cheap pre-filter
        if (mvv >= 0) return SCORE_CAPTURE + mvv;
        
        // Call SEE for suspicious captures
        int see_score = SEE(pos, move, 0);
        if (see_score >= 0) return SCORE_CAPTURE + mvv;

        // Bad captures
        return SCORE_CAPTURE / 2 + mvv;
    }
    if (flag == EN_PASSANT)
    {
        return SCORE_CAPTURE + 1000 - 100;
    }
    if (flag == PROMOTION)
    {
        return SCORE_PROMOTION + VICTIM_SCORES[get_promotion_piece_type(move)];
    }

    // Killer Moves
    if (ply < MAX_PLY) {
        if (move == killer_moves[ply][0]) return SCORE_KILLER_1;
        if (move == killer_moves[ply][1]) return SCORE_KILLER_2;
    }

    // History Moves + PST Delta
    int color = pos->whiteToMove ? WHITE : BLACK;
    int pst_delta = get_pst_score(attacker_piece, to) - get_pst_score(attacker_piece, from);
    
    int piece_type = PIECE_INDEX_TO_TYPE[attacker_piece];
    int history = history_table[color][piece_type][to];

    return history + pst_delta * 1;
}
