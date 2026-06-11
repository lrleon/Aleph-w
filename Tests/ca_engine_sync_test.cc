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

/**
 * @file ca_engine_sync_test.cc
 * @brief Phase 3 tests for the synchronous double-buffered CA engine.
 */

# include <array>
# include <initializer_list>
# include <set>
# include <span>
# include <utility>
# include <vector>

# include <gtest/gtest.h>

# include <ca-traits.H>
# include <tpl_ca_storage.H>
# include <tpl_ca_lattice.H>
# include <tpl_ca_neighborhood.H>
# include <tpl_ca_rule.H>
# include <tpl_ca_engine.H>
# include <ca-engine-utils.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace
{
  using Grid = Lattice<Dense_Cell_Storage<int, 2>, OpenBoundary>;
  using Engine = Synchronous_Engine<Grid, Game_Of_Life_Rule, Moore<2, 1>>;
  using Cell = std::pair<ca_index_t, ca_index_t>;

  std::set<Cell> to_set(std::initializer_list<Cell> cells)
  {
    return std::set<Cell>(cells.begin(), cells.end());
  }

  Grid make_grid(ca_size_t rows, ca_size_t cols,
                 std::initializer_list<Cell> alive)
  {
    Grid g({ rows, cols }, 0);
    for (const auto & c : alive)
      g.set({ c.first, c.second }, 1);
    return g;
  }

  Grid make_grid(ca_size_t rows, ca_size_t cols,
                 const std::vector<Cell> & alive)
  {
    Grid g({ rows, cols }, 0);
    for (const auto & c : alive)
      g.set({ c.first, c.second }, 1);
    return g;
  }

  std::set<Cell> alive_cells(const Grid & g)
  {
    std::set<Cell> out;
    const auto rows = static_cast<ca_index_t>(g.size(0));
    const auto cols = static_cast<ca_index_t>(g.size(1));
    for (ca_index_t i = 0; i < rows; ++i)
      for (ca_index_t j = 0; j < cols; ++j)
        if (g.at({ i, j }) != 0)
          out.insert({ i, j });
    return out;
  }

  Grid manual_life_step(const Grid & cur)
  {
    Grid nxt(cur.extents(), 0);
    auto rule = make_game_of_life_rule();
    Moore<2, 1> nh;
    std::array<int, Moore<2, 1>::size_v> buf { };

    const auto rows = static_cast<ca_index_t>(cur.size(0));
    const auto cols = static_cast<ca_index_t>(cur.size(1));
    for (ca_index_t i = 0; i < rows; ++i)
      for (ca_index_t j = 0; j < cols; ++j)
        {
          gather_neighbors(nh, cur, Coord_Vec<2>{ i, j },
                           std::span<int>(buf.data(), buf.size()));
          nxt.set({ i, j },
                  rule(cur.at({ i, j }),
                       Neighbor_View<int>(buf.data(), buf.size())));
        }
    return nxt;
  }

  Engine make_engine(Grid initial)
  {
    return Engine(std::move(initial), make_game_of_life_rule(), Moore<2, 1>{});
  }

  void expect_matches_reference_for_twenty_steps(Grid initial,
                                                 const char * name)
  {
    Grid ref = initial;
    auto engine = make_engine(std::move(initial));
    for (std::size_t t = 0; t < 20; ++t)
      {
        ASSERT_EQ(alive_cells(engine.frame()), alive_cells(ref))
          << name << " before step " << t;
        engine.step();
        ref = manual_life_step(ref);
      }
    EXPECT_EQ(alive_cells(engine.frame()), alive_cells(ref))
      << name << " after 20 steps";
    EXPECT_EQ(engine.steps_run(), 20u);
  }

  Grid make_pulsar()
  {
    Grid g({ 17, 17 }, 0);
    constexpr int offset = 2;
    const std::array<int, 4> sparse_rows = {{ 0, 5, 7, 12 }};
    const std::array<int, 6> dense_cols = {{ 2, 3, 4, 8, 9, 10 }};
    for (const int r : sparse_rows)
      for (const int c : dense_cols)
        g.set({ offset + r, offset + c }, 1);

    const std::array<int, 6> dense_rows = {{ 2, 3, 4, 8, 9, 10 }};
    const std::array<int, 4> sparse_cols = {{ 0, 5, 7, 12 }};
    for (const int r : dense_rows)
      for (const int c : sparse_cols)
        g.set({ offset + r, offset + c }, 1);
    return g;
  }

  std::vector<Cell> glider_cells()
  {
    return { { 1, 2 }, { 2, 3 }, { 3, 1 }, { 3, 2 }, { 3, 3 } };
  }

  std::set<Cell> shifted(const std::vector<Cell> & cells,
                         ca_index_t dr, ca_index_t dc)
  {
    std::set<Cell> out;
    for (const auto & c : cells)
      out.insert({ c.first + dr, c.second + dc });
    return out;
  }
}

TEST(CAEngineSync, MatchesManualReferenceForCanonicalPatterns)
{
  expect_matches_reference_for_twenty_steps(
    make_grid(5, 5, { { 2, 1 }, { 2, 2 }, { 2, 3 } }),
    "blinker");
  expect_matches_reference_for_twenty_steps(
    make_grid(6, 6, { { 2, 2 }, { 2, 3 }, { 2, 4 },
                      { 3, 1 }, { 3, 2 }, { 3, 3 } }),
    "toad");
  expect_matches_reference_for_twenty_steps(
    make_grid(6, 6, { { 1, 1 }, { 1, 2 }, { 2, 1 }, { 2, 2 },
                      { 3, 3 }, { 3, 4 }, { 4, 3 }, { 4, 4 } }),
    "beacon");
  expect_matches_reference_for_twenty_steps(
    make_grid(10, 10, glider_cells()),
    "glider");
  expect_matches_reference_for_twenty_steps(make_pulsar(), "pulsar");
}

TEST(CAEngineSync, OscillatorsFollowKnownPeriodsForTwentySteps)
{
  auto blinker = make_engine(
    make_grid(5, 5, { { 2, 1 }, { 2, 2 }, { 2, 3 } }));
  auto toad = make_engine(
    make_grid(6, 6, { { 2, 2 }, { 2, 3 }, { 2, 4 },
                      { 3, 1 }, { 3, 2 }, { 3, 3 } }));
  auto beacon = make_engine(
    make_grid(6, 6, { { 1, 1 }, { 1, 2 }, { 2, 1 }, { 2, 2 },
                      { 3, 3 }, { 3, 4 }, { 4, 3 }, { 4, 4 } }));

  const auto blinker0 = to_set({ { 2, 1 }, { 2, 2 }, { 2, 3 } });
  const auto blinker1 = to_set({ { 1, 2 }, { 2, 2 }, { 3, 2 } });
  const auto toad0 = to_set({ { 2, 2 }, { 2, 3 }, { 2, 4 },
                              { 3, 1 }, { 3, 2 }, { 3, 3 } });
  const auto toad1 = to_set({ { 1, 3 }, { 2, 1 }, { 2, 4 },
                              { 3, 1 }, { 3, 4 }, { 4, 2 } });
  const auto beacon0 = to_set({ { 1, 1 }, { 1, 2 }, { 2, 1 }, { 2, 2 },
                                { 3, 3 }, { 3, 4 }, { 4, 3 }, { 4, 4 } });
  const auto beacon1 = to_set({ { 1, 1 }, { 1, 2 }, { 2, 1 },
                                { 3, 4 }, { 4, 3 }, { 4, 4 } });

  for (std::size_t t = 0; t <= 20; ++t)
    {
      EXPECT_EQ(alive_cells(blinker.frame()), t % 2 == 0 ? blinker0 : blinker1)
        << "blinker t=" << t;
      EXPECT_EQ(alive_cells(toad.frame()), t % 2 == 0 ? toad0 : toad1)
        << "toad t=" << t;
      EXPECT_EQ(alive_cells(beacon.frame()), t % 2 == 0 ? beacon0 : beacon1)
        << "beacon t=" << t;
      if (t < 20)
        {
          blinker.step();
          toad.step();
          beacon.step();
        }
    }
}

TEST(CAEngineSync, GliderMovesOneCellDiagonallyEveryFourSteps)
{
  const auto cells = glider_cells();
  auto engine = make_engine(make_grid(10, 10, cells));

  for (std::size_t t = 0; t <= 20; ++t)
    {
      if (t % 4 == 0)
        {
          const auto d = static_cast<ca_index_t>(t / 4);
          EXPECT_EQ(alive_cells(engine.frame()), shifted(cells, d, d))
            << "glider t=" << t;
        }
      if (t < 20)
        engine.step();
    }
}

TEST(CAEngineSync, PulsarKeepsPeriodThree)
{
  const auto initial = alive_cells(make_pulsar());
  auto engine = make_engine(make_pulsar());

  for (std::size_t t = 1; t <= 20; ++t)
    {
      engine.step();
      if (t % 3 == 0)
        EXPECT_EQ(alive_cells(engine.frame()), initial) << "pulsar t=" << t;
    }
  EXPECT_EQ(engine.steps_run(), 20u);
}

TEST(CAEngineSync, HooksObservePreAndPostStepFrames)
{
  auto engine = make_engine(
    make_grid(5, 5, { { 2, 1 }, { 2, 2 }, { 2, 3 } }));
  std::vector<std::size_t> pre_steps;
  std::vector<std::size_t> post_steps;
  std::vector<std::size_t> pre_alive;
  std::vector<std::size_t> post_alive;

  engine.on_pre_step([&](std::size_t step, const Grid & frame)
    {
      pre_steps.push_back(step);
      pre_alive.push_back(alive_cells(frame).size());
    });
  engine.on_post_step([&](std::size_t step, const Grid & frame)
    {
      post_steps.push_back(step);
      post_alive.push_back(alive_cells(frame).size());
    });

  engine.run(2);

  EXPECT_EQ(pre_steps, (std::vector<std::size_t>{ 0, 1 }));
  EXPECT_EQ(post_steps, (std::vector<std::size_t>{ 1, 2 }));
  EXPECT_EQ(pre_alive, (std::vector<std::size_t>{ 3, 3 }));
  EXPECT_EQ(post_alive, (std::vector<std::size_t>{ 3, 3 }));
  EXPECT_EQ(engine.steps_run(), 2u);
}

TEST(CAEngineSync, TiledAndColumnMajorOrdersMatchRowMajor)
{
  using Tiled_Engine =
    Synchronous_Engine<Grid, Game_Of_Life_Rule, Moore<2, 1>, Tile<4, 3>>;
  using Column_Engine =
    Synchronous_Engine<Grid, Game_Of_Life_Rule, Moore<2, 1>, ColumnMajor>;

  Grid initial = make_pulsar();
  auto row_major = make_engine(initial);
  Tiled_Engine tiled(initial, make_game_of_life_rule(), Moore<2, 1>{});
  Column_Engine column(std::move(initial), make_game_of_life_rule(),
                       Moore<2, 1>{});

  for (std::size_t t = 0; t <= 20; ++t)
    {
      const auto expected = alive_cells(row_major.frame());
      EXPECT_EQ(alive_cells(tiled.frame()), expected) << "tiled t=" << t;
      EXPECT_EQ(alive_cells(column.frame()), expected) << "column t=" << t;
      if (t < 20)
        {
          row_major.step();
          tiled.step();
          column.step();
        }
    }
}

TEST(CAEngineUtils, WolframFactoryRunsSeededRule90Impulse)
{
  Lattice<Dense_Cell_Storage<int, 1>, OpenBoundary> row({ 7 }, 0);
  row.set({ 3 }, 1);
  auto engine = make_wolfram_engine(90, std::move(row));

  engine.step();

  EXPECT_EQ(engine.frame().at({ 0 }), 0);
  EXPECT_EQ(engine.frame().at({ 1 }), 0);
  EXPECT_EQ(engine.frame().at({ 2 }), 1);
  EXPECT_EQ(engine.frame().at({ 3 }), 0);
  EXPECT_EQ(engine.frame().at({ 4 }), 1);
  EXPECT_EQ(engine.frame().at({ 5 }), 0);
  EXPECT_EQ(engine.frame().at({ 6 }), 0);
  EXPECT_EQ(engine.steps_run(), 1u);
}
