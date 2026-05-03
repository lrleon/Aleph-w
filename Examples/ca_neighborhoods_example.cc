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
 * @file ca_neighborhoods_example.cc
 * @brief Pretty-print every neighborhood in the Phase 2 catalogue.
 *
 * For each 2D neighborhood (Moore<2,1>, Moore<2,2>, Von_Neumann<2,1>,
 * Von_Neumann<2,2>, Hex_Neighborhood, Triangular_Neighborhood with both
 * parities, plus a Custom example) the example draws a compact ASCII
 * grid where:
 *
 *   `o` = the centre cell
 *   `#` = a neighbour reported by `for_each_offset`
 *   `.` = an unrelated cell (within the bounding box)
 *
 * It also prints the runtime values of `radius()` and `size()` so the
 * user can sanity-check the catalogue at a glance.
 */

# include <algorithm>
# include <iostream>
# include <string>
# include <vector>

# include <ca-traits.H>
# include <tpl_ca_neighborhood.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace
{
  /// Render a neighborhood by drawing its offsets relative to a centre.
  /// `center` is only used by neighborhoods whose offsets depend on
  /// the centre (e.g. `Triangular_Neighborhood`).
  template <typename Nbh>
  void render(const std::string & title, const Nbh & nh,
              Coord_Vec<2> center = { 0, 0 })
  {
    std::vector<Offset_Vec<2>> offs;
    nh.for_each_offset(center, [&](const auto & o) { offs.push_back(o); });

    int min_r = 0, max_r = 0, min_c = 0, max_c = 0;
    for (auto o : offs)
      {
        min_r = std::min<int>(min_r, static_cast<int>(o[0]));
        max_r = std::max<int>(max_r, static_cast<int>(o[0]));
        min_c = std::min<int>(min_c, static_cast<int>(o[1]));
        max_c = std::max<int>(max_c, static_cast<int>(o[1]));
      }

    const int rows = max_r - min_r + 1;
    const int cols = max_c - min_c + 1;
    std::vector<std::string> grid(rows, std::string(cols, '.'));
    grid[-min_r][-min_c] = 'o';
    for (auto o : offs)
      grid[o[0] - min_r][o[1] - min_c] = '#';

    std::cout << "\n=== " << title
              << "  (radius=" << nh.radius()
              << ", size="    << nh.size() << ") ===\n";
    for (const auto & line : grid)
      std::cout << "  " << line << '\n';
  }
}

int main()
{
  std::cout << "Aleph::CA — Phase 2 neighborhood catalogue\n";
  std::cout << "==========================================\n";

  render("Moore<2, 1>",       Moore<2, 1>{});
  render("Moore<2, 2>",       Moore<2, 2>{});
  render("Von_Neumann<2, 1>", Von_Neumann<2, 1>{});
  render("Von_Neumann<2, 2>", Von_Neumann<2, 2>{});
  render("Hex_Neighborhood",  Hex_Neighborhood{});

  // Triangular_Neighborhood has parity-dependent offsets — show both.
  render("Triangular_Neighborhood (even parity)",
         Triangular_Neighborhood{}, Coord_Vec<2>{ 0, 0 });
  render("Triangular_Neighborhood (odd parity)",
         Triangular_Neighborhood{}, Coord_Vec<2>{ 0, 1 });

  // Knight-move custom neighborhood.
  Custom_Neighborhood<2, 8> knight({{
    Offset_Vec<2>{  1,  2 }, Offset_Vec<2>{  2,  1 },
    Offset_Vec<2>{  2, -1 }, Offset_Vec<2>{  1, -2 },
    Offset_Vec<2>{ -1, -2 }, Offset_Vec<2>{ -2, -1 },
    Offset_Vec<2>{ -2,  1 }, Offset_Vec<2>{ -1,  2 } }});
  render("Custom_Neighborhood (chess knight)", knight);

  return 0;
}