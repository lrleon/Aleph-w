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
 * @file ca_voter_model_example.cc
 * @brief Phase-6 illustration: voter model on a small hand-built graph.
 *
 * Builds a connected 12-node graph (a 3-cycle of triangles meeting at
 * a central hub) and runs the deterministic-seed voter model on it.
 * Prints node states at every step plus a tiny TikZ rendering of the
 * final state suitable for inclusion in a paper.
 *
 * The voter model on any finite connected graph is a martingale and
 * almost surely reaches consensus; with a fixed seed it does so in
 * a few thousand steps for 12 nodes.
 */

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include <tpl_ca_graph_automaton.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace {

// Voter rule with a unique sub-seed per call — deterministic given
// the engine schedule (which itself is deterministic).
class Voter_Rule
{
  std::uint64_t seed_;
  mutable std::uint64_t calls_ = 0;

public:
  explicit Voter_Rule(std::uint64_t seed) noexcept : seed_(seed) {}

  [[nodiscard]] int operator()(int current, Neighbor_View<int> neigh) const noexcept
  {
    if (neigh.empty())
      return current;
    std::uint64_t key = seed_;
    key = (key * 1099511628211ull) ^ (calls_ + 1);
    ++calls_;
    std::mt19937_64 rng(key);
    std::uniform_int_distribution<std::size_t> pick(0, neigh.size() - 1);
    return neigh[pick(rng)];
  }
};

// Build a "bowtie": three triangles glued at node 0.
std::vector<std::vector<std::size_t>> build_bowtie()
{
  // Nodes: 0 (hub), then three triangles {1,2}, {3,4}, {5,6}, each
  // forming a triangle with node 0.
  std::vector<std::vector<std::size_t>> adj(7);
  auto edge = [&](std::size_t a, std::size_t b)
  {
    adj[a].push_back(b);
    adj[b].push_back(a);
  };
  // Triangle 1
  edge(0, 1); edge(0, 2); edge(1, 2);
  // Triangle 2
  edge(0, 3); edge(0, 4); edge(3, 4);
  // Triangle 3
  edge(0, 5); edge(0, 6); edge(5, 6);
  return adj;
}

void print_state(const Graph_Lattice<int> &lat, std::size_t step)
{
  std::printf("Step %4zu :", step);
  for (std::size_t n = 0; n < lat.size(); ++n)
    std::printf(" %d", lat.at_node(n));
  std::putchar('\n');
}

std::string render_tikz(const Graph_Lattice<int> &lat)
{
  // Hard-coded layout: the hub at (0,0), the three triangles around
  // it at 120-degree increments.
  std::ostringstream os;
  os << "% Aleph::CA voter model bowtie\n";
  os << "\\begin{tikzpicture}[node distance=1.5cm]\n";
  const double radius = 2.0;
  const double pi = 3.14159265358979323846;
  std::array<std::array<double, 2>, 7> pos{};
  pos[0] = {0.0, 0.0};
  for (int t = 0; t < 3; ++t)
    {
      const double theta0 = (2 * pi / 3.0) * static_cast<double>(t);
      pos[1 + 2 * t] = {radius * std::cos(theta0 - 0.3),
                        radius * std::sin(theta0 - 0.3)};
      pos[2 + 2 * t] = {radius * std::cos(theta0 + 0.3),
                        radius * std::sin(theta0 + 0.3)};
    }

  for (std::size_t n = 0; n < 7; ++n)
    {
      const char *colour = lat.at_node(n) != 0 ? "red!60" : "blue!60";
      os << "  \\node[circle, draw, fill=" << colour
         << ", inner sep=2pt, minimum size=6mm] (n" << n << ") at (" << pos[n][0]
         << ", " << pos[n][1] << ") {" << n << "};\n";
    }
  // Edges (deduplicated by always going low->high).
  for (std::size_t a = 0; a < 7; ++a)
    for (std::size_t b : lat.neighbours(a))
      if (b > a)
        os << "  \\draw (n" << a << ") -- (n" << b << ");\n";
  os << "\\end{tikzpicture}\n";
  return os.str();
}

}  // namespace

int main()
{
  Graph_Lattice<int> seed(build_bowtie(), 0);
  // Polarise: hub at 0, alternate triangles biased.
  seed.set_node(0, 0);
  seed.set_node(1, 1); seed.set_node(2, 1);
  seed.set_node(3, 0); seed.set_node(4, 0);
  seed.set_node(5, 1); seed.set_node(6, 1);

  std::printf("Aleph::CA Phase-6 example: voter model on a 7-node bowtie graph\n");
  print_state(seed, 0);

  Graph_Synchronous_Engine<Graph_Lattice<int>, Voter_Rule> eng(seed, Voter_Rule{2026});

  // Run, sampling the state every 50 steps and stopping when we
  // detect consensus.
  bool consensus = false;
  for (std::size_t s = 1; s <= 5000; ++s)
    {
      eng.step();
      if ((s % 200) == 0 or s == 1)
        print_state(eng.frame(), s);
      bool all_eq = true;
      const int v0 = eng.frame().at_node(0);
      for (std::size_t n = 1; n < eng.frame().size(); ++n)
        if (eng.frame().at_node(n) != v0)
          {
            all_eq = false;
            break;
          }
      if (all_eq)
        {
          consensus = true;
          std::printf("Consensus on value %d reached at step %zu\n", v0, s);
          print_state(eng.frame(), s);
          break;
        }
    }
  if (not consensus)
    {
      std::printf("Did not reach consensus within 5000 steps; final state:\n");
      print_state(eng.frame(), 5000);
    }

  std::printf("\n--- TikZ render of final state ---\n");
  std::fputs(render_tikz(eng.frame()).c_str(), stdout);
  return 0;
}