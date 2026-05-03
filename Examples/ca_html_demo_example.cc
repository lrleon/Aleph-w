/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file ca_html_demo_example.cc
 * @brief Self-contained HTML demo for Gray-Scott reaction diffusion.
 *
 * Usage:
 *
 *   ca_html_demo_example [output.html] [steps] [size]
 */

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

#include <ca-html-player.H>
#include <ca-kernels.H>
#include <ca-rng.H>
#include <tpl_ca_continuous_rules.H>
#include <tpl_ca_engine.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_storage.H>

using namespace Aleph::CA;

namespace
{
using Cell = Reaction_Diffusion_Cell<double>;
using Gray_Lattice = Lattice<Dense_Cell_Storage<Cell, 2>, NeumannBoundary>;

std::size_t parse_size(const char *text, const std::size_t fallback)
{
  if (text == nullptr)
    return fallback;
  char *end = nullptr;
  const unsigned long value = std::strtoul(text, &end, 10);
  if (end == text or *end != '\0' or value == 0
      or value > std::numeric_limits<std::size_t>::max())
    return fallback;
  return static_cast<std::size_t>(value);
}

double seeded_noise(const std::uint64_t master, const Coord_Vec<2> &coord)
{
  const std::uint64_t h = cell_seed<2>(master, 0, coord);
  return static_cast<double>(h >> 11) * (1.0 / 9007199254740992.0) - 0.5;
}

void seed_initial_frame(Gray_Lattice &lat)
{
  constexpr std::uint64_t seed = 0x5eed1234abcddcbaull;
  const ca_size_t rows = lat.size(0);
  const ca_size_t cols = lat.size(1);
  const ca_size_t side = std::max<ca_size_t>(8, std::min(rows, cols) / 4);
  const ca_size_t r0 = rows / 2 - side / 2;
  const ca_size_t c0 = cols / 2 - side / 2;

  for (ca_size_t r = r0; r < r0 + side; ++r)
    for (ca_size_t c = c0; c < c0 + side; ++c)
      {
        const Coord_Vec<2> coord{static_cast<ca_index_t>(r), static_cast<ca_index_t>(c)};
        const double z = 0.03 * seeded_noise(seed, coord);
        lat.set(coord, Cell{0.50 + z, 0.25 - z});
      }
}

std::uint8_t byte(const double value)
{
  const double x = std::clamp(value, 0.0, 1.0);
  return static_cast<std::uint8_t>(255.0 * x + 0.5);
}

struct Gray_Scott_Colour
{
  [[nodiscard]] RGB8 operator()(const Cell &cell) const noexcept
  {
    return RGB8{byte(2.3 * cell.v),
                byte(1.35 * (1.0 - cell.u)),
                byte(0.25 + 1.7 * (cell.u - cell.v))};
  }
};
}

int main(int argc, char **argv)
{
  const std::string output = argc >= 2 ? argv[1] : "gray_scott.html";
  const std::size_t steps = argc >= 3 ? parse_size(argv[2], 360) : 360;
  const ca_size_t n = static_cast<ca_size_t>(argc >= 4 ? parse_size(argv[3], 72) : 72);

  Gray_Lattice lat({n, n}, Cell{1.0, 0.0});
  seed_initial_frame(lat);

  Gray_Scott_Rule<double> rule(/*feed=*/0.0367,
                               /*kill=*/0.0649,
                               /*du=*/0.16,
                               /*dv=*/0.08);
  Synchronous_Engine<Gray_Lattice, Gray_Scott_Rule<double>, Moore<2, 1>>
    engine(lat, rule);

  HTML_Player_Options opts;
  opts.title = "Gray-Scott reaction diffusion";
  opts.fps = 18;
  opts.cell_size = n <= 80 ? 7 : 4;
  HTML_Player_Frame_Sink html(output, Gray_Scott_Colour{}, opts);

  const std::size_t stride = std::max<std::size_t>(1, steps / 48);
  for (std::size_t step = 0; step <= steps; ++step)
    {
      if (step % stride == 0)
        html.accept(engine.steps_run(), engine.frame());
      if (step != steps)
        engine.step();
    }
  html.flush();

  std::cout << "Wrote " << html.size() << " Gray-Scott frames to " << output << '\n';
  return 0;
}
