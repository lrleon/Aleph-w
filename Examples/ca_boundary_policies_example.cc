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
 * @file ca_boundary_policies_example.cc
 * @brief Side-by-side demonstration of every Phase 1 boundary policy.
 *
 * Builds a 3x3 lattice whose cells encode `i*10 + j` (so each cell is
 * uniquely identifiable), then evaluates `at_safe(coord)` over a 7x7
 * window centred on the lattice. The window includes a one-cell halo
 * outside the lattice on every side, plus another one to make the
 * resolution policy obvious. Each boundary class is rendered next to
 * the others so the differences stand out at a glance.
 */

# include <iomanip>
# include <iostream>
# include <string>

# include <ca-traits.H>
# include <tpl_ca_storage.H>
# include <tpl_ca_lattice.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace
{
  /// Build a 3x3 `int` lattice initialised so that `at({i,j}) = i*10 + j`.
  template <typename Boundary>
  Lattice<Dense_Cell_Storage<int, 2>, Boundary> make_lattice()
  {
    Lattice<Dense_Cell_Storage<int, 2>, Boundary> lat({ 3, 3 }, 0);
    for (ca_index_t i = 0; i < 3; ++i)
      for (ca_index_t j = 0; j < 3; ++j)
        lat.set({ i, j }, static_cast<int>(i * 10 + j));
    return lat;
  }

  /// Render a 7x7 window centred on the lattice (axes go from -2 to +4).
  /// Cells inside `[0,3) x [0,3)` are highlighted with brackets so the
  /// reader can tell where the actual lattice ends.
  template <typename Lat>
  void render(const std::string & title, const Lat & lat)
  {
    std::cout << "\n=== " << title << " ===\n";
    std::cout << "       ";
    for (ca_index_t j = -2; j <= 4; ++j)
      std::cout << std::setw(4) << j;
    std::cout << "\n";
    for (ca_index_t i = -2; i <= 4; ++i)
      {
        std::cout << "  i=" << std::setw(2) << i << " ";
        for (ca_index_t j = -2; j <= 4; ++j)
          {
            const int v = lat.at_safe({ i, j });
            const bool inside =
              (i >= 0 and i < 3 and j >= 0 and j < 3);
            std::cout << (inside ? '[' : ' ')
                      << std::setw(2) << v
                      << (inside ? ']' : ' ');
          }
        std::cout << "\n";
      }
  }
}

int main()
{
  std::cout << "Aleph::CA — boundary policies side-by-side\n";
  std::cout << "==========================================\n";
  std::cout << "Centre lattice: 3x3 with cell(i,j) = i*10 + j.\n";
  std::cout << "Brackets [..] mark cells inside the lattice;\n";
  std::cout << "everything else is resolved by `at_safe` through\n";
  std::cout << "the boundary policy.\n";

  render("OpenBoundary (out-of-range = 0)",
         make_lattice<OpenBoundary>());

  render("ToroidalBoundary (wrap on each axis)",
         make_lattice<ToroidalBoundary>());

  render("ReflectiveBoundary (mirror, no repeat)",
         make_lattice<ReflectiveBoundary>());

  render("ConstantBoundary<int, 99>",
         make_lattice<ConstantBoundary<int, 99>>());

  return 0;
}
