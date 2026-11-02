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
 * @file graph_exhaustive_structure_test.cc
 * @brief The structure of every small multigraph, on the six kinds of
 *        graph, against a model of its lists.
 *
 * The model of a spec is what each kind of graph keeps: a node holds in
 * its list the arcs that leave it and, in a graph that is not directed,
 * the arcs that enter it from another node; a loop is in the list of its
 * node once. The tests compare with that model the incidence of every
 * multigraph of the domain of graph_harness.H: the counts, the iterators
 * of the graph and of a node, in both directions and with each set of
 * hidden arcs, the arcs in and out, through every form and through
 * In_Arcs_Index, also on a graph of an undirected kind that says that it
 * is directed, the searches of arcs, the graph left by each removal, by
 * each arc disconnected and connected again, by clear_graph() and by the
 * sorts, the copies, and the paths along every short walk.
 */

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_graph.H>
#include <single_graph.H>

#include "graph_filter_catalog.H"
#include "graph_harness.H"
#include "graph_oracles.H"

using namespace Aleph;
using namespace graph_harness;
using graph_filters::Hide_Mask;

namespace
{
  std::string text(const std::multiset<int> & s)
  {
    std::string r = "{";
    for (int x : s)
      r += (r.size() > 1 ? " " : "") + std::to_string(x);
    return r + "}";
  }

  std::string text(const std::vector<int> & v)
  {
    std::string r = "(";
    for (size_t i = 0; i < v.size(); ++i)
      r += (i == 0 ? "" : " ") + std::to_string(v[i]);
    return r + ")";
  }

  /// Which nodes and arcs of a spec are still in the graph
  struct Alive
  {
    std::vector<bool> nodes;
    std::vector<bool> arcs;

    explicit Alive(const Spec & s)
      : nodes(size_t(s.n), true), arcs(s.arcs.size(), true) {}

    /// Takes out the node x and its arcs
    void remove_node(const Spec & s, int x)
    {
      nodes[size_t(x)] = false;
      for (size_t i = 0; i < s.arcs.size(); ++i)
        if (s.arcs[i][0] == x or s.arcs[i][1] == x)
          arcs[i] = false;
    }
  };

  /// The arcs, by number, in the list of x in the model
  std::multiset<int> model_list(const Spec & s, const Alive & alive, int x, bool digraph)
  {
    std::multiset<int> r;
    for (size_t i = 0; i < s.arcs.size(); ++i)
      if (alive.arcs[i] and (s.arcs[i][0] == x or (not digraph and s.arcs[i][1] == x)))
        r.insert(int(i));
    return r;
  }

  /// The arcs, by number, whose end e (0: source, 1: target) is x
  std::multiset<int> model_end(const Spec & s, const Alive & alive, int x, int e)
  {
    std::multiset<int> r;
    for (size_t i = 0; i < s.arcs.size(); ++i)
      if (alive.arcs[i] and s.arcs[i][size_t(e)] == x)
        r.insert(int(i));
    return r;
  }

  /// The other end of arc i from x
  int other_end(const Spec & s, int i, int x)
  {
    return s.arcs[size_t(i)][0] == x ? s.arcs[size_t(i)][1] : s.arcs[size_t(i)][0];
  }

  /// The arc numbers that an iterator gives from where it is
  template <class It>
  std::vector<int> forward(It it)
  {
    std::vector<int> r;
    for (; it.has_curr(); it.next())
      r.push_back(num(it.get_curr()));
    return r;
  }

  /** What is wrong with the incidence of g against the model of the
      nodes and arcs of s that alive says: the counts, the nodes and arcs
      of the graph with their ends, and the list of each node, through
      get_num_arcs(p), Node_Arc_Iterator and get_connected_node(); empty
      if nothing. The nodes and arcs are found by their infos, so that g
      may be a copy. */
  template <class GT>
  std::string incidence_error(const GT & g, const Spec & s, const Alive & alive)
  {
    const bool digraph = g.is_digraph();
    const size_t n = size_t(std::count(alive.nodes.begin(), alive.nodes.end(), true));
    const size_t m = size_t(std::count(alive.arcs.begin(), alive.arcs.end(), true));
    if (g.get_num_nodes() != n or g.get_num_arcs() != m)
      return std::to_string(g.get_num_nodes()) + " nodes and " + std::to_string(g.get_num_arcs())
        + " arcs, not " + std::to_string(n) + " and " + std::to_string(m);

    std::vector<typename GT::Node *> node(size_t(s.n), nullptr);
    size_t seen = 0;
    for (auto it = g.get_node_it(); it.has_curr(); it.next_ne(), ++seen)
      {
        auto * p = it.get_curr();
        const int x = num(p);
        if (x < 0 or x >= s.n or not alive.nodes[size_t(x)] or node[size_t(x)] != nullptr)
          return "the node " + std::to_string(x) + " is not of the graph, or is twice";
        node[size_t(x)] = p;
      }
    if (seen != n)
      return "the node iterator gives " + std::to_string(seen) + " nodes";

    std::vector<bool> arc_seen(s.arcs.size(), false);
    for (auto it = g.get_arc_it(); it.has_curr(); it.next_ne())
      {
        auto * a = it.get_curr();
        const int i = num(a);
        if (i < 0 or size_t(i) >= s.arcs.size() or not alive.arcs[size_t(i)] or arc_seen[size_t(i)])
          return "the arc " + std::to_string(i) + " is not of the graph, or is twice";
        arc_seen[size_t(i)] = true;
        if (g.get_src_node(a) != node[size_t(s.arcs[size_t(i)][0])]
            or g.get_tgt_node(a) != node[size_t(s.arcs[size_t(i)][1])])
          return "the ends of the arc " + std::to_string(i) + " are wrong";
      }
    if (std::count(arc_seen.begin(), arc_seen.end(), true) != std::ptrdiff_t(m))
      return "the arc iterator misses arcs";

    for (int x = 0; x < s.n; ++x)
      {
        if (not alive.nodes[size_t(x)])
          continue;
        auto * p = node[size_t(x)];
        const auto want = model_list(s, alive, x, digraph);
        if (g.get_num_arcs(p) != want.size())
          return "get_num_arcs(" + std::to_string(x) + ") is " + std::to_string(g.get_num_arcs(p))
            + ", not " + std::to_string(want.size());
        std::multiset<int> got;
        for (typename GT::Node_Arc_Iterator it(p); it.has_curr(); it.next_ne())
          {
            auto * a = it.get_curr();
            got.insert(num(a));
            const int i = num(a);
            if (num(g.get_connected_node(a, p)) != other_end(s, i, x)
                or num(it.get_tgt_node_ne()) != other_end(s, i, x))
              return "the arc " + std::to_string(i) + " from " + std::to_string(x)
                + " goes to the wrong node";
          }
        if (got != want)
          return "the list of " + std::to_string(x) + " is " + text(got) + ", not " + text(want);
      }
    return "";
  }
}

template <class GT>
class ExhaustiveStructure : public ::testing::Test {};

TYPED_TEST_SUITE(ExhaustiveStructure, All_Kinds);

TYPED_TEST(ExhaustiveStructure, IncidenceMatchesTheModel)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  using Arc = typename GT::Arc;
  const bool digraph = GT().is_digraph();
  Check incidence("the counts, the iterators and the lists of the nodes");
  Check out("Out_Iterator and the out_ methods");
  Check in("In_Iterator(g, p) and the in_ methods");
  Check in_free("GT::In_Iterator(g, p) and the in_ functions that take the graph");
  Check in_list("In_Iterator(p) and the in_ functions of the node, if not directed");
  Check index("In_Arcs_Index");
  Check search("search_arc(u, v)");
  Check directed("search_directed_arc(u, v)");

  for_each_spec(domain(4, 3), digraph, [&] (const Spec & s)
    {
      Built<GT> b(s);
      const Alive alive(s);
      const In_Arcs_Index<GT> in_arcs_index(b.g);

      incidence.run();
      if (auto e = incidence_error(b.g, s, alive); not e.empty())
        incidence.fail(s, e);

      for (int x = 0; x < s.n; ++x)
        {
          Node * p = b.nodes[size_t(x)];
          const std::string at = "at " + std::to_string(x) + ": ";

          // The arcs out and in of a node, and the nodes at their other
          // ends, through the iterator and through every method
          auto check_end = [&] (Check & check, int e, auto && it_arcs, const DynList<Arc *> & arcs,
                                const DynList<Node *> & nodes, const auto & pairs, size_t degree,
                                size_t traversed, bool all_ok)
            {
              check.run();
              const auto want = model_end(s, alive, x, e);
              std::multiset<int> want_nodes;
              for (int i : want)
                want_nodes.insert(s.arcs[size_t(i)][size_t(1 - e)]);
              std::multiset<int> got_it(it_arcs.begin(), it_arcs.end()), got_arcs, got_nodes,
                pair_arcs, pair_nodes;
              for (auto it = arcs.get_it(); it.has_curr(); it.next_ne())
                got_arcs.insert(num(it.get_curr()));
              for (auto it = nodes.get_it(); it.has_curr(); it.next_ne())
                got_nodes.insert(num(it.get_curr()));
              for (auto it = pairs.get_it(); it.has_curr(); it.next_ne())
                {
                  pair_arcs.insert(num(std::get<0>(it.get_curr())));
                  pair_nodes.insert(num(std::get<1>(it.get_curr())));
                }
              if (got_it != want)
                check.fail(s, at + "the iterator gives " + text(got_it) + ", not " + text(want));
              else if (got_arcs != want or pair_arcs != want)
                check.fail(s, at + "the arcs are " + text(got_arcs) + ", not " + text(want));
              else if (got_nodes != want_nodes or pair_nodes != want_nodes)
                check.fail(s, at + "the nodes are " + text(got_nodes) + ", not " + text(want_nodes));
              else if (degree != want.size() or traversed != want.size() or not all_ok)
                check.fail(s, at + "the degree is " + std::to_string(degree) + ", not "
                           + std::to_string(want.size()));
            };

          {
            size_t traversed = 0;
            b.g.for_each_out_arc(p, [&] (Arc *) { ++traversed; });
            const bool all_ok = b.g.all_out_arcs(p, [&] (Arc * a) { return b.g.get_src_node(a) == p; });
            check_end(out, 0, forward(Out_Iterator<GT>(p)), b.g.out_arcs(p), b.g.out_nodes(p),
                      b.g.out_pairs(p), b.g.out_degree(p), traversed, all_ok);
          }
          auto into_p = [&] (Arc * a) { return b.g.get_tgt_node(a) == p; };
          {
            size_t traversed = 0;
            b.g.for_each_in_arc(p, [&] (Arc *) { ++traversed; });
            const bool all_ok = b.g.all_in_arcs(p, into_p);
            check_end(in, 1, forward(In_Iterator<GT>(b.g, p)), b.g.in_arcs(p), b.g.in_nodes(p),
                      b.g.in_pairs(p), b.g.in_degree(p), traversed, all_ok);
          }
          {
            size_t traversed = 0;
            for_each_in_arc(b.g, p, [&] (Arc *) { ++traversed; });
            const bool all_ok = all_in_arc(b.g, p, into_p) and traverse_in_arcs(b.g, p, into_p)
              and not exists_in_arc(b.g, p, [&] (Arc * a) { return not into_p(a); });
            check_end(in_free, 1, forward(typename GT::In_Iterator(b.g, p)), in_arcs(b.g, p),
                      in_nodes(b.g, p), in_pairs(b.g, p), in_degree(b.g, p), traversed, all_ok);
          }
          if constexpr (not DigraphType<GT>)
            {
              size_t traversed = 0;
              for_each_in_arc<GT>(p, [&] (Arc *) { ++traversed; });
              const bool all_ok = all_in_arc<GT>(p, into_p) and traverse_in_arcs<GT>(p, into_p);
              check_end(in_list, 1, forward(In_Iterator<GT>(p)), in_arcs<GT>(p), in_nodes<GT>(p),
                        in_pairs<GT>(p), in_degree<GT>(p), traversed, all_ok);
            }

          index.run();
          {
            const auto want = model_end(s, alive, x, 1);
            const auto got = forward(in_arcs_index.get_it(p));
            if (std::multiset<int>(got.begin(), got.end()) != want
                or in_arcs_index.in_degree(p) != want.size())
              index.fail(s, at + "gives " + text(got) + ", not " + text(want));
            else
              for (auto it = in_arcs_index.get_it(p); it.has_curr(); it.next_ne())
                if (it.get_node_ne() != b.g.get_src_node(it.get_curr_ne()))
                  {
                    index.fail(s, at + "the node of an arc is not its source");
                    break;
                  }
          }

          for (int y = 0; y < s.n; ++y)
            {
              Node * q = b.nodes[size_t(y)];
              bool joined = false, from_to = false;
              for (const auto & a : s.arcs)
                {
                  from_to = from_to or (a[0] == x and a[1] == y);
                  joined = joined or (a[0] == x and a[1] == y) or (a[0] == y and a[1] == x);
                }

              search.run();
              {
                Arc * a = b.g.search_arc(p, q);
                const bool want = digraph ? from_to : joined;
                if ((a != nullptr) != want)
                  search.fail(s, at + "to " + std::to_string(y) + (want ? " finds no arc" : " finds an arc"));
                else if (a != nullptr and not ((b.g.get_src_node(a) == p and b.g.get_tgt_node(a) == q)
                                               or (not digraph and b.g.get_src_node(a) == q
                                                   and b.g.get_tgt_node(a) == p)))
                  search.fail(s, at + "to " + std::to_string(y) + " finds an arc between other nodes");
              }

              directed.run();
              {
                Arc * a = b.g.search_directed_arc(p, q);
                if ((a != nullptr) != from_to)
                  directed.fail(s, at + "to " + std::to_string(y)
                                + (from_to ? " finds no arc" : " finds an arc"));
                else if (a != nullptr and (b.g.get_src_node(a) != p or b.g.get_tgt_node(a) != q))
                  directed.fail(s, at + "to " + std::to_string(y) + " finds an arc of other ends");
              }
            }
        }
    });

  incidence.expect();
  out.expect();
  in.expect();
  in_free.expect();
  in_list.expect();
  index.expect();
  search.expect();
  directed.expect();
}

namespace
{
  /** Walks it, an iterator on the arcs of the node x with the filter of
      the mask hidden: forward from its construction, again after
      reset_first(), and backward from reset_last() through prev(), where
      the iterator goes back. The three must give the arcs of the model
      that the filter shows, the last in the reverse order: those of the
      list of x if e < 0, or those whose end e (0: source, 1: target) is
      x. */
  template <class GT, class It>
  std::string walk_error(const Spec & s, Built<GT> & b, int x, int e, unsigned long hidden, It it)
  {
    const bool digraph = b.g.is_digraph();
    const Alive alive(s);
    std::multiset<int> want = e < 0 ? model_list(s, alive, x, digraph) : model_end(s, alive, x, e);
    for (auto i = want.begin(); i != want.end(); )
      i = graph_filters::shown(hidden, *i) ? std::next(i) : want.erase(i);

    const auto first = forward(it); // on a copy
    if (std::multiset<int>(first.begin(), first.end()) != want)
      return "forward it gives " + text(first) + ", not " + text(want);

    while (it.has_curr()) // off its end, as a loop leaves it
      it.next();
    it.reset_first();
    if (const auto again = forward(it); again != first)
      return "after reset_first() it gives " + text(again) + ", not " + text(first);

    // The iterators of List_SGraph and List_SDigraph go only forward: they
    // have no reset_last() nor prev()
    if constexpr (requires { it.reset_last(); it.prev(); })
      {
        std::vector<int> back;
        for (it.reset_last(); it.has_curr(); it.prev())
          back.push_back(num(it.get_curr()));
        std::reverse(back.begin(), back.end());
        if (back != first)
          return "backward it gives " + text(back) + " reversed, not " + text(first);
      }
    return "";
  }

  /// walk_error() on an iterator of kind It, built on the node x with the
  /// filter of the mask hidden
  template <class GT, template <class, class> class It>
  std::string iterator_error(const Spec & s, Built<GT> & b, int x, int e, unsigned long hidden)
  {
    using Filter = Hide_Mask<GT>;
    return walk_error(s, b, x, e, hidden, It<GT, Filter>(b.nodes[size_t(x)], Filter{hidden}));
  }
}

TYPED_TEST(ExhaustiveStructure, IteratorsWalkBothWaysWithTheFilter)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  using Filter = Hide_Mask<GT>;
  const bool digraph = GT().is_digraph();
  Check node_arc("Node_Arc_Iterator");
  Check out("Out_Iterator");
  Check in("In_Iterator(g, p)");
  Check in_list("In_Iterator(p), if not directed");
  Check index("In_Arcs_Index::Iterator");

  for_each_spec(domain(3, 3), digraph, [&] (const Spec & s)
    {
      Built<GT> b(s);
      for (unsigned long hidden = 0; hidden < (1UL << s.num_arcs()); ++hidden)
        {
          const In_Arcs_Index<GT, Filter> in_arcs_index(b.g, Filter{hidden});
          for (int x = 0; x < s.n; ++x)
            {
              Node * p = b.nodes[size_t(x)];
              const std::string at = "at " + std::to_string(x) + " hiding "
                + std::to_string(hidden) + ": ";
              node_arc.run();
              if (auto e = iterator_error<GT, Node_Arc_Iterator>(s, b, x, -1, hidden);
                  not e.empty())
                node_arc.fail(s, at + e);
              out.run();
              if (auto e = iterator_error<GT, Out_Iterator>(s, b, x, 0, hidden); not e.empty())
                out.fail(s, at + e);
              in.run();
              In_Iterator<GT, Filter> in_it(b.g, p, Filter{hidden});
              if (auto e = walk_error(s, b, x, 1, hidden, in_it); not e.empty())
                in.fail(s, at + e);
              if constexpr (not DigraphType<GT>)
                {
                  in_list.run();
                  if (auto e = iterator_error<GT, In_Iterator>(s, b, x, 1, hidden); not e.empty())
                    in_list.fail(s, at + e);
                }
              index.run();
              if (auto e = walk_error(s, b, x, 1, hidden, in_arcs_index.get_it(p)); not e.empty())
                index.fail(s, at + e);
            }
        }
    });

  node_arc.expect();
  out.expect();
  in.expect();
  in_list.expect();
  index.expect();
}

TYPED_TEST(ExhaustiveStructure, InArcsOfAGraphThatSaysThatItIsDirected)
{
  using GT = TypeParam;
  using Arc = typename GT::Arc;
  // A graph of an undirected kind, flipped with set_digraph(true) before
  // its arcs, keeps each arc only in the list of its source: the forms
  // that take the graph tell it at run time, and In_Iterator(g, p), which
  // would walk the list of p, refuses it
  Check lists("the counts, the iterators and the lists of the nodes");
  Check in("the in_ methods and the in_ functions that take the graph");
  Check index("In_Arcs_Index");
  Check refused("In_Iterator(g, p) and GT::In_Iterator(g, p) throw domain_error");

  if constexpr (not DigraphType<GT>)
    for_each_spec(domain(4, 3), true, [&] (const Spec & s)
      {
        GT g;
        g.set_digraph(true);
        std::vector<typename GT::Node *> node;
        for (int i = 0; i < s.n; ++i)
          node.push_back(g.insert_node(i));
        for (size_t i = 0; i < s.arcs.size(); ++i)
          g.insert_arc(node[size_t(s.arcs[i][0])], node[size_t(s.arcs[i][1])], int(i));
        const Alive alive(s);

        lists.run();
        if (auto e = incidence_error(g, s, alive); not e.empty())
          lists.fail(s, e);

        const In_Arcs_Index<GT> in_arcs_index(g);
        for (int x = 0; x < s.n; ++x)
          {
            auto * p = node[size_t(x)];
            const std::string at = "at " + std::to_string(x) + ": ";
            const auto want = model_end(s, alive, x, 1);
            auto numbers = [] (const DynList<Arc *> & l)
              {
                std::multiset<int> r;
                for (auto it = l.get_it(); it.has_curr(); it.next_ne())
                  r.insert(num(it.get_curr()));
                return r;
              };

            in.run();
            size_t traversed = 0;
            for_each_in_arc(g, p, [&] (Arc *) { ++traversed; });
            if (numbers(g.in_arcs(p)) != want or numbers(in_arcs(g, p)) != want
                or g.in_degree(p) != want.size() or in_degree(g, p) != want.size()
                or in_nodes(g, p).size() != want.size() or g.in_nodes(p).size() != want.size()
                or traversed != want.size())
              in.fail(s, at + "the arcs into it are not " + text(want));

            index.run();
            const auto got = forward(in_arcs_index.get_it(p));
            if (std::multiset<int>(got.begin(), got.end()) != want
                or in_arcs_index.in_degree(p) != want.size())
              index.fail(s, at + "gives " + text(got) + ", not " + text(want));

            refused.run();
            bool by_free = false, by_member = false;
            try { (void) In_Iterator<GT>(g, p); }
            catch (const std::domain_error &) { by_free = true; }
            try { (void) typename GT::In_Iterator(g, p); }
            catch (const std::domain_error &) { by_member = true; }
            if (not by_free or not by_member)
              refused.fail(s, at + "an In_Iterator(g, p) walks the list");
          }
      });

  lists.expect();
  in.expect();
  index.expect();
  refused.expect();
}

TYPED_TEST(ExhaustiveStructure, RemovalsMatchTheModel)
{
  using GT = TypeParam;
  const bool digraph = GT().is_digraph();
  Check arcs("remove_arc()");
  Check nodes("remove_node()");
  Check all("removing every node");

  for_each_spec(domain(4, 3), digraph, [&] (const Spec & s)
    {
      for (size_t i = 0; i < s.arcs.size(); ++i)
        {
          arcs.run();
          Built<GT> b(s);
          b.g.remove_arc(b.arcs[i]);
          Alive alive(s);
          alive.arcs[i] = false;
          if (auto e = incidence_error(b.g, s, alive); not e.empty())
            arcs.fail(s, "without the arc " + std::to_string(i) + ": " + e);
        }

      for (int x = 0; x < s.n; ++x)
        {
          nodes.run();
          Built<GT> b(s);
          b.g.remove_node(b.nodes[size_t(x)]);
          Alive alive(s);
          alive.remove_node(s, x);
          if (auto e = incidence_error(b.g, s, alive); not e.empty())
            nodes.fail(s, "without the node " + std::to_string(x) + ": " + e);
        }

      // Every node, in the order of the spec, checked after each removal
      all.run();
      Built<GT> b(s);
      Alive alive(s);
      for (int x = 0; x < s.n; ++x)
        {
          b.g.remove_node(b.nodes[size_t(x)]);
          alive.remove_node(s, x);
          if (auto e = incidence_error(b.g, s, alive); not e.empty())
            {
              all.fail(s, "after the node " + std::to_string(x) + ": " + e);
              break;
            }
        }
    });

  arcs.expect();
  nodes.expect();
  all.expect();
}

TYPED_TEST(ExhaustiveStructure, CopiesMatchTheModel)
{
  using GT = TypeParam;
  const bool digraph = GT().is_digraph();
  Check copy("the copy constructor");
  Check assign("the copy assignment");
  Check move("the move constructor and assignment");
  Check swap("swap()");
  Check mapped("copy_graph() with the cookies");

  // A graph to be overwritten, with a node and a loop
  auto other = []
    {
      GT g;
      auto * p = g.insert_node(-1);
      g.insert_arc(p, p, -1);
      return g;
    };

  for_each_spec(domain(4, 3), digraph, [&] (const Spec & s)
    {
      Built<GT> b(s);
      const Alive alive(s);
      const Spec none{};
      const Alive nothing(none);

      copy.run();
      {
        const GT c(b.g);
        if (auto e = incidence_error(c, s, alive); not e.empty())
          copy.fail(s, "the copy: " + e);
        else if (auto e2 = incidence_error(b.g, s, alive); not e2.empty())
          copy.fail(s, "the original: " + e2);
      }

      assign.run();
      {
        GT c = other();
        c = b.g;
        if (auto e = incidence_error(c, s, alive); not e.empty())
          assign.fail(s, "the copy: " + e);
        else if (auto e2 = incidence_error(b.g, s, alive); not e2.empty())
          assign.fail(s, "the original: " + e2);
      }

      move.run();
      {
        GT c(b.g);
        GT moved(std::move(c));
        GT assigned = other();
        assigned = std::move(moved);
        if (auto e = incidence_error(assigned, s, alive); not e.empty())
          move.fail(s, "the result: " + e);
        else
          {
            // What was moved from is still a graph
            auto * p = moved.insert_node(0);
            moved.insert_arc(p, p, 0);
            if (moved.get_num_nodes() < 1 or moved.get_num_arcs() < 1)
              move.fail(s, "a graph moved from does not take nodes");
          }
      }

      swap.run();
      {
        GT c(b.g), d;
        c.swap(d);
        if (auto e = incidence_error(d, s, alive); not e.empty())
          swap.fail(s, "the swapped graph: " + e);
        else if (auto e2 = incidence_error(c, none, nothing); not e2.empty())
          swap.fail(s, "the empty one: " + e2);
      }

      mapped.run();
      {
        GT c = other();
        copy_graph(c, b.g, true);
        bool ok = incidence_error(c, s, alive).empty();
        for (auto it = b.g.get_node_it(); ok and it.has_curr(); it.next_ne())
          {
            auto * p = it.get_curr();
            auto * q = mapped_node<GT>(p);
            ok = q != nullptr and num(q) == num(p) and mapped_node<GT>(q) == p;
          }
        for (auto it = b.g.get_arc_it(); ok and it.has_curr(); it.next_ne())
          {
            auto * a = it.get_curr();
            auto * ca = mapped_arc<GT>(a);
            ok = ca != nullptr and num(ca) == num(a) and mapped_arc<GT>(ca) == a;
          }
        if (not ok)
          mapped.fail(s, "a wrong copy or map");
      }
    });

  copy.expect();
  assign.expect();
  move.expect();
  swap.expect();
  mapped.expect();
}

namespace
{
  /// A walk of a spec: its start and its arcs, each taken from the end
  /// where the walk is (from its source only if directed)
  struct Spec_Walk
  {
    int start;
    std::vector<int> arcs;
    std::vector<int> nodes; // start, then the node after each arc
  };

  /// Calls f with every walk of s of up to len arcs
  template <class F>
  void for_each_walk(const Spec & s, size_t len, bool directed, F && f)
  {
    std::vector<Spec_Walk> todo;
    for (int x = 0; x < s.n; ++x)
      todo.push_back({x, {}, {x}});
    while (not todo.empty())
      {
        Spec_Walk w = todo.back();
        todo.pop_back();
        f(static_cast<const Spec_Walk &>(w));
        if (w.arcs.size() == len)
          continue;
        const int at = w.nodes.back();
        for (size_t i = 0; i < s.arcs.size(); ++i)
          for (int e = 0; e < (directed ? 1 : 2); ++e)
            {
              const auto & a = s.arcs[i];
              if (a[size_t(e)] != at or (e == 1 and a[0] == a[1]))
                continue;
              Spec_Walk next = w;
              next.arcs.push_back(int(i));
              next.nodes.push_back(a[size_t(1 - e)]);
              todo.push_back(std::move(next));
            }
      }
  }

  /// The arc of a graph whose number is i
  template <class GT>
  typename GT::Arc * arc_of(const GT & g, int i)
  {
    for (auto it = g.get_arc_it(); it.has_curr(); it.next_ne())
      if (num(it.get_curr()) == i)
        return it.get_curr();
    return nullptr;
  }

  template <class GT>
  std::vector<int> path_nodes(const Path<GT> & p)
  {
    std::vector<int> r;
    for (typename Path<GT>::Iterator it(p); it.has_current_node(); it.next_ne())
      r.push_back(num(it.get_current_node_ne()));
    return r;
  }

  template <class GT>
  std::vector<int> path_arcs(const Path<GT> & p)
  {
    std::vector<int> r;
    for (typename Path<GT>::Iterator it(p); it.has_current_arc(); it.next_ne())
      r.push_back(num(it.get_current_arc_ne()));
    return r;
  }

  /// What is wrong with p as the walk w; empty if nothing. When the path
  /// was built from the nodes, any arc between two of them does.
  template <class GT>
  std::string path_error(const Spec & s, const Path<GT> & p, const Spec_Walk & w,
                         bool any_arc, bool directed)
  {
    const auto nodes = path_nodes(p);
    const auto arcs = path_arcs(p);
    if (nodes != w.nodes)
      return "the nodes are " + text(nodes) + ", not " + text(w.nodes);
    if (arcs.size() != w.arcs.size() or p.size() != w.nodes.size())
      return "the path has " + std::to_string(arcs.size()) + " arcs";
    for (size_t i = 0; i < arcs.size(); ++i)
      {
        if (not any_arc and arcs[i] != w.arcs[i])
          return "the arcs are " + text(arcs) + ", not " + text(w.arcs);
        const auto & a = s.arcs[size_t(arcs[i])];
        const bool along = a[0] == w.nodes[i] and a[1] == w.nodes[i + 1];
        const bool against = a[1] == w.nodes[i] and a[0] == w.nodes[i + 1];
        if (not along and (directed or not against))
          return "the arc " + std::to_string(arcs[i]) + " does not join "
            + std::to_string(w.nodes[i]) + " and " + std::to_string(w.nodes[i + 1]);
      }
    if (num(p.get_first_node()) != w.nodes.front() or num(p.get_last_node()) != w.nodes.back())
      return "wrong first or last node";
    if (not arcs.empty() and (num(p.get_first_arc()) != arcs.front()
                              or num(p.get_last_arc()) != arcs.back()))
      return "wrong first or last arc";
    if (p.is_cycle() != (w.nodes.front() == w.nodes.back()))
      return "is_cycle() is wrong";

    // The other ways to read the path
    const GT & g = p.get_graph();
    std::vector<int> listed_nodes, listed_arcs;
    const auto node_list = p.nodes();
    const auto arc_list = p.arcs();
    for (auto it = node_list.get_it(); it.has_curr(); it.next_ne())
      listed_nodes.push_back(num(it.get_curr()));
    for (auto it = arc_list.get_it(); it.has_curr(); it.next_ne())
      listed_arcs.push_back(num(it.get_curr()));
    if (listed_nodes != nodes or listed_arcs != arcs)
      return "nodes() or arcs() differ from the iterator";
    for (int x : nodes)
      if (not p.contains_node(node_of(g, x)))
        return "contains_node() misses " + std::to_string(x);
    for (int i : arcs)
      if (not p.contains_arc(arc_of(g, i)))
        return "contains_arc() misses " + std::to_string(i);
    return "";
  }
}

TYPED_TEST(ExhaustiveStructure, PathsFollowEveryWalk)
{
  using GT = TypeParam;
  using Node = typename GT::Node;
  const bool digraph = GT().is_digraph();
  Check by_arcs("Path::append(Arc *)");
  Check by_nodes("Path::append(Node *)");
  Check ins_arcs("Path::insert(Arc *)");
  Check ins_nodes("Path::insert(Node *)");
  Check dir_arcs("Path::append_directed(Arc *) and insert_directed(Arc *)");
  Check dir_nodes("Path::append_directed(Node *) and insert_directed(Node *)");
  Check removals("Path::remove_last_node() and remove_first_node()");
  Check copies("the copies, moves and swaps of a path");
  Check check("Path::check()");
  Check check_directed("Path::check_directed()");

  for_each_spec(domain(3, 3), digraph, [&] (const Spec & s)
    {
      Built<GT> b(s);
      auto node = [&] (int x) -> Node * { return b.nodes[size_t(x)]; };

      // The walks along the arcs and, if not directed, against them
      for_each_walk(s, 3, digraph, [&] (const Spec_Walk & w)
        {
          by_arcs.run();
          Path<GT> p(b.g, node(w.start));
          for (int i : w.arcs)
            p.append(b.arcs[size_t(i)]);
          if (auto e = path_error(s, p, w, false, digraph); not e.empty())
            by_arcs.fail(s, e);

          by_nodes.run();
          Path<GT> q(b.g);
          for (int x : w.nodes)
            q.append(node(x));
          if (auto e = path_error(s, q, w, true, digraph); not e.empty())
            by_nodes.fail(s, e);

          ins_arcs.run();
          Path<GT> r(b.g, node(w.nodes.back()));
          for (size_t i = w.arcs.size(); i-- > 0; )
            r.insert(b.arcs[size_t(w.arcs[i])]);
          if (auto e = path_error(s, r, w, false, digraph); not e.empty())
            ins_arcs.fail(s, e);

          ins_nodes.run();
          Path<GT> t(b.g);
          for (size_t i = w.nodes.size(); i-- > 0; )
            t.insert(node(w.nodes[i]));
          if (auto e = path_error(s, t, w, true, digraph); not e.empty())
            ins_nodes.fail(s, e);

          check.run();
          if (not p.check())
            check.fail(s, "says that the path " + text(w.nodes) + " is not consistent");

          removals.run();
          {
            Path<GT> c = p;
            Spec_Walk rest = w;
            bool ok = true;
            while (ok and not rest.nodes.empty())
              {
                const bool last = rest.nodes.size() % 2 == 0;
                const int x = num(last ? c.remove_last_node() : c.remove_first_node());
                ok = x == (last ? rest.nodes.back() : rest.nodes.front());
                if (last)
                  {
                    rest.nodes.pop_back();
                    if (not rest.arcs.empty())
                      rest.arcs.pop_back();
                  }
                else
                  {
                    rest.nodes.erase(rest.nodes.begin());
                    if (not rest.arcs.empty())
                      rest.arcs.erase(rest.arcs.begin());
                  }
                ok = ok and (rest.nodes.empty() ? c.is_empty()
                             : path_error(s, c, rest, false, digraph).empty());
              }
            if (not ok)
              removals.fail(s, "taking out the nodes of " + text(w.nodes));
          }

          copies.run();
          {
            Path<GT> c(p), d(b.g);
            Path<GT> moved(std::move(c));
            d = moved;
            Path<GT> e(b.g);
            e.swap(d);
            if (not path_error(s, moved, w, false, digraph).empty()
                or not path_error(s, e, w, false, digraph).empty() or not d.is_empty())
              copies.fail(s, "of " + text(w.nodes));
          }

          // A walk against an arc is not a directed path
          check_directed.run();
          bool along = true;
          for (size_t i = 0; i < w.arcs.size(); ++i)
            along = along and s.arcs[size_t(w.arcs[i])][0] == w.nodes[i]
              and s.arcs[size_t(w.arcs[i])][1] == w.nodes[i + 1];
          if (p.check_directed() != along)
            check_directed.fail(s, "says that the path " + text(w.nodes)
                                + (along ? " is not" : " is") + " directed");
        });

      // The walks along the arcs, as directed, on every kind
      for_each_walk(s, 3, true, [&] (const Spec_Walk & w)
        {
          dir_arcs.run();
          Path<GT> p(b.g, node(w.start));
          for (int i : w.arcs)
            p.append_directed(b.arcs[size_t(i)]);
          Path<GT> r(b.g, node(w.nodes.back()));
          for (size_t i = w.arcs.size(); i-- > 0; )
            r.insert_directed(b.arcs[size_t(w.arcs[i])]);
          if (auto e = path_error(s, p, w, false, true); not e.empty())
            dir_arcs.fail(s, "appending: " + e);
          else if (auto e2 = path_error(s, r, w, false, true); not e2.empty())
            dir_arcs.fail(s, "inserting: " + e2);

          dir_nodes.run();
          Path<GT> q(b.g), t(b.g);
          for (int x : w.nodes)
            q.append_directed(node(x));
          for (size_t i = w.nodes.size(); i-- > 0; )
            t.insert_directed(node(w.nodes[i]));
          if (auto e = path_error(s, q, w, true, true); not e.empty())
            dir_nodes.fail(s, "appending: " + e);
          else if (auto e2 = path_error(s, t, w, true, true); not e2.empty())
            dir_nodes.fail(s, "inserting: " + e2);
        });
    });

  by_arcs.expect();
  by_nodes.expect();
  ins_arcs.expect();
  ins_nodes.expect();
  dir_arcs.expect();
  dir_nodes.expect();
  removals.expect();
  copies.expect();
  check.expect();
  check_directed.expect();
}

TYPED_TEST(ExhaustiveStructure, PathsRejectWhatDoesNotFollow)
{
  using GT = TypeParam;
  const bool digraph = GT().is_digraph();
  Check arc("Path::append(Arc *) with an arc away from the last node");
  Check node("Path::append(Node *) toward a node without an arc");
  Check inserted("Path::insert(Node *) from a node without an arc");
  Check inserted_directed("Path::insert_directed(Node *) from a node without an arc to the first");
  Check directed("Path::append_directed(Arc *) with an arc that does not leave the last node");

  for_each_spec(domain(3, 2), digraph, [&] (const Spec & s)
    {
      Built<GT> b(s);
      for (int x = 0; x < s.n; ++x)
        {
          const std::string at = "from " + std::to_string(x) + ": ";
          for (size_t i = 0; i < s.arcs.size(); ++i)
            {
              const auto & a = s.arcs[i];
              Path<GT> p(b.g, b.nodes[size_t(x)]);
              if (a[0] != x and a[1] != x)
                {
                  arc.run();
                  try
                    {
                      p.append(b.arcs[i]);
                      arc.fail(s, at + "takes the arc " + std::to_string(i));
                    }
                  catch (const std::invalid_argument &) {}
                }
              if (a[0] != x)
                {
                  directed.run();
                  Path<GT> q(b.g, b.nodes[size_t(x)]);
                  try
                    {
                      q.append_directed(b.arcs[i]);
                      directed.fail(s, at + "takes the arc " + std::to_string(i));
                    }
                  catch (const std::invalid_argument &) {}
                }
            }

          // append() goes along an arc from the last node, and insert()
          // along one to the first node; in a graph that is not directed,
          // along any arc between them
          for (int y = 0; y < s.n; ++y)
            {
              bool to_y = false, from_y = false;
              for (const auto & a : s.arcs)
                {
                  const bool x_y = a[0] == x and a[1] == y, y_x = a[0] == y and a[1] == x;
                  to_y = to_y or x_y or (not digraph and y_x);
                  from_y = from_y or y_x or (not digraph and x_y);
                }

              if (not to_y)
                {
                  node.run();
                  try
                    {
                      Path<GT> p(b.g, b.nodes[size_t(x)]);
                      p.append(b.nodes[size_t(y)]);
                      node.fail(s, at + "goes to " + std::to_string(y));
                    }
                  catch (const std::invalid_argument &) {}
                }

              if (not from_y)
                {
                  inserted.run();
                  Path<GT> p(b.g, b.nodes[size_t(x)]);
                  try
                    {
                      p.insert(b.nodes[size_t(y)]);
                      inserted.fail(s, at + "comes from " + std::to_string(y));
                    }
                  catch (const std::invalid_argument &)
                    {
                      if (p.size() != 1 or p.get_first_node() != b.nodes[size_t(x)])
                        inserted.fail(s, at + "changes the path");
                    }
                  catch (const std::exception & e)
                    {
                      inserted.fail(s, at + "throws " + std::string(typeid(e).name()));
                    }
                }
            }

          // insert_directed() needs an arc from the inserted node to the
          // first one, on every kind
          for (int y = 0; y < s.n; ++y)
            {
              bool from_y = false;
              for (const auto & a : s.arcs)
                from_y = from_y or (a[0] == y and a[1] == x);
              if (from_y)
                continue;

              inserted_directed.run();
              Path<GT> p(b.g, b.nodes[size_t(x)]);
              try
                {
                  p.insert_directed(b.nodes[size_t(y)]);
                  inserted_directed.fail(s, at + "comes from " + std::to_string(y));
                }
              catch (const std::invalid_argument &)
                {
                  if (p.size() != 1 or p.get_first_node() != b.nodes[size_t(x)])
                    inserted_directed.fail(s, at + "changes the path");
                }
              catch (const std::exception & e)
                {
                  inserted_directed.fail(s, at + "throws " + std::string(typeid(e).name()));
                }
            }
        }
    });

  arc.expect();
  node.expect();
  inserted.expect();
  inserted_directed.expect();
  directed.expect();
}

TYPED_TEST(ExhaustiveStructure, DisconnectedArcsComeBack)
{
  using GT = TypeParam;
  const bool digraph = GT().is_digraph();
  Check disconnect("disconnect_arc()");
  Check connect("connect_arc() after disconnect_arc()");

  for_each_spec(domain(4, 3), digraph, [&] (const Spec & s)
    {
      // List_SGraph keeps them private
      if constexpr (requires (GT & g, typename GT::Arc * a) { g.disconnect_arc(a);
                                                             g.connect_arc(a); })
        for (size_t i = 0; i < s.arcs.size(); ++i)
          {
            Built<GT> b(s);
            auto * a = b.arcs[i];
            disconnect.run();
            b.g.disconnect_arc(a);
            Alive alive(s);
            alive.arcs[i] = false;
            if (auto e = incidence_error(b.g, s, alive); not e.empty())
              disconnect.fail(s, "without the arc " + std::to_string(i) + ": " + e);

            connect.run();
            b.g.connect_arc(a);
            if (auto e = incidence_error(b.g, s, Alive(s)); not e.empty())
              connect.fail(s, "with the arc " + std::to_string(i) + " back: " + e);
          }
    });

  disconnect.expect();
  connect.expect();
}

TYPED_TEST(ExhaustiveStructure, ClearedGraphsAreEmptyAndTakeNodesAgain)
{
  using GT = TypeParam;
  const bool digraph = GT().is_digraph();
  Check clear("clear_graph()");

  for_each_spec(domain(4, 3), digraph, [&] (const Spec & s)
    {
      clear.run();
      Built<GT> b(s);
      clear_graph(b.g);
      const Spec none{};
      if (auto e = incidence_error(b.g, none, Alive(none)); not e.empty())
        clear.fail(s, "after clear_graph(): " + e);
      else
        {
          Built<GT> again(s);
          again.g.swap(b.g);
          if (auto e = incidence_error(b.g, s, Alive(s)); not e.empty())
            clear.fail(s, "a cleared graph does not take a graph: " + e);
        }
    });

  clear.expect();
}

TYPED_TEST(ExhaustiveStructure, SortsOrderTheIteratorsAndKeepTheGraph)
{
  using GT = TypeParam;
  const bool digraph = GT().is_digraph();
  Check arcs("sort_arcs()");
  Check nodes("sort_nodes()");

  // A key with ties, against the order of insertion
  auto arc_key = [] (int i) { return (7 - i) % 3; };
  auto node_key = [] (int x) { return (5 - x) % 2; };

  for_each_spec(domain(4, 3), digraph, [&] (const Spec & s)
    {
      Built<GT> b(s);
      if constexpr (requires { b.g.sort_arcs([] (auto *, auto *) { return false; }); })
        {
          arcs.run();
          b.g.sort_arcs([&] (typename GT::Arc * x, typename GT::Arc * y)
                        { return arc_key(num(x)) < arc_key(num(y)); });
          std::vector<int> order;
          for (auto it = b.g.get_arc_it(); it.has_curr(); it.next_ne())
            order.push_back(arc_key(num(it.get_curr())));
          if (not std::is_sorted(order.begin(), order.end()))
            arcs.fail(s, "the arcs are not sorted");
          else if (auto e = incidence_error(b.g, s, Alive(s)); not e.empty())
            arcs.fail(s, e);

          nodes.run();
          b.g.sort_nodes([&] (typename GT::Node * x, typename GT::Node * y)
                         { return node_key(num(x)) < node_key(num(y)); });
          std::vector<int> node_order;
          for (auto it = b.g.get_node_it(); it.has_curr(); it.next_ne())
            node_order.push_back(node_key(num(it.get_curr())));
          if (not std::is_sorted(node_order.begin(), node_order.end()))
            nodes.fail(s, "the nodes are not sorted");
          else if (auto e = incidence_error(b.g, s, Alive(s)); not e.empty())
            nodes.fail(s, e);
        }
    });

  arcs.expect();
  nodes.expect();
}

TYPED_TEST(ExhaustiveStructure, SingleGraphsHaveNoLoopsNorParallelArcs)
{
  using GT = TypeParam;
  const bool digraph = GT().is_digraph();
  Check single("Test_Single_Graph");

  for_each_spec(domain(4, 3), digraph, [&] (const Spec & s)
    {
      single.run();
      Built<GT> b(s);
      // Two arcs between the same nodes are parallel, in a graph that is
      // not directed whatever their directions
      std::set<std::pair<int, int>> seen;
      bool simple = true;
      for (const auto & a : s.arcs)
        {
          const auto key = digraph ? std::pair(a[0], a[1])
                                   : std::pair(std::min(a[0], a[1]), std::max(a[0], a[1]));
          simple = simple and a[0] != a[1] and seen.insert(key).second;
        }
      if (Test_Single_Graph<GT>()(b.g) != simple)
        single.fail(s, simple ? "says that a simple graph is not" : "says that it is simple");
    });

  single.expect();
}
