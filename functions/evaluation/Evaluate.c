#include "Evaluate.h"
#include "core/bitboard.h"
#include "core/types.h"
#include "data/Position.h"

static const int16_t PASSED_PAWN_BONUS[8] = { 0, 120, 80, 50, 30, 15, 15 };
static const int16_t ISOLATED_PAWN_PENALTY_BY_COUNT[9] = { 0, -10, -25, -50, -75, -75, -75, -75, -75 };
static const int16_t KING_PAWN_SHIELD_SCORES[6] = { 4, 7, 4, 3, 6, 3 };
const  float endGameThreshold = 500.0f * 2 + 320.0f + 300.0f;

static inline int evaluate_material(const Position* pos, bool whiteToMove);
static inline int evaluate_pawn_structure(const Position* pos, bool whiteToMove);
static inline int evaluate_king_safety(const Position* pos, bool whiteToMove);
static inline int evaluate_piece_activity(const Position* pos, bool whiteToMove);
static inline int evaluate_endgame(const Position* pos, bool whiteToMove);

int evaluate(const Position* pos)
{
  // normalize the score for the one that is to move
  int score = pos->whiteToMove ? pos->st->eval : -pos->st->eval;

  score += evaluate_pawn_structure(pos, pos->whiteToMove);
  score += evaluate_king_safety(pos, pos->whiteToMove);
  score += evaluate_piece_activity(pos, pos->whiteToMove);
  score += evaluate_endgame(pos, pos->whiteToMove);

  return score;
}

static inline __attribute__((always_inline)) int evaluate_material(const Position* pos, bool whiteToMove)
{
    return 0;
}

static inline __attribute__((always_inline)) int evaluate_pawn_structure(const Position* pos, bool whiteToMove)
{
    return 0;
}

static inline __attribute__((always_inline)) int evaluate_king_safety(const Position* pos, bool whiteToMove)
{
    return 0;
}

static inline __attribute__((always_inline)) int evaluate_piece_activity(const Position* pos, bool whiteToMove)
{
    return 0;
}

static inline __attribute__((always_inline)) int evaluate_endgame(const Position* pos, bool whiteToMove)
{
    return 0;
}