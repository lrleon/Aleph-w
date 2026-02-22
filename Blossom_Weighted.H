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


/** @file Blossom_Weighted.H
 *  @brief Maximum-weight matching in general undirected graphs.
 *
 *  This header provides an Edmonds-blossom based solver for
 *  maximum-weight matching in non-bipartite graphs.
 *
 *  ## Problem solved
 *
 *  Given an undirected graph @f$G=(V,E)@f$ where each edge @f$e \in E@f$ 
 *  has a weight @f$w(e)@f$, find a matching @f$M@f$ such that 
 *  @f$\sum_{e \in M} w(e)@f$ is maximized.
 *
 *  ## Highlights
 *
 *  - Works on all Aleph graph backends (`List_Graph`, `List_SGraph`,
 *    `Array_Graph`)
 *  - Supports arc filters
 *  - Optional maximum-cardinality mode (maximize cardinality first,
 *    then weight)
 *  - Validated with exhaustive oracle-backed tests across all three
 *    Aleph graph backends (`Tests/weighted_blossom_test.cc`)
 *
 *  ## Complexity
 *
 *  - Time: @f$O(V^3)@f$
 *  - Space: @f$O(V + E)@f$
 *
 *  ## Weight model
 *
 *  - The default weight accessor is `Dft_Dist<GT>`.
 *  - Current Aleph wrapper uses integral weights (`long long`).
 *
 *  @par Example
 *  @code
 *  using G = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
 *  G g;
 *  // ... build a weighted graph ...
 *  DynDlist<G::Arc*> matching;
 *  auto res = blossom_maximum_weight_matching(g, matching);
 *  
 *  std::cout << "Matching weight: " << res.total_weight << "\n";
 *  std::cout << "Matching size: " << res.cardinality << "\n";
 *  @endcode
 *
 *  @see Blossom.H For maximum-cardinality (unweighted) matching
 *  @ingroup Graphs
 */

#ifndef BLOSSOM_WEIGHTED_H
#define BLOSSOM_WEIGHTED_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <type_traits>
#include <utility>

#include <ah-errors.H>
#include <ahSort.H>
#include <cookie_guard.H>
#include <tpl_array.H>
#include <tpl_dynDlist.H>
#include <tpl_graph.H>
#include <tpl_graph_utils.H>

#include <blossom_weighted_mwmatching.H>

namespace Aleph
{
  /** @brief Result of weighted matching.
   *
   *  Stores the total weight and cardinality of the computed matching.
   *
   *  @tparam Weight_Type Numeric type used for the accumulated weight.
   *  @ingroup Graphs
   */
  template <typename Weight_Type = long long>
  struct Blossom_Weighted_Result
  {
    Weight_Type total_weight = Weight_Type{0}; ///< Sum of matched arc weights.
    size_t cardinality = 0; ///< Number of arcs in the matching.
  };


  namespace blossom_weighted_detail
  {
    /** @internal
     *  @brief Safe conversion of weights to long long for the internal solver.
     */
    template <typename T>
    long long to_ll_checked(T value)
    {
      static_assert(std::is_integral_v<T>, "Blossom_Weighted requires integral arc weights");

      if constexpr (std::is_signed_v<T>)
        {
          using Common = std::common_type_t<T, long long>;
          constexpr auto ll_max = std::numeric_limits<long long>::max();
          constexpr auto ll_min = std::numeric_limits<long long>::min();

          const auto v = static_cast<Common>(value);
          ah_overflow_error_if(v > static_cast<Common>(ll_max)
                               or v < static_cast<Common>(ll_min))
            << "Weight cannot be represented as long long";
        }
      else
        {
          using Common = std::common_type_t<T, long long>;
          constexpr auto ll_max = std::numeric_limits<long long>::max();

          const auto v = static_cast<Common>(value);
          ah_overflow_error_if(v > static_cast<Common>(ll_max))
            << "Weight cannot be represented as long long";
        }

      return static_cast<long long>(value);
    }
  } // namespace blossom_weighted_detail


  /** @brief Compute maximum-weight matching in a general undirected graph.
   *
   *  Finds a matching with the highest possible sum of edge weights. 
   *  If `max_cardinality` is true, it finds a matching that first maximizes 
   *  the number of edges, and among those, maximizes the total weight.
   *
   *  @tparam GT Aleph undirected graph type
   *             (`List_Graph`, `List_SGraph`, `Array_Graph`).
   *  @tparam Weight Weight accessor functor (must return an integral type).
   *  @tparam SA Arc filter.
   *
   *  @param[in] g Graph (must be undirected).
   *  @param[out] matching Output list where the selected matching arcs will be stored.
   *  @param[in] weight Weight accessor functor. Defaults to `Dft_Dist<GT>`.
   *  @param[in] sa Arc filter for graph traversal.
   *  @param[in] max_cardinality If true, optimize lexicographically:
   *             maximum cardinality first, then maximum weight.
   *
   *  @return A Blossom_Weighted_Result structure containing the total weight and cardinality.
   *
   *  @exception domain_error If `g` is a digraph.
   *  @exception overflow_error If weights exceed `long long` limits or the number 
   *                            of nodes exceeds the internal solver's limit.
   *
   *  @ingroup Graphs
   */
  template <class GT,
            class Weight = Dft_Dist<GT>,
            class SA = Dft_Show_Arc<GT>>
  Blossom_Weighted_Result<long long>
  compute_maximum_weight_general_matching(const GT & g,
                                          DynDlist<typename GT::Arc *> & matching,
                                          Weight weight = Weight(),
                                          SA sa = SA(),
                                          const bool max_cardinality = false)
  {
    using Node = typename GT::Node;
    using Arc = typename GT::Arc;
    using Raw_Weight = std::decay_t<decltype(weight(static_cast<Arc *>(nullptr)))>;
    using MwVertex = blossom_weighted_detail::mwmatching::VertexId;
    using MwEdge = blossom_weighted_detail::mwmatching::Edge<long long>;
    using MwPair = blossom_weighted_detail::mwmatching::VertexPair;

    static_assert(std::is_integral_v<Raw_Weight>,
                  "Blossom_Weighted requires integral arc weights");

    ah_domain_error_if(g.is_digraph())
      << "compute_maximum_weight_general_matching(): g is a digraph";

    matching.empty();

    if (g.get_num_nodes() == 0)
      return Blossom_Weighted_Result<long long>{0, 0};

    constexpr auto max_vertex = static_cast<size_t>(std::numeric_limits<MwVertex>::max());
    ah_overflow_error_if(g.get_num_nodes() > max_vertex)
      << "Graph has too many vertices for weighted blossom implementation";

    Cookie_Saver<GT> cookie_saver(g, true, false);

    size_t num_nodes = 0;
    for (Node_Iterator<GT> it(g); it.has_curr(); it.next_ne())
      {
        Node *p = it.get_curr();
        ++num_nodes;
        const auto encoded = num_nodes; // index + 1
        NODE_COOKIE(p) = reinterpret_cast<void *>(encoded);
      }

    struct Arc_Record
    {
      size_t u = 0;
      size_t v = 0;
      long long weight = 0;
      Arc *arc = nullptr;
    };

    Array<Arc_Record> records;
    records.reserve(g.get_num_arcs() + 1);

    for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
      {
        Arc *arc = it.get_curr_ne();
        Node *src = g.get_src_node(arc);
        Node *tgt = g.get_tgt_node(arc);

        const auto src_idx_plus_one = reinterpret_cast<uintptr_t>(NODE_COOKIE(src));
        const auto tgt_idx_plus_one = reinterpret_cast<uintptr_t>(NODE_COOKIE(tgt));

        ah_runtime_error_unless(src_idx_plus_one != 0 and tgt_idx_plus_one != 0)
          << "Weighted blossom internal node index mapping is invalid";

        auto u = src_idx_plus_one - 1;
        auto v = tgt_idx_plus_one - 1;
        if (u == v)
          continue; // Loops are irrelevant to matchings.

        if (u > v)
          std::swap(u, v);

        const long long w = blossom_weighted_detail::to_ll_checked(weight(arc));
        records.append(Arc_Record{u, v, w, arc});
      }

    if (records.is_empty())
      return Blossom_Weighted_Result<long long>{0, 0};

    struct Arc_Record_Less
    {
      bool operator()(const Arc_Record & a, const Arc_Record & b) const noexcept
      {
        if (a.u != b.u)
          return a.u < b.u;
        if (a.v != b.v)
          return a.v < b.v;
        return a.weight > b.weight;
      }
    };
    in_place_sort(records, Arc_Record_Less());

    Array<Arc_Record> best_edges;
    best_edges.reserve(records.size());
    for (size_t i = 0; i < records.size();)
      {
        const Arc_Record & best = records[i];
        best_edges.append(best);
        ++i;
        while (i < records.size()
               and records[i].u == best.u
               and records[i].v == best.v)
          ++i;
      }

    Array<MwEdge> edges;
    edges.reserve(best_edges.size());
    for (const Arc_Record & e: best_edges)
      edges.append(MwEdge(static_cast<MwVertex>(e.u),
                          static_cast<MwVertex>(e.v),
                          e.weight));

    Array<MwEdge> solver_edges;
    if (max_cardinality)
      solver_edges = blossom_weighted_detail::mwmatching::
          adjust_weights_for_maximum_cardinality_matching(edges);
    else
      solver_edges = edges;

    const Array<MwPair> matched_pairs = blossom_weighted_detail::mwmatching::
        maximum_weight_matching(solver_edges);

    long long total_weight = 0;
    size_t cardinality = 0;

    auto find_edge = [&best_edges](size_t u, size_t v) -> const Arc_Record *
      {
        size_t lo = 0;
        size_t hi = best_edges.size();

        while (lo < hi)
          {
            const size_t mid = lo + (hi - lo) / 2;
            if (const Arc_Record & cur = best_edges[mid]; cur.u < u or (cur.u == u and cur.v < v))
              lo = mid + 1;
            else
              hi = mid;
          }

        if (lo < best_edges.size())
          {
            const Arc_Record & cand = best_edges[lo];
            if (cand.u == u and cand.v == v)
              return &cand;
          }

        return nullptr;
      };

    for (const auto & [fst, snd]: matched_pairs)
      {
        auto u = static_cast<size_t>(fst);
        auto v = static_cast<size_t>(snd);
        if (u > v)
          std::swap(u, v);

        const Arc_Record *arc_info = find_edge(u, v);
        ah_runtime_error_unless(arc_info != nullptr)
          << "Weighted blossom returned an unknown matched pair";

        matching.append(arc_info->arc);
        const __int128 sum = static_cast<__int128>(total_weight)
                             + static_cast<__int128>(arc_info->weight);
        ah_overflow_error_if(sum > static_cast<__int128>(std::numeric_limits<long long>::max())
                             or sum < static_cast<__int128>(std::numeric_limits<long long>::min()))
          << "Matching total weight overflows long long";
        total_weight = static_cast<long long>(sum);
        ++cardinality;
      }

    return Blossom_Weighted_Result<long long>{total_weight, cardinality};
  }


  /** @brief Alias for compute_maximum_weight_general_matching().
   *  @ingroup Graphs
   */
  template <class GT,
            class Weight = Dft_Dist<GT>,
            class SA = Dft_Show_Arc<GT>>
  Blossom_Weighted_Result<long long>
  blossom_maximum_weight_matching(const GT & g,
                                  DynDlist<typename GT::Arc *> & matching,
                                  Weight weight = Weight(),
                                  SA sa = SA(),
                                  const bool max_cardinality = false)
  {
    return compute_maximum_weight_general_matching<GT, Weight, SA>(g, matching, std::move(weight), std::move(sa),
                                                                   max_cardinality);
  }


  /** @brief Functor wrapper for weighted blossom matching.
   *
   *  @ingroup Graphs
   */
  template <class GT,
            class Weight = Dft_Dist<GT>,
            class SA = Dft_Show_Arc<GT>>
  class Compute_Maximum_Weight_General_Matching
  {
    Weight weight_;
    SA sa_;
    bool max_cardinality_ = false;

  public:
    Compute_Maximum_Weight_General_Matching(Weight weight = Weight(),
                                            SA sa = SA(),
                                            const bool max_cardinality = false)
      : weight_(std::move(weight)),
        sa_(std::move(sa)),
        max_cardinality_(max_cardinality)
    {
      // empty
    }

    Blossom_Weighted_Result<long long>
    operator()(const GT & g, DynDlist<typename GT::Arc *> & matching)
    {
      return compute_maximum_weight_general_matching<GT, Weight, SA>(
                                                                     g, matching, weight_, sa_, max_cardinality_);
    }
  };
} // namespace Aleph

#endif // BLOSSOM_WEIGHTED_H
