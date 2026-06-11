/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file ca_live_x11_viewer_example.cc
 * @brief Live X11 viewer example for Conway's Game of Life.
 *
 * Usage:
 *
 *   ca_live_x11_viewer_example [steps]
 *
 * The viewer is a no-op when DISPLAY is not set, so the example remains
 * safe to compile and run in headless CI.
 */

#include <cstdlib>
#include <iostream>
#include <limits>
#include <utility>
#include <vector>

#include <ca-engine-utils.H>
#include <ca-x11-viewer.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_storage.H>

using namespace Aleph::CA;

namespace
{
using Grid = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;

std::size_t parse_steps(const char *text, const std::size_t fallback)
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

void stamp(Grid &grid,
           const ca_index_t row,
           const ca_index_t col,
           const std::vector<std::pair<int, int>> &cells)
{
  const ca_index_t rows = static_cast<ca_index_t>(grid.size(0));
  const ca_index_t cols = static_cast<ca_index_t>(grid.size(1));
  for (const auto &[dr, dc] : cells)
    grid.set({(row + dr + rows) % rows, (col + dc + cols) % cols}, 1);
}
}

int main(int argc, char **argv)
{
  const std::size_t steps = argc >= 2 ? parse_steps(argv[1], 400) : 400;

  Grid grid({48, 80}, 0);
  stamp(grid, 20, 30, {{0, 1}, {1, 2}, {2, 0}, {2, 1}, {2, 2}});
  stamp(grid, 10, 12, {{0, -1}, {0, 0}, {0, 1}});

  auto engine = make_gol_engine(std::move(grid));
  X11_Viewer_Options opts;
  opts.cell_size = 10;
  opts.delay_ms = 30;
  opts.title = "Aleph::CA Game of Life";

  X11_Frame_Sink viewer(Binary_RGB_Mapper<int>{0, {245, 247, 250}, {0, 40, 55}}, opts);
  for (std::size_t i = 0; i <= steps; ++i)
    {
      viewer.accept(engine.steps_run(), engine.frame());
      if (i != steps)
        engine.step();
    }
  viewer.flush();

  std::cout << "Ran " << steps << " Game-of-Life steps";
  if (not viewer.active())
    std::cout << " (viewer inactive; DISPLAY was probably unavailable)";
  std::cout << '\n';
  return 0;
}
