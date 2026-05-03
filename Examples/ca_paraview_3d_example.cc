/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file ca_paraview_3d_example.cc
 * @brief Simple 3D cellular automaton exported as VTK legacy files.
 *
 * Usage:
 *
 *   ca_paraview_3d_example [output-dir] [steps] [size]
 *
 * Open the generated `ca3d_*.vtk` files in ParaView as a time series.
 */

#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>

#include <ca-vtk.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_storage.H>

using namespace Aleph::CA;

namespace
{
using Grid3 = Lattice<Dense_Cell_Storage<int, 3>, OpenBoundary>;

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

void seed_sphere(Grid3 &grid)
{
  const double cz = 0.5 * static_cast<double>(grid.size(0) - 1);
  const double cy = 0.5 * static_cast<double>(grid.size(1) - 1);
  const double cx = 0.5 * static_cast<double>(grid.size(2) - 1);
  const double radius = 0.28 * static_cast<double>(
    std::min(grid.size(0), std::min(grid.size(1), grid.size(2))));

  for (ca_size_t z = 0; z < grid.size(0); ++z)
    for (ca_size_t y = 0; y < grid.size(1); ++y)
      for (ca_size_t x = 0; x < grid.size(2); ++x)
        {
          const double dz = static_cast<double>(z) - cz;
          const double dy = static_cast<double>(y) - cy;
          const double dx = static_cast<double>(x) - cx;
          if (std::sqrt(dx * dx + dy * dy + dz * dz) <= radius)
            grid.set({static_cast<ca_index_t>(z),
                      static_cast<ca_index_t>(y),
                      static_cast<ca_index_t>(x)}, 1);
        }
}

int alive_neighbours(const Grid3 &grid, const ca_index_t z, const ca_index_t y, const ca_index_t x)
{
  int count = 0;
  for (ca_index_t dz = -1; dz <= 1; ++dz)
    for (ca_index_t dy = -1; dy <= 1; ++dy)
      for (ca_index_t dx = -1; dx <= 1; ++dx)
        {
          if (dz == 0 and dy == 0 and dx == 0)
            continue;
          count += grid.at_safe({z + dz, y + dy, x + dx}) != 0 ? 1 : 0;
        }
  return count;
}

void evolve(Grid3 &cur, Grid3 &next)
{
  for (ca_size_t z = 0; z < cur.size(0); ++z)
    for (ca_size_t y = 0; y < cur.size(1); ++y)
      for (ca_size_t x = 0; x < cur.size(2); ++x)
        {
          const ca_index_t iz = static_cast<ca_index_t>(z);
          const ca_index_t iy = static_cast<ca_index_t>(y);
          const ca_index_t ix = static_cast<ca_index_t>(x);
          const int alive = alive_neighbours(cur, iz, iy, ix);
          const bool current = cur.at({iz, iy, ix}) != 0;
          const int value = (current and alive >= 5 and alive <= 10)
                            or (not current and alive == 6) ? 1 : 0;
          next.set({iz, iy, ix}, value);
        }
  cur.swap(next);
  next.fill(0);
}
}

int main(int argc, char **argv)
{
  const std::filesystem::path out_dir = argc >= 2 ? argv[1] : "ca3d_vtk_output";
  const std::size_t steps = argc >= 3 ? parse_size(argv[2], 16) : 16;
  const ca_size_t n = static_cast<ca_size_t>(argc >= 4 ? parse_size(argv[3], 32) : 32);
  std::filesystem::create_directories(out_dir);

  Grid3 cur({n, n, n}, 0);
  Grid3 next({n, n, n}, 0);
  seed_sphere(cur);

  VTK_Write_Options opts;
  opts.title = "Aleph::CA 3D growth";
  opts.scalar_name = "alive";

  for (std::size_t step = 0; step <= steps; ++step)
    {
      const auto path = out_dir / ("ca3d_" + std::to_string(step) + ".vtk");
      std::ofstream out(path);
      write_vtk_legacy(out, cur, opts);
      if (step != steps)
        evolve(cur, next);
    }

  std::cout << "Wrote " << (steps + 1) << " VTK frames to " << out_dir << '\n';
  std::cout << "Open ca3d_0.vtk in ParaView and enable file-series loading.\n";
  return 0;
}
