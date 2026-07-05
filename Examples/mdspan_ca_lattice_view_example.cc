
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
 * @file mdspan_ca_lattice_view_example.cc
 * @brief `Dense_Cell_Storage::view()`: an `Aleph::mdspan` over a CA lattice.
 *
 * ## Overview
 *
 * `CA::Dense_Cell_Storage<T, N>` (`tpl_ca_storage.H`) already exposes
 * `at(coord)`/`set(coord, v)` and 2D/3D shorthands `at(i,j)`/`set(i,j,v)`.
 * `.view()` (and the read-only `.view() const`) hands back an
 * `Aleph::mdspan` over the *same* buffer — zero allocation, zero copy —
 * so code that wants plain `grid(i, j)` indexing (e.g. porting a
 * numerical kernel that was written against a raw 2D array) can have it
 * without going through the checked `at`/`set` pair one call at a time.
 *
 * - **[1] Same buffer, two ways to touch it**: writes through `.view()`
 *   are immediately visible through `.at()`, and vice versa.
 * - **[2] A tiny stencil kernel over the view**: sums each cell's 4
 *   orthogonal neighbours directly through `operator()`, the kind of
 *   inner loop this view exists for.
 * - **[3] Read-only view**: pass a `const Dense_Cell_Storage&` to a
 *   function that only needs to read it, and get a read-only `mdspan`.
 */

#include <iostream>

#include <print_rule.H>
#include <tpl_ca_storage.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace
{
void demo_shared_buffer()
{
  std::cout << "[1] .view() aliases the storage's own buffer\n";
  print_rule();

  Dense_Cell_Storage<int, 2> grid({3, 3}, 0);
  auto v = grid.view();

  v(1, 1) = 5;                                    // write through the view
  std::cout << "grid.at(1, 1) after v(1,1)=5:  " << grid.at(1, 1) << " (expect 5)\n";

  grid.set(0, 0, 9);                               // write through the storage API
  std::cout << "v(0, 0) after grid.set(0,0,9): " << v(0, 0) << " (expect 9)\n\n";
}

// A minimal 4-neighbour (von Neumann) sum, reading interior cells only —
// the kind of loop that benefits from plain m(i, j) indexing.
int neighbour_sum(const Dense_Cell_Storage<int, 2>::const_view_type &v, size_t i, size_t j)
{
  return v(i - 1, j) + v(i + 1, j) + v(i, j - 1) + v(i, j + 1);
}

void demo_stencil_kernel()
{
  std::cout << "[2] A tiny stencil kernel driven directly through the view\n";
  print_rule();

  Dense_Cell_Storage<int, 2> grid({4, 4}, 1);   // every cell starts at 1
  auto v = grid.view();

  std::cout << "Sum of the 4 orthogonal neighbours of every interior cell\n"
               "(each has value 1, so every sum should be 4):\n";
  for (size_t i = 1; i + 1 < v.extent(0); ++i)
    {
      for (size_t j = 1; j + 1 < v.extent(1); ++j)
        std::cout << neighbour_sum(v, i, j) << " ";
      std::cout << "\n";
    }
  std::cout << "\n";
}

void print_view(const Dense_Cell_Storage<int, 2>::const_view_type &v)
{
  for (size_t i = 0; i < v.extent(0); ++i)
    {
      for (size_t j = 0; j < v.extent(1); ++j)
        std::cout << v(i, j) << " ";
      std::cout << "\n";
    }
}

void demo_read_only_view()
{
  std::cout << "[3] Read-only view for a function that only reads the lattice\n";
  print_rule();

  Dense_Cell_Storage<int, 2> grid({2, 3});
  int next = 0;
  for (size_t i = 0; i < 2; ++i)
    for (size_t j = 0; j < 3; ++j)
      grid.set(i, j, next++);

  print_view(grid.view());   // const Dense_Cell_Storage& -> const_view_type
  std::cout << "\n";
}
}  // namespace

int main()
{
  std::cout << "\n=== Dense_Cell_Storage::view(): mdspan over a CA lattice ===\n\n";

  demo_shared_buffer();
  demo_stencil_kernel();
  demo_read_only_view();

  std::cout << "Done.\n";
  return 0;
}