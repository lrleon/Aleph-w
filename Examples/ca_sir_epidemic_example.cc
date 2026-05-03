/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file ca_sir_epidemic_example.cc
 * @brief Phase 8 example: SIR epidemic on a 2-D toroidal lattice.
 *
 * Runs the discrete-time SIR rule
 *
 *     S -> I  with probability 1 - (1 - beta)^k        (k infected nbrs)
 *     I -> R  with probability gamma
 *     R -> R                                            (immune)
 *
 * for a 64x64 toroidal grid seeded with a single infected cell at the
 * centre. The example contrasts two regimes:
 *
 *   - "Subcritical" (R0 ~ 0.7): epidemic dies out rapidly.
 *   - "Supercritical" (R0 ~ 3.2): wavefront spreads radially until the
 *     supply of susceptibles is exhausted.
 *
 *  After every snapshot the example prints the lattice in ASCII plus
 *  the (S, I, R) tally and a tiny prevalence bar so the output reads
 *  like a printed epidemic curve.
 *
 *  Glyphs:  '.' susceptible  '*' infected  '#' recovered
 */

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
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

using SIR_Lattice = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;

struct Tally
{
  std::size_t s = 0;
  std::size_t i = 0;
  std::size_t r = 0;
};

Tally count(const SIR_Lattice &lat)
{
  Tally t;
  for (ca_size_t i = 0; i < lat.size(0); ++i)
    for (ca_size_t j = 0; j < lat.size(1); ++j)
      {
        const int v = lat.at({static_cast<ca_index_t>(i),
                              static_cast<ca_index_t>(j)});
        if (v == static_cast<int>(SIR_Cell::S))      ++t.s;
        else if (v == static_cast<int>(SIR_Cell::I)) ++t.i;
        else                                         ++t.r;
      }
  return t;
}

char glyph_for(int v)
{
  switch (static_cast<SIR_Cell>(v))
    {
    case SIR_Cell::S: return '.';
    case SIR_Cell::I: return '*';
    case SIR_Cell::R: return '#';
    }
  return '?';
}

void render(const SIR_Lattice &lat, std::ostream &os)
{
  for (ca_size_t i = 0; i < lat.size(0); ++i)
    {
      for (ca_size_t j = 0; j < lat.size(1); ++j)
        os << glyph_for(lat.at({static_cast<ca_index_t>(i),
                                static_cast<ca_index_t>(j)}));
      os << '\n';
    }
}

std::string bar(std::size_t v, std::size_t total, std::size_t width = 40)
{
  if (total == 0) return std::string(width, ' ');
  const std::size_t k
    = std::min(width, static_cast<std::size_t>(
                        (static_cast<double>(v) / static_cast<double>(total))
                        * static_cast<double>(width)));
  return std::string(k, '|') + std::string(width - k, ' ');
}

void run_scenario(const char *name, double beta, double gamma,
                  std::uint64_t master_seed,
                  std::size_t total_steps,
                  std::size_t snapshot_every)
{
  constexpr ca_size_t rows = 64;
  constexpr ca_size_t cols = 64;

  // Moore radius 1 has degree 8 ⇒ R0 estimate = beta * 8 / gamma.
  const double R0 = beta * 8.0 / gamma;

  std::cout << "\n========================================\n";
  std::cout << "  Scenario: " << name << "\n";
  std::cout << "  size=" << rows << "x" << cols
            << "  beta=" << beta << "  gamma=" << gamma
            << "  R0~" << std::fixed << std::setprecision(2) << R0
            << std::defaultfloat
            << "  seed=0x" << std::hex << master_seed << std::dec << '\n';
  std::cout << "========================================\n";

  SIR_Lattice lat({rows, cols}, static_cast<int>(SIR_Cell::S));
  lat.set({rows / 2, cols / 2}, static_cast<int>(SIR_Cell::I));

  SIR_Rule<> rule(beta, gamma, master_seed);
  Synchronous_Engine<SIR_Lattice, SIR_Rule<>, Moore<2, 1>> engine(lat, rule);

  const std::size_t total = rows * cols;
  std::size_t peak_I = 1;
  std::size_t peak_step = 0;

  std::cout << "\n--- step 0 (initial) ---\n";
  render(engine.frame(), std::cout);
  Tally t = count(engine.frame());
  std::cout << "S=" << t.s << " I=" << t.i << " R=" << t.r
            << "  [" << bar(t.i, total) << "]\n";

  for (std::size_t step = 1; step <= total_steps; ++step)
    {
      engine.step();
      t = count(engine.frame());
      if (t.i > peak_I) { peak_I = t.i; peak_step = step; }
      if (step % snapshot_every == 0 or step == total_steps or t.i == 0)
        {
          std::cout << "\n--- step " << step << " ---\n";
          render(engine.frame(), std::cout);
          std::cout << "S=" << t.s << " I=" << t.i << " R=" << t.r
                    << "  [" << bar(t.i, total) << "]\n";
        }
      if (t.i == 0)
        {
          std::cout << "\n[epidemic extinct at step " << step << "]\n";
          break;
        }
    }

  std::cout << "\nSummary: peak prevalence I=" << peak_I
            << " at step " << peak_step
            << "  (final attack rate R="
            << std::fixed << std::setprecision(3)
            << (static_cast<double>(t.r) / static_cast<double>(total))
            << std::defaultfloat << ")\n";
}

}  // namespace

int main()
{
  std::cout << "SIR epidemic on a 2-D toroidal lattice (Moore radius 1)\n";
  std::cout << "Glyphs: '.' susceptible  '*' infected  '#' recovered\n";

  // R0 ~ 0.05 * 8 / 0.5 = 0.8 < 1 ⇒ epidemic dies out.
  run_scenario("Subcritical (R0 < 1)",
               /*beta=*/0.05, /*gamma=*/0.5,
               /*seed=*/0x5EEDA1Cull, /*steps=*/120, /*every=*/30);

  // R0 ~ 0.20 * 8 / 0.5 = 3.2 > 1 ⇒ wavefront propagation.
  run_scenario("Supercritical (R0 > 1)",
               /*beta=*/0.20, /*gamma=*/0.5,
               /*seed=*/0x5EEDA1Cull, /*steps=*/40, /*every=*/8);

  std::cout << "\nNote: master_seed is identical across runs, so the\n"
               "      trace is bit-for-bit reproducible regardless of\n"
               "      the build mode and the number of worker threads.\n";
  return 0;
}