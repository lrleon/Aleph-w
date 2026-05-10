/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file ca_predator_prey_multifield_example.cc
 * @brief Phase 14 example: a 2-field Lotka–Volterra CA in SoA layout.
 *
 * Each cell carries two coupled real fields, prey `x` and predator
 * `y`, evolving under a discretised Lotka–Volterra system with
 * spatial diffusion:
 *
 *   dx/dt = α x − β x y + Du · ∇²x
 *   dy/dt = δ x y − γ y + Dv · ∇²y
 *
 * The example uses `Multi_Field_Lattice<Layout_SoA, ...>` so each
 * field owns its own contiguous buffer (vectorisable, the natural
 * layout for large grids), prints periodic ASCII frames, and reports
 * the global prey/predator population every snapshot so the
 * oscillation is visible at a glance.
 *
 * Run:
 *   ./ca_predator_prey_multifield_example [steps] [side]
 */

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <tuple>

#include <ca-traits.H>
#include <tpl_ca_multi_field_engine.H>
#include <tpl_ca_multi_field_lattice.H>
#include <tpl_ca_neighborhood.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace
{

using Lattice2 = Multi_Field_Lattice<Layout_SoA, ToroidalBoundary, 2, double, double>;

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

void seed_centre(Lattice2 &lat)
{
  // Homogeneous background with a 3×3 perturbation at the centre that
  // breaks symmetry and seeds an oscillation.
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
}

char glyph_for(const double x, const double y)
{
  if (x > 1.5 and y < 0.4) return 'X';        // prey-rich, predators few
  if (y > 0.7 and x < 0.6) return 'O';        // predator outbreak
  if (x > 1.0 and y > 0.4) return '#';        // mixed
  if (x < 0.3 and y < 0.3) return '.';        // depleted
  return '+';
}

void render(const Lattice2 &lat, const std::size_t step, std::ostream &os)
{
  double x_total = 0.0, y_total = 0.0;
  for (ca_size_t i = 0; i < lat.size(0); ++i)
    {
      for (ca_size_t j = 0; j < lat.size(1); ++j)
        {
          const Coord_Vec<2> c{static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)};
          const double x = lat.at<0>(c);
          const double y = lat.at<1>(c);
          x_total += x;
          y_total += y;
          os << glyph_for(x, y);
        }
      os << '\n';
    }
  os << "step " << step << "  Σprey=" << x_total << "  Σpred=" << y_total << "\n";
}

}  // namespace

int main(int argc, char **argv)
{
  std::size_t steps = 200;
  ca_size_t side = 32;
  try
    {
      if (argc >= 2) steps = static_cast<std::size_t>(std::stoul(argv[1]));
      if (argc >= 3) side = static_cast<ca_size_t>(std::stoul(argv[2]));
    }
  catch (const std::invalid_argument &)
    {
      std::cerr << "Invalid argument: expected non-negative integers for [steps] [side]\n";
      return 1;
    }
  catch (const std::out_of_range &)
    {
      std::cerr << "Argument out of range for [steps] [side]\n";
      return 1;
    }
  if (side < 4)
    {
      std::cerr << "Invalid side: must be ≥ 4 (need a centre with surrounding cells)\n";
      return 1;
    }

  Lattice2 lat({side, side});
  seed_centre(lat);

  std::cout << "Lotka–Volterra multi-field CA (SoA) — " << side << "×" << side
            << ", " << steps << " steps\n";
  std::cout << "Glyphs: '.' depleted  '+' mixed  '#' rich  'X' prey-rich  'O' predator surge\n\n";

  Multi_Field_Engine<Lattice2, Lotka_Volterra_Rule, Von_Neumann<2, 1>> eng(
    std::move(lat),
    Lotka_Volterra_Rule{/*alpha=*/1.0, /*beta=*/0.5, /*delta=*/0.4,
                        /*gamma=*/0.5, /*dt=*/0.05, /*diff=*/0.05},
    Von_Neumann<2, 1>{});

  std::cout << "Initial frame:\n";
  render(eng.frame(), 0, std::cout);
  std::cout << '\n';

  const std::size_t snapshot_every = steps / 4 == 0 ? 1 : steps / 4;
  for (std::size_t t = 0; t < steps; ++t)
    {
      eng.step();
      if ((t + 1) % snapshot_every == 0 or t + 1 == steps)
        {
          std::cout << "\nSnapshot:\n";
          render(eng.frame(), t + 1, std::cout);
        }
    }

  return 0;
}