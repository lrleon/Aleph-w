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
 * @file ca_multi_field_test.cc
 * @brief Phase 14 tests for multi-field CA lattices, rules and engine.
 *
 *  Coverage:
 *   - AoS vs SoA equivalence on a discrete predator-prey rule
 *     (frame must match bit-by-bit at every step).
 *   - Per-field SoA buffer is reachable as a regular `Lattice` and
 *     can be handed to the legacy `Synchronous_Engine`.
 *   - Lotka-Volterra continuous rule on a 64×64 toroidal grid stays
 *     bounded and exhibits a population cycle.
 *   - Gray-Scott reaction-diffusion migrated from the Phase 9
 *     monolithic `Reaction_Diffusion_Cell` rule to a multi-field
 *     `(u, v)` rule produces bit-exact identical frames.
 *   - `Field_Slice_Rule` propagates the untouched fields verbatim.
 */

#include <array>
#include <cmath>
#include <cstdint>
#include <tuple>
#include <vector>

#include <gtest/gtest.h>

#include <ca-traits.H>
#include <tpl_ca_continuous_rules.H>
#include <tpl_ca_engine.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_multi_field_engine.H>
#include <tpl_ca_multi_field_lattice.H>
#include <tpl_ca_multi_field_rule.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_rule.H>
#include <tpl_ca_storage.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace
{

// ---------- Discrete predator-prey rule (integer fields) -------------

struct Discrete_Predator_Prey_Rule
{
  // Fields: (prey, predator), both std::int32_t. Toy update used as a
  // golden discrete oracle to compare AoS and SoA frame-by-frame.
  using fields = std::tuple<std::int32_t, std::int32_t>;
  using nb_views
    = std::tuple<Neighbor_View<std::int32_t>, Neighbor_View<std::int32_t>>;

  std::int32_t operator()(const fields &cur, const nb_views &nb) const noexcept = delete;

  fields operator_impl(const fields &cur, const nb_views &nb) const noexcept
  {
    const auto prey = std::get<0>(cur);
    const auto pred = std::get<1>(cur);
    std::int32_t prey_n = 0;
    std::int32_t pred_n = 0;
    for (const auto &p : std::get<0>(nb)) prey_n += p;
    for (const auto &p : std::get<1>(nb)) pred_n += p;
    // Discrete coupling: prey grows from neighbour reservoir minus
    // local predator pressure; predators average neighbour density and
    // gain from prey but lose by self-density.
    const std::int32_t next_prey = (prey + (prey_n / 8) - pred);
    const std::int32_t next_pred = ((pred_n / 8) + (prey / 4) - (pred / 2));
    return {next_prey, next_pred};
  }
};

// Functor-callable variant matching Multi_Field_Rule contract.
struct Discrete_PP
{
  using fields = std::tuple<std::int32_t, std::int32_t>;
  using nb_views
    = std::tuple<Neighbor_View<std::int32_t>, Neighbor_View<std::int32_t>>;
  fields operator()(const fields &cur, const nb_views &nb) const noexcept
  {
    const auto prey = std::get<0>(cur);
    const auto pred = std::get<1>(cur);
    std::int32_t prey_n = 0;
    std::int32_t pred_n = 0;
    for (const auto &p : std::get<0>(nb)) prey_n += p;
    for (const auto &p : std::get<1>(nb)) pred_n += p;
    const std::int32_t next_prey = (prey + (prey_n / 8) - pred);
    const std::int32_t next_pred = ((pred_n / 8) + (prey / 4) - (pred / 2));
    return {next_prey, next_pred};
  }
};

template <typename Layout>
using PP_Lattice = Multi_Field_Lattice<Layout, ToroidalBoundary, 2,
                                       std::int32_t, std::int32_t>;

template <typename Layout>
PP_Lattice<Layout> make_seeded_pp(const ca_size_t side, const std::uint32_t seed)
{
  PP_Lattice<Layout> lat({side, side});
  // Deterministic seeding (no RNG dependency).
  for (ca_size_t i = 0; i < side; ++i)
    for (ca_size_t j = 0; j < side; ++j)
      {
        const std::int32_t prey = static_cast<std::int32_t>((i + j + seed) % 7);
        const std::int32_t pred = static_cast<std::int32_t>((i * 2 + j + seed) % 5);
        lat.template set<0>({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)},
                            prey);
        lat.template set<1>({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)},
                            pred);
      }
  return lat;
}

template <typename A, typename B>
bool frames_equal_pp(const A &a, const B &b)
{
  if (a.extents() != b.extents())
    return false;
  for (ca_size_t i = 0; i < a.size(0); ++i)
    for (ca_size_t j = 0; j < a.size(1); ++j)
      {
        const Coord_Vec<2> c{static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)};
        if (a.template at<0>(c) != b.template at<0>(c))
          return false;
        if (a.template at<1>(c) != b.template at<1>(c))
          return false;
      }
  return true;
}

}  // namespace

// =====================================================================
// AoS vs SoA equivalence
// =====================================================================

TEST(CAMultiField, AoSAndSoAProduceIdenticalFramesUnderDiscreteRule)
{
  constexpr ca_size_t side = 16;
  auto aos = make_seeded_pp<Layout_AoS>(side, 17);
  auto soa = make_seeded_pp<Layout_SoA>(side, 17);

  ASSERT_TRUE(frames_equal_pp(aos, soa)) << "initial frames differ";

  using Aos_Eng = Multi_Field_Engine<PP_Lattice<Layout_AoS>, Discrete_PP, Moore<2, 1>>;
  using Soa_Eng = Multi_Field_Engine<PP_Lattice<Layout_SoA>, Discrete_PP, Moore<2, 1>>;

  Aos_Eng aeng(std::move(aos), Discrete_PP{}, Moore<2, 1>{});
  Soa_Eng seng(std::move(soa), Discrete_PP{}, Moore<2, 1>{});

  for (std::size_t t = 0; t < 25; ++t)
    {
      ASSERT_TRUE(frames_equal_pp(aeng.frame(), seng.frame()))
        << "diverged at step " << t;
      aeng.step();
      seng.step();
    }
  EXPECT_TRUE(frames_equal_pp(aeng.frame(), seng.frame()))
    << "diverged after 25 steps";
}

// =====================================================================
// SoA per-field buffer plays nicely with mono-field engine
// =====================================================================

TEST(CAMultiField, SoAFieldExposesMonoFieldLatticeForLegacyEngine)
{
  using L = Multi_Field_Lattice<Layout_SoA, OpenBoundary, 2, int, double>;
  L lat({4, 4});
  // Seed only field<0> via the legacy Lattice<> reference.
  auto &f0 = lat.field<0>();
  for (ca_size_t i = 0; i < 4; ++i)
    for (ca_size_t j = 0; j < 4; ++j)
      f0.set({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)},
             static_cast<int>((i + j) & 1));

  // Run a mono-field Game-of-Life engine on field<0> only.
  using Mono_Eng = Synchronous_Engine<
    Lattice<Dense_Cell_Storage<int, 2>, OpenBoundary>, Game_Of_Life_Rule, Moore<2, 1>>;
  Mono_Eng eng(f0, make_game_of_life_rule(), Moore<2, 1>{});
  eng.step();

  // The SoA lattice's field<1> must remain at its default value (0.0).
  for (ca_size_t i = 0; i < 4; ++i)
    for (ca_size_t j = 0; j < 4; ++j)
      EXPECT_DOUBLE_EQ(
        lat.at<1>({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)}),
        0.0);
}

// =====================================================================
// Multi-field Gray-Scott migration: identical to monolithic rule
// =====================================================================

namespace
{

// A multi-field Gray-Scott rule that consumes (u, v) views directly
// instead of the monolithic Reaction_Diffusion_Cell. The arithmetic
// is reproduced verbatim from Gray_Scott_Rule so frames must match
// the monolithic version bit-by-bit.
class Multi_Field_Gray_Scott
{
  double feed_, kill_, du_, dv_, dt_;
  Kernel2D<double, 3, 3> lap_ = laplacian_5p_kernel<double>();

public:
  Multi_Field_Gray_Scott(double feed, double kill, double du, double dv, double dt = 1.0)
    : feed_(feed), kill_(kill), du_(du), dv_(dv), dt_(dt)
  {}

  std::tuple<double, double>
    operator()(const std::tuple<double, double> &cur,
               const std::tuple<Neighbor_View<double>, Neighbor_View<double>> &nb) const
  {
    const double u = std::get<0>(cur);
    const double v = std::get<1>(cur);
    const auto &uv = std::get<0>(nb);
    const auto &vv = std::get<1>(nb);
    double lu = u * lap_.center();
    double lv = v * lap_.center();
    for (std::size_t k = 0; k < uv.size(); ++k)
      lu += uv[k] * lap_.neighbour_weight(k);
    for (std::size_t k = 0; k < vv.size(); ++k)
      lv += vv[k] * lap_.neighbour_weight(k);
    const double uvv = u * v * v;
    double next_u = u + dt_ * (du_ * lu - uvv + feed_ * (1.0 - u));
    double next_v = v + dt_ * (dv_ * lv + uvv - (feed_ + kill_) * v);
    // Mirror the clamp-after-check logic from Gray_Scott_Rule.
    if (next_u < 0.0) next_u = 0.0;
    else if (next_u > 1.0) next_u = 1.0;
    if (next_v < 0.0) next_v = 0.0;
    else if (next_v > 1.0) next_v = 1.0;
    return {next_u, next_v};
  }
};

}  // namespace

TEST(CAMultiField, GrayScottMonolithicAndMultiFieldAgreeBitByBit)
{
  using Cell = Reaction_Diffusion_Cell<double>;
  constexpr ca_size_t side = 24;
  using Mono_Lat = Lattice<Dense_Cell_Storage<Cell, 2>, ToroidalBoundary>;
  Mono_Lat mono({side, side}, Cell{1.0, 0.0});

  using MF_Lat
    = Multi_Field_Lattice<Layout_AoS, ToroidalBoundary, 2, double, double>;
  MF_Lat mf({side, side}, std::tuple{1.0, 0.0});

  // Seed a small v-rich square at the centre — same on both lattices.
  for (ca_size_t i = side / 2 - 2; i < side / 2 + 2; ++i)
    for (ca_size_t j = side / 2 - 2; j < side / 2 + 2; ++j)
      {
        const Coord_Vec<2> c{static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)};
        mono.set(c, Cell{0.5, 0.25});
        mf.set<0>(c, 0.5);
        mf.set<1>(c, 0.25);
      }

  Synchronous_Engine<Mono_Lat, Gray_Scott_Rule<>, Moore<2, 1>> meng(
    std::move(mono), Gray_Scott_Rule<>{0.0367, 0.0649, 0.16, 0.08, 1.0},
    Moore<2, 1>{});
  Multi_Field_Engine<MF_Lat, Multi_Field_Gray_Scott, Moore<2, 1>> feng(
    std::move(mf), Multi_Field_Gray_Scott{0.0367, 0.0649, 0.16, 0.08, 1.0},
    Moore<2, 1>{});

  for (std::size_t t = 0; t < 50; ++t)
    {
      meng.step();
      feng.step();
    }

  // Bit-by-bit identical (same arithmetic, same neighbour ordering,
  // same boundary policy).
  for (ca_size_t i = 0; i < side; ++i)
    for (ca_size_t j = 0; j < side; ++j)
      {
        const Coord_Vec<2> c{static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)};
        const Cell mc = meng.frame().at(c);
        ASSERT_DOUBLE_EQ(feng.frame().at<0>(c), mc.u)
          << "u differs at (" << i << "," << j << ")";
        ASSERT_DOUBLE_EQ(feng.frame().at<1>(c), mc.v)
          << "v differs at (" << i << "," << j << ")";
      }
}

// =====================================================================
// Lotka-Volterra continuous predator-prey
// =====================================================================

namespace
{

class Lotka_Volterra_Rule
{
  double alpha_, beta_, delta_, gamma_, dt_;
  double diff_;

public:
  Lotka_Volterra_Rule(double alpha, double beta, double delta, double gamma,
                      double dt, double diffusion)
    : alpha_(alpha), beta_(beta), delta_(delta), gamma_(gamma), dt_(dt),
      diff_(diffusion) {}

  std::tuple<double, double>
    operator()(const std::tuple<double, double> &cur,
               const std::tuple<Neighbor_View<double>, Neighbor_View<double>> &nb) const
  {
    const double prey = std::get<0>(cur);
    const double pred = std::get<1>(cur);
    // 4-neighbour average (we use Von_Neumann<2,1>).
    double prey_lap = -static_cast<double>(std::get<0>(nb).size()) * prey;
    double pred_lap = -static_cast<double>(std::get<1>(nb).size()) * pred;
    for (const auto &p : std::get<0>(nb)) prey_lap += p;
    for (const auto &p : std::get<1>(nb)) pred_lap += p;
    const double next_prey = prey + dt_ * (alpha_ * prey - beta_ * prey * pred
                                           + diff_ * prey_lap);
    const double next_pred = pred + dt_ * (delta_ * prey * pred - gamma_ * pred
                                           + diff_ * pred_lap);
    return {next_prey < 0.0 ? 0.0 : next_prey,
            next_pred < 0.0 ? 0.0 : next_pred};
  }
};

}  // namespace

TEST(CAMultiField, LotkaVolterraStaysBoundedAndOscillates)
{
  using L = Multi_Field_Lattice<Layout_SoA, ToroidalBoundary, 2, double, double>;
  constexpr ca_size_t side = 32;
  L lat({side, side}, std::tuple{1.0, 0.5});  // homogeneous start

  // Perturb the centre to break symmetry.
  for (ca_size_t i = side / 2 - 1; i <= side / 2 + 1; ++i)
    for (ca_size_t j = side / 2 - 1; j <= side / 2 + 1; ++j)
      {
        const Coord_Vec<2> c{static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)};
        lat.set<0>(c, 1.5);
        lat.set<1>(c, 0.8);
      }

  Multi_Field_Engine<L, Lotka_Volterra_Rule, Von_Neumann<2, 1>> eng(
    std::move(lat), Lotka_Volterra_Rule{1.0, 0.5, 0.4, 0.5, 0.05, 0.05},
    Von_Neumann<2, 1>{});

  // Track total prey/predator population every step. With these
  // parameters the system should oscillate (no monotonic blow-up).
  std::vector<double> prey_history;
  std::vector<double> pred_history;
  prey_history.reserve(200);
  pred_history.reserve(200);
  for (std::size_t t = 0; t < 200; ++t)
    {
      eng.step();
      double prey_total = 0;
      double pred_total = 0;
      for (ca_size_t i = 0; i < side; ++i)
        for (ca_size_t j = 0; j < side; ++j)
          {
            const Coord_Vec<2> c{static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)};
            prey_total += eng.frame().at<0>(c);
            pred_total += eng.frame().at<1>(c);
            ASSERT_GE(eng.frame().at<0>(c), 0.0) << "prey<0 at step " << t;
            ASSERT_GE(eng.frame().at<1>(c), 0.0) << "pred<0 at step " << t;
            ASSERT_LT(eng.frame().at<0>(c), 1e6) << "prey blew up at step " << t;
            ASSERT_LT(eng.frame().at<1>(c), 1e6) << "pred blew up at step " << t;
          }
      prey_history.push_back(prey_total);
      pred_history.push_back(pred_total);
    }

  // Detect at least one prey local extremum (a turning point) — a
  // direct sanity check that the system actually oscillates rather
  // than monotonically driving to a fixed point.
  std::size_t turning_points = 0;
  for (std::size_t k = 1; k + 1 < prey_history.size(); ++k)
    {
      const double a = prey_history[k - 1];
      const double b = prey_history[k];
      const double c = prey_history[k + 1];
      if ((b > a and b > c) or (b < a and b < c))
        ++turning_points;
    }
  EXPECT_GT(turning_points, 1u)
    << "Lotka-Volterra prey trajectory shows no oscillation";
}

// =====================================================================
// Field_Slice_Rule: untouched fields propagate verbatim
// =====================================================================

TEST(CAMultiField, FieldSliceRulePreservesOtherFields)
{
  using L = Multi_Field_Lattice<Layout_SoA, OpenBoundary, 2, int, int>;
  L lat({3, 3}, std::tuple{0, 7});  // field 0 = 0, field 1 = 7
  // Place a single live cell in field<0>; field<1> stays at 7 everywhere.
  lat.set<0>({1, 1}, 1);

  Field_Slice_Rule<0, Game_Of_Life_Rule> slice(make_game_of_life_rule());
  Multi_Field_Engine<L, decltype(slice), Moore<2, 1>> eng(
    std::move(lat), std::move(slice), Moore<2, 1>{});
  eng.run(5);

  // Field<1> must be 7 everywhere (untouched by the GoL slice on
  // field<0>).
  for (ca_size_t i = 0; i < 3; ++i)
    for (ca_size_t j = 0; j < 3; ++j)
      EXPECT_EQ(eng.frame().at<1>({static_cast<ca_index_t>(i),
                                   static_cast<ca_index_t>(j)}),
                7);

  // Field<0> must actually evolve under Game of Life: a lone live cell
  // with no live neighbours dies on the very first step (B3/S23) and
  // stays dead forever, so after 5 steps the seeded centre must be 0.
  // Without this check the test could pass with a slice rule that did
  // nothing on field<0>.
  EXPECT_EQ(eng.frame().at<0>({static_cast<ca_index_t>(1),
                               static_cast<ca_index_t>(1)}),
            0);
  // For good measure, the whole field<0> must be empty (no cell has
  // ever had three live neighbours so no birth can have occurred).
  for (ca_size_t i = 0; i < 3; ++i)
    for (ca_size_t j = 0; j < 3; ++j)
      EXPECT_EQ(eng.frame().at<0>({static_cast<ca_index_t>(i),
                                   static_cast<ca_index_t>(j)}),
                0);
}