
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
 * @file mdspan_matrix_view_example.cc
 * @brief `Aleph::mdspan` as a zero-copy 2D "matrix" view over a flat buffer.
 *
 * ## Overview
 *
 * `Aleph::mdspan` never owns memory: it is a pointer plus a shape. This
 * example walks through the case that motivates it most directly —
 * numerical code that already works with a flat, contiguous buffer
 * (`Array<T>`, `std::vector<T>`, a `T*` from a C library, ...) and wants
 * readable `m(row, col)` indexing without transposing or copying anything.
 *
 * - **[1] Row-major (the default)**: wrap a flat buffer and index it like
 *   a matrix, matching the C/row-major convention Aleph itself uses.
 * - **[2] Column-major interop**: view the *same* buffer as column-major
 *   (`Aleph::layout_left`) without moving a single byte — useful when
 *   interfacing with Fortran/BLAS-style routines that expect that layout.
 * - **[3] Read-only views**: `mdspan<const T, ...>` exposes a buffer for
 *   reading without allowing the view to mutate it.
 */

#include <iostream>
#include <vector>

#include <ah-mdspan.H>
#include <print_rule.H>

using namespace Aleph;

namespace
{
void demo_row_major()
{
  std::cout << "[1] Row-major view (Aleph::layout_right, the default)\n";
  print_rule();

  // A 3x4 matrix stored flat, as numerical code typically hands it over.
  std::vector<double> buf(3 * 4, 0.0);
  mdspan<double, dextents<size_t, 2>> m(buf.data(), 3, 4);

  for (size_t i = 0; i < m.extent(0); ++i)
    for (size_t j = 0; j < m.extent(1); ++j)
      m(i, j) = static_cast<double>(i * 10 + j);

  std::cout << "m(" << m.extent(0) << "x" << m.extent(1) << "):\n";
  for (size_t i = 0; i < m.extent(0); ++i)
    {
      for (size_t j = 0; j < m.extent(1); ++j)
        std::cout << m(i, j) << "\t";
      std::cout << "\n";
    }
  std::cout << "Row-major: m(1, 2) sits at buf[1*4 + 2] = buf["
            << (1 * m.extent(1) + 2) << "] = " << buf[1 * 4 + 2] << "\n\n";
}

void demo_column_major_interop()
{
  std::cout << "[2] Column-major view (Aleph::layout_left) over the same data\n";
  print_rule();

  // Same flat buffer, no copy: just a different layout policy on top.
  std::vector<int> buf(2 * 3);
  mdspan<int, dextents<size_t, 2>, layout_left> m(buf.data(), 2, 3);

  int next = 0;
  for (size_t j = 0; j < m.extent(1); ++j)   // fastest-varying axis is first
    for (size_t i = 0; i < m.extent(0); ++i)
      m(i, j) = next++;

  std::cout << "Column-major fill order produced buf = [";
  for (size_t k = 0; k < buf.size(); ++k)
    std::cout << buf[k] << (k + 1 < buf.size() ? ", " : "");
  std::cout << "]\n";
  std::cout << "(first axis varies fastest: m(0,0),m(1,0),m(0,1),... -- the\n"
               " layout a Fortran/BLAS routine would expect from this buffer)\n\n";
}

void demo_read_only_view()
{
  std::cout << "[3] Read-only view over const data\n";
  print_rule();

  const std::vector<double> readings{98.6, 99.1, 97.9, 100.2};
  mdspan<const double, dextents<size_t, 1>> m(readings.data(), readings.size());

  double total = 0.0;
  for (size_t i = 0; i < m.extent(0); ++i)
    total += m(i);
  std::cout << "Average of " << m.extent(0) << " read-only samples: "
            << (total / static_cast<double>(m.extent(0))) << "\n";
  std::cout << "(m(0) = readings[0]; the view has no write access to \"const double\")\n\n";
}
}  // namespace

int main()
{
  std::cout << "\n=== Aleph::mdspan: zero-copy matrix views ===\n\n";

  demo_row_major();
  demo_column_major_interop();
  demo_read_only_view();

  std::cout << "Done.\n";
  return 0;
}