/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file ca_reproductions_smoke_test.cc
 * @brief Reduced per-PR mechanics checks for the weekly CA reproductions.
 */

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <vector>

#include <gtest/gtest.h>

#include <reproductions/ca_reproduction_support.H>
#include <tpl_ca_hashlife.H>
#include <tpl_ca_stochastic_rules.H>

using namespace Aleph::CA;
using namespace Aleph::CA::Reproductions;

TEST(CAReproductionsSmoke, LogLogFitRecoversSyntheticExponent)
{
  std::vector<std::size_t> samples;
  for (std::size_t size = 2; size <= 64; ++size)
    for (std::size_t copy = 0; copy < 20000 / (size * size); ++copy)
      samples.push_back(size);

  const Linear_Fit fit = fit_log_log_histogram(samples, 2, 64, 10);
  const Linear_Fit weighted = fit_log_log_histogram(samples, 2, 64, 10, 1.0);
  EXPECT_NEAR(fit.slope, -2.0, 0.25);
  EXPECT_NEAR(weighted.slope, fit.slope - 1.0, 0.10);
  EXPECT_GT(fit.r_squared, 0.95);
}

TEST(CAReproductionsSmoke, BTWSandpileRelaxesSequentialDrops)
{
  static_assert(not BTW_Sandpile::update_scheme::requires_double_buffer);
  BTW_Sandpile sandpile(16, 0x425457u);
  std::size_t nonzero = 0;
  for (std::size_t i = 0; i < 2000; ++i)
    if (sandpile.drop_random().size != 0)
      ++nonzero;

  EXPECT_TRUE(sandpile.stable());
  EXPECT_GT(nonzero, 0u);
}

TEST(CAReproductionsSmoke, SchellingMoransIDetectsClusters)
{
  using Grid = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;
  constexpr int a = static_cast<int>(Schelling_Cell::TYPE_A);
  constexpr int b = static_cast<int>(Schelling_Cell::TYPE_B);
  constexpr int empty = static_cast<int>(Schelling_Cell::EMPTY);
  Grid frame({20, 20}, a);
  for (ca_size_t row = 0; row < frame.size(0); ++row)
    for (ca_size_t column = frame.size(1) / 2; column < frame.size(1); ++column)
      frame.set({static_cast<ca_index_t>(row), static_cast<ca_index_t>(column)}, b);

  EXPECT_GT(morans_i_binary(frame, empty, a, b), 0.7);
}

TEST(CAReproductionsSmoke, DrosselSchwablFirePropagatesLocally)
{
  using Grid = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;
  constexpr int tree = static_cast<int>(Forest_Cell::TREE);
  constexpr int burning = static_cast<int>(Forest_Cell::BURNING);
  constexpr int empty = static_cast<int>(Forest_Cell::EMPTY);
  Grid frame({9, 9}, tree);
  frame.set({4, 4}, burning);
  Synchronous_Engine<Grid, Forest_Fire_Rule<>, Moore<2, 1>>
    engine(std::move(frame), Forest_Fire_Rule<>(0.0, 0.0, 0xD2055u));

  engine.step();
  std::size_t burning_count = 0;
  for (ca_size_t row = 0; row < engine.frame().size(0); ++row)
    for (ca_size_t column = 0; column < engine.frame().size(1); ++column)
      if (engine.frame().at({static_cast<ca_index_t>(row),
                             static_cast<ca_index_t>(column)}) == burning)
        ++burning_count;
  EXPECT_EQ(engine.frame().at({4, 4}), empty);
  EXPECT_EQ(burning_count, 8u);
}

TEST(CAReproductionsSmoke, GrayScottGoldenComparisonIsPixelBased)
{
  const Gray_Scott_Lattice frame
    = run_gray_scott(gray_scott_presets[2], 24, 40, 0x5eedu);
  std::ostringstream lhs(std::ios::binary);
  std::ostringstream rhs(std::ios::binary);
  write_png(lhs, frame, gray_scott_rgb);
  write_png(rhs, frame, gray_scott_rgb);
  std::istringstream lhs_in(lhs.str(), std::ios::binary);
  std::istringstream rhs_in(rhs.str(), std::ios::binary);

  EXPECT_DOUBLE_EQ(mean_channel_difference(decode_native_png(lhs_in),
                                           decode_native_png(rhs_in)),
                   0.0);
}

TEST(CAReproductionsSmoke, GosperGunEmitsGlidersUnderHashlife)
{
  Hashlife_Engine engine;
  for_each_gosper_gun_cell([&](const std::int64_t x, const std::int64_t y)
  {
    engine.set_alive(x, y);
  });
  const std::uint64_t initial = engine.population();
  engine.run(120);
  EXPECT_GT(engine.population(), initial);
}
