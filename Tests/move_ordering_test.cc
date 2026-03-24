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
  table.record(0, 10);

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

  EXPECT_EQ(moves[0].move, 20);
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