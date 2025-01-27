/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2016 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef UCI_H
#define UCI_H

#include <string.h>

#include "types.h"

struct Option;
typedef struct Option Option;

typedef void (*OnChange)(Option *);

enum {
  OPT_TYPE_CHECK, OPT_TYPE_SPIN, OPT_TYPE_BUTTON, OPT_TYPE_STRING,
  OPT_TYPE_COMBO, OPT_TYPE_DISABLED
};

enum {
  OPT_CONTEMPT,
  OPT_ANALYSIS_CONTEMPT,
  OPT_THREADS,
  OPT_HASH,
  OPT_CLEAR_HASH,
  OPT_PONDER,
  OPT_MULTI_PV,
  OPT_SKILL_LEVEL,
  OPT_MOVE_OVERHEAD,
  OPT_SLOW_MOVER,
  OPT_NODES_TIME,
  OPT_ANALYSE_MODE,
  OPT_CHESS960,
  OPT_SYZ_PATH,
  OPT_SYZ_PROBE_DEPTH,
  OPT_SYZ_50_MOVE,
  OPT_SYZ_PROBE_LIMIT,
  OPT_SYZ_USE_DTM,
  OPT_BOOK_FILE,
  OPT_BOOK_FILE2,
  OPT_BOOK_BEST_MOVE,
  OPT_BOOK_DEPTH,
#ifdef NNUE
  OPT_EVAL_FILE,
#ifndef NNUE_PURE
  OPT_USE_NNUE,
#endif
#endif
  OPT_LARGE_PAGES,
  OPT_NUMA,
  OPT_FUTILITY_MARGIN_GAIN,
  OPT_REDUCTION_A,
  OPT_REDUCTION_B,
  OPT_REDUCTION_C,
  OPT_STAT_BONUS_A,
  OPT_STAT_BONUS_B,
  OPT_STAT_BONUS_C,
  OPT_STAT_BONUS_D,
  OPT_REDUCTION_INIT,
  OPT_COUNTER_MOVE_HISTORY_THRESHOLD,
  OPT_ASPIRATION_DELTA_A,
  OPT_ASPIRATION_DELTA_B,
  OPT_ASPIRATION_DELTA_C,
  OPT_ASPIRATION_DELTA_D,
  OPT_BONUS_INITIAL_GAIN,
  OPT_BONUS_INITIAL_THRESHOLD,
  OPT_IMPROVEMENT_DEFAULT,
  OPT_RAZORING_A,
  OPT_RAZORING_B,
  OPT_FUTILITY_A,
  OPT_FUTILITY_DEPTH,
  OPT_NULL_MOVE_THRESH_A,
  OPT_NULL_MOVE_THRESH_B,
  OPT_NULL_MOVE_THRESH_C,
  OPT_NULL_MOVE_THRESH_D,
  OPT_NULL_MOVE_THRESH_E,
  OPT_NULL_MOVE_RA,
  OPT_NULL_MOVE_RB,
  OPT_NULL_MOVE_RC,
  OPT_NULL_MOVE_RD,
  OPT_NULL_MOVE_RE,
  OPT_NULL_MOVE_DEPTH,
  OPT_NULL_MOVE_PLY_A,
  OPT_NULL_MOVE_PLY_B,
  OPT_PROB_CUT_BETA_A,
  OPT_PROB_CUT_BETA_B,
  OPT_PROB_CUT_DEPTH_LIMIT,
  OPT_PROB_CUT_DEPTH,
  OPT_TT_DECREASE_A,
  OPT_TT_DECREASE_B,
  OPT_TT_DECREASE_DEPTH,
  OPT_PROB_CUT_BETA_C,
  OPT_PROB_CUT_DEPTH_THRESH,
  OPT_SHALLOW_PRUNING_DEPTH_A,
  OPT_SHALLOW_PRUNING_A,
  OPT_SHALLOW_PRUNING_B,
  OPT_SHALLOW_PRUNING_C,
  OPT_SSE_THRESHOLD,
  OPT_SHALLOW_PRUNING_DEPTH_B,
  OPT_SHALLOW_PRUNING_D,
  OPT_SHALLOW_PRUNING_GAIN,
  OPT_SHALLOW_PRUNING_DEPTH_C,
  OPT_SHALLOW_PRUNING_E,
  OPT_SHALLOW_PRUNING_F,
  OPT_SHALLOW_PRUNING_G,
  OPT_SHALLOW_PRUNING_H,
  OPT_SHALLOW_PRUNING_I,
  OPT_SINGULAR_EXT_DEPTH_A,
  OPT_SINGULAR_EXT_DEPTH_B,
  OPT_SINGULAR_EXT_DEPTH_C,
  OPT_SINGULAR_BETA_A,
  OPT_SINGULAR_EXTENTION_A,
  OPT_SINGULAR_EXTENTION_B,
  OPT_SINGULAR_EXT_DEPTH_D,
  OPT_SINGULAR_EXTENTION_C,
  OPT_SINGULAR_EXTENTION_D,
  OPT_LMR_DEPTH_THRESHOLD,
  OPT_LMR_MOVE_COUNT_THRESHOLD,
  OPT_LMR_DEC_TT_PV,
  OPT_LMR_DEC_MOVE_COUNT,
  OPT_LMR_DEC_SINGULAR,
  OPT_LMR_INC_CUT_NODE,
  OPT_LMR_INC_TT_CAPTURE,
  OPT_LMR_PV_NODE_A,
  OPT_LMR_PV_NODE_B,
  OPT_LMR_PV_NODE_C,
  OPT_LMR_CUTOFF_CNT_THRESH,
  OPT_LMR_INC_CUTOFF_CNT,
  OPT_LMR_STAT_GAIN,
  OPT_LMR_STAT_DELTA,
  OPT_LMR_R_DEC_A,
  OPT_LMR_R_DEC_B,
  OPT_LMR_R_DEC_DEPTH_A,
  OPT_LMR_R_DEC_DEPTH_B,
  OPT_LMR_DEEP_SEARCH_A,
  OPT_LMR_DEEP_SEARCH_B,
  OPT_MATE_BETA_DELTA,
  OPT_MATE_DEPTH_THRESHOLD,
  OPT_MATE_EXTRA_BONUS,
  OPT_FUTILITY_BASE_DELTA,
  OPT_FALLING_EVAL_A,
  OPT_FALLING_EVAL_B,
  OPT_FALLING_EVAL_C,
  OPT_FALLING_EVAL_D,
  OPT_FALLING_EVAL_CLAMP_MIN,
  OPT_FALLING_EVAL_CLAMP_MAX,
  OPT_TIME_REDUCTION_DEPTH,
  OPT_TIME_REDUCTION_A,
  OPT_TIME_REDUCTION_B,
  OPT_TIME_REDUCTION_C,
  OPT_TIME_REDUCTION_D,
  OPT_BEST_MOVE_INSTABILITY_A,
  OPT_TOTAL_TIME_GAIN,
  OPT_OPT_EXTRA_A,
  OPT_OPT_EXTRA_B,
  OPT_OPT_EXTRA_C,
  OPT_OPT_CONSTANT_A,
  OPT_OPT_CONSTANT_B,
  OPT_OPT_CONSTANT_C,
  OPT_MAX_CONSTANT_A,
  OPT_MAX_CONSTANT_B,
  OPT_MAX_CONSTANT_C,
  OPT_OPT_SCALE_A,
  OPT_OPT_SCALE_B,
  OPT_OPT_SCALE_C,
  OPT_OPT_SCALE_D,
  OPT_MAX_SCALE_A,
  OPT_MAX_SCALE_B,
  OPT_MAXIMUM_TIME_A,
  OPT_MAXIMUM_TIME_B,
};

struct Option {
  char *name;
  int type;
  int def, minVal, maxVal;
  char *defString;
  OnChange onChange;
  int value;
  char *valString;
};

void options_init(void);
void options_free(void);
void print_options(void);
int option_value(int opt);
const char *option_string_value(int opt);
const char *option_default_string_value(int opt);
void option_set_value(int opt, int value);
bool option_set_by_name(char *name, char *value);

void setoption(char *str);
void position(Position *pos, char *str);

void uci_loop(int argc, char* argv[]);
char *uci_value(char *str, Value v);
char *uci_square(char *str, Square s);
char *uci_move(char *str, Move m, int chess960);
void print_pv(Position *pos, Depth depth, Value alpha, Value beta);
Move uci_to_move(const Position *pos, char *str);

#endif
