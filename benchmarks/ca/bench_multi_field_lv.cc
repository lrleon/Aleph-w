/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file bench_multi_field_lv.cc
 * @brief Perf-gate anchor: 2-field Lotka-Volterra CA in SoA layout, 512x512.
 */

#include <cstddef>
#include <cstdint>
#include <tuple>

#include <benchmarks/ca/bench_support.H>
#include <ca-traits.H>
#include <tpl_ca_multi_field_engine.H>
#include <tpl_ca_multi_field_lattice.H>
#include <tpl_ca_neighborhood.H>

using namespace Aleph::CA;
using namespace Aleph::CA::Bench;

namespace
{

using Grid = Multi_Field_Lattice<Layout_SoA, ToroidalBoundary, 2, double, double>;

/** @brief Discretised two-field Lotka-Volterra reaction-diffusion rule. */
class Lotka_Volterra_Rule
{
  double alpha_, beta_, delta_, gamma_, dt_, diff_;

public:
  Lotka_Volterra_Rule(double alpha, double beta, double delta, double gamma,
                      double dt, double diffusion)
    : alpha_(alpha), beta_(beta), delta_(delta), gamma_(gamma), dt_(dt),
      diff_(diffusion)
  {}

  std::tuple<double, double>
    operator()(const std::tuple<double, double> &cur,
               const std::tuple<Neighbor_View<double>, Neighbor_View<double>> &nb) const
  {
    const double x = std::get<0>(cur);
    const double y = std::get<1>(cur);
    double lap_x = -static_cast<double>(std::get<0>(nb).size()) * x;
    double lap_y = -static_cast<double>(std::get<1>(nb).size()) * y;
    for (const auto &p : std::get<0>(nb)) lap_x += p;
    for (const auto &p : std::get<1>(nb)) lap_y += p;
    const double next_x = x + dt_ * (alpha_ * x - beta_ * x * y + diff_ * lap_x);
    const double next_y = y + dt_ * (delta_ * x * y - gamma_ * y + diff_ * lap_y);
    return {next_x < 0.0 ? 0.0 : next_x, next_y < 0.0 ? 0.0 : next_y};
  }
};

/** @brief Build the homogeneous background plus a symmetry-breaking core.
 * @param side square lattice side length.
 * @return the seeded SoA frame.
 */
Grid make_seed(const ca_size_t side)
{
  Grid lat({side, side});
  lat.fill(std::tuple{1.0, 0.5});
  const ca_size_t cr = lat.size(0) / 2;
  const ca_size_t cc = lat.size(1) / 2;
  for (ca_size_t i = cr - 1; i <= cr + 1; ++i)
    for (ca_size_t j = cc - 1; j <= cc + 1; ++j)
      {
        const Coord_Vec<2> c{static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)};
        lat.set<0>(c, 1.5);
        lat.set<1>(c, 0.8);
      }
  return lat;
}

}  // namespace

int main()
{
  constexpr ca_size_t side = 512;
  constexpr std::size_t steps = 500;
  const double processed_cells
    = static_cast<double>(side) * static_cast<double>(side) * static_cast<double>(steps);

  const Bench_Result result = measure("multi_field_lv_512", processed_cells, resolve_repeats(3), [&]
  {
    Multi_Field_Engine<Grid, Lotka_Volterra_Rule, Von_Neumann<2, 1>> engine(
      make_seed(side),
      Lotka_Volterra_Rule{/*alpha=*/1.0, /*beta=*/0.5, /*delta=*/0.4,
                          /*gamma=*/0.5, /*dt=*/0.05, /*diff=*/0.05},
      Von_Neumann<2, 1>{});
    engine.run(steps);
  });

  emit_json(result);
  return 0;
}