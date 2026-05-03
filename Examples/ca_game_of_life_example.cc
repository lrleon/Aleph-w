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
 * @file ca_game_of_life_example.cc
 * @brief Conway's Game of Life over a toroidal 2D lattice.
 *
 * Builds a Phase 3 `Synchronous_Engine` through `make_gol_engine()`,
 * stamps one of the canonical Life patterns into the initial frame and
 * renders each synchronous step in-place on stdout.
 *
 * Usage:
 *   ca_game_of_life_example [pattern] [steps] [width] [height]
 *
 * Available patterns:
 *   block        — 2x2 still life
 *   blinker      — period-2 oscillator
 *   toad         — period-2 oscillator
 *   beacon       — period-2 oscillator
 *   glider       — diagonal spaceship (default)
 *   r-pentomino  — methuselah; long-lived chaotic activity
 *   gosper       — Gosper glider gun
 *
 * Defaults: glider, 40 steps, 32x16 toroidal grid.
 */

# include <chrono>
# include <cerrno>
# include <cstdlib>
# include <iostream>
# include <limits>
# include <string>
# include <thread>
# include <utility>
# include <vector>

# include <ca-traits.H>
# include <tpl_ca_storage.H>
# include <tpl_ca_lattice.H>
# include <ca-engine-utils.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace
{
  using Grid = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;
  constexpr std::size_t Max_Steps = 100000;
  constexpr ca_size_t Max_Dimension = 4096;

  bool parse_positive_size(const char * text, const char * name,
                           std::size_t max_value, std::size_t & out)
  {
    if (text == nullptr or text[0] == '-')
      {
        std::cerr << "Invalid " << name << ": '"
                  << (text == nullptr ? "" : text)
                  << "' (expected a positive integer)\n";
        return false;
      }
    errno = 0;
    char * end = nullptr;
    const unsigned long long value = std::strtoull(text, &end, 10);
    if (errno == ERANGE or end == text or *end != '\0' or value == 0 or
        value > std::numeric_limits<std::size_t>::max() or
        value > max_value)
      {
        std::cerr << "Invalid " << name << ": '" << text
                  << "' (expected an integer in [1, "
                  << max_value << "])\n";
        return false;
      }
    out = static_cast<std::size_t>(value);
    return true;
  }

  bool parse_positive_ca_size(const char * text, const char * name,
                              ca_size_t & out)
  {
    std::size_t parsed = 0;
    if (not parse_positive_size(text, name, Max_Dimension, parsed))
      return false;
    const auto ca_max = std::numeric_limits<ca_size_t>::max();
    const auto idx_max = static_cast<std::size_t>(
      std::numeric_limits<ca_index_t>::max());
    if (parsed > ca_max or parsed > idx_max)
      {
        std::cerr << "Invalid " << name << ": '" << text
                  << "' (value is too large)\n";
        return false;
      }
    out = static_cast<ca_size_t>(parsed);
    return true;
  }

  /// Render a frame to stdout, using a top/bottom border for clarity.
  void render(const Grid & g, std::size_t step, std::size_t alive)
  {
    const auto rows = static_cast<ca_index_t>(g.size(0));
    const auto cols = static_cast<ca_index_t>(g.size(1));

    std::cout << "\033[H";  // move cursor home; the caller has cleared the screen
    std::cout << "Game of Life — step " << step
              << " (" << alive << " alive)\n";
    std::cout << '+' << std::string(cols, '-') << "+\n";
    for (ca_index_t i = 0; i < rows; ++i)
      {
        std::cout << '|';
        for (ca_index_t j = 0; j < cols; ++j)
          std::cout << (g.at({ i, j }) ? '#' : ' ');
        std::cout << "|\n";
      }
    std::cout << '+' << std::string(cols, '-') << "+\n";
    std::cout.flush();
  }

  /// Count alive cells without mutating the grid.
  std::size_t alive_count(const Grid & g)
  {
    const auto rows = static_cast<ca_index_t>(g.size(0));
    const auto cols = static_cast<ca_index_t>(g.size(1));
    std::size_t a = 0;
    for (ca_index_t i = 0; i < rows; ++i)
      for (ca_index_t j = 0; j < cols; ++j)
        if (g.at({ i, j })) ++a;
    return a;
  }

  /// Stamp a list of (row, col) cells alive into `g`, centred at
  /// `(cr, cc)`. Coordinates outside the grid are clamped via the
  /// toroidal wrap of the lattice.
  void stamp(Grid & g, ca_index_t cr, ca_index_t cc,
             const std::vector<std::pair<int, int>> & pattern)
  {
    const auto rows = static_cast<ca_index_t>(g.size(0));
    const auto cols = static_cast<ca_index_t>(g.size(1));
    if (rows == 0 or cols == 0)
      return;
    for (auto [r, c] : pattern)
      {
        ca_index_t i = ((cr + r) % rows + rows) % rows;
        ca_index_t j = ((cc + c) % cols + cols) % cols;
        g.set({ i, j }, 1);
      }
  }

  /// Patterns expressed as offsets from the stamp centre.
  std::vector<std::pair<int, int>> pattern_for(const std::string & name)
  {
    if (name == "block")
      return { {0,0}, {0,1}, {1,0}, {1,1} };

    if (name == "blinker")
      return { {0,-1}, {0,0}, {0,1} };

    if (name == "toad")
      return { {0,1}, {0,2}, {0,3},
               {1,0}, {1,1}, {1,2} };

    if (name == "beacon")
      return { {0,0}, {0,1}, {1,0}, {1,1},
               {2,2}, {2,3}, {3,2}, {3,3} };

    if (name == "r-pentomino")
      return { {0,1}, {0,2}, {1,0}, {1,1}, {2,1} };

    if (name == "gosper")
      return {
        // Left block
        {4,0}, {4,1}, {5,0}, {5,1},
        // Left ship
        {2,12}, {2,13}, {3,11}, {3,15}, {4,10}, {4,16}, {5,10}, {5,14},
        {5,16}, {5,17}, {6,10}, {6,16}, {7,11}, {7,15}, {8,12}, {8,13},
        // Right ship
        {0,24}, {1,22}, {1,24}, {2,20}, {2,21}, {3,20}, {3,21},
        {4,20}, {4,21}, {5,22}, {5,24}, {6,24},
        // Right block
        {2,34}, {2,35}, {3,34}, {3,35}
      };

    // Default
    return { {0,1}, {1,2}, {2,0}, {2,1}, {2,2} };  // glider
  }
}

int main(int argc, char ** argv)
{
  std::string pattern = (argc >= 2) ? argv[1] : "glider";
  std::size_t steps   = 40;
  ca_size_t   width   = 32;
  ca_size_t   height  = 16;

  if (argc >= 3 and not parse_positive_size(argv[2], "steps",
                                            Max_Steps, steps))
    return 1;
  if (argc >= 4 and not parse_positive_ca_size(argv[3], "width", width))
    return 1;
  if (argc >= 5 and not parse_positive_ca_size(argv[4], "height", height))
    return 1;

  // Gosper's gun is wider than the default grid; auto-grow if needed.
  if (pattern == "gosper")
    {
      if (width  < 50) width  = 50;
      if (height < 20) height = 20;
    }

  Grid grid({ height, width }, 0);
  stamp(grid, static_cast<ca_index_t>(height / 2),
        static_cast<ca_index_t>(width / 2),
        pattern_for(pattern));

  auto engine = make_gol_engine(std::move(grid));

  std::cout << "\033[2J";  // clear screen once; render() repaints in place
  for (std::size_t t = 0; t <= steps; ++t)
    {
      const Grid & frame = engine.frame();
      render(frame, engine.steps_run(), alive_count(frame));
      if (t == steps) break;
      std::this_thread::sleep_for(std::chrono::milliseconds(80));
      engine.step();
    }

  return 0;
}
