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
 * @file graph_exhaustive_index_test.cc
 * @brief Index_Graph on every small multigraph, on the six kinds of
 *        graph, against a model.
 *
 * Index_Graph keeps the nodes by their infos and the arcs by their ends,
 * one arc for each pair, unordered unless the graph is directed. The test
 * builds through it every multigraph of the domain of graph_harness.H: a
 * second arc between the same ends must throw domain_error, and the
 * others go in. Then every search must give what the graph has, after the
 * insertions, after the removal of each arc and after that of each node.
 * An index that keeps an arc that the graph freed would read it on a
 * search; the test sees it by the size of the index, without searching.
 */

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_graph.H>
#include <tpl_indexGraph.H>

#include "graph_harness.H"
#include "graph_oracles.H"

using namespace Aleph;
using namespace graph_harness;

namespace
{
  /// The key of an arc in the index: its ends, unordered unless directed
  std::pair<int, int> key(int u, int v, bool directed)
  {
    return directed ? std::pair(u, v) : std::pair(std::min(u, v), std::max(u, v));
  }

  /// A graph built through an index from a spec: the arcs that went in,
  /// by number, and those that threw
  template <class GT>
  struct Indexed
  {
    GT g;
    Index_Graph<GT> idx;
    std::vector<typename GT::Node *> nodes;
    std::map<int, typename GT::Arc *> arcs; // the arcs that went in
    std::set<int> rejected;

    explicit Indexed(const Spec & s) : idx(g)
    {
      std::set<std::pair<int, int>> keys;
      for (int x = 0; x < s.n; ++x)
        nodes.push_back(idx.insert_node(x));
      for (size_t i = 0; i < s.arcs.size(); ++i)
        {
          const int u = s.arcs[i][0], v = s.arcs[i][1];
          try
            {
              arcs[int(i)] = idx.insert_arc(nodes[size_t(u)], nodes[size_t(v)], int(i));
              keys.insert(key(u, v, g.is_digraph()));
            }
          catch (const std::domain_error &)
            {
              rejected.insert(int(i));
            }
        }
    }
  };

  /// What is wrong with the index and the graph against the arcs and the
  /// nodes that are left; empty if nothing. With searches false, only the
  /// sizes are compared, which do not read the arcs.
  template <class GT>
  std::string index_error(Indexed<GT> & x, const Spec & s, const std::set<int> & nodes_left,
                          const std::map<int, typename GT::Arc *> & arcs_left, bool searches)
  {
    if (x.idx.get_num_nodes() != nodes_left.size() or x.idx.get_num_arcs() != arcs_left.size())
      return "the index has " + std::to_string(x.idx.get_num_nodes()) + " nodes and "
        + std::to_string(x.idx.get_num_arcs()) + " arcs, not " + std::to_string(nodes_left.size())
        + " and " + std::to_string(arcs_left.size());
    if (x.g.get_num_nodes() != nodes_left.size() or x.g.get_num_arcs() != arcs_left.size())
      return "the graph has " + std::to_string(x.g.get_num_nodes()) + " nodes and "
        + std::to_string(x.g.get_num_arcs()) + " arcs";
    if (not searches)
      return "";

    const bool directed = x.g.is_digraph();
    for (int v = 0; v < s.n; ++v)
      {
        auto * p = x.idx.search_node(v);
        if ((p != nullptr) != (nodes_left.count(v) > 0) or (p != nullptr and num(p) != v))
          return "search_node(" + std::to_string(v) + ") is wrong";
        if (p != nullptr and x.idx.search_node(p) != p)
          return "search_node(p) does not give p";
      }
    for (int u : nodes_left)
      for (int v : nodes_left)
        {
          int want = -1;
          for (const auto & [i, a] : arcs_left)
            if (key(s.arcs[size_t(i)][0], s.arcs[size_t(i)][1], directed) == key(u, v, directed))
              want = i;
          auto * a = x.idx.search_arc(x.nodes[size_t(u)], x.nodes[size_t(v)]);
          if ((a == nullptr ? -1 : num(a)) != want)
            return "search_arc(" + std::to_string(u) + ", " + std::to_string(v) + ") gives "
              + std::to_string(a == nullptr ? -1 : num(a)) + ", not " + std::to_string(want);
        }
    return "";
  }
}

template <class GT>
class ExhaustiveIndex : public ::testing::Test {};

TYPED_TEST_SUITE(ExhaustiveIndex, All_Kinds);

TYPED_TEST(ExhaustiveIndex, SearchesMatchWhatTheGraphHas)
{
  using GT = TypeParam;
  const bool directed = GT().is_digraph();
  Check build("Index_Graph, the insertions");
  Check arcs("Index_Graph::remove_arc()");
  Check nodes("Index_Graph::remove_node()");
  Check arcs_in("Index_Graph::remove_node() of a node with arcs in");

  for_each_spec(domain(4, 3), directed, [&] (const Spec & s)
    {
      {
        build.run();
        Indexed<GT> x(s);
        // A second arc between the same ends is the one rejected
        std::set<std::pair<int, int>> seen;
        bool ok = true;
        for (size_t i = 0; i < s.arcs.size(); ++i)
          {
            const bool repeated
              = not seen.insert(key(s.arcs[i][0], s.arcs[i][1], directed)).second;
            ok = ok and repeated == (x.rejected.count(int(i)) > 0);
          }
        std::set<int> all;
        for (int v = 0; v < s.n; ++v)
          all.insert(v);
        if (not ok)
          build.fail(s, "it takes a repeated pair of ends, or rejects a new one");
        else if (auto e = index_error(x, s, all, x.arcs, true); not e.empty())
          build.fail(s, e);
      }

      // Each arc that went in, removed
      for (size_t i = 0; i < s.arcs.size(); ++i)
        {
          Indexed<GT> x(s);
          if (not x.arcs.count(int(i)))
            continue;
          arcs.run();
          auto left = x.arcs;
          x.idx.remove_arc(left[int(i)]);
          left.erase(int(i));
          std::set<int> all;
          for (int v = 0; v < s.n; ++v)
            all.insert(v);
          if (auto e = index_error(x, s, all, left, true); not e.empty())
            arcs.fail(s, "without the arc " + std::to_string(i) + ": " + e);
        }

      // Each node removed, with its arcs
      for (int v = 0; v < s.n; ++v)
        {
          Indexed<GT> x(s);
          auto left = x.arcs;
          bool in_from_others = false;
          for (auto it = left.begin(); it != left.end(); )
            {
              const auto & a = s.arcs[size_t(it->first)];
              in_from_others = in_from_others or (a[1] == v and a[0] != v);
              it = a[0] == v or a[1] == v ? left.erase(it) : std::next(it);
            }
          Check & check = in_from_others ? arcs_in : nodes;
          check.run();
          x.idx.remove_node(x.nodes[size_t(v)]);
          std::set<int> rest;
          for (int y = 0; y < s.n; ++y)
            if (y != v)
              rest.insert(y);
          // A search would read the arcs that the index kept and the graph
          // freed: only the sizes, if the sizes are wrong
          if (auto e = index_error(x, s, rest, left, false); not e.empty())
            check.fail(s, "without the node " + std::to_string(v) + ": " + e);
          else if (auto e2 = index_error(x, s, rest, left, true); not e2.empty())
            check.fail(s, "without the node " + std::to_string(v) + ": " + e2);
        }
    });

  build.expect();
  arcs.expect();
  nodes.expect();
  arcs_in.expect();
}
