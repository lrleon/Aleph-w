/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file ca_sandpile_example.cc
 * @brief Phase 13 example: Bak–Tang–Wiesenfeld self-organised criticality.
 *
 * The classical sandpile is the textbook example of a CA whose
 * dynamics need a *sequential* update — every visit to a cell
 * relaxes it once, redistributing grains to its 4-neighbour cross.
 * The example seeds the centre of a 16×16 grid with grains, then
 * triggers an avalanche by dropping grains one at a time. After
 * each drop we measure the avalanche size (number of cells that
 * relaxed before the system stabilised); the empirical avalanche
 * size distribution famously follows a power law, the hallmark of
 * self-organised criticality (Bak, Tang & Wiesenfeld, 1987).
 *
 * Run:
 *   ./ca_sandpile_example [drops] [side]
 */

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

#include <ca-traits.H>
#include <tpl_ca_async_engine.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_storage.H>
#include <tpl_ca_update_scheme.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace
{

using Grid = Lattice<Dense_Cell_Storage<int, 2>, OpenBoundary>;

/// Bak-Tang-Wiesenfeld toppling rule. With Sequential_Update each
/// cell relaxes once per sweep: cells at or above 4 lose 4 grains
/// and donate 1 to each over-threshold neighbour.
struct Sandpile_Rule
{
  template <typename State>
  State operator()(const State &s, Neighbor_View<State> nh) const noexcept
  {
    constexpr State threshold = 4;
    State result = s;
    if (result >= threshold)
      result -= threshold;
    for (const auto &n : nh)
      if (n >= threshold)
        result += 1;
    return result;
  }
};

/// Render a frame using a two-character map from heights to glyphs.
void render(const Grid &g, std::ostream &os)
{
  static const std::array<const char *, 5> glyphs = {" .", " :", " +", " #", "##"};
  for (ca_size_t i = 0; i < g.size(0); ++i)
    {
      for (ca_size_t j = 0; j < g.size(1); ++j)
        {
          const auto v = g.at({static_cast<ca_index_t>(i),
                               static_cast<ca_index_t>(j)});
          const std::size_t idx = v < 0 ? 0 : (v > 4 ? 4 : static_cast<std::size_t>(v));
          os << glyphs[idx];
        }
      os << '\n';
    }
}

/// True iff every cell is below the toppling threshold.
bool stabilised(const Grid &g)
{
  for (ca_size_t i = 0; i < g.size(0); ++i)
    for (ca_size_t j = 0; j < g.size(1); ++j)
      if (g.at({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)}) >= 4)
        return false;
  return true;
}

/// Number of cells that lost grains compared to the previous frame.
std::size_t cells_changed(const Grid &before, const Grid &after)
{
  std::size_t n = 0;
  for (ca_size_t i = 0; i < before.size(0); ++i)
    for (ca_size_t j = 0; j < before.size(1); ++j)
      {
        const Coord_Vec<2> c{static_cast<ca_index_t>(i),
                             static_cast<ca_index_t>(j)};
        if (before.at(c) != after.at(c))
          ++n;
      }
  return n;
}

}  // namespace

int main(int argc, char **argv)
{
  std::size_t drops = 80;
  ca_size_t side = 16;
  try
    {
      if (argc >= 2)
        drops = static_cast<std::size_t>(std::stoul(argv[1]));
      if (argc >= 3)
        side = static_cast<ca_size_t>(std::stoul(argv[2]));
    }
  catch (const std::invalid_argument &)
    {
      std::cerr << "Invalid argument: expected non-negative integers for [drops] [side]\n";
      return 1;
    }
  catch (const std::out_of_range &)
    {
      std::cerr << "Argument out of range for [drops] [side]\n";
      return 1;
    }
  if (side == 0)
    {
      std::cerr << "Invalid side: must be a positive integer\n";
      return 1;
    }

  Grid grid({side, side}, 0);
  using Engine = Async_Engine<Grid, Sandpile_Rule, Von_Neumann<2, 1>,
                              Sequential_Update<RowMajor>>;

  std::cout << "BTW Sandpile — " << side << "×" << side << ", " << drops
            << " drops at the centre\n\n";

  std::size_t total_avalanche_cells = 0;
  for (std::size_t k = 0; k < drops; ++k)
    {
      // Drop a grain at the centre.
      const auto cr = static_cast<ca_index_t>(side / 2);
      const auto cc = static_cast<ca_index_t>(side / 2);
      grid.set({cr, cc}, grid.at({cr, cc}) + 1);

      // Relax until stable using the Sequential_Update strategy
      // (single-buffer, in-place sweep).
      Engine engine(std::move(grid), Sandpile_Rule{}, Von_Neumann<2, 1>{});
      Grid pre = engine.frame();
      std::size_t sweeps = 0;
      while (not stabilised(engine.frame()) and sweeps < 4 * side * side)
        {
          engine.step();
          ++sweeps;
        }
      const std::size_t aval = cells_changed(pre, engine.frame());
      total_avalanche_cells += aval;
      grid = engine.frame();

      if (k % (drops < 20 ? 1 : 10) == 0 or k + 1 == drops)
        std::cout << "drop " << k + 1 << "/" << drops
                  << " — avalanche size " << aval
                  << " (cumulative " << total_avalanche_cells
                  << ", " << sweeps << " sweeps)\n";
    }

  std::cout << "\nFinal frame:\n\n";
  render(grid, std::cout);
  std::cout << "\nAverage avalanche size: "
            << static_cast<double>(total_avalanche_cells) / static_cast<double>(drops)
            << "\n";
  return 0;
}