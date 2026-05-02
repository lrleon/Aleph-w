/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file ca_gray_scott_example.cc
 * @brief Phase 9 example: Gray-Scott reaction-diffusion with PPM export.
 *
 * Builds a continuous 2-D cellular automaton over a square Neumann
 * lattice and runs the Gray-Scott model with the common "mitosis-like"
 * preset:
 *
 *   F = 0.0367, K = 0.0649, Du = 0.16, Dv = 0.08, dt = 1
 *
 * The initial condition is `u=1, v=0` everywhere plus a deterministic
 * noisy square in the centre. The final `v` field is exported as a PPM
 * image. Defaults match the Phase 9 acceptance scenario:
 *
 *   ./ca_gray_scott_example [output.ppm] [steps] [size]
 *
 * Example:
 *
 *   ./ca_gray_scott_example gray_scott.ppm 5000 256
 */

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

#include <ah-errors.H>

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

using Cell = Reaction_Diffusion_Cell<double>;
using Gray_Lattice = Lattice<Dense_Cell_Storage<Cell, 2>, NeumannBoundary>;

double seeded_noise(const std::uint64_t master, const Coord_Vec<2> &coord)
{
  const std::uint64_t h = cell_seed<2>(master, 0, coord);
  const double unit = static_cast<double>(h >> 11)
                      * (1.0 / 9007199254740992.0);
  return unit - 0.5;
}

std::size_t parse_size_arg(const char *text, const std::size_t fallback)
{
  if (text == nullptr)
    return fallback;
  char *end = nullptr;
  const unsigned long value = std::strtoul(text, &end, 10);
  if (end == text or *end != '\0' or value == 0)
    return fallback;
  return static_cast<std::size_t>(value);
}

void seed_initial_frame(Gray_Lattice &lat, const std::uint64_t master)
{
  const ca_size_t rows = lat.size(0);
  const ca_size_t cols = lat.size(1);
  const ca_size_t side = std::max<ca_size_t>(8, std::min(rows, cols) / 5);
  const ca_size_t r0 = rows / 2 - side / 2;
  const ca_size_t c0 = cols / 2 - side / 2;

  for (ca_size_t i = r0; i < r0 + side; ++i)
    for (ca_size_t j = c0; j < c0 + side; ++j)
      {
        const Coord_Vec<2> c{static_cast<ca_index_t>(i),
                             static_cast<ca_index_t>(j)};
        const double z = 0.02 * seeded_noise(master, c);
        lat.set(c, Cell{0.50 + z, 0.25 - z});
      }
}

unsigned char to_byte(const double x)
{
  const double y = std::clamp(x, 0.0, 1.0);
  return static_cast<unsigned char>(255.0 * y + 0.5);
}

void write_ppm(const Gray_Lattice &lat, const std::string &path)
{
  std::ofstream out(path, std::ios::binary);
  ah_runtime_error_if(not out) << "could not open " << path << " for writing";

  out << "P6\n" << lat.size(1) << ' ' << lat.size(0) << "\n255\n";
  for (ca_size_t i = 0; i < lat.size(0); ++i)
    for (ca_size_t j = 0; j < lat.size(1); ++j)
      {
        const Cell c = lat.at({static_cast<ca_index_t>(i),
                               static_cast<ca_index_t>(j)});
        const unsigned char r = to_byte(2.2 * c.v);
        const unsigned char g = to_byte(1.2 * (1.0 - c.u));
        const unsigned char b = to_byte(1.0 - 1.8 * c.v);
        out.put(static_cast<char>(r));
        out.put(static_cast<char>(g));
        out.put(static_cast<char>(b));
      }
}

struct Stats
{
  double mean_v = 0.0;
  double max_v = 0.0;
  std::size_t peaks = 0;
};

Stats stats_for_v(const Gray_Lattice &lat)
{
  Stats stats;
  double sum = 0.0;
  for (ca_size_t i = 0; i < lat.size(0); ++i)
    for (ca_size_t j = 0; j < lat.size(1); ++j)
      {
        const Cell c = lat.at({static_cast<ca_index_t>(i),
                               static_cast<ca_index_t>(j)});
        sum += c.v;
        if (c.v > stats.max_v)
          stats.max_v = c.v;
        if (c.v > 0.10)
          ++stats.peaks;
      }
  stats.mean_v = sum / static_cast<double>(lat.size(0) * lat.size(1));
  return stats;
}

}  // namespace

int main(int argc, char **argv)
{
  const std::string output = argc > 1 ? argv[1] : "gray_scott.ppm";
  const std::size_t steps = argc > 2 ? parse_size_arg(argv[2], 5000) : 5000;
  const ca_size_t n = static_cast<ca_size_t>(argc > 3
                                             ? parse_size_arg(argv[3], 256)
                                             : 256);
  constexpr std::uint64_t master_seed = 0x5eed1234abcddcbaull;

  std::cout << "Gray-Scott reaction-diffusion example\n";
  std::cout << "  grid      : " << n << "x" << n << " Neumann boundary\n";
  std::cout << "  params    : F=0.0367 K=0.0649 Du=0.16 Dv=0.08 dt=1\n";
  std::cout << "  seed      : 0x" << std::hex << master_seed << std::dec << '\n';
  std::cout << "  steps     : " << steps << '\n';
  std::cout << "  output    : " << output << "\n\n";

  Gray_Lattice lat({n, n}, Cell{1.0, 0.0});
  seed_initial_frame(lat, master_seed);

  Gray_Scott_Rule<double> rule(/*F=*/0.0367,
                               /*K=*/0.0649,
                               /*Du=*/0.16,
                               /*Dv=*/0.08);
  Synchronous_Engine<Gray_Lattice, Gray_Scott_Rule<double>, Moore<2, 1>>
    engine(lat, rule);

  const std::size_t report_every = std::max<std::size_t>(1, steps / 10);
  for (std::size_t step = 0; step < steps; ++step)
    {
      engine.step();
      if ((step + 1) % report_every == 0 or step + 1 == steps)
        {
          const Stats stats = stats_for_v(engine.frame());
          std::cout << "  step " << std::setw(5) << (step + 1)
                    << "  mean(v)=" << std::fixed << std::setprecision(5)
                    << stats.mean_v
                    << "  max(v)=" << stats.max_v
                    << "  peaks(v>0.10)=" << stats.peaks << '\n';
        }
    }

  write_ppm(engine.frame(), output);
  std::cout << "\nWrote " << output << '\n';
  return 0;
}
