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
 * @file ah_cpp_compat_example.cc
 * @brief Demonstrates `Aleph::expected` from the C++20/23 compatibility layer.
 *
 * `Aleph::expected<T,E>` is a non-throwing result type: a function returns
 * *either* a value or an error, and the caller decides how to react. It maps to
 * `std::expected` when the toolchain provides C++23, and to an in-house polyfill
 * under C++20. This example shows the canonical "parse without exceptions"
 * pattern plus the monadic combinators.
 *
 * Build & run:
 *   cmake --build build --target ah_cpp_compat_example
 *   ./build/Examples/ah_cpp_compat_example
 */

#include <iostream>
#include <string>

#include <ah-cpp-compat.H>

using Aleph::expected;
using Aleph::unexpected;

namespace
{

// A parse-like operation: turn a string into a positive int, or explain why
// it could not. No exceptions cross the boundary — the error is a value.
expected<int, std::string> parse_positive(const std::string &text)
{
  if (text.empty())
    return unexpected<std::string>("empty input");

  int value = 0;
  for (char c : text)
    {
      if (c < '0' or c > '9')
        return unexpected<std::string>("not a number: '" + text + "'");
      value = value * 10 + (c - '0');
    }

  if (value == 0)
    return unexpected<std::string>("must be > 0");

  return value;
}

void report(const std::string &input)
{
  auto r = parse_positive(input);
  std::cout << "  parse_positive(\"" << input << "\") -> ";
  if (r)  // explicit operator bool
    std::cout << "value " << *r << '\n';
  else
    std::cout << "error: " << r.error() << '\n';
}

}  // namespace

int main()
{
  std::cout << "C++23 library facilities detected on this toolchain:\n"
            << "  std::expected  = " << ALEPH_HAS_STD_EXPECTED << '\n'
            << "  std::generator = " << ALEPH_HAS_STD_GENERATOR << '\n'
            << "  std::flat_map  = " << ALEPH_HAS_STD_FLAT_MAP << '\n'
            << "  std::mdspan    = " << ALEPH_HAS_STD_MDSPAN << "\n\n";

  std::cout << "Basic value / error reporting:\n";
  report("42");
  report("7x");
  report("0");
  report("");

  std::cout << "\nvalue_or (supply a fallback instead of branching):\n";
  std::cout << "  parse_positive(\"100\").value_or(-1) = "
            << parse_positive("100").value_or(-1) << '\n';
  std::cout << "  parse_positive(\"bad\").value_or(-1) = "
            << parse_positive("bad").value_or(-1) << '\n';

  std::cout << "\nMonadic chaining (transform / and_then / or_else):\n";

  // transform: apply a pure function to the value, propagate errors untouched.
  auto squared = parse_positive("9").transform([](int v) { return v * v; });
  std::cout << "  \"9\" |> transform(square)        = "
            << (squared ? std::to_string(*squared) : squared.error()) << '\n';

  // and_then: chain another fallible step.
  auto chained = parse_positive("5").and_then(
      [](int v) -> expected<int, std::string>
      {
        if (v > 1000)
          return unexpected<std::string>("too large");
        return v + 100;
      });
  std::cout << "  \"5\"  |> and_then(+100 if <=1000) = "
            << (chained ? std::to_string(*chained) : chained.error()) << '\n';

  // or_else: recover from an error with a default.
  auto recovered = parse_positive("oops").or_else(
      [](const std::string &) -> expected<int, std::string> { return 0; });
  std::cout << "  \"oops\" |> or_else(0)            = "
            << (recovered ? std::to_string(*recovered) : recovered.error())
            << '\n';

  return 0;
}