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
 * @file tpl_ca_stochastic_rules_test.cc
 * @brief Phase 8 tests for `tpl_ca_stochastic_rules.H`.
 *
 * Coverage:
 *  - `Probabilistic_Rule` legacy 2-arg and Phase 8 3-arg dispatch.
 *  - `apply_rule` chooses the contextual overload when available.
 *  - Forest fire, SIR, Ising and Schelling rules' canonical
 *    transitions on hand-crafted micro-frames.
 *  - Cross-thread bit-for-bit reproducibility on the parallel engine
 *    for every stochastic rule.
 *  - SIR with `R0 < 1` extinguishes the epidemic.
 */

#include <algorithm>
#include <array>
#include <cstdint>
#include <random>
#include <vector>

#include <gtest/gtest.h>

#include <ca-rng.H>
#include <ca-traits.H>
#include <tpl_ca_concepts.H>
#include <tpl_ca_engine.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_parallel_engine.H>
#include <tpl_ca_rule.H>
#include <tpl_ca_stochastic_rules.H>
#include <tpl_ca_storage.H>

using namespace Aleph;
using namespace Aleph::CA;

// ---------------------------------------------------------------------------
// Common helpers.
// ---------------------------------------------------------------------------

namespace
{

template <typename L>
bool frames_equal(const L &a, const L &b)
{
  if (a.extents() != b.extents())
    return false;
  using coord_t = typename L::coord_type;
  if constexpr (L::rank == 1)
    {
      const ca_size_t n0 = a.size(0);
      for (ca_size_t i = 0; i < n0; ++i)
        if (a.at(coord_t{static_cast<ca_index_t>(i)})
            != b.at(coord_t{static_cast<ca_index_t>(i)}))
          return false;
    }
  else if constexpr (L::rank == 2)
    {
      const ca_size_t n0 = a.size(0);
      const ca_size_t n1 = a.size(1);
      for (ca_size_t i = 0; i < n0; ++i)
        for (ca_size_t j = 0; j < n1; ++j)
          if (a.at(coord_t{static_cast<ca_index_t>(i),
                           static_cast<ca_index_t>(j)})
              != b.at(coord_t{static_cast<ca_index_t>(i),
                              static_cast<ca_index_t>(j)}))
            return false;
    }
  return true;
}

template <typename L>
std::size_t count_state(const L &lat, int target)
{
  std::size_t total = 0;
  using coord_t = typename L::coord_type;
  if constexpr (L::rank == 2)
    {
      const ca_size_t n0 = lat.size(0);
      const ca_size_t n1 = lat.size(1);
      for (ca_size_t i = 0; i < n0; ++i)
        for (ca_size_t j = 0; j < n1; ++j)
          if (static_cast<int>(lat.at(coord_t{static_cast<ca_index_t>(i),
                                              static_cast<ca_index_t>(j)}))
              == target)
            ++total;
    }
  return total;
}

}  // namespace

// ---------------------------------------------------------------------------
// Probabilistic_Rule: legacy and reproducible dispatch.
// ---------------------------------------------------------------------------

TEST(CAProbabilisticRule, LegacyTwoArgFunctorStillWorks)
{
  // A pure 2-arg functor: counts neighbours, returns 1 iff there are at
  // least 3 alive ones (toy GoL-ish rule). No randomness involved.
  auto counter = [](const int &, Neighbor_View<int> v) -> int
  {
    std::size_t a = 0;
    for (const auto &x : v)
      if (x != 0)
        ++a;
    return a >= 3 ? 1 : 0;
  };
  Probabilistic_Rule rule(counter);

  std::array<int, 4> nbuf{1, 1, 0, 1};
  EXPECT_EQ(rule(0, Neighbor_View<int>(nbuf.data(), nbuf.size())), 1);
}

TEST(CAProbabilisticRule, ContextualRouteUsesCellSeedDeterminism)
{
  // Functor that uses the engine reference and burns one draw.
  auto burn = [](const int &, Neighbor_View<int>, std::mt19937_64 &eng) -> int
  { return static_cast<int>(eng() & 0xff); };

  Probabilistic_Rule<decltype(burn), std::mt19937_64> rule(burn, 0xC0FFEE);

  Cell_Context<2> ctx{3, Coord_Vec<2>{4, 7}};
  std::array<int, 1> nbuf{0};
  const int a = rule(0, Neighbor_View<int>(nbuf.data(), 1), ctx);
  const int b = rule(0, Neighbor_View<int>(nbuf.data(), 1), ctx);
  EXPECT_EQ(a, b);

  // Different context -> deterministically different seed and first draw.
  Cell_Context<2> ctx2{4, Coord_Vec<2>{4, 7}};
  const std::uint64_t seed1 = cell_seed<2>(rule.master_seed(), ctx);
  const std::uint64_t seed2 = cell_seed<2>(rule.master_seed(), ctx2);
  EXPECT_NE(seed1, seed2);
  std::mt19937_64 eng1{static_cast<std::mt19937_64::result_type>(seed1)};
  std::mt19937_64 eng2{static_cast<std::mt19937_64::result_type>(seed2)};
  EXPECT_NE(eng1(), eng2());
}

// ---------------------------------------------------------------------------
// apply_rule: forwards the contextual call when supported.
// ---------------------------------------------------------------------------

namespace
{

/// Reports either the step index (contextual) or -1 (legacy) so we can
/// observe which overload `apply_rule` selected.
struct Step_Reporter
{
  template <typename State>
  State operator()(const State &, Neighbor_View<State>) const
  {
    return static_cast<State>(-1);
  }
  template <typename State, std::size_t Rank>
  State operator()(const State &, Neighbor_View<State>,
                   const Cell_Context<Rank> &ctx) const
  {
    return static_cast<State>(ctx.step);
  }
};

/// Pure 2-arg rule used to check that `apply_rule` falls back to the
/// legacy signature when the rule does not accept a `Cell_Context`.
struct View_Size_Rule
{
  template <typename State>
  State operator()(const State &, Neighbor_View<State> v) const
  {
    return static_cast<State>(v.size());
  }
};

}  // namespace

TEST(CAApplyRule, ContextualOverloadIsPickedUp)
{
  Step_Reporter r;
  std::array<int, 1> nbuf{0};
  Cell_Context<2> ctx{42, Coord_Vec<2>{0, 0}};
  EXPECT_EQ(apply_rule(r, 0, Neighbor_View<int>(nbuf.data(), 1), ctx), 42);
}

TEST(CAApplyRule, FallsBackToLegacyWhenContextNotAccepted)
{
  View_Size_Rule r;
  std::array<int, 4> nbuf{};
  Cell_Context<2> ctx{0, Coord_Vec<2>{0, 0}};
  EXPECT_EQ(apply_rule(r, 0, Neighbor_View<int>(nbuf.data(), nbuf.size()), ctx),
            4);
}

// ---------------------------------------------------------------------------
// Forest fire: canonical transitions.
// ---------------------------------------------------------------------------

TEST(CAForestFire, BurningAlwaysBecomesEmpty)
{
  Forest_Fire_Rule<> rule(0.05, 0.001, /*seed=*/0x1u);
  const std::array<int, 8> nbuf{0, 0, 0, 0, 0, 0, 0, 0};
  Cell_Context<2> ctx{0, Coord_Vec<2>{0, 0}};
  const int next = rule(static_cast<int>(Forest_Cell::BURNING),
                        Neighbor_View<int>(nbuf.data(), nbuf.size()), ctx);
  EXPECT_EQ(next, static_cast<int>(Forest_Cell::EMPTY));
}

TEST(CAForestFire, TreeIgnitesWhenAnyNeighbourBurns)
{
  Forest_Fire_Rule<> rule(0.0, 0.0, /*seed=*/0x1u);
  std::array<int, 8> nbuf{0, 0, 0, static_cast<int>(Forest_Cell::BURNING),
                          0, 0, 0, 0};
  Cell_Context<2> ctx{0, Coord_Vec<2>{1, 1}};
  EXPECT_EQ(rule(static_cast<int>(Forest_Cell::TREE),
                 Neighbor_View<int>(nbuf.data(), nbuf.size()), ctx),
            static_cast<int>(Forest_Cell::BURNING));
}

TEST(CAForestFire, EmptyStaysEmptyWhenGrowthIsZero)
{
  Forest_Fire_Rule<> rule(0.0, 0.0, /*seed=*/0x1u);
  std::array<int, 8> nbuf{};
  Cell_Context<2> ctx{0, Coord_Vec<2>{0, 0}};
  EXPECT_EQ(rule(static_cast<int>(Forest_Cell::EMPTY),
                 Neighbor_View<int>(nbuf.data(), nbuf.size()), ctx),
            static_cast<int>(Forest_Cell::EMPTY));
}

TEST(CAForestFire, EmptyAlwaysGrowsWhenGrowthIsOne)
{
  Forest_Fire_Rule<> rule(1.0, 0.0, /*seed=*/0x1u);
  std::array<int, 8> nbuf{};
  Cell_Context<2> ctx{0, Coord_Vec<2>{0, 0}};
  EXPECT_EQ(rule(static_cast<int>(Forest_Cell::EMPTY),
                 Neighbor_View<int>(nbuf.data(), nbuf.size()), ctx),
            static_cast<int>(Forest_Cell::TREE));
}

TEST(CAForestFire, FireSpreadsOnSmallToroidalLattice)
{
  using L = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;
  L lat({5, 5}, static_cast<int>(Forest_Cell::TREE));
  // Light a fire at (2, 2). With p_growth = 0 and p_lightning = 0 the
  // dynamics are deterministic: the fire spreads outwards over time.
  lat.set({2, 2}, static_cast<int>(Forest_Cell::BURNING));

  Synchronous_Engine<L, Forest_Fire_Rule<>, Moore<2, 1>> engine(
    lat, Forest_Fire_Rule<>(0.0, 0.0, 0xABCDu), Moore<2, 1>{});

  engine.step();

  const auto &f = engine.frame();
  EXPECT_EQ(f.at({2, 2}), static_cast<int>(Forest_Cell::EMPTY));
  // All immediate Moore neighbours of (2, 2) are now burning.
  for (int di = -1; di <= 1; ++di)
    for (int dj = -1; dj <= 1; ++dj)
      if (di != 0 or dj != 0)
        EXPECT_EQ(f.at({2 + di, 2 + dj}),
                  static_cast<int>(Forest_Cell::BURNING));
}

// ---------------------------------------------------------------------------
// SIR: extinction with R0 < 1.
// ---------------------------------------------------------------------------

TEST(CASir, RecoveredStaysRecovered)
{
  SIR_Rule<> rule(0.5, 0.5, 0x1u);
  std::array<int, 4> nbuf{static_cast<int>(SIR_Cell::I),
                          static_cast<int>(SIR_Cell::I),
                          static_cast<int>(SIR_Cell::I),
                          static_cast<int>(SIR_Cell::I)};
  Cell_Context<2> ctx{0, Coord_Vec<2>{0, 0}};
  EXPECT_EQ(rule(static_cast<int>(SIR_Cell::R),
                 Neighbor_View<int>(nbuf.data(), nbuf.size()), ctx),
            static_cast<int>(SIR_Cell::R));
}

TEST(CASir, SusceptibleStaysSWithoutInfectedNeighbours)
{
  SIR_Rule<> rule(1.0, 0.5, 0x1u);
  std::array<int, 4> nbuf{};
  Cell_Context<2> ctx{0, Coord_Vec<2>{0, 0}};
  EXPECT_EQ(rule(static_cast<int>(SIR_Cell::S),
                 Neighbor_View<int>(nbuf.data(), nbuf.size()), ctx),
            static_cast<int>(SIR_Cell::S));
}

TEST(CASir, InfectedAlwaysRecoversAtGammaOne)
{
  SIR_Rule<> rule(0.5, 1.0, 0x1u);
  std::array<int, 4> nbuf{};
  Cell_Context<2> ctx{0, Coord_Vec<2>{0, 0}};
  EXPECT_EQ(rule(static_cast<int>(SIR_Cell::I),
                 Neighbor_View<int>(nbuf.data(), nbuf.size()), ctx),
            static_cast<int>(SIR_Cell::R));
}

TEST(CASir, R0BelowOneExtinguishesEpidemic)
{
  // beta=0.05, gamma=0.5, Moore radius 1 ⇒ R0 ≈ 0.05*8/0.5 = 0.8 < 1.
  // Start with a single infected cell on a 32×32 toroidal lattice.
  // After 200 steps the epidemic should be extinct.
  using L = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;
  L lat({32, 32}, static_cast<int>(SIR_Cell::S));
  lat.set({16, 16}, static_cast<int>(SIR_Cell::I));

  SIR_Rule<> rule(0.05, 0.5, 0xACE0FBAFFu);
  Synchronous_Engine<L, SIR_Rule<>, Moore<2, 1>> engine(lat, rule);
  engine.run(200);

  EXPECT_EQ(count_state(engine.frame(), static_cast<int>(SIR_Cell::I)), 0u);
}

// ---------------------------------------------------------------------------
// Ising rules: limit cases.
// ---------------------------------------------------------------------------

TEST(CAIsingGlauber, FlipProbabilityOneHalfWhenDeltaEnergyZero)
{
  // With s=+1, J=1, H=0 and an equal number of +1 / -1 neighbours,
  // dE = 0 ⇒ p_flip = 1/2. Average over many cells should be ≈ 0.5.
  Ising_Glauber_Rule<> rule(/*T=*/2.0, 1.0, 0.0, /*seed=*/0x42u);

  std::array<int, 4> nbuf{1, -1, 1, -1};
  Cell_Context<2> ctx{0, Coord_Vec<2>{0, 0}};
  std::size_t flipped = 0;
  const std::size_t trials = 1024;
  for (std::size_t k = 0; k < trials; ++k)
    {
      ctx.coord[0] = static_cast<ca_index_t>(k);
      const int n = rule(static_cast<int>(1),
                         Neighbor_View<int>(nbuf.data(), nbuf.size()), ctx);
      if (n != 1)
        ++flipped;
    }
  // Expect close to 50% flipped — be generous to absorb fluctuations.
  EXPECT_GT(flipped, trials * 35u / 100u);
  EXPECT_LT(flipped, trials * 65u / 100u);
}

TEST(CAIsingMetropolis, AlwaysFlipsWhenEnergyDecreases)
{
  // s=+1 surrounded by all +1 neighbours and J=-1 (anti-ferromagnetic).
  // dE = 2 * 1 * (-1 * 4 + 0) = -8 < 0 ⇒ Metropolis always flips.
  Ising_Metropolis_Rule<> rule(/*T=*/0.5, /*J=*/-1.0, /*H=*/0.0, 0xCAFEu);

  std::array<int, 4> nbuf{1, 1, 1, 1};
  Cell_Context<2> ctx{0, Coord_Vec<2>{0, 0}};
  EXPECT_EQ(rule(1, Neighbor_View<int>(nbuf.data(), nbuf.size()), ctx), -1);
}

TEST(CAIsingMetropolis, NeverFlipsWhenEnergyMassivelyIncreases)
{
  // s=+1, J=1, all +1 neighbours, low T. Flip would raise energy by
  // dE = 2 * 1 * (1 * 4) = 8. p_flip = exp(-8 / 0.05) ≈ 1.27e-70.
  // 256 trials should never observe a flip.
  Ising_Metropolis_Rule<> rule(/*T=*/0.05, /*J=*/1.0, 0.0, 0xCAFEu);

  std::array<int, 4> nbuf{1, 1, 1, 1};
  for (std::size_t k = 0; k < 256; ++k)
    {
      Cell_Context<2> ctx{k, Coord_Vec<2>{static_cast<ca_index_t>(k), 0}};
      EXPECT_EQ(rule(1, Neighbor_View<int>(nbuf.data(), nbuf.size()), ctx), 1);
    }
}

// ---------------------------------------------------------------------------
// Schelling.
// ---------------------------------------------------------------------------

TEST(CASchelling, IsolatedAgentStaysPut)
{
  Schelling_Rule<> rule(0.5, 1.0, 1.0, 0xCAFEu);
  std::array<int, 4> nbuf{}; // all empty
  Cell_Context<2> ctx{0, Coord_Vec<2>{0, 0}};
  EXPECT_EQ(rule(static_cast<int>(Schelling_Cell::TYPE_A),
                 Neighbor_View<int>(nbuf.data(), nbuf.size()), ctx),
            static_cast<int>(Schelling_Cell::TYPE_A));
}

TEST(CASchelling, UnhappyAgentVacates)
{
  Schelling_Rule<> rule(/*threshold=*/0.6, /*p_move=*/1.0, /*p_fill=*/0.0,
                        0x1234u);
  // Type A surrounded by 4 type B occupied + 0 type A occupied.
  // fraction = 0/4 = 0 < 0.6 ⇒ unhappy ⇒ moves out (becomes EMPTY).
  std::array<int, 4> nbuf{
    static_cast<int>(Schelling_Cell::TYPE_B),
    static_cast<int>(Schelling_Cell::TYPE_B),
    static_cast<int>(Schelling_Cell::TYPE_B),
    static_cast<int>(Schelling_Cell::TYPE_B),
  };
  Cell_Context<2> ctx{0, Coord_Vec<2>{0, 0}};
  EXPECT_EQ(rule(static_cast<int>(Schelling_Cell::TYPE_A),
                 Neighbor_View<int>(nbuf.data(), nbuf.size()), ctx),
            static_cast<int>(Schelling_Cell::EMPTY));
}

TEST(CASchelling, HappyAgentStaysPut)
{
  Schelling_Rule<> rule(/*threshold=*/0.4, 1.0, 0.0, 0x1234u);
  // 3 same / 1 other ⇒ fraction = 0.75 ≥ 0.4 ⇒ happy.
  std::array<int, 4> nbuf{
    static_cast<int>(Schelling_Cell::TYPE_A),
    static_cast<int>(Schelling_Cell::TYPE_A),
    static_cast<int>(Schelling_Cell::TYPE_A),
    static_cast<int>(Schelling_Cell::TYPE_B),
  };
  Cell_Context<2> ctx{0, Coord_Vec<2>{0, 0}};
  EXPECT_EQ(rule(static_cast<int>(Schelling_Cell::TYPE_A),
                 Neighbor_View<int>(nbuf.data(), nbuf.size()), ctx),
            static_cast<int>(Schelling_Cell::TYPE_A));
}

TEST(CASchelling, EmptyCellFillsWithMajority)
{
  Schelling_Rule<> rule(0.5, 0.0, 1.0, 0x55u);
  std::array<int, 4> nbuf{
    static_cast<int>(Schelling_Cell::TYPE_A),
    static_cast<int>(Schelling_Cell::TYPE_A),
    static_cast<int>(Schelling_Cell::TYPE_B),
    static_cast<int>(Schelling_Cell::EMPTY),
  };
  Cell_Context<2> ctx{0, Coord_Vec<2>{0, 0}};
  EXPECT_EQ(rule(static_cast<int>(Schelling_Cell::EMPTY),
                 Neighbor_View<int>(nbuf.data(), nbuf.size()), ctx),
            static_cast<int>(Schelling_Cell::TYPE_A));
}

// ---------------------------------------------------------------------------
// Cross-thread bit-for-bit reproducibility.
// ---------------------------------------------------------------------------

namespace
{

template <typename Lattice, typename Rule>
Lattice run_parallel(const Lattice &seed, Rule rule, std::size_t steps,
                     std::size_t parts)
{
  Parallel_Engine_Config cfg;
  cfg.num_partitions = parts;
  cfg.min_parallel_cells = 0;  // force the parallel path even for tiny grids
  Parallel_Synchronous_Engine<Lattice, Rule, Moore<2, 1>> engine(
    seed, std::move(rule), Moore<2, 1>{}, cfg);
  engine.run(steps);
  return engine.frame();
}

template <typename L>
void seed_random_states(L &lat, std::uint32_t seed,
                        std::initializer_list<int> states,
                        const std::vector<double> &weights)
{
  std::mt19937 rng(seed);
  std::discrete_distribution<int> pick(weights.begin(), weights.end());
  using coord_t = typename L::coord_type;
  const ca_size_t n0 = lat.size(0);
  const ca_size_t n1 = lat.size(1);
  std::vector<int> table(states.begin(), states.end());
  for (ca_size_t i = 0; i < n0; ++i)
    for (ca_size_t j = 0; j < n1; ++j)
      lat.set(coord_t{static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)},
              table[pick(rng)]);
}

}  // namespace

TEST(CAStochasticReproducibility, ForestFireIdenticalAcrossThreadCounts)
{
  using L = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;
  L seed({16, 16});
  seed_random_states(seed, 0xABCDu,
                     {static_cast<int>(Forest_Cell::EMPTY),
                      static_cast<int>(Forest_Cell::TREE),
                      static_cast<int>(Forest_Cell::BURNING)},
                     {0.4, 0.55, 0.05});

  Forest_Fire_Rule<> rule(0.05, 0.001, /*seed=*/0xCC11u);
  L f1 = run_parallel(seed, rule, 50, /*parts=*/1);
  L f2 = run_parallel(seed, rule, 50, /*parts=*/2);
  L f4 = run_parallel(seed, rule, 50, /*parts=*/4);
  L f8 = run_parallel(seed, rule, 50, /*parts=*/8);

  EXPECT_TRUE(frames_equal(f1, f2));
  EXPECT_TRUE(frames_equal(f1, f4));
  EXPECT_TRUE(frames_equal(f1, f8));
}

TEST(CAStochasticReproducibility, SIRIdenticalAcrossThreadCounts)
{
  using L = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;
  L seed({20, 20}, static_cast<int>(SIR_Cell::S));
  seed.set({10, 10}, static_cast<int>(SIR_Cell::I));
  seed.set({4, 5}, static_cast<int>(SIR_Cell::I));

  SIR_Rule<> rule(0.2, 0.1, /*seed=*/0xDD22u);
  L f1 = run_parallel(seed, rule, 80, /*parts=*/1);
  L f3 = run_parallel(seed, rule, 80, /*parts=*/3);
  L f8 = run_parallel(seed, rule, 80, /*parts=*/8);

  EXPECT_TRUE(frames_equal(f1, f3));
  EXPECT_TRUE(frames_equal(f1, f8));
}

TEST(CAStochasticReproducibility, IsingGlauberIdenticalAcrossThreadCounts)
{
  using L = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;
  L seed({16, 16});
  std::mt19937 rng(0x4242u);
  std::bernoulli_distribution flip(0.5);
  for (ca_size_t i = 0; i < seed.size(0); ++i)
    for (ca_size_t j = 0; j < seed.size(1); ++j)
      seed.set({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)},
               flip(rng) ? 1 : -1);

  Ising_Glauber_Rule<> rule(/*T=*/2.5, 1.0, 0.0, /*seed=*/0xEE33u);
  L f1 = run_parallel(seed, rule, 80, /*parts=*/1);
  L f4 = run_parallel(seed, rule, 80, /*parts=*/4);
  L f8 = run_parallel(seed, rule, 80, /*parts=*/8);

  EXPECT_TRUE(frames_equal(f1, f4));
  EXPECT_TRUE(frames_equal(f1, f8));
}

TEST(CAStochasticReproducibility, SchellingIdenticalAcrossThreadCounts)
{
  using L = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;
  L seed({16, 16});
  seed_random_states(seed, 0xBEEFu,
                     {static_cast<int>(Schelling_Cell::EMPTY),
                      static_cast<int>(Schelling_Cell::TYPE_A),
                      static_cast<int>(Schelling_Cell::TYPE_B)},
                     {0.1, 0.45, 0.45});

  Schelling_Rule<> rule(0.5, 0.7, 0.7, /*seed=*/0xFE33u);
  L f1 = run_parallel(seed, rule, 60, /*parts=*/1);
  L f2 = run_parallel(seed, rule, 60, /*parts=*/2);
  L f8 = run_parallel(seed, rule, 60, /*parts=*/8);

  EXPECT_TRUE(frames_equal(f1, f2));
  EXPECT_TRUE(frames_equal(f1, f8));
}

// ---------------------------------------------------------------------------
// Sequential vs parallel equivalence: same seed must yield same trajectory.
// ---------------------------------------------------------------------------

TEST(CAStochasticReproducibility, SequentialAndParallelMatchForestFire)
{
  using L = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;
  L seed({24, 24});
  seed_random_states(seed, 0xF1F1u,
                     {static_cast<int>(Forest_Cell::EMPTY),
                      static_cast<int>(Forest_Cell::TREE),
                      static_cast<int>(Forest_Cell::BURNING)},
                     {0.3, 0.6, 0.1});

  Forest_Fire_Rule<> rule(0.02, 0.001, /*seed=*/0xC1u);

  Synchronous_Engine<L, Forest_Fire_Rule<>, Moore<2, 1>> seq(seed, rule);
  seq.run(40);

  L par = run_parallel(seed, rule, 40, /*parts=*/4);
  EXPECT_TRUE(frames_equal(seq.frame(), par));
}
