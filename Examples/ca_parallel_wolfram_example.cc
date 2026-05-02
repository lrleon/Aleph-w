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
 * @file ca_parallel_wolfram_example.cc
 * @brief Phase-5 illustration: parallel 1D Wolfram rule 30 / 110.
 *
 * Runs two of the most famous elementary CAs (rule 30, the canonical
 * "computational chaos" rule, and rule 110, Cook-proven Turing
 * complete) on a wide 1D lattice using the parallel synchronous
 * engine. Prints the first 32 generations as ASCII art plus a side
 * benchmark comparing the parallel engine to the sequential one.
 */

#include <array>
#include <cstdint>
#include <cstdio>

#include <ca-bench.H>
#include <ca-traits.H>
#include <tpl_ca_engine.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_parallel_engine.H>
#include <tpl_ca_rule.H>
#include <tpl_ca_storage.H>

using namespace Aleph;
using namespace Aleph::CA;

using Lat_t = Lattice<Dense_Cell_Storage<int, 1>, OpenBoundary>;

namespace {

void print_row(const Lat_t &lat)
{
  for (std::size_t i = 0; i < lat.size(0); ++i)
    std::putchar(lat.at({static_cast<ca_index_t>(i)}) != 0 ? '#' : '.');
  std::putchar('\n');
}

bool rows_equal(const Lat_t &a, const Lat_t &b)
{
  if (a.extents() != b.extents())
    return false;
  for (std::size_t i = 0; i < a.size(0); ++i)
    if (a.at({static_cast<ca_index_t>(i)}) != b.at({static_cast<ca_index_t>(i)}))
      return false;
  return true;
}

void render(std::uint8_t rule_no, std::size_t width, std::size_t generations)
{
  Custom_Neighborhood<1, 2> nh(
    std::array<Offset_Vec<1>, 2>{Offset_Vec<1>{-1}, Offset_Vec<1>{1}});
  Lat_t seed({width}, 0);
  seed.set({static_cast<ca_index_t>(width / 2)}, 1);

  Parallel_Engine_Config cfg;
  cfg.num_partitions = 4;
  cfg.min_parallel_cells = 0;
  Parallel_Synchronous_Engine<Lat_t, Lookup_Rule<2, 2>, Custom_Neighborhood<1, 2>> par(
    seed, make_wolfram_elementary_rule(rule_no), nh, cfg);

  std::printf("\n--- Wolfram rule %u, %zu cells, %zu generations ---\n",
              static_cast<unsigned>(rule_no), width, generations);
  print_row(par.frame());
  for (std::size_t gen = 1; gen < generations; ++gen)
    {
      par.step();
      print_row(par.frame());
    }
}

void bench(std::uint8_t rule_no, std::size_t width, std::size_t steps)
{
  Custom_Neighborhood<1, 2> nh(
    std::array<Offset_Vec<1>, 2>{Offset_Vec<1>{-1}, Offset_Vec<1>{1}});
  Lat_t seed({width}, 0);
  seed.set({static_cast<ca_index_t>(width / 2)}, 1);

  Synchronous_Engine<Lat_t, Lookup_Rule<2, 2>, Custom_Neighborhood<1, 2>> seq(
    seed, make_wolfram_elementary_rule(rule_no), nh);

  Parallel_Engine_Config cfg;
  cfg.num_partitions = 8;
  cfg.min_parallel_cells = 0;
  Parallel_Synchronous_Engine<Lat_t, Lookup_Rule<2, 2>, Custom_Neighborhood<1, 2>> par(
    seed, make_wolfram_elementary_rule(rule_no), nh, cfg);

  const double t_seq = bench_seconds([&] { seq.run(steps); });
  const double t_par = bench_seconds([&] { par.run(steps); });

  std::printf("\nrule=%-3u N=%-7zu steps=%-6zu sequential=%.4fs parallel=%.4fs "
              "speedup=%.2fx (parallel=%s)\n",
              static_cast<unsigned>(rule_no), width, steps, t_seq, t_par,
              t_seq / t_par,
              format_throughput(static_cast<double>(width) * steps / t_par));
  if (not rows_equal(seq.frame(), par.frame()))
    {
      std::printf("*** divergence between sequential and parallel rule %u ***\n",
                  static_cast<unsigned>(rule_no));
      std::exit(1);
    }
}

}  // namespace

int main()
{
  std::printf("Aleph::CA Phase-5 example: parallel 1D Wolfram CAs\n");

  // Visual ASCII renders so the reader can verify the canonical
  // patterns (rule 30 noise, rule 110 sliding triangles).
  render(30, 79, 32);
  render(110, 79, 32);

  // Microbench on a wide lattice. Numbers vary by hardware; the
  // example mostly exercises the scheduler and confirms equivalence.
  bench(30, 1u << 17, 200);
  bench(110, 1u << 17, 200);

  std::printf("\nParallel Wolfram rules 30 and 110 matched the sequential reference.\n");
  return 0;
}