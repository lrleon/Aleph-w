/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file tpl_ca_continuous_rules_test.cc
 * @brief Phase 9 tests for continuous and history CA rules.
 *
 * Coverage:
 *  - `Continuous_Rule` scalar convolution dispatch and runtime guards.
 *  - Gray-Scott local transitions and deterministic pattern statistics.
 *  - FitzHugh-Nagumo local transitions and stability guards.
 *  - `History_Cell` ring-buffer semantics.
 *  - `History_Rule` direct invocation and `Synchronous_Engine` integration.
 */

#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>

#include <gtest/gtest.h>

#include <ca-kernels.H>
#include <ca-rng.H>
#include <ca-traits.H>
#include <tpl_ca_continuous_rules.H>
#include <tpl_ca_engine.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_storage.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace
{

constexpr double eps = 1e-12;

using RD_Cell = Reaction_Diffusion_Cell<double>;
using RD_Lattice = Lattice<Dense_Cell_Storage<RD_Cell, 2>, NeumannBoundary>;

double seeded_noise(const std::uint64_t master, const Coord_Vec<2> &coord)
{
  const std::uint64_t h = cell_seed<2>(master, 0, coord);
  const double unit = static_cast<double>(h >> 11)
                      * (1.0 / 9007199254740992.0);
  return unit - 0.5;
}

RD_Lattice make_gray_scott_seed(const ca_size_t n,
                                const std::uint64_t master)
{
  RD_Lattice lat({n, n}, RD_Cell{1.0, 0.0});
  const ca_size_t lo = n / 2 - n / 10;
  const ca_size_t hi = n / 2 + n / 10;
  for (ca_size_t i = lo; i <= hi; ++i)
    for (ca_size_t j = lo; j <= hi; ++j)
      {
        const Coord_Vec<2> c{static_cast<ca_index_t>(i),
                             static_cast<ca_index_t>(j)};
        const double z = 0.02 * seeded_noise(master, c);
        lat.set(c, RD_Cell{0.50 + z, 0.25 - z});
      }
  return lat;
}

struct VStats
{
  double mean = 0.0;
  double variance = 0.0;
  double max_value = 0.0;
  std::size_t peaks = 0;
};

VStats stats_for_v(const RD_Lattice &lat)
{
  const double total = static_cast<double>(lat.size(0) * lat.size(1));
  double sum = 0.0;
  double sum2 = 0.0;
  double max_v = 0.0;
  std::size_t peaks = 0;

  for (ca_size_t i = 0; i < lat.size(0); ++i)
    for (ca_size_t j = 0; j < lat.size(1); ++j)
      {
        const RD_Cell cell = lat.at({static_cast<ca_index_t>(i),
                                     static_cast<ca_index_t>(j)});
        sum += cell.v;
        sum2 += cell.v * cell.v;
        if (cell.v > max_v)
          max_v = cell.v;
        if (cell.v > 0.10)
          ++peaks;
      }

  const double mean = sum / total;
  return {mean, sum2 / total - mean * mean, max_v, peaks};
}

bool frames_equal(const RD_Lattice &a, const RD_Lattice &b)
{
  if (a.extents() != b.extents())
    return false;
  for (ca_size_t i = 0; i < a.size(0); ++i)
    for (ca_size_t j = 0; j < a.size(1); ++j)
      if (a.at({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)})
          != b.at({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)}))
        return false;
  return true;
}

}  // namespace

TEST(CAContinuousRule, AppliesScalarConvolutionAndDt)
{
  const auto rule = make_continuous_rule(
    [](double current, double conv, double dt)
    {
      return current + dt * (conv - current);
    },
    mean_3x3_kernel<double>(),
    /*dt=*/0.5,
    /*min=*/0.0,
    /*max=*/10.0);

  const std::array<double, 8> neighbours{1.0, 1.0, 1.0, 1.0,
                                         1.0, 1.0, 1.0, 1.0};
  EXPECT_NEAR(rule(3.0, Neighbor_View<double>(neighbours.data(),
                                              neighbours.size())),
              19.0 / 9.0, eps);
}

TEST(CAContinuousRule, ValidatesDtNeighbourCountAndOutputRange)
{
  EXPECT_THROW(make_continuous_rule(
                 [](double, double, double) { return 0.0; },
                 mean_3x3_kernel<double>(),
                 /*dt=*/0.0),
               std::domain_error);

  const auto rule = make_continuous_rule(
    [](double, double, double) { return 2.0; },
    mean_3x3_kernel<double>(),
    /*dt=*/1.0,
    /*min=*/0.0,
    /*max=*/1.0);
  const std::array<double, 8> neighbours{};
  EXPECT_THROW(rule(0.0, Neighbor_View<double>(neighbours.data(),
                                               neighbours.size())),
               std::runtime_error);

  const auto ok_rule = make_continuous_rule(
    [](double, double conv) { return conv; },
    mean_3x3_kernel<double>());
  const std::array<double, 2> too_short{};
  EXPECT_THROW(ok_rule(0.0, Neighbor_View<double>(too_short.data(),
                                                  too_short.size())),
               std::length_error);
}

TEST(CAGrayScottRule, ValidatesParameters)
{
  EXPECT_THROW((Gray_Scott_Rule<double>(-0.01, 0.06, 0.16, 0.08)),
               std::domain_error);
  EXPECT_THROW((Gray_Scott_Rule<double>(0.04, 1.01, 0.16, 0.08)),
               std::domain_error);
  EXPECT_THROW((Gray_Scott_Rule<double>(0.04, 0.06, -0.1, 0.08)),
               std::domain_error);
  EXPECT_THROW((Gray_Scott_Rule<double>(0.04, 0.06, 0.16, 0.08, 1.01)),
               std::domain_error);
}

TEST(CAGrayScottRule, UniformRestStateIsStable)
{
  Gray_Scott_Rule<double> rule(0.04, 0.06, 0.16, 0.08);
  const std::array<RD_Cell, 8> neighbours{RD_Cell{1.0, 0.0},
                                          RD_Cell{1.0, 0.0},
                                          RD_Cell{1.0, 0.0},
                                          RD_Cell{1.0, 0.0},
                                          RD_Cell{1.0, 0.0},
                                          RD_Cell{1.0, 0.0},
                                          RD_Cell{1.0, 0.0},
                                          RD_Cell{1.0, 0.0}};

  const RD_Cell next = rule(RD_Cell{1.0, 0.0},
                            Neighbor_View<RD_Cell>(neighbours.data(),
                                                   neighbours.size()));
  EXPECT_DOUBLE_EQ(next.u, 1.0);
  EXPECT_DOUBLE_EQ(next.v, 0.0);
}

TEST(CAGrayScottRule, LocalPatchStepMatchesExplicitEulerFormula)
{
  Gray_Scott_Rule<double> rule(0.04, 0.06, 0.16, 0.08);
  const std::array<RD_Cell, 8> neighbours{RD_Cell{1.0, 0.0},
                                          RD_Cell{1.0, 0.0},
                                          RD_Cell{1.0, 0.0},
                                          RD_Cell{1.0, 0.0},
                                          RD_Cell{1.0, 0.0},
                                          RD_Cell{1.0, 0.0},
                                          RD_Cell{1.0, 0.0},
                                          RD_Cell{1.0, 0.0}};

  const RD_Cell next = rule(RD_Cell{0.5, 0.25},
                            Neighbor_View<RD_Cell>(neighbours.data(),
                                                   neighbours.size()));
  EXPECT_NEAR(next.u, 0.80875, eps);
  EXPECT_NEAR(next.v, 0.17625, eps);
}

TEST(CAGrayScottRule, DeterministicSeedProducesReproduciblePatterns)
{
  constexpr ca_size_t n = 48;
  constexpr std::uint64_t master = 0x9E3779B97F4A7C15ull;
  constexpr std::size_t steps = 360;

  Gray_Scott_Rule<double> rule(0.0367, 0.0649, 0.16, 0.08);
  Synchronous_Engine<RD_Lattice, Gray_Scott_Rule<double>, Moore<2, 1>>
    a(make_gray_scott_seed(n, master), rule);
  Synchronous_Engine<RD_Lattice, Gray_Scott_Rule<double>, Moore<2, 1>>
    b(make_gray_scott_seed(n, master), rule);

  a.run(steps);
  b.run(steps);

  EXPECT_TRUE(frames_equal(a.frame(), b.frame()));

  const VStats stats = stats_for_v(a.frame());
  EXPECT_GT(stats.max_value, 0.18);
  EXPECT_GT(stats.variance, 1e-4);
  EXPECT_GT(stats.peaks, 8u);
  EXPECT_LT(stats.peaks, static_cast<std::size_t>(n * n / 2));
}

TEST(CAFitzHughNagumoRule, LocalRestStepMatchesEquation)
{
  FitzHugh_Nagumo_Rule<double> rule(/*a=*/0.7,
                                    /*b=*/0.8,
                                    /*epsilon=*/0.08,
                                    /*Du=*/1.0,
                                    /*Dv=*/0.0,
                                    /*dt=*/0.01);
  const std::array<RD_Cell, 8> neighbours{};
  const RD_Cell next = rule(RD_Cell{0.0, 0.0},
                            Neighbor_View<RD_Cell>(neighbours.data(),
                                                   neighbours.size()));

  EXPECT_NEAR(next.u, 0.0, eps);
  EXPECT_NEAR(next.v, 0.00056, eps);
}

TEST(CAFitzHughNagumoRule, ValidatesParametersAndBlowUp)
{
  EXPECT_THROW((FitzHugh_Nagumo_Rule<double>(0.7, 0.0, 0.08, 1.0, 0.0)),
               std::domain_error);
  EXPECT_THROW((FitzHugh_Nagumo_Rule<double>(0.7, 0.8, 0.08, 1.0, 0.0, 0.50)),
               std::domain_error);

  FitzHugh_Nagumo_Rule<double> rule(0.7, 0.8, 0.08, 1.0, 0.0, 0.01);
  const std::array<RD_Cell, 8> neighbours{};
  EXPECT_THROW(rule(RD_Cell{1000.0, 0.0},
                    Neighbor_View<RD_Cell>(neighbours.data(),
                                           neighbours.size())),
               std::runtime_error);
}

TEST(CAHistoryCell, RingBufferKeepsRecentStates)
{
  History_Cell<int, 3> h(1);
  EXPECT_EQ(h.current(), 1);
  EXPECT_EQ(h.previous(1), 1);

  h.push(2);
  EXPECT_EQ(h.current(), 2);
  EXPECT_EQ(h.previous(1), 1);
  EXPECT_EQ(h.previous(2), 1);

  h.push(3);
  h.push(5);
  EXPECT_EQ(h.current(), 5);
  EXPECT_EQ(h.previous(1), 3);
  EXPECT_EQ(h.previous(2), 2);
  EXPECT_THROW(h.previous(3), std::out_of_range);
}

TEST(CAHistoryRule, DirectInvocationSupportsHistoryAwareFunctor)
{
  using Cell = History_Cell<int, 3>;
  const auto rule = make_history_rule<3>(
    [](const Cell &current, Neighbor_View<Cell> neighbours) -> int
    {
      int sum = current.current() + current.previous(1);
      for (const auto &n : neighbours)
        sum += n.current();
      return sum;
    });

  const Cell current(2);
  const std::array<Cell, 2> neighbours{Cell(3), Cell(5)};
  const Cell next = rule(current, Neighbor_View<Cell>(neighbours.data(),
                                                     neighbours.size()));
  EXPECT_EQ(next.current(), 12);
  EXPECT_EQ(next.previous(1), 2);
}

TEST(CAHistoryRule, IntegratesWithSynchronousEngine)
{
  using Cell = History_Cell<int, 3>;
  using Hist_Lattice = Lattice<Dense_Cell_Storage<Cell, 1>, ToroidalBoundary>;

  Hist_Lattice lat({4}, Cell(1));
  const auto fib_like = make_history_rule<3>(
    [](int current, int previous) -> int
    {
      return current + previous;
    });

  Synchronous_Engine<Hist_Lattice, decltype(fib_like), Moore<1, 1>>
    engine(lat, fib_like);

  engine.step();
  EXPECT_EQ(engine.frame().at({0}).current(), 2);
  EXPECT_EQ(engine.frame().at({0}).previous(1), 1);

  engine.step();
  EXPECT_EQ(engine.frame().at({0}).current(), 3);
  EXPECT_EQ(engine.frame().at({0}).previous(1), 2);

  engine.step();
  EXPECT_EQ(engine.frame().at({0}).current(), 5);
  EXPECT_EQ(engine.frame().at({0}).previous(1), 3);
}
