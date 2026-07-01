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
 * @file ca_io_expected_example.cc
 * @brief Parsing CA patterns without exceptions via the `try_read_*` overloads.
 *
 * The `try_read_*` readers in `ca-io.H` return `Aleph::expected<Result,
 * std::string>` instead of throwing. This is convenient when the input is
 * untrusted (a user-supplied file): an invalid pattern is an ordinary result,
 * not an exceptional condition, and the call site reads as a simple branch.
 *
 * Build & run:
 *   cmake --build build --target ca_io_expected_example
 *   ./build/Examples/ca_io_expected_example
 */

#include <cstddef>
#include <iostream>
#include <sstream>
#include <string>

#include <ca-io.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace
{

void load_rle(const std::string &label, const std::string &text)
{
  // No try/catch: the error is a value we inspect.
  auto result = try_read_rle_string(text);
  std::cout << "  " << label << ": ";
  if (result)
    std::cout << "ok — " << result->width << "x" << result->height << " grid, "
              << result->alive.size() << " live cell(s)\n";
  else
    std::cout << "rejected — " << result.error() << '\n';
}

}  // namespace

int main()
{
  std::cout << "Parsing RLE patterns with try_read_rle_string (no exceptions):\n";

  load_rle("glider", "x = 3, y = 3, rule = B3/S23\nbob$2bo$3o!\n");
  load_rle("blinker", "x = 3, y = 1, rule = B3/S23\n3o!\n");
  load_rle("garbage", "this is not a pattern @@@");
  load_rle("truncated", "x = 5, y = 5, rule = B3/S23\n");

  // A pipeline that loads from one of several formats, recovering on failure.
  std::cout << "\nFalling back across formats with or_else:\n";
  const std::string maybe_life = "#Life 1.06\n0 0\n1 0\n2 0\n";

  std::istringstream rle_in(maybe_life);
  auto cells =
      try_read_rle(rle_in)                         // try RLE first...
          .transform([](const RLE_Pattern &p) { return p.alive.size(); })
          .or_else(
              [&](const std::string &) -> expected<std::size_t, std::string>
              {
                // ...not RLE; fall back to Life 1.06.
                std::istringstream life_in(maybe_life);
                return try_read_life_106(life_in).transform(
                    [](const Binary_Cell_Pattern &p) { return p.alive.size(); });
              });

  if (cells)
    std::cout << "  parsed " << *cells << " live cell(s) after fallback\n";
  else
    std::cout << "  every format failed: " << cells.error() << '\n';

  return 0;
}