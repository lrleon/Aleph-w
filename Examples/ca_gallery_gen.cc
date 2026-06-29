/*
                          Aleph_w

  Data structures & Algorithms
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
 * @file ca_gallery_gen.cc
 * @brief Phase 28 — render one PNG per canonical built-in CA rule.
 *
 * Produces the visual gallery under `docs/gallery/`. Two families are
 * covered, which together span the bulk of the built-in catalogue:
 *
 *   - **Life-like outer-totalistic rules** (Conway, HighLife, Day & Night,
 *     Seeds, Maze, 2x2, Replicator, Life-without-Death) rendered as a 2D
 *     snapshot after a fixed number of steps from a deterministic soup.
 *   - **Elementary Wolfram rules** (30, 54, 60, 90, 110, 150, 184, 250)
 *     rendered as space-time diagrams (row = time) from a single impulse.
 *
 * Stochastic / continuous rules (Forest Fire, Gray-Scott, Ising, SIR,
 * Schelling) are demonstrated quantitatively in `reproductions/` instead.
 *
 * Usage:
 *   ca_gallery_gen [output-dir]      (default: docs/gallery)
 *
 * Deterministic: same output bytes on every run (fixed seeds), so the
 * committed PNGs are reproducible.
 */

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>

#include <ca-io.H>
#include <ca-png.H>
#include <ca-engine-utils.H>
#include <tpl_ca_engine.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_rule.H>
#include <tpl_ca_storage.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace fs = std::filesystem;

namespace
{

using Grid2D = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;

// Write `frame` (rank-2) to `path` as PNG using `mapper`.
template <typename Lattice, typename Mapper>
void write_png_file(const fs::path &path, const Lattice &frame, Mapper mapper)
{
  std::ofstream out(path, std::ios::binary);
  write_png(out, frame, mapper);
  if (not out)
    std::cerr << "warning: failed writing " << path << '\n';
}

// Deterministic Bernoulli soup keyed by the rule name so each panel is
// stable but visually distinct.
Grid2D soup(ca_size_t side, double density, const std::string &key)
{
  std::mt19937 rng(0xA1E90000u ^ static_cast<std::uint32_t>(
                                     std::hash<std::string>{}(key)));
  std::uniform_real_distribution<double> u(0.0, 1.0);
  Grid2D g({side, side}, 0);
  for (ca_size_t i = 0; i < side; ++i)
    for (ca_size_t j = 0; j < side; ++j)
      g.set({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)},
            u(rng) < density ? 1 : 0);
  return g;
}

// Render a Life-like outer-totalistic rule `F(state, alive_count) -> state`.
template <typename F>
void render_life_like(const fs::path &dir, const std::string &name, F func,
                      ca_size_t side, std::size_t steps, double density,
                      RGB8 on)
{
  using Rule = Outer_Totalistic_Rule<F>;
  Synchronous_Engine<Grid2D, Rule, Moore<2, 1>> eng(
      soup(side, density, name), Rule(func), Moore<2, 1>{});
  eng.run(steps);
  const RGB8 off{18, 18, 24};
  write_png_file(dir / (name + ".png"), eng.frame(),
                 [on, off](int v) { return v != 0 ? on : off; });
  std::cout << "  " << name << ".png (" << side << "x" << side << ", "
            << steps << " steps)\n";
}

// Render an elementary Wolfram rule as a space-time diagram.
void render_wolfram(const fs::path &dir, int rule_no, ca_size_t width,
                    ca_size_t steps, RGB8 on)
{
  Lattice<Dense_Cell_Storage<int, 1>, OpenBoundary> row({width}, 0);
  row.set({static_cast<ca_index_t>(width / 2)}, 1);
  auto eng = make_wolfram_engine(static_cast<std::uint8_t>(rule_no),
                                 std::move(row));

  Grid2D img({steps, width}, 0);
  for (ca_size_t t = 0; t < steps; ++t)
    {
      for (ca_size_t x = 0; x < width; ++x)
        img.set({static_cast<ca_index_t>(t), static_cast<ca_index_t>(x)},
                eng.frame().at({static_cast<ca_index_t>(x)}));
      eng.step();
    }

  const RGB8 off{248, 248, 252};
  const std::string name = "wolfram_" + std::to_string(rule_no);
  write_png_file(dir / (name + ".png"), img,
                 [on, off](int v) { return v != 0 ? on : off; });
  std::cout << "  " << name << ".png (" << width << "x" << steps
            << " space-time)\n";
}

}  // namespace

int main(int argc, char *argv[])
{
  const fs::path out_dir = argc >= 2 ? fs::path(argv[1]) : fs::path("docs/gallery");
  fs::create_directories(out_dir);

  std::cout << "Rendering gallery into " << out_dir << "\n";

  // --- Life-like outer-totalistic rules (Moore-8 neighbour count) ---------
  std::cout << "Life-like rules:\n";
  // Conway B3/S23.
  render_life_like(out_dir, "conway_b3s23",
      [](int s, std::size_t n) { return (n == 3 or (s != 0 and n == 2)) ? 1 : 0; },
      96, 80, 0.30, RGB8{120, 220, 140});
  // HighLife B36/S23 (replicator-bearing).
  render_life_like(out_dir, "highlife_b36s23",
      [](int s, std::size_t n)
      { return ((n == 3 or n == 6) or (s != 0 and n == 2) or (s != 0 and n == 3)) ? 1 : 0; },
      96, 80, 0.30, RGB8{240, 180, 90});
  // Day & Night B3678/S34678 (symmetric).
  render_life_like(out_dir, "day_and_night_b3678s34678",
      [](int s, std::size_t n)
      {
        const bool birth = (n == 3 or n == 6 or n == 7 or n == 8);
        const bool survive = (n == 3 or n == 4 or n == 6 or n == 7 or n == 8);
        return ((s == 0 and birth) or (s != 0 and survive)) ? 1 : 0;
      },
      96, 64, 0.45, RGB8{150, 180, 250});
  // Seeds B2/S (everything dies, explosive growth).
  render_life_like(out_dir, "seeds_b2s",
      [](int s, std::size_t n) { return (s == 0 and n == 2) ? 1 : 0; },
      96, 24, 0.06, RGB8{250, 120, 120});
  // Maze B3/S12345.
  render_life_like(out_dir, "maze_b3s12345",
      [](int s, std::size_t n)
      {
        const bool survive = (n >= 1 and n <= 5);
        return ((s == 0 and n == 3) or (s != 0 and survive)) ? 1 : 0;
      },
      96, 90, 0.18, RGB8{200, 160, 250});
  // 2x2 B36/S125.
  render_life_like(out_dir, "two_by_two_b36s125",
      [](int s, std::size_t n)
      {
        const bool birth = (n == 3 or n == 6);
        const bool survive = (n == 1 or n == 2 or n == 5);
        return ((s == 0 and birth) or (s != 0 and survive)) ? 1 : 0;
      },
      96, 80, 0.30, RGB8{120, 210, 230});
  // Replicator B1357/S1357.
  render_life_like(out_dir, "replicator_b1357s1357",
      [](int, std::size_t n) { return (n % 2 == 1) ? 1 : 0; },
      96, 40, 0.004, RGB8{240, 210, 120});
  // Life without Death B3/S012345678.
  render_life_like(out_dir, "life_without_death_b3s8",
      [](int s, std::size_t n) { return ((s == 0 and n == 3) or s != 0) ? 1 : 0; },
      96, 50, 0.06, RGB8{170, 230, 160});

  // --- Elementary Wolfram rules (space-time diagrams) ---------------------
  std::cout << "Wolfram elementary rules:\n";
  const RGB8 ink{30, 30, 40};
  render_wolfram(out_dir, 30, 201, 100, ink);
  render_wolfram(out_dir, 54, 201, 100, ink);
  render_wolfram(out_dir, 60, 201, 100, ink);
  render_wolfram(out_dir, 90, 201, 100, ink);
  render_wolfram(out_dir, 110, 201, 100, ink);
  render_wolfram(out_dir, 150, 201, 100, ink);
  render_wolfram(out_dir, 184, 201, 100, ink);
  render_wolfram(out_dir, 250, 201, 100, ink);

  std::cout << "Done.\n";
  return 0;
}