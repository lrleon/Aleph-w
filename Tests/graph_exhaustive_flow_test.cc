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
 * @file graph_exhaustive_flow_test.cc
 * @brief Maximum flows, cuts, decompositions, minimum costs, assignments and
 *        the applications of the networks on every small network, against
 *        the oracles.
 *
 * Each test runs one family of algorithms over every digraph of a domain of
 * graph_harness.H (up to 4 nodes and 3 arcs for the maximum flows; one more
 * of each with ALEPH_GRAPH_HARNESS=heavy), with loops, parallel and
 * antiparallel arcs and the arcs in two orders (in one for the floating
 * flows, which are exact there and run as the integers do), with the
 * source and the sink taken over every pair of nodes, and compares each
 * answer with an oracle of this file, which uses the standard library
 * only, and GMP for the exact rationals. A network has the sources and the
 * sinks that its arcs give, so the tests make the source the only one with
 * arcs of capacity 0 to the others, and the sink likewise; if the source
 * has arcs in or the sink arcs out, the flow goes from a node added before
 * the source to one added after the sink. Every flow that an algorithm
 * leaves is checked: within the capacities, conserved, and of the value
 * that it returns.
 *
 * The families: the maximum flows (augmenting paths, preflows, Dinic, the
 * scaling and HLPP), from zero, from a flow already there and on a second
 * run, with every class of capacities and on random networks; the minimum
 * cuts, the decompositions and the statistics; the augmenting paths and
 * semi-paths; the super sources and sinks; the floating flows of tenths,
 * of real numbers and of capacities far apart, against exact rationals;
 * the cheapest maximum flows (cycle canceling, network simplex and
 * successive shortest paths), the transshipment, the transportation and
 * the assignments, by flow and by Hungarian; the applications of
 * net_apps.H; the capacities of the nodes; the supplies and demands; the
 * copies of the networks; the networks when memory runs out; and Johnson
 * with integer weights next to the limits.
 *
 * The capacities, and the costs, are in units of the type of the flow: 1
 * for the integers and 1/4 for the floating types, so that a floating flow
 * is exact; the spec that a failure prints has them in w.
 */

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#include <map>
#include <memory>
#include <new>
#include <numeric>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <gmpxx.h>
#include <gtest/gtest.h>

#include "allocation_failure.H"

#include <tpl_net.H>
#include <tpl_maxflow.H>
#include <tpl_netcost.H>
#include <tpl_mincost.H>
#include <tpl_netcapgraph.H>
#include <tpl_net_sup_dem.H>
#include <net_apps.H>
#include <Hungarian.H>
#include <Johnson.H>

#include "graph_harness.H"
#include "graph_oracles.H"

using namespace Aleph;
using namespace graph_harness;
namespace oracle = graph_oracles;

namespace
{
  // ------------------------------------------------------------------
  // Amounts of flow

  /// The amount of one unit of capacity or cost in the type F: 1 for the
  /// integers, 1/4 for the floating types
  template <typename F>
  constexpr long long Units_Per_One = std::is_floating_point_v<F> ? 4 : 1;

  /// units units as an amount of F
  template <typename F>
  F amount(long long units)
  {
    return F(units) / F(Units_Per_One<F>);
  }

  /// Whether x is exactly units units
  template <typename F>
  bool is_amount(F x, long long units)
  {
    return x * F(Units_Per_One<F>) == F(units);
  }

  template <typename F>
  std::string text(F x)
  {
    if constexpr (std::is_floating_point_v<F>)
      {
        char buf[40];
        std::snprintf(buf, sizeof buf, "%.17g", double(x));
        return buf;
      }
    else
      return std::to_string(x);
  }

  // ------------------------------------------------------------------
  // Networks built from specs

  template <class Net>
  concept Cost_Network = requires (typename Net::Arc * a) { a->cost; };

  /// Inserts in net a node with the info i
  template <class Net>
  typename Net::Node * insert_numbered_node(Net & net, int i)
  {
    return net.insert_node(i);
  }

  /// The info of the nodes that the tests add before the source and after
  /// the sink
  constexpr int Added_Source = -1;
  constexpr int Added_Sink = -2;

  /** A network made from a spec: node i and arc i of the spec are nodes[i]
      and arcs[i], with the capacities and costs given in units; the arcs
      that pin() adds follow in arcs. */
  template <class Net>
  struct Built_Net
  {
    using Node = typename Net::Node;
    using Arc = typename Net::Arc;
    using F = typename Net::Flow_Type;

    Net net;
    std::vector<Node *> nodes;
    std::vector<Arc *> arcs;
    size_t spec_arcs = 0;
    Node * src = nullptr; // the source and the sink of the flow
    Node * snk = nullptr;
    bool added_ends = false; // whether src and snk were added for the flow

    Built_Net(const Spec & s, const std::vector<long long> & cap,
              const std::vector<long long> & cost = {})
    {
      for (int i = 0; i < s.n; ++i)
        nodes.push_back(insert_numbered_node(net, i));
      for (size_t i = 0; i < s.arcs.size(); ++i)
        arcs.push_back(add(nodes[size_t(s.arcs[i][0])], nodes[size_t(s.arcs[i][1])],
                           amount<F>(cap[i]),
                           i < cost.size() ? amount<F>(cost[i]) : F{0}));
      spec_arcs = arcs.size();
    }

    /// A network with the capacities of s in s.w
    explicit Built_Net(const Spec & s) : Built_Net(s, s.w) {}

    /// A network whose capacities are the amounts cap, not units
    struct Amounts {};
    Built_Net(const Spec & s, const std::vector<F> & cap, Amounts)
    {
      for (int i = 0; i < s.n; ++i)
        nodes.push_back(insert_numbered_node(net, i));
      for (size_t i = 0; i < s.arcs.size(); ++i)
        arcs.push_back(add(nodes[size_t(s.arcs[i][0])], nodes[size_t(s.arcs[i][1])], cap[i]));
      spec_arcs = arcs.size();
    }

    Built_Net(const Built_Net &) = delete;
    Built_Net & operator = (const Built_Net &) = delete;

    Arc * add(Node * u, Node * v, F cap, F cost = F{0})
    {
      if constexpr (Cost_Network<Net>)
        return net.insert_arc(u, v, cap, cost);
      else
        {
          (void) cost;
          return net.insert_arc(u, v, cap, F{0});
        }
    }

    /** Makes nodes[s] the source and nodes[t] the sink of the flow. If s
        has no arcs in and t no arcs out, they are the ends, and arcs of
        capacity 0 from s to the other sources and from the other sinks to
        t leave them the only ones; else the ends are a node added before
        s and one added after t, joined to them by arcs that no flow
        fills. */
    void pin(int s, int t)
    {
      if (s == t)
        {
          src = snk = nodes[size_t(s)];
          return;
        }
      Node * a = nodes[size_t(s)], * z = nodes[size_t(t)];
      added_ends = not net.is_source(a) or not net.is_sink(z);
      if (added_ends)
        {
          F total{1};
          for (Arc * arc : arcs)
            total += arc->cap;
          src = insert_numbered_node(net, Added_Source);
          snk = insert_numbered_node(net, Added_Sink);
          arcs.push_back(add(src, a, total));
          arcs.push_back(add(z, snk, total));
        }
      else
        {
          src = a;
          snk = z;
        }

      std::vector<Node *> others;
      for (auto it = net.get_src_nodes().get_it(); it.has_curr(); it.next_ne())
        if (it.get_curr() != src)
          others.push_back(it.get_curr());
      for (Node * p : others)
        arcs.push_back(add(src, p, F{0}));

      others.clear();
      for (auto it = net.get_sink_nodes().get_it(); it.has_curr(); it.next_ne())
        if (it.get_curr() != snk)
          others.push_back(it.get_curr());
      for (Node * p : others)
        arcs.push_back(add(p, snk, F{0}));
    }

    /// The number of a spec arc, or -1 for an added one or one of no
    /// network of this
    int number(Arc * a) const
    {
      for (size_t i = 0; i < spec_arcs; ++i)
        if (arcs[i] == a)
          return int(i);
      return -1;
    }

    /// The value of the flow: what leaves the source, less what enters it
    F value() const
    {
      return net.get_out_flow(src) - net.get_in_flow(src);
    }

    /// The flows of the arcs, in the order of arcs
    std::vector<F> flows() const
    {
      std::vector<F> r;
      for (Arc * a : arcs)
        r.push_back(a->flow);
      return r;
    }

    /// The flows back, as flows() gave them
    void set_flows(const std::vector<F> & f)
    {
      for (size_t i = 0; i < arcs.size(); ++i)
        arcs[i]->flow = f[i];
    }
  };

  template <class Node>
  std::string name_of(Node * p)
  {
    const int i = p->get_info();
    return i == Added_Source ? "s*" : i == Added_Sink ? "t*" : std::to_string(i);
  }

  /// What is wrong with the flow left in b: an arc out of [0, cap], or a
  /// node other than the ends that does not send all that it receives;
  /// empty if nothing. With tolerance, a floating flow is conserved within
  /// that of check_network().
  template <class Net>
  std::string flow_error(const Built_Net<Net> & b, bool tolerance = false)
  {
    using F = typename Net::Flow_Type;
    for (typename Net::Arc_Iterator it(b.net); it.has_curr(); it.next_ne())
      {
        auto a = it.get_curr();
        if (not (a->flow >= F{0} and a->flow <= a->cap))
          return "the arc " + name_of(b.net.get_src_node(a)) + "->" + name_of(b.net.get_tgt_node(a))
            + " carries " + text(a->flow) + " of " + text(a->cap);
      }
    for (typename Net::Node_Iterator it(b.net); it.has_curr(); it.next_ne())
      {
        auto p = it.get_curr();
        if (p == b.src or p == b.snk)
          continue;
        const F in = b.net.get_in_flow(p), out = b.net.get_out_flow(p);
        const bool kept = tolerance
          ? std::abs(double(in) - double(out)) <= 1e-9 * std::max(std::abs(double(in)),
                                                                 std::abs(double(out)))
          : in == out;
        if (not kept)
          return "the node " + name_of(p) + " receives " + text(in) + " and sends " + text(out);
      }
    return "";
  }

  /// What is wrong with b as a maximum flow of want units that the
  /// algorithm said is got; empty if nothing
  template <class Net>
  std::string max_flow_error(const Built_Net<Net> & b, typename Net::Flow_Type got,
                             long long want)
  {
    if (auto e = flow_error(b); not e.empty())
      return e;
    if (not is_amount(b.value(), want))
      return "the flow is " + text(b.value()) + ", not "
        + text(amount<typename Net::Flow_Type>(want));
    if (got != b.value())
      return "returns " + text(got) + " for a flow of " + text(b.value());
    return "";
  }

  /// The pairs of ends of the flows of s: every pair of different nodes,
  /// or, with one node, that node to itself
  std::vector<std::pair<int, int>> ends_of(const Spec & s)
  {
    std::vector<std::pair<int, int>> r;
    if (s.n == 1)
      r.push_back({0, 0});
    for (int a = 0; a < s.n; ++a)
      for (int z = 0; z < s.n; ++z)
        if (a != z)
          r.push_back({a, z});
    return r;
  }

  /// "from a to z: " for the messages, with the note of added ends
  template <class Net>
  std::string where(const Built_Net<Net> & b, int a, int z)
  {
    return "from " + std::to_string(a) + " to " + std::to_string(z)
      + (b.added_ends ? " through added ends" : "") + ": ";
  }

  /// The spec s with the capacities w
  Spec with_w(const Spec & s, const std::vector<long long> & w)
  {
    Spec r = s;
    r.w = w;
    return r;
  }

  /// Calls f(spec) with s and each assignment of values to its arcs, in w
  template <class F>
  void for_each_assignment(const Spec & s, const std::vector<long long> & values, F f)
  {
    std::vector<size_t> pick(s.arcs.size(), 0);
    while (true)
      {
        std::vector<long long> w;
        for (size_t k : pick)
          w.push_back(values[k]);
        f(with_w(s, w));
        size_t i = 0;
        while (i < pick.size() and ++pick[i] == values.size())
          pick[i++] = 0;
        if (i == pick.size())
          return;
      }
  }

  /// The structural capacities: arc i has i + 1 units, so that parallel
  /// arcs differ
  Spec structural(const Spec & s)
  {
    std::vector<long long> w;
    for (size_t i = 0; i < s.arcs.size(); ++i)
      w.push_back(static_cast<long long>(i) + 1);
    return with_w(s, w);
  }

  // ------------------------------------------------------------------
  // Oracles

  /// The maximum flow from a to z by Edmonds-Karp on the matrix of the
  /// capacities cap, in any exact number type
  template <typename N>
  N exact_max_flow(const Spec & s, const std::vector<N> & cap, int a, int z)
  {
    const size_t n = size_t(s.n);
    std::vector<std::vector<N>> c(n, std::vector<N>(n, N(0)));
    for (size_t i = 0; i < s.arcs.size(); ++i)
      if (s.arcs[i][0] != s.arcs[i][1])
        c[size_t(s.arcs[i][0])][size_t(s.arcs[i][1])] += cap[i];
    N flow(0);
    while (true)
      {
        std::vector<int> parent(n, -1);
        parent[size_t(a)] = a;
        std::vector<int> queue = {a};
        for (size_t h = 0; h < queue.size(); ++h)
          for (size_t v = 0; v < n; ++v)
            if (parent[v] == -1 and c[size_t(queue[h])][v] > 0)
              {
                parent[v] = queue[h];
                queue.push_back(int(v));
              }
        if (parent[size_t(z)] == -1)
          return flow;
        N b = c[size_t(parent[size_t(z)])][size_t(z)];
        for (int v = z; v != a; v = parent[size_t(v)])
          b = std::min(b, c[size_t(parent[size_t(v)])][size_t(v)]);
        for (int v = z; v != a; v = parent[size_t(v)])
          {
            c[size_t(parent[size_t(v)])][size_t(v)] -= b;
            c[size_t(v)][size_t(parent[size_t(v)])] += b;
          }
        flow += b;
      }
  }

  /// The maximum flow from a to z in units, the capacities in s.w; 0 from
  /// a node to itself
  long long max_flow_units(const Spec & s, int a, int z)
  {
    if (a == z)
      return 0;
    return oracle::max_flow(s, a, z);
  }

  /// The capacity of a minimum cut between a and z, by brute force over
  /// the sets of nodes that hold a and not z
  long long min_cut_units(const Spec & s, int a, int z)
  {
    long long best = oracle::Inf;
    for (unsigned long set = 0; set < (1UL << s.n); ++set)
      {
        if (((set >> a) & 1UL) == 0 or ((set >> z) & 1UL) != 0)
          continue;
        long long cut = 0;
        for (size_t i = 0; i < s.arcs.size(); ++i)
          if (((set >> s.arcs[i][0]) & 1UL) != 0 and ((set >> s.arcs[i][1]) & 1UL) == 0)
            cut += s.w[i];
        best = std::min(best, cut);
      }
    return best;
  }

  // ------------------------------------------------------------------
  // The maximum flow algorithms

  /// A maximum flow algorithm, by its name
  template <class Net>
  struct Solver
  {
    std::string name;
    typename Net::Flow_Type (*run)(Net &);
  };

  template <class Net>
  std::vector<Solver<Net>> augmenting_solvers()
  {
    return {{"ford_fulkerson_maximum_flow()", &ford_fulkerson_maximum_flow<Net>},
            {"edmonds_karp_maximum_flow()", &edmonds_karp_maximum_flow<Net>}};
  }

  template <class Net>
  std::vector<Solver<Net>> preflow_solvers()
  {
    return {{"fifo_preflow_maximum_flow()", &fifo_preflow_maximum_flow<Net>},
            {"heap_preflow_maximum_flow()", &heap_preflow_maximum_flow<Net>}};
  }

  /// The preflow with a random order, apart: it seeds its order with the
  /// time, and costs as much as the others together
  template <class Net>
  std::vector<Solver<Net>> random_preflow_solvers()
  {
    return {{"random_preflow_maximum_flow()", &random_preflow_maximum_flow<Net>}};
  }

  template <class Net>
  std::vector<Solver<Net>> advanced_solvers()
  {
    return {{"dinic_maximum_flow()", &dinic_maximum_flow<Net>},
            {"capacity_scaling_maximum_flow()", &capacity_scaling_maximum_flow<Net>},
            {"hlpp_maximum_flow()", &hlpp_maximum_flow<Net>}};
  }

  template <class Net>
  std::vector<Solver<Net>> all_solvers()
  {
    auto r = augmenting_solvers<Net>();
    for (auto & x : preflow_solvers<Net>())
      r.push_back(x);
    for (auto & x : random_preflow_solvers<Net>())
      r.push_back(x);
    for (auto & x : advanced_solvers<Net>())
      r.push_back(x);
    return r;
  }

  /// The functors of the maximum flow algorithms, in the order of
  /// all_solvers()
  template <class Net>
  std::vector<Solver<Net>> functor_solvers()
  {
    using F = typename Net::Flow_Type;
    return {
      {"Ford_Fulkerson_Maximum_Flow",
       [] (Net & n) -> F { return Ford_Fulkerson_Maximum_Flow<Net>()(n); }},
      {"Edmonds_Karp_Maximum_Flow",
       [] (Net & n) -> F { return Edmonds_Karp_Maximum_Flow<Net>()(n); }},
      {"Fifo_Preflow_Maximum_Flow",
       [] (Net & n) -> F { return Fifo_Preflow_Maximum_Flow<Net>()(n); }},
      {"Heap_Preflow_Maximum_Flow",
       [] (Net & n) -> F { return Heap_Preflow_Maximum_Flow<Net>()(n); }},
      {"Random_Preflow_Maximum_Flow",
       [] (Net & n) -> F { return Random_Preflow_Maximum_Flow<Net>()(n); }},
      {"Dinic_Maximum_Flow", [] (Net & n) -> F { return Dinic_Maximum_Flow<Net>()(n); }},
      {"Capacity_Scaling_Maximum_Flow",
       [] (Net & n) -> F { return Capacity_Scaling_Maximum_Flow<Net>()(n); }},
      {"HLPP_Maximum_Flow", [] (Net & n) -> F { return HLPP_Maximum_Flow<Net>()(n); }}};
  }

  /// A class of networks where a family of algorithms fails, by now: what
  /// it is, and whether a failure there ends the program, so that its
  /// networks are left out
  struct Known_Class
  {
    std::string why;
    bool ends_program = false;
  };

  /** The checks of the algorithms of a family. On the networks of no known
      class, algorithm i is checked by plain[i]; on those of class k, from
      1, by a check that expects failures, made when first needed, which
      leaves the networks out if the class ends the program. */
  class Family_Checks
  {
    std::vector<std::string> names;
    std::vector<Known_Class> classes;
    std::vector<Check> plain;
    std::map<std::pair<int, size_t>, Check> known;

  public:
    template <class Solvers>
    explicit Family_Checks(const Solvers & solvers, std::vector<Known_Class> known_classes = {})
      : classes(std::move(known_classes))
    {
      for (const auto & s : solvers)
        {
          names.push_back(s.name);
          plain.emplace_back(s.name);
        }
    }

    /// The check of algorithm i on a network of class k, 0 for none, or
    /// nullptr if the network is left out
    Check * of(size_t i, int k)
    {
      if (k == 0)
        return &plain[i];
      const Known_Class & c = classes[size_t(k - 1)];
      Check & check = known.try_emplace({k, i}, names[i] + ", " + c.why, true).first->second;
      if (c.ends_program)
        {
          check.skip();
          return nullptr;
        }
      return &check;
    }

    void expect() const
    {
      for (const auto & c : plain)
        c.expect();
      for (const auto & [key, c] : known)
        c.expect();
    }
  };

  /// No known class
  template <class Net>
  int no_class(size_t, const Built_Net<Net> &)
  {
    return 0;
  }

  /// Expects each solver to refuse the network of s, as its arcs make it,
  /// if it has no single source and sink
  template <class Net>
  void check_refusal(const std::vector<Solver<Net>> & solvers, const Spec & s, Check & refuse)
  {
    Built_Net<Net> b(s);
    if (b.net.is_single_source() and b.net.is_single_sink())
      return;
    for (const auto & solver : solvers)
      {
        refuse.run();
        try
          {
            solver.run(b.net);
            refuse.fail(s, solver.name + " does not throw");
          }
        catch (const std::domain_error &) {}
        catch (const std::exception & e)
          {
            refuse.fail(s, solver.name + " throws " + e.what());
          }
      }
  }

  /** Runs each solver, from the zero flow, on every network of the specs
      that for_each gives, between every pair of ends, and checks the flow
      that it leaves against the oracle; the network that the arcs make,
      without a single source and sink, it must refuse with domain_error.
      classify(i, b) tells the known class of the network b for solver i. */
  template <class Net, class For_Each, class Classify>
  void check_max_flows(const std::vector<Solver<Net>> & solvers, For_Each for_each,
                       Family_Checks & checks, Check & refuse, Classify classify)
  {
    for_each([&] (const Spec & s)
      {
        check_refusal(solvers, s, refuse);
        for (auto [a, z] : ends_of(s))
          {
            Built_Net<Net> b(s);
            b.pin(a, z);
            if (not (b.net.is_single_source() and b.net.is_single_sink()))
              {
                // One node with loops, refused above; else the sets of the
                // sources and sinks are wrong
                if (a != z)
                  refuse.fail(s, where(b, a, z) + "the network keeps other sources or sinks");
                continue;
              }
            const long long want = max_flow_units(s, a, z);
            for (size_t i = 0; i < solvers.size(); ++i)
              {
                Check * check = checks.of(i, classify(i, b));
                if (check == nullptr)
                  continue;
                b.net.reset();
                check->run();
                typename Net::Flow_Type got{};
                try
                  {
                    got = solvers[i].run(b.net);
                  }
                catch (const std::exception & e)
                  {
                    check->fail(s, where(b, a, z) + "throws " + e.what());
                    continue;
                  }
                if (auto e = max_flow_error(b, got, want); not e.empty())
                  check->fail(s, where(b, a, z) + e);
              }
          }
      });
  }

  /// Calls f with every spec of d, with the structural capacities
  auto structural_specs(const Domain & d)
  {
    return [d] (auto f)
      {
        for_each_spec(d, true, [&] (const Spec & s) { f(structural(s)); });
      };
  }

  /// The values of the classes of capacities: 0, 1 and 2, or, for the
  /// floating types, 0, 3/4 and 3/2
  template <typename F>
  std::vector<long long> capacity_class()
  {
    if constexpr (std::is_floating_point_v<F>)
      return {0, 3, 6};
    else
      return {0, 1, 2};
  }

  /// The domain of the classes of capacities: every digraph of 3 nodes
  /// and 3 arcs without loops, with the arcs in one order; 4 and 3 with
  /// ALEPH_GRAPH_HARNESS=heavy
  Domain class_domain()
  {
    Domain d = domain(3, 3);
    d.max_arcs = 3;
    d.loops = false;
    d.reversed = false;
    return d;
  }

  /// Calls f with every spec of d and every assignment of the capacity
  /// class of F
  template <typename F>
  auto class_specs(const Domain & d)
  {
    return [d] (auto f)
      {
        for_each_spec(d, true, [&] (const Spec & s)
          {
            for_each_assignment(s, capacity_class<F>(), f);
          });
      };
  }

  /// Calls f with count random networks of 6 to 8 nodes and 8 to 16 arcs,
  /// with loops and parallel arcs, and capacities from 0 to 5 units, from
  /// the seed of the harness
  auto random_specs(unsigned first, unsigned count)
  {
    return [first, count] (auto f)
      {
        std::mt19937 rng(seed(first));
        for (unsigned k = 0; k < count; ++k)
          {
            const int n = 6 + int(rng() % 3);
            const int m = 8 + int(rng() % 9);
            Spec s = random_spec(rng, n, m, true);
            random_weights(rng, s, 0, 5);
            f(s);
          }
      };
  }
}

// ======================================================================
// Maximum flows

template <class Net>
class ExhaustiveMaxFlow : public ::testing::Test {};

/// The networks of integers, of floating flows and of costs
using Flow_Nets =
  ::testing::Types<Net_Graph<Net_Node<int>, Net_Arc<int, int>>,
                   Net_Graph<Net_Node<int>, Net_Arc<int, double>>,
                   Net_Cost_Graph<Net_Cost_Node<int>, Net_Cost_Arc<Empty_Class, long>>>;

TYPED_TEST_SUITE(ExhaustiveMaxFlow, Flow_Nets);

namespace
{
  /// Whether the structural networks of Net take the arcs in one order
  /// only: the floating flows, exact in units of 1/4, run there as the
  /// integers do, which take the arcs in both orders
  template <class Net>
  constexpr bool in_one_order = std::is_floating_point_v<typename Net::Flow_Type>;

  /** The structural domain of the maximum flows: every digraph of 4 nodes
      and 3 arcs, with the arcs in two orders unless one_order or
      in_one_order. The networks with costs, which run the same algorithms
      with flows of long, take every digraph of 3 nodes and 3 arcs. */
  template <class Net>
  Domain max_flow_domain(bool one_order = false)
  {
    Domain d = Cost_Network<Net> ? domain(3, 3) : domain(4, 3);
    d.reversed = not (one_order or in_one_order<Net>);
    return d;
  }

  /// The networks of the maximum flow tests: those of max_flow_domain()
  /// with the structural capacities, or every digraph of 3 nodes and 3
  /// arcs without loops with each assignment of the class of capacities
  /// and random ones of 6 to 8 nodes
  enum class Flow_Domain { Structural, Capacities };

  template <class Net, class Classify>
  void check_family(const std::vector<Solver<Net>> & solvers, Family_Checks & checks,
                    Check & refuse, Flow_Domain which, unsigned seed_base, Classify classify,
                    bool one_order = false)
  {
    if (which == Flow_Domain::Structural)
      check_max_flows(solvers, structural_specs(max_flow_domain<Net>(one_order)), checks,
                      refuse, classify);
    else
      {
        check_max_flows(solvers, class_specs<typename Net::Flow_Type>(class_domain()), checks,
                        refuse, classify);
        check_max_flows(solvers, random_specs(seed_base, heavy() ? 400 : 40), checks, refuse,
                        classify);
      }
    checks.expect();
    refuse.expect();
  }

  /// The checks of the augmenting paths
  template <class Net>
  void check_augmenting_paths(Flow_Domain which)
  {
    const auto solvers = augmenting_solvers<Net>();
    Family_Checks checks(solvers);
    Check refuse("the augmenting paths without one source and one sink");
    check_family(solvers, checks, refuse, which, 11, no_class<Net>);
  }

  /// The checks of the preflows, with a queue or a heap of active nodes, or,
  /// if random, with a random order
  template <class Net>
  void check_preflows(Flow_Domain which, bool random)
  {
    const auto solvers = random ? random_preflow_solvers<Net>() : preflow_solvers<Net>();
    Family_Checks checks(solvers);
    Check refuse("the preflows without one source and one sink");
    // The random order does not need the arcs in two orders, and costs
    // twice as much
    check_family(solvers, checks, refuse, which, random ? 13 : 12, no_class<Net>, random);
  }

  /// The checks of Dinic, the scaling and HLPP
  template <class Net>
  void check_advanced(Flow_Domain which)
  {
    const auto solvers = advanced_solvers<Net>();
    Family_Checks checks(solvers);
    Check refuse("Dinic, the scaling and HLPP without one source and one sink");
    check_family(solvers, checks, refuse, which, 14, no_class<Net>);
  }
}

TYPED_TEST(ExhaustiveMaxFlow, AugmentingPathsMatchTheOracle)
{
  check_augmenting_paths<TypeParam>(Flow_Domain::Structural);
}

TYPED_TEST(ExhaustiveMaxFlow, PreflowsMatchTheOracle)
{
  check_preflows<TypeParam>(Flow_Domain::Structural, false);
}

TYPED_TEST(ExhaustiveMaxFlow, RandomPreflowMatchesTheOracle)
{
  check_preflows<TypeParam>(Flow_Domain::Structural, true);
}

TYPED_TEST(ExhaustiveMaxFlow, DinicScalingAndHlppMatchTheOracle)
{
  check_advanced<TypeParam>(Flow_Domain::Structural);
}

template <class Net>
class ExhaustiveMaxFlowCapacities : public ::testing::Test {};

/// The networks of integers and of floating flows
using Capacity_Nets =
  ::testing::Types<Net_Graph<Net_Node<int>, Net_Arc<int, int>>,
                   Net_Graph<Net_Node<int>, Net_Arc<int, double>>>;

TYPED_TEST_SUITE(ExhaustiveMaxFlowCapacities, Capacity_Nets);

TYPED_TEST(ExhaustiveMaxFlowCapacities, AugmentingPathsTakeEveryCapacity)
{
  check_augmenting_paths<TypeParam>(Flow_Domain::Capacities);
}

TYPED_TEST(ExhaustiveMaxFlowCapacities, PreflowsTakeEveryCapacity)
{
  check_preflows<TypeParam>(Flow_Domain::Capacities, false);
}

TYPED_TEST(ExhaustiveMaxFlowCapacities, RandomPreflowTakesEveryCapacity)
{
  check_preflows<TypeParam>(Flow_Domain::Capacities, true);
}

TYPED_TEST(ExhaustiveMaxFlowCapacities, DinicScalingAndHlppTakeEveryCapacity)
{
  check_advanced<TypeParam>(Flow_Domain::Capacities);
}

// ======================================================================
// Flows to start from, and second runs

namespace
{
  /// The nodes of a network, with their positions
  template <class Net>
  struct Node_Index
  {
    std::vector<typename Net::Node *> nodes;
    std::map<typename Net::Node *, size_t> pos;

    explicit Node_Index(const Net & net)
    {
      for (typename Net::Node_Iterator it(net); it.has_curr(); it.next_ne())
        {
          pos[it.get_curr()] = nodes.size();
          nodes.push_back(it.get_curr());
        }
    }
  };

  /// What put_initial_flow() put: the value of the flow, in units, and
  /// whether it goes around a cycle too
  struct Initial_Flow
  {
    long long value = 0;
    bool cycle = false;
  };

  /** Puts in b, from the zero flow, the bottleneck of the first path of
      arcs with capacity from the source to the sink, by breadth, and then,
      if cycle, that of the first cycle of arcs with capacity left that a
      search in depth from each node finds, which goes around without
      changing the value. */
  template <class Net>
  Initial_Flow put_initial_flow(Built_Net<Net> & b, bool cycle = true)
  {
    using Arc = typename Net::Arc;
    using F = typename Net::Flow_Type;
    b.net.reset();
    const Node_Index<Net> ix(b.net);
    const size_t n = ix.nodes.size();
    auto left = [] (Arc * a) { return a->cap - a->flow; };

    long long value = 0;
    if (b.src != b.snk)
      {
        std::vector<Arc *> by(n, nullptr);
        std::vector<bool> seen(n, false);
        std::vector<size_t> queue = {ix.pos.at(b.src)};
        seen[queue[0]] = true;
        for (size_t h = 0; h < queue.size(); ++h)
          for (Arc * a : b.arcs)
            if (ix.pos.at(b.net.get_src_node(a)) == queue[h] and left(a) > F{0})
              if (const size_t v = ix.pos.at(b.net.get_tgt_node(a)); not seen[v])
                {
                  seen[v] = true;
                  by[v] = a;
                  queue.push_back(v);
                }
        if (const size_t z = ix.pos.at(b.snk); seen[z])
          {
            F neck = std::numeric_limits<F>::max();
            for (size_t v = z; by[v] != nullptr; v = ix.pos.at(b.net.get_src_node(by[v])))
              neck = std::min(neck, left(by[v]));
            for (size_t v = z; by[v] != nullptr; v = ix.pos.at(b.net.get_src_node(by[v])))
              by[v]->flow += neck;
            value = static_cast<long long>(neck * F(Units_Per_One<F>));
          }
      }

    if (not cycle)
      return {value, false};

    // A cycle: a search in depth along arcs with capacity left, from each
    // node, until an arc goes back to a node on the stack
    for (size_t start = 0; start < n; ++start)
      {
        std::vector<Arc *> stack;      // the arcs from start
        std::vector<size_t> on = {start};
        std::vector<int> state(n, 0);  // 0 new, 1 on the stack, 2 done
        std::vector<size_t> next(n, 0);
        state[start] = 1;
        while (not on.empty())
          {
            const size_t u = on.back();
            bool advanced = false;
            for (; next[u] < b.arcs.size(); ++next[u])
              {
                Arc * a = b.arcs[next[u]];
                if (ix.pos.at(b.net.get_src_node(a)) != u or left(a) <= F{0})
                  continue;
                const size_t v = ix.pos.at(b.net.get_tgt_node(a));
                if (state[v] == 1)
                  {
                    // The cycle: from v along the stack to u, and a
                    std::vector<Arc *> c = {a};
                    for (size_t k = stack.size(); k > 0 and on[k] != v; --k)
                      c.push_back(stack[k - 1]);
                    F neck = std::numeric_limits<F>::max();
                    for (Arc * x : c)
                      neck = std::min(neck, left(x));
                    for (Arc * x : c)
                      x->flow += neck;
                    return {value, true};
                  }
                if (state[v] == 0)
                  {
                    state[v] = 1;
                    stack.push_back(a);
                    on.push_back(v);
                    ++next[u];
                    advanced = true;
                    break;
                  }
              }
            if (not advanced)
              {
                state[u] = 2;
                on.pop_back();
                if (not stack.empty())
                  stack.pop_back();
              }
          }
      }
    return {value, false};
  }

  /// The specs of the tests that run every algorithm on a network:
  /// every digraph of 3 nodes and 3 arcs (4 and 4 with
  /// ALEPH_GRAPH_HARNESS=heavy) with the structural capacities, with the
  /// arcs in two orders unless in_one_order
  template <class Net>
  auto small_structural_specs()
  {
    Domain d = domain(3, 3);
    d.reversed = not in_one_order<Net>;
    return structural_specs(d);
  }
}

TYPED_TEST(ExhaustiveMaxFlow, MaxFlowsFromAFlowAlreadyThere)
{
  using Net = TypeParam;
  const auto solvers = all_solvers<Net>();
  Family_Checks checks(solvers);

  small_structural_specs<Net>()([&] (const Spec & s)
    {
      for (auto [a, z] : ends_of(s))
        {
          Built_Net<Net> b(s);
          b.pin(a, z);
          if (not (b.net.is_single_source() and b.net.is_single_sink()))
            continue;
          const long long want = max_flow_units(s, a, z);
          for (size_t i = 0; i < solvers.size(); ++i)
            {
              const long long start = put_initial_flow(b).value;
              Check * check = checks.of(i, 0);
              check->run();
              const std::string at = where(b, a, z) + "from a flow of "
                + text(amount<typename Net::Flow_Type>(start)) + ": ";
              try
                {
                  const auto got = solvers[i].run(b.net);
                  if (auto e = max_flow_error(b, got, want); not e.empty())
                    check->fail(s, at + e);
                }
              catch (const std::exception & e)
                {
                  check->fail(s, at + "throws " + e.what());
                }
            }
        }
    });
  checks.expect();
}

TYPED_TEST(ExhaustiveMaxFlow, SecondRunsKeepTheMaximum)
{
  using Net = TypeParam;
  const auto solvers = all_solvers<Net>();
  const auto functors = functor_solvers<Net>();
  Family_Checks checks(functors);

  small_structural_specs<Net>()([&] (const Spec & s)
    {
      for (auto [a, z] : ends_of(s))
        {
          Built_Net<Net> b(s);
          b.pin(a, z);
          if (not (b.net.is_single_source() and b.net.is_single_sink()))
            continue;
          const long long want = max_flow_units(s, a, z);
          for (size_t i = 0; i < solvers.size(); ++i)
            {
              Check * check = checks.of(i, 0);
              check->run();
              b.net.reset();
              try
                {
                  solvers[i].run(b.net);
                  const auto got = functors[i].run(b.net);
                  if (auto e = max_flow_error(b, got, want); not e.empty())
                    check->fail(s, where(b, a, z) + "the second run: " + e);
                }
              catch (const std::exception & e)
                {
                  check->fail(s, where(b, a, z) + "throws " + e.what());
                }
            }
        }
    });
  checks.expect();
}

// ======================================================================
// Minimum cuts

namespace
{
  template <class Net>
  using Node_Set = DynSetTree<typename Net::Node *>;

  template <class Net>
  using Arc_List = DynList<typename Net::Arc *>;

  /// A minimum cut, by its name
  template <class Net>
  struct Cutter
  {
    std::string name;
    typename Net::Flow_Type (*run)(Net &, Node_Set<Net> &, Node_Set<Net> &, Arc_List<Net> &,
                                   Arc_List<Net> &);
  };

  /// min_cut() with each maximum flow, in the order of all_solvers(), and
  /// the functor Min_Cut with its default, the heap preflow
  template <class Net>
  std::vector<Cutter<Net>> cutters()
  {
    return {
      {"min_cut<Ford_Fulkerson_Maximum_Flow>()", &min_cut<Net, Ford_Fulkerson_Maximum_Flow>},
      {"min_cut<Edmonds_Karp_Maximum_Flow>()", &min_cut<Net, Edmonds_Karp_Maximum_Flow>},
      {"min_cut<Fifo_Preflow_Maximum_Flow>()", &min_cut<Net, Fifo_Preflow_Maximum_Flow>},
      {"min_cut<Heap_Preflow_Maximum_Flow>()", &min_cut<Net, Heap_Preflow_Maximum_Flow>},
      {"min_cut<Random_Preflow_Maximum_Flow>()", &min_cut<Net, Random_Preflow_Maximum_Flow>},
      {"min_cut<Dinic_Maximum_Flow>()", &min_cut<Net, Dinic_Maximum_Flow>},
      {"min_cut<Capacity_Scaling_Maximum_Flow>()", &min_cut<Net, Capacity_Scaling_Maximum_Flow>},
      {"min_cut<HLPP_Maximum_Flow>()", &min_cut<Net, HLPP_Maximum_Flow>},
      {"Min_Cut", [] (Net & net, Node_Set<Net> & vs, Node_Set<Net> & vt, Arc_List<Net> & cuts,
                      Arc_List<Net> & cutt) { return Min_Cut<Net>()(net, vs, vt, cuts, cutt); }}};
  }

  /// The nodes that the residual network of the flow in b reaches from its
  /// source
  template <class Net>
  std::set<typename Net::Node *> residual_reach(const Built_Net<Net> & b)
  {
    using F = typename Net::Flow_Type;
    std::set<typename Net::Node *> r = {b.src};
    for (bool grew = true; grew; )
      {
        grew = false;
        for (auto a : b.arcs)
          {
            auto u = b.net.get_src_node(a), v = b.net.get_tgt_node(a);
            if (r.count(u) and not r.count(v) and a->cap - a->flow > F{0})
              grew = r.insert(v).second;
            else if (r.count(v) and not r.count(u) and a->flow > F{0})
              grew = r.insert(u).second;
          }
      }
    return r;
  }

  /// What is wrong with the cut (vs, vt, cuts, cutt) of the flow in b, of
  /// capacity got, as a minimum cut of want units; empty if nothing
  template <class Net>
  std::string cut_error(const Built_Net<Net> & b, const Node_Set<Net> & vs,
                        const Node_Set<Net> & vt, const Arc_List<Net> & cuts,
                        const Arc_List<Net> & cutt, typename Net::Flow_Type got, long long want)
  {
    using F = typename Net::Flow_Type;
    if (not is_amount(got, want))
      return "returns " + text(got) + ", not " + text(amount<F>(want));
    if (auto e = flow_error(b); not e.empty())
      return e;
    std::set<typename Net::Node *> s_side, t_side;
    for (auto it = vs.get_it(); it.has_curr(); it.next_ne())
      s_side.insert(it.get_curr());
    for (auto it = vt.get_it(); it.has_curr(); it.next_ne())
      t_side.insert(it.get_curr());
    if (s_side.size() + t_side.size() != b.net.get_num_nodes() or s_side.count(b.src) == 0
        or t_side.count(b.snk) == 0)
      return "the sides are not a partition with the source and the sink apart";
    for (auto p : s_side)
      if (t_side.count(p))
        return "the node " + name_of(p) + " is on both sides";
    if (s_side != residual_reach(b))
      return "the side of the source is not what the residual network reaches";

    std::multiset<typename Net::Arc *> want_cuts, want_cutt, got_cuts, got_cutt;
    for (auto a : b.arcs)
      {
        const bool u = s_side.count(b.net.get_src_node(a)), v = s_side.count(b.net.get_tgt_node(a));
        if (u and not v)
          want_cuts.insert(a);
        else if (v and not u)
          want_cutt.insert(a);
      }
    F capacity{0};
    for (auto it = cuts.get_it(); it.has_curr(); it.next_ne())
      {
        got_cuts.insert(it.get_curr());
        capacity += it.get_curr()->cap;
      }
    for (auto it = cutt.get_it(); it.has_curr(); it.next_ne())
      got_cutt.insert(it.get_curr());
    if (got_cuts != want_cuts)
      return "the arcs of the cut are not those from the side of the source to the other";
    if (got_cutt != want_cutt)
      return "the arcs back are not those from the side of the sink to the other";
    if (capacity != got)
      return "the arcs of the cut have " + text(capacity) + ", not " + text(got);
    return "";
  }
}

TYPED_TEST(ExhaustiveMaxFlow, MinCutsMatchTheOracle)
{
  using Net = TypeParam;
  const auto cuts = cutters<Net>();
  Family_Checks checks(cuts);

  small_structural_specs<Net>()([&] (const Spec & s)
    {
      for (auto [a, z] : ends_of(s))
        {
          Built_Net<Net> b(s);
          b.pin(a, z);
          if (not (b.net.is_single_source() and b.net.is_single_sink()) or a == z)
            continue;
          const long long want = min_cut_units(s, a, z);
          for (size_t i = 0; i < cuts.size(); ++i)
            {
              Check * check = checks.of(i, 0);
              check->run();
              b.net.reset();
              Node_Set<Net> vs, vt;
              Arc_List<Net> cs, ct;
              try
                {
                  const auto got = cuts[i].run(b.net, vs, vt, cs, ct);
                  if (auto e = cut_error(b, vs, vt, cs, ct, got, want); not e.empty())
                    check->fail(s, where(b, a, z) + e);
                }
              catch (const std::exception & e)
                {
                  check->fail(s, where(b, a, z) + "throws " + e.what());
                }
            }
        }
    });
  checks.expect();
}

// ======================================================================
// Decompositions and statistics

namespace
{
  /** What is wrong with d as a decomposition of the flow in b, of the
      value of the flow, into paths from the source to the sink and cycles;
      empty if nothing. If rounded, a floating flow, whose nodes keep it
      but for rounding, may be carried but for rounding: within 1e-9 of
      each arc, and of the value, relative to them or to 1. */
  template <class Net>
  std::string decomposition_error(const Built_Net<Net> & b, const FlowDecomposition<Net> & d,
                                  bool rounded = false)
  {
    using F = typename Net::Flow_Type;
    using Arc = typename Net::Arc;
    using Node = typename Net::Node;
    std::map<Arc *, F> sum;
    auto same = [rounded] (F x, F y)
      {
        if constexpr (std::is_floating_point_v<F>)
          {
            if (rounded)
              return std::abs(x - y) <= 1e-9 * std::max({F{1}, std::abs(x), std::abs(y)});
          }
        else
          (void) rounded;
        return x == y;
      };

    auto walk = [&] (const DynList<Arc *> & arcs, const DynList<Node *> & nodes, F flow,
                     const char * what) -> std::string
      {
        std::vector<Arc *> as;
        std::vector<Node *> ns;
        for (auto it = arcs.get_it(); it.has_curr(); it.next_ne())
          as.push_back(it.get_curr());
        for (auto it = nodes.get_it(); it.has_curr(); it.next_ne())
          ns.push_back(it.get_curr());
        if (as.empty() or ns.size() != as.size() + 1 or not (flow > F{0}))
          return std::string("a ") + what + " of " + std::to_string(as.size()) + " arcs, "
            + std::to_string(ns.size()) + " nodes and flow " + text(flow);
        for (size_t k = 0; k < as.size(); ++k)
          {
            if (b.net.get_src_node(as[k]) != ns[k] or b.net.get_tgt_node(as[k]) != ns[k + 1])
              return std::string("a ") + what + " does not follow its arcs";
            sum[as[k]] += flow;
          }
        return "";
      };

    for (auto it = d.paths.get_it(); it.has_curr(); it.next_ne())
      {
        const auto & p = it.get_curr();
        if (auto e = walk(p.arcs, p.nodes, p.flow, "path"); not e.empty())
          return e;
        if (p.nodes.get_first() != b.src or p.nodes.get_last() != b.snk)
          return "a path does not go from the source to the sink";
      }
    for (auto it = d.cycles.get_it(); it.has_curr(); it.next_ne())
      {
        const auto & c = it.get_curr();
        if (auto e = walk(c.arcs, c.nodes, c.flow, "cycle"); not e.empty())
          return e;
        if (c.nodes.get_first() != c.nodes.get_last())
          return "a cycle is not closed";
      }
    for (Arc * a : b.arcs)
      if (not same(sum[a], a->flow))
        return "the paths and cycles carry " + text(sum[a]) + " on the arc "
          + name_of(b.net.get_src_node(a)) + "->" + name_of(b.net.get_tgt_node(a)) + ", not "
          + text(a->flow);
    if (not same(d.total_flow(), b.value()))
      return "total_flow() is " + text(d.total_flow()) + ", not " + text(b.value());
    if (d.num_paths() != d.paths.size() or d.num_cycles() != d.cycles.size())
      return "num_paths() or num_cycles() do not count them";
    return "";
  }

  /// What is wrong with the statistics st of the flow in b; empty if
  /// nothing
  template <class Net>
  std::string statistics_error(const Built_Net<Net> & b,
                               const FlowStatistics<typename Net::Flow_Type> & st)
  {
    using F = typename Net::Flow_Type;
    F capacity{0};
    size_t saturated = 0, empty = 0, partial = 0;
    for (auto a : b.arcs)
      {
        capacity += a->cap;
        if (a->flow == a->cap)
          ++saturated;
        else if (a->flow == F{0})
          ++empty;
        else
          ++partial;
      }
    if (st.total_flow != b.value() or st.total_capacity != capacity)
      return "the flow " + text(st.total_flow) + " and capacity " + text(st.total_capacity)
        + ", not " + text(b.value()) + " and " + text(capacity);
    if (st.num_saturated_arcs != saturated or st.num_empty_arcs != empty
        or st.num_partial_arcs != partial)
      return "the counts of the arcs are wrong";
    const double use = capacity > F{0} ? double(b.value()) / double(capacity) : 0.0;
    if (st.utilization != use)
      return "the utilization is " + text(st.utilization) + ", not " + text(use);
    return "";
  }
}

namespace
{
  /** Calls f(s, b, where) for every network of max_flow_domain(), with
      the arcs in one order, the reverse of that of for_each_spec() if
      reversed, between every pair of ends, with the flow that
      put_initial_flow() leaves, a path and a cycle. Without a single source
      and sink, the network that the arcs make must be refused. */
  template <class Net, class F>
  void for_each_flow_to_decompose(Check & refuse, bool reversed, F f)
  {
    for_each_spec(max_flow_domain<Net>(true), true, [&] (const Spec & given)
      {
        Spec r = given;
        if (reversed)
          std::reverse(r.arcs.begin(), r.arcs.end());
        const Spec s = structural(r);
        {
          Built_Net<Net> b(s);
          if (not (b.net.is_single_source() and b.net.is_single_sink()))
            {
              refuse.run();
              try
                {
                  decompose_flow(b.net);
                  refuse.fail(s, "does not throw");
                }
              catch (const std::domain_error &) {}
            }
        }
        for (auto [a, z] : ends_of(s))
          {
            Built_Net<Net> b(s);
            b.pin(a, z);
            if (not (b.net.is_single_source() and b.net.is_single_sink()))
              continue;
            put_initial_flow(b);
            f(s, b, where(b, a, z));
          }
      });
  }
}

TYPED_TEST(ExhaustiveMaxFlow, DecompositionsRebuildFlowsWithCycles)
{
  using Net = TypeParam;
  Check check("decompose_flow(), of a flow with a cycle");
  Check refuse("decompose_flow() without one source and one sink");
  // The arcs in the reverse order: the other is in the test of the maximum
  // flows
  for_each_flow_to_decompose<Net>(refuse, true, [&] (const Spec & s, Built_Net<Net> & b,
                                                     const std::string & at)
    {
      check.run();
      const auto before = b.flows();
      if (auto e = decomposition_error(b, decompose_flow(b.net)); not e.empty())
        check.fail(s, at + e);
      else if (b.flows() != before)
        check.fail(s, at + "changes the flows");
    });
  check.expect();
  refuse.expect();
}

TYPED_TEST(ExhaustiveMaxFlow, DecompositionsRebuildTheMaximumFlow)
{
  using Net = TypeParam;
  Check function("decompose_flow()");
  Check functor("Decompose_Flow");
  Check stats("compute_flow_statistics()");
  Check refuse("decompose_flow() without one source and one sink, again");
  // The arcs in the order of for_each_spec(): the reverse is in the test of
  // the flows with cycles
  for_each_flow_to_decompose<Net>(refuse, false, [&] (const Spec & s, Built_Net<Net> & b,
                                                      const std::string & at)
    {
      edmonds_karp_maximum_flow(b.net);
      const auto after = b.flows();
      function.run();
      if (auto e = decomposition_error(b, decompose_flow(b.net)); not e.empty())
        function.fail(s, at + e);
      else if (b.flows() != after)
        function.fail(s, at + "changes the flows");

      functor.run();
      if (auto e = decomposition_error(b, Decompose_Flow<Net>()(b.net)); not e.empty())
        functor.fail(s, at + e);

      stats.run();
      if (auto e = statistics_error(b, compute_flow_statistics(b.net)); not e.empty())
        stats.fail(s, at + e);
    });
  for (const Check * c : {&function, &functor, &stats, &refuse})
    c->expect();
}

// ======================================================================
// Augmenting paths

namespace
{
  /// Whether the residual network of the flow in b has a path from u to v
  /// whose arcs keep each more than 0 and at least slack: an arc forward
  /// keeps its capacity less its flow, and backward its flow
  template <class Net>
  bool residual_path(const Built_Net<Net> & b, typename Net::Node * u, typename Net::Node * v,
                     typename Net::Flow_Type slack)
  {
    using F = typename Net::Flow_Type;
    auto enough = [slack] (F x) { return x > F{0} and x >= slack; };
    std::set<typename Net::Node *> r = {u};
    for (bool grew = true; grew; )
      {
        grew = false;
        for (auto a : b.arcs)
          {
            auto p = b.net.get_src_node(a), q = b.net.get_tgt_node(a);
            if (r.count(p) and not r.count(q) and enough(a->cap - a->flow))
              grew = r.insert(q).second;
            else if (r.count(q) and not r.count(p) and enough(a->flow))
              grew = r.insert(p).second;
          }
      }
    return r.count(v) > 0;
  }

  /// What is wrong with p as an augmenting path of the flow in b from u
  /// to v whose arcs keep each at least slack; empty if nothing
  template <class Net>
  std::string augmenting_path_error(const Built_Net<Net> & b, const Path<Net> & p,
                                    typename Net::Node * u, typename Net::Node * v,
                                    typename Net::Flow_Type slack)
  {
    using F = typename Net::Flow_Type;
    std::vector<typename Net::Node *> ns;
    std::vector<typename Net::Arc *> as;
    for (typename Path<Net>::Iterator it(p); it.has_current_node(); it.next_ne())
      {
        ns.push_back(it.get_current_node_ne());
        if (it.has_current_arc())
          as.push_back(it.get_current_arc_ne());
      }
    if (ns.empty() or ns.front() != u or ns.back() != v or as.size() + 1 != ns.size())
      return "the path does not go from the start to the end";
    for (size_t k = 0; k < as.size(); ++k)
      {
        auto a = as[k];
        if (b.net.get_connected_node(a, ns[k]) != ns[k + 1])
          return "an arc of the path does not join its nodes";
        const F keep = remaining_flow<Net>(ns[k], a);
        if (not (keep > F{0} and keep >= slack))
          return "the path takes the arc " + name_of(b.net.get_src_node(a)) + "->"
            + name_of(b.net.get_tgt_node(a)) + ", which keeps " + text(keep) + " from "
            + name_of(ns[k]) + ", not " + text(slack);
      }
    return "";
  }

  /// What is wrong with the semi-path sp, from u to v, as an augmenting
  /// path of the flow in b whose least slack is given and at least slack;
  /// empty if nothing
  template <class Net>
  std::string semi_path_error(const Built_Net<Net> & b, const DynList<Parc<Net>> & sp,
                              typename Net::Node * u, typename Net::Node * v,
                              typename Net::Flow_Type given, typename Net::Flow_Type slack)
  {
    using F = typename Net::Flow_Type;
    auto at = u;
    F least = std::numeric_limits<F>::max();
    if (sp.is_empty())
      return "the semi-path is empty";
    for (auto it = sp.get_it(); it.has_curr(); it.next_ne())
      {
        auto [a, forward] = it.get_curr();
        F keep;
        if (forward)
          {
            if (b.net.get_src_node(a) != at)
              return "an arc forward does not leave the node where the semi-path is";
            keep = a->cap - a->flow;
            at = b.net.get_tgt_node(a);
          }
        else
          {
            if (b.net.get_tgt_node(a) != at)
              return "an arc backward does not enter the node where the semi-path is";
            keep = a->flow;
            at = b.net.get_src_node(a);
          }
        least = std::min(least, keep);
      }
    if (at != v)
      return "the semi-path does not end at the end";
    if (least != given)
      return "the slack is " + text(given) + ", not " + text(least);
    if (not (least > F{0} and least >= slack))
      return "the slack " + text(least) + " is less than " + text(slack);
    return "";
  }
}

TYPED_TEST(ExhaustiveMaxFlow, AugmentingPathSearchesMatchTheOracle)
{
  using Net = TypeParam;
  using F = typename Net::Flow_Type;
  using Node = typename Net::Node;
  Check dfs("Find_Aumenting_Path_DFS(start, end, slack)");
  Check bfs("Find_Aumenting_Path_BFS(start, end, slack)");
  Check free_dfs("find_aumenting_path_dfs()");
  Check free_bfs("find_aumenting_path_bfs()");
  Check increase("increase_flow(net, path)");
  Check semi_dfs("Find_Aumenting_Path_DFS(slack)");
  Check semi_bfs("Find_Aumenting_Path_BFS(slack)");
  Check semi_free("find_aumenting_semi_path_dfs() and _bfs()");
  Check semi_fill("Find_Aumenting_Path_BFS::semi_path()");
  Check dec("find_decrementing_path_dfs() and _bfs()");
  Check dec_member("Find_Aumenting_Path_DFS::find_dec_path()");
  Check round("increase_flow(net, semi_path, slack) and decrease_flow()");

  small_structural_specs<Net>()([&] (const Spec & s)
    {
      for (auto [a, z] : ends_of(s))
        {
          if (a == z)
            continue;
          Built_Net<Net> b(s);
          b.pin(a, z);
          for (long long units : {0LL, 1LL, 2LL})
            {
              put_initial_flow(b, false);
              const F slack = amount<F>(units);
              const std::string at = where(b, a, z) + "slack " + text(slack) + ": ";
              Node * u = b.src, * v = b.snk;
              const bool want = residual_path(b, u, v, slack);
              const bool want_back = residual_path(b, v, u, slack);

              auto check_path = [&] (Check & check, const Path<Net> & p)
                {
                  check.run();
                  if (p.is_empty() == want)
                    check.fail(s, at + (want ? "finds no path" : "finds a path"));
                  else if (want)
                    if (auto e = augmenting_path_error(b, p, u, v, slack); not e.empty())
                      check.fail(s, at + e);
                };
              check_path(dfs, Find_Aumenting_Path_DFS<Net>(b.net)(u, v, slack));
              check_path(bfs, Find_Aumenting_Path_BFS<Net>(b.net)(u, v, slack));
              check_path(free_dfs, find_aumenting_path_dfs(b.net, slack));
              check_path(free_bfs, find_aumenting_path_bfs(b.net, slack));

              // The slack that increase_flow() adds is the least of the path
              if (want)
                {
                  Check & check = increase;
                  check.run();
                  const Path<Net> p = find_aumenting_path_bfs(b.net, slack);
                  F least = std::numeric_limits<F>::max();
                  for (typename Path<Net>::Iterator it(p); it.has_current_arc(); it.next_ne())
                    least = std::min(least, remaining_flow<Net>(it.get_current_node_ne(),
                                                                it.get_current_arc_ne()));
                  const F before = b.value();
                  const F added = increase_flow(b.net, p);
                  if (added != least or b.value() != before + added)
                    check.fail(s, at + "adds " + text(added) + " for a least slack of "
                               + text(least));
                  else if (not (added >= slack and added > F{0}))
                    check.fail(s, at + "adds " + text(added) + ", less than the slack asked");
                  else if (auto e = flow_error(b); not e.empty())
                    check.fail(s, at + e);
                  put_initial_flow(b, false);
                }

              auto check_semi = [&] (Check & check, const SemiPath<Net> & sp, Node * from,
                                     Node * to, bool expected)
                {
                  check.run();
                  if (std::get<0>(sp) != expected)
                    check.fail(s, at + (expected ? "finds no semi-path" : "finds a semi-path"));
                  else if (expected)
                    if (auto e = semi_path_error(b, std::get<2>(sp), from, to, std::get<1>(sp),
                                                 slack); not e.empty())
                      check.fail(s, at + e);
                };
              check_semi(semi_dfs, Find_Aumenting_Path_DFS<Net>(b.net)(slack), u, v, want);
              check_semi(semi_bfs, Find_Aumenting_Path_BFS<Net>(b.net)(slack), u, v, want);
              check_semi(semi_free, find_aumenting_semi_path_dfs(b.net, slack), u, v, want);
              check_semi(semi_free, find_aumenting_semi_path_bfs(b.net, slack), u, v, want);
              check_semi(dec, find_decrementing_path_dfs(b.net, slack), v, u, want_back);
              check_semi(dec, find_decrementing_path_bfs(b.net, slack), v, u, want_back);
              check_semi(dec_member, Find_Aumenting_Path_DFS<Net>(b.net).find_dec_path(slack),
                         v, u, want_back);

              semi_fill.run();
              {
                DynList<Parc<Net>> sp;
                const F got = Find_Aumenting_Path_BFS<Net>(b.net).semi_path(u, v, sp, slack);
                if (want == sp.is_empty())
                  semi_fill.fail(s, at + (want ? "finds no semi-path" : "finds a semi-path"));
                else if (want)
                  {
                    if (auto e = semi_path_error(b, sp, u, v, got, slack); not e.empty())
                      semi_fill.fail(s, at + e);
                  }
                else if (got != F{0})
                  semi_fill.fail(s, at + "returns " + text(got) + " without a semi-path");
              }

              // Adding the slack of a semi-path and taking it away leaves
              // the flow as it was
              if (want)
                {
                  round.run();
                  const auto before = b.flows();
                  const F value = b.value();
                  const auto sp = find_aumenting_semi_path_bfs(b.net, slack);
                  increase_flow(b.net, std::get<2>(sp), std::get<1>(sp));
                  if (auto e = flow_error(b); not e.empty())
                    round.fail(s, at + "after increase_flow(): " + e);
                  else if (b.value() != value + std::get<1>(sp))
                    round.fail(s, at + "the value does not grow by the slack");
                  decrease_flow(b.net, std::get<2>(sp), std::get<1>(sp));
                  if (b.flows() != before)
                    round.fail(s, at + "decrease_flow() does not undo increase_flow()");
                }
            }
        }
    });
  for (const Check * c : {&dfs, &bfs, &free_dfs, &free_bfs, &increase, &semi_dfs, &semi_bfs,
                          &semi_free, &semi_fill, &dec, &dec_member, &round})
    c->expect();
}

// ======================================================================
// Super sources and sinks

namespace
{
  /// The numbers of the nodes of the spec that the sets give
  template <class Net>
  std::set<int> numbers_of(const DynSetTree<typename Net::Node *> & set)
  {
    std::set<int> r;
    for (auto it = set.get_it(); it.has_curr(); it.next_ne())
      r.insert(it.get_curr()->get_info());
    return r;
  }

  /// The sources and the sinks of s, by its arcs; a loop is an arc in
  std::pair<std::set<int>, std::set<int>> ends_by_arcs(const Spec & s)
  {
    std::set<int> sources, sinks;
    for (int x = 0; x < s.n; ++x)
      {
        sources.insert(x);
        sinks.insert(x);
      }
    for (const auto & a : s.arcs)
      {
        sources.erase(a[1]);
        sinks.erase(a[0]);
      }
    return {sources, sinks};
  }

  /// The maximum flow from the sources of s to its sinks, through a super
  /// source with an arc to each source of its capacity out, and a super
  /// sink likewise, where there are several of them
  long long super_max_flow(const Spec & s)
  {
    auto [sources, sinks] = ends_by_arcs(s);
    Spec x = s;
    int from = sources.size() == 1 ? *sources.begin() : x.n++;
    int to = sinks.size() == 1 ? *sinks.begin() : x.n++;
    if (sources.size() > 1)
      for (int v : sources)
        {
          long long out = 0;
          for (size_t i = 0; i < s.arcs.size(); ++i)
            out += s.arcs[i][0] == v ? s.w[i] : 0;
          x.arcs.push_back({from, v});
          x.w.push_back(out);
        }
    if (sinks.size() > 1)
      for (int v : sinks)
        {
          long long in = 0;
          for (size_t i = 0; i < s.arcs.size(); ++i)
            in += s.arcs[i][1] == v ? s.w[i] : 0;
          // The super source may have an arc to v
          for (size_t i = s.arcs.size(); i < x.arcs.size(); ++i)
            in += x.arcs[i][1] == v ? x.w[i] : 0;
          x.arcs.push_back({v, to});
          x.w.push_back(in);
        }
    return max_flow_units(x, from, to);
  }
}

TYPED_TEST(ExhaustiveMaxFlow, SuperSourcesAndSinksMatchTheOracle)
{
  using Net = TypeParam;
  Check make("make_super_nodes(), and a maximum flow");
  Check refuse("make_super_source() and make_super_sink() without sources or sinks");
  Check unmake("unmake_super_nodes()");
  Check one_by_one("make_super_source() and make_super_sink()");

  structural_specs(max_flow_domain<Net>())([&] (const Spec & s)
    {
      const auto [sources, sinks] = ends_by_arcs(s);
      if (sources.empty() or sinks.empty())
        {
          refuse.run();
          Built_Net<Net> b(s);
          try
            {
              b.net.make_super_nodes();
              refuse.fail(s, "does not throw");
            }
          catch (const std::domain_error &) {}
          catch (const std::exception & e)
            {
              refuse.fail(s, std::string("throws ") + e.what());
            }
          return;
        }

      make.run();
      Built_Net<Net> b(s);
      try
        {
          b.net.make_super_nodes();
        }
      catch (const std::exception & e)
        {
          make.fail(s, std::string("throws ") + e.what());
          return;
        }
      const size_t added = (sources.size() > 1) + (sinks.size() > 1);
      if (b.net.get_num_nodes() != size_t(s.n) + added or not b.net.is_single_source()
          or not b.net.is_single_sink() or b.net.with_super_source != (sources.size() > 1)
          or b.net.with_super_sink != (sinks.size() > 1))
        {
          make.fail(s, "a wrong network: " + std::to_string(b.net.get_num_nodes()) + " nodes, "
                    + std::to_string(b.net.get_src_nodes().size()) + " sources, "
                    + std::to_string(b.net.get_sink_nodes().size()) + " sinks");
          return;
        }
      b.src = b.net.get_source();
      b.snk = b.net.get_sink();
      for (typename Net::Arc_Iterator it(b.net); it.has_curr(); it.next_ne())
        if (b.number(it.get_curr()) < 0)
          b.arcs.push_back(it.get_curr());
      const auto got = edmonds_karp_maximum_flow(b.net);
      if (auto e = max_flow_error(b, got, super_max_flow(s)); not e.empty())
        make.fail(s, e);

      unmake.run();
      std::vector<typename Net::Flow_Type> spec_flows;
      for (size_t i = 0; i < b.spec_arcs; ++i)
        spec_flows.push_back(b.arcs[i]->flow);
      b.net.unmake_super_nodes();
      bool same_flows = true;
      for (size_t i = 0; i < b.spec_arcs; ++i)
        same_flows = same_flows and b.arcs[i]->flow == spec_flows[i];
      if (b.net.get_num_nodes() != size_t(s.n) or b.net.get_num_arcs() != s.num_arcs()
          or numbers_of<Net>(b.net.get_src_nodes()) != sources
          or numbers_of<Net>(b.net.get_sink_nodes()) != sinks or b.net.with_super_source
          or b.net.with_super_sink or not same_flows)
        unmake.fail(s, "the network is not as it was");

      // One by one, the same
      one_by_one.run();
      Built_Net<Net> c(s);
      c.net.make_super_source();
      c.net.make_super_sink();
      if (not c.net.is_single_source() or not c.net.is_single_sink())
        one_by_one.fail(s, "more than one source or sink");
      else
        {
          c.net.unmake_super_sink();
          c.net.unmake_super_source();
          if (numbers_of<Net>(c.net.get_src_nodes()) != sources
              or numbers_of<Net>(c.net.get_sink_nodes()) != sinks
              or c.net.get_num_nodes() != size_t(s.n))
            one_by_one.fail(s, "the network is not as it was");
        }
    });
  make.expect();
  refuse.expect();
  unmake.expect();
  one_by_one.expect();
}

// ======================================================================
// Minimum costs

namespace
{
  /// Whether x is exactly units2 units of cost times units of flow: 1 for
  /// the integers, 1/16 for the floating types
  template <typename F>
  bool is_cost(F x, long long units2)
  {
    return x * F(Units_Per_One<F>) * F(Units_Per_One<F>) == F(units2);
  }

  template <typename F>
  F cost_amount(long long units2)
  {
    return F(units2) / F(Units_Per_One<F> * Units_Per_One<F>);
  }

  /// The maximum flow from a to z, with the capacities in s.w, and the
  /// least cost of such a flow with cost, by brute force over the integer
  /// flows of the arcs (the cheapest maximum flow has integer flows)
  std::pair<long long, long long> brute_min_cost(const Spec & s,
                                                 const std::vector<long long> & cost, int a, int z)
  {
    std::vector<long long> f(s.arcs.size(), 0);
    long long best_value = -1, best_cost = 0;
    while (true)
      {
        std::vector<long long> balance(size_t(s.n), 0);
        for (size_t i = 0; i < f.size(); ++i)
          {
            balance[size_t(s.arcs[i][0])] -= f[i];
            balance[size_t(s.arcs[i][1])] += f[i];
          }
        bool kept = -balance[size_t(a)] >= 0;
        for (int x = 0; x < s.n and kept; ++x)
          kept = x == a or x == z or balance[size_t(x)] == 0;
        if (kept)
          {
            long long c = 0;
            for (size_t i = 0; i < f.size(); ++i)
              c += f[i] * cost[i];
            const long long value = -balance[size_t(a)];
            if (value > best_value or (value == best_value and c < best_cost))
              {
                best_value = value;
                best_cost = c;
              }
          }
        size_t i = 0;
        while (i < f.size() and ++f[i] > s.w[i])
          f[i++] = 0;
        if (i == f.size())
          return {best_value, best_cost};
      }
  }

  /// The same, for networks too large for the brute force, by successive
  /// shortest paths with Bellman-Ford on the residual arcs; the costs are
  /// not negative, so the residual network never has a negative cycle
  std::pair<long long, long long> ssp_min_cost(const Spec & s, const std::vector<long long> & cost,
                                               int a, int z)
  {
    struct R { int u, v; long long cap, cost; size_t back; };
    std::vector<R> r;
    for (size_t i = 0; i < s.arcs.size(); ++i)
      if (s.arcs[i][0] != s.arcs[i][1])
        {
          r.push_back({s.arcs[i][0], s.arcs[i][1], s.w[i], cost[i], r.size() + 1});
          r.push_back({s.arcs[i][1], s.arcs[i][0], 0, -cost[i], r.size() - 1});
        }
    long long value = 0, total = 0;
    while (true)
      {
        std::vector<long long> d(size_t(s.n), oracle::Inf);
        std::vector<size_t> by(size_t(s.n), r.size());
        d[size_t(a)] = 0;
        for (int round = 0; round < s.n; ++round)
          for (size_t k = 0; k < r.size(); ++k)
            if (r[k].cap > 0 and d[size_t(r[k].u)] != oracle::Inf
                and d[size_t(r[k].u)] + r[k].cost < d[size_t(r[k].v)])
              {
                d[size_t(r[k].v)] = d[size_t(r[k].u)] + r[k].cost;
                by[size_t(r[k].v)] = k;
              }
        if (d[size_t(z)] == oracle::Inf)
          return {value, total};
        long long neck = oracle::Inf;
        for (int v = z; v != a; v = r[by[size_t(v)]].u)
          neck = std::min(neck, r[by[size_t(v)]].cap);
        for (int v = z; v != a; v = r[by[size_t(v)]].u)
          {
            r[by[size_t(v)]].cap -= neck;
            r[r[by[size_t(v)]].back].cap += neck;
          }
        value += neck;
        total += neck * d[size_t(z)];
      }
  }

  /// A minimum cost flow algorithm, by its name: it runs on the network
  /// from the flow in it, and gives the flow and cost that it returns, if
  /// it returns them; again is the form that runs it a second time, its
  /// functor where it has one
  template <class Net>
  struct Cost_Solver
  {
    using F = typename Net::Flow_Type;
    std::string name;
    std::pair<F, F> (*run)(Net &);
    std::pair<F, F> (*again)(Net &);
    bool returns = false;
  };

  /// The cycle canceling (0 and 1), the network simplex (2 and 3) and the
  /// successive shortest paths (4)
  template <class Net>
  std::vector<Cost_Solver<Net>> cost_solvers()
  {
    using F = typename Net::Flow_Type;
    using P = std::pair<F, F>;
    return {
      {"max_flow_min_cost_by_cycle_canceling()",
       [] (Net & n) { max_flow_min_cost_by_cycle_canceling(n); return P(); },
       [] (Net & n) { Max_Flow_Min_Cost_By_Cycle_Canceling<Net>()(n); return P(); }},
      {"max_flow_min_cost_by_cycle_canceling<Edmonds_Karp_Maximum_Flow>()",
       [] (Net & n) { max_flow_min_cost_by_cycle_canceling<Net, Edmonds_Karp_Maximum_Flow>(n);
                      return P(); },
       [] (Net & n) { Max_Flow_Min_Cost_By_Cycle_Canceling<Net, Edmonds_Karp_Maximum_Flow>()(n);
                      return P(); }},
      {"max_flow_min_cost_by_network_simplex()",
       [] (Net & n) { max_flow_min_cost_by_network_simplex(n); return P(); },
       [] (Net & n) { Max_Flow_Min_Cost_By_Network_Simplex<Net>()(n); return P(); }},
      {"max_flow_min_cost_by_network_simplex<Dinic_Maximum_Flow>()",
       [] (Net & n) { max_flow_min_cost_by_network_simplex<Net, Dinic_Maximum_Flow>(n);
                      return P(); },
       [] (Net & n) { Max_Flow_Min_Cost_By_Network_Simplex<Net, Dinic_Maximum_Flow>()(n);
                      return P(); }},
      {"successive_shortest_paths()", [] (Net & n) { return successive_shortest_paths(n); },
       [] (Net & n) { return Successive_Shortest_Paths<Net>()(n); }, true}};
  }

  bool is_cycle_canceling(size_t i) { return i <= 1; }
  bool is_ssp(size_t i) { return i == 4; }

  /// What is wrong with the flow in b as a cheapest maximum flow of value
  /// and cost want, from an algorithm that returned got, if it returns;
  /// empty if nothing
  template <class Net>
  std::string min_cost_error(const Built_Net<Net> & b, const Cost_Solver<Net> & solver,
                             const std::pair<typename Net::Flow_Type,
                                             typename Net::Flow_Type> & got,
                             std::pair<long long, long long> want)
  {
    using F = typename Net::Flow_Type;
    if (auto e = flow_error(b); not e.empty())
      return e;
    if (not is_amount(b.value(), want.first))
      return "the flow is " + text(b.value()) + ", not " + text(amount<F>(want.first));
    if (not is_cost(b.net.flow_cost(), want.second))
      return "the cost is " + text(b.net.flow_cost()) + ", not "
        + text(cost_amount<F>(want.second));
    if (solver.returns and (got.first != b.value() or got.second != b.net.flow_cost()))
      return "returns " + text(got.first) + " and " + text(got.second) + " for a flow of "
        + text(b.value()) + " and cost " + text(b.net.flow_cost());
    return "";
  }

  /// The costs, for the messages
  std::string costs_text(const std::vector<long long> & cost)
  {
    std::string r = "costs {";
    for (size_t i = 0; i < cost.size(); ++i)
      r += (i == 0 ? "" : " ") + std::to_string(cost[i]);
    return r + "}: ";
  }

  /** Whether the residual network of the flow in b has a cycle of
      negative cost that its source reaches, and whether it has one at
      all, by Bellman-Ford: an arc goes forward, at its cost, if it has
      capacity left, and backward, at minus its cost, if it has flow. */
  template <class Net>
  std::pair<bool, bool> negative_residual_cycles(const Built_Net<Net> & b)
  {
    using F = typename Net::Flow_Type;
    std::map<typename Net::Node *, size_t> index;
    for (Node_Iterator<Net> it(b.net); it.has_curr(); it.next_ne())
      index.emplace(it.get_curr(), index.size());
    struct Residual
    {
      size_t u, v;
      F cost;
    };
    std::vector<Residual> r;
    for (Arc_Iterator<Net> it(b.net); it.has_curr(); it.next_ne())
      {
        auto a = it.get_curr();
        const size_t u = index[b.net.get_src_node(a)], v = index[b.net.get_tgt_node(a)];
        if (a->flow < a->cap)
          r.push_back({u, v, a->cost});
        if (a->flow > F{0})
          r.push_back({v, u, -a->cost});
      }
    // Whether the distances from the source, or from every node, still
    // change after as many passes as nodes
    auto negative = [&] (bool from_source)
      {
        const size_t n = index.size();
        std::vector<bool> reached(n, not from_source);
        std::vector<F> d(n, F{0});
        if (from_source)
          reached[index[b.src]] = true;
        for (size_t pass = 0; pass < n; ++pass)
          {
            bool changed = false;
            for (const auto & e : r)
              if (reached[e.u] and (not reached[e.v] or d[e.u] + e.cost < d[e.v]))
                {
                  d[e.v] = d[e.u] + e.cost;
                  reached[e.v] = true;
                  changed = true;
                }
            if (not changed)
              return false;
          }
        return true;
      };
    return {negative(true), negative(false)};
  }

  /// The costs of the structural tests: arc i costs 2, 0 or 1 units, by i
  /// modulo 3, so that parallel arcs differ
  std::vector<long long> structural_costs(const Spec & s)
  {
    std::vector<long long> c;
    for (size_t i = 0; i < s.arcs.size(); ++i)
      c.push_back(std::vector<long long>{2, 0, 1}[i % 3]);
    return c;
  }

  /** Runs each cost solver, from the zero flow and then, if twice, once
      more, on the network of s with the costs cost, between every pair of
      ends, and checks the runs against the cheapest maximum flow of
      min_cost. Unless variants, the cycle canceling and the network
      simplex run with their first maximum flow only: the others differ in
      that flow, which the structural tests take. */
  template <class Net, class Oracle>
  void check_min_costs(const Spec & s, const std::vector<long long> & cost, Oracle min_cost,
                       Family_Checks & first, Family_Checks & second, bool twice = true,
                       int only = -1, bool variants = true)
  {
    const auto solvers = cost_solvers<Net>();
    for (auto [a, z] : ends_of(s))
      {
        if (a == z)
          continue;
        Built_Net<Net> b(s, s.w, cost);
        b.pin(a, z);
        const auto want = min_cost(s, cost, a, z);
        for (size_t i = 0; i < solvers.size(); ++i)
          {
            // only: 0 for the cycle canceling, 1 for the others
            if (only >= 0 and is_cycle_canceling(i) != (only == 0))
              continue;
            if (not variants and (i == 1 or i == 3))
              continue;
            Check * check = first.of(i, 0);
            b.net.reset();
            check->run();
            try
              {
                const auto got = solvers[i].run(b.net);
                if (auto e = min_cost_error(b, solvers[i], got, want); not e.empty())
                  {
                    check->fail(s, where(b, a, z) + costs_text(cost) + e);
                    continue;
                  }
                if (not twice)
                  continue;
                Check * again = second.of(i, 0);
                again->run();
                const auto got2 = solvers[i].again(b.net);
                if (auto e = min_cost_error(b, solvers[i], got2, want); not e.empty())
                  again->fail(s, where(b, a, z) + costs_text(cost) + "the second run: " + e);
              }
            catch (const std::exception & e)
              {
                check->fail(s, where(b, a, z) + costs_text(cost) + "throws " + e.what());
              }
          }
      }
  }

  /// The checks of the first runs and of the second runs of the cost
  /// solvers
  template <class Net>
  struct Cost_Checks
  {
    Family_Checks first;
    Family_Checks second;

    Cost_Checks()
      : first(cost_solvers<Net>()),
        second(second_names())
    {}

    static std::vector<Solver<Net>> second_names()
    {
      std::vector<Solver<Net>> r;
      for (const auto & s : cost_solvers<Net>())
        r.push_back({s.name + ", then its functor", nullptr});
      return r;
    }

    void expect() const
    {
      first.expect();
      second.expect();
    }
  };

  /// The smallest networks found where the network simplex was not
  /// cheapest, as it took no directed cycle as a pivot, with their costs:
  /// after Ford-Fulkerson, from 2 to 1, with the directed cycle
  /// 0->1->2->0 and two arcs 0->1, the dearer one with the flow; and after
  /// Ford-Fulkerson and after Dinic, from 0 to 3 and from 2 to 0, where the
  /// tree takes an arc of no capacity that closes a directed cycle. The
  /// tests of the network simplex start with them.
  std::vector<std::pair<Spec, std::vector<long long>>> simplex_witnesses()
  {
    std::vector<std::pair<Spec, std::vector<long long>>> r;
    Spec s;
    s.n = 3;
    s.arcs = {{2, 0}, {1, 2}, {0, 1}, {0, 1}};
    s.w = {1, 2, 3, 4};
    r.push_back({s, {2, 0, 1, 2}});
    s.n = 6;
    s.arcs = {{3, 2}, {4, 3}, {3, 0}, {5, 0}, {0, 4}, {4, 3}, {1, 4}};
    s.w = {0, 1, 0, 0, 1, 1, 0};
    r.push_back({s, {0, 0, 0, 0, 0, 1, 0}});
    s.arcs = {{1, 4}, {3, 0}, {1, 4}, {2, 1}, {4, 3}, {4, 2}};
    s.w = {1, 1, 1, 1, 1, 0};
    r.push_back({s, {1, 0, 0, 0, 0, 0}});
    return r;
  }
}

template <class Net>
class ExhaustiveMinCost : public ::testing::Test {};

/// The networks with costs, of integers and of floating flows
using Cost_Nets =
  ::testing::Types<Net_Cost_Graph<Net_Cost_Node<int>, Net_Cost_Arc<Empty_Class, long>>,
                   Net_Cost_Graph<Net_Cost_Node<int>, Net_Cost_Arc<Empty_Class, double>>>;

TYPED_TEST_SUITE(ExhaustiveMinCost, Cost_Nets);

TYPED_TEST(ExhaustiveMinCost, CycleCancelingMatchesTheOracle)
{
  using Net = TypeParam;
  Cost_Checks<Net> checks;
  small_structural_specs<Net>()([&] (const Spec & s)
    {
      check_min_costs<Net>(s, structural_costs(s), brute_min_cost, checks.first, checks.second,
                           true, 0);
    });
  checks.expect();
}

TYPED_TEST(ExhaustiveMinCost, SimplexAndShortestPathsMatchTheOracle)
{
  using Net = TypeParam;
  Cost_Checks<Net> checks;
  for (const auto & [s, cost] : simplex_witnesses())
    check_min_costs<Net>(s, cost, ssp_min_cost, checks.first, checks.second, true, 1);
  small_structural_specs<Net>()([&] (const Spec & s)
    {
      check_min_costs<Net>(s, structural_costs(s), brute_min_cost, checks.first, checks.second,
                           true, 1);
    });
  checks.expect();
}

TYPED_TEST(ExhaustiveMinCost, CheapestMaxFlowsTakeEveryCost)
{
  using Net = TypeParam;
  Cost_Checks<Net> checks;
  for (const auto & [s, cost] : simplex_witnesses())
    check_min_costs<Net>(s, cost, ssp_min_cost, checks.first, checks.second, false);
  // Unit capacities, and every assignment of the costs 0 and 2, once
  for_each_spec(class_domain(), true, [&] (const Spec & s)
    {
      const Spec unit = with_w(s, std::vector<long long>(s.arcs.size(), 1));
      for_each_assignment(s, {0, 2}, [&] (const Spec & c)
        {
          check_min_costs<Net>(unit, c.w, brute_min_cost, checks.first, checks.second, false,
                               -1, false);
        });
    });
  checks.expect();
}

TYPED_TEST(ExhaustiveMinCost, CheapestMaxFlowsOnRandomNetworks)
{
  using Net = TypeParam;
  Cost_Checks<Net> checks;

  for (const auto & [s, cost] : simplex_witnesses())
    check_min_costs<Net>(s, cost, ssp_min_cost, checks.first, checks.second);

  std::mt19937 rng(seed(21));
  for (int k = 0; k < (heavy() ? 200 : 20); ++k)
    {
      Spec s = random_spec(rng, 6 + int(rng() % 3), 8 + int(rng() % 9), true);
      random_weights(rng, s, 0, 3);
      Spec c = s;
      random_weights(rng, c, 0, 5);
      check_min_costs<Net>(s, c.w, ssp_min_cost, checks.first, checks.second, true, -1, false);
    }
  checks.expect();
}

TYPED_TEST(ExhaustiveMinCost, CheapestMaxFlowsFromAFlowAlreadyThere)
{
  using Net = TypeParam;
  // Every solver starts from the flow in the network; the successive
  // shortest paths need a residual network without cycles of negative
  // cost, and refuse it if the source reaches one: when one is apart from
  // the source, what they do is not defined, and it is not checked. The
  // cycle canceling and the network simplex run with their first maximum
  // flow only, as the others start from a flow in the tests of the
  // maximum flows
  const auto solvers = cost_solvers<Net>();
  Family_Checks checks(solvers);
  small_structural_specs<Net>()([&] (const Spec & s)
    {
      const auto cost = structural_costs(s);
      for (auto [a, z] : ends_of(s))
        {
          if (a == z)
            continue;
          Built_Net<Net> b(s, s.w, cost);
          b.pin(a, z);
          const auto want = brute_min_cost(s, cost, a, z);
          for (size_t i = 0; i < solvers.size(); ++i)
            {
              if (i == 1 or i == 3)
                continue;
              put_initial_flow(b);
              bool refused = false;
              if (is_ssp(i))
                {
                  const auto [reached, anywhere] = negative_residual_cycles(b);
                  if (anywhere and not reached)
                    continue;
                  refused = reached;
                }
              Check * check = checks.of(i, 0);
              check->run();
              if (refused)
                {
                  const auto before = b.flows();
                  try
                    {
                      solvers[i].run(b.net);
                      check->fail(s, where(b, a, z) + costs_text(cost)
                                  + "from a flow whose residual network has a cycle of"
                                  " negative cost: does not throw");
                    }
                  catch (const std::domain_error &)
                    {
                      if (b.flows() != before)
                        check->fail(s, where(b, a, z) + costs_text(cost)
                                    + "throws, and the flows changed");
                    }
                  catch (const std::exception & e)
                    {
                      check->fail(s, where(b, a, z) + costs_text(cost) + "throws " + e.what());
                    }
                  continue;
                }
              try
                {
                  const auto got = solvers[i].run(b.net);
                  if (auto e = min_cost_error(b, solvers[i], got, want); not e.empty())
                    check->fail(s, where(b, a, z) + costs_text(cost) + "from a flow: " + e);
                }
              catch (const std::exception & e)
                {
                  check->fail(s, where(b, a, z) + costs_text(cost) + "throws " + e.what());
                }
            }
        }
    });
  checks.expect();
}

// ======================================================================
// Floating flows: tenths, real numbers and capacities next to each other

namespace
{
  using Double_Net = Net_Graph<Net_Node<int>, Net_Arc<int, double>>;
  using Double_Built = Built_Net<Double_Net>;

  /// The exact rationals of the doubles
  std::vector<mpq_class> exactly(const std::vector<double> & x)
  {
    std::vector<mpq_class> r;
    for (double v : x)
      r.emplace_back(v);
    return r;
  }

  /// Whether x is within 1e-9, relative, of the exact q
  bool near(double x, const mpq_class & q)
  {
    const double e = q.get_d();
    return std::abs(x - e) <= 1e-9 * std::max(1.0, std::abs(e));
  }

  /// The capacity of a minimum cut between a and z, in rationals
  mpq_class exact_min_cut(const Spec & s, const std::vector<mpq_class> & cap, int a, int z)
  {
    mpq_class best = -1;
    for (unsigned long set = 0; set < (1UL << s.n); ++set)
      {
        if (((set >> a) & 1UL) == 0 or ((set >> z) & 1UL) != 0)
          continue;
        mpq_class cut = 0;
        for (size_t i = 0; i < s.arcs.size(); ++i)
          if (((set >> s.arcs[i][0]) & 1UL) != 0 and ((set >> s.arcs[i][1]) & 1UL) == 0)
            cut += cap[i];
        if (best < 0 or cut < best)
          best = cut;
      }
    return best;
  }

  /// What is wrong with the floating flow in b, which the algorithm said is
  /// got, as a maximum flow near want: an arc out of [0, cap] (exactly),
  /// a node that does not keep its flow, within the tolerance of
  /// check_network(), or a value far from want; empty if nothing
  std::string floating_error(const Double_Built & b, double got, const mpq_class & want)
  {
    if (auto e = flow_error(b, true); not e.empty())
      return e;
    if (not near(b.value(), want))
      return "the flow is " + text(b.value()) + ", not near " + text(want.get_d());
    if (not near(got, mpq_class(b.value())))
      return "returns " + text(got) + " for a flow of " + text(b.value());
    return "";
  }

  /// The floating solvers: every maximum flow, in the order of
  /// all_solvers()
  std::vector<Solver<Double_Net>> floating_solvers()
  {
    return all_solvers<Double_Net>();
  }

  /// Calls f(spec, capacities) with every spec of 3 nodes and 3 arcs
  /// without loops, the arcs in one order, and each assignment of the
  /// tenths 0.1, 0.2 and 0.3
  template <class F>
  void for_each_tenths_network(F f)
  {
    for_each_spec(class_domain(), true, [&] (const Spec & s)
      {
        for_each_assignment(s, {1, 2, 3}, [&] (const Spec & t)
          {
            std::vector<double> cap;
            for (long long k : t.w)
              cap.push_back(double(k) / 10);
            f(t, cap);
          });
      });
  }

}

TEST(ExhaustiveFloatingFlow, TenthsStayNearTheExactMaximum)
{
  const auto solvers = floating_solvers();
  Family_Checks checks(solvers);

  for_each_tenths_network([&] (const Spec & s, const std::vector<double> & cap)
    {
      const auto exact = exactly(cap);
      for (auto [a, z] : ends_of(s))
        {
          Double_Built b(s, cap, Double_Built::Amounts{});
          b.pin(a, z);
          const mpq_class want = a == z ? mpq_class(0) : exact_max_flow(s, exact, a, z);
          for (size_t i = 0; i < solvers.size(); ++i)
            {
              Check * check = checks.of(i, 0);
              b.net.reset();
              check->run();
              const double got = solvers[i].run(b.net);
              if (auto e = floating_error(b, got, want); not e.empty())
                check->fail(s, where(b, a, z) + e);
            }
        }
    });
  checks.expect();
}

TEST(ExhaustiveFloatingFlow, CutsDecompositionsAndStatisticsOfTenths)
{
  Check cut("min_cut<Edmonds_Karp_Maximum_Flow>(), with tenths");
  Check stats("compute_flow_statistics(), with tenths");
  Check decomposition("decompose_flow(), with tenths");

  for_each_tenths_network([&] (const Spec & s, const std::vector<double> & cap)
    {
      const auto exact = exactly(cap);
      for (auto [a, z] : ends_of(s))
        {
          if (a == z)
            continue;
          cut.run();
          Double_Built b(s, cap, Double_Built::Amounts{});
          b.pin(a, z);
          Node_Set<Double_Net> vs, vt;
          Arc_List<Double_Net> cs, ct;
          const double got = min_cut<Double_Net, Edmonds_Karp_Maximum_Flow>(b.net, vs, vt, cs, ct);
          if (not near(got, exact_min_cut(s, exact, a, z)))
            cut.fail(s, where(b, a, z) + "returns " + text(got));
          else if (vs.size() + vt.size() != b.net.get_num_nodes() or not vs.contains(b.src)
                   or not vt.contains(b.snk))
            cut.fail(s, where(b, a, z) + "the sides are not a partition");

          stats.run();
          const auto st = compute_flow_statistics(b.net);
          if (not near(st.total_flow, mpq_class(b.value())))
            stats.fail(s, where(b, a, z) + "the flow is " + text(st.total_flow));

          // The flow of Edmonds-Karp, which its nodes keep but for rounding
          decomposition.run();
          const auto before = b.flows();
          if (auto e = decomposition_error(b, decompose_flow(b.net), true); not e.empty())
            decomposition.fail(s, where(b, a, z) + e);
          else if (b.flows() != before)
            decomposition.fail(s, where(b, a, z) + "changes the flows");
        }
    });
  for (const Check * c : {&cut, &stats, &decomposition})
    c->expect();
}

namespace
{
  /** Runs every floating solver on the network of s with the capacities
      cap, between each pair of ends, against the exact maximum flow; the
      failures show the spec shown. The value may be off by scale, the
      rounding of the largest flows that the network can carry, if larger
      than the tolerance of near(). */
  void check_floating(const std::vector<Solver<Double_Net>> & solvers, Family_Checks & checks,
                      const Spec & s, const std::vector<double> & cap, const Spec & shown,
                      const std::vector<std::pair<int, int>> & ends, double scale = 0)
  {
    const auto exact = exactly(cap);
    for (auto [a, z] : ends)
      {
        Double_Built b(s, cap, Double_Built::Amounts{});
        b.pin(a, z);
        const mpq_class want = a == z ? mpq_class(0) : exact_max_flow(s, exact, a, z);
        for (size_t i = 0; i < solvers.size(); ++i)
          {
            Check * check = checks.of(i, 0);
            b.net.reset();
            check->run();
            const double got = solvers[i].run(b.net);
            std::string e;
            if (scale == 0)
              e = floating_error(b, got, want);
            else if (e = flow_error(b, true); e.empty())
              {
                if (std::abs(b.value() - want.get_d()) > scale)
                  e = "the flow is " + text(b.value()) + ", not within " + text(scale) + " of "
                    + text(want.get_d());
                else if (got != b.value())
                  e = "returns " + text(got) + " for a flow of " + text(b.value());
              }
            if (not e.empty())
              check->fail(shown, where(b, a, z) + e);
          }
      }
  }
}

TEST(ExhaustiveFloatingFlow, RealsStayNearTheExactMaximum)
{
  const auto solvers = floating_solvers();
  Family_Checks checks(solvers);
  auto check_network = [&] (const Spec & s, const std::vector<double> & cap, const Spec & shown,
                            const std::vector<std::pair<int, int>> & ends)
    {
      check_floating(solvers, checks, s, cap, shown, ends);
    };

  // s->a (f), a->t (c), s->b (4) and b->a (4), for tenths f < c: the second
  // path adds to a->t its slack c - f, whose sum with f may round above c
  Spec two;
  two.n = 4;
  two.arcs = {{0, 1}, {1, 3}, {0, 2}, {2, 1}};
  for (int f = 1; f < 40; ++f)
    for (int c = f + 1; c < 40; ++c)
      check_network(two, {f / 10.0, c / 10.0, 4.0, 4.0}, with_w(two, {f, c, 40, 40}),
                    {{0, 3}});

  // The smallest networks found, of the random ones, where HLPP left a
  // residue of rounding beside a dead end, where HLPP and the preflow with
  // a queue left one with a maximum flow of 0 through added ends, and
  // where the random order failed the assertion of a push
  Spec dead;
  dead.n = 7;
  dead.arcs = {{1, 2}, {0, 4}, {1, 4}, {0, 4}, {1, 0}, {0, 6}, {3, 4}, {1, 4}};
  check_network(dead, {7.9158743311190074, 1.9230814882856702, 7.0493412600464511,
                       1.1462173944341092, 1.9342502520033173, 7.6346719763601429,
                       7.9868164808084732, 1.7200262856274111}, dead, {{1, 2}});
  Spec none;
  none.n = 6;
  none.arcs = {{3, 5}, {1, 5}, {3, 4}, {2, 2}, {4, 1}, {4, 2}};
  check_network(none, {8.4754518873571989, 0.033883492579154795, 2.8544016310908091,
                       5.348472093612588, 7.6055185660473672, 7.6209729593968252}, none,
                {{1, 0}});
  none.n = 8;
  none.arcs = {{3, 0}, {1, 5}, {3, 4}, {0, 3}, {1, 6}, {6, 0}, {0, 1}, {1, 0}, {4, 5}};
  check_network(none, {5.8762413240161377, 9.6866242843038588, 9.8716668953306925,
                       6.9485217634542904, 4.5520971709691489, 0.56788063415950651,
                       6.8545648904471443, 5.3250521674031983, 3.7891499589133839}, none,
                {{1, 7}});
  Spec order;
  order.n = 7;
  order.arcs = {{5, 3}, {3, 4}, {3, 0}, {0, 0}, {4, 4}, {5, 2}, {0, 6}, {6, 1}, {1, 2}, {3, 0},
                {4, 5}, {4, 3}, {6, 3}, {3, 4}, {0, 5}};
  check_network(order, {6.7274672427936437, 0.67896405431885454, 8.3171002335098816,
                        3.142334997783744, 9.7484939902099423, 4.1710752481577531,
                        0.9841479965002714, 8.4695910615758159, 0.16659269185525827,
                        8.2594276634024109, 0.081880796394979249, 3.4538564369078739,
                        2.9875620372348917, 4.8549585581071586, 9.0570840433200761},
                order, {{4, 1}});

  // Random real numbers from 0.01 to 10
  std::mt19937 rng(seed(31));
  std::uniform_real_distribution<double> real(0.01, 10.0);
  for (int k = 0; k < (heavy() ? 400 : 40); ++k)
    {
      const Spec s = random_spec(rng, 6 + int(rng() % 3), 8 + int(rng() % 9), true);
      std::vector<double> cap;
      for (size_t i = 0; i < s.arcs.size(); ++i)
        cap.push_back(real(rng));
      check_network(s, cap, s, ends_of(s));
    }
  checks.expect();
}

TEST(ExhaustiveFloatingFlow, PathsCarryTheLeastCapacityExactly)
{
  // On a path, the maximum flow is the least capacity, and no rounding is
  // needed: every arc carries exactly it with the augmenting paths, Dinic
  // and the scaling (0, 1, 5 and 6). The preflows and HLPP leave as flow
  // an excess up to a residue of rounding of the largest capacity, so that
  // an arc may carry one ulp more or less: they are taken with the
  // tolerance of check_network(), 1e-9, relative
  const auto solvers = all_solvers<Double_Net>();
  Family_Checks checks(solvers);
  const std::vector<double> close = {3.0, std::nextafter(3.0, 0.0), std::nextafter(3.0, 4.0)};

  for (int n = 2; n <= 4; ++n)
    {
      Spec s;
      s.n = n;
      for (int i = 0; i + 1 < n; ++i)
        s.arcs.push_back({i, i + 1});
      for_each_assignment(s, {0, 1, 2}, [&] (const Spec & t)
        {
          std::vector<double> cap;
          for (long long k : t.w)
            cap.push_back(close[size_t(k)]);
          const double least = *std::min_element(cap.begin(), cap.end());
          Double_Built b(s, cap, Double_Built::Amounts{});
          b.pin(0, n - 1);
          for (size_t i = 0; i < solvers.size(); ++i)
            {
              const bool exact = i <= 1 or i == 5 or i == 6;
              auto is_least = [&] (double x)
                {
                  return exact ? x == least : std::abs(x - least) <= 1e-9 * least;
                };
              Check * check = checks.of(i, 0);
              b.net.reset();
              check->run();
              const double got = solvers[i].run(b.net);
              std::string e;
              if (not exact and not b.net.check_network())
                e = "check_network() is false";
              for (size_t k = 0; k < b.spec_arcs and e.empty(); ++k)
                if (not is_least(b.arcs[k]->flow))
                  e = "the arc " + std::to_string(k) + " carries " + text(b.arcs[k]->flow)
                    + ", not " + text(least);
              if (e.empty() and not is_least(got))
                e = "returns " + text(got) + ", not " + text(least);
              if (not e.empty())
                {
                  Spec shown = t;
                  check->fail(shown, "capacities 3, 3 - ulp, 3 + ulp by 0, 1, 2: " + e);
                }
            }
        });
    }
  checks.expect();
}


TEST(ExhaustiveFloatingFlow, SmallFlowsBesideLargeCapacitiesStay)
{
  // A small flow next to a large capacity, in the network or in its own
  // node, is no residue of rounding: s->a (1e15), a->b (10), b->t (100)
  // carries 10, and likewise 1e12 next to 0.05; a node that receives 10
  // beside an idle arc of 1e15 into it sends them on; every network of 3
  // nodes and 3 arcs without loops, with each assignment of a small and a
  // large capacity, between every pair of ends; and random networks of
  // capacities far apart
  const auto solvers = floating_solvers();
  Family_Checks checks(solvers);
  Spec chain;
  chain.n = 3;
  chain.arcs = {{0, 1}, {1, 2}};
  check_floating(solvers, checks, chain, {1e15, 10.0}, with_w(chain, {0, 1}), {{0, 2}});
  check_floating(solvers, checks, chain, {1e12, 0.05}, with_w(chain, {0, 1}), {{0, 2}});
  Spec three = chain;
  three.n = 4;
  three.arcs = {{0, 1}, {1, 2}, {2, 3}};
  check_floating(solvers, checks, three, {1e15, 10.0, 100.0}, with_w(three, {0, 1, 2}), {{0, 3}});
  check_floating(solvers, checks, three, {1e12, 0.05, 1.0}, with_w(three, {0, 1, 2}), {{0, 3}});
  // 0 -> 1 -> 2 -> 3 carries 10, beside 4 -> 2 of 1e15, idle, as 4 has
  // nothing to send: the capacities of node 2 reach 1e15, its flows 10
  Spec idle;
  idle.n = 5;
  idle.arcs = {{0, 1}, {1, 2}, {2, 3}, {4, 2}};
  for (const double small : {10.0, 0.05})
    for (const double large : {1e12, 1e15})
      check_floating(solvers, checks, idle, {small, small, small, large},
                     with_w(idle, {1, 1, 1, 2}), {{0, 3}});

  // The large flows of a node, as a circulation of 1e15 that a preflow
  // makes, keep the small ones only within their rounding, eight epsilons
  // of the capacities, which also bound the arcs that pin the ends: a
  // small flow that goes is beyond it. The networks take, by turns, 0.05
  // next to 1e12 and 10 next to 1e15
  size_t turn = 0;
  for_each_spec(class_domain(), true, [&] (const Spec & s)
    {
      // Not a structured binding, which the lambda below could not take
      // with Clang 14
      const bool first = turn++ % 2 == 0;
      const double small = first ? 0.05 : 10.0, large = first ? 1e12 : 1e15;
      for_each_assignment(s, {0, 1}, [&] (const Spec & t)
        {
          std::vector<double> cap;
          for (long long k : t.w)
            cap.push_back(k == 0 ? small : large);
          const double total = 2 * (1 + std::accumulate(cap.begin(), cap.end(), 0.0));
          check_floating(solvers, checks, s, cap, t, ends_of(s),
                         8 * std::numeric_limits<double>::epsilon() * total);
        });
    });

  // The smallest networks found, of random ones with capacities far apart,
  // where the rounded sums of the flows of a node lost a small flow beside
  // a large one that went back later: a cycle of 3.6e-6 beside 6e5 left
  // apart by 5e-11, and 0.1875 beside 7.9e12 lost 1.2e-5; where the return
  // of the dust moved the rounding of the sums of a node around a cycle,
  // and the dust that mattered stayed; and where it moved a lack of dust
  // along the arcs of least flow around a cycle, and not to the node next
  // to it whose flows take it
  Spec far;
  far.n = 6;
  far.arcs = {{3, 3}, {2, 3}, {1, 5}, {3, 0}, {5, 1}, {3, 1}, {1, 4}, {2, 5}};
  check_floating(solvers, checks, far,
                 {55118.193817935826, 149126.55127876526, 3.6210321744802924e-06,
                  0.0385901963482553, 603731.40689728025, 1314.8553040957299,
                  953459.62917306356, 0.00026489820008349357}, far, {{5, 0}});
  far.arcs = {{2, 0}, {0, 1}, {0, 5}, {0, 4}, {5, 0}, {2, 4}, {4, 4}, {2, 3}};
  check_floating(solvers, checks, far,
                 {0.59427109889739216, 7953873572469.2061, 0.81688221777390935,
                  0.18751894271814884, 328344486869.60187, 3936541668632.1372,
                  0.87745357597236218, 56718122500789.414}, far, {{5, 4}});
  far.n = 7;
  far.arcs = {{2, 0}, {6, 3}, {1, 6}, {1, 4}, {0, 0}, {5, 0}, {1, 1}, {0, 1}, {4, 5}, {0, 6},
              {6, 4}};
  check_floating(solvers, checks, far,
                 {7.8578759915140388, 1.4834611226725403, 1.142136746397745e-06,
                  71865.908828292086, 16876.398523053449, 0.00022356931471620252,
                  0.00085033886634015452, 0.0006983639430014119, 2768.5456274341955,
                  815923.95045026485, 1.9315827618482626}, far, {{0, 6}});
  far.arcs = {{4, 2}, {5, 1}, {1, 4}, {3, 0}, {3, 4}, {3, 5}, {4, 5}, {4, 1}, {5, 1}, {5, 3},
              {4, 5}, {2, 3}, {5, 3}, {3, 3}, {5, 3}, {6, 1}, {3, 5}};
  check_floating(solvers, checks, far,
                 {0.96592538716777065, 41828262273.129745, 0.46122354099487478,
                  1096140671258.7633, 0.26271163702451467, 0.71954778904404204,
                  0.17797677450110017, 49428360953814.516, 0.45721147537616996,
                  591860561869202.25, 0.3543226821218215, 4609845414.2492914,
                  0.70720933063203661, 4845511438574.6699, 122077735988.32132,
                  97320921918.880493, 110642036269421.2}, far, {{2, 6}});

  // Random networks of capacities from 1e-6 to 1e6, uniform in their
  // logarithm, between every pair of ends
  std::mt19937 rng(seed(71));
  std::uniform_real_distribution<double> exponent(-6.0, 6.0);
  for (int k = 0; k < (heavy() ? 300 : 30); ++k)
    {
      const Spec s = random_spec(rng, 6 + int(rng() % 3), 8 + int(rng() % 9), true);
      std::vector<double> cap;
      for (size_t i = 0; i < s.arcs.size(); ++i)
        cap.push_back(std::pow(10.0, exponent(rng)));
      check_floating(solvers, checks, s, cap, s, ends_of(s));
    }
  checks.expect();
}

// ======================================================================
// Assignments, transportation and transshipment

namespace
{
  /// A matrix of costs, by rows
  using Matrix = std::vector<std::vector<long long>>;

  std::string matrix_text(const Matrix & c)
  {
    std::string r = "{";
    for (size_t i = 0; i < c.size(); ++i)
      {
        r += i == 0 ? "{" : " {";
        for (size_t j = 0; j < c[i].size(); ++j)
          r += (j == 0 ? "" : " ") + std::to_string(c[i][j]);
        r += "}";
      }
    return r + "}";
  }

  /// The least (or, if most, the largest) sum of an assignment of
  /// min(rows, cols) rows to as many columns of c, one to one, by brute
  /// force over the permutations
  long long best_assignment(const Matrix & c, bool most = false)
  {
    const size_t m = c.size(), n = m == 0 ? 0 : c[0].size();
    const size_t k = std::min(m, n);
    // The larger side, permuted; its first k entries take the smaller side
    std::vector<size_t> perm(std::max(m, n));
    std::iota(perm.begin(), perm.end(), size_t(0));
    long long best = most ? std::numeric_limits<long long>::lowest()
                          : std::numeric_limits<long long>::max();
    do
      {
        long long sum = 0;
        for (size_t i = 0; i < k; ++i)
          sum += m <= n ? c[i][perm[i]] : c[perm[i]][i];
        best = most ? std::max(best, sum) : std::min(best, sum);
      }
    while (std::next_permutation(perm.begin(), perm.end()));
    return k == 0 ? 0 : best;
  }

  /// Calls f with every matrix of rows x cols with the values given
  template <class F>
  void for_each_matrix(size_t rows, size_t cols, const std::vector<long long> & values, F f)
  {
    std::vector<size_t> pick(rows * cols, 0);
    while (true)
      {
        Matrix c(rows, std::vector<long long>(cols));
        for (size_t k = 0; k < pick.size(); ++k)
          c[k / cols][k % cols] = values[pick[k]];
        f(c);
        size_t i = 0;
        while (i < pick.size() and ++pick[i] == values.size())
          pick[i++] = 0;
        if (i == pick.size())
          return;
      }
  }

  /// The matrices of the assignment tests: every one of up to 2 x 3 and
  /// 3 x 2 with the costs -1, 0 and 2, every 3 x 3 with 0 and 2, and
  /// random ones of up to 5 x 5 with costs from -9 to 9
  template <class F>
  void for_each_assignment_matrix(bool square, F f)
  {
    for (size_t m = 1; m <= 3; ++m)
      for (size_t n = 1; n <= 3; ++n)
        if ((not square or m == n) and m * n <= 6)
          for_each_matrix(m, n, {-1, 0, 2}, f);
    for_each_matrix(3, 3, {0, 2}, f);
    std::mt19937 rng(seed(41));
    for (int k = 0; k < (heavy() ? 300 : 40); ++k)
      {
        const size_t m = 1 + rng() % 5, n = square ? m : 1 + rng() % 5;
        Matrix c(m, std::vector<long long>(n));
        for (auto & row : c)
          for (auto & x : row)
            x = static_cast<long long>(rng() % 19) - 9;
        f(c);
      }
  }

  /// What is wrong with the row of each column, row_to_col, and the column
  /// of each row, col_to_row, as a one to one assignment of min(rows, cols)
  /// that costs total in c; empty if nothing
  template <class Array_Of_Long, typename C>
  std::string assignment_error(const Matrix & c, const Array_Of_Long & row_to_col,
                               const Array_Of_Long & col_to_row, C total)
  {
    const size_t m = c.size(), n = c[0].size();
    if (row_to_col.size() != m or col_to_row.size() != n)
      return "the arrays have " + std::to_string(row_to_col.size()) + " and "
        + std::to_string(col_to_row.size()) + " entries";
    long long sum = 0;
    size_t assigned = 0;
    for (size_t i = 0; i < m; ++i)
      {
        const long j = row_to_col[i];
        if (j == -1)
          continue;
        if (j < 0 or size_t(j) >= n or col_to_row[size_t(j)] != long(i))
          return "the row " + std::to_string(i) + " has the column " + std::to_string(j);
        sum += c[i][size_t(j)];
        ++assigned;
      }
    if (assigned != std::min(m, n))
      return std::to_string(assigned) + " rows assigned, not " + std::to_string(std::min(m, n));
    if (C(sum) != total)
      return "the pairs cost " + std::to_string(sum) + ", not " + text(total);
    return "";
  }
}

template <class C>
class ExhaustiveAssignment : public ::testing::Test {};

/// The types of the costs: integers and floating
using Cost_Types = ::testing::Types<int, long, double>;

TYPED_TEST_SUITE(ExhaustiveAssignment, Cost_Types);

TYPED_TEST(ExhaustiveAssignment, HungarianMatchesTheOracle)
{
  using C = TypeParam;
  Check function("hungarian_assignment()");
  Check object("Hungarian_Assignment");
  Check most("hungarian_max_assignment()");

  for_each_assignment_matrix(false, [&] (const Matrix & c)
    {
      const size_t m = c.size(), n = c[0].size();
      Spec shown;
      shown.n = int(m);
      const std::string what = "the costs " + matrix_text(c) + ": ";
      DynMatrix<C> cost(m, n);
      cost.allocate();
      for (size_t i = 0; i < m; ++i)
        for (size_t j = 0; j < n; ++j)
          cost(i, j) = C(c[i][j]);
      const long long want = best_assignment(c);

      function.run();
      {
        const auto r = hungarian_assignment(cost);
        if (r.total_cost != C(want))
          function.fail(shown, what + "costs " + text(r.total_cost) + ", not "
                        + std::to_string(want));
        else if (auto e = assignment_error(c, r.row_to_col, r.col_to_row, r.total_cost);
                 not e.empty())
          function.fail(shown, what + e);
        else if (r.get_pairs().size() != std::min(m, n) or r.orig_rows != m or r.orig_cols != n)
          function.fail(shown, what + "get_pairs() or the sizes are wrong");
      }

      object.run();
      {
        const Hungarian_Assignment<C> h(cost);
        bool ok = h.get_total_cost() == C(want) and h.rows() == m and h.cols() == n
          and h.dimension() == std::max(m, n)
          and assignment_error(c, h.get_row_assignments(), h.get_col_assignments(),
                               h.get_total_cost()).empty()
          and h.get_assignments().size() == std::min(m, n);
        for (size_t i = 0; i < m and ok; ++i)
          ok = h.get_assignment(i) == h.get_row_assignments()[i];
        try
          {
            (void) h.get_assignment(m);
            ok = false;
          }
        catch (const std::out_of_range &) {}
        if (not ok)
          object.fail(shown, what + "a wrong answer from a query");
      }

      most.run();
      const auto r = hungarian_max_assignment(cost);
      const long long best = best_assignment(c, true);
      if (r.total_cost != C(best))
        most.fail(shown, what + "gives " + text(r.total_cost) + ", not " + std::to_string(best));
      else if (auto e = assignment_error(c, r.row_to_col, r.col_to_row, r.total_cost);
               not e.empty())
        most.fail(shown, what + e);
    });
  function.expect();
  object.expect();
  most.expect();
}

TYPED_TEST(ExhaustiveAssignment, AssignmentByFlowMatchesTheOracle)
{
  using C = TypeParam;
  Check check("solve_assignment()");
  for_each_assignment_matrix(true, [&] (const Matrix & c)
    {
      Spec shown;
      shown.n = int(c.size());
      std::vector<std::vector<C>> cost(c.size());
      for (size_t i = 0; i < c.size(); ++i)
        for (long long x : c[i])
          cost[i].push_back(C(x));
      check.run();
      const auto r = solve_assignment(cost);
      const long long want = best_assignment(c);
      std::vector<long> row_to_col(c.size(), -1), col_to_row(c.size(), -1);
      for (auto it = r.assignments.get_it(); it.has_curr(); it.next_ne())
        {
          const auto [i, j] = it.get_curr();
          if (i < c.size() and j < c.size())
            {
              row_to_col[i] = long(j);
              col_to_row[j] = long(i);
            }
        }
      if (not r.feasible or r.total_cost != C(want))
        check.fail(shown, "the costs " + matrix_text(c) + ": feasible " + std::to_string(r.feasible)
                   + " and cost " + text(r.total_cost) + ", not " + std::to_string(want));
      else if (auto e = assignment_error(c, row_to_col, col_to_row, r.total_cost); not e.empty())
        check.fail(shown, "the costs " + matrix_text(c) + ": " + e);
      else if (r.assignments.size() != c.size())
        check.fail(shown, "the costs " + matrix_text(c) + ": a pair twice");
    });
  {
    check.run();
    const auto r = solve_assignment(std::vector<std::vector<C>>{});
    if (not r.feasible or not r.assignments.is_empty())
      check.fail(Spec(), "no workers: not feasible, or pairs");
  }
  check.expect();
}

TEST(ExhaustiveAssignmentLimits, HungarianWithCostsNearTheLimits)
{
  constexpr int Max = std::numeric_limits<int>::max();
  // int keeps its potentials in long long: an optimum that fits is exact,
  // with rows of costs of max() / 2 or more too, which the infinity of int
  // took for no column, and one that does not fit, which overflowed the
  // potentials of int, throws overflow_error
  Check check("Hungarian_Assignment<int>, costs that fit");
  Check beyond("Hungarian_Assignment<int>, a row of costs from max() / 2");
  Check overflow("Hungarian_Assignment<int>, an optimum beyond max()");

  const std::vector<Matrix> cases = {
    {{1000000000, 1000000000}, {1000000000, 1000000000}},
    {{1000000000, 100000000}, {1000000000, 1000000000}},
    {{700000000, 700000000, 700000000}, {700000000, 700000000, 700000000},
     {700000000, 700000000, 700000000}},
    {{Max, 0}, {0, Max}},
    {{Max / 2, 0}},
    {{Max / 2 + 1}},
    {{Max, Max}, {0, 0}},
    {{1000000000, 1000000000, 1000000000}, {1000000000, 1000000000, 1000000000},
     {1000000000, 1000000000, 1000000000}}};
  for (const auto & c : cases)
    {
      bool row_beyond = false;
      for (const auto & row : c)
        row_beyond = row_beyond or *std::min_element(row.begin(), row.end()) >= Max / 2;
      const long long want = best_assignment(c);
      DynMatrix<int> cost(c.size(), c[0].size());
      cost.allocate();
      for (size_t i = 0; i < c.size(); ++i)
        for (size_t j = 0; j < c[i].size(); ++j)
          cost(i, j) = int(c[i][j]);
      if (want > Max)
        {
          overflow.run();
          try
            {
              const auto r = hungarian_assignment(cost);
              overflow.fail(Spec(), "the costs " + matrix_text(c) + ": gives "
                            + std::to_string(r.total_cost) + ", not overflow_error");
            }
          catch (const std::overflow_error &)
            {
              // what it must do
            }
          catch (const std::exception & e)
            {
              overflow.fail(Spec(), "the costs " + matrix_text(c) + ": throws " + e.what());
            }
          continue;
        }
      Check & which = row_beyond ? beyond : check;
      which.run();
      try
        {
          const auto r = hungarian_assignment(cost);
          if (r.total_cost != want)
            which.fail(Spec(), "the costs " + matrix_text(c) + ": " + std::to_string(r.total_cost)
                       + ", not " + std::to_string(want));
        }
      catch (const std::exception & e)
        {
          which.fail(Spec(), "the costs " + matrix_text(c) + ": throws " + e.what());
        }
    }
  check.expect();
  beyond.expect();
  overflow.expect();
}

namespace
{
  /// The least cost of a transportation of the supplies sup to the demands
  /// dem, all shipped, with the costs c, by brute force over the integer
  /// shipments; -1 if the totals differ
  long long best_transport(const std::vector<long long> & sup, const std::vector<long long> & dem,
                           const Matrix & c)
  {
    if (std::accumulate(sup.begin(), sup.end(), 0LL)
        != std::accumulate(dem.begin(), dem.end(), 0LL))
      return -1;
    const size_t m = sup.size(), n = dem.size();
    std::vector<long long> x(m * n, 0);
    long long best = -1;
    while (true)
      {
        bool ok = true;
        for (size_t i = 0; i < m and ok; ++i)
          {
            long long row = 0;
            for (size_t j = 0; j < n; ++j)
              row += x[i * n + j];
            ok = row == sup[i];
          }
        for (size_t j = 0; j < n and ok; ++j)
          {
            long long col = 0;
            for (size_t i = 0; i < m; ++i)
              col += x[i * n + j];
            ok = col == dem[j];
          }
        if (ok)
          {
            long long cost = 0;
            for (size_t k = 0; k < x.size(); ++k)
              cost += x[k] * c[k / n][k % n];
            if (best < 0 or cost < best)
              best = cost;
          }
        size_t k = 0;
        while (k < x.size() and ++x[k] > std::min(sup[k / n], dem[k % n]))
          x[k++] = 0;
        if (k == x.size())
          return best;
      }
  }
}

TYPED_TEST(ExhaustiveAssignment, TransportationMatchesTheOracle)
{
  using C = TypeParam;
  Check check("solve_transportation()");
  for (size_t m = 1; m <= 2; ++m)
    for (size_t n = 1; n <= 2; ++n)
      for_each_matrix(1, m + n, {0, 1, 2}, [&] (const Matrix & amounts)
        {
          const std::vector<long long> sup(amounts[0].begin(), amounts[0].begin() + long(m));
          const std::vector<long long> dem(amounts[0].begin() + long(m), amounts[0].end());
          for_each_matrix(m, n, {0, 1, 3}, [&] (const Matrix & c)
            {
              check.run();
              std::vector<C> s, d;
              for (auto x : sup)
                s.push_back(C(x));
              for (auto x : dem)
                d.push_back(C(x));
              std::vector<std::vector<C>> cost(m);
              for (size_t i = 0; i < m; ++i)
                for (auto x : c[i])
                  cost[i].push_back(C(x));
              const auto r = solve_transportation(s, d, cost);
              const long long want = best_transport(sup, dem, c);
              const std::string what = "supplies " + matrix_text({sup}) + ", demands "
                + matrix_text({dem}) + ", costs " + matrix_text(c) + ": ";
              if (r.feasible != (want >= 0) or (want >= 0 and r.total_cost != C(want)))
                {
                  check.fail(Spec(), what + "feasible " + std::to_string(r.feasible) + " and cost "
                             + text(r.total_cost) + ", not " + std::to_string(want));
                  return;
                }
              if (want < 0)
                return;
              C total{0};
              for (size_t i = 0; i < m; ++i)
                {
                  C row{0};
                  for (size_t j = 0; j < n; ++j)
                    {
                      row += r.shipments[i][j];
                      total += r.shipments[i][j] * cost[i][j];
                    }
                  if (row != s[i])
                    check.fail(Spec(), what + "a supply is not shipped");
                }
              for (size_t j = 0; j < n; ++j)
                {
                  C col{0};
                  for (size_t i = 0; i < m; ++i)
                    col += r.shipments[i][j];
                  if (col != d[j])
                    check.fail(Spec(), what + "a demand is not met");
                }
              if (total != r.total_cost)
                check.fail(Spec(), what + "the shipments cost " + text(total));
            });
        });
  check.expect();
}

TEST(ExhaustiveAssignmentLimits, TransportationOfTenths)
{
  // The totals of the supplies and of the demands are equal, but some, as
  // {0.1, 0.2} and {0.3}, only but for rounding
  Check check("solve_transportation(), with tenths");
  for (size_t m = 1; m <= 2; ++m)
    for (size_t n = 1; n <= 2; ++n)
      for_each_matrix(1, m + n, {1, 2, 3}, [&] (const Matrix & amounts)
        {
          const std::vector<long long> sup(amounts[0].begin(), amounts[0].begin() + long(m));
          const std::vector<long long> dem(amounts[0].begin() + long(m), amounts[0].end());
          if (std::accumulate(sup.begin(), sup.end(), 0LL)
              != std::accumulate(dem.begin(), dem.end(), 0LL))
            return;
          std::vector<double> s, d;
          for (auto x : sup)
            s.push_back(double(x) / 10);
          for (auto x : dem)
            d.push_back(double(x) / 10);
          Check & c = check;
          c.run();
          const std::vector<std::vector<double>> cost(m, std::vector<double>(n, 1.0));
          const auto r = solve_transportation(s, d, cost);
          mpq_class shipped = 0; // each unit costs 1
          for (double x : s)
            shipped += mpq_class(x);
          if (not r.feasible or not near(r.total_cost, shipped))
            c.fail(Spec(), "supplies " + matrix_text({sup}) + " and demands " + matrix_text({dem})
                   + " in tenths: feasible " + std::to_string(r.feasible) + ", cost "
                   + text(r.total_cost));
        });
  check.expect();
}

namespace
{
  /// The least cost of a flow with out - in = d at every node, in the
  /// capacities s.w, by brute force over the integer flows; -1 if there is
  /// none
  long long best_transshipment(const Spec & s, const std::vector<long long> & cost,
                               const std::vector<long long> & d)
  {
    std::vector<long long> f(s.arcs.size(), 0);
    long long best = -1;
    while (true)
      {
        std::vector<long long> net(size_t(s.n), 0);
        for (size_t i = 0; i < f.size(); ++i)
          {
            net[size_t(s.arcs[i][0])] += f[i];
            net[size_t(s.arcs[i][1])] -= f[i];
          }
        if (net == d)
          {
            long long c = 0;
            for (size_t i = 0; i < f.size(); ++i)
              c += f[i] * cost[i];
            if (best < 0 or c < best)
              best = c;
          }
        size_t i = 0;
        while (i < f.size() and ++f[i] > s.w[i])
          f[i++] = 0;
        if (i == f.size())
          return best;
      }
  }
}

TYPED_TEST(ExhaustiveMinCost, TransshipmentMatchesTheOracle)
{
  using Net = TypeParam;
  using F = typename Net::Flow_Type;
  Check check("solve_transshipment()");

  for_each_spec(class_domain(), true, [&] (const Spec & raw)
    {
      const Spec s = structural(raw);
      const auto cost = structural_costs(s);
      std::vector<long long> values = {-1, 0, 1};
      Spec nodes_as_arcs; // the demands, one per node, through an assignment
      nodes_as_arcs.n = s.n;
      nodes_as_arcs.arcs.assign(size_t(s.n), {0, 0});
      for_each_assignment(nodes_as_arcs, values, [&] (const Spec & dem)
        {
          const std::vector<long long> & d = dem.w;
          const long long total = std::accumulate(d.begin(), d.end(), 0LL);
          const bool nothing = std::all_of(d.begin(), d.end(), [] (long long x) { return x == 0; });
          Check & c = check;
          Built_Net<Net> b(s, s.w, cost);
          const size_t nodes = b.net.get_num_nodes(), arcs = b.net.get_num_arcs();
          std::map<typename Net::Node *, F> demand;
          for (int v = 0; v < s.n; ++v)
            demand[b.nodes[size_t(v)]] = amount<F>(d[size_t(v)]);
          const std::string what = "demands " + matrix_text({d}) + ", " + costs_text(cost);
          c.run();
          TransshipmentResult<F> r;
          try
            {
              r = solve_transshipment(b.net, [&] (typename Net::Node * p) { return demand[p]; });
            }
          catch (const std::exception & e)
            {
              c.fail(s, what + "throws " + e.what());
              return;
            }
          const long long want = total != 0 ? -1 : nothing ? 0 : best_transshipment(s, cost, d);
          if (r.feasible != (want >= 0))
            {
              c.fail(s, what + (want >= 0 ? "says infeasible" : "says feasible"));
              return;
            }
          if (b.net.get_num_nodes() != nodes or b.net.get_num_arcs() != arcs)
            {
              c.fail(s, what + "leaves the super nodes");
              return;
            }
          if (want <= 0 and nothing)
            return;
          if (want < 0)
            return;
          long long supply = 0;
          for (long long x : d)
            supply += std::max(x, 0LL);
          if (not is_amount(r.total_flow, supply) or not is_cost(r.total_cost, want))
            {
              c.fail(s, what + "ships " + text(r.total_flow) + " at " + text(r.total_cost)
                     + ", not " + text(amount<F>(supply)) + " at " + text(cost_amount<F>(want)));
              return;
            }
          // The flows of the network are that transshipment
          for (int v = 0; v < s.n; ++v)
            {
              auto p = b.nodes[size_t(v)];
              if (b.net.get_out_flow(p) - b.net.get_in_flow(p) != demand[p])
                {
                  c.fail(s, what + "the flows left do not meet the demand of "
                         + std::to_string(v));
                  return;
                }
            }
          if (b.net.flow_cost() != r.total_cost)
            c.fail(s, what + "the flows left cost " + text(b.net.flow_cost()));
        });
    });
  check.expect();
}

// ======================================================================
// The applications of net_apps.H

namespace
{
  /// Whether there is a flow with lower[i] <= f[i] <= cap[i] and
  /// in - out = d at every node, by brute force over the integer flows
  bool circulation_exists(const Spec & s, const std::vector<long long> & lower,
                          const std::vector<long long> & d)
  {
    std::vector<long long> f = lower;
    while (true)
      {
        std::vector<long long> net(size_t(s.n), 0);
        for (size_t i = 0; i < f.size(); ++i)
          {
            net[size_t(s.arcs[i][1])] += f[i];
            net[size_t(s.arcs[i][0])] -= f[i];
          }
        if (net == d)
          return true;
        size_t i = 0;
        while (i < f.size() and ++f[i] > s.w[i])
          f[i] = lower[i], ++i;
        if (i == f.size())
          return false;
      }
  }
}

TEST(ExhaustiveNetApps, CirculationsMatchTheOracle)
{
  using Net = Net_Graph<Net_Node<int>, Net_Arc<int, long>>;
  Check check("solve_circulation()");
  Check restored("solve_circulation(), the network after");

  for_each_spec(class_domain(), true, [&] (const Spec & raw)
    {
      const Spec s = structural(raw);
      Spec per_node;
      per_node.n = s.n;
      per_node.arcs.assign(size_t(s.n), {0, 0});
      for_each_assignment(per_node, {-1, 0, 1}, [&] (const Spec & dem)
        {
          for_each_assignment(s, {0, 1}, [&] (const Spec & low)
            {
              const auto & d = dem.w;
              const auto & lower = low.w;
              Built_Net<Net> b(s);
              std::map<Net::Node *, long> demand;
              std::map<Net::Arc *, long> lb;
              for (int v = 0; v < s.n; ++v)
                demand[b.nodes[size_t(v)]] = long(d[size_t(v)]);
              for (size_t i = 0; i < b.arcs.size(); ++i)
                lb[b.arcs[i]] = long(lower[i]);
              const std::string what = "demands " + matrix_text({d}) + ", lower bounds "
                + matrix_text({lower}) + ": ";
              check.run();
              CirculationResult<long> r;
              try
                {
                  r = solve_circulation(b.net, [&] (Net::Node * p) { return demand[p]; },
                                        [&] (Net::Arc * a) { return lb[a]; });
                }
              catch (const std::exception & e)
                {
                  check.fail(s, what + "throws " + e.what());
                  return;
                }
              const bool want = circulation_exists(s, lower, d);
              if (r.feasible != want)
                check.fail(s, what + (want ? "says infeasible" : "says feasible"));
              else if (want)
                {
                  // The flows that it gives meet the bounds and the demands
                  std::vector<long> in(size_t(s.n), 0), out(size_t(s.n), 0);
                  std::string e;
                  for (size_t i = 0; i < b.arcs.size() and e.empty(); ++i)
                    {
                      auto a = b.arcs[i];
                      const long f = r.flow.contains(a) ? r.flow.find(a) : -1;
                      if (f < lower[i] or f > s.w[i])
                        e = "the arc " + std::to_string(i) + " gets " + std::to_string(f);
                      out[size_t(s.arcs[i][0])] += f;
                      in[size_t(s.arcs[i][1])] += f;
                    }
                  for (int v = 0; v < s.n and e.empty(); ++v)
                    if (in[size_t(v)] - out[size_t(v)] != d[size_t(v)])
                      e = "the node " + std::to_string(v) + " gets in - out = "
                        + std::to_string(in[size_t(v)] - out[size_t(v)]);
                  if (not e.empty())
                    check.fail(s, what + "the flows: " + e);
                }

              restored.run();
              bool same = b.net.get_num_nodes() == size_t(s.n)
                and b.net.get_num_arcs() == s.num_arcs();
              for (size_t i = 0; i < b.arcs.size() and same; ++i)
                same = b.arcs[i]->cap == s.w[i] and b.arcs[i]->flow == 0;
              if (not same)
                restored.fail(s, what + "the network is not as it was");
            });
        });
    });
  check.expect();
  restored.expect();
}

TEST(ExhaustiveNetApps, ProjectSelectionsMatchTheOracle)
{
  Check check("solve_project_selection()");
  auto run = [&] (const Spec & s, const Spec & p)
        {
          // Project i requires project j for each arc i -> j
          std::vector<Project<long>> projects;
          for (int i = 0; i < s.n; ++i)
            {
              std::vector<size_t> pre;
              for (const auto & a : s.arcs)
                if (a[0] == i)
                  pre.push_back(size_t(a[1]));
              projects.emplace_back(size_t(i), long(p.w[size_t(i)]), pre);
            }
          long best = 0;
          for (unsigned long set = 0; set < (1UL << s.n); ++set)
            {
              bool closed = true;
              for (const auto & a : s.arcs)
                closed = closed and (((set >> a[0]) & 1UL) == 0 or ((set >> a[1]) & 1UL) != 0);
              long profit = 0;
              for (int i = 0; i < s.n; ++i)
                profit += ((set >> i) & 1UL) ? long(p.w[size_t(i)]) : 0;
              if (closed)
                best = std::max(best, profit);
            }
          check.run();
          const auto r = solve_project_selection(projects);
          std::set<size_t> chosen(r.selected.begin(), r.selected.end());
          long profit = 0, revenue = 0, cost = 0;
          bool closed = chosen.size() == r.selected.size();
          for (size_t i : chosen)
            {
              profit += long(p.w[i]);
              revenue += std::max(0L, long(p.w[i]));
              cost += std::max(0L, -long(p.w[i]));
              for (size_t j : projects[i].prerequisites)
                closed = closed and chosen.count(j) > 0;
            }
          const std::string what = "profits " + matrix_text({p.w}) + ": ";
          if (r.max_profit != best)
            check.fail(s, what + "the profit is " + std::to_string(r.max_profit) + ", not "
                       + std::to_string(best));
          else if (not closed or profit != best or r.total_revenue != revenue
                   or r.total_cost != cost)
            check.fail(s, what + "the selection is not closed, or not of that profit");
        };
  // Project i requires j for each arc i -> j: every digraph of 3 nodes and
  // 3 arcs with each assignment of the profits -2, -1, 1 and 2, and random
  // ones of 5 and 6 projects
  for_each_spec(domain(3, 3), true, [&] (const Spec & s)
    {
      if (s.n == 0)
        return;
      Spec per_node;
      per_node.n = s.n;
      per_node.arcs.assign(size_t(s.n), {0, 0});
      for_each_assignment(per_node, {-2, -1, 1, 2}, [&] (const Spec & p) { run(s, p); });
    });
  std::mt19937 rng(seed(61));
  for (int k = 0; k < (heavy() ? 2000 : 200); ++k)
    {
      const int n = 5 + int(rng() % 2);
      Spec s = random_spec(rng, n, int(rng() % 8), true);
      Spec p;
      p.n = n;
      p.arcs.assign(size_t(n), {0, 0});
      random_weights(rng, p, -3, 3);
      run(s, p);
    }
  check.expect();
}

namespace
{
  /// Whether team x cannot finish first, or tied first: in every outcome
  /// of the games left between the others, where x wins all of its own,
  /// some other team ends with more wins than x can have
  bool eliminated(const std::vector<Team> & t, size_t x)
  {
    const int top = t[x].wins + t[x].remaining;
    std::vector<std::pair<size_t, size_t>> pairs;
    for (size_t i = 0; i < t.size(); ++i)
      for (size_t j = i + 1; j < t.size(); ++j)
        if (i != x and j != x)
          pairs.push_back({i, j});
    // Each pair splits its games: k won by the first
    std::vector<int> k(pairs.size(), 0);
    while (true)
      {
        std::vector<int> w(t.size());
        for (size_t i = 0; i < t.size(); ++i)
          w[i] = t[i].wins;
        for (size_t p = 0; p < pairs.size(); ++p)
          {
            const int g = t[pairs[p].first].against[pairs[p].second];
            w[pairs[p].first] += k[p];
            w[pairs[p].second] += g - k[p];
          }
        bool first = true;
        for (size_t i = 0; i < t.size(); ++i)
          first = first and (i == x or w[i] <= top);
        if (first)
          return false;
        size_t p = 0;
        while (p < pairs.size() and ++k[p] > t[pairs[p].first].against[pairs[p].second])
          k[p++] = 0;
        if (p == pairs.size())
          return true;
      }
  }

  /// Whether the teams R prove that x is out: their wins and the games
  /// between them are more than |R| times what x can win
  bool proves(const std::vector<Team> & t, size_t x, const std::vector<size_t> & r)
  {
    if (r.empty())
      return false;
    long total = 0;
    for (size_t i : r)
      {
        if (i == x or i >= t.size())
          return false;
        total += t[i].wins;
        for (size_t j : r)
          if (i < j)
            total += t[i].against[j];
      }
    return total > long(r.size()) * (t[x].wins + t[x].remaining);
  }
}

TEST(ExhaustiveNetApps, BaseballEliminationsMatchTheOracle)
{
  Check check("check_baseball_elimination()");
  Check certificate("check_baseball_elimination(), the certificate");

  auto run = [&] (const std::vector<Team> & t)
    {
      Spec shown;
      shown.n = int(t.size());
      for (size_t x = 0; x < t.size(); ++x)
        {
          std::string what = "wins";
          for (const auto & team : t)
            what += " " + std::to_string(team.wins);
          what += ", games";
          for (size_t i = 0; i < t.size(); ++i)
            for (size_t j = i + 1; j < t.size(); ++j)
              what += " " + std::to_string(t[i].against[j]);
          what += ", team " + std::to_string(x) + ": ";
          check.run();
          const auto r = check_baseball_elimination(t, x);
          const bool want = eliminated(t, x);
          if (r.eliminated != want or r.max_possible_wins != t[x].wins + t[x].remaining)
            {
              check.fail(shown, what + (want ? "says not eliminated" : "says eliminated"));
              continue;
            }
          if (want)
            {
              certificate.run();
              if (not proves(t, x, r.certificate))
                certificate.fail(shown, what + "the certificate proves nothing");
            }
        }
    };

  // Every league of 3 teams with up to 2 wins and up to 2 games between
  // each pair, and random ones of 4 and 5 teams
  for (int w0 = 0; w0 <= 2; ++w0)
    for (int w1 = 0; w1 <= 2; ++w1)
      for (int w2 = 0; w2 <= 2; ++w2)
        for (int g01 = 0; g01 <= 2; ++g01)
          for (int g02 = 0; g02 <= 2; ++g02)
            for (int g12 = 0; g12 <= 2; ++g12)
              {
                std::vector<Team> t(3);
                const int w[] = {w0, w1, w2};
                const int g[3][3] = {{0, g01, g02}, {g01, 0, g12}, {g02, g12, 0}};
                for (size_t i = 0; i < 3; ++i)
                  {
                    t[i].wins = w[i];
                    t[i].against.assign(g[i], g[i] + 3);
                    t[i].remaining = g[i][0] + g[i][1] + g[i][2];
                  }
                run(t);
              }
  std::mt19937 rng(seed(51));
  for (int k = 0; k < (heavy() ? 2000 : 200); ++k)
    {
      const size_t n = 4 + rng() % 2;
      std::vector<Team> t(n);
      for (auto & team : t)
        {
          team.wins = int(rng() % 4);
          team.against.assign(n, 0);
        }
      for (size_t i = 0; i < n; ++i)
        for (size_t j = i + 1; j < n; ++j)
          t[i].against[j] = t[j].against[i] = int(rng() % 3);
      for (size_t i = 0; i < n; ++i)
        t[i].remaining = std::accumulate(t[i].against.begin(), t[i].against.end(), 0)
          + int(rng() % 2);
      run(t);
    }
  check.expect();
  certificate.expect();
}

TEST(ExhaustiveNetApps, SegmentationsMatchTheOracle)
{
  Check check("segment_image()");
  const std::vector<std::pair<size_t, size_t>> grids = {{1, 1}, {1, 2}, {2, 1}, {1, 3}, {2, 2}};
  for (const auto & grid : grids)
    {
      // Not structured bindings: the lambdas below take them, and Clang 14
      // does not let a lambda take a binding
      const size_t rows = grid.first, cols = grid.second;
      const size_t pixels = rows * cols;
      const std::vector<long long> values = pixels == 4 ? std::vector<long long>{0, 2}
                                                        : std::vector<long long>{0, 1, 2};
      for (long long smooth : {0LL, 1LL})
        for_each_matrix(1, 2 * pixels, values, [&] (const Matrix & m)
          {
            std::vector<std::vector<std::array<double, 2>>> data(
              rows, std::vector<std::array<double, 2>>(cols));
            for (size_t p = 0; p < pixels; ++p)
              data[p / cols][p % cols] = {double(m[0][2 * p]), double(m[0][2 * p + 1])};
            auto energy = [&] (unsigned long labels)
              {
                double e = 0;
                for (size_t p = 0; p < pixels; ++p)
                  e += data[p / cols][p % cols][(labels >> p) & 1UL];
                for (size_t p = 0; p < pixels; ++p)
                  {
                    const unsigned long here = (labels >> p) & 1UL;
                    if (p % cols + 1 < cols and here != ((labels >> (p + 1)) & 1UL))
                      e += double(smooth);
                    if (p + cols < pixels and here != ((labels >> (p + cols)) & 1UL))
                      e += double(smooth);
                  }
                return e;
              };
            double best = std::numeric_limits<double>::max();
            for (unsigned long labels = 0; labels < (1UL << pixels); ++labels)
              best = std::min(best, energy(labels));
            check.run();
            const auto r = segment_image(rows, cols, data, double(smooth));
            unsigned long got = 0;
            bool shaped = r.labels.size() == rows;
            for (size_t i = 0; i < rows and shaped; ++i)
              {
                shaped = r.labels[i].size() == cols;
                for (size_t j = 0; j < cols and shaped; ++j)
                  {
                    shaped = r.labels[i][j] == 0 or r.labels[i][j] == 1;
                    got |= (unsigned long) (r.labels[i][j] == 1) << (i * cols + j);
                  }
              }
            Spec shown;
            shown.n = int(pixels);
            const std::string what = std::to_string(rows) + " x " + std::to_string(cols)
              + ", smoothness " + std::to_string(smooth) + ", costs " + matrix_text(m) + ": ";
            if (not shaped)
              check.fail(shown, what + "the labels are not of the image");
            else if (r.energy != best or energy(got) != best)
              check.fail(shown, what + "the energy is " + text(r.energy) + " for labels of "
                         + text(energy(got)) + ", not " + text(best));
          });
    }
  check.expect();
}

namespace
{
  /// Whether some respondents can answer some questions so that each
  /// question gets from its minimum to its maximum of answers and each
  /// respondent answers from its minimum to its maximum, only those it is
  /// eligible for, by brute force over the sets of pairs
  bool survey_exists(const std::vector<SurveyQuestion> & q, const std::vector<SurveyRespondent> & r)
  {
    std::vector<std::pair<size_t, size_t>> pairs;
    for (size_t i = 0; i < r.size(); ++i)
      {
        const auto & e = r[i].eligible_questions;
        for (size_t j : std::set<size_t>(e.begin(), e.end()))
          if (j < q.size())
            pairs.push_back({i, j});
      }
    for (unsigned long set = 0; set < (1UL << pairs.size()); ++set)
      {
        std::vector<int> by_q(q.size(), 0), by_r(r.size(), 0);
        for (size_t k = 0; k < pairs.size(); ++k)
          if ((set >> k) & 1UL)
            {
              ++by_r[pairs[k].first];
              ++by_q[pairs[k].second];
            }
        bool ok = true;
        for (size_t j = 0; j < q.size() and ok; ++j)
          ok = by_q[j] >= q[j].min_responses and by_q[j] <= q[j].max_responses;
        for (size_t i = 0; i < r.size() and ok; ++i)
          ok = by_r[i] >= r[i].min_questions and by_r[i] <= r[i].max_questions;
        if (ok)
          return true;
      }
    return false;
  }
}

TEST(ExhaustiveNetApps, SurveysMatchTheOracle)
{
  Check check("design_survey()");

  const std::vector<std::pair<int, int>> bounds = {{0, 1}, {1, 1}, {0, 2}, {1, 2}};
  for (size_t nq = 1; nq <= 2; ++nq)
    for (size_t nr = 1; nr <= 2; ++nr)
      {
        // The bounds of each question and respondent, and the set of
        // questions of each respondent
        const size_t knobs = nq + 2 * nr;
        std::vector<size_t> pick(knobs, 0);
        while (true)
          {
            std::vector<SurveyQuestion> q;
            std::vector<SurveyRespondent> r;
            for (size_t j = 0; j < nq; ++j)
              q.push_back({j, bounds[pick[j]].first, bounds[pick[j]].second});
            for (size_t i = 0; i < nr; ++i)
              {
                const auto [lo, hi] = bounds[pick[nq + 2 * i]];
                std::vector<size_t> eligible;
                for (size_t j = 0; j < nq; ++j)
                  if ((pick[nq + 2 * i + 1] >> j) & 1U)
                    eligible.push_back(j);
                r.push_back({i, lo, hi, eligible});
              }
            Check & c = check;
            c.run();
            const auto res = design_survey(q, r);
            const bool want = survey_exists(q, r);
            std::string what = std::to_string(nq) + " questions";
            for (const auto & x : q)
              what += " [" + std::to_string(x.min_responses) + ","
                + std::to_string(x.max_responses) + "]";
            what += ", respondents";
            for (const auto & x : r)
              {
                what += " [" + std::to_string(x.min_questions) + ","
                  + std::to_string(x.max_questions) + "] of {";
                for (size_t j : x.eligible_questions)
                  what += std::to_string(j);
                what += "}";
              }
            what += ": ";
            if (res.feasible != want)
              c.fail(Spec(), what + (want ? "says infeasible" : "says feasible"));
            else if (want)
              {
                // The pairs it gives keep every bound
                std::vector<int> by_q(nq, 0), by_r(nr, 0);
                bool ok = true;
                std::set<std::pair<size_t, size_t>> seen;
                for (auto [ri, qj] : res.assignments)
                  {
                    const auto & e = r[ri < nr ? ri : 0].eligible_questions;
                    ok = ok and ri < nr and qj < nq and seen.insert({ri, qj}).second
                      and std::find(e.begin(), e.end(), qj) != e.end();
                    if (ok)
                      {
                        ++by_r[ri];
                        ++by_q[qj];
                      }
                  }
                for (size_t j = 0; j < nq and ok; ++j)
                  ok = by_q[j] >= q[j].min_responses and by_q[j] <= q[j].max_responses;
                for (size_t i = 0; i < nr and ok; ++i)
                  ok = by_r[i] >= r[i].min_questions and by_r[i] <= r[i].max_questions;
                if (not ok)
                  c.fail(Spec(), what + "the answers break a bound");
              }
            size_t k = 0;
            while (k < knobs)
              {
                const size_t limit = k < nq or (k - nq) % 2 == 0 ? bounds.size() : (1U << nq);
                if (++pick[k] < limit)
                  break;
                pick[k++] = 0;
              }
            if (k == knobs)
              break;
          }
      }
  check.expect();
}

// ======================================================================
// Capacities of the nodes, supplies and demands

namespace
{
  /// The maximum flow from a to z when node i lets at most node_cap[i]
  /// through, or anything if node_cap[i] is negative: each node split in
  /// two, joined by an arc of its capacity
  long long node_capacity_max_flow(const Spec & s, const std::vector<long long> & node_cap,
                                   int a, int z)
  {
    long long total = 1;
    for (long long w : s.w)
      total += w;
    Spec x;
    x.n = 2 * s.n;
    for (int v = 0; v < s.n; ++v)
      {
        x.arcs.push_back({v, s.n + v});
        x.w.push_back(node_cap[size_t(v)] < 0 ? total : node_cap[size_t(v)]);
      }
    for (size_t i = 0; i < s.arcs.size(); ++i)
      {
        x.arcs.push_back({s.n + s.arcs[i][0], s.arcs[i][1]});
        x.w.push_back(s.w[i]);
      }
    return max_flow_units(x, a, s.n + z);
  }
}

template <class Net>
class ExhaustiveNodeCapacities : public ::testing::Test {};

using Node_Cap_Nets =
  ::testing::Types<Net_Cap_Graph<Net_Cap_Node<int, long>, Net_Arc<int, long>>,
                   Net_Cap_Graph<Net_Cap_Node<int, double>, Net_Arc<int, double>>>;

TYPED_TEST_SUITE(ExhaustiveNodeCapacities, Node_Cap_Nets);

TYPED_TEST(ExhaustiveNodeCapacities, AuxiliaryNetworksMatchTheOracle)
{
  using Net = TypeParam;
  using F = typename Net::Flow_Type;
  Check flow("compute_aux_net(), dinic_maximum_flow() and update()");
  // The augmenting paths must keep the cookies of the arcs of the
  // auxiliary network, which update() follows; they run with one of each
  // three assignments of the capacities of the nodes
  Check paths("compute_aux_net(), edmonds_karp_maximum_flow() and update()");
  Check node_flows("update(), the flows of the nodes");
  Check total("get_total_flow()");
  Check freed("free_aux_net() and reset_flows()");

  for_each_spec(class_domain(), true, [&] (const Spec & raw)
    {
      const Spec s = structural(raw);
      Spec per_node;
      per_node.n = s.n;
      per_node.arcs.assign(size_t(s.n), {0, 0});
      // -1 for a node without limit
      size_t assignment = 0;
      for_each_assignment(per_node, {-1, 0, 1}, [&] (const Spec & caps)
        {
          const bool with_paths = assignment++ % 3 == 0;
          for (auto [a, z] : ends_of(s))
            {
              if (a == z)
                continue;
              Built_Net<Net> b(s);
              for (int v = 0; v < s.n; ++v)
                if (caps.w[size_t(v)] >= 0)
                  Net::set_node_cap(b.nodes[size_t(v)], amount<F>(caps.w[size_t(v)]));
              b.pin(a, z);
              const std::string what = where(b, a, z) + "node capacities "
                + matrix_text({caps.w}) + ": ";
              const long long want = node_capacity_max_flow(s, caps.w, a, z);

              if (with_paths)
              {
                Built_Net<Net> c(s);
                for (int v = 0; v < s.n; ++v)
                  if (caps.w[size_t(v)] >= 0)
                    Net::set_node_cap(c.nodes[size_t(v)], amount<F>(caps.w[size_t(v)]));
                c.pin(a, z);
                paths.run();
                auto aux = c.net.compute_aux_net();
                edmonds_karp_maximum_flow(*aux);
                bool mapped = true;
                for (typename Net::Aux_Net::Arc_Iterator it(*aux); it.has_curr(); it.next_ne())
                  mapped = mapped and ARC_COOKIE(it.get_curr()) != nullptr;
                if (not mapped)
                  paths.fail(s, what + "an arc of the auxiliary network lost its cookie");
                else
                  {
                    c.net.update();
                    if (auto e = max_flow_error(c, c.value(), want); not e.empty())
                      paths.fail(s, what + e);
                  }
              }

              flow.run();
              auto aux = b.net.compute_aux_net();
              if (not (aux->is_single_source() and aux->is_single_sink()))
                {
                  flow.fail(s, what + "the auxiliary network has no single source and sink");
                  continue;
                }
              dinic_maximum_flow(*aux);
              b.net.update();
              if (auto e = max_flow_error(b, b.value(), want); not e.empty())
                {
                  flow.fail(s, what + e);
                  continue;
                }

              node_flows.run();
              bool kept = b.net.check_node_capacities();
              for (int v = 0; v < s.n and kept; ++v)
                {
                  auto p = b.nodes[size_t(v)];
                  const F through = std::max(b.net.get_in_flow(p), b.net.get_out_flow(p));
                  kept = p->in_flow == through and p->out_flow == through;
                }
              if (not kept)
                node_flows.fail(s, what + "a node has not its flow, or passes its capacity");

              total.run();
              if (b.net.get_total_flow() != b.value())
                total.fail(s, what + "gives " + text(b.net.get_total_flow()) + " for a flow of "
                           + text(b.value()));

              freed.run();
              b.net.free_aux_net();
              const F value = b.value();
              b.net.reset_flows();
              bool zero = not b.net.has_aux_net() and is_amount(value, want);
              for (auto arc : b.arcs)
                zero = zero and arc->flow == F{0};
              for (int v = 0; v < s.n; ++v)
                zero = zero and b.nodes[size_t(v)]->in_flow == F{0};
              if (not zero)
                freed.fail(s, what + "the flows or the auxiliary network are left");
            }
        });
    });
  flow.expect();
  paths.expect();
  node_flows.expect();
  total.expect();
  freed.expect();
}

TEST(ExhaustiveSupplyDemand, TheAuxiliaryNetworkMatchesTheOracle)
{
  using Node = Net_Sup_Dem_Node<int, long>;
  using Net = Net_Sup_Dem_Graph<Node, Net_Arc<int, long>>;
  // A node supplies, or demands, at most its out_cap, or in_cap, which
  // are unlimited unless set; they are set here to the capacities of its
  // arcs, or left unset
  Check aux("compute_aux_net()");
  Check feasible("is_feasible(), after a maximum flow");
  Check trivial("is_feasible(), without supplies or without demands");
  Check freed("free_aux_net()");

  for_each_spec(class_domain(), true, [&] (const Spec & raw)
    {
      const Spec s = structural(raw);
      Spec per_node;
      per_node.n = s.n;
      per_node.arcs.assign(size_t(s.n), {0, 0});
      for_each_assignment(per_node, {-1, 0, 1}, [&] (const Spec & sup)
        {
          const bool any = std::any_of(sup.w.begin(), sup.w.end(),
                                       [] (long long x) { return x != 0; });
          for (bool set : {false, true})
            {
              const std::string what = "supplies " + matrix_text({sup.w})
                + (set ? ", the capacities of the nodes set: " : ": ");
              Built_Net<Net> b(s);
              bool fits = true;
              for (int v = 0; v < s.n; ++v)
                {
                  auto p = b.nodes[size_t(v)];
                  p->supply_flow = long(sup.w[size_t(v)]);
                  if (set)
                    {
                      p->out_cap = b.net.get_out_cap(p);
                      p->in_cap = b.net.get_in_cap(p);
                    }
                  fits = fits and p->supply_flow <= p->out_cap and -p->supply_flow <= p->in_cap;
                }
              aux.run();
              try
                {
                  b.net.compute_aux_net();
                  if (not fits)
                    aux.fail(s, what + "takes a supply above the capacity of its node");
                }
              catch (const std::range_error &)
                {
                  if (fits)
                    aux.fail(s, what + "throws range_error");
                  continue;
                }
              if (not any)
                {
                  if (b.net.exist_aux_net() or b.net.get_num_nodes() != size_t(s.n))
                    aux.fail(s, what + "adds super nodes without supplies or demands");
                  continue;
                }

              // Feasible if the flows from the supplies to the demands fill
              // all the arcs of the super nodes
              long long supply = 0, demand = 0;
              for (long long x : sup.w)
                (x > 0 ? supply : demand) += std::abs(x);
              Spec x = s;
              const int from = x.n++, to = x.n++;
              for (int v = 0; v < s.n; ++v)
                if (sup.w[size_t(v)] > 0)
                  {
                    x.arcs.push_back({from, v});
                    x.w.push_back(sup.w[size_t(v)]);
                  }
                else if (sup.w[size_t(v)] < 0)
                  {
                    x.arcs.push_back({v, to});
                    x.w.push_back(-sup.w[size_t(v)]);
                  }
              const bool want = supply == demand and supply > 0
                and max_flow_units(x, from, to) == supply;

              if (supply == 0 or demand == 0)
                {
                  // Without a super source, or without a super sink:
                  // nothing can be shipped
                  trivial.run();
                  if (b.net.is_feasible())
                    trivial.fail(s, what + "says feasible");
                }
              else if (not (b.net.is_single_source() and b.net.is_single_sink()))
                aux.fail(s, what + std::to_string(b.net.get_src_nodes().size()) + " sources and "
                         + std::to_string(b.net.get_sink_nodes().size()) + " sinks");
              else
                {
                  feasible.run();
                  edmonds_karp_maximum_flow(b.net);
                  if (b.net.is_feasible() != want)
                    feasible.fail(s, what + (want ? "says infeasible" : "says feasible"));
                }

              freed.run();
              b.net.free_aux_net();
              if (b.net.exist_aux_net() or b.net.get_num_nodes() != size_t(s.n)
                  or b.net.get_num_arcs() != s.num_arcs())
                freed.fail(s, what + "the network is not as it was");
            }
        });
    });
  aux.expect();
  feasible.expect();
  trivial.expect();
  freed.expect();
}

// ======================================================================
// Copies

namespace
{
  /// What a network holds, in the order of its nodes and arcs: the info
  /// of each node, and the ends, capacity and flow of each arc, with what
  /// extra(node) and extra(arc) add as numbers
  template <class Net, class Node_Extra, class Arc_Extra>
  std::vector<double> contents(const Net & net, Node_Extra node_extra, Arc_Extra arc_extra)
  {
    std::vector<double> r;
    for (typename Net::Node_Iterator it(net); it.has_curr(); it.next_ne())
      {
        r.push_back(it.get_curr()->get_info());
        r.push_back(node_extra(it.get_curr()));
      }
    for (typename Net::Arc_Iterator it(net); it.has_curr(); it.next_ne())
      {
        auto a = it.get_curr();
        r.push_back(net.get_src_node(a)->get_info());
        r.push_back(net.get_tgt_node(a)->get_info());
        r.push_back(double(a->cap));
        r.push_back(double(a->flow));
        r.push_back(arc_extra(a));
      }
    return r;
  }

  /** Checks the copies and moves of networks of type Net made from the
      specs of 3 nodes and 2 arcs, each with a flow in it: those of the
      constructor and the assignment keep what contents() reads, the
      assignment the cookie of its target, and the moves the contents.
      prepare(b) gives the network what its type adds. */
  template <class Net, class Node_Extra, class Arc_Extra, class Prepare>
  void check_copies(Check & ctor, Check & assign, Check & cookie, Check & move,
                    Node_Extra node_extra, Arc_Extra arc_extra, Prepare prepare)
  {
    for_each_spec(domain(3, 2), true, [&] (const Spec & raw)
      {
        const Spec s = structural(raw);
        Built_Net<Net> b(s);
        if (s.n >= 2)
          {
            b.pin(0, s.n - 1);
            put_initial_flow(b);
          }
        prepare(b);
        const auto want = contents(b.net, node_extra, arc_extra);

        ctor.run();
        {
          const Net copy(b.net);
          if (contents(copy, node_extra, arc_extra) != want)
            ctor.fail(s, "the copy is not the network");
        }

        assign.run();
        cookie.run();
        {
          Built_Net<Net> other(structural(raw));
          int mark = 0;
          other.net.get_cookie() = &mark;
          other.net = b.net;
          if (contents(other.net, node_extra, arc_extra) != want)
            assign.fail(s, "the network assigned is not the network");
          if (other.net.get_cookie() != &mark)
            cookie.fail(s, "the cookie of the target is lost");
        }

        move.run();
        {
          Built_Net<Net> fresh(s);
          if (s.n >= 2)
            {
              fresh.pin(0, s.n - 1);
              put_initial_flow(fresh);
            }
          prepare(fresh);
          const auto before = contents(fresh.net, node_extra, arc_extra);
          Net moved(std::move(fresh.net));
          Net target;
          target = std::move(moved);
          if (contents(target, node_extra, arc_extra) != before)
            move.fail(s, "the network moved is not the network");
        }
      });
  }

  template <class P>
  double nothing(P) { return 0; }
}

TEST(ExhaustiveNetworkCopies, CopiesKeepTheNetwork)
{
  using Plain = Net_Graph<Net_Node<int>, Net_Arc<int, long>>;
  using Cost = Net_Cost_Graph<Net_Cost_Node<int>, Net_Cost_Arc<Empty_Class, long>>;
  using Cap = Net_Cap_Graph<Net_Cap_Node<int, long>, Net_Arc<int, long>>;
  using Sup = Net_Sup_Dem_Graph<Net_Sup_Dem_Node<int, long>, Net_Arc<int, long>>;

  Check plain_ctor("Net_Graph(const Net_Graph &)");
  Check plain_assign("Net_Graph::operator=(const Net_Graph &)");
  // operator=() copies and swaps, and the target keeps its cookie
  Check plain_cookie("Net_Graph::operator=(const Net_Graph &), the cookie");
  Check plain_move("Net_Graph(Net_Graph &&) and operator=(Net_Graph &&)");
  check_copies<Plain>(plain_ctor, plain_assign, plain_cookie, plain_move,
                      nothing<Plain::Node *>, nothing<Plain::Arc *>, [] (auto &) {});

  Check cost_ctor("Net_Cost_Graph(const Net_Cost_Graph &)");
  Check cost_assign("Net_Cost_Graph::operator=(const Net_Cost_Graph &)");
  Check cost_cookie("Net_Cost_Graph::operator=(const Net_Cost_Graph &), the cookie");
  Check cost_move("Net_Cost_Graph(Net_Cost_Graph &&) and operator=(Net_Cost_Graph &&)");
  check_copies<Cost>(cost_ctor, cost_assign, cost_cookie, cost_move, nothing<Cost::Node *>,
                     [] (Cost::Arc * a) { return double(a->cost); },
                     [] (Built_Net<Cost> & b)
                       {
                         long c = 1;
                         for (auto a : b.arcs)
                           a->cost = c++;
                       });

  // The node fields, small numbers, as one number of a few digits each
  auto digits = [] (std::initializer_list<long> fields)
    {
      double r = 0;
      for (long x : fields)
        r = 100 * r + double(x);
      return r;
    };

  Check cap_ctor("Net_Cap_Graph(const Net_Cap_Graph &), the capacities and flows of the nodes");
  Check cap_assign("Net_Cap_Graph::operator=(const Net_Cap_Graph &), the capacities and flows"
                   " of the nodes");
  Check cap_cookie("Net_Cap_Graph::operator=(const Net_Cap_Graph &), the cookie");
  Check cap_move("Net_Cap_Graph(Net_Cap_Graph &&) and operator=(Net_Cap_Graph &&)");
  check_copies<Cap>(cap_ctor, cap_assign, cap_cookie, cap_move,
                    [&] (Cap::Node * p) { return digits({p->max_cap, p->in_flow, p->out_flow}); },
                    nothing<Cap::Arc *>,
                    [] (Built_Net<Cap> & b)
                      {
                        long c = 1;
                        for (auto p : b.nodes)
                          {
                            Cap::set_node_cap(p, c);
                            p->in_flow = c + 10;
                            p->out_flow = c + 20;
                            ++c;
                          }
                      });

  Check sup_ctor("Net_Sup_Dem_Graph(const Net_Sup_Dem_Graph &), the supplies, capacities and"
                 " flows of the nodes");
  Check sup_assign("Net_Sup_Dem_Graph::operator=(const Net_Sup_Dem_Graph &), the supplies,"
                   " capacities and flows of the nodes");
  Check sup_cookie("Net_Sup_Dem_Graph::operator=(const Net_Sup_Dem_Graph &), the cookie");
  Check sup_move("Net_Sup_Dem_Graph(Net_Sup_Dem_Graph &&) and its move assignment");
  check_copies<Sup>(sup_ctor, sup_assign, sup_cookie, sup_move,
                    [&] (Sup::Node * p)
                      {
                        return digits({p->supply_flow, p->out_cap, p->in_cap, p->in_flow,
                                       p->out_flow});
                      },
                    nothing<Sup::Arc *>,
                    [] (Built_Net<Sup> & b)
                      {
                        long c = 1;
                        for (auto p : b.nodes)
                          {
                            p->supply_flow = c;
                            p->out_cap = c + 10;
                            p->in_cap = c + 20;
                            p->in_flow = c + 30;
                            p->out_flow = c + 40;
                            ++c;
                          }
                      });

  for (const Check * c : {&plain_ctor, &plain_assign, &plain_cookie, &plain_move, &cost_ctor,
                          &cost_assign, &cost_cookie, &cost_move, &cap_ctor, &cap_assign,
                          &cap_cookie, &cap_move, &sup_ctor, &sup_assign, &sup_cookie, &sup_move})
    c->expect();
}

// ======================================================================
// Networks when memory runs out

namespace
{
  /// The allocations that op(state) makes on a new state from make
  template <class Make, class Op>
  size_t allocations_of(Make make, Op op)
  {
    auto st = make();
    const size_t before = allocations;
    op(*st);
    return allocations - before;
  }

  /** Runs op(state) on a new state from make, with the allocation number
      k failing, for each k of the allocations that op makes, and then
      after(state, k, threw); the k for which leave_out(k) is true, where a
      failure would end the program, do not run. Nothing else may allocate
      while op runs. */
  template <class Make, class Op, class After, class Leave_Out>
  void for_each_failure(Make make, Op op, After after, Leave_Out leave_out)
  {
    const size_t points = allocations_of(make, op);
    for (size_t k = 1; k <= points; ++k)
      {
        if (leave_out(k))
          continue;
        auto st = make();
        bool threw = false;
        {
          Fail_At fail(k);
          try
            {
              op(*st);
            }
          catch (const std::bad_alloc &)
            {
              threw = true;
            }
        }
        after(*st, k, threw);
      }
  }

  template <class Make, class Op, class After>
  void for_each_failure(Make make, Op op, After after)
  {
    for_each_failure(make, op, after, [] (size_t) { return false; });
  }

  /// What a failure must leave as it was in a network: its numbers of
  /// nodes and arcs, its sources and sinks by number, and its flags
  template <class Net>
  std::vector<long> net_state(const Net & net)
  {
    std::vector<long> r = {long(net.get_num_nodes()), long(net.get_num_arcs()),
                           long(net.get_src_nodes().size()), long(net.get_sink_nodes().size()),
                           long(net.with_super_source), long(net.with_super_sink)};
    for (auto it = net.get_src_nodes().get_it(); it.has_curr(); it.next_ne())
      r.push_back(it.get_curr()->get_info());
    for (auto it = net.get_sink_nodes().get_it(); it.has_curr(); it.next_ne())
      r.push_back(1000 + it.get_curr()->get_info());
    std::sort(r.begin() + 6, r.end());
    return r;
  }

  /// Whether the sets of sources and sinks of net are those that its arcs
  /// give
  template <class Net>
  bool ends_kept(const Net & net)
  {
    for (typename Net::Node_Iterator it(net); it.has_curr(); it.next_ne())
      {
        auto p = it.get_curr();
        bool in = false, out = false;
        for (typename Net::Node_Arc_Iterator j(p); j.has_curr(); j.next_ne())
          {
            in = in or net.get_tgt_node(j.get_curr()) == p;
            out = out or net.get_src_node(j.get_curr()) == p;
          }
        if (net.is_source(p) == in or net.is_sink(p) == out)
          return false;
      }
    return true;
  }
}

class ExhaustiveFlowMemory : public ::testing::Test
{
protected:
  void SetUp() override
  {
    if (ALEPH_ALLOCATION_FAILURE_UNDER_TSAN)
      GTEST_SKIP() << "ThreadSanitizer replaces the global operator new";
  }
};

TEST_F(ExhaustiveFlowMemory, SuperNodesWhenMemoryRunsOut)
{
  using Net = Net_Graph<Net_Node<int>, Net_Arc<int, long>>;
  using B = Built_Net<Net>;
  // A failure leaves the network as it was
  Check source("make_super_source(), a failed allocation");
  Check sink("make_super_sink(), a failed allocation");
  Check nodes("make_super_nodes(), a failed allocation");
  // A failure leaves the super node, maybe with fewer arcs, and the sets
  // of sources and sinks right; another call goes on
  Check unmake("unmake_super_source() and unmake_super_sink(), a failed allocation");
  // Even after one that took some arcs of the super node, whose ends are
  // sources, or sinks, again: the network keeps its super node
  Check again("unmake_super_source() and unmake_super_sink(), another call after a failed one");

  for_each_spec(domain(3, 3), true, [&] (const Spec & raw)
    {
      const Spec s = structural(raw);
      // Not structured bindings, which the lambdas below could not take with
      // Clang 14
      const auto ends = ends_by_arcs(s);
      const auto & sources = ends.first;
      const auto & sinks = ends.second;
      auto make = [&] { return std::make_unique<B>(s); };
      const auto before = net_state(B(s).net);
      auto after = [&] (Check & check)
        {
          return [&] (B & b, size_t k, bool threw)
            {
              check.run();
              if (threw and net_state(b.net) != before)
                check.fail(s, "the allocation " + std::to_string(k)
                           + " fails, and the network is not as it was");
            };
        };
      if (sources.size() > 1)
        for_each_failure(make, [] (B & b) { b.net.make_super_source(); }, after(source));
      if (sinks.size() > 1)
        for_each_failure(make, [] (B & b) { b.net.make_super_sink(); }, after(sink));
      if (sources.empty() or sinks.empty() or (sources.size() < 2 and sinks.size() < 2))
        return;

      for_each_failure(make, [] (B & b) { b.net.make_super_nodes(); }, after(nodes));

      // The super nodes made, each undone with a failure
      auto made = [&] { auto b = make(); b->net.make_super_nodes(); return b; };
      for (int which = 0; which < 2; ++which)
        {
          auto op = [which] (B & b)
            {
              if (which == 0)
                b.net.unmake_super_source();
              else
                b.net.unmake_super_sink();
            };
          const auto undone = [&]
            {
              auto b = made();
              op(*b);
              return net_state(b->net);
            }();
          for_each_failure(made, op, [&] (B & b, size_t k, bool threw)
            {
              unmake.run();
              const std::string at = "the allocation " + std::to_string(k) + " fails";
              if (not ends_kept(b.net))
                {
                  unmake.fail(s, at + ", and the sources or sinks are wrong");
                  return;
                }
              if (not threw)
                return;
              again.run();
              op(b);
              if (net_state(b.net) != undone)
                again.fail(s, at + ", and another call does not undo the super node");
            });
        }
    });
  for (const Check * c : {&source, &sink, &nodes, &unmake, &again})
    c->expect();
}

TEST_F(ExhaustiveFlowMemory, CopiesWhenMemoryRunsOut)
{
  using Net = Net_Graph<Net_Node<int>, Net_Arc<int, long>>;
  using Other = Net_Graph<Net_Node<int>, Net_Arc<int, int>>;
  // A copy that fails leaves the target empty, even when memory stays
  // out, though removing an arc of a network allocates
  Check single("copy_graph(), into a network, a failed allocation");
  Check persistent("copy_graph(), into a network, without memory");
  Check inter_single("inter_copy_graph(), into a network, a failed allocation");
  Check inter_persistent("inter_copy_graph(), into a network, without memory");

  struct Pair
  {
    Built_Net<Net> to;
    Built_Net<Net> from;
    Pair(const Spec & a, const Spec & b) : to(a), from(b) {}
  };
  struct Inter_Pair
  {
    Built_Net<Net> to;
    Built_Net<Other> from;
    Inter_Pair(const Spec & a, const Spec & b) : to(a), from(b) {}
  };

  Spec target;
  target.n = 3;
  target.arcs = {{0, 1}, {1, 2}};
  target = structural(target);
  for_each_spec(domain(3, 2), true, [&] (const Spec & raw)
    {
      const Spec s = structural(raw);
      auto make = [&] { return std::make_unique<Pair>(target, s); };
      for_each_failure(make, [] (Pair & p) { copy_graph(p.to.net, p.from.net); },
                       [&] (Pair & p, size_t k, bool threw)
                         {
                           single.run();
                           if (threw and (p.to.net.get_num_nodes() != 0
                                          or p.to.net.get_num_arcs() != 0))
                             single.fail(s, "the allocation " + std::to_string(k)
                                         + " fails, and the target is not empty");
                         });
      auto make_inter = [&] { return std::make_unique<Inter_Pair>(target, s); };
      for_each_failure(make_inter, [] (Inter_Pair & p) { inter_copy_graph(p.to.net, p.from.net); },
                       [&] (Inter_Pair & p, size_t k, bool threw)
                         {
                           inter_single.run();
                           if (threw and (p.to.net.get_num_nodes() != 0
                                          or p.to.net.get_num_arcs() != 0))
                             inter_single.fail(s, "the allocation " + std::to_string(k)
                                               + " fails, and the target is not empty");
                         });

      // No memory from the first allocation on
      for (Check * c : {&persistent, &inter_persistent})
        {
          Pair p(target, s);
          Inter_Pair q(target, s);
          bool threw = false;
          {
            No_Memory none;
            try
              {
                if (c == &persistent)
                  copy_graph(p.to.net, p.from.net);
                else
                  inter_copy_graph(q.to.net, q.from.net);
              }
            catch (const std::bad_alloc &)
              {
                threw = true;
              }
          }
          const auto & to = c == &persistent ? p.to.net : q.to.net;
          c->run();
          if (not threw)
            c->fail(s, "copies without memory");
          else if (to.get_num_nodes() != 0 or to.get_num_arcs() != 0)
            c->fail(s, "the target keeps " + std::to_string(to.get_num_nodes()) + " nodes and "
                    + std::to_string(to.get_num_arcs()) + " arcs");
        }
    });
  single.expect();
  persistent.expect();
  inter_single.expect();
  inter_persistent.expect();
}

TEST_F(ExhaustiveFlowMemory, NodeCapacitiesWhenMemoryRunsOut)
{
  using Net = Net_Cap_Graph<Net_Cap_Node<int, long>, Net_Arc<int, long>>;
  // The copy assignment gives the strong guarantee: a copy that fails
  // leaves the network as it was, with its auxiliary network
  Check assign("Net_Cap_Graph::operator=(const Net_Cap_Graph &), a failed allocation");
  // free_aux_net(), the destructor and the move assignment free the
  // auxiliary network without allocating, as removing the arcs of a
  // network would, so that nothing can fail there
  Check freeing("free_aux_net(), without allocating");
  Check dtor("~Net_Cap_Graph(), with its auxiliary network, without allocating");
  Check move("Net_Cap_Graph::operator=(Net_Cap_Graph &&), without allocating");

  struct Two
  {
    Built_Net<Net> x;
    Built_Net<Net> y;
    Two(const Spec & a, const Spec & b) : x(a), y(b)
    {
      long c = 1;
      for (auto p : x.nodes)
        Net::set_node_cap(p, c++);
      x.net.compute_aux_net();
    }
  };

  Spec other;
  other.n = 2;
  other.arcs = {{0, 1}};
  other = structural(other);
  for_each_spec(domain(3, 2), true, [&] (const Spec & raw)
    {
      const Spec s = structural(raw);
      auto make = [&] { return std::make_unique<Two>(s, other); };
      const auto before = [&]
        {
          auto t = make();
          return contents(t->x.net, [] (Net::Node * p) { return double(p->max_cap); },
                          nothing<Net::Arc *>);
        }();
      for_each_failure(make, [] (Two & t) { t.x.net = t.y.net; },
                       [&] (Two & t, size_t k, bool threw)
                         {
                           assign.run();
                           const auto now = contents(t.x.net, [] (Net::Node * p)
                                                       { return double(p->max_cap); },
                                                     nothing<Net::Arc *>);
                           if (threw and (now != before or not t.x.net.has_aux_net()))
                             assign.fail(s, "the allocation " + std::to_string(k)
                                         + " fails, and the network is not as it was");
                         });
      freeing.run();
      if (const size_t k = allocations_of(make, [] (Two & t) { t.x.net.free_aux_net(); }); k != 0)
        freeing.fail(s, "makes " + std::to_string(k) + " allocations");
      struct Owned
      {
        std::unique_ptr<Two> two;
        Owned(const Spec & a, const Spec & b) : two(std::make_unique<Two>(a, b)) {}
      };
      dtor.run();
      if (const size_t k = allocations_of([&] { return std::make_unique<Owned>(s, other); },
                                          [] (Owned & o) { o.two.reset(); });
          k != 0)
        dtor.fail(s, "makes " + std::to_string(k) + " allocations");
      move.run();
      if (const size_t k = allocations_of(make, [] (Two & t) { t.x.net = std::move(t.y.net); });
          k != 0)
        move.fail(s, "makes " + std::to_string(k) + " allocations");
    });
  assign.expect();
  freeing.expect();
  dtor.expect();
  move.expect();
}

namespace
{
  using Long_Cost_Net = Net_Cost_Graph<Net_Cost_Node<int>, Net_Cost_Arc<Empty_Class, long>>;

  /** A cost network that tells whether a failed allocation came in one of
      its removals of a node, which allocate in a network: there
      solve_transshipment(), which removes its super nodes, may leave them.
      Where the removal starts is not fixed: Dijkstra in
      successive_shortest_paths() breaks ties by the addresses of the nodes,
      and so the number of its allocations changes from run to run. */
  struct Removal_Watched_Net : Long_Cost_Net
  {
    bool failed_in_removal = false;

    void remove_node(Node * p) override
    {
      try
        {
          Long_Cost_Net::remove_node(p);
        }
      catch (const std::bad_alloc &)
        {
          failed_in_removal = true;
          throw;
        }
    }
  };
}

TEST_F(ExhaustiveFlowMemory, CheapestFlowsWhenMemoryRunsOut)
{
  using Net = Long_Cost_Net;
  using Watched = Removal_Watched_Net;
  using F = Net::Flow_Type;
  // A throw leaves the flows as they were, though Dijkstra allocates its
  // heap after the flows of a path have changed
  Check ssp("successive_shortest_paths(), a failed allocation");
  // A throw leaves the network as it was, unless the failure came where
  // the super nodes are removed, outside any destructor: then they may
  // stay, with the network and its sources and sinks right
  Check transshipment("solve_transshipment(), a failed allocation");
  Check removal("solve_transshipment(), a failed allocation in the removal of its super nodes");

  small_structural_specs<Net>()([&] (const Spec & s)
    {
      if (s.n < 2)
        return;
      const auto cost = structural_costs(s);
      auto make = [&]
        {
          auto b = std::make_unique<Built_Net<Net>>(s, s.w, cost);
          b->pin(0, s.n - 1);
          return b;
        };
      for_each_failure(make, [] (Built_Net<Net> & b) { successive_shortest_paths(b.net); },
                       [&] (Built_Net<Net> & b, size_t k, bool threw)
                         {
                           ssp.run();
                           bool zero = true;
                           for (auto a : b.arcs)
                             zero = zero and a->flow == F{0};
                           if (threw and not zero)
                             ssp.fail(s, "the allocation " + std::to_string(k)
                                      + " fails, and the flows changed");
                         });

      // Node 0 supplies one unit to the last node, which needs it, if the
      // network can ship it
      std::vector<long long> d(size_t(s.n), 0);
      d[0] = 1;
      d[size_t(s.n - 1)] = -1;
      if (best_transshipment(s, cost, d) < 0)
        return;
      auto plain = [&] { return std::make_unique<Built_Net<Watched>>(s, s.w, cost); };
      auto demand_of = [d] (const Built_Net<Watched> & b)
        {
          return [&b, d] (Watched::Node * p)
            {
              for (int v = 0; v < int(b.nodes.size()); ++v)
                if (b.nodes[size_t(v)] == p)
                  return amount<F>(d[size_t(v)]);
              return F{0};
            };
        };
      for_each_failure(plain,
                       [&] (Built_Net<Watched> & b) { solve_transshipment(b.net, demand_of(b)); },
                       [&] (Built_Net<Watched> & b, size_t k, bool threw)
                         {
                           const std::string at = "the allocation " + std::to_string(k) + " fails";
                           if (b.net.failed_in_removal)
                             {
                               // The nodes and arcs of the network stay
                               removal.run();
                               const std::set<Watched::Node *> nodes(b.nodes.begin(),
                                                                     b.nodes.end());
                               const std::set<Watched::Arc *> arcs(b.arcs.begin(), b.arcs.end());
                               size_t kept_nodes = 0, kept_arcs = 0;
                               for (Node_Iterator<Watched> it(b.net); it.has_curr(); it.next_ne())
                                 kept_nodes += nodes.count(it.get_curr());
                               for (Arc_Iterator<Watched> it(b.net); it.has_curr(); it.next_ne())
                                 kept_arcs += arcs.count(it.get_curr());
                               if (not threw)
                                 removal.fail(s, at + " in a removal, and nothing throws");
                               else if (kept_nodes != nodes.size() or kept_arcs != arcs.size())
                                 removal.fail(s, at + ", and nodes or arcs of the network go");
                               else if (not ends_kept(b.net))
                                 removal.fail(s, at + ", and the sources or sinks are wrong");
                               return;
                             }
                           transshipment.run();
                           if (threw and (b.net.get_num_nodes() != size_t(s.n)
                                          or b.net.get_num_arcs() != s.num_arcs()))
                             transshipment.fail(s, at + ", and the super nodes stay");
                         });
    });
  ssp.expect();
  transshipment.expect();
  removal.expect();
}

// ======================================================================
// Johnson, with integer weights next to the limits

TEST(ExhaustiveJohnson, IntegerDistancesThatFitDoNotOverflow)
{
  using GT = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
  constexpr long long Max = std::numeric_limits<int>::max();
  Check build("Johnson(g)");
  // A reweighted arc, w + h(u) - h(v), passes max() when h(v) < h(u),
  // though no distance does
  Check build_over("Johnson(g), a reweighted arc beyond max()");
  Check distances("Johnson::get_distance()");
  // Dijkstra adds the reweighted arcs, and a sum d(s, u) + w + h(s) -
  // h(v) may pass max(), though d(s, v) fits
  Check distances_over("Johnson::get_distance(), a reweighted sum beyond max()");
  Check negative("Johnson(g), with a negative cycle");

  auto run = [&] (const Spec & s)
    {
      bool neg = false;
      const auto d = oracle::shortest_distances(s, true, neg);
      GT g;
      std::vector<GT::Node *> nodes;
      for (int i = 0; i < s.n; ++i)
        nodes.push_back(g.insert_node(i));
      for (size_t i = 0; i < s.arcs.size(); ++i)
        g.insert_arc(nodes[size_t(s.arcs[i][0])], nodes[size_t(s.arcs[i][1])], int(s.w[i]));
      if (neg)
        {
          negative.run();
          try
            {
              Johnson<GT> j(g);
              negative.fail(s, "does not throw");
            }
          catch (const std::domain_error &) {}
          catch (const std::exception & e)
            {
              negative.fail(s, std::string("throws ") + e.what());
            }
          return;
        }

      // The potentials: the distances from a node joined to all by arcs of 0
      std::vector<long long> h(size_t(s.n), 0);
      for (int u = 0; u < s.n; ++u)
        for (int v = 0; v < s.n; ++v)
          if (d[size_t(u)][size_t(v)] != oracle::Inf)
            h[size_t(v)] = std::min(h[size_t(v)], d[size_t(u)][size_t(v)]);
      bool over = false;
      for (size_t i = 0; i < s.arcs.size(); ++i)
        over = over or s.w[i] + h[size_t(s.arcs[i][0])] - h[size_t(s.arcs[i][1])] > Max;
      Check & c = over ? build_over : build;
      c.run();
      std::unique_ptr<Johnson<GT>> j;
      try
        {
          j = std::make_unique<Johnson<GT>>(g);
        }
      catch (const std::exception & e)
        {
          c.fail(s, std::string("throws ") + e.what());
          return;
        }

      for (int a = 0; a < s.n; ++a)
        {
          bool sum_over = false;
          for (size_t i = 0; i < s.arcs.size(); ++i)
            {
              const int u = s.arcs[i][0], v = s.arcs[i][1];
              if (d[size_t(a)][size_t(u)] != oracle::Inf)
                sum_over = sum_over or d[size_t(a)][size_t(u)] + s.w[i] + h[size_t(a)]
                                       - h[size_t(v)] > Max;
            }
          Check & dc = sum_over ? distances_over : distances;
          dc.run();
          for (int z = 0; z < s.n; ++z)
            {
              const long long want = d[size_t(a)][size_t(z)];
              const std::string at = "from " + std::to_string(a) + " to " + std::to_string(z)
                + ": ";
              try
                {
                  const long long got = j->get_distance(nodes[size_t(a)], nodes[size_t(z)]);
                  if (want == oracle::Inf ? got != Max : want > Max or got != want)
                    {
                      dc.fail(s, at + "gives " + std::to_string(got) + ", not "
                              + (want == oracle::Inf ? "max()" : std::to_string(want)));
                      break;
                    }
                }
              catch (const std::overflow_error & e)
                {
                  if (want != oracle::Inf and want <= Max)
                    {
                      dc.fail(s, at + "throws " + e.what() + " for " + std::to_string(want));
                      break;
                    }
                }
            }
        }
    };

  // Small weights, on every digraph of 3 nodes and 3 arcs
  for_each_spec(domain(3, 3), true, [&] (const Spec & s)
    {
      for_each_assignment(s, {-1, 0, 2}, run);
    });
  // Weights next to the limits, on every one of 3 nodes and 2 arcs
  Domain d = domain(3, 2);
  for_each_spec(d, true, [&] (const Spec & s)
    {
      for_each_assignment(s, {-10, 1, Max - 5}, run);
    });
  build.expect();
  build_over.expect();
  distances.expect();
  distances_over.expect();
  negative.expect();
}

// ======================================================================
// The members of the networks, and their residual networks

TYPED_TEST(ExhaustiveMaxFlow, MembersKeepTheirContracts)
{
  using Net = TypeParam;
  using F = typename Net::Flow_Type;
  Check sums("get_in_cap(), get_out_cap(), get_in_flow(), get_out_flow() and the degrees");
  Check lists("out_arcs(), in_arcs(), out_nodes() and in_nodes()");
  Check checks("check_node(), check_network() and flow_value()");
  Check setters("set_cap(), set_flow(), get_cap(), get_flow() and reset()");

  small_structural_specs<Net>()([&] (const Spec & s)
    {
      for (auto [a, z] : ends_of(s))
        {
          Built_Net<Net> b(s);
          b.pin(a, z);
          put_initial_flow(b);

          sums.run();
          lists.run();
          for (typename Net::Node_Iterator it(b.net); it.has_curr(); it.next_ne())
            {
              auto p = it.get_curr();
              F in_cap{0}, out_cap{0}, in_flow{0}, out_flow{0};
              size_t in = 0, out = 0;
              std::multiset<typename Net::Arc *> ins, outs;
              std::multiset<typename Net::Node *> from, to;
              for (auto arc : b.arcs)
                {
                  if (b.net.get_tgt_node(arc) == p)
                    {
                      in_cap += arc->cap;
                      in_flow += arc->flow;
                      ++in;
                      ins.insert(arc);
                      from.insert(b.net.get_src_node(arc));
                    }
                  if (b.net.get_src_node(arc) == p)
                    {
                      out_cap += arc->cap;
                      out_flow += arc->flow;
                      ++out;
                      outs.insert(arc);
                      to.insert(b.net.get_tgt_node(arc));
                    }
                }
              if (b.net.get_in_cap(p) != in_cap or b.net.get_out_cap(p) != out_cap
                  or b.net.get_in_flow(p) != in_flow or b.net.get_out_flow(p) != out_flow
                  or b.net.get_in_degree(p) != in or b.net.get_out_degree(p) != out)
                sums.fail(s, where(b, a, z) + "a wrong sum or degree at the node " + name_of(p));
              auto as_set = [] (const auto & l)
                {
                  std::multiset<std::remove_cvref_t<decltype(l.get_first())>> r;
                  for (auto j = l.get_it(); j.has_curr(); j.next_ne())
                    r.insert(j.get_curr());
                  return r;
                };
              if (as_set(b.net.in_arcs(p)) != ins or as_set(b.net.out_arcs(p)) != outs
                  or as_set(b.net.in_nodes(p)) != from or as_set(b.net.out_nodes(p)) != to)
                lists.fail(s, where(b, a, z) + "a wrong list at the node " + name_of(p));
            }

          // A valid flow passes, and the value is that of the source
          checks.run();
          if (s.n >= 2 and (not b.net.check_network() or b.net.flow_value() != b.value()))
            checks.fail(s, where(b, a, z) + "the flow to start from is refused");
          else if (s.n >= 2)
            for (auto arc : b.arcs)
              if (b.net.get_src_node(arc) != b.net.get_tgt_node(arc)
                  and arc->cap > arc->flow and b.net.get_tgt_node(arc) != b.snk)
                {
                  // One more unit on an arc into an inner node breaks it
                  const F before = arc->flow;
                  arc->flow = std::min(arc->cap, before + amount<F>(1));
                  if (b.net.check_node(b.net.get_tgt_node(arc)) or b.net.check_network())
                    checks.fail(s, where(b, a, z) + "an unbalanced flow passes");
                  arc->flow = before;
                  break;
                }

          setters.run();
          for (auto arc : b.arcs)
            {
              const F cap = arc->cap, flow = arc->flow;
              bool ok = b.net.get_cap(arc) == cap and b.net.get_flow(arc) == flow;
              try
                {
                  b.net.set_flow(arc, cap + amount<F>(1));
                  ok = false;
                }
              catch (const std::out_of_range &) {}
              try
                {
                  if (flow > F{0})
                    {
                      b.net.set_cap(arc, flow - amount<F>(1));
                      ok = false;
                    }
                }
              catch (const std::out_of_range &) {}
              ok = ok and arc->cap == cap and arc->flow == flow;
              b.net.set_cap(arc, cap + amount<F>(1));
              b.net.set_flow(arc, cap);
              ok = ok and arc->cap == cap + amount<F>(1) and arc->flow == cap;
              b.net.set_flow(arc, flow);
              b.net.set_cap(arc, cap);
              if (not ok)
                setters.fail(s, where(b, a, z) + "a setter does not keep its contract");
            }
          {
            // insert_arc() refuses a flow above the capacity, and the
            // network stays as it was
            const size_t arcs = b.net.get_num_arcs();
            const auto sources = b.net.get_src_nodes().size();
            const auto sinks = b.net.get_sink_nodes().size();
            bool threw = false;
            try
              {
                if constexpr (Cost_Network<Net>)
                  b.net.Net_Graph<typename Net::Node, typename Net::Arc>::insert_arc(
                    b.src, b.snk, amount<F>(1), amount<F>(2));
                else
                  b.net.insert_arc(b.src, b.snk, amount<F>(1), amount<F>(2));
              }
            catch (const std::overflow_error &)
              {
                threw = true;
              }
            if (not threw or b.net.get_num_arcs() != arcs or b.net.get_src_nodes().size() != sources
                or b.net.get_sink_nodes().size() != sinks)
              setters.fail(s, where(b, a, z) + "insert_arc() takes a flow above the capacity");
          }
          b.net.reset();
          for (auto arc : b.arcs)
            if (arc->flow != F{0})
              {
                setters.fail(s, where(b, a, z) + "reset() leaves a flow");
                break;
              }
        }
    });
  sums.expect();
  lists.expect();
  checks.expect();
  setters.expect();
}

TYPED_TEST(ExhaustiveMinCost, ResidualNetworksMirrorTheFlow)
{
  using Net = TypeParam;
  using F = typename Net::Flow_Type;
  Check preflow("preflow_create_residual_net() and update_flow()");
  Check cost("build_residual_net(), check_residual_net() and residual_to_net()");

  small_structural_specs<Net>()([&] (const Spec & s)
    {
      for (auto [a, z] : ends_of(s))
        {
          if (a == z)
            continue;
          Built_Net<Net> b(s, s.w, structural_costs(s));
          b.pin(a, z);
          put_initial_flow(b);
          const auto flows = b.flows();

          preflow.run();
          {
            auto [rnet, rs, rt] = preflow_create_residual_net(b.net);
            bool ok = rnet.get_num_nodes() == b.net.get_num_nodes()
              and rnet.get_num_arcs() == 2 * b.net.get_num_arcs()
              and NODE_COOKIE(b.src) == static_cast<void *>(rs)
              and NODE_COOKIE(b.snk) == static_cast<void *>(rt);
            size_t images = 0;
            using Rnet = PP_Res_Net<Net>;
            for (typename Rnet::Arc_Iterator it(rnet); it.has_curr() and ok; it.next_ne())
              {
                auto r = it.get_curr();
                if (r->is_residual())
                  continue;
                ++images;
                void * from = b.net.get_src_node(r->img), * to = b.net.get_tgt_node(r->img);
                ok = r->cap == r->img->cap and r->flow == r->img->flow and r->dup->dup == r
                  and r->dup->flow == r->cap - r->flow
                  and NODE_COOKIE(rnet.get_src_node(r)) == from
                  and NODE_COOKIE(rnet.get_tgt_node(r)) == to;
              }
            b.net.reset();
            update_flow(rnet);
            if (not ok or images != b.net.get_num_arcs() or b.flows() != flows)
              preflow.fail(s, where(b, a, z) + "the residual network does not mirror the flow");
          }

          cost.run();
          {
            Residual_Net<F> rnet;
            DynMapTree<void *, void *> map;
            auto rs = build_residual_net(b.net, rnet, map);
            bool ok = check_residual_net(rnet) and rnet.get_num_arcs() == 2 * b.net.get_num_arcs()
              and rs == mapped_node<Net, Residual_Net<F>>(b.src)
              and map.size() == b.net.get_num_arcs();
            for (auto arc : b.arcs)
              {
                auto r = static_cast<typename Residual_Net<F>::Arc *>(map.find(arc));
                ok = ok and not r->is_residual and r->cap == arc->cap and r->flow == arc->flow
                  and r->cost == arc->cost and r->img->is_residual and r->img->cost == -arc->cost
                  and r->img->flow == arc->cap - arc->flow;
              }
            b.net.reset();
            residual_to_net<Net>(map);
            if (not ok or b.flows() != flows)
              cost.fail(s, where(b, a, z) + "the residual network does not mirror the flow");
          }
        }
    });
  preflow.expect();
  cost.expect();
}
