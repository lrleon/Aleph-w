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
 * @file ca_ghost_boundary_showcase_example.cc
 * @brief Visual tour of every Phase 4 boundary policy for `Ghost_Lattice`.
 *
 * Seeds a 4x4 lattice whose cells encode `i*10 + j` and then renders
 * the 8x8 store region (4 interior + 2 halo cells per side) for every
 * policy. The interior is framed with `|` and the halo region is
 * visually distinct, so the user can tell at a glance how each policy
 * fills the halo.
 */

#include <iomanip>
#include <iostream>
#include <string>

#include <ca-traits.H>
#include <tpl_ca_storage.H>
#include <tpl_ca_ghost_lattice.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace
{
  constexpr ca_size_t INTERIOR = 4;
  constexpr std::size_t HALO = 2;

  /// Build a 4x4 Ghost_Lattice with cells = `i*10 + j` and the given halo radius.
  template <typename Boundary>
  Ghost_Lattice<Dense_Cell_Storage<int, 2>, Boundary, HALO>
  make_showcase_lattice()
  {
    Ghost_Lattice<Dense_Cell_Storage<int, 2>, Boundary, HALO>
      g({ INTERIOR, INTERIOR }, 0);
    for (ca_index_t i = 0; i < static_cast<ca_index_t>(INTERIOR); ++i)
      for (ca_index_t j = 0; j < static_cast<ca_index_t>(INTERIOR); ++j)
        g.set({ i, j }, static_cast<int>(i * 10 + j));
    g.refresh_halo();
    return g;
  }

  /// Render the full 8x8 store region (halo + interior + halo).
  template <typename Boundary>
  void render(const std::string &name,
              const Ghost_Lattice<Dense_Cell_Storage<int, 2>, Boundary, HALO> &g)
  {
    const ca_index_t h = static_cast<ca_index_t>(HALO);
    const ca_index_t n = static_cast<ca_index_t>(INTERIOR);

    std::cout << "\n=== " << name << " ===\n";
    for (ca_index_t i = -h; i < n + h; ++i)
      {
        for (ca_index_t j = -h; j < n + h; ++j)
          {
            const bool boundary_row = (j == 0 or j == n);
            if (boundary_row)
              std::cout << "| ";
            std::cout << std::setw(3) << g.at_safe({ i, j }) << ' ';
          }
        std::cout << '\n';
        if (i == -1 or i == n - 1)
          {
            // Separator row between halo and interior.
            for (ca_index_t j = -h; j < n + h; ++j)
              std::cout << "----";
            std::cout << '\n';
          }
      }
  }
}

int main()
{
  std::cout <<
    "Ghost_Lattice boundary showcase\n"
    "-------------------------------\n"
    "Interior is a 4x4 lattice with cells = i*10 + j (00,01,... ,33).\n"
    "Halo thickness is 2, so you see a 2-cell ring around the interior.\n"
    "Vertical bars mark the user-visible borders on axis 1.\n";

  render("OpenBoundary       (halo = 0)",
         make_showcase_lattice<OpenBoundary>());
  render("ConstantBoundary<99>  (Dirichlet)",
         make_showcase_lattice<ConstantBoundary<int, 99>>());
  render("NeumannBoundary    (clamp to border)",
         make_showcase_lattice<NeumannBoundary>());
  render("PeriodicBoundary   (toroidal wrap)",
         make_showcase_lattice<PeriodicBoundary>());
  render("ReflectiveBoundary (mirror at edge)",
         make_showcase_lattice<ReflectiveBoundary>());

  return 0;
}
