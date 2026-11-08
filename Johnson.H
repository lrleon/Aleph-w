
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


/** @file Johnson.H
 *  @brief Johnson's algorithm for all-pairs shortest paths.
 *
 *  Implements Johnson's algorithm combining Bellman-Ford and Dijkstra
 *  for finding all-pairs shortest paths in sparse graphs, including
 *  graphs with negative edge weights. O(VE log V) complexity.
 *
 *  ## Algorithm Overview
 *
 *  Johnson's algorithm works in the following steps:
 *  1. Run Bellman-Ford from a virtual source, which reaches every node
 *     by an arc of weight 0 and is not added to the graph
 *     (Bellman_Ford::compute_nodes_weights()), to compute the potential
 *     h(v) of each node: min(0, the least distance from a node to v)
 *  2. Reweight all arcs: w'(u,v) = w(u,v) + h(u) - h(v), which is >= 0
 *  3. For each source, run Dijkstra on the reweighted arcs
 *  4. Adjust distances back: d(s,t) = d'(s,t) - h(s) + h(t)
 *
 *  With an integer distance type, w' and the sums of Dijkstra are kept in
 *  the unsigned type of the same width: as lowest() <= h <= 0, w' is at
 *  most max() + max() + 1, and d'(s,t) fits whenever d(s,t) fits. If a
 *  potential is below lowest(), the potentials, w' and the sums are kept
 *  in an integer of 128 bits instead, where they fit too. So every
 *  distance that fits in the type is given, and only one that does not
 *  makes a query throw.
 *
 *  ## Complexity
 *
 *  | Phase | Time |
 *  |-------|------|
 *  | Bellman-Ford | O(VE) |
 *  | V × Dijkstra | O(V E log V) |
 *  | **Total** | **O(V E log V)** |
 *
 *  For sparse graphs (E = O(V)), this is O(V² log V), better than
 *  Floyd-Warshall's O(V³).
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

#ifndef JOHNSON_H
#define JOHNSON_H

# include <ah-graph-concepts.H>

#include <Dijkstra.H>
#include <Bellman_Ford.H>
#include <tpl_graph.H>
#include <tpl_dynList.H>
#include <tpl_dynMapTree.H>
#include <ah-errors.H>
#include <algorithm>
#include <concepts>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace Aleph
{

namespace johnson_detail
{
/** @brief An integer of 128 bits, in two words of 64.

    Johnson keeps in it the potentials, the new weights and the sums of
    Dijkstra when a potential is below `lowest()`, which only a signed
    integer type of up to 64 bits has: they fit in it. Its operations are
    those of the unsigned integers modulo 2^128, and signed_less() compares
    two of them as integers in two's complement, as the potentials are.
*/
struct Wide
{
  std::uint64_t hi = 0; ///< The high word
  std::uint64_t lo = 0; ///< The low word

  /// The integer x, in two's complement
  template <std::integral T>
  [[nodiscard]] static constexpr Wide of(const T x) noexcept
  {
    if constexpr (std::is_signed_v<T>)
      return Wide{x < 0 ? ~std::uint64_t(0) : std::uint64_t(0),
                  static_cast<std::uint64_t>(static_cast<std::int64_t>(x))};
    else
      return Wide{0, static_cast<std::uint64_t>(x)};
  }

  /// a + b, modulo 2^128
  [[nodiscard]] friend constexpr Wide operator + (const Wide & a, const Wide & b) noexcept
  {
    const std::uint64_t lo = a.lo + b.lo;
    return Wide{a.hi + b.hi + (lo < a.lo ? 1 : 0), lo};
  }

  /// a - b, modulo 2^128
  [[nodiscard]] friend constexpr Wide operator - (const Wide & a, const Wide & b) noexcept
  {
    return Wide{a.hi - b.hi - (a.lo < b.lo ? 1 : 0), a.lo - b.lo};
  }

  /// Whether a and b are the same integer
  [[nodiscard]] friend constexpr bool operator == (const Wide & a, const Wide & b) noexcept
    = default;

  /// Whether a < b, as unsigned integers
  [[nodiscard]] friend constexpr bool operator < (const Wide & a, const Wide & b) noexcept
  {
    return a.hi < b.hi or (a.hi == b.hi and a.lo < b.lo);
  }

  /// Whether a < b, as integers in two's complement
  [[nodiscard]] static constexpr bool signed_less(const Wide & a, const Wide & b) noexcept
  {
    constexpr std::uint64_t sign = std::uint64_t(1) << 63;
    return Wide{a.hi ^ sign, a.lo} < Wide{b.hi ^ sign, b.lo};
  }
};
} // namespace johnson_detail

/** Johnson's algorithm for all-pairs shortest paths.

    This class implements Johnson's algorithm for computing shortest paths
    between all pairs of vertices in a weighted directed graph. It handles
    graphs with negative edge weights (but not negative cycles).

    The algorithm is efficient for sparse graphs, with complexity
    O(V E log V), compared to Floyd-Warshall's O(V³).

    The constructor reads the graph once. It copies it, which
    get_reweighted_graph() gives, and calls the filter on each of its arcs
    and the distance on each arc that the filter shows: always on the arcs
    of the graph, never on their copies. The graph is not changed: neither
    its nodes nor its arcs, nor their infos, control bits, counters or
    cookies. The queries are answered from a list of the arcs that the
    filter shows, with their new weights, on which a query runs Dijkstra
    from its source, unless that source is the one of the last query that
    ran it: the tree of the shortest paths from that one is kept.

    @tparam GT Graph type (typically List_Digraph).
    @tparam Distance Arc weight accessor class. Must provide:
            - `Distance_Type`: The numeric type for weights
            - `operator()(Arc*)`: Returns the weight of an arc
    @tparam Ait Arc iterator template with which Bellman-Ford traverses the
            arcs that the filter shows, on a digraph of the solver.
    @tparam NAit Node arc iterator template with which Bellman-Ford takes,
            on that digraph, the arcs out of a node.
    @tparam SA Arc filter: the arcs that it hides do not count.

    Usage example:
    @code
    List_Digraph<Node, Arc> g;
    // ... build graph with possibly negative edges ...

    try {
      Johnson<List_Digraph<Node, Arc>> johnson(g);

      // Query shortest path between any pair
      auto path = johnson.find_path(src, tgt);
      auto dist = johnson.get_distance(src, tgt);
    }
    catch (const std::domain_error& e) {
      // Graph has a negative cycle
    }
    @endcode

    @warning The graph must be a **digraph**. For undirected graphs,
             convert to digraph first (each edge becomes two arcs). On one
             that is not directed, each arc is taken in both directions,
             so that an arc of negative weight is a negative cycle.

    @see Bellman_Ford Dijkstra_Min_Paths Floyd_All_Shortest_Paths

    @ingroup Graphs
 */
template <AlephGraph GT,
          ArcDistance<GT> Distance = Dft_Dist<GT>,
          template <class, class> class Ait = Arc_Iterator,
          template <class, class> class NAit = Node_Arc_Iterator,
          ArcFilter<GT> SA = Dft_Show_Arc<GT>>
class Johnson
{
public:
  using Node = typename GT::Node;
  using Arc = typename GT::Arc;
  using Distance_Type = typename Distance::Distance_Type;

private:
  // An integer type, other than bool, keeps the new weights and their sums
  // in its unsigned type
  static constexpr bool Integer = std::is_integral_v<Distance_Type>
    and not std::is_same_v<std::remove_cv_t<Distance_Type>, bool>;

  // The type of the new weights w + h(u) - h(v) and of their sums: for an
  // integer type, the unsigned one of the same width, where they fit
  // whenever the distances fit; else the type of the distances
  using Weight =
    typename std::conditional_t<Integer,
                                std::make_unsigned<std::remove_cv_t<Distance_Type>>,
                                std::type_identity<Distance_Type>>::type;

  // A signed integer type of up to 64 bits may have a potential below
  // lowest(): then the potentials, the new weights and the sums of
  // Dijkstra are kept in an integer of 128 bits, where they fit
  static constexpr bool Signed_Integer = Integer and std::is_signed_v<Distance_Type>
    and sizeof(Distance_Type) <= sizeof(std::int64_t);

  using Wide = johnson_detail::Wide;

  static constexpr size_t None = std::numeric_limits<size_t>::max();

  // A way in which an arc that the filter shows is taken: from the node
  // number u to the node number v, with the weight w; copy is the copy of
  // the arc, or nullptr for the second way of an arc of a graph that is
  // not directed
  struct Shown_Arc
  {
    size_t u;
    size_t v;
    Distance_Type w;
    Arc * arc;
    Arc * copy;
  };

  // The digraph on which Bellman-Ford computes the potentials: a node for
  // each one of the graph, with its number, and an arc for each Shown_Arc,
  // with its weight
  using Potential_Graph = List_Digraph<Graph_Node<size_t>, Graph_Arc<Distance_Type>>;

  const GT* orig_graph = nullptr;
  GT reweighted;                      ///< Copy of the graph
  DynMapTree<Node*, size_t> number;   ///< Of each node of the graph, from 0
  std::vector<Node*> nodes;           ///< The node of each number
  std::vector<Node*> copies;          ///< Its copy in the reweighted graph
  std::vector<Distance_Type> h;       ///< Its potential, from Bellman-Ford

  // The arcs on which Dijkstra runs. Those out of the node number u are
  // the e from out_first[u] to out_first[u + 1] - 1: to the node number
  // out_target[e], of new weight out_weight[e], ways of the arc out_arc[e]
  // of the graph. A loop shortens no path: it is not there.
  std::vector<size_t> out_first;
  std::vector<size_t> out_target;
  std::vector<Weight> out_weight;
  std::vector<Arc*> out_arc;

  // Whether the solver has a graph, and the type of its potentials: that
  // of the distances, or, when one is below lowest(), 128 bits (wide_h). A
  // query checks it once, as it checked whether the solver had a graph.
  enum Mode : unsigned char
  {
    No_Graph = 0,
    Narrow_Potentials,
    Wide_Potentials
  };
  Mode mode = No_Graph;

  // What Dijkstra knows of a node
  enum : unsigned char
  {
    Unreached = 0, // no path from the source
    Labeled,       // a path from the source, maybe not a shortest one
    Settled,       // its distance from the source
    Too_Far,       // the sums of its paths leave the range of Weight
    Settled_Wide   // as Settled, on the wide weights
  };

  // The tree of the shortest paths from the last source from which a
  // query ran Dijkstra: for each node, what Dijkstra knows of it, its
  // distance on the new weights, and the node and the arc (an index of the
  // arcs of Dijkstra) before it
  std::vector<unsigned char> state;
  std::vector<Weight> reach;
  std::vector<size_t> parent;
  std::vector<size_t> via;

  // The heap of Dijkstra, of pairs (distance, node), with room for each
  // pair that it may get: one for the source and one for each arc. A node
  // may have several pairs in it: the first one out is that of its
  // distance, and the others are left aside.
  std::vector<std::pair<Weight, size_t>> heap;

  // With the mode Wide_Potentials, the potentials, the new weights, the
  // distances of Dijkstra on them and its heap are these, in an integer of
  // 128 bits, instead of out_weight, reach and heap, which are left empty;
  // h keeps the potentials that fit, and 1, which no potential is, for
  // those below lowest()
  std::vector<Wide> wide_h;
  std::vector<Wide> wide_weight;
  std::vector<Wide> wide_reach;
  std::vector<std::pair<Wide, size_t>> wide_heap;

  // The new weights, the distances of Dijkstra and its heap: the wide ones
  // if W
  template <bool W>
  auto & weights_of() noexcept
  {
    if constexpr (W)
      return wide_weight;
    else
      return out_weight;
  }

  template <bool W>
  auto & reach_of() noexcept
  {
    if constexpr (W)
      return wide_reach;
    else
      return reach;
  }

  template <bool W>
  auto & heap_of() noexcept
  {
    if constexpr (W)
      return wide_heap;
    else
      return heap;
  }

  // The number of the source of the tree that is kept, or None. A move of
  // the solver gives it to the new one and leaves the old one without it.
  struct Kept_Source
  {
    size_t node = None;

    Kept_Source() = default;

    Kept_Source(Kept_Source && other) noexcept
      : node(std::exchange(other.node, None))
    {
      // empty
    }

    Kept_Source & operator = (Kept_Source && other) noexcept
    {
      node = std::exchange(other.node, None);
      return *this;
    }
  } kept;

  // The number of a node of the graph
  size_t number_of(Node * p) const
  {
    auto * q = number.search(p);
    ah_domain_error_if(q == nullptr)
      << "Node not found in mapping (not from original graph?)";
    return q->second;
  }

  // w + hu - hv, which is >= 0 and, with an integer type, at most max() +
  // max() + 1, as lowest() <= h <= 0: exact in Weight, modulo its range
  static Weight reweight(const Distance_Type & w, const Distance_Type & hu,
                         const Distance_Type & hv)
  {
    if constexpr (Integer)
      return Weight(Weight(Weight(w) + Weight(hu)) - Weight(hv));
    else
      return w + hu - hv;
  }

  // a + b in sum, or false if it leaves the range of Weight, which only an
  // integer type has
  static bool add(const Weight & a, const Weight & b, Weight & sum)
  {
    if constexpr (Integer)
      if (b > Weight(std::numeric_limits<Weight>::max() - a))
        return false;

    sum = Weight(a + b);
    return true;
  }

  // As add(), for unsigned integers of 128 bits
  static bool add(const Wide & a, const Wide & b, Wide & sum)
  {
    sum = a + b;
    return not (sum < a);
  }

  // The new weight of an arc, in the info of its copy, if the info can take
  // a distance: max() if, with an integer type, it is beyond max()
  static void write_info(Arc * copy, const Weight & w)
  {
    if constexpr (std::is_assignable_v<typename GT::Arc_Type &, const Distance_Type &>)
      {
        if constexpr (Integer)
          {
            constexpr Distance_Type max = std::numeric_limits<Distance_Type>::max();
            const Distance_Type value = w > Weight(max) ? max : static_cast<Distance_Type>(w);
            copy->get_info() = value;
          }
        else
          copy->get_info() = w;
      }
  }

  // As write_info(), for a new weight of 128 bits
  static void write_info(Arc * copy, const Wide & w)
  {
    if constexpr (std::is_assignable_v<typename GT::Arc_Type &, const Distance_Type &>)
      {
        constexpr Distance_Type max = std::numeric_limits<Distance_Type>::max();
        const Distance_Type value = w.hi != 0 or w.lo > static_cast<std::uint64_t>(max)
          ? max : static_cast<Distance_Type>(w.lo);
        copy->get_info() = value;
      }
  }

  // The distance to a node that cannot be reached: infinity, or max() for
  // a type that has none, as an integer type, whose infinity() is 0
  static constexpr Distance_Type unreachable() noexcept
  {
    if constexpr (std::numeric_limits<Distance_Type>::has_infinity)
      return std::numeric_limits<Distance_Type>::infinity();
    else
      return std::numeric_limits<Distance_Type>::max();
  }

  // The distance d(s, t) = d'(s, t) - h(s) + h(t), from d'(s, t), that of
  // the new weights. With an integer type it is >= h(t) - h(s) >=
  // lowest(), and above max() if d'(s, t) - h(s) > max() - h(t), whose
  // terms are >= 0 and fit in Weight; else it is exact modulo the range of
  // Weight
  static Distance_Type unweighted(const Weight & d_prime, const Distance_Type & hs,
                                  const Distance_Type & ht)
  {
    if constexpr (Integer)
      {
        const Weight minus_hs = Weight(Weight(0) - Weight(hs));
        const Weight minus_ht = Weight(Weight(0) - Weight(ht));
        const Weight room =
          Weight(Weight(std::numeric_limits<Distance_Type>::max()) + minus_ht);

        ah_overflow_error_if(minus_hs > room or d_prime > Weight(room - minus_hs))
          << "Integer overflow: a distance is above max()";

        return static_cast<Distance_Type>(Weight(Weight(d_prime + minus_hs) - minus_ht));
      }
    else
      return d_prime - hs + ht;
  }

  // As unweighted(), on the wide weights: d(s, t) = d'(s, t) - h(s) +
  // h(t), exact in 128 bits, if it fits in the type
  static Distance_Type wide_unweighted(const Wide & d_prime, const Wide & hs, const Wide & ht)
  {
    const Wide d = d_prime - hs + ht;
    ah_overflow_error_if(Wide::signed_less(Wide::of(std::numeric_limits<Distance_Type>::max()), d))
      << "Integer overflow: a distance is above max()";
    ah_overflow_error_if(Wide::signed_less(d, Wide::of(std::numeric_limits<Distance_Type>::lowest())))
      << "Integer overflow: a distance is below lowest()";

    return static_cast<Distance_Type>(static_cast<std::int64_t>(d.lo));
  }

  // The distance from the node number s, the source of the tree that is
  // kept, to the node number t
  Distance_Type distance_to(size_t s, size_t t) const
  {
    if (state[t] == Unreached)
      return unreachable();

    if (state[t] != Settled) [[unlikely]]
      return distance_apart(s, t);

    return unweighted(reach[t], h[s], h[t]);
  }

  // As distance_to(), for a node too far, which throws, or settled on the
  // wide weights
  [[gnu::noinline]] Distance_Type distance_apart(size_t s, size_t t) const
  {
    if constexpr (Signed_Integer)
      if (state[t] == Settled_Wide)
        return wide_unweighted(wide_reach[t], wide_h[s], wide_h[t]);

    ah_overflow_error() << "Integer overflow: a distance is above max()";
  }

  // Gives the state Too_Far to the nodes that the nodes too far reach and
  // Dijkstra did not, whose distances, as theirs, are beyond max(); with
  // the heap of the wide weights if W
  template <bool W>
  void spread_too_far()
  {
    auto & stack = heap_of<W>();
    using Key = typename std::remove_reference_t<decltype(stack)>::value_type::first_type;
    // Key{} is 0; Clang 14 does not take Key(0) for an aggregate

    for (size_t v = 0; v < state.size(); ++v)
      if (state[v] == Too_Far)
        stack.emplace_back(Key{}, v);

    while (not stack.empty())
      {
        const size_t u = stack.back().second;
        stack.pop_back();
        for (size_t e = out_first[u], end = out_first[u + 1]; e < end; ++e)
          if (const size_t v = out_target[e]; state[v] == Unreached)
            {
              state[v] = Too_Far;
              stack.emplace_back(Key{}, v);
            }
      }
  }

  // Runs Dijkstra from the node number s on the new weights, the wide ones
  // if W, which leaves the nodes that it reaches as Settled, or
  // Settled_Wide. A sum that leaves the range of the weights, which only
  // an integer type has, is no path: a node that is reached only through
  // such sums is too far, as its distance is beyond max(), and so are the
  // nodes that only it reaches. It does not allocate.
  template <bool W>
  [[gnu::always_inline]] inline void paint(size_t s)
  {
    using Key = typename std::remove_reference_t<decltype(weights_of<W>())>::value_type;
    constexpr unsigned char Done = W ? Settled_Wide : Settled;
    const auto & weight = weights_of<W>();
    auto & dist = reach_of<W>();
    auto & pairs = heap_of<W>();

    const auto later = [] (const std::pair<Key, size_t> & a,
                           const std::pair<Key, size_t> & b)
      {
        return b.first < a.first;
      };

    kept.node = None; // until the tree is whole
    std::fill(state.begin(), state.end(), Unreached);
    pairs.clear();
    bool too_far = false;

    state[s] = Labeled;
    dist[s] = Key{};
    parent[s] = via[s] = None;
    pairs.emplace_back(Key{}, s);
    while (not pairs.empty())
      {
        std::pop_heap(pairs.begin(), pairs.end(), later);
        const size_t u = pairs.back().second;
        pairs.pop_back();
        if (state[u] == Done)
          continue; // a pair left aside

        state[u] = Done;
        const Key du = dist[u];
        for (size_t e = out_first[u], end = out_first[u + 1]; e < end; ++e)
          {
            const size_t v = out_target[e];
            if (state[v] == Done)
              continue;

            Key dv{};
            if (not add(du, weight[e], dv))
              {
                if (state[v] == Unreached)
                  {
                    state[v] = Too_Far;
                    too_far = true;
                  }
                continue;
              }

            if (state[v] == Labeled and not (dv < dist[v]))
              continue;

            state[v] = Labeled;
            dist[v] = dv;
            parent[v] = u;
            via[v] = e;
            pairs.emplace_back(dv, v);
            std::push_heap(pairs.begin(), pairs.end(), later);
          }
      }

    if (too_far)
      spread_too_far<W>();

    kept.node = s;
  }

  // As paint(), on the wide weights
  [[gnu::noinline]] void paint_wide(size_t s) { paint<true>(s); }

  // Runs Dijkstra from the node number s, unless the tree that is kept is
  // that of s
  void paint_from(size_t s)
  {
    if (kept.node == s)
      return;

    if constexpr (Signed_Integer)
      if (mode == Wide_Potentials) [[unlikely]]
        {
          paint_wide(s);
          return;
        }

    paint<false>(s);
  }

public:
  /** Construct a Johnson's all-pairs shortest paths executor.

      The constructor:
      1. Copies the input graph, and calls the filter on each arc and the
         distance on each arc that the filter shows, once
      2. Runs Bellman-Ford to compute node potentials
      3. Reweights all edges to be non-negative

      With an integer type, a potential below `lowest()` throws nothing:
      the potentials, the new weights and the sums of Dijkstra are kept in
      an integer of 128 bits, so that only the queries whose distances do
      not fit throw.

      @param[in] g The graph to operate on. It is not changed.
      @param[in] d Arc-weight accessor (default constructed).
      @param[in] __sa Arc filter (default constructed).
      @throw std::bad_alloc if there is not enough memory.
      @throw std::domain_error if a negative cycle is detected, or, with
      a floating type, a weight is -inf or NaN, as Bellman-Ford throws.
      @throw anything that the filter or the distance throws.
   */
  explicit Johnson(const GT& g, Distance d = Distance(), SA __sa = SA())
    : orig_graph(&g)
  {
    const size_t n = g.get_num_nodes();
    nodes.reserve(n);
    copies.reserve(n);

    Potential_Graph pg;
    std::vector<typename Potential_Graph::Node *> pnodes;
    pnodes.reserve(n);
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne())
      {
        Node * p = it.get_curr_ne();
        number.insert(p, nodes.size());
        pnodes.push_back(pg.insert_node(nodes.size()));
        copies.push_back(reweighted.insert_node(p->get_info()));
        nodes.push_back(p);
      }

    // Each arc is copied; those that the filter shows are taken from their
    // sources, and also from their targets if the graph is not directed
    const bool directed = g.is_digraph();
    std::vector<Shown_Arc> shown;
    for (typename GT::Arc_Iterator it(g); it.has_curr(); it.next_ne())
      {
        Arc * a = it.get_curr_ne();
        const size_t u = number.find(g.get_src_node(a));
        const size_t v = number.find(g.get_tgt_node(a));
        Arc * copy = reweighted.insert_arc(copies[u], copies[v], a->get_info());
        if (not __sa(a))
          continue;

        const Distance_Type w = d(a);
        shown.push_back(Shown_Arc{u, v, w, a, copy});
        if (not directed and u != v)
          shown.push_back(Shown_Arc{v, u, w, a, nullptr});
      }

    // The potentials; this throws with a negative cycle. With a signed
    // integer type, Bellman-Ford throws overflow_error when a sum goes
    // below lowest(), as when a potential is below it, or on a negative
    // cycle of large weights: the potentials are computed again in 128
    // bits, which also tells the negative cycle
    for (const Shown_Arc & x : shown)
      pg.insert_arc(pnodes[x.u], pnodes[x.v], x.w);
    bool wide = false;
    if constexpr (Signed_Integer)
      {
        try
          {
            narrow_potentials(pg, pnodes);
          }
        catch (const std::overflow_error &)
          {
            h.clear();
            wide_potentials(shown);
            wide = true;
          }
      }
    else
      narrow_potentials(pg, pnodes);

    // The arcs of Dijkstra, grouped by their sources in the order of the
    // arcs of the graph, with their new weights, which the copies of the
    // arcs take as infos
    out_first.assign(n + 1, 0);
    for (const Shown_Arc & x : shown)
      if (x.u != x.v)
        ++out_first[x.u + 1];
    for (size_t i = 0; i < n; ++i)
      out_first[i + 1] += out_first[i];

    if constexpr (Signed_Integer)
      if (wide) [[unlikely]]
        {
          wide_arcs(shown);
          return;
        }

    const size_t m = out_first[n];
    out_target.resize(m);
    out_weight.resize(m);
    out_arc.resize(m);
    std::vector<size_t> next(out_first.begin(), out_first.end() - 1);
    for (const Shown_Arc & x : shown)
      {
        const Weight w = reweight(x.w, h[x.u], h[x.v]);
        if (x.copy != nullptr)
          write_info(x.copy, w);
        if (x.u == x.v)
          continue;

        const size_t e = next[x.u]++;
        out_target[e] = x.v;
        out_weight[e] = w;
        out_arc[e] = x.arc;
      }

    state.assign(n, Unreached);
    reach.resize(n);
    parent.assign(n, None);
    via.assign(n, None);
    heap.reserve(m + 1);

    mode = Narrow_Potentials;
  }

private:
  // The potentials in h, from Bellman-Ford on pg, whose node number i is
  // pnodes[i]. In line, as when it was in the constructor: out of line, its
  // loop runs more instructions.
  [[gnu::always_inline]] inline void narrow_potentials(Potential_Graph & pg,
                         const std::vector<typename Potential_Graph::Node *> & pnodes)
  {
    Bellman_Ford<Potential_Graph, Dft_Dist<Potential_Graph>, Ait, NAit> bf(pg);
    const auto potentials = bf.compute_nodes_weights();
    h.reserve(pnodes.size());
    for (size_t i = 0; i < pnodes.size(); ++i)
      h.push_back(potentials.find(pnodes[i]));
  }

  // The potentials in wide_h, in 128 bits: a Bellman-Ford
  // from a virtual source that reaches every node by an arc of weight 0,
  // on the arcs shown. Without a negative cycle a potential is at least
  // (n - 1) lowest(), and the pass n changes none; with one, after n
  // passes a potential is at least n m lowest(): they fit in 128 bits.
  void wide_potentials(const std::vector<Shown_Arc> & shown)
  {
    const size_t n = nodes.size();
    wide_h.assign(n, Wide{});
    for (size_t pass = 1; ; ++pass)
      {
        bool changed = false;
        for (const Shown_Arc & x : shown)
          if (const Wide sum = wide_h[x.u] + Wide::of(x.w);
              Wide::signed_less(sum, wide_h[x.v]))
            {
              wide_h[x.v] = sum;
              changed = true;
            }

        if (not changed)
          break;

        ah_domain_error_if(pass >= n) << "negative cycles detected";
      }
  }

  // As the end of the constructor, for the wide potentials: the arcs of
  // Dijkstra, with their new weights in 128 bits
  void wide_arcs(const std::vector<Shown_Arc> & shown)
  {
    const size_t n = nodes.size();
    const size_t m = out_first[n];
    out_target.resize(m);
    wide_weight.resize(m);
    out_arc.resize(m);
    std::vector<size_t> next(out_first.begin(), out_first.end() - 1);
    for (const Shown_Arc & x : shown)
      {
        const Wide w = Wide::of(x.w) + wide_h[x.u] - wide_h[x.v];
        if (x.copy != nullptr)
          write_info(x.copy, w);
        if (x.u == x.v)
          continue;

        const size_t e = next[x.u]++;
        out_target[e] = x.v;
        wide_weight[e] = w;
        out_arc[e] = x.arc;
      }

    state.assign(n, Unreached);
    wide_reach.resize(n);
    parent.assign(n, None);
    via.assign(n, None);
    wide_heap.reserve(m + 1);

    h.resize(n);
    for (size_t i = 0; i < n; ++i)
      h[i] = Wide::signed_less(wide_h[i], Wide::of(std::numeric_limits<Distance_Type>::lowest()))
        ? Distance_Type(1) : static_cast<Distance_Type>(static_cast<std::int64_t>(wide_h[i].lo));

    mode = Wide_Potentials;
  }

public:

  /// Default constructor (uninitialized state)
  Johnson() = default;

  /// Johnson is not copyable: a copy would point to the nodes of the copy
  /// of the graph that this one keeps
  Johnson(const Johnson &) = delete;

  /// Johnson is not copyable: a copy would point to the nodes of the copy
  /// of the graph that this one keeps
  Johnson & operator = (const Johnson &) = delete;

  /// Move constructor: the tree that is kept moves too
  Johnson(Johnson &&) = default;

  /// Move assignment: the tree that is kept moves too
  Johnson & operator = (Johnson &&) = default;

  /** Check if the algorithm has been initialized.
   *  @return true if ready for queries, false otherwise.
   */
  [[nodiscard]] bool is_initialized() const noexcept { return mode != No_Graph; }

  /** Get the shortest distance from source to target.

      The tree of the shortest paths from the last source is kept, so that
      the queries from it do not run Dijkstra again.

      @param[in] src Source node (from original graph).
      @param[in] tgt Target node (from original graph).
      @return The shortest distance from src to tgt or, if tgt cannot be
      reached, `std::numeric_limits<Distance_Type>::infinity()`, or `max()`
      for a type without infinity, as an integer type.
      @throw std::domain_error if not initialized or nodes not found.
      @throw std::overflow_error if, with an integer type, the distance
      from src to tgt does not fit in it: it is above `max()`, or below
      `lowest()`, which only a graph with a potential below `lowest()`
      has. Only that one; the others are given.
   */
  Distance_Type get_distance(Node* src, Node* tgt)
    requires std::is_convertible_v<typename GT::Arc_Type, Distance_Type>
  {
    ah_domain_error_if(mode == No_Graph) << "Johnson not initialized";
    ah_domain_error_if(src == nullptr or tgt == nullptr) << "Source or target node is null";

    // Same node - distance is 0
    if (src == tgt)
      return Distance_Type(0);

    const size_t s = number_of(src);
    const size_t t = number_of(tgt);

    // Run Dijkstra from src, if its tree is not the one that is kept
    paint_from(s);

    return distance_to(s, t);
  }

  /** Find the shortest path from source to target.

      @param[in] src Source node (from original graph).
      @param[in] tgt Target node (from original graph).
      @return Path in the original graph from src to tgt, or an empty path if unreachable.
      @throw std::domain_error if not initialized or nodes not found.
      @throw std::overflow_error if, with an integer type, the distance
      from src to tgt does not fit in it, as get_distance() throws.
   */
  Path<GT> find_path(Node* src, Node* tgt)
    requires std::is_convertible_v<typename GT::Arc_Type, Distance_Type>
  {
    ah_domain_error_if(mode == No_Graph) << "Johnson not initialized";
    ah_domain_error_if(src == nullptr or tgt == nullptr) << "Source or target node is null";

    const size_t s = number_of(src);
    const size_t t = number_of(tgt);

    Path<GT> path(*orig_graph);
    if (s == t)
      {
        path.init(src);
        return path;
      }

    // Run Dijkstra from src, if its tree is not the one that is kept
    paint_from(s);

    if (state[t] == Unreached)
      return path;

    (void) distance_to(s, t); // overflow_error if it does not fit

    std::vector<Arc*> arcs; // of the tree, from tgt back to src
    for (size_t v = t; v != s; v = parent[v])
      arcs.push_back(out_arc[via[v]]);

    path.init(src);
    for (auto it = arcs.rbegin(); it != arcs.rend(); ++it)
      path.append(*it);

    return path;
  }

  /** Compute the shortest distance for all pairs.

      @return A map where key is (src, tgt) pair and value is shortest
      distance, or, for a target that cannot be reached, the value that
      get_distance() gives.
      @throw std::domain_error if not initialized.
      @throw std::overflow_error as get_distance(), if a distance does not
      fit in the type.
   */
  DynMapTree<std::pair<Node*, Node*>, Distance_Type>
  compute_all_pairs_distances()
    requires std::is_convertible_v<typename GT::Arc_Type, Distance_Type>
  {
    ah_domain_error_if(mode == No_Graph) << "Johnson not initialized";

    DynMapTree<std::pair<Node*, Node*>, Distance_Type> result;

    // For each node as source
    for (size_t s = 0; s < nodes.size(); ++s)
      {
        // Run Dijkstra from it, if its tree is not the one that is kept
        paint_from(s);

        // Get distance to all targets
        for (size_t t = 0; t < nodes.size(); ++t)
          result.insert(std::make_pair(nodes[s], nodes[t]), distance_to(s, t));
      }

    return result;
  }

  /** Get the node potential (h value) for a node.

      The potential h(v) is the distance to v from a virtual source that
      reaches every node by an arc of weight 0: min(0, the least distance
      from a node to v), as Bellman_Ford::compute_nodes_weights() computes
      it.

      @param[in] node Node from the original graph.
      @return The potential h(node).
      @throw std::domain_error if not initialized or node not found.
      @throw std::overflow_error if, with an integer type, the potential
      is below `lowest()`.
   */
  Distance_Type get_potential(Node* node) const
  {
    ah_domain_error_if(mode == No_Graph) << "Johnson not initialized";
    const Distance_Type p = h[number_of(node)];
    if constexpr (Signed_Integer)
      ah_overflow_error_if(p > Distance_Type(0)) // 1, for one below lowest()
        << "Integer overflow: a potential is below lowest()";
    return p;
  }

  /** Get access to the internal reweighted graph.

      It is a copy of the graph, with all its nodes and all its arcs, even
      those that the filter hides, which keep their infos. Each arc that
      the filter shows has as its info its new weight w(u,v) + h(u) - h(v),
      converted to the type of the info: `max()` if, with an integer type,
      it is above `max()`; an integer info keeps only the integer part of
      a floating new weight, which the queries keep whole. If the type of
      the info cannot take a distance, the infos are those of the graph.
      The queries do not read it, nor paint it.

      Useful for debugging or advanced operations.

      @return Reference to the internal graph copy.
   */
  GT& get_reweighted_graph() noexcept { return reweighted; }

  /** Get const access to the internal reweighted graph.

      @return Reference to the internal graph copy, as the non-const
      overload.
   */
  const GT& get_reweighted_graph() const noexcept { return reweighted; }

  /** Get the copy of a node in the reweighted graph.

      @param[in] orig Original node.
      @return Corresponding node in the reweighted graph.
      @throw std::domain_error if the node is not one of the graph.
   */
  Node* get_copy_node(Node* orig) const { return copies[number_of(orig)]; }
};

} // namespace Aleph

#endif // JOHNSON_H
