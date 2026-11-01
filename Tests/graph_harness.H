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
 * @file graph_harness.H
 * @brief Every small multigraph, on every kind of graph, for the tests.
 *
 * A Spec describes a small multigraph by its number of nodes and the list
 * of its arcs in the order in which they are inserted, each as the pair
 * (source, target) given to insert_arc(). for_each_spec() calls a function
 * with every Spec of a Domain: every labeled multigraph of up to
 * max_nodes nodes and max_arcs arcs, with loops and parallel arcs if the
 * domain allows them, with each arc inserted from either end (a graph
 * that is not directed takes u-v and v-u for the same arc, but stores it
 * from the end it was given) and with the arcs in two orders. The specs
 * come smallest first, so the first one that fails a test is minimal.
 *
 * build() makes a graph of any kind from a Spec. The info of each node is
 * its number and that of each arc its number in the Spec, so that the
 * tests compare by those numbers: the order of the nodes of List_SGraph,
 * of Ady_Mat and of some results follows their addresses.
 *
 * The type lists name the six kinds of graph for typed tests, and
 * run_with_small_stack() runs a function in a thread with a small stack,
 * where a search that recurses once per node overflows.
 *
 * The environment variable ALEPH_GRAPH_HARNESS sets the size of the
 * domains: heavy enlarges them, light takes one arc less, and normal is
 * the size that a build without a sanitizer takes; with one, the harness
 * runs many times slower, and takes light. ALEPH_GRAPH_HARNESS_SEED sets
 * the seed of the random tests and ALEPH_GRAPH_HARNESS_SHOW how many
 * failures each check prints (8).
 */

#ifndef ALEPH_TESTS_GRAPH_HARNESS_H
#define ALEPH_TESTS_GRAPH_HARNESS_H

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_graph.H>
#include <tpl_sgraph.H>
#include <tpl_agraph.H>

#include "graph_filter_catalog.H"

#if defined(__unix__) || defined(__APPLE__)
#  include <pthread.h>
#  define ALEPH_GRAPH_HARNESS_HAS_SMALL_STACK 1
#else
#  define ALEPH_GRAPH_HARNESS_HAS_SMALL_STACK 0
#endif

// A build with the address, thread or memory sanitizer
#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
#  define ALEPH_GRAPH_HARNESS_SANITIZED 1
#elif defined(__has_feature)
#  if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer) || \
      __has_feature(memory_sanitizer)
#    define ALEPH_GRAPH_HARNESS_SANITIZED 1
#  endif
#endif
#ifndef ALEPH_GRAPH_HARNESS_SANITIZED
#  define ALEPH_GRAPH_HARNESS_SANITIZED 0
#endif

namespace graph_harness
{
  /// A small multigraph: its nodes are 0 .. n - 1, and arcs[i] is the
  /// pair (source, target) of the i-th arc, in the order of insertion.
  /// w[i], if given, is the weight of the i-th arc.
  struct Spec
  {
    int n = 0;
    std::vector<std::array<int, 2>> arcs;
    std::vector<long long> w;

    [[nodiscard]] size_t num_arcs() const noexcept { return arcs.size(); }

    [[nodiscard]] bool has_loop() const noexcept
    {
      for (const auto & a : arcs)
        if (a[0] == a[1])
          return true;
      return false;
    }

    /// The spec as text, ready to be written back as a test case
    [[nodiscard]] std::string str() const
    {
      std::ostringstream s;
      s << "n=" << n << " arcs={";
      for (size_t i = 0; i < arcs.size(); ++i)
        {
          s << (i == 0 ? "" : " ") << arcs[i][0] << "->" << arcs[i][1];
          if (i < w.size())
            s << ':' << w[i];
        }
      s << '}';
      return s.str();
    }
  };

  /// Which multigraphs for_each_spec() gives
  struct Domain
  {
    int max_nodes = 4;
    int max_arcs = 3;
    bool loops = true;      ///< arcs from a node to itself
    bool parallel = true;   ///< several arcs between the same nodes
    bool directions = true; ///< each arc from either end (not directed)
    bool reversed = true;   ///< also the arcs in the reverse order
  };

  /// The sizes of the domains
  enum class Mode { light, normal, heavy };

  /// ALEPH_GRAPH_HARNESS=light, normal or heavy, or else light in a build
  /// with a sanitizer and normal without one
  inline Mode mode()
  {
    const char * e = std::getenv("ALEPH_GRAPH_HARNESS");
    const std::string m = e == nullptr ? "" : e;
    if (m == "heavy")
      return Mode::heavy;
    if (m == "normal")
      return Mode::normal;
    if (m == "light")
      return Mode::light;
    return ALEPH_GRAPH_HARNESS_SANITIZED ? Mode::light : Mode::normal;
  }

  /// Whether the mode is heavy, which asks for the larger domains
  inline bool heavy() { return mode() == Mode::heavy; }

  /// The domain of max_nodes nodes and max_arcs arcs, of one more of each
  /// in heavy mode, and of one arc less in light mode
  inline Domain domain(int max_nodes, int max_arcs)
  {
    Domain d;
    d.max_nodes = heavy() ? max_nodes + 1 : max_nodes;
    d.max_arcs = heavy() ? max_arcs + 1
                         : mode() == Mode::light ? max_arcs - 1 : max_arcs;
    return d;
  }

  /// The seed of the random tests: ALEPH_GRAPH_HARNESS_SEED, or seed
  inline unsigned seed(unsigned seed)
  {
    const char * e = std::getenv("ALEPH_GRAPH_HARNESS_SEED");
    return e == nullptr ? seed : unsigned(std::strtoul(e, nullptr, 10));
  }

  /// How many failures a check prints: ALEPH_GRAPH_HARNESS_SHOW, or 8
  inline size_t failures_shown()
  {
    const char * e = std::getenv("ALEPH_GRAPH_HARNESS_SHOW");
    return e == nullptr ? 8 : size_t(std::strtoul(e, nullptr, 10));
  }

  /// Calls f(spec) with every spec of d, smallest first, and returns how
  /// many there were. In a directed domain (u, v) and (v, u) are different
  /// arcs; otherwise each arc is also given from its other end.
  template <class F>
  size_t for_each_spec(const Domain & d, bool directed, F && f)
  {
    size_t count = 0;
    for (int n = 0; n <= d.max_nodes; ++n)
      {
        // The places where an arc can be: one per pair of nodes, or per
        // ordered pair in a digraph
        std::vector<std::array<int, 2>> places;
        for (int u = 0; u < n; ++u)
          for (int v = directed ? 0 : u; v < n; ++v)
            if (u != v or d.loops)
              places.push_back({u, v});
        const int p = int(places.size());

        for (int m = 0; m <= d.max_arcs; ++m)
          {
            if (m > 0 and (p == 0 or (not d.parallel and m > p)))
              break;

            // Every multiset of m places (a set if no parallel arcs), as a
            // nondecreasing (or increasing) sequence of place numbers
            std::vector<int> pick(size_t(m), 0);
            if (not d.parallel)
              for (int i = 0; i < m; ++i)
                pick[size_t(i)] = i;

            while (true)
              {
                std::vector<int> flippable;
                for (int i = 0; i < m; ++i)
                  if (places[size_t(pick[size_t(i)])][0] != places[size_t(pick[size_t(i)])][1])
                    flippable.push_back(i);
                const unsigned masks =
                  directed or not d.directions ? 1U : 1U << flippable.size();

                for (unsigned mask = 0; mask < masks; ++mask)
                  {
                    Spec s;
                    s.n = n;
                    for (int i = 0; i < m; ++i)
                      s.arcs.push_back(places[size_t(pick[size_t(i)])]);
                    for (size_t k = 0; k < flippable.size(); ++k)
                      if (mask & (1U << k))
                        std::swap(s.arcs[size_t(flippable[k])][0],
                                  s.arcs[size_t(flippable[k])][1]);
                    f(static_cast<const Spec &>(s));
                    ++count;
                    if (d.reversed and m > 1)
                      {
                        std::reverse(s.arcs.begin(), s.arcs.end());
                        f(static_cast<const Spec &>(s));
                        ++count;
                      }
                  }

                // The next multiset, in lexicographic order
                int i = m - 1;
                if (d.parallel)
                  {
                    while (i >= 0 and pick[size_t(i)] == p - 1)
                      --i;
                    if (i < 0)
                      break;
                    const int x = pick[size_t(i)] + 1;
                    for (int j = i; j < m; ++j)
                      pick[size_t(j)] = x;
                  }
                else
                  {
                    while (i >= 0 and pick[size_t(i)] == p - m + i)
                      --i;
                    if (i < 0)
                      break;
                    ++pick[size_t(i)];
                    for (int j = i + 1; j < m; ++j)
                      pick[size_t(j)] = pick[size_t(j - 1)] + 1;
                  }
              }
          }
      }
    return count;
  }

  /// Calls f(spec) with every simple graph of n nodes, for n from min_nodes
  /// to max_nodes: each set of pairs of distinct nodes, or of ordered
  /// pairs in a digraph, as arcs from the smaller node unless directed.
  /// Returns how many there were.
  template <class F>
  size_t for_each_simple_graph(int min_nodes, int max_nodes, bool directed, F && f)
  {
    size_t count = 0;
    for (int n = min_nodes; n <= max_nodes; ++n)
      {
        std::vector<std::array<int, 2>> pairs;
        for (int u = 0; u < n; ++u)
          for (int v = directed ? 0 : u + 1; v < n; ++v)
            if (u != v)
              pairs.push_back({u, v});
        for (unsigned long set = 0; set < (1UL << pairs.size()); ++set, ++count)
          {
            Spec s;
            s.n = n;
            for (size_t i = 0; i < pairs.size(); ++i)
              if (set & (1UL << i))
                s.arcs.push_back(pairs[i]);
            f(static_cast<const Spec &>(s));
          }
      }
    return count;
  }

  /// A random spec of n nodes and m arcs, with loops and parallel arcs if
  /// asked; parallel is ignored when m exceeds the number of places
  inline Spec random_spec(std::mt19937 & rng, int n, int m, bool directed,
                          bool loops = true, bool parallel = true)
  {
    Spec s;
    s.n = n;
    if (n == 0)
      return s;
    std::vector<std::array<int, 2>> places;
    for (int u = 0; u < n; ++u)
      for (int v = directed ? 0 : u; v < n; ++v)
        if (u != v or loops)
          places.push_back({u, v});
    if (places.empty())
      return s;
    if (not parallel and size_t(m) > places.size())
      parallel = true;
    std::vector<bool> taken(places.size(), false);
    std::uniform_int_distribution<size_t> pick(0, places.size() - 1);
    while (s.arcs.size() < size_t(m))
      {
        const size_t k = pick(rng);
        if (not parallel and taken[k])
          continue;
        taken[k] = true;
        auto a = places[k];
        if (not directed and rng() % 2 == 1)
          std::swap(a[0], a[1]);
        s.arcs.push_back(a);
      }
    return s;
  }

  /// Gives each arc of s a weight from lo to hi
  inline void random_weights(std::mt19937 & rng, Spec & s, long long lo, long long hi)
  {
    std::uniform_int_distribution<long long> w(lo, hi);
    s.w.clear();
    for (size_t i = 0; i < s.arcs.size(); ++i)
      s.w.push_back(w(rng));
  }

  /// A graph made from a spec, with its nodes and arcs by number
  template <class GT>
  struct Built
  {
    GT g;
    std::vector<typename GT::Node *> nodes;
    std::vector<typename GT::Arc *> arcs;

    explicit Built(const Spec & s)
    {
      for (int i = 0; i < s.n; ++i)
        nodes.push_back(g.insert_node(i));
      for (size_t i = 0; i < s.arcs.size(); ++i)
        arcs.push_back(g.insert_arc(nodes[size_t(s.arcs[i][0])],
                                    nodes[size_t(s.arcs[i][1])], int(i)));
    }

    Built(const Built &) = delete;
    Built & operator = (const Built &) = delete;
  };

  /// The number of a node of a built graph
  template <class Node>
  [[nodiscard]] int num(Node * p) { return p->get_info(); }

  /// The node of a graph whose number is i
  template <class GT>
  [[nodiscard]] typename GT::Node * node_of(const GT & g, int i)
  {
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne())
      if (it.get_curr_ne()->get_info() == i)
        return it.get_curr_ne();
    return nullptr;
  }

  /// The kinds of graph that are not directed, with int infos
  using Undirected_Kinds =
    ::testing::Types<Aleph::List_Graph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>,
                     Aleph::List_SGraph<Aleph::Graph_Snode<int>, Aleph::Graph_Sarc<int>>,
                     Aleph::Array_Graph<Aleph::Graph_Anode<int>, Aleph::Graph_Aarc<int>>>;

  /// The directed kinds, with int infos
  using Directed_Kinds =
    ::testing::Types<Aleph::List_Digraph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>,
                     Aleph::List_SDigraph<Aleph::Graph_Snode<int>, Aleph::Graph_Sarc<int>>,
                     Aleph::Array_Digraph<Aleph::Graph_Anode<int>, Aleph::Graph_Aarc<int>>>;

  /// The six kinds, with int infos
  using All_Kinds =
    ::testing::Types<Aleph::List_Graph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>,
                     Aleph::List_SGraph<Aleph::Graph_Snode<int>, Aleph::Graph_Sarc<int>>,
                     Aleph::Array_Graph<Aleph::Graph_Anode<int>, Aleph::Graph_Aarc<int>>,
                     Aleph::List_Digraph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>,
                     Aleph::List_SDigraph<Aleph::Graph_Snode<int>, Aleph::Graph_Sarc<int>>,
                     Aleph::Array_Digraph<Aleph::Graph_Anode<int>, Aleph::Graph_Aarc<int>>>;

  /// Runs f in a thread whose stack has kb KiB, and returns false if the
  /// thread could not run; without POSIX threads, it runs f in this thread
  template <class F>
  bool run_with_small_stack(F && f, size_t kb = 256)
  {
#if ALEPH_GRAPH_HARNESS_HAS_SMALL_STACK
    pthread_attr_t attr;
    if (pthread_attr_init(&attr) != 0)
      return false;
    pthread_attr_setstacksize(&attr, kb * 1024);
    using Fn = std::remove_reference_t<F>;
    auto start = [] (void * arg) -> void *
      {
        (*static_cast<Fn *>(arg))();
        return nullptr;
      };
    pthread_t thread;
    const int created = pthread_create(&thread, &attr, start, &f);
    pthread_attr_destroy(&attr);
    if (created != 0)
      return false;
    return pthread_join(thread, nullptr) == 0;
#else
    f();
    return true;
#endif
  }

  /** The failures of one property over a domain.

      A check counts the specs and the failures, keeps the first ones as
      text, and at the end expects no failure. A check marked as a known
      failure, during the work that fixes it, expects at least one, so that
      the mark goes as soon as the fix lands. A known failure that would end
      the program, as an assertion does, cannot run: its specs are left out
      with skip(), and counted. */
  class Check
  {
    std::string name;
    bool known_failure;
    size_t runs = 0;
    size_t failures = 0;
    size_t skipped = 0;
    std::vector<std::string> first;

  public:
    explicit Check(std::string what, bool known = false)
      : name(std::move(what)), known_failure(known) {}

    /// Counts a run of the property
    void run() noexcept { ++runs; }

    /// Counts a run left out because it would end the program
    void skip() noexcept { ++skipped; }

    /// Counts a failure on spec, with what went wrong
    void fail(const Spec & s, const std::string & what)
    {
      ++failures;
      if (first.size() < failures_shown())
        first.push_back(s.str() + ": " + what);
    }

    [[nodiscard]] size_t num_failures() const noexcept { return failures; }

    /// Expects what the mark says, and prints the first failures
    void expect() const
    {
      std::ostringstream s;
      for (const auto & f : first)
        s << "\n  " << f;
      if (known_failure)
        {
          EXPECT_GT(failures + skipped, 0U)
            << name << ": a known failure no longer fails; remove its mark";
          std::cout << "[  KNOWN   ] " << name << ": " << failures << " of " << runs
                    << " runs fail";
          if (skipped > 0)
            std::cout << ", " << skipped << " left out";
          std::cout << s.str() << std::endl;
        }
      else
        EXPECT_EQ(failures + skipped, 0U)
          << name << ": " << failures << " of " << runs << " runs fail, "
          << skipped << " left out" << s.str();
    }
  };
  /** Calls f(filt, name), as the loop over the filters of a test does,
      and again for each call k of the filter in that first call, with the
      filter throwing graph_filters::Thrown_By_Filter at its call k: what
      it throws has to come out of f, through the algorithm that f calls,
      if f calls the filter that many times again. For a filter of
      graph_filter_catalog.H whose call is not noexcept. */
  template <class F, class Filt>
  void with_each_throw(F & f, const Filt & filt, const char * name)
  {
    using graph_filters::Filter_Throws;
    Filter_Throws::throw_at = 0;
    Filter_Throws::calls = 0;
    f(filt, name);
    const long n = Filter_Throws::calls;
    for (long k = 1; k <= n; ++k)
      {
        Filter_Throws::calls = 0;
        Filter_Throws::throw_at = k;
        bool out = false;
        try
          {
            f(filt, name);
          }
        catch (const graph_filters::Thrown_By_Filter &)
          {
            out = true;
          }
        Filter_Throws::throw_at = 0;
        // Another call of f can call the filter fewer times, when the order
        // of a search depends on the addresses of a graph that f builds:
        // then the call k did not come, and nothing threw
        EXPECT_TRUE(out or Filter_Throws::calls < k)
          << name << ": what the filter threw at its call " << k << " of " << n
          << " did not come out";
      }
  }
} // namespace graph_harness

/** EXPECT_THROW(statement, type), for a statement that calls a filter:
    in a call of graph_harness::with_each_throw(), in which the filter
    throws at one of its calls, what the filter throws goes on, and type is
    taken if it comes first. */
#define EXPECT_THROW_OR_FILTER(statement, type)                     \
  do                                                                \
    if (::graph_filters::Filter_Throws::throw_at == 0)              \
      EXPECT_THROW(statement, type);                                \
    else                                                            \
      try                                                           \
        {                                                           \
          statement;                                                \
        }                                                           \
      catch (const type &)                                          \
        {                                                           \
        }                                                           \
  while (false)

#endif // ALEPH_TESTS_GRAPH_HARNESS_H
