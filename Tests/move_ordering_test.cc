/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include <random>
#include <string>

#include <gtest/gtest.h>

#include <search_move_ordering.H>

using namespace Aleph;

// --- Killer_Move_Table tests ---

TEST(KillerMoveTable, EmptyTableReportsFalse)
{
  Killer_Move_Table<int> table;
  EXPECT_FALSE(table.is_killer(0, 42));
  EXPECT_FALSE(table.is_killer(5, 99));
}

TEST(KillerMoveTable, RecordAndQuery)
{
  Killer_Move_Table<int> table;
  table.record(0, 10);

  EXPECT_TRUE(table.is_killer(0, 10));
  EXPECT_FALSE(table.is_killer(0, 20));
  EXPECT_FALSE(table.is_killer(1, 10));
}

TEST(KillerMoveTable, TwoSlotsPerDepth)
{
  Killer_Move_Table<int> table;
  table.record(0, 10);
  table.record(0, 20);

  EXPECT_TRUE(table.is_killer(0, 10));
  EXPECT_TRUE(table.is_killer(0, 20));
}

TEST(KillerMoveTable, ThirdRecordEvictsOldest)
{
  Killer_Move_Table<int> table;
  table.record(0, 10);
  table.record(0, 20);
  table.record(0, 30);

  EXPECT_FALSE(table.is_killer(0, 10));
  EXPECT_TRUE(table.is_killer(0, 20));
  EXPECT_TRUE(table.is_killer(0, 30));
}

TEST(KillerMoveTable, DuplicateRecordIsNoOp)
{
  Killer_Move_Table<int> table;
  table.record(0, 10);
  table.record(0, 20);
  // 20 is now primary; recording it again must be a no-op (no rotation).
  table.record(0, 20);

  EXPECT_TRUE(table.is_killer(0, 10));
  EXPECT_TRUE(table.is_killer(0, 20));
}

TEST(KillerMoveTable, IndependentDepths)
{
  Killer_Move_Table<int> table;
  table.record(0, 10);
  table.record(1, 20);

  EXPECT_TRUE(table.is_killer(0, 10));
  EXPECT_FALSE(table.is_killer(0, 20));
  EXPECT_TRUE(table.is_killer(1, 20));
  EXPECT_FALSE(table.is_killer(1, 10));
}

TEST(KillerMoveTable, ClearResetsAll)
{
  Killer_Move_Table<int> table;
  table.record(0, 10);
  table.record(1, 20);
  table.clear();

  EXPECT_FALSE(table.is_killer(0, 10));
  EXPECT_FALSE(table.is_killer(1, 20));
}

TEST(KillerMoveTable, SparseDepthAccess)
{
  Killer_Move_Table<int> table;
  table.record(100, 42);
  EXPECT_TRUE(table.is_killer(100, 42));
  EXPECT_FALSE(table.is_killer(50, 42));
}

TEST(KillerMoveTable, NonComparableMovesUseNoOpTable)
{
  struct Opaque
  {
    int value;
  };

  Killer_Move_Table<Opaque> table;
  static_assert(not Killer_Move_Table<Opaque>::supported);
  EXPECT_FALSE(table.is_killer(0, Opaque{1}));
  table.record(0, Opaque{1});
  EXPECT_FALSE(table.is_killer(0, Opaque{1}));
}

TEST(KillerMoveTable, StringMoves)
{
  Killer_Move_Table<std::string> table;
  static_assert(Killer_Move_Table<std::string>::supported);

  table.record(0, "e2e4");
  table.record(0, "d2d4");

  EXPECT_TRUE(table.is_killer(0, "e2e4"));
  EXPECT_TRUE(table.is_killer(0, "d2d4"));
  EXPECT_FALSE(table.is_killer(0, "c2c4"));
}

// --- History_Heuristic_Table tests ---

TEST(HistoryHeuristicTable, EmptyTableReturnsZero)
{
  History_Heuristic_Table<int> table;
  EXPECT_EQ(table.score(42), 0u);
}

TEST(HistoryHeuristicTable, RecordAndScore)
{
  History_Heuristic_Table<int> table;
  table.record(10, 5);
  EXPECT_EQ(table.score(10), 5u);
}

TEST(HistoryHeuristicTable, AccumulatesBonus)
{
  History_Heuristic_Table<int> table;
  table.record(10, 3);
  table.record(10, 7);
  EXPECT_EQ(table.score(10), 10u);
}

TEST(HistoryHeuristicTable, DefaultBonusIsOne)
{
  History_Heuristic_Table<int> table;
  table.record(10);
  table.record(10);
  table.record(10);
  EXPECT_EQ(table.score(10), 3u);
}

TEST(HistoryHeuristicTable, IndependentKeys)
{
  History_Heuristic_Table<int> table;
  table.record(1, 10);
  table.record(2, 20);

  EXPECT_EQ(table.score(1), 10u);
  EXPECT_EQ(table.score(2), 20u);
  EXPECT_EQ(table.score(3), 0u);
}

TEST(HistoryHeuristicTable, ClearResetsAll)
{
  History_Heuristic_Table<int> table;
  table.record(1, 100);
  table.record(2, 200);
  table.clear();

  EXPECT_EQ(table.score(1), 0u);
  EXPECT_EQ(table.score(2), 0u);
}

TEST(HistoryHeuristicTable, NullTableIsNoOp)
{
  Null_History_Heuristic_Table table;
  static_assert(not Null_History_Heuristic_Table::supported);
  EXPECT_EQ(table.score(42), 0u);
  table.record(42, 100);
  EXPECT_EQ(table.score(42), 0u);
}

// --- sort_ranked_moves tests ---

TEST(SortRankedMoves, SortsByPriorityDescending)
{
  Array<RankedMove<int, int>> moves;
  moves.append({10, 1, 0, false, 0});
  moves.append({20, 3, 1, false, 0});
  moves.append({30, 2, 2, false, 0});

  sort_ranked_moves(moves,
                    [](int a, int b) { return a > b; },
                    false, false);

  EXPECT_EQ(moves[0].move, 20);
  EXPECT_EQ(moves[1].move, 30);
  EXPECT_EQ(moves[2].move, 10);
}

TEST(SortRankedMoves, KillerMovesFirst)
{
  Array<RankedMove<int, int>> moves;
  moves.append({10, 5, 0, false, 0});
  moves.append({20, 1, 1, true, 0});
  moves.append({30, 3, 2, false, 0});

  sort_ranked_moves(moves,
                    [](int a, int b) { return a > b; },
                    true, false);

  EXPECT_EQ(moves[0].move, 20);  // killer first
  EXPECT_EQ(moves[1].move, 10);  // priority 5 > 3
  EXPECT_EQ(moves[2].move, 30);  // priority 3
}

TEST(SortRankedMoves, HistoryBreaksTies)
{
  Array<RankedMove<int, int>> moves;
  moves.append({10, 5, 0, false, 10});
  moves.append({20, 5, 1, false, 50});
  moves.append({30, 5, 2, false, 30});

  sort_ranked_moves(moves,
                    [](int a, int b) { return a > b; },
                    false, true);

  EXPECT_EQ(moves[0].move, 20);
  EXPECT_EQ(moves[1].move, 30);
  EXPECT_EQ(moves[2].move, 10);
}

TEST(SortRankedMoves, OrdinalBreaksTiesWhenEverythingEqual)
{
  Array<RankedMove<int, int>> moves;
  moves.append({10, 5, 2, false, 0});
  moves.append({20, 5, 0, false, 0});
  moves.append({30, 5, 1, false, 0});

  sort_ranked_moves(moves,
                    [](int a, int b) { return a > b; },
                    false, false);

  EXPECT_EQ(moves[0].move, 20);
  EXPECT_EQ(moves[1].move, 30);
  EXPECT_EQ(moves[2].move, 10);
}

TEST(SortRankedMoves, KillerThenHistoryThenPriority)
{
  Array<RankedMove<int, int>> moves;
  moves.append({1, 100, 0, false, 0});   // high priority, no killer, no history
  moves.append({2, 1,   1, true, 0});    // low priority, killer
  moves.append({3, 50,  2, false, 999}); // medium priority, high history
  moves.append({4, 90,  3, false, 1});   // high-ish priority, low history

  sort_ranked_moves(moves,
                    [](int a, int b) { return a > b; },
                    true, true);

  EXPECT_EQ(moves[0].move, 2);  // killer first
  EXPECT_EQ(moves[1].move, 3);  // history 999
  EXPECT_EQ(moves[2].move, 4);  // history 1
  EXPECT_EQ(moves[3].move, 1);  // history 0, highest priority among remaining
}

TEST(SortRankedMoves, SingleElementIsNoOp)
{
  Array<RankedMove<int, int>> moves;
  moves.append({42, 1, 0, false, 0});

  sort_ranked_moves(moves,
                    [](int a, int b) { return a > b; },
                    true, true);

  EXPECT_EQ(moves[0].move, 42);
}

TEST(SortRankedMoves, EmptyIsNoOp)
{
  Array<RankedMove<int, int>> moves;

  sort_ranked_moves(moves,
                    [](int a, int b) { return a > b; },
                    true, true);

  EXPECT_TRUE(moves.is_empty());
}

// --- Large-input edge-case coverage ---

TEST(KillerMoveTable, LargeDepthRange)
{
  Killer_Move_Table<int> table;
  constexpr size_t N = 2000;

  // Record one move per depth across a large range.
  for (size_t d = 0; d < N; ++d)
    table.record(d, static_cast<int>(d * 10));

  for (size_t d = 0; d < N; ++d)
    EXPECT_TRUE(table.is_killer(d, static_cast<int>(d * 10)));

  // Unrecorded moves must not appear.
  for (size_t d = 0; d < N; ++d)
    EXPECT_FALSE(table.is_killer(d, -1));

  table.clear();
  for (size_t d = 0; d < N; ++d)
    EXPECT_FALSE(table.is_killer(d, static_cast<int>(d * 10)));
}

TEST(KillerMoveTable, EvictionUnderManyInsertsPerDepth)
{
  Killer_Move_Table<int> table;
  // Insert 10 distinct moves at depth 0; only the last 2 should survive.
  for (int i = 1; i <= 10; ++i)
    table.record(0, i);

  EXPECT_FALSE(table.is_killer(0, 1));
  EXPECT_FALSE(table.is_killer(0, 8));
  EXPECT_TRUE(table.is_killer(0, 9));
  EXPECT_TRUE(table.is_killer(0, 10));
}

TEST(KillerMoveTable, NonComparableLargeInput)
{
  struct Opaque { int v; };
  Killer_Move_Table<Opaque> table;
  static_assert(not Killer_Move_Table<Opaque>::supported);

  for (size_t d = 0; d < 500; ++d)
    {
      table.record(d, Opaque{static_cast<int>(d)});
      EXPECT_FALSE(table.is_killer(d, Opaque{static_cast<int>(d)}));
    }
  table.clear();  // must not crash
}

TEST(HistoryHeuristicTable, LargeKeySet)
{
  History_Heuristic_Table<int> table;
  constexpr int N = 5000;

  for (int k = 0; k < N; ++k)
    table.record(k, static_cast<size_t>(k + 1));

  for (int k = 0; k < N; ++k)
    EXPECT_EQ(table.score(k), static_cast<size_t>(k + 1));

  table.clear();
  for (int k = 0; k < N; ++k)
    EXPECT_EQ(table.score(k), 0u);
}

TEST(HistoryHeuristicTable, AccumulatesLargeBonuses)
{
  History_Heuristic_Table<int> table;
  constexpr size_t REPS = 1000;

  for (size_t i = 0; i < REPS; ++i)
    table.record(42, 3);

  EXPECT_EQ(table.score(42), REPS * 3);
}

TEST(HistoryHeuristicTable, NullTableLargeInput)
{
  Null_History_Heuristic_Table table;
  static_assert(not Null_History_Heuristic_Table::supported);

  for (int k = 0; k < 1000; ++k)
    table.record(k, 99);

  for (int k = 0; k < 1000; ++k)
    EXPECT_EQ(table.score(k), 0u);

  table.clear();  // must not crash
}

// --- Randomized stress tests for sort_ranked_moves ---

TEST(SortRankedMoves, RandomizedStress)
{
  constexpr size_t N      = 1000;
  constexpr unsigned SEED = 42u;
  std::mt19937 rng(SEED);

  auto rand_int  = [&](int lo, int hi)
    { return std::uniform_int_distribution<int>(lo, hi)(rng); };
  auto rand_uint = [&](size_t lo, size_t hi)
    { return static_cast<size_t>(std::uniform_int_distribution<size_t>(lo, hi)(rng)); };
  auto rand_bool = [&]() { return static_cast<bool>(rng() & 1u); };

  for (bool killer_first : {false, true})
    for (bool use_history : {false, true})
      {
        Array<RankedMove<int, int>> moves;
        for (size_t i = 0; i < N; ++i)
          moves.append(RankedMove<int,int>{rand_int(0, 100),
                                          rand_int(0, 50),
                                          i,
                                          rand_bool(),
                                          rand_uint(0, 200)});

        sort_ranked_moves(moves,
                          [](int a, int b) { return a > b; },
                          killer_first, use_history);

        ASSERT_EQ(moves.size(), N);

        for (size_t i = 0; i + 1 < N; ++i)
          {
            const auto &a = moves[i];
            const auto &b = moves[i + 1];

            if (killer_first)
              {
                if (a.killer and not b.killer)
                  continue;  // correct: killer before non-killer
                if (not a.killer and b.killer)
                  FAIL() << "Non-killer before killer at index " << i;
              }

            if (a.killer == b.killer)
              {
                if (use_history and a.history_score != b.history_score)
                  EXPECT_GE(a.history_score, b.history_score)
                    << "History ordering violated at index " << i;
                else if (not use_history)
                  {
                    if (a.priority != b.priority)
                      EXPECT_GE(a.priority, b.priority)
                        << "Priority ordering violated at index " << i;
                    else
                      EXPECT_LE(a.ordinal, b.ordinal)
                        << "Ordinal tiebreak violated at index " << i;
                  }
              }
          }
      }
}