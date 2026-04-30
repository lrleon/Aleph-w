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
# include <cstdint>
# include <cstdlib>
# include <iostream>
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

  Row step_row(const Row & cur, const Lookup_Rule<2, 2> & rule)
  {
    Row nxt(cur.extents());
    Custom_Neighborhood<1, 2> nh({{ Offset_Vec<1>{ -1 },
                                    Offset_Vec<1>{  1 } }});
    std::array<int, 2> buf { };
    for (ca_index_t i = 0; i < static_cast<ca_index_t>(cur.size(0)); ++i)
      {
        gather_neighbors(nh, cur, Coord_Vec<1>{ i }, buf.data());
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

  if (argc >= 2)
    focus_rule = std::atoi(argv[1]);
  if (argc >= 3)
    width = static_cast<ca_size_t>(std::atoll(argv[2]));
  if (argc >= 4)
    steps = static_cast<std::size_t>(std::atoll(argv[3]));

  if (focus_rule >= 0 and focus_rule <= 255)
    run_rule(static_cast<unsigned>(focus_rule), width, steps);
  else
    for (unsigned r = 0; r < 256; ++r)
      run_rule(r, width, steps);

  return 0;
}