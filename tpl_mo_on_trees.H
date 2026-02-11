
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


/** @file tpl_mo_on_trees.H
 *  @brief Mo's algorithm on trees: offline subtree and path queries.
 *
 *  Extension of Mo's algorithm to tree structures.  Given a rooted tree
 *  with N nodes and Q offline queries (either over subtrees or over
 *  paths), this class answers all queries in O((N+Q)√N) time by
 *  flattening the tree into an array via an Euler tour and then
 *  applying the classic Mo sweep.
 *
 *  ## Why Mo on Trees?
 *
 *  Many tree problems ask aggregate questions over subtrees or paths
 *  (e.g. "how many distinct species live in this subtree?" or "what is
 *  the most frequent label on the path from u to v?").  Naïve answers
 *  cost O(N) per query, giving O(NQ) total.  Mo on Trees reduces this
 *  to O((N+Q)√N) — a substantial improvement when Q is large.
 *
 *  ## How it works
 *
 *  1. **Euler-tour flattening** – a DFS from the root assigns each node
 *     entry and exit timestamps.  Subtrees become contiguous ranges in
 *     the entry-time order; paths become ranges in a double-occurrence
 *     tour combined with a toggle trick.
 *  2. **LCA via binary lifting** – needed for path queries to correctly
 *     identify the range and handle the lowest common ancestor.
 *  3. **Mo sweep** – queries are sorted into √N blocks and swept with a
 *     sliding window (snake-optimized), exactly as in `Gen_Mo_Algorithm`.
 *
 *  ## Complexity
 *  | Phase               | Time               | Space      |
 *  |---------------------|--------------------|------------|
 *  | Build (Euler + LCA) | O(N log N)         | O(N log N) |
 *  | Sort queries        | O(Q lg Q)          | O(Q)       |
 *  | Sweep (subtree)     | O((N+Q) √N)        | O(N+Q)     |
 *  | Sweep (path)        | O((N+Q) √N)        | O(N+Q)     |
 *
 *  ## Generic design
 *
 *  The class is templated on a graph type `GT` and a `Policy`.
 *  `GT` can be any Aleph-w graph: `List_Graph`, `List_SGraph`, or
 *  `Array_Graph` (and their digraph variants if the tree is stored as
 *  a directed graph).  The tree topology is read but **never modified**;
 *  all auxiliary data is stored externally — zero performance impact on
 *  existing graph and tree structures.
 *
 *  The `Policy` must satisfy the same `MoPolicy` concept used by
 *  `Gen_Mo_Algorithm` (see tpl_mo_algorithm.H).
 *
 *  @see tpl_mo_algorithm.H  Mo's algorithm on arrays.
 *  @see tpl_graph.H         List_Graph.
 *  @see tpl_sgraph.H        List_SGraph.
 *  @see tpl_agraph.H        Array_Graph.
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_MO_ON_TREES_H
# define TPL_MO_ON_TREES_H

# include <algorithm>
# include <cmath>
# include <concepts>
# include <vector>
# include <tpl_array.H>
# include <tpl_dynMapOhash.H>
# include <tpl_dynList.H>
# include <tpl_dynListStack.H>
# include <tpl_mo_algorithm.H>
# include <tpl_tree_node.H>
# include <ah-errors.H>

namespace Aleph
{
  /** @brief Concept constraining a graph type usable for Mo on Trees.
   *
   *  Requires node/arc types, an adjacency iterator constructible from
   *  a node pointer, a method to query the number of nodes, and a
   *  method to obtain the connected node of an arc.
   *
   *  @tparam GT  Graph type.
   *  @ingroup Graphs
   */
  template <typename GT>
  concept MoTreeGraph = requires(const GT & g,
                                 typename GT::Node * p,
                                 typename GT::Arc * a)
    {
      typename GT::Node;
      typename GT::Arc;
      typename GT::Node::Node_Type;
      { g.vsize() } -> std::convertible_to<size_t>;
      { g.get_connected_node(a, p) } -> std::same_as<typename GT::Node *>;
    };

  // ================================================================
  // Gen_Mo_On_Trees
  // ================================================================

  /** @brief Offline subtree and path queries on trees via Mo's algorithm.
   *
   *  Given a tree (represented as any Aleph-w graph type), a root node,
   *  and a `Policy` satisfying `MoPolicy`, this class preprocesses the
   *  tree in O(N log N) and then answers batches of subtree or path
   *  queries in O((N+Q)√N) time.
   *
   *  @tparam GT      Graph type (`List_Graph`, `List_SGraph`, or
   *                  `Array_Graph`).
   *  @tparam Policy  A type satisfying
   *                  `MoPolicy<Policy, GT::Node::Node_Type>`.
   *
   *  @par Example (subtree distinct count)
   *  @code
   *  // Build a tree
   *  List_Graph<Graph_Node<int>, Graph_Arc<Empty_Class>> g;
   *  auto * n0 = g.insert_node(10);
   *  auto * n1 = g.insert_node(20);
   *  auto * n2 = g.insert_node(10);
   *  g.insert_arc(n0, n1);
   *  g.insert_arc(n0, n2);
   *
   *  // Distinct count on subtree rooted at n0 → 2 (values 10, 20)
   *  using G = decltype(g);
   *  Gen_Mo_On_Trees<G, Distinct_Count_Policy<int>> mot(g, n0);
   *  auto ans = mot.subtree_solve({n0});
   *  // ans(0) == 2
   *  @endcode
   *
   *  @ingroup Graphs
   */
  template <class GT, class Policy>
    requires MoTreeGraph<GT> and MoPolicy<Policy, typename GT::Node::Node_Type>
  class Gen_Mo_On_Trees
  {
  public:
    using Node = typename GT::Node;
    using Arc = typename GT::Arc;
    using T = typename Node::Node_Type;
    using answer_type = typename Policy::answer_type;

  private:
    static constexpr size_t NONE = ~size_t(0);

    const GT & g_;
    Node * root_;
    mutable Policy pol_;
    size_t n_ = 0;

    // Node <-> ID mapping (external, zero impact on graph)
    Array<Node*> id_to_node_;
    MapOLhash<Node*, size_t> node_to_id_;
    Array<T> node_values_;

    // Single-occurrence Euler tour (for subtree queries, size n)
    Array<size_t> tin_;
    Array<size_t> tout_;
    Array<T> flat_sub_;

    // Double-occurrence Euler tour (for path queries, size 2n)
    Array<size_t> first_;
    Array<size_t> last_;
    Array<size_t> flat_node_;

    // Binary lifting for LCA
    size_t log_n_ = 0;
    Array<size_t> depth_;
    Array<size_t> parent_;
    Array<size_t> up_;  // flattened [log_n_][n_]: up_(k * n_ + v)

    // ── Build helpers ──────────────────────────────────────────────

    void build()
    {
      n_ = g_.vsize();
      if (n_ == 0)
        return;

      // 1. Assign contiguous IDs to nodes (pre-allocate OLhash for n_ entries)
      id_to_node_ = Array<Node*>::create(n_);
      node_values_ = Array<T>::create(n_);
      {
        MapOLhash<Node*, size_t> tmp(n_);
        node_to_id_.swap(tmp);
      }

      {
        size_t next_id = 0;
        g_.for_each_node([&](Node * p)
          {
            node_to_id_.insert(p, next_id);
            id_to_node_(next_id) = p;
            node_values_(next_id) = p->get_info();
            ++next_id;
          });
      }

      ah_domain_error_if(node_to_id_.search(root_) == nullptr)
        << "Gen_Mo_On_Trees: root node not found in graph";

      // 2. Pre-collect adjacency lists as node IDs (Array for O(1) access)
      auto adj = Array<Array<size_t>>::create(n_);
      g_.for_each_node([&](Node * p)
        {
          const size_t pid = node_to_id_.find(p);
          DynList<size_t> neighbors;
          for (auto it = typename GT::Node_Arc_Iterator(p);
               it.has_curr(); it.next_ne())
            {
              auto * a = it.get_curr();
              auto * neighbor = g_.get_connected_node(a, p);
              neighbors.append(node_to_id_.find(neighbor));
            }
          adj(pid) = Array<size_t>(neighbors);
        });

      // 3. Iterative DFS to build Euler tours + parent/depth
      tin_       = Array<size_t>::create(n_);
      tout_      = Array<size_t>::create(n_);
      flat_sub_  = Array<T>::create(n_);
      first_     = Array<size_t>::create(n_);
      last_      = Array<size_t>::create(n_);
      flat_node_ = Array<size_t>::create(2 * n_);
      depth_     = Array<size_t>::create(n_);
      parent_    = Array<size_t>::create(n_);
      for (size_t i = 0; i < n_; ++i)
        parent_(i) = NONE;

      auto visited = Array<bool>::create(n_);
      for (size_t i = 0; i < n_; ++i)
        visited(i) = false;

      size_t sub_timer  = 0;
      size_t path_timer = 0;

      struct Frame { size_t id; size_t child_idx; };
      DynListStack<Frame> stk;

      const size_t root_id = node_to_id_.find(root_);
      visited(root_id) = true;
      depth_(root_id)  = 0;
      parent_(root_id) = NONE;

      tin_(root_id) = sub_timer;
      flat_sub_(sub_timer) = node_values_(root_id);
      ++sub_timer;

      first_(root_id) = path_timer;
      flat_node_(path_timer) = root_id;
      ++path_timer;

      stk.push({root_id, 0});

      while (not stk.is_empty())
        {
          auto & fr = stk.top();
          bool pushed = false;

          while (fr.child_idx < adj(fr.id).size())
            {
              const size_t nid = adj(fr.id)(fr.child_idx++);
              if (visited(nid))
                continue;

              visited(nid) = true;
              parent_(nid) = fr.id;
              depth_(nid)  = depth_(fr.id) + 1;

              tin_(nid) = sub_timer;
              flat_sub_(sub_timer) = node_values_(nid);
              ++sub_timer;

              first_(nid) = path_timer;
              flat_node_(path_timer) = nid;
              ++path_timer;

              stk.push({nid, 0});
              pushed = true;
              break;
            }

          if (not pushed)
            {
              tout_(fr.id) = sub_timer - 1;

              last_(fr.id) = path_timer;
              flat_node_(path_timer) = fr.id;
              ++path_timer;

              (void) stk.pop();
            }
        }

      ah_domain_error_if(sub_timer != n_)
        << "Gen_Mo_On_Trees: graph is not connected (not a tree)";

      // 4. Binary lifting table for LCA
      log_n_ = 1;
      while ((static_cast<size_t>(1) << log_n_) < n_)
        ++log_n_;
      ++log_n_;

      up_ = Array<size_t>::create(log_n_ * n_);
      for (size_t i = 0; i < log_n_ * n_; ++i)
        up_(i) = NONE;

      for (size_t v = 0; v < n_; ++v)
        up_(0 * n_ + v) = parent_(v);

      for (size_t k = 1; k < log_n_; ++k)
        for (size_t v = 0; v < n_; ++v)
          up_(k * n_ + v) = (up_((k-1) * n_ + v) == NONE)
            ? NONE
            : up_((k-1) * n_ + up_((k-1) * n_ + v));
    }

    // LCA via binary lifting – O(log N)
    size_t lca(size_t u, size_t v) const
    {
      if (depth_(u) < depth_(v))
        std::swap(u, v);

      size_t diff = depth_(u) - depth_(v);
      for (size_t k = 0; k < log_n_; ++k)
        if ((diff >> k) & 1)
          u = up_(k * n_ + u);

      if (u == v)
        return u;

      for (int k = static_cast<int>(log_n_) - 1; k >= 0; --k)
        if (up_(k * n_ + u) != up_(k * n_ + v))
          {
            u = up_(k * n_ + u);
            v = up_(k * n_ + v);
          }

      return up_(0 * n_ + u);
    }

    // Standard Mo sweep (used for subtree queries)
    Array<answer_type> mo_sweep(const Array<T> & data,
                                Array<Mo_Query> queries,
                                size_t q, size_t n) const
    {
      const size_t block = std::max<size_t>(
        1, static_cast<size_t>(std::sqrt(static_cast<double>(n))));

      std::sort(&queries(0), &queries(0) + q,
        [block](const Mo_Query & a, const Mo_Query & b)
          {
            const size_t ba = a.l / block;
            const size_t bb = b.l / block;
            if (ba != bb)
              return ba < bb;
            return (ba & 1) ? (a.r > b.r) : (a.r < b.r);
          });

      pol_.init(data, n);

      auto answers = Array<answer_type>::create(q);

      size_t cur_l = queries(0).l;
      size_t cur_r = queries(0).l;
      pol_.add(data, cur_l);

      while (cur_r < queries(0).r)
        pol_.add(data, ++cur_r);

      answers(queries(0).id) = pol_.answer();

      for (size_t i = 1; i < q; ++i)
        {
          const size_t ql = queries(i).l;
          const size_t qr = queries(i).r;

          while (cur_r < qr)
            pol_.add(data, ++cur_r);

          while (cur_l > ql)
            pol_.add(data, --cur_l);

          while (cur_r > qr)
            pol_.remove(data, cur_r--);

          while (cur_l < ql)
            pol_.remove(data, cur_l++);

          answers(queries(i).id) = pol_.answer();
        }

      return answers;
    }

  public:

    /** @brief Construct from a graph and a root node.
     *
     *  Performs an O(N log N) preprocessing step (Euler tour + binary
     *  lifting for LCA).  The graph is **not modified** in any way.
     *
     *  @param g     The tree (graph with N nodes and N-1 edges).
     *  @param root  Root node for the DFS-based Euler tour.
     *  @param p     Policy object (default-constructed if omitted).
     *  @throw std::domain_error if root is not in the graph or the
     *         graph is not connected.
     */
    Gen_Mo_On_Trees(const GT & g, Node * root, Policy p = Policy())
      : g_(g), root_(root), pol_(std::move(p))
    {
      build();
    }

    /** @brief Number of nodes in the tree. */
    [[nodiscard]] size_t size() const noexcept { return n_; }

    /** @brief True if the tree is empty. */
    [[nodiscard]] bool is_empty() const noexcept { return n_ == 0; }

    // ── Subtree queries ──────────────────────────────────────────

    /** @brief Solve subtree queries.
     *
     *  For each node in @p query_roots, answers the policy query over
     *  the entire subtree rooted at that node.
     *
     *  Uses the single-occurrence Euler tour: the subtree of u is the
     *  contiguous range [tin(u), tout(u)] in DFS entry-time order.
     *
     *  @param query_roots  Array of root nodes for the subtree queries.
     *  @return Array of answers in the same order as @p query_roots.
     *  @throw std::domain_error if any node is not in the tree.
     */
    Array<answer_type> subtree_solve(
        const Array<Node*> & query_roots) const
    {
      const size_t q = query_roots.size();
      if (q == 0)
        return Array<answer_type>();

      auto queries = Array<Mo_Query>::create(q);
      for (size_t i = 0; i < q; ++i)
        {
          auto * p = node_to_id_.search(query_roots(i));
          ah_domain_error_if(p == nullptr)
            << "subtree_solve: query " << i << " node not in tree";
          const size_t id = p->second;
          queries(i) = {tin_(id), tout_(id), i};
        }

      return mo_sweep(flat_sub_, std::move(queries), q, n_);
    }

    /** @brief Solve subtree queries (initializer-list overload).
     *  @param il  initializer list of root nodes.
     */
    Array<answer_type> subtree_solve(
        std::initializer_list<Node*> il) const
    {
      const Array<Node*> roots(il);
      return subtree_solve(roots);
    }

    // ── Path queries ─────────────────────────────────────────────

    /** @brief Solve path queries.
     *
     *  For each pair (u, v), answers the policy query over all nodes on
     *  the unique path from u to v (both endpoints included).
     *
     *  Uses the double-occurrence Euler tour with a toggle trick:
     *  each node appears twice (entry + exit).  A node is "active"
     *  when it has been toggled an odd number of times within the
     *  current window.  The LCA of each query is handled separately.
     *
     *  @param query_pairs  Array of (u, v) node-pointer pairs.
     *  @return Array of answers in the same order as @p query_pairs.
     *  @throw std::domain_error if any node is not in the tree.
     */
    Array<answer_type> path_solve(
        const Array<std::pair<Node*, Node*>> & query_pairs) const
    {
      const size_t q = query_pairs.size();
      if (q == 0)
        return Array<answer_type>();

      struct Path_Query
      {
        size_t l, r, id, lca_id;
      };

      auto pqueries = Array<Path_Query>::create(q);
      for (size_t i = 0; i < q; ++i)
        {
          auto * pu = node_to_id_.search(query_pairs(i).first);
          auto * pv = node_to_id_.search(query_pairs(i).second);
          ah_domain_error_if(pu == nullptr or pv == nullptr)
            << "path_solve: query " << i << " node not in tree";

          size_t u = pu->second;
          size_t v = pv->second;

          if (first_(u) > first_(v))
            std::swap(u, v);

          const size_t ancestor = lca(u, v);

          if (ancestor == u)
            pqueries(i) = {first_(u), first_(v), i, NONE};
          else
            pqueries(i) = {last_(u), first_(v), i, ancestor};
        }

      // Snake sort
      const size_t tour_sz = 2 * n_;
      const size_t block = std::max<size_t>(
        1, static_cast<size_t>(
          std::sqrt(static_cast<double>(tour_sz))));

      std::sort(&pqueries(0), &pqueries(0) + q,
        [block](const Path_Query & a, const Path_Query & b)
          {
            const size_t ba = a.l / block;
            const size_t bb = b.l / block;
            if (ba != bb)
              return ba < bb;
            return (ba & 1) ? (a.r > b.r) : (a.r < b.r);
          });

      pol_.init(node_values_, n_);

      auto answers = Array<answer_type>::create(q);
      auto active = Array<bool>::create(n_);
      for (size_t i = 0; i < n_; ++i)
        active(i) = false;

      // Toggle: flip node activity and call policy add/remove
      auto toggle = [&](size_t pos)
        {
          const size_t nid = flat_node_(pos);
          if (active(nid))
            {
              pol_.remove(node_values_, nid);
              active(nid) = false;
            }
          else
            {
              pol_.add(node_values_, nid);
              active(nid) = true;
            }
        };

      // Initialize window to first query
      size_t cur_l = pqueries(0).l;
      size_t cur_r = pqueries(0).l;
      toggle(cur_l);

      while (cur_r < pqueries(0).r)
        toggle(++cur_r);

      if (pqueries(0).lca_id != NONE)
        {
          pol_.add(node_values_, pqueries(0).lca_id);
          answers(pqueries(0).id) = pol_.answer();
          pol_.remove(node_values_, pqueries(0).lca_id);
        }
      else
        answers(pqueries(0).id) = pol_.answer();

      // Sweep remaining queries
      for (size_t i = 1; i < q; ++i)
        {
          const size_t ql = pqueries(i).l;
          const size_t qr = pqueries(i).r;

          while (cur_r < qr) toggle(++cur_r);
          while (cur_l > ql) toggle(--cur_l);
          while (cur_r > qr) toggle(cur_r--);
          while (cur_l < ql) toggle(cur_l++);

          if (pqueries(i).lca_id != NONE)
            {
              pol_.add(node_values_, pqueries(i).lca_id);
              answers(pqueries(i).id) = pol_.answer();
              pol_.remove(node_values_, pqueries(i).lca_id);
            }
          else
            answers(pqueries(i).id) = pol_.answer();
        }

      return answers;
    }

    /** @brief Solve path queries (initializer-list overload).
     *  @param il  initializer list of (u, v) pairs.
     */
    Array<answer_type> path_solve(
        std::initializer_list<std::pair<Node*, Node*>> il) const
    {
      const Array<std::pair<Node*, Node*>> pairs(il);
      return path_solve(pairs);
    }
  };

  // ================================================================
  // Gen_Mo_On_Tree_Node  —  Mo on N-ary trees (Tree_Node<T>)
  // ================================================================

  /** @brief Offline subtree and path queries on N-ary trees (Tree_Node).
   *
   *  Same algorithm as Gen_Mo_On_Trees but works directly with the
   *  first-child / next-sibling representation of `Tree_Node<T>`.
   *  The tree is traversed via `get_left_child()` / `get_right_sibling()`
   *  instead of graph adjacency iterators.
   *
   *  @tparam T       Value type stored in each Tree_Node.
   *  @tparam Policy  A type satisfying `MoPolicy<Policy, T>`.
   *
   *  @par Example
   *  @code
   *  auto * r = new Tree_Node<int>(10);
   *  auto * a = new Tree_Node<int>(20);
   *  auto * b = new Tree_Node<int>(10);
   *  r->insert_rightmost_child(a);
   *  r->insert_rightmost_child(b);
   *
   *  Gen_Mo_On_Tree_Node<int, Distinct_Count_Policy<int>> mot(r);
   *  auto ans = mot.subtree_solve({r});   // ans(0) == 2
   *  @endcode
   *
   *  @ingroup Trees
   */
  template <typename T, class Policy>
    requires MoPolicy<Policy, T>
  class Gen_Mo_On_Tree_Node
  {
  public:
    using Node = Tree_Node<T>;
    using answer_type = typename Policy::answer_type;

  private:
    static constexpr size_t NONE = ~size_t(0);

    Node * root_;
    mutable Policy pol_;
    size_t n_ = 0;

    // Node <-> ID mapping
    Array<Node*> id_to_node_;
    MapOLhash<Node*, size_t> node_to_id_;
    Array<T> node_values_;

    // Single-occurrence Euler tour (subtree queries, size n)
    Array<size_t> tin_;
    Array<size_t> tout_;
    Array<T> flat_sub_;

    // Double-occurrence Euler tour (path queries, size 2n)
    Array<size_t> first_;
    Array<size_t> last_;
    Array<size_t> flat_node_;

    // Binary lifting for LCA
    size_t log_n_ = 0;
    Array<size_t> depth_;
    Array<size_t> parent_;
    Array<size_t> up_;  // flattened [log_n_][n_]: up_(k * n_ + v)

    // ── Build ──────────────────────────────────────────────────────

    void build()
    {
      // 1. Count nodes
      n_ = 0;
      root_->traverse([&](const Node *) { ++n_; return true; });

      if (n_ == 0)
        return;

      // 2. Assign IDs (pre-allocate OLhash for n_ entries)
      id_to_node_  = Array<Node*>::create(n_);
      node_values_ = Array<T>::create(n_);
      {
        MapOLhash<Node*, size_t> tmp(n_);
        node_to_id_.swap(tmp);
      }

      // Pre-collect adjacency as Array<Array<size_t>> for O(1) indexed access
      auto adj = Array<Array<size_t>>::create(n_);
      {
        size_t next_id = 0;
        root_->traverse([&](const Node * p)
          {
            auto * mp = const_cast<Node*>(p);
            node_to_id_.insert(mp, next_id);
            id_to_node_(next_id) = mp;
            node_values_(next_id) = mp->get_key();
            ++next_id;
            return true;
          });

        // Build adjacency: parent↔child edges (undirected)
        root_->traverse([&](const Node * p)
          {
            auto * mp = const_cast<Node*>(p);
            const size_t pid = node_to_id_.find(mp);
            DynList<size_t> neighbors;
            // Add children
            for (auto * c = mp->get_left_child(); c != nullptr;
                 c = c->get_right_sibling())
              neighbors.append(node_to_id_.find(c));
            // Add parent (if not root)
            Node * par = mp->get_parent();
            if (par != nullptr)
              neighbors.append(node_to_id_.find(par));
            adj(pid) = Array<size_t>(neighbors);
            return true;
          });
      }

      // 3. Iterative DFS for Euler tours + parent/depth
      tin_       = Array<size_t>::create(n_);
      tout_      = Array<size_t>::create(n_);
      flat_sub_  = Array<T>::create(n_);
      first_     = Array<size_t>::create(n_);
      last_      = Array<size_t>::create(n_);
      flat_node_ = Array<size_t>::create(2 * n_);
      depth_     = Array<size_t>::create(n_);
      parent_    = Array<size_t>::create(n_);
      for (size_t i = 0; i < n_; ++i)
        parent_(i) = NONE;

      auto visited = Array<bool>::create(n_);
      for (size_t i = 0; i < n_; ++i)
        visited(i) = false;

      size_t sub_timer  = 0;
      size_t path_timer = 0;

      struct Frame { size_t id; size_t child_idx; };
      DynListStack<Frame> stk;

      const size_t root_id = node_to_id_.find(root_);
      visited(root_id) = true;
      depth_(root_id)  = 0;
      parent_(root_id) = NONE;

      tin_(root_id) = sub_timer;
      flat_sub_(sub_timer) = node_values_(root_id);
      ++sub_timer;

      first_(root_id) = path_timer;
      flat_node_(path_timer) = root_id;
      ++path_timer;

      stk.push({root_id, 0});

      while (not stk.is_empty())
        {
          auto & fr = stk.top();
          bool pushed = false;

          while (fr.child_idx < adj(fr.id).size())
            {
              const size_t nid = adj(fr.id)(fr.child_idx++);
              if (visited(nid))
                continue;

              visited(nid) = true;
              parent_(nid) = fr.id;
              depth_(nid)  = depth_(fr.id) + 1;

              tin_(nid) = sub_timer;
              flat_sub_(sub_timer) = node_values_(nid);
              ++sub_timer;

              first_(nid) = path_timer;
              flat_node_(path_timer) = nid;
              ++path_timer;

              stk.push({nid, 0});
              pushed = true;
              break;
            }

          if (not pushed)
            {
              tout_(fr.id) = sub_timer - 1;

              last_(fr.id) = path_timer;
              flat_node_(path_timer) = fr.id;
              ++path_timer;

              (void) stk.pop();
            }
        }

      ah_domain_error_if(sub_timer != n_)
        << "Gen_Mo_On_Tree_Node: tree traversal inconsistency";

      // 4. Binary lifting table for LCA
      log_n_ = 1;
      while ((size_t(1) << log_n_) < n_)
        ++log_n_;
      ++log_n_;

      up_ = Array<size_t>::create(log_n_ * n_);
      for (size_t i = 0; i < log_n_ * n_; ++i)
        up_(i) = NONE;

      for (size_t v = 0; v < n_; ++v)
        up_(0 * n_ + v) = parent_(v);

      for (size_t k = 1; k < log_n_; ++k)
        for (size_t v = 0; v < n_; ++v)
          up_(k * n_ + v) = (up_((k-1) * n_ + v) == NONE)
            ? NONE
            : up_((k-1) * n_ + up_((k-1) * n_ + v));
    }

    // LCA via binary lifting – O(log N)
    size_t lca(size_t u, size_t v) const
    {
      if (depth_(u) < depth_(v))
        std::swap(u, v);

      size_t diff = depth_(u) - depth_(v);
      for (size_t k = 0; k < log_n_; ++k)
        if ((diff >> k) & 1)
          u = up_(k * n_ + u);

      if (u == v)
        return u;

      for (int k = static_cast<int>(log_n_) - 1; k >= 0; --k)
        if (up_(k * n_ + u) != up_(k * n_ + v))
          {
            u = up_(k * n_ + u);
            v = up_(k * n_ + v);
          }

      return up_(0 * n_ + u);
    }

    // Standard Mo sweep
    Array<answer_type> mo_sweep(const Array<T> & data,
                                Array<Mo_Query> queries,
                                size_t q, size_t nn) const
    {
      const size_t block = std::max<size_t>(
        1, static_cast<size_t>(std::sqrt(static_cast<double>(nn))));

      std::sort(&queries(0), &queries(0) + q,
        [block](const Mo_Query & a, const Mo_Query & b)
          {
            const size_t ba = a.l / block;
            const size_t bb = b.l / block;
            if (ba != bb)
              return ba < bb;
            return (ba & 1) ? (a.r > b.r) : (a.r < b.r);
          });

      pol_.init(data, nn);

      auto answers = Array<answer_type>::create(q);

      size_t cur_l = queries(0).l;
      size_t cur_r = queries(0).l;
      pol_.add(data, cur_l);

      while (cur_r < queries(0).r)
        pol_.add(data, ++cur_r);

      answers(queries(0).id) = pol_.answer();

      for (size_t i = 1; i < q; ++i)
        {
          const size_t ql = queries(i).l;
          const size_t qr = queries(i).r;

          while (cur_r < qr) pol_.add(data, ++cur_r);
          while (cur_l > ql) pol_.add(data, --cur_l);
          while (cur_r > qr) pol_.remove(data, cur_r--);
          while (cur_l < ql) pol_.remove(data, cur_l++);

          answers(queries(i).id) = pol_.answer();
        }

      return answers;
    }

  public:

    /** @brief Construct from a Tree_Node root.
     *
     *  Performs an O(N log N) preprocessing step.
     *  The tree is **not modified** in any way.
     *
     *  @param root  Root of the N-ary tree.
     *  @param p     Policy object (default-constructed if omitted).
     *  @throw std::domain_error if the tree is inconsistent.
     */
    Gen_Mo_On_Tree_Node(Node * root, Policy p = Policy())
      : root_(root), pol_(std::move(p))
    {
      ah_domain_error_if(root == nullptr)
        << "Gen_Mo_On_Tree_Node: root is null";
      build();
    }

    /** @brief Number of nodes in the tree. */
    [[nodiscard]] size_t size() const noexcept { return n_; }

    /** @brief True if the tree is empty. */
    [[nodiscard]] bool is_empty() const noexcept { return n_ == 0; }

    // ── Subtree queries ──────────────────────────────────────────

    /** @brief Solve subtree queries on the N-ary tree.
     *  @param query_roots  Array of subtree root nodes.
     *  @return Array of answers in the same order.
     */
    Array<answer_type> subtree_solve(
        const Array<Node*> & query_roots) const
    {
      const size_t q = query_roots.size();
      if (q == 0)
        return Array<answer_type>();

      auto queries = Array<Mo_Query>::create(q);
      for (size_t i = 0; i < q; ++i)
        {
          auto * p = node_to_id_.search(query_roots(i));
          ah_domain_error_if(p == nullptr)
            << "subtree_solve: query " << i << " node not in tree";
          const size_t id = p->second;
          queries(i) = {tin_(id), tout_(id), i};
        }

      return mo_sweep(flat_sub_, std::move(queries), q, n_);
    }

    /** @brief Solve subtree queries (initializer-list overload). */
    Array<answer_type> subtree_solve(
        std::initializer_list<Node*> il) const
    {
      const Array<Node*> roots(il);
      return subtree_solve(roots);
    }

    // ── Path queries ─────────────────────────────────────────────

    /** @brief Solve path queries on the N-ary tree.
     *  @param query_pairs  Array of (u, v) node-pointer pairs.
     *  @return Array of answers in the same order.
     */
    Array<answer_type> path_solve(
        const Array<std::pair<Node*, Node*>> & query_pairs) const
    {
      const size_t q = query_pairs.size();
      if (q == 0)
        return Array<answer_type>();

      struct Path_Query
      {
        size_t l, r, id, lca_id;
      };

      auto pqueries = Array<Path_Query>::create(q);
      for (size_t i = 0; i < q; ++i)
        {
          auto * pu = node_to_id_.search(query_pairs(i).first);
          auto * pv = node_to_id_.search(query_pairs(i).second);
          ah_domain_error_if(pu == nullptr or pv == nullptr)
            << "path_solve: query " << i << " node not in tree";

          size_t u = pu->second;
          size_t v = pv->second;

          if (first_(u) > first_(v))
            std::swap(u, v);

          const size_t ancestor = lca(u, v);

          if (ancestor == u)
            pqueries(i) = {first_(u), first_(v), i, NONE};
          else
            pqueries(i) = {last_(u), first_(v), i, ancestor};
        }

      // Snake sort
      const size_t tour_sz = 2 * n_;
      const size_t block = std::max<size_t>(
        1, static_cast<size_t>(
          std::sqrt(static_cast<double>(tour_sz))));

      std::sort(&pqueries(0), &pqueries(0) + q,
        [block](const Path_Query & a, const Path_Query & b)
          {
            const size_t ba = a.l / block;
            const size_t bb = b.l / block;
            if (ba != bb)
              return ba < bb;
            return (ba & 1) ? (a.r > b.r) : (a.r < b.r);
          });

      pol_.init(node_values_, n_);

      auto answers = Array<answer_type>::create(q);
      auto active = Array<bool>::create(n_);
      for (size_t i = 0; i < n_; ++i)
        active(i) = false;

      auto toggle = [&](size_t pos)
        {
          const size_t nid = flat_node_(pos);
          if (active(nid))
            {
              pol_.remove(node_values_, nid);
              active(nid) = false;
            }
          else
            {
              pol_.add(node_values_, nid);
              active(nid) = true;
            }
        };

      size_t cur_l = pqueries(0).l;
      size_t cur_r = pqueries(0).l;
      toggle(cur_l);

      while (cur_r < pqueries(0).r)
        toggle(++cur_r);

      if (pqueries(0).lca_id != NONE)
        {
          pol_.add(node_values_, pqueries(0).lca_id);
          answers(pqueries(0).id) = pol_.answer();
          pol_.remove(node_values_, pqueries(0).lca_id);
        }
      else
        answers(pqueries(0).id) = pol_.answer();

      for (size_t i = 1; i < q; ++i)
        {
          const size_t ql = pqueries(i).l;
          const size_t qr = pqueries(i).r;

          while (cur_r < qr) toggle(++cur_r);
          while (cur_l > ql) toggle(--cur_l);
          while (cur_r > qr) toggle(cur_r--);
          while (cur_l < ql) toggle(cur_l++);

          if (pqueries(i).lca_id != NONE)
            {
              pol_.add(node_values_, pqueries(i).lca_id);
              answers(pqueries(i).id) = pol_.answer();
              pol_.remove(node_values_, pqueries(i).lca_id);
            }
          else
            answers(pqueries(i).id) = pol_.answer();
        }

      return answers;
    }

    /** @brief Solve path queries (initializer-list overload). */
    Array<answer_type> path_solve(
        std::initializer_list<std::pair<Node*, Node*>> il) const
    {
      const Array<std::pair<Node*, Node*>> pairs(il);
      return path_solve(pairs);
    }
  };

  // ================================================================
  // Convenient typedefs
  // ================================================================

  /** @brief Mo on Trees specialised for counting distinct node values.
   *  @tparam GT  Graph type.
   *  @ingroup Graphs
   */
  template <class GT>
  using Distinct_Count_Mo_On_Trees =
    Gen_Mo_On_Trees<GT,
                    Distinct_Count_Policy<typename GT::Node::Node_Type>>;

  /** @brief Mo on Trees specialised for the "powerful array" query.
   *  @tparam GT  Graph type.
   *  @ingroup Graphs
   */
  template <class GT>
  using Powerful_Array_Mo_On_Trees =
    Gen_Mo_On_Trees<GT,
                    Powerful_Array_Policy<typename GT::Node::Node_Type>>;

  /** @brief Mo on Trees specialised for range mode queries.
   *  @tparam GT  Graph type.
   *  @ingroup Graphs
   */
  template <class GT>
  using Range_Mode_Mo_On_Trees =
    Gen_Mo_On_Trees<GT,
                    Range_Mode_Policy<typename GT::Node::Node_Type>>;

  /** @brief Mo on Tree_Node specialised for counting distinct values.
   *  @tparam T  Value type stored in Tree_Node.
   *  @ingroup Trees
   */
  template <typename T>
  using Distinct_Count_Mo_On_Tree_Node =
    Gen_Mo_On_Tree_Node<T, Distinct_Count_Policy<T>>;

  /** @brief Mo on Tree_Node specialised for the "powerful array" query.
   *  @tparam T  Value type stored in Tree_Node.
   *  @ingroup Trees
   */
  template <typename T>
  using Powerful_Array_Mo_On_Tree_Node =
    Gen_Mo_On_Tree_Node<T, Powerful_Array_Policy<T>>;

  /** @brief Mo on Tree_Node specialised for range mode queries.
   *  @tparam T  Value type stored in Tree_Node.
   *  @ingroup Trees
   */
  template <typename T>
  using Range_Mode_Mo_On_Tree_Node =
    Gen_Mo_On_Tree_Node<T, Range_Mode_Policy<T>>;

} // namespace Aleph

# endif // TPL_MO_ON_TREES_H
