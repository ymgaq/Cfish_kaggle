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

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#ifndef _WIN32
#include <sys/mman.h>
#endif

#include "evaluate.h"
#include "misc.h"
#include "numa.h"
#include "polybook.h"
#include "search.h"
#include "settings.h"
#include "thread.h"
#include "tt.h"
#include "uci.h"

// 'On change' actions, triggered by an option's value change
static void on_clear_hash(Option *opt)
{
  (void)opt;

  if (settings.ttSize)
    search_clear();
}

static void on_hash_size(Option *opt)
{
  delayedSettings.ttSize = opt->value;
}

static void on_numa(Option *opt)
{
#ifdef NUMA
  read_numa_nodes(opt->valString);
#else
  (void)opt;
#endif
}

static void on_threads(Option *opt)
{
  delayedSettings.numThreads = opt->value;
}

static void on_tb_path(Option *opt) {}

static void on_large_pages(Option *opt)
{
  delayedSettings.largePages = opt->value;
}

static void on_book_file(Option *opt)
{
  pb_init(&polybook, opt->valString);
}

static void on_book_file2(Option *opt)
{
  pb_init(&polybook2, opt->valString);
}

static void on_best_book_move(Option *opt)
{
  pb_set_best_book_move(opt->value);
}

static void on_book_depth(Option *opt)
{
  pb_set_book_depth(opt->value);
}

static void init_all_params();

static void on_param(Option *opt)
{
  init_all_params();
}

#ifdef IS_64BIT
#define MAXHASHMB 33554432
#else
#define MAXHASHMB 2048
#endif

static Option optionsMap[] = {
  { "Contempt", OPT_TYPE_SPIN, 24, -100, 100, NULL, NULL, 0, NULL },
  { "Analysis Contempt", OPT_TYPE_COMBO, 0, 0, 0,
    "Off var Off var White var Black", NULL, 0, NULL },
  { "Threads", OPT_TYPE_SPIN, 1, 1, MAX_THREADS, NULL, on_threads, 0, NULL },
  { "Hash", OPT_TYPE_SPIN, 1024, 1, MAXHASHMB, NULL, on_hash_size, 0, NULL },
  { "Clear Hash", OPT_TYPE_BUTTON, 0, 0, 0, NULL, on_clear_hash, 0, NULL },
  { "Ponder", OPT_TYPE_CHECK, 0, 0, 0, NULL, NULL, 0, NULL },
  { "MultiPV", OPT_TYPE_SPIN, 1, 1, 500, NULL, NULL, 0, NULL },
  { "Skill Level", OPT_TYPE_SPIN, 20, 0, 20, NULL, NULL, 0, NULL },
  { "Move Overhead", OPT_TYPE_SPIN, 10, 0, 5000, NULL, NULL, 0, NULL },
  { "Slow Mover", OPT_TYPE_SPIN, 100, 10, 1000, NULL, NULL, 0, NULL },
  { "nodestime", OPT_TYPE_SPIN, 0, 0, 10000, NULL, NULL, 0, NULL },
  { "UCI_AnalyseMode", OPT_TYPE_CHECK, 0, 0, 0, NULL, NULL, 0, NULL },
  { "UCI_Chess960", OPT_TYPE_CHECK, 0, 0, 0, NULL, NULL, 0, NULL },
  { "SyzygyPath", OPT_TYPE_STRING, 0, 0, 0, "<empty>", on_tb_path, 0, NULL },
  { "SyzygyProbeDepth", OPT_TYPE_SPIN, 1, 1, 100, NULL, NULL, 0, NULL },
  { "Syzygy50MoveRule", OPT_TYPE_CHECK, 1, 0, 0, NULL, NULL, 0, NULL },
  { "SyzygyProbeLimit", OPT_TYPE_SPIN, 7, 0, 7, NULL, NULL, 0, NULL },
  { "SyzygyUseDTM", OPT_TYPE_CHECK, 1, 0, 0, NULL, NULL, 0, NULL },
  { "BookFile", OPT_TYPE_STRING, 0, 0, 0, "<empty>", on_book_file, 0, NULL },
  { "BookFile2", OPT_TYPE_STRING, 0, 0, 0, "<empty>", on_book_file2, 0, NULL },
  { "BestBookMove", OPT_TYPE_CHECK, 1, 0, 0, NULL, on_best_book_move, 0, NULL },
  { "BookDepth", OPT_TYPE_SPIN, 255, 1, 255, NULL, on_book_depth, 0, NULL },
#ifdef NNUE
  { "EvalFile", OPT_TYPE_STRING, 0, 0, 0, DefaultEvalFile, NULL, 0, NULL },
#ifndef NNUE_PURE
  { "Use NNUE", OPT_TYPE_COMBO, 0, 0, 0,
    "Hybrid var Hybrid var Pure var Classical", NULL, 0, NULL },
#endif
#endif
  { "LargePages", OPT_TYPE_CHECK, 0, 0, 0, NULL, on_large_pages, 0, NULL },
  { "NUMA", OPT_TYPE_STRING, 0, 0, 0, "all", on_numa, 0, NULL },
  { "futilityMarginGain", OPT_TYPE_SPIN, 165, 100, 200, NULL, on_param, 0, NULL },
  { "reductionA", OPT_TYPE_SPIN, 1642, 1000, 2000, NULL, on_param, 0, NULL },
  { "reductionB", OPT_TYPE_SPIN, 1024, 500, 1500, NULL, on_param, 0, NULL },
  { "reductionC", OPT_TYPE_SPIN, 916, 500, 1500, NULL, on_param, 0, NULL },
  { "statBonusA", OPT_TYPE_SPIN, 12, 1, 30, NULL, on_param, 0, NULL },
  { "statBonusB", OPT_TYPE_SPIN, 282, 100, 500, NULL, on_param, 0, NULL },
  { "statBonusC", OPT_TYPE_SPIN, 349, 100, 500, NULL, on_param, 0, NULL },
  { "statBonusD", OPT_TYPE_SPIN, 1594, 1000, 2000, NULL, on_param, 0, NULL },
  { "reductionInit", OPT_TYPE_SPIN, 2026, 1000, 3000, NULL, on_param, 0, NULL },
  { "counterMoveHistoryThreshold", OPT_TYPE_SPIN, -1, -150, 0, NULL, on_param, 0, NULL },
  { "aspirationDeltaA", OPT_TYPE_SPIN, 10, 0, 30, NULL, on_param, 0, NULL },
  { "aspirationDeltaB", OPT_TYPE_SPIN, 15620, 10000, 20000, NULL, on_param, 0, NULL },
  { "aspirationDeltaC", OPT_TYPE_SPIN, 4, 2, 7, NULL, on_param, 0, NULL },
  { "aspirationDeltaD", OPT_TYPE_SPIN, 2, 1, 10, NULL, on_param, 0, NULL },
  { "bonusInitialGain", OPT_TYPE_SPIN, -19, -100, 0, NULL, on_param, 0, NULL },
  { "bonusInitialThreshold", OPT_TYPE_SPIN, 1914, 1000, 3000, NULL, on_param, 0, NULL },
  { "improvementDefault", OPT_TYPE_SPIN, 168, 0, 400, NULL, on_param, 0, NULL },
  { "razoringA", OPT_TYPE_SPIN, -369, -500, 0, NULL, on_param, 0, NULL },
  { "razoringB", OPT_TYPE_SPIN, -254, -500, 0, NULL, on_param, 0, NULL },
  { "futilityA", OPT_TYPE_SPIN, 303, 100, 500, NULL, on_param, 0, NULL },
  { "futilityDepth", OPT_TYPE_SPIN, 8, 3, 12, NULL, on_param, 0, NULL },
  { "nullMoveThreshA", OPT_TYPE_SPIN, 17139, 10000, 20000, NULL, on_param, 0, NULL },
  { "nullMoveThreshB", OPT_TYPE_SPIN, -20, -100, 0, NULL, on_param, 0, NULL },
  { "nullMoveThreshC", OPT_TYPE_SPIN, 13, 1, 20, NULL, on_param, 0, NULL },
  { "nullMoveThreshD", OPT_TYPE_SPIN, 233, 100, 500, NULL, on_param, 0, NULL },
  { "nullMoveThreshE", OPT_TYPE_SPIN, 25, 1, 50, NULL, on_param, 0, NULL },
  { "nullMoveRA", OPT_TYPE_SPIN, 168, 100, 500, NULL, on_param, 0, NULL },
  { "nullMoveRB", OPT_TYPE_SPIN, 7, 1, 20, NULL, on_param, 0, NULL },
  { "nullMoveRC", OPT_TYPE_SPIN, 3, 1, 10, NULL, on_param, 0, NULL },
  { "nullMoveRD", OPT_TYPE_SPIN, 4, 1, 10, NULL, on_param, 0, NULL },
  { "nullMoveRE", OPT_TYPE_SPIN, 861, 100, 1500, NULL, on_param, 0, NULL },
  { "nullMoveDepth", OPT_TYPE_SPIN, 14, 8, 20, NULL, on_param, 0, NULL },
  { "nullMovePlyA", OPT_TYPE_SPIN, 3, 1, 5, NULL, on_param, 0, NULL },
  { "nullMovePlyB", OPT_TYPE_SPIN, 4, 1, 8, NULL, on_param, 0, NULL },
  { "probCutBetaA", OPT_TYPE_SPIN, 191, 100, 300, NULL, on_param, 0, NULL },
  { "probCutBetaB", OPT_TYPE_SPIN, 54, 10, 100, NULL, on_param, 0, NULL },
  { "probCutDepthLimit", OPT_TYPE_SPIN, 4, 1, 10, NULL, on_param, 0, NULL },
  { "probCutDepth", OPT_TYPE_SPIN, 3, 1, 10, NULL, on_param, 0, NULL },
  { "ttDecreaseA", OPT_TYPE_SPIN, 3, 1, 5, NULL, on_param, 0, NULL },
  { "ttDecreaseB", OPT_TYPE_SPIN, 2, 1, 5, NULL, on_param, 0, NULL },
  { "ttDecreaseDepth", OPT_TYPE_SPIN, 9, 1, 20, NULL, on_param, 0, NULL },
  { "probCutBetaC", OPT_TYPE_SPIN, 417, 300, 500, NULL, on_param, 0, NULL },
  { "probCutDepthThresh", OPT_TYPE_SPIN, 2, 1, 5, NULL, on_param, 0, NULL },
  { "shallowPruningDepthA", OPT_TYPE_SPIN, 7, 1, 15, NULL, on_param, 0, NULL },
  { "shallowPruningA", OPT_TYPE_SPIN, 180, 100, 300, NULL, on_param, 0, NULL },
  { "shallowPruningB", OPT_TYPE_SPIN, 201, 100, 300, NULL, on_param, 0, NULL },
  { "shallowPruningC", OPT_TYPE_SPIN, 6, 1, 10, NULL, on_param, 0, NULL },
  { "sseThreshold", OPT_TYPE_SPIN, -222, -500, 0, NULL, on_param, 0, NULL },
  { "shallowPruningDepthB", OPT_TYPE_SPIN, 5, 1, 10, NULL, on_param, 0, NULL },
  { "shallowPruningD", OPT_TYPE_SPIN, -3875, -5000, -1000, NULL, on_param, 0, NULL },
  { "shallowPruningGain", OPT_TYPE_SPIN, 2, 1, 5, NULL, on_param, 0, NULL },
  { "shallowPruningDepthC", OPT_TYPE_SPIN, 13, 1, 20, NULL, on_param, 0, NULL },
  { "shallowPruningE", OPT_TYPE_SPIN, 106, 50, 200, NULL, on_param, 0, NULL },
  { "shallowPruningF", OPT_TYPE_SPIN, 145, 100, 200, NULL, on_param, 0, NULL },
  { "shallowPruningG", OPT_TYPE_SPIN, 52, 10, 100, NULL, on_param, 0, NULL },
  { "shallowPruningH", OPT_TYPE_SPIN, -24, -100, 0, NULL, on_param, 0, NULL },
  { "shallowPruningI", OPT_TYPE_SPIN, -15, -50, 0, NULL, on_param, 0, NULL },
  { "singularExtDepthA", OPT_TYPE_SPIN, 4, 1, 8, NULL, on_param, 0, NULL },
  { "singularExtDepthB", OPT_TYPE_SPIN, 2, 1, 5, NULL, on_param, 0, NULL },
  { "singularExtDepthC", OPT_TYPE_SPIN, 3, 1, 5, NULL, on_param, 0, NULL },
  { "singularBetaA", OPT_TYPE_SPIN, 3, 1, 5, NULL, on_param, 0, NULL },
  { "singularExtentionA", OPT_TYPE_SPIN, 25, 10, 50, NULL, on_param, 0, NULL },
  { "singularExtentionB", OPT_TYPE_SPIN, 9, 5, 15, NULL, on_param, 0, NULL },
  { "singularExtDepthD", OPT_TYPE_SPIN, 9, 5, 15, NULL, on_param, 0, NULL },
  { "singularExtentionC", OPT_TYPE_SPIN, 82, 50, 150, NULL, on_param, 0, NULL },
  { "singularExtentionD", OPT_TYPE_SPIN, 5177, 1000, 10000, NULL, on_param, 0, NULL },
  { "lmrDepthThreshold", OPT_TYPE_SPIN, 2, 1, 5, NULL, on_param, 0, NULL },
  { "lmrMoveCountThreshold", OPT_TYPE_SPIN, 7, 1, 15, NULL, on_param, 0, NULL },
  { "lmrDecTTPv", OPT_TYPE_SPIN, 2, 0, 3, NULL, on_param, 0, NULL },
  { "lmrDecMoveCount", OPT_TYPE_SPIN, 1, 0, 3, NULL, on_param, 0, NULL },
  { "lmrDecSingular", OPT_TYPE_SPIN, 1, 0, 3, NULL, on_param, 0, NULL },
  { "lmrIncCutNode", OPT_TYPE_SPIN, 2, 0, 3, NULL, on_param, 0, NULL },
  { "lmrIncTTCapture", OPT_TYPE_SPIN, 1, 0, 3, NULL, on_param, 0, NULL },
  { "lmrPvNodeA", OPT_TYPE_SPIN, 1, 1, 5, NULL, on_param, 0, NULL },
  { "lmrPvNodeB", OPT_TYPE_SPIN, 11, 1, 20, NULL, on_param, 0, NULL },
  { "lmrPvNodeC", OPT_TYPE_SPIN, 3, 1, 5, NULL, on_param, 0, NULL },
  { "lmrCutoffCntThresh", OPT_TYPE_SPIN, 3, 1, 5, NULL, on_param, 0, NULL },
  { "lmrIncCutoffCnt", OPT_TYPE_SPIN, 1, 0, 3, NULL, on_param, 0, NULL },
  { "lmrStatGain", OPT_TYPE_SPIN, 2, 1, 5, NULL, on_param, 0, NULL },
  { "lmrStatDelta", OPT_TYPE_SPIN, 4433, 1000, 10000, NULL, on_param, 0, NULL },
  { "lmrRDecA", OPT_TYPE_SPIN, 13628, 10000, 20000, NULL, on_param, 0, NULL },
  { "lmrRDecB", OPT_TYPE_SPIN, 4000, 1000, 10000, NULL, on_param, 0, NULL },
  { "lmrRDecDepthA", OPT_TYPE_SPIN, 7, 3, 10, NULL, on_param, 0, NULL },
  { "lmrRDecDepthB", OPT_TYPE_SPIN, 19, 11, 30, NULL, on_param, 0, NULL },
  { "lmrDeepSearchA", OPT_TYPE_SPIN, 64, 10, 100, NULL, on_param, 0, NULL },
  { "lmrDeepSearchB", OPT_TYPE_SPIN, 11, 1, 20, NULL, on_param, 0, NULL },
  { "mateBetaDelta", OPT_TYPE_SPIN, 137, 50, 250, NULL, on_param, 0, NULL },
  { "mateDepthThreshold", OPT_TYPE_SPIN, 5, 1, 10, NULL, on_param, 0, NULL },
  { "mateExtraBonus", OPT_TYPE_SPIN, 62, 10, 100, NULL, on_param, 0, NULL },
  { "futilityBaseDelta", OPT_TYPE_SPIN, 153, 50, 200, NULL, on_param, 0, NULL },
  { "fallingEvalA", OPT_TYPE_SPIN, 66, 10, 100, NULL, on_param, 0, NULL },
  { "fallingEvalB", OPT_TYPE_SPIN, 14, 5, 25, NULL, on_param, 0, NULL },
  { "fallingEvalC", OPT_TYPE_SPIN, 6, 3, 10, NULL, on_param, 0, NULL },
  { "fallingEvalD", OPT_TYPE_SPIN, 617, 100, 1000, NULL, on_param, 0, NULL },
  { "fallingEvalClampMin", OPT_TYPE_SPIN, 51, 1, 99, NULL, on_param, 0, NULL },
  { "fallingEvalClampMax", OPT_TYPE_SPIN, 151, 101, 200, NULL, on_param, 0, NULL },
  { "timeReductionDepth", OPT_TYPE_SPIN, 8, 4, 12, NULL, on_param, 0, NULL },
  { "timeReductionA", OPT_TYPE_SPIN, 156, 100, 200, NULL, on_param, 0, NULL },
  { "timeReductionB", OPT_TYPE_SPIN, 69, 30, 120, NULL, on_param, 0, NULL },
  { "timeReductionC", OPT_TYPE_SPIN, 140, 100, 180, NULL, on_param, 0, NULL },
  { "timeReductionD", OPT_TYPE_SPIN, 217, 150, 250, NULL, on_param, 0, NULL },
  { "bestMoveInstabilityA", OPT_TYPE_SPIN, 179, 100, 250, NULL, on_param, 0, NULL },
  { "totalTimeGain", OPT_TYPE_SPIN, 75, 30, 100, NULL, on_param, 0, NULL },
  { "optExtraA", OPT_TYPE_SPIN, 100, 50, 150, NULL, on_param, 0, NULL },
  { "optExtraB", OPT_TYPE_SPIN, 125, 50, 150, NULL, on_param, 0, NULL },
  { "optExtraC", OPT_TYPE_SPIN, 111, 50, 150, NULL, on_param, 0, NULL },
  { "optConstantA", OPT_TYPE_SPIN, 334, 100, 500, NULL, on_param, 0, NULL },
  { "optConstantB", OPT_TYPE_SPIN, 30, 10, 50, NULL, on_param, 0, NULL },
  { "optConstantC", OPT_TYPE_SPIN, 49, 10, 100, NULL, on_param, 0, NULL },
  { "maxConstantA", OPT_TYPE_SPIN, 340, 100, 500, NULL, on_param, 0, NULL },
  { "maxConstantB", OPT_TYPE_SPIN, 300, 100, 500, NULL, on_param, 0, NULL },
  { "maxConstantC", OPT_TYPE_SPIN, 276, 100, 500, NULL, on_param, 0, NULL },
  { "optScaleA", OPT_TYPE_SPIN, 120, 100, 200, NULL, on_param, 0, NULL },
  { "optScaleB", OPT_TYPE_SPIN, 31, 20, 40, NULL, on_param, 0, NULL },
  { "optScaleC", OPT_TYPE_SPIN, 44, 30, 60, NULL, on_param, 0, NULL },
  { "optScaleD", OPT_TYPE_SPIN, 21, 10, 50, NULL, on_param, 0, NULL },
  { "maxScaleA", OPT_TYPE_SPIN, 69, 50, 100, NULL, on_param, 0, NULL },
  { "maxScaleB", OPT_TYPE_SPIN, 122, 100, 150, NULL, on_param, 0, NULL },
  { "maximumTimeA", OPT_TYPE_SPIN, 84, 50, 150, NULL, on_param, 0, NULL },
  { "maximumTimeB", OPT_TYPE_SPIN, 10, 0, 30, NULL, on_param, 0, NULL },
  { 0 }
};

static void init_all_params() {
  futilityMarginGain = optionsMap[OPT_FUTILITY_MARGIN_GAIN].value;
  reductionA = optionsMap[OPT_REDUCTION_A].value;
  reductionB = optionsMap[OPT_REDUCTION_B].value;
  reductionC = optionsMap[OPT_REDUCTION_C].value;
  statBonusA = optionsMap[OPT_STAT_BONUS_A].value;
  statBonusB = optionsMap[OPT_STAT_BONUS_B].value;
  statBonusC = optionsMap[OPT_STAT_BONUS_C].value;
  statBonusD = optionsMap[OPT_STAT_BONUS_D].value;
  reductionInit = optionsMap[OPT_REDUCTION_INIT].value;
  counterMoveHistoryThreshold = optionsMap[OPT_COUNTER_MOVE_HISTORY_THRESHOLD].value;
  aspirationDeltaA = optionsMap[OPT_ASPIRATION_DELTA_A].value;
  aspirationDeltaB = optionsMap[OPT_ASPIRATION_DELTA_B].value;
  aspirationDeltaC = optionsMap[OPT_ASPIRATION_DELTA_C].value;
  aspirationDeltaD = optionsMap[OPT_ASPIRATION_DELTA_D].value;
  bonusInitialGain = optionsMap[OPT_BONUS_INITIAL_GAIN].value;
  bonusInitialThreshold = optionsMap[OPT_BONUS_INITIAL_THRESHOLD].value;
  improvementDefault = optionsMap[OPT_IMPROVEMENT_DEFAULT].value;
  razoringA = optionsMap[OPT_RAZORING_A].value;
  razoringB = optionsMap[OPT_RAZORING_B].value;
  futilityA = optionsMap[OPT_FUTILITY_A].value;
  futilityDepth = optionsMap[OPT_FUTILITY_DEPTH].value;
  nullMoveThreshA = optionsMap[OPT_NULL_MOVE_THRESH_A].value;
  nullMoveThreshB = optionsMap[OPT_NULL_MOVE_THRESH_B].value;
  nullMoveThreshC = optionsMap[OPT_NULL_MOVE_THRESH_C].value;
  nullMoveThreshD = optionsMap[OPT_NULL_MOVE_THRESH_D].value;
  nullMoveThreshE = optionsMap[OPT_NULL_MOVE_THRESH_E].value;
  nullMoveRA = optionsMap[OPT_NULL_MOVE_RA].value;
  nullMoveRB = optionsMap[OPT_NULL_MOVE_RB].value;
  nullMoveRC = optionsMap[OPT_NULL_MOVE_RC].value;
  nullMoveRD = optionsMap[OPT_NULL_MOVE_RD].value;
  nullMoveRE = optionsMap[OPT_NULL_MOVE_RE].value;
  nullMoveDepth = optionsMap[OPT_NULL_MOVE_DEPTH].value;
  nullMovePlyA = optionsMap[OPT_NULL_MOVE_PLY_A].value;
  nullMovePlyB = optionsMap[OPT_NULL_MOVE_PLY_B].value;
  probCutBetaA = optionsMap[OPT_PROB_CUT_BETA_A].value;
  probCutBetaB = optionsMap[OPT_PROB_CUT_BETA_B].value;
  probCutDepthLimit = optionsMap[OPT_PROB_CUT_DEPTH_LIMIT].value;
  probCutDepth = optionsMap[OPT_PROB_CUT_DEPTH].value;
  ttDecreaseA = optionsMap[OPT_TT_DECREASE_A].value;
  ttDecreaseB = optionsMap[OPT_TT_DECREASE_B].value;
  ttDecreaseDepth = optionsMap[OPT_TT_DECREASE_DEPTH].value;
  probCutBetaC = optionsMap[OPT_PROB_CUT_BETA_C].value;
  probCutDepthThresh = optionsMap[OPT_PROB_CUT_DEPTH_THRESH].value;
  shallowPruningDepthA = optionsMap[OPT_SHALLOW_PRUNING_DEPTH_A].value;
  shallowPruningA = optionsMap[OPT_SHALLOW_PRUNING_A].value;
  shallowPruningB = optionsMap[OPT_SHALLOW_PRUNING_B].value;
  shallowPruningC = optionsMap[OPT_SHALLOW_PRUNING_C].value;
  sseThreshold = optionsMap[OPT_SSE_THRESHOLD].value;
  shallowPruningDepthB = optionsMap[OPT_SHALLOW_PRUNING_DEPTH_B].value;
  shallowPruningD = optionsMap[OPT_SHALLOW_PRUNING_D].value;
  shallowPruningGain = optionsMap[OPT_SHALLOW_PRUNING_GAIN].value;
  shallowPruningDepthC = optionsMap[OPT_SHALLOW_PRUNING_DEPTH_C].value;
  shallowPruningE = optionsMap[OPT_SHALLOW_PRUNING_E].value;
  shallowPruningF = optionsMap[OPT_SHALLOW_PRUNING_F].value;
  shallowPruningG = optionsMap[OPT_SHALLOW_PRUNING_G].value;
  shallowPruningH = optionsMap[OPT_SHALLOW_PRUNING_H].value;
  shallowPruningI = optionsMap[OPT_SHALLOW_PRUNING_I].value;
  singularExtDepthA = optionsMap[OPT_SINGULAR_EXT_DEPTH_A].value;
  singularExtDepthB = optionsMap[OPT_SINGULAR_EXT_DEPTH_B].value;
  singularExtDepthC = optionsMap[OPT_SINGULAR_EXT_DEPTH_C].value;
  singularBetaA = optionsMap[OPT_SINGULAR_BETA_A].value;
  singularExtentionA = optionsMap[OPT_SINGULAR_EXTENTION_A].value;
  singularExtentionB = optionsMap[OPT_SINGULAR_EXTENTION_B].value;
  singularExtDepthD = optionsMap[OPT_SINGULAR_EXT_DEPTH_D].value;
  singularExtentionC = optionsMap[OPT_SINGULAR_EXTENTION_C].value;
  singularExtentionD = optionsMap[OPT_SINGULAR_EXTENTION_D].value;
  lmrDepthThreshold = optionsMap[OPT_LMR_DEPTH_THRESHOLD].value;
  lmrMoveCountThreshold = optionsMap[OPT_LMR_MOVE_COUNT_THRESHOLD].value;
  lmrDecTTPv = optionsMap[OPT_LMR_DEC_TT_PV].value;
  lmrDecMoveCount = optionsMap[OPT_LMR_DEC_MOVE_COUNT].value;
  lmrDecSingular = optionsMap[OPT_LMR_DEC_SINGULAR].value;
  lmrIncCutNode = optionsMap[OPT_LMR_INC_CUT_NODE].value;
  lmrIncTTCapture = optionsMap[OPT_LMR_INC_TT_CAPTURE].value;
  lmrPvNodeA = optionsMap[OPT_LMR_PV_NODE_A].value;
  lmrPvNodeB = optionsMap[OPT_LMR_PV_NODE_B].value;
  lmrPvNodeC = optionsMap[OPT_LMR_PV_NODE_C].value;
  lmrCutoffCntThresh = optionsMap[OPT_LMR_CUTOFF_CNT_THRESH].value;
  lmrIncCutoffCnt = optionsMap[OPT_LMR_INC_CUTOFF_CNT].value;
  lmrStatGain = optionsMap[OPT_LMR_STAT_GAIN].value;
  lmrStatDelta = optionsMap[OPT_LMR_STAT_DELTA].value;
  lmrRDecA = optionsMap[OPT_LMR_R_DEC_A].value;
  lmrRDecB = optionsMap[OPT_LMR_R_DEC_B].value;
  lmrRDecDepthA = optionsMap[OPT_LMR_R_DEC_DEPTH_A].value;
  lmrRDecDepthB = optionsMap[OPT_LMR_R_DEC_DEPTH_B].value;
  lmrDeepSearchA = optionsMap[OPT_LMR_DEEP_SEARCH_A].value;
  lmrDeepSearchB = optionsMap[OPT_LMR_DEEP_SEARCH_B].value;
  mateBetaDelta = optionsMap[OPT_MATE_BETA_DELTA].value;
  mateDepthThreshold = optionsMap[OPT_MATE_DEPTH_THRESHOLD].value;
  mateExtraBonus = optionsMap[OPT_MATE_EXTRA_BONUS].value;
  futilityBaseDelta = optionsMap[OPT_FUTILITY_BASE_DELTA].value;
  fallingEvalA = optionsMap[OPT_FALLING_EVAL_A].value;
  fallingEvalB = optionsMap[OPT_FALLING_EVAL_B].value;
  fallingEvalC = optionsMap[OPT_FALLING_EVAL_C].value;
  fallingEvalD = optionsMap[OPT_FALLING_EVAL_D].value;
  fallingEvalClampMin = optionsMap[OPT_FALLING_EVAL_CLAMP_MIN].value;
  fallingEvalClampMax = optionsMap[OPT_FALLING_EVAL_CLAMP_MAX].value;
  timeReductionDepth = optionsMap[OPT_TIME_REDUCTION_DEPTH].value;
  timeReductionA = optionsMap[OPT_TIME_REDUCTION_A].value;
  timeReductionB = optionsMap[OPT_TIME_REDUCTION_B].value;
  timeReductionC = optionsMap[OPT_TIME_REDUCTION_C].value;
  timeReductionD = optionsMap[OPT_TIME_REDUCTION_D].value;
  bestMoveInstabilityA = optionsMap[OPT_BEST_MOVE_INSTABILITY_A].value;
  totalTimeGain = optionsMap[OPT_TOTAL_TIME_GAIN].value;
  optExtraA = optionsMap[OPT_OPT_EXTRA_A].value;
  optExtraB = optionsMap[OPT_OPT_EXTRA_B].value;
  optExtraC = optionsMap[OPT_OPT_EXTRA_C].value;
  optConstantA = optionsMap[OPT_OPT_CONSTANT_A].value;
  optConstantB = optionsMap[OPT_OPT_CONSTANT_B].value;
  optConstantC = optionsMap[OPT_OPT_CONSTANT_C].value;
  maxConstantA = optionsMap[OPT_MAX_CONSTANT_A].value;
  maxConstantB = optionsMap[OPT_MAX_CONSTANT_B].value;
  maxConstantC = optionsMap[OPT_MAX_CONSTANT_C].value;
  optScaleA = optionsMap[OPT_OPT_SCALE_A].value;
  optScaleB = optionsMap[OPT_OPT_SCALE_B].value;
  optScaleC = optionsMap[OPT_OPT_SCALE_C].value;
  optScaleD = optionsMap[OPT_OPT_SCALE_D].value;
  maxScaleA = optionsMap[OPT_MAX_SCALE_A].value;
  maxScaleB = optionsMap[OPT_MAX_SCALE_B].value;
  maximumTimeA = optionsMap[OPT_MAXIMUM_TIME_A].value;
  maximumTimeB = optionsMap[OPT_MAXIMUM_TIME_B].value;
}

// options_init() initializes the UCI options to their hard-coded default
// values.

void options_init()
{
  char *s;
  size_t len;

#ifdef NUMA
  // On a non-NUMA machine, disable the NUMA option to diminish confusion.
  if (!numaAvail)
    optionsMap[OPT_NUMA].type = OPT_TYPE_DISABLED;
#else
  optionsMap[OPT_NUMA].type = OPT_TYPE_DISABLED;
#endif
#ifdef _WIN32
  // Disable the LargePages option if the machine does not support it.
  if (!large_pages_supported())
    optionsMap[OPT_LARGE_PAGES].type = OPT_TYPE_DISABLED;
#endif
#if defined(__linux__) && !defined(MADV_HUGEPAGE)
  optionsMap[OPT_LARGE_PAGES].type = OPT_TYPE_DISABLED;
#endif
  optionsMap[OPT_SKILL_LEVEL].type = OPT_TYPE_DISABLED;
  if (sizeof(size_t) < 8) {
    optionsMap[OPT_SYZ_PROBE_LIMIT].def = 5;
    optionsMap[OPT_SYZ_PROBE_LIMIT].maxVal = 5;
  }
  for (Option *opt = optionsMap; opt->name != NULL; opt++) {
    if (opt->type == OPT_TYPE_DISABLED)
      continue;
    switch (opt->type) {
    case OPT_TYPE_CHECK:
    case OPT_TYPE_SPIN:
      opt->value = opt->def;
    case OPT_TYPE_BUTTON:
      break;
    case OPT_TYPE_STRING:
      opt->valString = strdup(opt->defString);
      break;
    case OPT_TYPE_COMBO:
      s = strstr(opt->defString, " var");
      len = strlen(opt->defString) - strlen(s);
      opt->valString = malloc(len + 1);
      strncpy(opt->valString, opt->defString, len);
      opt->valString[len] = 0;
      for (s = opt->valString; *s; s++)
        *s = tolower(*s);
      break;
    }
    if (opt->onChange)
      opt->onChange(opt);
  }
}

void options_free(void)
{
  for (Option *opt = optionsMap; opt->name != NULL; opt++)
    if (opt->type == OPT_TYPE_STRING)
      free(opt->valString);
}

static const char *optTypeStr[] = {
  "check", "spin", "button", "string", "combo"
};

// print_options() prints all options in the format required by the
// UCI protocol.

void print_options(void)
{
  for (Option *opt = optionsMap; opt->name != NULL; opt++) {
    if (opt->type == OPT_TYPE_DISABLED)
      continue;
    printf("option name %s type %s", opt->name, optTypeStr[opt->type]);
    switch (opt->type) {
    case OPT_TYPE_CHECK:
      printf(" default %s", opt->def ? "true" : "false");
      break;
    case OPT_TYPE_SPIN:
      printf(" default %d min %d max %d", opt->def, opt->minVal, opt->maxVal);
    case OPT_TYPE_BUTTON:
      break;
    case OPT_TYPE_STRING:
    case OPT_TYPE_COMBO:
      printf(" default %s", opt->defString);
      break;
    }
    printf("\n");
  }
  fflush(stdout);
}

int option_value(int optIdx)
{
  return optionsMap[optIdx].value;
}

const char *option_string_value(int optIdx)
{
  return optionsMap[optIdx].valString;
}

const char *option_default_string_value(int optIdx)
{
  return optionsMap[optIdx].defString;
}

void option_set_value(int optIdx, int value)
{
  Option *opt = &optionsMap[optIdx];

  opt->value = value;
  if (opt->onChange)
    opt->onChange(opt);
}

bool option_set_by_name(char *name, char *value)
{
  for (Option *opt = optionsMap; opt->name != NULL; opt++) {
    if (opt->type == OPT_TYPE_DISABLED)
      continue;
    if (strcasecmp(opt->name, name) == 0) {
      int val;
      switch (opt->type) {
      case OPT_TYPE_CHECK:
        if (strcmp(value, "true") == 0)
          opt->value = 1;
        else if (strcmp(value, "false") == 0)
          opt->value = 0;
        else
          return true;
        break;
      case OPT_TYPE_SPIN:
        val = atoi(value);
        if (val < opt->minVal || val > opt->maxVal)
          return true;
        opt->value = val;
      case OPT_TYPE_BUTTON:
        break;
      case OPT_TYPE_STRING:
        free(opt->valString);
        opt->valString = strdup(value);
        break;
      case OPT_TYPE_COMBO:
        free(opt->valString);
        opt->valString = strdup(value);
        for (char *s = opt->valString; *s; s++)
          *s = tolower(*s);
      }
      if (opt->onChange)
        opt->onChange(opt);
      return true;
    }
  }

  return false;
}
