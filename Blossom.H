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


/** @file Blossom.H
 *  @brief Edmonds' Blossom algorithm for maximum matching in general graphs.
 *
 *  This file implements the classic Edmonds-Blossom algorithm for finding a
 *  maximum cardinality matching in an undirected graph. 
 *
 *  Unlike bipartite matching, general graph matching must account for odd 
 *  cycles. The algorithm identifies such cycles (called "blossoms") and 
 *  contracts them into single super-nodes to find augmenting paths that 
 *  would otherwise be hidden.
 *
 *  ## Complexity
 *
 *  | Algorithm | Time | Space |
 *  |-----------|------|-------|
 *  | Edmonds-Blossom | O(V^3) | O(V + E) |
 *
 *  @par Example
 *  @code
 *  List_Graph<Node, Arc> g;
 *  // ... add nodes and arcs ...
 *
 *  DynDlist<List_Graph<Node, Arc>::Arc*> matching;
 *  size_t size = compute_maximum_cardinality_general_matching(g, matching);
 *  // matching now contains the arcs of an optimal matching
 *  @endcode
 *
 *  @example blossom_example.cc
 *
 *  @see tpl_bipartite.H For specialized bipartite matching algorithms.
 *  @see Hungarian.H For minimum/maximum weight assignment matching.
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath Leon
 */

# ifndef BLOSSOM_H
# define BLOSSOM_H

# include <cstdint>
# include <cstddef>
# include <cassert>
# include <functional>
# include <utility>

# include <cookie_guard.H>
# include <tpl_array.H>
# include <tpl_dynListQueue.H>
# include <tpl_dynMapTree.H>
# include <tpl_dynDlist.H>
# include <tpl_graph.H>
# include <ah-errors.H>

namespace Aleph
{
  namespace blossom_detail
  {
    /** @internal
     *  @brief Internal implementation of Edmonds' Blossom algorithm.
     *
     *  The Edmonds_Blossom_Matcher class encapsulates the state and logic 
     *  required to find a maximum cardinality matching in a general graph.
     *  It uses blossom contraction to handle odd cycles that prevent 
     *  standard bipartite matching techniques from working.
     *
     *  Key internal structures:
     *  - `match`: Stores the mate of each vertex.
     *  - `parent`: Stores the parent in the alternating BFS tree.
     *  - `base`: Stores the base vertex of the blossom containing a vertex.
     *  - `bfs_queue`: Used for the alternating path search.
     */
    template <class GT, class SA>
    class Edmonds_Blossom_Matcher
    {
    public:
      using Node = typename GT::Node;
      using Arc = typename GT::Arc;

    private:
      using Pair_Key = std::pair<size_t, size_t>;

      static constexpr long No_Vertex = -1;

      const GT & g;
      SA sa_;
      Cookie_Saver<GT> cookie_saver_;

      Array<Node *> nodes_;
      Array<Array<size_t>> adjacency_;
      DynMapTree<Pair_Key, Arc *> pair_to_arc_;

      Array<long> match_;
      Array<long> parent_;
      Array<long> base_;
      Array<char> in_queue_;
      Array<char> in_blossom;
      DynListQueue<size_t> bfs_queue_;

      static Pair_Key normalized_pair(size_t u, size_t v) noexcept
      {
        if (u > v)
          std::swap(u, v);
        return std::make_pair(u, v);
      }

      /** @brief Index nodes from 0 to n-1 and set up the cookie-based mapping. */
      void build_node_index()
      {
        nodes_.empty();

        nodes_.reserve(g.get_num_nodes());
        size_t idx = 0;
        for (Node_Iterator<GT> it(g); it.has_curr(); it.next_ne())
          {
            Node *p = it.get_curr();
            nodes_.append(p);
            NODE_COOKIE(p) = reinterpret_cast<void *>(idx + 1);
            ++idx;
          }
      }

      /** @brief Build a simplified adjacency list for faster traversal. */
      void build_adjacency()
      {
        const size_t n = nodes_.size();
        adjacency_.empty();
        adjacency_.reserve(n);
        for (size_t i = 0; i < n; ++i)
          adjacency_.append(Array<size_t>());

        pair_to_arc_.empty();

        for (Arc_Iterator<GT, SA> it(g, sa_); it.has_curr(); it.next_ne())
          {
            Arc *a = it.get_curr_ne();
            Node *src = g.get_src_node(a);
            Node *tgt = g.get_tgt_node(a);

            const auto src_idx_plus_one = reinterpret_cast<uintptr_t>(NODE_COOKIE(src));
            const auto tgt_idx_plus_one = reinterpret_cast<uintptr_t>(NODE_COOKIE(tgt));
            if (src_idx_plus_one == 0 or tgt_idx_plus_one == 0)
              continue;

            const size_t u = src_idx_plus_one - 1;
            const size_t v = tgt_idx_plus_one - 1;
            if (u == v) // loops never belong to a matching
              continue;

            if (const Pair_Key key = normalized_pair(u, v); pair_to_arc_.search(key) == nullptr)
              pair_to_arc_.insert(key, a);
          }

        for (typename DynMapTree<Pair_Key, Arc *>::Iterator it(pair_to_arc_);
             it.has_curr();
             it.next_ne())
          {
            const auto & pair_item = it.get_curr();
            const size_t u = pair_item.first.first;
            const size_t v = pair_item.first.second;
            adjacency_[u].append(v);
            adjacency_[v].append(u);
          }
      }

      /** @brief Find the lowest common ancestor of two nodes in the alternating tree.
       *
       *  Used to find the base of a new blossom.
       */
      [[nodiscard]] size_t lca(size_t a, size_t b) const
      {
        Array<char> visited;
        visited.reserve(nodes_.size());
        for (size_t i = 0; i < nodes_.size(); ++i)
          visited.append(0);

        while (true)
          {
            a = static_cast<size_t>(base_[a]);
            visited[a] = 1;
            if (match_[a] == No_Vertex)
              break;
            a = static_cast<size_t>(parent_[static_cast<size_t>(match_[a])]);
          }

        while (true)
          {
            b = static_cast<size_t>(base_[b]);
            if (visited[b])
              return b;
            if (match_[b] == No_Vertex)
              break;
            b = static_cast<size_t>(parent_[static_cast<size_t>(match_[b])]);
          }

        assert(false and "Blossom::lca fallback reached");
        return b;
      }

      /** @brief Mark the path from v to the blossom base and set up parents. */
      void mark_path(size_t v, const size_t blossom_base, size_t child)
      {
        while (static_cast<size_t>(base_[v]) != blossom_base)
          {
            const auto matched_v = static_cast<size_t>(match_[v]);
            in_blossom[static_cast<size_t>(base_[v])] = 1;
            in_blossom[static_cast<size_t>(base_[matched_v])] = 1;
            parent_[v] = static_cast<long>(child);
            child = matched_v;
            v = static_cast<size_t>(parent_[matched_v]);
          }
      }

      void clear_queue()
      {
        bfs_queue_.clear();
      }

      /** @brief Search for an augmenting path starting from root.
       *
       *  If a blossom is encountered, it is contracted.
       *
       *  @return The endpoint node ID of the augmenting path, or No_Vertex.
       */
      long find_augmenting_path(const size_t root)
      {
        const size_t n = nodes_.size();

        for (size_t i = 0; i < n; ++i)
          {
            parent_[i] = No_Vertex;
            in_queue_[i] = 0;
          }
        for (size_t i = 0; i < n; ++i)
          base_[i] = static_cast<long>(i);

        clear_queue();
        bfs_queue_.put(root);
        in_queue_[root] = 1;

        while (not bfs_queue_.is_empty())
          {
            const size_t v = bfs_queue_.get();
            for (const size_t u: adjacency_[v])
              {
                if (base_[v] == base_[u] or match_[v] == static_cast<long>(u))
                  continue;

                const bool creates_blossom = u == root or
                                             (match_[u] != No_Vertex and
                                              parent_[static_cast<size_t>(match_[u])] != No_Vertex);

                if (creates_blossom)
                  {
                    const size_t cur_base = lca(v, u);
                    for (size_t i = 0; i < n; ++i)
                      in_blossom[i] = 0;
                    mark_path(v, cur_base, u);
                    mark_path(u, cur_base, v);

                    for (size_t i = 0; i < n; ++i)
                      if (in_blossom[static_cast<size_t>(base_[i])])
                        {
                          base_[i] = static_cast<long>(cur_base);
                          if (not in_queue_[i])
                            {
                              bfs_queue_.put(i);
                              in_queue_[i] = 1;
                            }
                        }
                  }
                else if (parent_[u] == No_Vertex)
                  {
                    parent_[u] = static_cast<long>(v);
                    if (match_[u] == No_Vertex)
                      return static_cast<long>(u);

                    if (const auto m = static_cast<size_t>(match_[u]); not in_queue_[m])
                      {
                        bfs_queue_.put(m);
                        in_queue_[m] = 1;
                      }
                  }
              }
          }

        return No_Vertex;
      }

      /** @brief Augment the matching along the path ending at endpoint. */
      void augment_matching(const long endpoint)
      {
        long v = endpoint;
        while (v != No_Vertex)
          {
            const long pv = parent_[static_cast<size_t>(v)];
            if (pv == No_Vertex)
              break;

            const long next = match_[static_cast<size_t>(pv)];
            match_[static_cast<size_t>(v)] = pv;
            match_[static_cast<size_t>(pv)] = v;
            v = next;
          }
      }

    public:
      /** @brief Initialize the matcher with a graph and an optional filter. */
      explicit Edmonds_Blossom_Matcher(const GT & graph, SA __sa = SA())
        : g(graph), sa_(std::move(__sa)), cookie_saver_(g, true, false)
      {
        build_node_index();
        build_adjacency();

        const size_t n = nodes_.size();
        match_.empty();
        parent_.empty();
        base_.empty();
        in_queue_.empty();
        in_blossom.empty();
        match_.reserve(n);
        parent_.reserve(n);
        base_.reserve(n);
        in_queue_.reserve(n);
        in_blossom.reserve(n);
        for (size_t i = 0; i < n; ++i)
          {
            match_.append(No_Vertex);
            parent_.append(No_Vertex);
            base_.append(No_Vertex);
            in_queue_.append(0);
            in_blossom.append(0);
          }
      }

      /** @brief Execute the matching algorithm.
       *
       *  Iteratively finds augmenting paths until no more can be found.
       *
       *  @return Total cardinality of the matching.
       */
      size_t solve()
      {
        for (long & i : match_)
          i = No_Vertex;

        for (size_t i = 0; i < nodes_.size(); ++i)
          {
            if (match_[i] != No_Vertex)
              continue;

            if (const long endpoint = find_augmenting_path(i); endpoint != No_Vertex)
              augment_matching(endpoint);
          }

        size_t cardinality = 0;
        for (size_t i = 0; i < match_.size(); ++i)
          if (match_[i] != No_Vertex and i < static_cast<size_t>(match_[i]))
            ++cardinality;

        return cardinality;
      }

      /** @brief Returns the match vector (mate of each node index). */
      [[nodiscard]] const Array<long> &get_match_vector() const noexcept
      {
        return match_;
      }

      /** @brief Returns the arc connecting node indices u and v. */
      [[nodiscard]] Arc * get_pair_arc(size_t u, size_t v) const noexcept
      {
        const Pair_Key key = normalized_pair(u, v);
        const auto * item = pair_to_arc_.search(key);
        if (item == nullptr)
          return nullptr;
        return item->second;
      }
    };
  } // namespace blossom_detail


  /** @brief Computes a maximum cardinality matching in a general graph.
   *
   *  Applies Edmonds' Blossom algorithm to an undirected graph. The resulting 
   *  matching arcs are stored in the provided list.
   *
   *  @tparam GT Graph type (e.g., `List_Graph`).
   *  @tparam SA Arc filter type (defaults to show all arcs).
   *
   *  @param[in]  g        The undirected graph to process.
   *  @param[out] matching A list where the arcs of the maximum matching will be stored.
   *  @param[in]  sa       An optional arc filter instance.
   *
   *  @return The cardinality of the maximum matching found (@f$|M|@f$).
   *
   *  @throws ah_domain_error if `g` is a directed graph (digraph).
   *  @throws ah_bad_alloc if memory allocation for internal structures fails.
   *
   *  @note Self-loops and parallel arcs are handled correctly. Self-loops are 
   *        ignored as they cannot participate in a matching.
   *
   *  @par **Complexity**: Time @f$O(V^2 * E)@f$, Space @f$O(V + E)@f$.
   *
   *  @ingroup Graphs
   */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  size_t compute_maximum_cardinality_general_matching
  (const GT & g, DynDlist<typename GT::Arc *> & matching, SA sa = SA())
  {
    ah_domain_error_if(g.is_digraph())
      << "compute_maximum_cardinality_general_matching(): g is a digraph";

    matching.empty();

    blossom_detail::Edmonds_Blossom_Matcher<GT, SA> matcher(g, sa);
    const size_t cardinality = matcher.solve();

    const auto & mate = matcher.get_match_vector();
    for (size_t i = 0; i < mate.size(); ++i)
      if (mate[i] != -1 and i < static_cast<size_t>(mate[i]))
        {
          auto *arc = matcher.get_pair_arc(i, static_cast<size_t>(mate[i]));
          ah_runtime_error_unless(arc != nullptr)
            << "Blossom internal error: missing arc for matched pair";
          matching.append(arc);
        }

    return cardinality;
  }


  /** @brief Alias of compute_maximum_cardinality_general_matching().
   *
   *  This convenience function keeps the word "blossom" explicit in
   *  the call site.
   *
   *  @ingroup Graphs
   */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  size_t blossom_maximum_cardinality_matching
  (const GT & g, DynDlist<typename GT::Arc *> & matching, SA sa = SA())
  {
    return compute_maximum_cardinality_general_matching<GT, SA>(g, matching, sa);
  }


  /** @brief Functor wrapper for maximum cardinality general matching.
   *
   *  @tparam GT Graph type.
   *  @tparam SA Arc filter type.
   *
   *  @ingroup Graphs
   */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  class Compute_Maximum_Cardinality_General_Matching
  {
    SA sa_;

  public:
    explicit Compute_Maximum_Cardinality_General_Matching(SA __sa = SA())
      : sa_(std::move(__sa))
    {
      // empty
    }

    /** @brief Computes a maximum matching.
     *
     *  @param[in] g Undirected graph.
     *  @param[out] matching Resulting matching arcs.
     *  @return Cardinality of the matching.
     */
    size_t operator()(const GT & g, DynDlist<typename GT::Arc *> & matching)
    {
      return compute_maximum_cardinality_general_matching<GT, SA>(g, matching, sa_);
    }
  };
} // namespace Aleph

# endif // BLOSSOM_H
