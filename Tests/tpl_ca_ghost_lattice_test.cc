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
 * @file tpl_ca_ghost_lattice_test.cc
 * @brief Full-coverage tests for `Aleph::CA::Ghost_Lattice` (Phase 4).
 *
 * Scope:
 *   - Concept conformance (`LatticeLike`).
 *   - Strict `at`/`set` bounds-check semantics.
 *   - `refresh_halo` fills the halo correctly for every policy.
 *   - `at_safe` on the ghost lattice matches the classic `Lattice` at
 *     every in-halo coordinate, for every boundary policy (`Open`,
 *     `Periodic`/`Toroidal`, `Reflective`, `Dirichlet`/`Constant`,
 *     `Neumann`).
 *   - Engine equivalence: running Conway's Game of Life through the
 *     `Synchronous_Engine` for 100+ steps on a classic `Lattice` and
 *     on a `Ghost_Lattice` yields bit-identical states.
 *   - Halo radius >= 2 also works.
 *   - 1D and 3D sanity checks.
 */

#include <array>
#include <cstdint>
#include <stdexcept>

#include <gtest/gtest.h>

#include <ca-traits.H>
#include <tpl_ca_concepts.H>
#include <tpl_ca_storage.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_ghost_lattice.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_rule.H>
#include <tpl_ca_engine.H>

using namespace Aleph;
using namespace Aleph::CA;

// ---------------------------------------------------------------------------
// Concept conformance.
// ---------------------------------------------------------------------------

static_assert(LatticeLike<Ghost_Lattice<Dense_Cell_Storage<int, 2>, OpenBoundary, 1>>);
static_assert(LatticeLike<Ghost_Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary, 1>>);
static_assert(LatticeLike<Ghost_Lattice<Dense_Cell_Storage<int, 2>, ReflectiveBoundary, 2>>);
static_assert(LatticeLike<Ghost_Lattice<Dense_Cell_Storage<int, 2>, NeumannBoundary, 1>>);
static_assert(
  LatticeLike<Ghost_Lattice<Dense_Cell_Storage<int, 2>, ConstantBoundary<int, 7>, 1>>);

// ---------------------------------------------------------------------------
// Small helpers used throughout this file.
// ---------------------------------------------------------------------------

namespace
{
  /// Build a 3x3 `int` Ghost_Lattice with cells = `i*10 + j`, halo = H.
  template <typename Boundary, std::size_t H = 1>
  Ghost_Lattice<Dense_Cell_Storage<int, 2>, Boundary, H>
  make_3x3_ghost_int()
  {
    Ghost_Lattice<Dense_Cell_Storage<int, 2>, Boundary, H> g({ 3, 3 }, 0);
    for (ca_index_t i = 0; i < 3; ++i)
      for (ca_index_t j = 0; j < 3; ++j)
        g.set({ i, j }, static_cast<int>(i * 10 + j));
    g.refresh_halo();
    return g;
  }

  /// Build a 3x3 classic Lattice with cells = `i*10 + j`.
  template <typename Boundary>
  Lattice<Dense_Cell_Storage<int, 2>, Boundary>
  make_3x3_classic_int()
  {
    Lattice<Dense_Cell_Storage<int, 2>, Boundary> lat({ 3, 3 }, 0);
    for (ca_index_t i = 0; i < 3; ++i)
      for (ca_index_t j = 0; j < 3; ++j)
        lat.set({ i, j }, static_cast<int>(i * 10 + j));
    return lat;
  }
}

// ---------------------------------------------------------------------------
// Strict access.
// ---------------------------------------------------------------------------

TEST(CAGhostLatticeStrictAccess, InRangeMatchesStampedValues)
{
  auto g = make_3x3_ghost_int<OpenBoundary>();
  for (ca_index_t i = 0; i < 3; ++i)
    for (ca_index_t j = 0; j < 3; ++j)
      EXPECT_EQ(g.at({ i, j }), i * 10 + j);
}

TEST(CAGhostLatticeStrictAccess, OutOfRangeThrows)
{
  auto g = make_3x3_ghost_int<OpenBoundary>();
  EXPECT_THROW(g.at({ -1, 0 }), std::out_of_range);
  EXPECT_THROW(g.at({ 3, 0 }), std::out_of_range);
  EXPECT_THROW(g.set({ -1, 0 }, 1), std::out_of_range);
  EXPECT_THROW(g.set({ 0, 3 }, 1), std::out_of_range);
}

TEST(CAGhostLatticeShape, DimensionSizeExtentsHaloRadius)
{
  Ghost_Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary, 2> g({ 4, 5 }, 0);
  EXPECT_EQ(g.dimension(), 2u);
  EXPECT_EQ(g.size(), 4u * 5u);
  EXPECT_EQ(g.size(0), 4u);
  EXPECT_EQ(g.size(1), 5u);
  EXPECT_EQ(g.extents()[0], 4u);
  EXPECT_EQ(g.extents()[1], 5u);
  EXPECT_EQ(g.store_extents()[0], 4u + 4u);  // +2*halo
  EXPECT_EQ(g.store_extents()[1], 5u + 4u);
  EXPECT_EQ(g.halo_radius(), 2u);
}

TEST(CAGhostLatticeShape, SizeAxisOutOfRangeThrows)
{
  Ghost_Lattice<Dense_Cell_Storage<int, 2>, OpenBoundary, 1> g({ 3, 3 }, 0);
  EXPECT_THROW(g.size(2), std::out_of_range);
}

// ---------------------------------------------------------------------------
// fill(): the user-visible region is set, the halo remains untouched
// until refresh_halo is called again.
// ---------------------------------------------------------------------------

TEST(CAGhostLatticeFill, FillSetsOnlyInteriorUntilRefresh)
{
  Ghost_Lattice<Dense_Cell_Storage<int, 2>, ConstantBoundary<int, 9>, 1> g({ 3, 3 }, 0);
  g.refresh_halo();
  // Halo is 9s after the initial refresh.
  EXPECT_EQ(g.at_safe({ -1, 0 }), 9);
  EXPECT_EQ(g.at_safe({ 3, 0 }), 9);

  g.fill(4);
  // Interior is now 4. Halo is still 9 (fill leaves halo untouched).
  EXPECT_EQ(g.at({ 1, 1 }), 4);
  EXPECT_EQ(g.at_safe({ -1, 1 }), 9);

  g.refresh_halo();
  // After refresh the halo still keeps its constant 9 (constant policy).
  EXPECT_EQ(g.at_safe({ -1, 1 }), 9);
}

// ---------------------------------------------------------------------------
// refresh_halo for every policy produces the right cells.
// ---------------------------------------------------------------------------

TEST(CAGhostLatticeHalo, OpenBoundaryHaloIsZero)
{
  auto g = make_3x3_ghost_int<OpenBoundary>();
  // Every halo cell must be zero.
  EXPECT_EQ(g.at_safe({ -1, 0 }), 0);
  EXPECT_EQ(g.at_safe({ -1, -1 }), 0);
  EXPECT_EQ(g.at_safe({ 3, 3 }), 0);
  EXPECT_EQ(g.at_safe({ 0, 3 }), 0);
  // Interior still intact.
  EXPECT_EQ(g.at_safe({ 1, 1 }), 11);
}

TEST(CAGhostLatticeHalo, DirichletConstantValueHalo)
{
  using Boundary = ConstantBoundary<int, 42>;
  Ghost_Lattice<Dense_Cell_Storage<int, 2>, Boundary, 1> g({ 3, 3 }, 0);
  for (ca_index_t i = 0; i < 3; ++i)
    for (ca_index_t j = 0; j < 3; ++j)
      g.set({ i, j }, static_cast<int>(i * 10 + j));
  g.refresh_halo();

  EXPECT_EQ(g.at_safe({ -1, 0 }), 42);
  EXPECT_EQ(g.at_safe({ 3, 0 }), 42);
  EXPECT_EQ(g.at_safe({ -1, -1 }), 42);
  EXPECT_EQ(g.at_safe({ 3, 3 }), 42);
  EXPECT_EQ(g.at_safe({ 1, 1 }), 11);
}

TEST(CAGhostLatticeHalo, PeriodicHaloCopiesOppositeEdge)
{
  auto g = make_3x3_ghost_int<ToroidalBoundary>();
  auto lat = make_3x3_classic_int<ToroidalBoundary>();

  // Single-axis wrap on each side.
  EXPECT_EQ(g.at_safe({ -1, 1 }), lat.at_safe({ -1, 1 }));
  EXPECT_EQ(g.at_safe({ 3, 1 }), lat.at_safe({ 3, 1 }));
  EXPECT_EQ(g.at_safe({ 1, -1 }), lat.at_safe({ 1, -1 }));
  EXPECT_EQ(g.at_safe({ 1, 3 }), lat.at_safe({ 1, 3 }));

  // Corners.
  EXPECT_EQ(g.at_safe({ -1, -1 }), lat.at_safe({ -1, -1 }));
  EXPECT_EQ(g.at_safe({ 3, 3 }), lat.at_safe({ 3, 3 }));
}

TEST(CAGhostLatticeHalo, ReflectiveHaloMirrorsInterior)
{
  auto g = make_3x3_ghost_int<ReflectiveBoundary>();
  auto lat = make_3x3_classic_int<ReflectiveBoundary>();

  // First halo cell on each side mirrors the closest interior cell.
  EXPECT_EQ(g.at_safe({ -1, 1 }), lat.at({ 0, 1 }));
  EXPECT_EQ(g.at_safe({ 3, 1 }), lat.at({ 2, 1 }));
  EXPECT_EQ(g.at_safe({ 1, -1 }), lat.at({ 1, 0 }));
  EXPECT_EQ(g.at_safe({ 1, 3 }), lat.at({ 1, 2 }));

  // Corners.
  EXPECT_EQ(g.at_safe({ -1, -1 }), lat.at({ 0, 0 }));
  EXPECT_EQ(g.at_safe({ 3, 3 }), lat.at({ 2, 2 }));
}

TEST(CAGhostLatticeHalo, NeumannHaloClampsToBoundary)
{
  auto g = make_3x3_ghost_int<NeumannBoundary>();

  // Clamp: -1 -> 0, 3 -> 2.
  EXPECT_EQ(g.at_safe({ -1, 1 }), g.at({ 0, 1 }));
  EXPECT_EQ(g.at_safe({ 3, 1 }), g.at({ 2, 1 }));
  EXPECT_EQ(g.at_safe({ 1, -1 }), g.at({ 1, 0 }));
  EXPECT_EQ(g.at_safe({ 1, 3 }), g.at({ 1, 2 }));
  // Corners.
  EXPECT_EQ(g.at_safe({ -1, -1 }), g.at({ 0, 0 }));
  EXPECT_EQ(g.at_safe({ 3, 3 }), g.at({ 2, 2 }));
}

// ---------------------------------------------------------------------------
// at_safe equivalence: Ghost_Lattice<B> and Lattice<B> must agree for
// every coordinate inside the halo AND every user-visible coordinate.
// ---------------------------------------------------------------------------

template <typename Boundary>
static void expect_equivalent_at_safe()
{
  auto g = make_3x3_ghost_int<Boundary>();
  auto l = make_3x3_classic_int<Boundary>();

  // Walk every coordinate from -2 to 4 inclusive (covers interior +
  // halo + slow-path territory).
  for (ca_index_t i = -2; i <= 4; ++i)
    for (ca_index_t j = -2; j <= 4; ++j)
      EXPECT_EQ(g.at_safe({ i, j }), l.at_safe({ i, j }))
        << "mismatch at (" << i << ", " << j << ")";
}

TEST(CAGhostLatticeEquivalence, OpenMatchesClassic)
{
  expect_equivalent_at_safe<OpenBoundary>();
}
TEST(CAGhostLatticeEquivalence, ToroidalMatchesClassic)
{
  expect_equivalent_at_safe<ToroidalBoundary>();
}
TEST(CAGhostLatticeEquivalence, ReflectiveMatchesClassic)
{
  expect_equivalent_at_safe<ReflectiveBoundary>();
}
TEST(CAGhostLatticeEquivalence, NeumannMatchesClassic)
{
  expect_equivalent_at_safe<NeumannBoundary>();
}

TEST(CAGhostLatticeEquivalence, DirichletMatchesClassic)
{
  using Boundary = ConstantBoundary<int, 77>;
  Ghost_Lattice<Dense_Cell_Storage<int, 2>, Boundary, 1> g({ 3, 3 }, 0);
  Lattice<Dense_Cell_Storage<int, 2>, Boundary> l({ 3, 3 }, 0);
  for (ca_index_t i = 0; i < 3; ++i)
    for (ca_index_t j = 0; j < 3; ++j)
      {
        g.set({ i, j }, static_cast<int>(i * 10 + j));
        l.set({ i, j }, static_cast<int>(i * 10 + j));
      }
  g.refresh_halo();
  for (ca_index_t i = -2; i <= 4; ++i)
    for (ca_index_t j = -2; j <= 4; ++j)
      EXPECT_EQ(g.at_safe({ i, j }), l.at_safe({ i, j }))
        << "mismatch at (" << i << ", " << j << ")";
}

// ---------------------------------------------------------------------------
// Halo radius > 1: the halo has more than one ring.
// ---------------------------------------------------------------------------

TEST(CAGhostLatticeHaloRadius2, BothRingsCarryPolicyValues)
{
  // Halo = 2 so -2 is valid as a no-branch fast-path read.
  auto g = make_3x3_ghost_int<ToroidalBoundary, 2>();
  auto l = make_3x3_classic_int<ToroidalBoundary>();

  // Inner halo ring (|offset| == 1).
  for (ca_index_t i = -1; i <= 3; ++i)
    for (ca_index_t j = -1; j <= 3; ++j)
      EXPECT_EQ(g.at_safe({ i, j }), l.at_safe({ i, j }));

  // Outer halo ring (|offset| == 2).
  EXPECT_EQ(g.at_safe({ -2, 1 }), l.at_safe({ -2, 1 }));
  EXPECT_EQ(g.at_safe({ 4, 1 }), l.at_safe({ 4, 1 }));
  EXPECT_EQ(g.at_safe({ 1, -2 }), l.at_safe({ 1, -2 }));
  EXPECT_EQ(g.at_safe({ 1, 4 }), l.at_safe({ 1, 4 }));
  EXPECT_EQ(g.at_safe({ -2, -2 }), l.at_safe({ -2, -2 }));
  EXPECT_EQ(g.at_safe({ 4, 4 }), l.at_safe({ 4, 4 }));

  // Beyond halo (|offset| >= 3) we hit the slow policy path, which
  // must still agree with the classic lattice.
  EXPECT_EQ(g.at_safe({ -3, 1 }), l.at_safe({ -3, 1 }));
  EXPECT_EQ(g.at_safe({ 5, 1 }), l.at_safe({ 5, 1 }));
}

// ---------------------------------------------------------------------------
// 1D lattice sanity: the same machinery scales down.
// ---------------------------------------------------------------------------

TEST(CAGhostLattice1D, PeriodicHaloAndEquivalence)
{
  Ghost_Lattice<Dense_Cell_Storage<int, 1>, ToroidalBoundary, 1> g({ 5 }, 0);
  Lattice<Dense_Cell_Storage<int, 1>, ToroidalBoundary> l({ 5 }, 0);
  for (ca_index_t i = 0; i < 5; ++i)
    {
      g.set({ i }, static_cast<int>(i + 1));
      l.set({ i }, static_cast<int>(i + 1));
    }
  g.refresh_halo();
  for (ca_index_t i = -3; i <= 7; ++i)
    EXPECT_EQ(g.at_safe({ i }), l.at_safe({ i }));
}

// ---------------------------------------------------------------------------
// 3D lattice sanity: every corner of the halo is populated.
// ---------------------------------------------------------------------------

TEST(CAGhostLattice3D, ReflectiveCornerConsistency)
{
  Ghost_Lattice<Dense_Cell_Storage<int, 3>, ReflectiveBoundary, 1> g({ 2, 2, 2 }, 0);
  Lattice<Dense_Cell_Storage<int, 3>, ReflectiveBoundary> l({ 2, 2, 2 }, 0);
  int v = 1;
  for (ca_index_t i = 0; i < 2; ++i)
    for (ca_index_t j = 0; j < 2; ++j)
      for (ca_index_t k = 0; k < 2; ++k, ++v)
        {
          g.set({ i, j, k }, v);
          l.set({ i, j, k }, v);
        }
  g.refresh_halo();

  for (ca_index_t i = -1; i <= 2; ++i)
    for (ca_index_t j = -1; j <= 2; ++j)
      for (ca_index_t k = -1; k <= 2; ++k)
        EXPECT_EQ(g.at_safe({ i, j, k }), l.at_safe({ i, j, k }));
}

// ---------------------------------------------------------------------------
// swap and copy semantics.
// ---------------------------------------------------------------------------

TEST(CAGhostLattice, SwapExchangesContents)
{
  Ghost_Lattice<Dense_Cell_Storage<int, 2>, OpenBoundary, 1> a({ 2, 2 }, 1);
  Ghost_Lattice<Dense_Cell_Storage<int, 2>, OpenBoundary, 1> b({ 3, 3 }, 9);
  a.swap(b);
  EXPECT_EQ(a.size(0), 3u);
  EXPECT_EQ(b.size(0), 2u);
  EXPECT_EQ(a.at({ 0, 0 }), 9);
  EXPECT_EQ(b.at({ 0, 0 }), 1);
}

// ---------------------------------------------------------------------------
// Engine equivalence (the Phase 4 acceptance criterion).
//
// Running the classic synchronous engine for >= 100 steps on a regular
// `Lattice` and on a `Ghost_Lattice` must produce identical frames,
// for every supported boundary policy.
// ---------------------------------------------------------------------------

namespace
{
  /// Deterministic seed used across equivalence tests.
  inline void seed_rpentomino(auto &lat, ca_index_t rows, ca_index_t cols)
  {
    const ca_index_t ci = rows / 2, cj = cols / 2;
    lat.set({ ci - 1, cj }, 1);
    lat.set({ ci - 1, cj + 1 }, 1);
    lat.set({ ci, cj - 1 }, 1);
    lat.set({ ci, cj }, 1);
    lat.set({ ci + 1, cj }, 1);
  }

  template <typename L>
  bool frames_match(const L &a, const L &b)
  {
    if (a.size(0) != b.size(0) or a.size(1) != b.size(1))
      return false;
    for (ca_index_t i = 0; i < static_cast<ca_index_t>(a.size(0)); ++i)
      for (ca_index_t j = 0; j < static_cast<ca_index_t>(a.size(1)); ++j)
        if (a.at({ i, j }) != b.at({ i, j }))
          return false;
    return true;
  }

  template <typename A, typename B>
  bool frames_match_cross(const A &a, const B &b)
  {
    if (a.size(0) != b.size(0) or a.size(1) != b.size(1))
      return false;
    for (ca_index_t i = 0; i < static_cast<ca_index_t>(a.size(0)); ++i)
      for (ca_index_t j = 0; j < static_cast<ca_index_t>(a.size(1)); ++j)
        if (a.at({ i, j }) != b.at({ i, j }))
          return false;
    return true;
  }
}

template <typename Boundary>
static void run_engine_equivalence(std::size_t steps)
{
  constexpr ca_size_t ROWS = 16;
  constexpr ca_size_t COLS = 16;

  using Storage = Dense_Cell_Storage<int, 2>;
  using ClassicL = Lattice<Storage, Boundary>;
  using GhostL = Ghost_Lattice<Storage, Boundary, 1>;

  ClassicL l_init({ ROWS, COLS }, 0);
  GhostL g_init({ ROWS, COLS }, 0);
  seed_rpentomino(l_init, ROWS, COLS);
  seed_rpentomino(g_init, ROWS, COLS);

  using ClassicEngine = Synchronous_Engine<ClassicL, Game_Of_Life_Rule, Moore<2, 1>>;
  using GhostEngine = Synchronous_Engine<GhostL, Game_Of_Life_Rule, Moore<2, 1>>;

  ClassicEngine classic(std::move(l_init), make_game_of_life_rule(), Moore<2, 1>{});
  GhostEngine ghost(std::move(g_init), make_game_of_life_rule(), Moore<2, 1>{});

  for (std::size_t s = 0; s < steps; ++s)
    {
      classic.step();
      ghost.step();
      ASSERT_TRUE(frames_match_cross(classic.frame(), ghost.frame()))
        << "frames diverge at step " << s + 1;
    }
}

TEST(CAGhostEngineEquivalence, OpenBoundaryAt100Steps)
{
  run_engine_equivalence<OpenBoundary>(100);
}

TEST(CAGhostEngineEquivalence, ToroidalBoundaryAt150Steps)
{
  run_engine_equivalence<ToroidalBoundary>(150);
}

TEST(CAGhostEngineEquivalence, ReflectiveBoundaryAt120Steps)
{
  run_engine_equivalence<ReflectiveBoundary>(120);
}

TEST(CAGhostEngineEquivalence, NeumannBoundaryAt100Steps)
{
  run_engine_equivalence<NeumannBoundary>(100);
}

TEST(CAGhostEngineEquivalence, DirichletZeroAt100Steps)
{
  // With V = 0 the Dirichlet halo is indistinguishable from `Open`.
  run_engine_equivalence<ConstantBoundary<int, 0>>(100);
}

TEST(CAGhostEngineEquivalence, DirichletOneAt50Steps)
{
  // A non-zero Dirichlet floods the border with alive neighbours, so
  // the grid saturates quickly. We still verify frame equivalence.
  run_engine_equivalence<ConstantBoundary<int, 1>>(50);
}

// ---------------------------------------------------------------------------
// Halo radius 2 with the engine: a Von_Neumann radius-2 rule must work
// without the slow path kicking in (since halo radius >= neighbourhood).
// ---------------------------------------------------------------------------

// A trivial totalistic rule that counts alive neighbours mod 2.
// Defined at namespace scope because local classes may not have member
// templates, and `Outer_Totalistic_Rule::operator()` is templated.
namespace
{
  struct R2_Parity_Functor
  {
    constexpr int operator()(const int &, std::size_t alive) const noexcept
    {
      return alive % 2 == 0 ? 0 : 1;
    }
  };
}

TEST(CAGhostEngineRadius2, VonNeumannR2EquivalenceOnDirichlet)
{
  constexpr ca_size_t ROWS = 8;
  constexpr ca_size_t COLS = 8;

  using Storage = Dense_Cell_Storage<int, 2>;
  using Boundary = ConstantBoundary<int, 0>;
  using ParityRule = Outer_Totalistic_Rule<R2_Parity_Functor>;
  using Nbh = Von_Neumann<2, 2>;

  using ClassicL = Lattice<Storage, Boundary>;
  using GhostL = Ghost_Lattice<Storage, Boundary, 2>;

  ClassicL l_init({ ROWS, COLS }, 0);
  GhostL g_init({ ROWS, COLS }, 0);
  // Seed a single alive cell in the middle.
  l_init.set({ 4, 4 }, 1);
  g_init.set({ 4, 4 }, 1);

  Synchronous_Engine<ClassicL, ParityRule, Nbh> classic(
    std::move(l_init), ParityRule(R2_Parity_Functor{}), Nbh{});
  Synchronous_Engine<GhostL, ParityRule, Nbh> ghost(
    std::move(g_init), ParityRule(R2_Parity_Functor{}), Nbh{});

  for (std::size_t s = 0; s < 60; ++s)
    {
      classic.step();
      ghost.step();
      ASSERT_TRUE(frames_match_cross(classic.frame(), ghost.frame()))
        << "R=2 frames diverge at step " << s + 1;
    }
}
