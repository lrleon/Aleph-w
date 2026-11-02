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
 * @file graph_exhaustive_planarity_test.cc
 * @brief The planarity test on every small graph, against Kuratowski.
 *
 * On 6 nodes or less, a graph is not planar if and only if it has, as a
 * subgraph, K5, K5 with an edge split by a sixth node, or K3,3: the
 * oracle looks for them. The test runs on every simple graph of up to 5
 * nodes, on a sample of the simple graphs of 6 with 8 arcs or more (all
 * of them with ALEPH_GRAPH_HARNESS=heavy), on every multigraph of up to
 * 3 nodes and 3 arcs with each set of hidden arcs, and on K5 and K3,3
 * with sets of hidden arcs, on the six kinds of graph. Besides the answer, it checks the embedding of
 * a planar graph, by its faces and Euler's formula in each component,
 * and the certificate of one that is not, by its paths, with checks of
 * its own.
 */

#include <algorithm>
#include <map>
#include <random>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_graph.H>
#include <Planarity_Test.H>

#include "graph_filter_catalog.H"
#include "graph_harness.H"
#include "graph_oracles.H"

using namespace Aleph;
using namespace graph_harness;
namespace oracle = graph_oracles;
using graph_filters::Hide_Mask;

namespace
{
  using Adjacency = std::vector<std::vector<bool>>;

  /// The simple graph of the arcs of s that hidden shows, not directed
  Adjacency simple_adjacency(const Spec & s, unsigned long hidden = 0)
  {
    Adjacency adj(size_t(s.n), std::vector<bool>(size_t(s.n), false));
    for (size_t i = 0; i < s.arcs.size(); ++i)
      {
        const int u = s.arcs[i][0], v = s.arcs[i][1];
        if (u != v and graph_filters::shown(hidden, int(i)))
          adj[size_t(u)][size_t(v)] = adj[size_t(v)][size_t(u)] = true;
      }
    return adj;
  }

  size_t num_edges(const Adjacency & adj)
  {
    size_t m = 0;
    for (size_t u = 0; u < adj.size(); ++u)
      for (size_t v = u + 1; v < adj.size(); ++v)
        m += adj[u][v];
    return m;
  }

  /// Whether a simple graph of up to 6 nodes is planar: whether it has no
  /// K5, no K5 with an edge split by a sixth node and no K3,3
  bool planar(const Adjacency & adj)
  {
    const int n = int(adj.size());
    auto all_joined = [&] (const std::vector<int> & nodes, int a = -1, int b = -1)
      {
        for (size_t i = 0; i < nodes.size(); ++i)
          for (size_t j = i + 1; j < nodes.size(); ++j)
            {
              const int u = nodes[i], v = nodes[j];
              if ((u == a and v == b) or (u == b and v == a))
                continue;
              if (not adj[size_t(u)][size_t(v)])
                return false;
            }
        return true;
      };

    // K5 on each set of 5 nodes; with 6, the node left out may split an
    // edge
    for (int out = 0; out < n; ++out)
      {
        if (n == 5 and out > 0)
          break;
        std::vector<int> five;
        for (int x = 0; x < n; ++x)
          if (n == 5 or x != out)
            five.push_back(x);
        if (five.size() != 5)
          continue;
        if (all_joined(five))
          return false;
        if (n == 6)
          for (int a : five)
            for (int b : five)
              if (a < b and adj[size_t(out)][size_t(a)] and adj[size_t(out)][size_t(b)]
                  and all_joined(five, a, b))
                return false;
      }

    // K3,3 on each split of 6 nodes in two triples
    if (n == 6)
      for (int x = 1; x < 6; ++x)
        for (int y = x + 1; y < 6; ++y)
          {
            const std::vector<int> side = {0, x, y};
            std::vector<int> other;
            for (int z = 1; z < 6; ++z)
              if (z != x and z != y)
                other.push_back(z);
            bool all = true;
            for (int u : side)
              for (int v : other)
                all = all and adj[size_t(u)][size_t(v)];
            if (all)
              return false;
          }
    return true;
  }

  /** What is wrong with the rotation system of r as a plane embedding of
      the simple graph adj: each node turns around its neighbors, each
      once, and the faces that the rotation gives satisfy, in each
      component with an edge, V - E + F = 2; empty if nothing. faces gets
      the faces of the whole drawing, where the components share their
      outer face: V - E + F = 1 + C. */
  template <class GT>
  std::string embedding_error(const Planarity_Test_Result<GT> & r, const Adjacency & adj,
                              size_t & faces)
  {
    const int n = int(adj.size());
    std::map<int, std::vector<int>> rot;
    for (size_t i = 0; i < r.embedding_rotation.size(); ++i)
      {
        const auto & e = r.embedding_rotation[i];
        const int x = num(e.node);
        if (x < 0 or x >= n or rot.count(x))
          return "the node " + std::to_string(x) + " is twice in the rotation";
        auto & around = rot[x];
        for (size_t j = 0; j < e.cw_neighbors.size(); ++j)
          around.push_back(num(e.cw_neighbors[j]));
      }
    for (int x = 0; x < n; ++x)
      {
        std::vector<int> want;
        for (int y = 0; y < n; ++y)
          if (adj[size_t(x)][size_t(y)])
            want.push_back(y);
        std::vector<int> got = rot.count(x) ? rot[x] : std::vector<int>();
        std::sort(got.begin(), got.end());
        if (got != want)
          return "the rotation of " + std::to_string(x) + " is not its neighbors";
      }

    // The faces, as the orbits of the darts: after u -> v comes v -> w,
    // with w the neighbor of v after u in its rotation
    std::set<std::pair<int, int>> seen;
    const auto comp = [&]
      {
        Spec s;
        s.n = n;
        for (int u = 0; u < n; ++u)
          for (int v = u + 1; v < n; ++v)
            if (adj[size_t(u)][size_t(v)])
              s.arcs.push_back({u, v});
        return oracle::component_of(s);
      }();
    std::map<int, int> faces_of;
    faces = 0;
    for (int u = 0; u < n; ++u)
      for (int v : rot[u])
        {
          if (seen.count({u, v}))
            continue;
          ++faces;
          ++faces_of[comp[size_t(u)]];
          int a = u, b = v;
          while (seen.insert({a, b}).second)
            {
              const auto & around = rot[b];
              const auto at = size_t(std::find(around.begin(), around.end(), a) - around.begin());
              if (at == around.size())
                return "the rotation of " + std::to_string(b) + " misses " + std::to_string(a);
              const int c = around[(at + 1) % around.size()];
              a = b;
              b = c;
            }
          if (a != u or b != v)
            return "a face does not close";
        }

    std::map<int, int> nodes_of, edges_of;
    for (int u = 0; u < n; ++u)
      {
        ++nodes_of[comp[size_t(u)]];
        for (int v = u + 1; v < n; ++v)
          edges_of[comp[size_t(u)]] += adj[size_t(u)][size_t(v)];
      }
    size_t with_edges = 0;
    for (const auto & [c, e] : edges_of)
      {
        if (e == 0)
          continue;
        ++with_edges;
        if (nodes_of[c] - e + faces_of[c] != 2)
          return "Euler's formula fails in a component: " + std::to_string(nodes_of[c])
            + " nodes, " + std::to_string(e) + " edges, " + std::to_string(faces_of[c]) + " faces";
      }
    faces = faces - with_edges + 1;
    return "";
  }

  /** What is wrong with the certificate of r as a Kuratowski subdivision
      of the simple graph adj: its paths join pairs of branch nodes, as
      the edges of K5 or of K3,3, along edges of adj, without sharing
      nodes but their ends; or, for a minimal obstruction, its edges are
      those of a graph that is not planar. Empty if nothing. */
  template <class GT>
  std::string certificate_error(const Planarity_Test_Result<GT> & r, const Adjacency & adj)
  {
    const int n = int(adj.size());
    if (not r.has_nonplanar_certificate)
      return "no certificate";

    if (r.certificate_type == Planarity_Certificate_Type::Minimal_NonPlanar_Obstruction)
      {
        Adjacency sub(size_t(n), std::vector<bool>(size_t(n), false));
        for (size_t i = 0; i < r.certificate_obstruction_edges.size(); ++i)
          {
            const int u = num(r.certificate_obstruction_edges[i].src);
            const int v = num(r.certificate_obstruction_edges[i].tgt);
            if (not adj[size_t(u)][size_t(v)])
              return "an edge of the obstruction is not of the graph";
            sub[size_t(u)][size_t(v)] = sub[size_t(v)][size_t(u)] = true;
          }
        return planar(sub) ? "the obstruction is planar" : "";
      }

    const bool k5 = r.certificate_type == Planarity_Certificate_Type::K5_Subdivision;
    if (not k5 and r.certificate_type != Planarity_Certificate_Type::K33_Subdivision)
      return std::string("a certificate of type ") + to_string(r.certificate_type);
    std::set<int> branch;
    for (size_t i = 0; i < r.certificate_branch_nodes.size(); ++i)
      branch.insert(num(r.certificate_branch_nodes[i]));
    if (branch.size() != (k5 ? 5U : 6U) or r.certificate_paths.size() != (k5 ? 10U : 9U))
      return "the certificate has " + std::to_string(branch.size()) + " branch nodes and "
        + std::to_string(r.certificate_paths.size()) + " paths";

    std::set<int> inner;
    std::set<std::pair<int, int>> joined;
    std::map<int, int> degree;
    for (size_t i = 0; i < r.certificate_paths.size(); ++i)
      {
        const auto & p = r.certificate_paths[i];
        if (p.nodes.size() < 2)
          return "a path of less than two nodes";
        const int a = num(p.nodes[0]), b = num(p.nodes[p.nodes.size() - 1]);
        if (not branch.count(a) or not branch.count(b) or a == b
            or not joined.insert({std::min(a, b), std::max(a, b)}).second)
          return "a path does not join two branch nodes, or joins them twice";
        ++degree[a];
        ++degree[b];
        for (size_t j = 0; j + 1 < p.nodes.size(); ++j)
          if (not adj[size_t(num(p.nodes[j]))][size_t(num(p.nodes[j + 1]))])
            return "a path takes an edge that is not of the graph";
        for (size_t j = 1; j + 1 < p.nodes.size(); ++j)
          if (branch.count(num(p.nodes[j])) or not inner.insert(num(p.nodes[j])).second)
            return "two paths share an inner node";
      }

    if (k5)
      return ""; // 10 distinct pairs of 5 nodes: all of them
    // The 9 pairs are those of K3,3: every branch node in 3, two sides
    for (const auto & [x, d] : degree)
      if (d != 3)
        return "a branch node of K3,3 is in " + std::to_string(d) + " paths";
    std::map<int, int> side;
    side[*branch.begin()] = 0;
    for (bool grew = true; grew; )
      {
        grew = false;
        for (const auto & [a, b] : joined)
          for (const auto & [x, y] : {std::pair(a, b), std::pair(b, a)})
            if (side.count(x) and not side.count(y))
              {
                side[y] = 1 - side[x];
                grew = true;
              }
      }
    for (const auto & [a, b] : joined)
      if (side[a] == side[b])
        return "the paths of K3,3 join two nodes of one side";
    return "";
  }

  /// Runs the planarity test on s hiding the arcs of hidden, with its
  /// embedding or its certificate
  template <class GT>
  void check_planarity(const Spec & s, unsigned long hidden, Check & answer,
                       Check & embedding, Check & certificate, Check & counts)
  {
    using Filter = Hide_Mask<GT>;
    Built<GT> b(s);
    const auto adj = simple_adjacency(s, hidden);
    const bool want = planar(adj);
    const std::string at = hidden == 0 ? "" : "hiding " + std::to_string(hidden) + ": ";

    answer.run();
    const bool got = is_planar_graph<GT, Filter>(b.g, Filter{hidden});
    if (got != want)
      answer.fail(s, at + (got ? "says planar" : "says not planar"));

    Planarity_Test_Options options;
    options.compute_embedding = true;
    options.compute_nonplanar_certificate = true;
    const auto r = planarity_test<GT, Filter>(b.g, Filter{hidden}, options);

    // What it says of the input
    counts.run();
    size_t loops = 0, shown = 0;
    for (size_t i = 0; i < s.arcs.size(); ++i)
      if (graph_filters::shown(hidden, int(i)))
        {
          ++shown;
          loops += s.arcs[i][0] == s.arcs[i][1];
        }
    if (r.is_planar != want or r.num_nodes != size_t(s.n) or r.num_input_arcs != shown
        or r.simplified_num_edges != num_edges(adj) or r.ignored_loops != loops
        or r.ignored_parallel_arcs != shown - loops - num_edges(adj)
        or r.input_is_digraph != b.g.is_digraph())
      counts.fail(s, at + "wrong counts of the input");

    if (want)
      {
        embedding.run();
        size_t faces = 0;
        if (not r.has_combinatorial_embedding)
          embedding.fail(s, at + "no embedding");
        else if (auto e = embedding_error(r, adj, faces); not e.empty())
          embedding.fail(s, at + e);
        // The graph without nodes has no faces, though 1 + C gives one
        else if (r.embedding_num_faces != (s.n == 0 ? 0 : faces))
          embedding.fail(s, at + "it says " + std::to_string(r.embedding_num_faces)
                         + " faces, and the rotation gives " + std::to_string(faces));
      }
    else
      {
        certificate.run();
        if (auto e = certificate_error(r, adj); not e.empty())
          certificate.fail(s, at + e);
      }
  }
}

template <class GT>
class ExhaustivePlanarity : public ::testing::Test {};

TYPED_TEST_SUITE(ExhaustivePlanarity, All_Kinds);

TYPED_TEST(ExhaustivePlanarity, SimpleGraphsMatchKuratowski)
{
  using GT = TypeParam;
  Check answer("is_planar_graph()");
  Check embedding("the embedding of a planar graph");
  Check certificate("the certificate of a graph that is not planar");
  Check counts("the counts of Planarity_Test_Result");

  // Every simple graph of up to 5 nodes; a digraph takes each pair once,
  // from the smaller node
  for_each_simple_graph(0, 5, false, [&] (const Spec & s)
    {
      check_planarity<GT>(s, 0, answer, embedding, certificate, counts);
    });

  // Simple graphs of 6 nodes and 8 edges or more, where K3,3 and the
  // split K5 are: a sample of them, or all with heavy
  std::mt19937 rng(seed(733));
  for_each_simple_graph(6, 6, false, [&] (const Spec & s)
    {
      if (s.num_arcs() >= 8 and (heavy() or rng() % 48 == 0))
        check_planarity<GT>(s, 0, answer, embedding, certificate, counts);
    });

  answer.expect();
  embedding.expect();
  certificate.expect();
  counts.expect();
}

TYPED_TEST(ExhaustivePlanarity, MultigraphsAndHiddenArcsMatchKuratowski)
{
  using GT = TypeParam;
  const bool digraph = GT().is_digraph();
  Check answer("is_planar_graph()");
  Check embedding("the embedding of a planar graph");
  Check certificate("the certificate of a graph that is not planar");
  Check counts("the counts of Planarity_Test_Result");

  // Every graph of 4 nodes or less is planar: loops, parallel arcs and
  // hidden arcs are what these are for
  for_each_spec(domain(3, 3), digraph, [&] (const Spec & s)
    {
      for (unsigned long hidden = 0; hidden < (1UL << s.num_arcs()); ++hidden)
        check_planarity<GT>(s, hidden, answer, embedding, certificate, counts);
    });

  // K5 and K3,3 with each set of hidden arcs: hiding one makes them planar
  Spec k5, k33;
  k5.n = 5;
  for (int u = 0; u < 5; ++u)
    for (int v = u + 1; v < 5; ++v)
      k5.arcs.push_back({u, v});
  k33.n = 6;
  for (int u = 0; u < 3; ++u)
    for (int v = 3; v < 6; ++v)
      k33.arcs.push_back({u, v});
  for (const Spec * s : {&k5, &k33})
    for (unsigned long hidden = 0; hidden < (1UL << s->num_arcs()); hidden += hidden < 64 ? 1 : 37)
      check_planarity<GT>(*s, hidden, answer, embedding, certificate, counts);

  answer.expect();
  embedding.expect();
  certificate.expect();
  counts.expect();
}
