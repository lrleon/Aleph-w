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
 * @file ca_hashlife_example.cc
 * @brief A guided tour of `Aleph::CA::Hashlife_Engine` (Phase 10).
 *
 * The example walks through three classic Conway's Game of Life patterns
 * to illustrate what the Hashlife engine does, why it scales to enormous
 * generation counts, and how the user-facing API is meant to be used:
 *
 *   1. **Glider** — the canonical 5-cell traveler. Drawn frame-by-frame
 *      so you can watch it move.
 *   2. **R-pentomino** — a tiny chaotic seed that stabilises around step
 *      1103 with 116 cells. We show the population trajectory.
 *   3. **Gosper glider gun** — the first known finite, infinitely-growing
 *      pattern. We chain exponential advances to step 2^20 and report the
 *      population, bounding box and cache stats — work that would take
 *      the dense synchronous engine many minutes.
 *
 * Run:
 *   ./build/Examples/ca_hashlife_example
 *
 * No command-line arguments. No external assets — patterns are coded as
 * cell sets so the example is self-contained.
 */

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include <tpl_ca_hashlife.H>

using namespace Aleph::CA;

namespace
{
  // ------------------------------------------------------------------
  // Pretty-printing helpers
  // ------------------------------------------------------------------

  void print_section(const std::string &title)
  {
    std::cout << '\n';
    std::cout << "═══════════════════════════════════════════════════════════════\n";
    std::cout << " " << title << '\n';
    std::cout << "═══════════════════════════════════════════════════════════════\n";
  }

  /// Render the engine's alive cells inside the rectangle
  /// `[x_min, x_max) × [y_min, y_max)` as ASCII art.
  void render_window(const Hashlife_Engine &engine,
                     std::int64_t x_min, std::int64_t y_min,
                     std::int64_t x_max, std::int64_t y_max,
                     const std::string &caption)
  {
    const std::int64_t w = x_max - x_min;
    const std::int64_t h = y_max - y_min;
    std::vector<std::string> rows(static_cast<std::size_t>(h),
                                  std::string(static_cast<std::size_t>(w), '.'));
    engine.for_each_alive([&](std::int64_t x, std::int64_t y)
    {
      if (x >= x_min and x < x_max and y >= y_min and y < y_max)
        rows[static_cast<std::size_t>(y - y_min)]
            [static_cast<std::size_t>(x - x_min)] = 'O';
    });
    std::cout << "  ┌";
    for (std::int64_t i = 0; i < w; ++i) std::cout << "─";
    std::cout << "┐  " << caption << '\n';
    for (const auto &r : rows)
      std::cout << "  │" << r << "│\n";
    std::cout << "  └";
    for (std::int64_t i = 0; i < w; ++i) std::cout << "─";
    std::cout << "┘\n";
  }

  // ------------------------------------------------------------------
  // Canonical patterns expressed in local (top-left = 0,0) coordinates.
  // ------------------------------------------------------------------

  void seed_glider(Hashlife_Engine &e, std::int64_t ox = 0, std::int64_t oy = 0)
  {
    e.set_alive(ox + 1, oy + 0);
    e.set_alive(ox + 2, oy + 1);
    e.set_alive(ox + 0, oy + 2);
    e.set_alive(ox + 1, oy + 2);
    e.set_alive(ox + 2, oy + 2);
  }

  void seed_r_pentomino(Hashlife_Engine &e, std::int64_t ox = 0, std::int64_t oy = 0)
  {
    e.set_alive(ox + 1, oy + 0);
    e.set_alive(ox + 2, oy + 0);
    e.set_alive(ox + 0, oy + 1);
    e.set_alive(ox + 1, oy + 1);
    e.set_alive(ox + 1, oy + 2);
  }

  /// Standard Gosper glider gun (B3/S23) — places the canonical 36-cell
  /// configuration with its top-left at `(ox, oy)`.
  void seed_gosper_gun(Hashlife_Engine &e,
                       std::int64_t ox = 0, std::int64_t oy = 0)
  {
    static constexpr int xy[][2] = {
      { 1, 5 }, { 1, 6 }, { 2, 5 }, { 2, 6 },
      { 11, 5 }, { 11, 6 }, { 11, 7 }, { 12, 4 }, { 12, 8 },
      { 13, 3 }, { 13, 9 }, { 14, 3 }, { 14, 9 },
      { 15, 6 }, { 16, 4 }, { 16, 8 },
      { 17, 5 }, { 17, 6 }, { 17, 7 }, { 18, 6 },
      { 21, 3 }, { 21, 4 }, { 21, 5 },
      { 22, 3 }, { 22, 4 }, { 22, 5 },
      { 23, 2 }, { 23, 6 },
      { 25, 1 }, { 25, 2 }, { 25, 6 }, { 25, 7 },
      { 35, 3 }, { 35, 4 }, { 36, 3 }, { 36, 4 },
    };
    for (const auto &p : xy)
      e.set_alive(ox + p[0], oy + p[1]);
  }

  // ------------------------------------------------------------------
  // Demo 1 — watch a glider travel diagonally.
  // ------------------------------------------------------------------

  void demo_glider()
  {
    print_section("Demo 1 — Glider: 5 cells, period 4, moves (+1,+1) per period");

    Hashlife_Engine e;
    seed_glider(e);

    // Show 5 frames spaced by 4 generations: glider returns to the same
    // shape but shifted one cell south-east each time.
    for (int frame = 0; frame < 5; ++frame)
      {
        const auto bb = e.bbox();
        std::cout << "  step " << std::setw(4) << e.generation()
                  << "  pop=" << e.population()
                  << "  bbox=(" << bb.x_min << ',' << bb.y_min
                  << ")…(" << bb.x_max << ',' << bb.y_max << ")\n";
        render_window(e, -1, -1, 8, 8, "");
        e.run(4);
      }
  }

  // ------------------------------------------------------------------
  // Demo 2 — R-pentomino: chaotic stabilisation around step 1103.
  // ------------------------------------------------------------------

  void demo_r_pentomino()
  {
    print_section("Demo 2 — R-pentomino: 5 chaotic cells stabilising at step 1103");

    Hashlife_Engine e;
    seed_r_pentomino(e);
    std::cout << "  Initial seed (5 cells):\n";
    render_window(e, -1, -1, 8, 8, "step 0");

    // Hashlife always advances by powers of two, so the actual generation
    // count usually overshoots the requested milestone. We poll instead
    // of subtracting, which keeps the walker moving forward monotonically.
    constexpr std::uint64_t milestones[] = {
      10u, 100u, 500u, 1000u, 1103u, 2000u, 5000u
    };
    for (const std::uint64_t target : milestones)
      {
        const auto current = static_cast<std::uint64_t>(e.generation());
        if (target > current)
          e.run(target - current);
        const auto bb = e.bbox();
        std::cout << "  step " << std::setw(5) << e.generation()
                  << "  pop=" << std::setw(4) << e.population()
                  << "  bbox=" << std::setw(4) << bb.width()
                  << " x " << std::setw(4) << bb.height() << '\n';
      }
    std::cout << "  → the methuselah settles into still-lifes, blinkers and"
                 " 6 escaping gliders.\n";
  }

  // ------------------------------------------------------------------
  // Demo 3 — Gosper gun at exponential generation counts.
  // ------------------------------------------------------------------

  void demo_gosper_gun()
  {
    print_section("Demo 3 — Gosper gun: exponential advances up to step 2²⁰");

    Hashlife_Engine e;
    seed_gosper_gun(e, /*ox=*/0, /*oy=*/0);
    std::cout << "  Seed (36 cells, classic Gosper gun configuration):\n";
    render_window(e, -1, -1, 41, 13, "step 0");

    using clock = std::chrono::steady_clock;
    std::cout << "\n  Calling advance(k) for k = 4, 8, 12, 16, 20:\n";
    std::cout << "  k   | 2^k generations |       pop |    cache nodes |   wall time\n";
    std::cout << "  ----+-----------------+-----------+----------------+--------------\n";
    for (unsigned k : { 4u, 8u, 12u, 16u, 20u })
      {
        const auto t0 = clock::now();
        const std::uint64_t advanced = e.advance(k);
        const auto dt = std::chrono::duration<double, std::milli>(
                          clock::now() - t0).count();
        const auto stats = e.stats();
        std::cout << "  " << std::setw(2) << k
                  << "  | " << std::setw(15) << advanced
                  << " | " << std::setw(9) << e.population()
                  << " | " << std::setw(14) << stats.canonical_nodes
                  << " | " << std::fixed << std::setprecision(2)
                  << std::setw(8) << dt << " ms\n";
      }

    const auto bb = e.bbox();
    std::cout << "\n  Final state: " << e.population() << " alive cells across a "
              << bb.width() << " × " << bb.height() << " bounding box.\n";
    std::cout << "  Generation: " << e.generation() << '\n';

    const auto stats = e.stats();
    std::cout << "  Cache: " << stats.canonical_nodes << " canonical nodes, "
              << stats.result_hits << " result hits, "
              << stats.result_misses << " result misses, "
              << stats.result_cache_clears << " evictions.\n";
    std::cout << "  Hit ratio: " << std::fixed << std::setprecision(2)
              << (100.0 * stats.result_hits
                  / std::max<std::size_t>(1, stats.result_hits + stats.result_misses))
              << " %  (high hit ratio = lots of repeated subtree work avoided)\n";
  }

  // ------------------------------------------------------------------
  // Demo 4 — RLE round-trip through HighLife.
  // ------------------------------------------------------------------

  void demo_rle_round_trip()
  {
    print_section("Demo 4 — RLE I/O and a pinch of HighLife");

    Hashlife_Engine src(HighLife);
    seed_glider(src);

    const std::string serialised = src.save_rle_string("glider, evolved by HighLife");
    std::cout << "  RLE serialisation of the seed:\n";
    for (const char c : serialised)
      std::cout << ((c == '\n') ? std::string("\n  ") : std::string(1, c));
    std::cout << '\n';

    Hashlife_Engine dst;
    dst.load_rle_string(serialised);
    std::cout << "\n  After parsing, dst.rule = " << format_rule(dst.rule())
              << "  population = " << dst.population() << '\n';
  }

}  // namespace


int main()
{
  std::cout << "Aleph::CA::Hashlife_Engine — guided tour\n";
  std::cout << "Outer-totalistic binary cellular automata at exponential scale.\n";

  demo_glider();
  demo_r_pentomino();
  demo_gosper_gun();
  demo_rle_round_trip();

  std::cout << "\nDone.\n";
  return 0;
}