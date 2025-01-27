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

#ifndef SEARCH_H
#define SEARCH_H

#include "misc.h"
#include "position.h"
#include "thread.h"
#include "types.h"

// RootMove struct is used for moves at the root of the tree. For each root
// move we store a score and a PV (really a refutation in the case of moves
// which fail low). Score is normally set at -VALUE_INFINITE for all non-pv
// moves.

struct RootMove {
  int pvSize;
  Value score;
  Value previousScore;
  Value averageScore;
  int selDepth;
  int tbRank;
  Value tbScore;
  Move pv[MAX_PLY];
};

typedef struct RootMove RootMove;

struct RootMoves {
  int size;
  RootMove move[MAX_MOVES];
};

typedef struct RootMoves RootMoves;

/// LimitsType struct stores information sent by GUI about available time to
/// search the current move, maximum depth/time, if we are in analysis mode or
/// if we have to ponder while it's our opponent's turn to move.

struct LimitsType {
  int time[2];
  int inc[2];
  int npmsec;
  int movestogo;
  int depth;
  int movetime;
  int mate;
  bool infinite;
  uint64_t nodes;
  TimePoint startTime;
  int numSearchmoves;
  Move searchmoves[MAX_MOVES];
};

typedef struct LimitsType LimitsType;

extern LimitsType Limits;

INLINE int use_time_management(void)
{
  return Limits.time[WHITE] || Limits.time[BLACK];
}

void search_init(void);
void search_clear(void);
uint64_t perft(Position *pos, Depth depth);
void start_thinking(Position *pos, bool ponderMode);

extern int futilityMarginGain;  // 165
extern int reductionA;  // 1642
extern int reductionB;  // 1024
extern int reductionC;  // 916
extern int statBonusA;  // 12
extern int statBonusB;  // 282
extern int statBonusC;  // 349
extern int statBonusD;  // 1594
extern int reductionInit;  // 2026
extern int counterMoveHistoryThreshold;  // -1
extern int aspirationDeltaA;  // 10
extern int aspirationDeltaB;  // 15620
extern int aspirationDeltaC;  // 4 d
extern int aspirationDeltaD;  // 2
extern int bonusInitialGain;  // -19
extern int bonusInitialThreshold;  // 1914
extern int improvementDefault;  // 168
extern int razoringA;  // -369
extern int razoringB;  // -254
extern int futilityA;  // 303 d
extern int futilityDepth;  // 8
extern int nullMoveThreshA;  // 17139
extern int nullMoveThreshB;  // -20
extern int nullMoveThreshC;  // 13 d
extern int nullMoveThreshD;  // 233
extern int nullMoveThreshE;  // 25 d
extern int nullMoveRA;  // 168 d
extern int nullMoveRB;  // 7
extern int nullMoveRC;  // 3 d
extern int nullMoveRD;  // 4
extern int nullMoveRE;  // 861
extern int nullMoveDepth;  // 14
extern int nullMovePlyA;  // 3
extern int nullMovePlyB;  // 4 d
extern int probCutBetaA;  // 191
extern int probCutBetaB;  // 54
extern int probCutDepthLimit;  // 4
extern int probCutDepth;  // 3
extern int ttDecreaseA;  // 3
extern int ttDecreaseB;  // 2
extern int ttDecreaseDepth;  // 9
extern int probCutBetaC;  // 417
extern int probCutDepthThresh;  // 2
extern int shallowPruningDepthA;  // 7
extern int shallowPruningA;  // 180
extern int shallowPruningB;  // 201
extern int shallowPruningC;  // 6 d
extern int sseThreshold;  // -222
extern int shallowPruningDepthB;  // 5
extern int shallowPruningD;  // -3875
extern int shallowPruningGain;  // 2
extern int shallowPruningDepthC;  // 13
extern int shallowPruningE;  // 106
extern int shallowPruningF;  // 145
extern int shallowPruningG;  // 52 d
extern int shallowPruningH;  // -24
extern int shallowPruningI;  // -15
extern int singularExtDepthA;  // 4
extern int singularExtDepthB;  // 2
extern int singularExtDepthC;  // 3
extern int singularBetaA;  // 3
extern int singularExtentionA;  // 25
extern int singularExtentionB;  // 9
extern int singularExtDepthD;  // 9
extern int singularExtentionC;  // 82
extern int singularExtentionD;  // 5177
extern int lmrDepthThreshold;  // 2
extern int lmrMoveCountThreshold;  // 7
extern int lmrDecTTPv;  // 2
extern int lmrDecMoveCount;  // 1
extern int lmrDecSingular;  // 1
extern int lmrIncCutNode;  // 2
extern int lmrIncTTCapture;  // 1
extern int lmrPvNodeA;  // 1
extern int lmrPvNodeB;  // 11
extern int lmrPvNodeC;  // 3
extern int lmrCutoffCntThresh;  // 3
extern int lmrIncCutoffCnt;  // 1
extern int lmrStatGain;  // 2
extern int lmrStatDelta;  // 4433
extern int lmrRDecA;  // 13628
extern int lmrRDecB;  // 4000
extern int lmrRDecDepthA;  // 7
extern int lmrRDecDepthB;  // 19
extern int lmrDeepSearchA;  // 64
extern int lmrDeepSearchB;  // 11
extern int mateBetaDelta;  // 137
extern int mateDepthThreshold;  // 5
extern int mateExtraBonus;  // 62
extern int futilityBaseDelta;  // 153
extern int fallingEvalA;  // 66
extern int fallingEvalB;  // 14
extern int fallingEvalC;  // 6
extern int fallingEvalD;  // 617
extern int fallingEvalClampMin;  // 51
extern int fallingEvalClampMax;  // 151
extern int timeReductionDepth;  // 8
extern int timeReductionA;  // 156
extern int timeReductionB;  // 69
extern int timeReductionC;  // 140
extern int timeReductionD;  // 217
extern int bestMoveInstabilityA; // 179
extern int totalTimeGain;  // 75
extern int optExtraA;  // 100
extern int optExtraB;  // 125
extern int optExtraC;  // 111
extern int optConstantA;  // 334
extern int optConstantB;  // 30
extern int optConstantC;  // 49
extern int maxConstantA;  // 340
extern int maxConstantB;  // 300
extern int maxConstantC;  // 276
extern int optScaleA;  // 120
extern int optScaleB;  // 31
extern int optScaleC;  // 44
extern int optScaleD;  // 21
extern int maxScaleA;  // 69
extern int maxScaleB;  // 122
extern int maximumTimeA;  // 84
extern int maximumTimeB;  // 10

#endif
