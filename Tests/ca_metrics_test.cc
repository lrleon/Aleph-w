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
 * @file ca_metrics_test.cc
 * @brief Tests for `Aleph::CA` metric helpers (Phase 7).
 *
 * Scope:
 *   - count_state, count_alive, density on rectangular and graph
 *     lattices.
 *   - state_histogram and shannon_entropy edge cases.
 *   - frame_hash determinism: identical frames hash the same; the
 *     hash is stable across an entire test process.
 *   - frames_equal and cell_diff_count consistency with frame_hash.
 *   - for_each_cell visits every cell in row-major order.
 */

#include <cmath>
#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include <ca-metrics.H>
#include <ca-traits.H>
#include <tpl_ca_concepts.H>
#include <tpl_ca_graph_automaton.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_storage.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace {

using L2 = Lattice<Dense_Cell_Storage<int, 2>, OpenBoundary>;
using L1 = Lattice<Dense_Cell_Storage<int, 1>, OpenBoundary>;
using L3 = Lattice<Dense_Cell_Storage<int, 3>, OpenBoundary>;

}  // namespace

TEST(CAMetrics, CountStateAndCountAliveAreConsistent)
{
  L2 lat({4, 4}, 0);
  EXPECT_EQ(count_state(lat, 0), 16u);
  EXPECT_EQ(count_state(lat, 1), 0u);
  EXPECT_EQ(count_alive(lat), 0u);

  lat.set({1, 1}, 2);
  lat.set({2, 2}, 1);
  lat.set({3, 0}, 1);
  EXPECT_EQ(count_state(lat, 0), 13u);
  EXPECT_EQ(count_state(lat, 1), 2u);
  EXPECT_EQ(count_state(lat, 2), 1u);
  EXPECT_EQ(count_alive(lat), 3u);
}

TEST(CAMetrics, DensityIsZeroOnEmptyLattice)
{
  L2 empty({0, 0}, 0);
  EXPECT_DOUBLE_EQ(density(empty, 0), 0.0);
  EXPECT_DOUBLE_EQ(alive_density(empty), 0.0);
}

TEST(CAMetrics, DensityMatchesCountOverTotal)
{
  L2 lat({3, 3}, 0);
  lat.set({0, 0}, 1);
  lat.set({1, 1}, 1);
  lat.set({2, 2}, 1);
  EXPECT_DOUBLE_EQ(density(lat, 1), 3.0 / 9.0);
  EXPECT_DOUBLE_EQ(alive_density(lat), 3.0 / 9.0);
}

TEST(CAMetrics, StateHistogramSumsToTotal)
{
  L2 lat({4, 4}, 0);
  for (ca_index_t i = 0; i < 4; ++i)
    lat.set({i, i}, static_cast<int>(i % 3));
  const auto h = state_histogram(lat, 4);
  ASSERT_EQ(h.size(), 4u);
  ca_size_t total = 0;
  for (auto c : h)
    total += c;
  EXPECT_EQ(total, 16u);
}

TEST(CAMetrics, ShannonEntropyZeroForUniformLattice)
{
  L2 lat({4, 4}, 0);
  EXPECT_DOUBLE_EQ(shannon_entropy(lat, 4), 0.0);
}

TEST(CAMetrics, ShannonEntropyForBalancedBinaryReachesLn2)
{
  // Half 0, half 1. Entropy should be ln(2) nats.
  L2 lat({4, 4}, 0);
  for (ca_index_t i = 0; i < 4; ++i)
    for (ca_index_t j = 0; j < 4; ++j)
      lat.set({i, j}, ((i + j) & 1) != 0 ? 1 : 0);
  EXPECT_NEAR(shannon_entropy(lat, 2), std::log(2.0), 1e-12);
}

TEST(CAMetrics, FrameHashEqualForEqualFrames)
{
  L2 a({3, 3}, 0);
  L2 b({3, 3}, 0);
  EXPECT_EQ(frame_hash(a), frame_hash(b));

  a.set({1, 1}, 1);
  b.set({1, 1}, 1);
  EXPECT_EQ(frame_hash(a), frame_hash(b));
  EXPECT_TRUE(frames_equal(a, b));
}

TEST(CAMetrics, FrameHashDiffersForDifferentFrames)
{
  L2 a({3, 3}, 0);
  L2 b({3, 3}, 0);
  a.set({1, 1}, 1);
  b.set({2, 2}, 1);
  EXPECT_NE(frame_hash(a), frame_hash(b));
  EXPECT_FALSE(frames_equal(a, b));
}

TEST(CAMetrics, FrameHashStableAcrossLatticeReconstruction)
{
  // Build the same frame twice from independent storage objects;
  // the hash must coincide.
  auto build = []() {
    L2 lat({4, 4}, 0);
    for (ca_index_t i = 0; i < 4; ++i)
      for (ca_index_t j = 0; j < 4; ++j)
        lat.set({i, j}, ((i * 4 + j) % 3 == 0) ? 1 : 0);
    return lat;
  };
  EXPECT_EQ(frame_hash(build()), frame_hash(build()));
}

TEST(CAMetrics, CellDiffCountMatchesActivity)
{
  L2 a({4, 4}, 0);
  L2 b({4, 4}, 0);
  EXPECT_EQ(cell_diff_count(a, b), 0u);

  a.set({0, 0}, 1);
  a.set({1, 1}, 1);
  a.set({2, 2}, 1);
  EXPECT_EQ(cell_diff_count(a, b), 3u);

  b.set({0, 0}, 1);
  b.set({3, 3}, 1);
  // a has alive at (0,0),(1,1),(2,2); b has alive at (0,0),(3,3).
  // Differences: (1,1), (2,2), (3,3) → 3 diffs.
  EXPECT_EQ(cell_diff_count(a, b), 3u);
}

TEST(CAMetrics, ForEachCellWalksRank1And3LatticesToo)
{
  L1 a({5}, 0);
  for (ca_index_t i = 0; i < 5; ++i)
    a.set({i}, static_cast<int>(i));
  std::vector<int> seen;
  for_each_cell(a, [&](int v) { seen.push_back(v); });
  EXPECT_EQ(seen, (std::vector<int>{0, 1, 2, 3, 4}));

  L3 b({2, 2, 2}, 0);
  ca_size_t cnt = 0;
  for_each_cell(b, [&](int) { ++cnt; });
  EXPECT_EQ(cnt, 8u);
}

TEST(CAMetrics, MetricsWorkOnGraphLattice)
{
  Graph_Lattice<int> g(make_path_graph_adjacency(5, true), 0);
  g.set_node(0, 1);
  g.set_node(2, 1);
  EXPECT_EQ(count_state(g, 1), 2u);
  EXPECT_EQ(count_alive(g), 2u);
  EXPECT_DOUBLE_EQ(density(g, 1), 0.4);

  // Hash + equality.
  Graph_Lattice<int> g2(make_path_graph_adjacency(5, true), 0);
  g2.set_node(0, 1);
  g2.set_node(2, 1);
  EXPECT_EQ(frame_hash(g), frame_hash(g2));
  EXPECT_TRUE(frames_equal(g, g2));

  g2.set_node(4, 1);
  EXPECT_NE(frame_hash(g), frame_hash(g2));
  EXPECT_EQ(cell_diff_count(g, g2), 1u);
}