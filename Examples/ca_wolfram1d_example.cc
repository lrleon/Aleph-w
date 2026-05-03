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
 * @file ca_wolfram1d_example.cc
 * @brief Print one row of every elementary 1D Wolfram rule (0..255)
 *        starting from a single-1 impulse at the centre of a length-N
 *        lattice and advancing for `T` steps under an open boundary.
 *
 * Usage:
 *   ca_wolfram1d_example [rule] [width] [steps]
 *
 * With no arguments it prints all 256 rules at width 61 over 30 steps,
 * using ASCII (`#` for 1, ` ` for 0). Pass a single rule number to
 * focus the output.
 */

# include <array>
# include <cerrno>
# include <cstdint>
# include <cstdlib>
# include <iostream>
# include <limits>
# include <span>
# include <string>

# include <ca-traits.H>
# include <tpl_ca_storage.H>
# include <tpl_ca_lattice.H>
# include <tpl_ca_neighborhood.H>
# include <tpl_ca_rule.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace
{
  using Row = Lattice<Dense_Cell_Storage<int, 1>, OpenBoundary>;
  constexpr ca_size_t Max_Width = 100000;
  constexpr std::size_t Max_Steps = 100000;

  bool parse_rule_number(const char * text, int & out)
  {
    errno = 0;
    char * end = nullptr;
    const long value = std::strtol(text, &end, 10);
    if (errno == ERANGE or end == text or *end != '\0' or
        value < 0 or value > 255)
      {
        std::cerr << "Invalid rule: '" << text
                  << "' (expected an integer in [0, 255])\n";
        return false;
      }
    out = static_cast<int>(value);
    return true;
  }

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
    if (not parse_positive_size(text, name, Max_Width, parsed))
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

  Row step_row(const Row & cur, const Lookup_Rule<2, 2> & rule)
  {
    Row nxt(cur.extents());
    Custom_Neighborhood<1, 2> nh({{ Offset_Vec<1>{ -1 },
                                    Offset_Vec<1>{  1 } }});
    std::array<int, 2> buf { };
    for (ca_index_t i = 0; i < static_cast<ca_index_t>(cur.size(0)); ++i)
      {
        gather_neighbors(nh, cur, Coord_Vec<1>{ i },
                         std::span<int>(buf.data(), buf.size()));
        nxt.set({ i },
                rule(cur.at({ i }), Neighbor_View<int>(buf.data(), 2)));
      }
    return nxt;
  }

  void print_row(const Row & r)
  {
    std::string s;
    s.reserve(r.size(0));
    for (ca_index_t i = 0; i < static_cast<ca_index_t>(r.size(0)); ++i)
      s.push_back(r.at({ i }) ? '#' : ' ');
    std::cout << s << '\n';
  }

  void run_rule(unsigned rule_no, ca_size_t width, std::size_t steps)
  {
    auto rule = make_wolfram_elementary_rule(
      static_cast<std::uint8_t>(rule_no));
    Row row({ width });
    row.set({ static_cast<ca_index_t>(width / 2) }, 1);

    std::cout << "----- Rule " << rule_no << " -----\n";
    print_row(row);
    for (std::size_t t = 1; t <= steps; ++t)
      {
        row = step_row(row, rule);
        print_row(row);
      }
  }
}

int main(int argc, char ** argv)
{
  ca_size_t width = 61;
  std::size_t steps = 30;
  int focus_rule = -1;

  if (argc >= 2 and not parse_rule_number(argv[1], focus_rule))
    return 1;
  if (argc >= 3 and not parse_positive_ca_size(argv[2], "width", width))
    return 1;
  if (argc >= 4 and not parse_positive_size(argv[3], "steps",
                                            Max_Steps, steps))
    return 1;

  if (focus_rule >= 0 and focus_rule <= 255)
    run_rule(static_cast<unsigned>(focus_rule), width, steps);
  else
    for (unsigned r = 0; r < 256; ++r)
      run_rule(r, width, steps);

  return 0;
}
