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

/** @file Tree_DP.H
 *  @brief Generic tree dynamic programming and rerooting algorithms.
 *
 *  Provides a powerful framework for solving dynamic programming problems on
 *  trees represented as Aleph graphs.
 *
 *  Two main patterns are supported:
 *  - **Bottom-up Tree DP**: Standard DP that computes values from leaves up
 *    to the root. Useful for subtree-related queries (size, height, etc.).
 *  - **Rerooting DP**: An O(n) technique that computes the answer for *every*
 *    node as if it were the root of the tree. This is achieved through two
 *    passes (bottom-up and top-down) using prefix and suffix combinations.
 *
 *  Additionally, high-level convenience functions are provided for common
 *  queries like subtree sizes and tree diameters.
 *
 *  @example tree_dp_example.cc
 *
 *  @ingroup Algorithms
 *  @author Leandro Rabindranath Leon
 */

#ifndef TREE_DP_H
#define TREE_DP_H

#include <algorithm>
#include <cstddef>
#include <functional>
#include <limits>
#include <utility>

#include <ah-errors.H>
#include <tpl_array.H>
#include <tpl_dynListStack.H>
#include <tpl_dynMapOhash.H>
#include <tpl_dynMapTree.H>
#include <tpl_graph.H>

namespace Aleph {
namespace tree_dp_detail {
/** @brief Pre-processes and extracts tree topology from a graph.
 *
 *  Verifies tree properties, assigns internal IDs to nodes, and
 *  establishes parent-child relationships for a given root.
 *
 *  @tparam GT Graph type.
 *  @tparam SA Arc filter.
 */
template <class GT, class SA>
class Tree_Topology
{
public:
  using Node = typename GT::Node; /**< Node type. */
  using Arc = typename GT::Arc;   /**< Arc type. */

  /** @brief Sentinel for null/none parent or id. */
  static constexpr size_t NONE = std::numeric_limits<size_t>::max();

private:
  const GT *graph_ = nullptr; /**< Source graph. */
  SA sa_;                     /**< Arc filter. */
  Node *root_ = nullptr;      /**< Tree root. */
  size_t n_ = 0;              /**< Number of nodes. */

  Array<Node *> id_to_node_;             /**< Mapping from id to node pointer. */
  MapOLhash<Node *, size_t> node_to_id_; /**< Mapping from node pointer to id. */
  Array<Array<size_t>> children_;        /**< Children list in the rooted tree. */
  Array<size_t> parent_;                 /**< Parent id for each node. */
  Array<size_t> order_;                  /**< Post-order traversal (leaves first). */

  /** @brief Assign unique IDs to nodes and validate the root. */
  void index_nodes()
  {
    n_ = graph_->get_num_nodes();
    if (n_ == 0)
      {
        ah_domain_error_if(root_ != nullptr)
          << "Tree_Topology: non-null root provided for empty graph";
        return;
      }

    id_to_node_ = Array<Node *>::create(n_);
    {
      MapOLhash<Node *, size_t> tmp(n_);
      node_to_id_.swap(tmp);
    }

    size_t next_id = 0;
    for (Node_Iterator<GT> it(*graph_); it.has_curr(); it.next_ne())
      {
        Node *p = it.get_curr();
        id_to_node_(next_id) = p;
        node_to_id_.insert(p, next_id);
        ++next_id;
      }

    if (root_ == nullptr)
      root_ = id_to_node_(0);
    else
      ah_domain_error_if(node_to_id_.search(root_) == nullptr)
        << "Tree_Topology: root node is not part of the graph";
  }

  /** @brief Build undirected adjacency list and verify tree properties. */
  void build_adjacency()
  {
    if (n_ == 0)
      return;

    children_.reserve(n_);
    for (size_t i = 0; i < n_; ++i)
      children_.append(Array<size_t>());

    using Pair_Key = std::pair<size_t, size_t>;
    DynMapTree<Pair_Key, char> unique_edges;
    size_t edge_count = 0;

    for (Arc_Iterator<GT, SA> it(*graph_, sa_); it.has_curr(); it.next_ne())
      {
        Arc *a = it.get_curr_ne();
        Node *src = graph_->get_src_node(a);
        Node *tgt = graph_->get_tgt_node(a);

        const auto *si = node_to_id_.search(src);
        const auto *ti = node_to_id_.search(tgt);
        ah_runtime_error_unless(si != nullptr and ti != nullptr)
          << "Tree_Topology: arc endpoint not indexed";

        const size_t u = si->second;
        const size_t v = ti->second;
        ah_domain_error_if(u == v) << "Tree_Topology: self-loop detected";

        Pair_Key key = u < v ? std::make_pair(u, v) : std::make_pair(v, u);
        if (unique_edges.search(key) != nullptr)
          continue;  // skip duplicate
        unique_edges.insert(key, 1);
        ++edge_count;
      }

    ah_domain_error_if(n_ > 1 and edge_count != n_ - 1)
      << "Tree_Topology: not a tree (expected " << (n_ - 1) << " edges, got " << edge_count << ")";

    for (typename DynMapTree<Pair_Key, char>::Iterator it(unique_edges); it.has_curr(); it.next_ne())
      {
        const auto &[fst, snd] = it.get_curr();
        const size_t u = fst.first;
        const size_t v = fst.second;
        children_(u).append(v);
        children_(v).append(u);
      }
  }

  /** @brief BFS/DFS traversal to establish parent-child relations and order. */
  void build_order()
  {
    if (n_ == 0)
      return;

    parent_ = Array<size_t>::create(n_);
    for (size_t i = 0; i < n_; ++i)
      parent_(i) = NONE;

    order_.reserve(n_);

    Array<char> visited = Array<char>::create(n_);
    for (size_t i = 0; i < n_; ++i)
      visited(i) = 0;

    const size_t root_id = node_to_id_.find(root_);

    struct Frame
    {
      size_t node;
      size_t next_child;
    };
    DynListStack<Frame> stack;

    visited(root_id) = 1;
    stack.push({root_id, 0});

    while (not stack.is_empty())
      {
        auto &fr = stack.top();
        if (fr.next_child == children_(fr.node).size())
          {
            order_.append(fr.node);
            (void) stack.pop();
            continue;
          }

        const size_t nxt = children_(fr.node)(fr.next_child++);
        if (visited(nxt))
          continue;

        visited(nxt) = 1;
        parent_(nxt) = fr.node;
        stack.push({nxt, 0});
      }

    ah_domain_error_if(order_.size() != n_) << "Tree_Topology: graph is not connected";

    // Filter children_ to only contain children after rooting
    for (size_t i = 0; i < n_; ++i)
      {
        Array<size_t> children;
        const size_t p = parent_(i);
        for (size_t j = 0; j < children_(i).size(); ++j)
          if (children_(i)[j] != p)
            children.append(children_(i)[j]);
        children_(i).swap(children);
      }
  }

public:
  /** @brief Preprocess tree topology.
   *  @param[in] g    The graph tree.
   *  @param[in] root The root node.
   *  @param[in] sa   The arc filter.
   *  @throws ah_domain_error if not a tree or root not in graph.
   */
  Tree_Topology(const GT &g, Node *root, SA sa = SA()) : graph_(&g), sa_(std::move(sa)), root_(root)
  {
    index_nodes();
    build_adjacency();
    build_order();
  }

  /** @brief Returns number of nodes. */
  [[nodiscard]] size_t size() const noexcept
  {
    return n_;
  }

  /** @brief Returns root node pointer. */
  [[nodiscard]] Node *root() const noexcept
  {
    return root_;
  }

  /** @brief Returns internal ID of a node. */
  [[nodiscard]] size_t id_of(Node *node) const
  {
    const auto *item = node_to_id_.search(node);
    ah_domain_error_if(item == nullptr) << "Tree_Topology::id_of: node not in graph";
    return item->second;
  }

  /** @brief Returns node pointer for a given ID. */
  [[nodiscard]] Node *node_of(size_t id) const
  {
    ah_out_of_range_error_if(id >= n_) << "Tree_Topology::node_of: id out of range";
    return id_to_node_(id);
  }

  /** @brief Returns children IDs of a node. */
  [[nodiscard]] const Array<size_t> &children(size_t id) const
  {
    return children_(id);
  }

  /** @brief Returns parent ID of a node. */
  [[nodiscard]] size_t parent(size_t id) const noexcept
  {
    return parent_(id);
  }

  /** @brief Returns post-order traversal (leaves first, root last). */
  [[nodiscard]] const Array<size_t> &post_order() const noexcept
  {
    return order_;
  }

  /** @brief Returns all node pointers indexed by ID. */
  [[nodiscard]] const Array<Node *> &nodes() const noexcept
  {
    return id_to_node_;
  }
};
}  // namespace tree_dp_detail

/** @brief Generic bottom-up tree dynamic programming.
 *
 *  This class computes a DP value for every node in a rooted tree by
 *  combining the results of its children in post-order traversal.
 *
 *  The user defines the logic by providing:
 *  - An `Init_Fn` to set the base value for each node.
 *  - A `Combine_Fn` to merge a child's result into the parent's accumulator.
 *
 *  @tparam GT  Graph type.
 *  @tparam T   Value type computed at each node.
 *  @tparam SA  Arc filter (default Dft_Show_Arc).
 *
 *  @par **Complexity**: Time O(n), Space O(n), where n is the number of nodes.
 *
 *  @par **Example**: Subtree Size
 *  @code
 *  using G = List_Graph<>;
 *  Gen_Tree_DP<G, size_t> dp(g, root,
 *    [](auto *node) { return 1; },  // Each node starts with size 1
 *    [](auto *par, const size_t & acc, auto *child, const size_t & child_val) {
 *      return acc + child_val;      // Add subtree size of child to parent
 *    });
 *  size_t root_subtree = dp.value(root); // Should be total nodes in the tree
 *  @endcode
 */
template <class GT, typename T, class SA = Dft_Show_Arc<GT>>
class Gen_Tree_DP
{
public:
  using Node = typename GT::Node; /**< Node type. */

  /** @brief Initialization function signature.
   *  Given a node, returns its initial DP value.
   */
  using Init_Fn = std::function<T(Node *)>;

  /** @brief Combine function signature.
   *  Merges a child's DP result into the parent's current accumulator.
   *  Signature: `(parent_node, current_accumulator, child_node, child_result) -> new_accumulator`
   */
  using Combine_Fn = std::function<T(Node *, const T &, Node *, const T &)>;

private:
  tree_dp_detail::Tree_Topology<GT, SA> topo_; /**< Tree topology and order. */
  Array<T> dp_;                                /**< Computed DP values. */

public:
  /** @brief Construct and compute bottom-up DP.
   *
   *  @param[in] g        The graph (must be a tree under filter SA).
   *  @param[in] root     Root node.
   *  @param[in] init     Initialization function for each node.
   *  @param[in] combine  Combine function to fold children.
   *  @param[in] sa       Arc filter.
   */
  Gen_Tree_DP(const GT &g, Node *root, Init_Fn init, Combine_Fn combine, SA sa = SA())
    : topo_(g, root, std::move(sa))
  {
    const size_t n = topo_.size();
    if (n == 0)
      return;

    dp_ = Array<T>::create(n);

    // Initialize all nodes
    for (size_t i = 0; i < n; ++i)
      dp_(i) = init(topo_.node_of(i));

    // Process in post-order (leaves first)
    const auto &order = topo_.post_order();
    for (size_t k = 0; k < n; ++k)
      {
        const size_t v = order[k];
        const size_t par = topo_.parent(v);
        if (par == tree_dp_detail::Tree_Topology<GT, SA>::NONE)
          continue;  // root, no parent to update
        dp_(par) = combine(topo_.node_of(par), dp_(par), topo_.node_of(v), dp_(v));
      }
  }

  /** @brief Returns the DP value for a given node.
   *  @param[in] node Node pointer.
   *  @return Constant reference to the computed DP value.
   */
  [[nodiscard]] const T &value(Node *node) const
  {
    return dp_(topo_.id_of(node));
  }

  /** @brief Returns all DP values (indexed by internal node ID). */
  [[nodiscard]] const Array<T> &values() const noexcept
  {
    return dp_;
  }

  /** @brief Returns the number of nodes in the tree. */
  [[nodiscard]] size_t size() const noexcept
  {
    return topo_.size();
  }

  /** @brief Returns the node pointer for a given internal ID.
   *  @param[in] id Internal node ID.
   *  @return Node pointer.
   */
  [[nodiscard]] Node *node_of(size_t id) const
  {
    return topo_.node_of(id);
  }

  /** @brief Returns the internal ID for a given node pointer.
   *  @param[in] node Node pointer.
   *  @return Internal node ID.
   */
  [[nodiscard]] size_t id_of(Node *node) const
  {
    return topo_.id_of(node);
  }
};

/** @brief Convenient alias for Gen_Tree_DP.
 *  @tparam GT Graph type.
 *  @tparam T  Value type.
 *  @tparam SA Arc filter.
 */
template <class GT, typename T, class SA = Dft_Show_Arc<GT>>
using Tree_DP = Gen_Tree_DP<GT, T, SA>;

/** @brief Generic rerooting dynamic programming (all-roots DP).
 *
 *  Efficiently computes a DP answer for *every node* in the tree as if
 *  each node were the root.
 *
 *  Standard bottom-up DP only gives the answer for a fixed root.
 *  Rerooting DP achieves the all-roots result in O(n) by performing:
 *  1. A **bottom-up pass**: Computes "downward" values for a fixed root.
 *  2. A **top-down pass**: Combines downward results with prefix/suffix
 *     merges to compute "upward" (parent-side) contributions.
 *
 *  The user provides:
 *  - `identity`: Neutral element for the merge operation.
 *  - `init`: Base value for a single node.
 *  - `merge`: Associative operation to combine results from children.
 *  - `apply_edge`: Function to transform a subtree's result when
 *    passing through an edge to the parent.
 *
 *  @tparam GT  Graph type.
 *  @tparam T   Value type.
 *  @tparam SA  Arc filter.
 *
 *  @par **Complexity**: Time O(n), Space O(n).
 *
 *  @par **Example**: Tree Diameter / Eccentricities
 *  @code
 *  Gen_Reroot_DP<G, size_t> dp(g, some_root,
 *    0,                         // identity
 *    [](auto *n) { return 0; }, // init: leaf distance is 0
 *    [](size_t a, size_t b) { return std::max(a, b); }, // merge max
 *    [](auto *p, auto *c, size_t val) { return val + 1; } // apply edge: dist+1
 *  );
 *  size_t max_dist_from_v = dp.value(v); // Max distance from v to any leaf
 *  @endcode
 */
template <class GT, typename T, class SA = Dft_Show_Arc<GT>>
class Gen_Reroot_DP
{
public:
  using Node = typename GT::Node; /**< Node type. */

  /** @brief Initialization function signature.
   *  Returns the base contribution of a node.
   */
  using Init_Fn = std::function<T(Node *)>;

  /** @brief Merge function signature.
   *  An associative binary operation to combine results from multiple branches.
   */
  using Merge_Fn = std::function<T(const T &, const T &)>;

  /** @brief Edge transformation signature.
   *  Applies the effect of an edge between parent and child to a subtree result.
   *  Signature: `(parent, child, subtree_merged_value) -> contribution_to_parent`
   */
  using Apply_Edge_Fn = std::function<T(Node *, Node *, const T &)>;

private:
  tree_dp_detail::Tree_Topology<GT, SA> topo_; /**< Tree topology and order. */
  T identity_;                                 /**< Identity for merge operation. */
  Array<T> init_vals_;                         /**< Cached per-node base values. */
  Array<T> dp_down_;                           /**< Bottom-up DP results. */
  Array<T> dp_up_;                             /**< Top-down contribution from parent side. */
  Array<T> answer_;                            /**< Final answer for each node as root. */

public:
  /** @brief Construct and compute rerooting DP.
   *
   *  @param[in] g           The graph (tree under SA).
   *  @param[in] root        Initial root for bottom-up pass.
   *  @param[in] identity    Neutral element for merge.
   *  @param[in] init        Base value for each node.
   *  @param[in] merge       Associative merge operation.
   *  @param[in] apply_edge  Transform child value across an edge.
   *  @param[in] sa          Arc filter.
   */
  Gen_Reroot_DP(const GT &g,
                Node *root,
                const T &identity,
                Init_Fn init,
                Merge_Fn merge,
                Apply_Edge_Fn apply_edge,
                SA sa = SA())
    : topo_(g, root, std::move(sa)), identity_(identity)
  {
    const size_t n = topo_.size();
    if (n == 0)
      return;

    init_vals_ = Array<T>::create(n);
    dp_down_ = Array<T>::create(n);
    dp_up_ = Array<T>::create(n);
    answer_ = Array<T>::create(n);

    for (size_t i = 0; i < n; ++i)
      {
        init_vals_(i) = init(topo_.node_of(i));
        dp_down_(i) = init_vals_[i];
        dp_up_(i) = identity_;
      }

    // Phase 1: bottom-up
    const auto &order = topo_.post_order();
    for (size_t k = 0; k < n; ++k)
      {
        const size_t v = order[k];
        const size_t par = topo_.parent(v);
        if (par == tree_dp_detail::Tree_Topology<GT, SA>::NONE)
          continue;
        const T contrib = apply_edge(topo_.node_of(par), topo_.node_of(v), dp_down_(v));
        dp_down_(par) = merge(dp_down_(par), contrib);
      }

    // Phase 2: top-down with prefix/suffix
    // Process nodes in reverse post-order (root first)
    for (size_t k = n; k-- > 0;)
      {
        const size_t v = order[k];
        Node *vn = topo_.node_of(v);

        // Collect children of v
        const auto &children = topo_.children(v);
        const size_t nc = children.size();
        if (nc == 0)
          continue;

        Array<T> contribs = Array<T>::create(nc);
        for (size_t j = 0; j < nc; ++j)
          {
            const size_t c = children[j];
            contribs(j) = apply_edge(vn, topo_.node_of(c), dp_down_(c));
          }

        // Build prefix and suffix arrays of merged contributions
        Array<T> prefix = Array<T>::create(nc + 1);
        Array<T> suffix = Array<T>::create(nc + 1);
        prefix(0) = identity_;
        suffix(nc) = identity_;

        for (size_t j = 0; j < nc; ++j)
          prefix(j + 1) = merge(prefix(j), contribs(j));

        for (size_t j = nc; j-- > 0;)
          suffix(j) = merge(suffix(j + 1), contribs(j));

        const T base = merge(init_vals_[v], dp_up_(v));

        // For each child, compute dp_up
        for (size_t j = 0; j < nc; ++j)
          {
            const size_t c = children[j];
            Node *cn = topo_.node_of(c);
            // Value of v without child c's subtree:
            // merge(init(v), dp_up(v), prefix(j), suffix(j+1))
            T without_c = merge(base, prefix(j));
            without_c = merge(without_c, suffix(j + 1));
            dp_up_(c) = apply_edge(cn, vn, without_c);
          }
      }

    // Phase 3: compute answers
    for (size_t i = 0; i < n; ++i)
      answer_(i) = merge(dp_down_(i), dp_up_(i));
  }

  /** @brief Returns the answer for a given node as root.
   *  @param[in] node Node pointer.
   *  @return Constant reference to the computed DP answer.
   */
  [[nodiscard]] const T &value(Node *node) const
  {
    return answer_(topo_.id_of(node));
  }

  /** @brief Returns all computed answers (indexed by internal ID). */
  [[nodiscard]] const Array<T> &values() const noexcept
  {
    return answer_;
  }

  /** @brief Returns the number of nodes in the tree. */
  [[nodiscard]] size_t size() const noexcept
  {
    return topo_.size();
  }

  /** @brief Returns the node pointer for a given internal ID.
   *  @param[in] id Internal node ID.
   *  @return Node pointer.
   */
  [[nodiscard]] Node *node_of(size_t id) const
  {
    return topo_.node_of(id);
  }

  /** @brief Returns the internal ID for a given node pointer.
   *  @param[in] node Node pointer.
   *  @return Internal node ID.
   */
  [[nodiscard]] size_t id_of(Node *node) const
  {
    return topo_.id_of(node);
  }
};

/** @brief Convenient alias for Gen_Reroot_DP.
 *  @tparam GT Graph type.
 *  @tparam T  Value type.
 *  @tparam SA Arc filter.
 */
template <class GT, typename T, class SA = Dft_Show_Arc<GT>>
using Reroot_DP = Gen_Reroot_DP<GT, T, SA>;

// ── Convenience functions ─────────────────────────────────────────

/** @brief Compute subtree sizes for every node.
 *
 *  @param[in] g     The graph (tree).
 *  @param[in] root  Root node.
 *  @param[in] sa    Arc filter.
 *  @return Array of subtree sizes indexed by internal node id.
 *
 *  @note Complexity: O(n).
 */
template <class GT, class SA = Dft_Show_Arc<GT>>
[[nodiscard]] Array<size_t> tree_subtree_sizes(const GT &g, typename GT::Node *root, SA sa = SA())
{
  Gen_Tree_DP<GT, size_t, SA> dp(g,
                                 root,
                                 [](auto *) -> size_t
  {
    return 1;
  },
                                 [](auto *, const size_t &acc, auto *, const size_t &child) -> size_t
  {
    return acc + child;
  },
                                 std::move(sa));

  // Copy values out
  const auto &vals = dp.values();
  Array<size_t> result = Array<size_t>::create(vals.size());
  for (size_t i = 0; i < vals.size(); ++i)
    result(i) = vals[i];
  return result;
}

/** @brief Compute the maximum distance from each node to any leaf.
 *
 *  Uses rerooting DP to compute the eccentricity (max distance)
 *  for every node.
 *
 *  @param[in] g     The graph (tree).
 *  @param[in] root  Root node.
 *  @param[in] sa    Arc filter.
 *  @return Array of max distances indexed by internal node id.
 *
 *  @note Complexity: O(n).
 */
template <class GT, class SA = Dft_Show_Arc<GT>>
[[nodiscard]] Array<size_t> tree_max_distance(const GT &g, typename GT::Node *root, SA sa = SA())
{
  Gen_Reroot_DP<GT, size_t, SA> dp(g,
                                   root,
                                   static_cast<size_t>(0),
                                   [](auto *) -> size_t
  {
    return 0;
  },
                                   [](const size_t &a, const size_t &b) -> size_t
  {
    return std::max(a, b);
  },
                                   [](auto *, auto *, const size_t &v) -> size_t
  {
    return v + 1;
  },
                                   std::move(sa));

  const auto &vals = dp.values();
  Array<size_t> result = Array<size_t>::create(vals.size());
  for (size_t i = 0; i < vals.size(); ++i)
    result(i) = vals[i];
  return result;
}

/** @brief Compute sum of distances from each node to all others.
 *
 *  Uses rerooting DP with (count, sum_dist) pairs.
 *
 *  @param[in] g     The graph (tree).
 *  @param[in] root  Root node.
 *  @param[in] sa    Arc filter.
 *  @return Array of sum-of-distances indexed by internal node id.
 *
 *  @note Complexity: O(n).
 */
template <class GT, class SA = Dft_Show_Arc<GT>>
[[nodiscard]] Array<size_t> tree_sum_of_distances(const GT &g, typename GT::Node *root, SA sa = SA())
{
  using P = std::pair<size_t, size_t>;  // (count, sum_dist)

  Gen_Reroot_DP<GT, P, SA> dp(g,
                              root,
                              P{0, 0},
                              [](auto *) -> P
  {
    return {1, 0};
  },
                              [](const P &a, const P &b) -> P
  {
    return {a.first + b.first, a.second + b.second};
  },
                              [](auto *, auto *, const P &v) -> P
  {
    return {v.first, v.second + v.first};
  },
                              std::move(sa));

  const auto &vals = dp.values();
  Array<size_t> result = Array<size_t>::create(vals.size());
  for (size_t i = 0; i < vals.size(); ++i)
    result(i) = vals[i].second;
  return result;
}
}  // namespace Aleph

#endif  // TREE_DP_H
