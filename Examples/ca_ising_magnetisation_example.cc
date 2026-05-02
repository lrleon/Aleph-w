/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file ca_ising_magnetisation_example.cc
 * @brief Phase 8 example: Ising spins with Glauber dynamics + temperature sweep.
 *
 * Builds a 32x32 toroidal Ising lattice with Von-Neumann neighbours
 * (degree 4) and runs the Glauber single-spin-flip dynamics from a
 * disordered initial state. The example sweeps a small grid of
 * temperatures (T = 1.0, 2.0, 2.27, 2.5, 4.0) and reports, for each
 * temperature, the average absolute magnetisation per site after a
 * burn-in.
 *
 * The exact 2-D Ising critical temperature on the square lattice is
 *     T_c = 2 * J / ln(1 + sqrt(2)) ~ 2.269 J
 * so we expect:
 *
 *   - T < T_c: ordered phase, |<m>| close to 1.
 *   - T ~ T_c: critical fluctuations.
 *   - T > T_c: disordered phase, |<m>| close to 0.
 *
 * After the sweep we render the final spin configuration of the cold,
 * critical, and hot regimes so the visual contrast is obvious.
 *
 *  Glyphs:  '+' spin up    '-' spin down
 */

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>

#include <ca-rng.H>
#include <ca-traits.H>
#include <tpl_ca_engine.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_storage.H>
#include <tpl_ca_stochastic_rules.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace
{

using Ising_Lattice = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;

/// Seed a slightly biased initial configuration. The bias is the
/// standard pedagogical trick that breaks the Z2 symmetry of the Ising
/// Hamiltonian — synchronous dynamics from a perfectly symmetric
/// 50/50 init can get stuck in metastable mixed states well below T_c.
void seed_disordered(Ising_Lattice &lat, std::uint32_t rng_seed,
                     double up_fraction = 0.60)
{
  std::mt19937 rng(rng_seed);
  std::bernoulli_distribution flip(up_fraction);
  for (ca_size_t i = 0; i < lat.size(0); ++i)
    for (ca_size_t j = 0; j < lat.size(1); ++j)
      lat.set({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)},
              flip(rng) ? 1 : -1);
}

double mean_magnetisation(const Ising_Lattice &lat)
{
  long long s = 0;
  for (ca_size_t i = 0; i < lat.size(0); ++i)
    for (ca_size_t j = 0; j < lat.size(1); ++j)
      s += lat.at({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)});
  return static_cast<double>(s)
         / static_cast<double>(lat.size(0) * lat.size(1));
}

void render(const Ising_Lattice &lat, std::ostream &os)
{
  for (ca_size_t i = 0; i < lat.size(0); ++i)
    {
      for (ca_size_t j = 0; j < lat.size(1); ++j)
        os << (lat.at({static_cast<ca_index_t>(i),
                       static_cast<ca_index_t>(j)}) > 0 ? '+' : '-');
      os << '\n';
    }
}

std::string bar(double absolute, std::size_t width = 40)
{
  const std::size_t k
    = std::min(width, static_cast<std::size_t>(absolute
                                                * static_cast<double>(width)));
  return std::string(k, '|') + std::string(width - k, ' ');
}

struct Run_Result
{
  double avg_abs_m = 0.0;
  Ising_Lattice final_frame;
};

Run_Result run_at_temperature(const Ising_Lattice &seed,
                              double T,
                              std::uint64_t master_seed,
                              std::size_t burn_in,
                              std::size_t sample_steps)
{
  Ising_Glauber_Rule<> rule(T, /*J=*/1.0, /*H=*/0.0, master_seed);
  Synchronous_Engine<Ising_Lattice, Ising_Glauber_Rule<>, Von_Neumann<2, 1>>
    engine(seed, rule);

  // Burn-in: discard transient.
  engine.run(burn_in);

  // Sample magnetisation over the next `sample_steps` frames.
  double acc = 0.0;
  for (std::size_t s = 0; s < sample_steps; ++s)
    {
      engine.step();
      acc += std::abs(mean_magnetisation(engine.frame()));
    }
  return {acc / static_cast<double>(sample_steps), engine.frame()};
}

}  // namespace

int main()
{
  constexpr ca_size_t rows = 32;
  constexpr ca_size_t cols = 32;
  constexpr std::uint64_t master_seed = 0xCAFEBABEull;
  constexpr std::size_t burn_in = 200;
  constexpr std::size_t sample_steps = 200;

  std::cout << "Ising / Glauber dynamics on a 32x32 toroidal lattice\n";
  std::cout << "  J = 1.0  H = 0.0  Von-Neumann radius 1 (degree 4)\n";
  std::cout << "  initial bias = 60/40 up/down (breaks Z2 symmetry)\n";
  std::cout << "  exact 2-D critical temperature  T_c ~ 2.269\n";
  std::cout << "  master_seed = 0x" << std::hex << master_seed << std::dec
            << "  burn_in = " << burn_in
            << "  sample_steps = " << sample_steps << "\n\n";

  Ising_Lattice seed({rows, cols});
  seed_disordered(seed, /*rng_seed=*/0xBEEFu);

  const std::array<double, 5> Ts = {1.0, 2.0, 2.269, 2.5, 4.0};

  std::cout << "Temperature sweep:\n";
  std::cout << "    T    |<m>|             magnetisation curve\n";
  std::cout << "  -----  -----   ----------------------------------------\n";

  Run_Result cold{}, critical{}, hot{};
  for (std::size_t k = 0; k < Ts.size(); ++k)
    {
      const double T = Ts[k];
      const Run_Result r
        = run_at_temperature(seed, T, master_seed, burn_in, sample_steps);
      std::cout << "  " << std::setw(5) << std::fixed << std::setprecision(3) << T
                << "  " << std::setw(5) << std::fixed << std::setprecision(3)
                << r.avg_abs_m
                << "   [" << bar(r.avg_abs_m) << "]\n";
      if (k == 0)            cold     = r;
      else if (k == 2)       critical = r;
      else if (k == Ts.size() - 1) hot = r;
    }
  std::cout << std::defaultfloat;

  std::cout << "\n--- Final spin configurations ---\n";
  std::cout << "\nT = 1.0  (ordered, |<m>| ~ 1)\n";
  render(cold.final_frame, std::cout);
  std::cout << "\nT = 2.269 (~T_c, critical fluctuations)\n";
  render(critical.final_frame, std::cout);
  std::cout << "\nT = 4.0  (disordered, |<m>| ~ 0)\n";
  render(hot.final_frame, std::cout);

  std::cout << "\nNote: the same `master_seed` reproduces every value\n"
               "      bit-for-bit on any thread count via cell_seed().\n";
  return 0;
}