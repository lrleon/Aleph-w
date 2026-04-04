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

/** @file LCA.H
 *  @brief Lowest Common Ancestor (LCA) on rooted trees represented as Aleph graphs.
 *
 *  The Lowest Common Ancestor (LCA) of two nodes@f$u@f$and@f$v@f$in a rooted tree
 *  is the deepest node that is an ancestor of both@f$u@f$and@f$v@f$.
 *
 *  @par Concept Visualization
 *  @code
 *          [0] (Root)          LCA(4, 5) = 2
 *         /   \                LCA(4, 6) = 0
 *       [1]   [2]              LCA(1, 4) = 0
 *            /   \             LCA(2, 5) = 2 (a node can be its own ancestor)
 *          [3]   [4]
 *               /
 *             [5]
 *  @endcode
 *
 *  This header provides two distinct LCA engines with different trade-offs:
 *
 *  ### 1. Binary Lifting (`Gen_Binary_Lifting_LCA`)
 *  Uses a jump table (sparse table over ancestors) where `up[k][v]` stores the
 * @f$2^k@f$-th ancestor of node@f$v@f$.
 *  - **Preprocessing**:@f$O(n \log n)@f$time and space.
 *  - **Query**:@f$O(\log n)@f$time.
 *  - **Best for**: General purpose, especially when memory is more constrained than
 *    the Euler approach, or when path-based properties are not needed.
 *
 *  ### 2. Euler Tour + RMQ (`Gen_Euler_RMQ_LCA`)
 *  Reduces the LCA problem to a Range Minimum Query (RMQ) problem. It performs an
 *  Euler tour (recording nodes as they are visited and returned to) and stores
 *  the depths. The LCA of@f$u@f$and@f$v@f$is the node with minimum depth
 *  between their first occurrences in the tour.
 *  - **Preprocessing**:@f$O(n \log n)@f$time and space (using a Sparse Table for RMQ).
 *  - **Query**:@f$O(1)@f$time.
 *  - **Best for**: Heavy query workloads where constant-time response is critical.
 *
 *  Both classes:
 *  - Work on Aleph graph backends (`List_Graph`, `List_SGraph`, `Array_Graph`).
 *  - Accept arc filters (`SA`) to define the tree topology over a general graph.
 *  - **Strict Validation**: Upon construction, the filtered graph is validated to be
 *    a valid tree (connected, acyclic,@f$m = n - 1@f$). If validation fails,
 *    a domain error exception is thrown.
 *  - Provide distance calculation between any two nodes:@f$dist(u, v) = depth(u) + depth(v) - 2
 * \cdot depth(LCA(u, v))@f$.
 *
 *  @par Example: Distance between nodes
 *  @code
 *  using G = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
 *  G g;
 *  // ... build a tree in g ...
 *  Euler_RMQ_LCA<G> lca(g, root);
 *
 *  size_t d = lca.distance(node1, node2);
 *  std::cout << "Distance: " << d << " edges\n";
 *  @endcode
 *
 *  @see tpl_sparse_table.H
 *  @see tpl_graph.H
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

#ifndef LCA_H
#define LCA_H

#include <algorithm>
#include <bit>
#include <cstddef>
#include <limits>
#include <utility>

#include <ah-errors.H>
#include <tpl_array.H>
#include <tpl_dynListStack.H>
#include <tpl_dynMapOhash.H>
#include <tpl_dynMapTree.H>
#include <tpl_graph.H>
#include <tpl_sparse_table.H>

namespace Aleph {
namespace lca_detail {
/** @internal
 *  @brief Internal helper that encapsulates the tree topology and DFS data.
 *
 *  Rooted_Tree_Data performs several critical tasks during construction:
 *  1. **Node Indexing**: Maps graph nodes to integer IDs in the range [0, n-1].
 *  2. **Arc Filtering**: Uses the provided @p SA filter to define the tree edges.
 *  3. **Topology Validation**: Ensures the filtered graph is connected and acyclic
 *     (i.e., a tree) and has exactly@f$n-1@f$edges.
 *  4. **DFS Traversal**: Computes parent pointers, node depths, entry/exit times
 *     (tin/tout), and the Euler tour sequence.
 */
template <class GT, class SA>
class Rooted_Tree_Data
{
public:
  using Node = typename GT::Node;
  using Arc = typename GT::Arc;

  static constexpr size_t NONE = std::numeric_limits<size_t>::max();

private:
  using Pair_Key = std::pair<size_t, size_t>;

  const GT *graph_ = nullptr;
  SA sa_;
  Node *root_ = nullptr;
  size_t root_id_ = NONE;

  size_t n_ = 0;

  Array<Node *> id_to_node_;
  MapOLhash<Node *, size_t> node_to_id_;

  Array<Array<size_t>> adjacency_;
  Array<size_t> parent_;
  Array<size_t> depth_;
  Array<size_t> tin_;
  Array<size_t> tout_;
  Array<size_t> first_;
  Array<size_t> euler_;
  size_t euler_size_ = 0;

  static Pair_Key normalize_pair(size_t u, size_t v) noexcept
  {
    if (u > v)
      std::swap(u, v);
    return std::make_pair(u, v);
  }

  void index_nodes()
  {
    n_ = graph_->get_num_nodes();

    if (n_ == 0)
      {
        ah_domain_error_if(root_ != nullptr)
          << "Rooted_Tree_Data: root node provided but graph is empty";
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

    ah_runtime_error_unless(next_id == n_) << "Rooted_Tree_Data: failed to index all graph nodes";

    if (root_ == nullptr)
      root_ = id_to_node_(0);

    ah_domain_error_if(node_to_id_.search(root_) == nullptr)
      << "Rooted_Tree_Data: root node does not belong to graph";

    root_id_ = node_to_id_.find(root_);
  }

  void build_simple_adjacency()
  {
    if (n_ == 0)
      return;

    adjacency_.empty();
    adjacency_.reserve(n_);
    for (size_t i = 0; i < n_; ++i)
      adjacency_.append(Array<size_t>());

    DynMapTree<Pair_Key, char> unique_edges;
    size_t edge_count = 0;

    for (Arc_Iterator<GT, SA> it(*graph_, sa_); it.has_curr(); it.next_ne())
      {
        Arc *a = it.get_curr_ne();
        Node *src = graph_->get_src_node(a);
        Node *tgt = graph_->get_tgt_node(a);

        const auto *src_item = node_to_id_.search(src);
        const auto *tgt_item = node_to_id_.search(tgt);

        ah_runtime_error_unless(src_item != nullptr and tgt_item != nullptr)
          << "Rooted_Tree_Data: arc endpoint is not indexed";

        const size_t u = src_item->second;
        const size_t v = tgt_item->second;

        ah_domain_error_if(u == v) << "Rooted_Tree_Data: self-loop detected in filtered graph";

        const Pair_Key key = normalize_pair(u, v);
        ah_domain_error_if(unique_edges.search(key) != nullptr)
          << "Rooted_Tree_Data: parallel/duplicate edge detected in filtered graph";

        unique_edges.insert(key, 1);
        ++edge_count;
      }

    ah_domain_error_if(edge_count != n_ - 1)
      << "Rooted_Tree_Data: filtered graph is not a tree (expected " << (n_ - 1) << " edges, got "
      << edge_count << ")";

    for (typename DynMapTree<Pair_Key, char>::Iterator it(unique_edges); it.has_curr(); it.next_ne())
      {
        const auto &[fst, snd] = it.get_curr();
        const size_t u = fst.first;
        const size_t v = fst.second;
        adjacency_(u).append(v);
        adjacency_(v).append(u);
      }
  }

  void build_dfs_data()
  {
    if (n_ == 0)
      return;

    parent_ = Array<size_t>::create(n_);
    depth_ = Array<size_t>::create(n_);
    tin_ = Array<size_t>::create(n_);
    tout_ = Array<size_t>::create(n_);
    first_ = Array<size_t>::create(n_);
    euler_ = Array<size_t>::create(2 * n_ - 1);

    for (size_t i = 0; i < n_; ++i)
      parent_(i) = NONE;

    Array<char> visited = Array<char>::create(n_);
    for (size_t i = 0; i < n_; ++i)
      visited(i) = 0;

    struct Frame
    {
      size_t node;
      size_t parent;
      size_t next_child;
    };

    DynListStack<Frame> stack;

    size_t timer = 0;
    size_t visited_count = 1;

    visited(root_id_) = 1;
    depth_(root_id_) = 0;
    parent_(root_id_) = NONE;
    tin_(root_id_) = timer++;

    euler_size_ = 0;
    first_(root_id_) = euler_size_;
    euler_(euler_size_++) = root_id_;

    stack.push({root_id_, NONE, 0});

    while (not stack.is_empty())
      {
        auto &fr = stack.top();

        if (fr.next_child == adjacency_(fr.node).size())
          {
            tout_(fr.node) = timer - 1;
            (void) stack.pop();
            if (not stack.is_empty())
              euler_(euler_size_++) = stack.top().node;
            continue;
          }

        const size_t nxt = adjacency_(fr.node)(fr.next_child++);
        if (nxt == fr.parent)
          continue;

        ah_domain_error_if(visited(nxt)) << "Rooted_Tree_Data: filtered graph is not acyclic";

        visited(nxt) = 1;
        ++visited_count;

        parent_(nxt) = fr.node;
        depth_(nxt) = depth_(fr.node) + 1;
        tin_(nxt) = timer++;

        first_(nxt) = euler_size_;
        euler_(euler_size_++) = nxt;

        stack.push({nxt, fr.node, 0});
      }

    ah_domain_error_if(visited_count != n_) << "Rooted_Tree_Data: filtered graph is not connected";

    ah_runtime_error_unless(euler_size_ == 2 * n_ - 1)
      << "Rooted_Tree_Data: unexpected Euler tour size";
  }

  void check_id(const size_t id, const char *where) const
  {
    ah_out_of_range_error_if(id >= n_)
      << where << ": id=" << id << " is out of range [0, " << n_ << ")";
  }

public:
  Rooted_Tree_Data(const GT &g, Node *root, SA sa = SA())
    : graph_(&g), sa_(std::move(sa)), root_(root)
  {
    index_nodes();
    build_simple_adjacency();
    build_dfs_data();
  }

  [[nodiscard]] size_t size() const noexcept
  {
    return n_;
  }
  [[nodiscard]] bool is_empty() const noexcept
  {
    return n_ == 0;
  }
  [[nodiscard]] Node *root() const noexcept
  {
    return root_;
  }
  [[nodiscard]] size_t root_id() const noexcept
  {
    return root_id_;
  }

  [[nodiscard]] const Array<Node *> &id_to_node() const noexcept
  {
    return id_to_node_;
  }
  [[nodiscard]] const Array<size_t> &parent() const noexcept
  {
    return parent_;
  }
  [[nodiscard]] const Array<size_t> &depth() const noexcept
  {
    return depth_;
  }
  [[nodiscard]] const Array<size_t> &tin() const noexcept
  {
    return tin_;
  }
  [[nodiscard]] const Array<size_t> &tout() const noexcept
  {
    return tout_;
  }
  [[nodiscard]] const Array<size_t> &first() const noexcept
  {
    return first_;
  }
  [[nodiscard]] const Array<size_t> &euler() const noexcept
  {
    return euler_;
  }
  [[nodiscard]] size_t euler_size() const noexcept
  {
    return euler_size_;
  }

  [[nodiscard]] size_t id_of(const Node *node) const
  {
    ah_invalid_argument_if(node == nullptr) << "Rooted_Tree_Data::id_of: null node";

    const auto *item = node_to_id_.search(const_cast<Node *>(node));
    ah_domain_error_if(item == nullptr) << "Rooted_Tree_Data::id_of: node does not belong to graph";

    return item->second;
  }

  [[nodiscard]] Node *node_of(const size_t id) const
  {
    check_id(id, "Rooted_Tree_Data::node_of");
    return id_to_node_(id);
  }

  void validate_id(const size_t id, const char *where) const
  {
    check_id(id, where);
  }

  [[nodiscard]] bool is_ancestor(const size_t u, const size_t v) const
  {
    check_id(u, "Rooted_Tree_Data::is_ancestor");
    check_id(v, "Rooted_Tree_Data::is_ancestor");
    return tin_(u) <= tin_(v) and tout_(v) <= tout_(u);
  }
};

struct Depth_Node
{
  size_t depth = 0;
  size_t node = 0;
};

struct Depth_Node_Min_Op
{
  Depth_Node operator()(const Depth_Node &a, const Depth_Node &b) const noexcept
  {
    if (a.depth < b.depth)
      return a;
    if (b.depth < a.depth)
      return b;
    return (a.node <= b.node) ? a : b;
  }
};
}  // namespace lca_detail

/** @brief LCA via binary lifting on a rooted tree.
 *
 *  Binary lifting is a technique that uses a jump table (sparse table) to find
 *  ancestors efficiently. For each node, we precompute its 1st, 2nd, 4th, 8th, ...
 * @f$2^k@f$-th ancestor.
 *
 *  To find the LCA of@f$u@f$and@f$v@f$:
 *  1. Lift the deeper node until it is at the same depth as the other.
 *  2. If they are now the same node, that is the LCA.
 *  3. Otherwise, jump upwards by the largest possible powers of 2 such that
 *     the resulting nodes are still different. The parent of the final nodes
 *     is the LCA.
 *
 *  Build time is@f$O(n \log n)@f$, each LCA query costs@f$O(\log n)@f$.
 *
 *  @tparam GT Graph type (`List_Graph`, `List_SGraph`, `Array_Graph`).
 *  @tparam SA Arc filter type (default: `Dft_Show_Arc<GT>`).
 *
 *  @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>>
class Gen_Binary_Lifting_LCA
{
public:
  using Node = typename GT::Node;

private:
  using Topology = lca_detail::Rooted_Tree_Data<GT, SA>;
  static constexpr size_t NONE = Topology::NONE;

  Topology topology_;
  size_t levels_ = 0;
  Array<size_t> up_;  // flattened: up_[k * n + v]

  [[nodiscard]] size_t n() const noexcept
  {
    return topology_.size();
  }

  size_t &up_at(const size_t k, const size_t v) noexcept
  {
    return up_(k * n() + v);
  }

  [[nodiscard]] size_t up_at(const size_t k, const size_t v) const noexcept
  {
    return up_(k * n() + v);
  }

  void ensure_not_empty(const char *where) const
  {
    ah_domain_error_if(is_empty()) << where << ": tree is empty";
  }

  /** @internal Build the jump table up[k][v] = 2^k-th ancestor of v. */
  void build_jump_table()
  {
    if (is_empty())
      return;

    levels_ = static_cast<size_t>(std::bit_width(n()));

    up_ = Array<size_t>::create(levels_ * n());
    for (size_t i = 0; i < levels_ * n(); ++i)
      up_(i) = NONE;

    for (size_t v = 0; v < n(); ++v)
      up_at(0, v) = topology_.parent()(v);

    for (size_t k = 1; k < levels_; ++k)
      for (size_t v = 0; v < n(); ++v)
        {
          const size_t mid = up_at(k - 1, v);
          up_at(k, v) = (mid == NONE) ? NONE : up_at(k - 1, mid);
        }
  }

  /** @internal Lift node v by exactly delta levels up. */
  [[nodiscard]] size_t lift(size_t v, size_t delta) const
  {
    for (size_t k = 0; k < levels_ and delta > 0 and v != NONE; ++k)
      if ((delta >> k) & 1U)
        v = up_at(k, v);
    return v;
  }

public:
  /** @brief Construct from graph + explicit root.
   *
   *  The constructor validates that the filtered graph is a tree.
   *
   *  @param[in] g Rooted tree topology (filtered by @p sa).
   *  @param[in] root Root node pointer (must belong to @p g).
   *  @param[in] sa Arc filter.
   *  @throw ah_domain_error if the filtered graph is not a tree.
   */
  Gen_Binary_Lifting_LCA(const GT &g, Node *root, SA sa = SA()) : topology_(g, root, std::move(sa))
  {
    build_jump_table();
  }

  /** @brief Construct using the first graph node as root.
   *
   *  @param[in] g Rooted tree topology (filtered by @p sa).
   *  @param[in] sa Arc filter.
   */
  Gen_Binary_Lifting_LCA(const GT &g, SA sa = SA()) : topology_(g, nullptr, std::move(sa))
  {
    build_jump_table();
  }

  /** @brief Total number of nodes in the tree. */
  [[nodiscard]] size_t size() const noexcept
  {
    return topology_.size();
  }

  /** @brief Returns true if the tree has no nodes. */
  [[nodiscard]] bool is_empty() const noexcept
  {
    return topology_.is_empty();
  }

  /** @brief Returns the root node of the tree. */
  [[nodiscard]] Node *root() const noexcept
  {
    return topology_.root();
  }

  /** @brief Returns the internal ID of the root node. */
  [[nodiscard]] size_t root_id() const noexcept
  {
    return topology_.root_id();
  }

  /** @brief Returns the number of levels in the jump table (@f$\lceil \log_2 n \rceil@f$). */
  [[nodiscard]] size_t num_levels() const noexcept
  {
    return levels_;
  }

  /** @brief Map an internal ID [0, n-1] back to a Node pointer. */
  [[nodiscard]] Node *node_of(const size_t id) const
  {
    return topology_.node_of(id);
  }

  /** @brief Map a Node pointer to its internal ID [0, n-1]. */
  [[nodiscard]] size_t id_of(const Node *node) const
  {
    return topology_.id_of(node);
  }

  /** @brief Distance from root to node by ID. */
  [[nodiscard]] size_t depth_of_id(const size_t id) const
  {
    topology_.validate_id(id, "Gen_Binary_Lifting_LCA::depth_of_id");
    return topology_.depth()(id);
  }

  /** @brief Distance from root to node. */
  [[nodiscard]] size_t depth_of(const Node *node) const
  {
    return depth_of_id(id_of(node));
  }

  /** @brief Parent ID of node @p id, or `NONE` if root. */
  [[nodiscard]] size_t parent_id(const size_t id) const
  {
    topology_.validate_id(id, "Gen_Binary_Lifting_LCA::parent_id");
    return topology_.parent()(id);
  }

  /** @brief Parent of node, or `nullptr` if root. */
  [[nodiscard]] Node *parent_of(const Node *node) const
  {
    const size_t pid = parent_id(id_of(node));
    return pid == NONE ? nullptr : node_of(pid);
  }

  /** @brief Returns true if node @p u is an ancestor of @p v (or u == v). */
  [[nodiscard]] bool is_ancestor_id(const size_t u, const size_t v) const
  {
    return topology_.is_ancestor(u, v);
  }

  /** @brief Returns true if node @p u is an ancestor of @p v (or u == v). */
  [[nodiscard]] bool is_ancestor(const Node *u, const Node *v) const
  {
    return is_ancestor_id(id_of(u), id_of(v));
  }

  /** @brief k-th ancestor by id in O(log n).
   *
   *  @param[in] id Node id.
   *  @param[in] k Number of steps up.
   *  @return Ancestor id, or `NONE` if `k > depth(id)`.
   */
  [[nodiscard]] size_t kth_ancestor_id(const size_t id, const size_t k) const
  {
    topology_.validate_id(id, "Gen_Binary_Lifting_LCA::kth_ancestor_id");
    if (k > topology_.depth()(id))
      return NONE;
    return lift(id, k);
  }

  /** @brief k-th ancestor by node pointer in O(log n).
   *
   *  @return Ancestor node, or `nullptr` if it does not exist.
   */
  [[nodiscard]] Node *kth_ancestor(const Node *node, const size_t k) const
  {
    const size_t a = kth_ancestor_id(id_of(node), k);
    return a == NONE ? nullptr : node_of(a);
  }

  /** @brief LCA query by node ids in O(log n). */
  [[nodiscard]] size_t lca_id(size_t u, size_t v) const
  {
    ensure_not_empty("Gen_Binary_Lifting_LCA::lca_id");
    topology_.validate_id(u, "Gen_Binary_Lifting_LCA::lca_id");
    topology_.validate_id(v, "Gen_Binary_Lifting_LCA::lca_id");

    if (topology_.depth()(u) < topology_.depth()(v))
      std::swap(u, v);

    u = lift(u, topology_.depth()(u) - topology_.depth()(v));
    if (u == v)
      return u;

    for (size_t k = levels_; k-- > 0;)
      {
        const size_t uu = up_at(k, u);
        const size_t vv = up_at(k, v);
        if (uu != vv)
          {
            u = uu;
            v = vv;
          }
      }

    const size_t ret = topology_.parent()(u);
    ah_runtime_error_unless(ret != NONE)
      << "Gen_Binary_Lifting_LCA::lca_id: internal invalid parent";
    return ret;
  }

  /** @brief LCA query by node pointers in O(log n). */
  [[nodiscard]] Node *lca(const Node *u, const Node *v) const
  {
    return node_of(lca_id(id_of(u), id_of(v)));
  }

  /** @brief Number of edges on the path between two ids in O(log n). */
  [[nodiscard]] size_t distance_id(const size_t u, const size_t v) const
  {
    const size_t a = lca_id(u, v);
    return topology_.depth()(u) + topology_.depth()(v) - 2 * topology_.depth()(a);
  }

  /** @brief Number of edges on the path between two nodes in O(log n). */
  [[nodiscard]] size_t distance(const Node *u, const Node *v) const
  {
    return distance_id(id_of(u), id_of(v));
  }
};

/** @brief LCA via Euler tour + RMQ on depth in a rooted tree.
 *
 *  This engine reduces the LCA problem to a Range Minimum Query (RMQ) problem.
 *
 *  **The Reduction**:
 *  1. Perform an Euler tour of the tree, recording each node as it is visited
 *     and as we return from its children. The tour has size@f$2n - 1@f$.
 *  2. For each node in the tour, store its depth.
 *  3. Store the first occurrence index `first[v]` of each node `v` in the tour.
 *  4. The LCA of@f$u@f$and@f$v@f$is the node with the **minimum depth**
 *     in the Euler tour range `[first[u], first[v]]`.
 *
 *  By using a Sparse Table for RMQ, we achieve constant time@f$O(1)@f$queries
 *  after@f$O(n \log n)@f$preprocessing.
 *
 *  @tparam GT Graph type (`List_Graph`, `List_SGraph`, `Array_Graph`).
 *  @tparam SA Arc filter type (default: `Dft_Show_Arc<GT>`).
 *
 *  @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>>
class Gen_Euler_RMQ_LCA
{
public:
  using Node = typename GT::Node;

private:
  using Topology = lca_detail::Rooted_Tree_Data<GT, SA>;
  static constexpr size_t NONE = Topology::NONE;
  using Depth_Node = lca_detail::Depth_Node;
  using Depth_Node_Min_Op = lca_detail::Depth_Node_Min_Op;

  Topology topology_;
  Array<Depth_Node> euler_depth_;
  Gen_Sparse_Table<Depth_Node, Depth_Node_Min_Op> rmq_;

  void ensure_not_empty(const char *where) const
  {
    ah_domain_error_if(is_empty()) << where << ": tree is empty";
  }

  /** @internal Build the depth-based Euler tour and the RMQ sparse table. */
  void build_rmq()
  {
    if (is_empty())
      return;

    euler_depth_ = Array<Depth_Node>::create(topology_.euler_size());
    for (size_t i = 0; i < topology_.euler_size(); ++i)
      {
        const size_t node = topology_.euler()(i);
        euler_depth_(i) = Depth_Node{topology_.depth()(node), node};
      }

    rmq_ = Gen_Sparse_Table<Depth_Node, Depth_Node_Min_Op>(euler_depth_, Depth_Node_Min_Op());
  }

public:
  /** @brief Construct from graph + explicit root.
   *
   *  The constructor validates that the filtered graph is a tree.
   *
   *  @throw ah_domain_error if the filtered graph is not a tree.
   */
  Gen_Euler_RMQ_LCA(const GT &g, Node *root, SA sa = SA())
    : topology_(g, root, std::move(sa)), rmq_(1, Depth_Node(), Depth_Node_Min_Op())
  {
    build_rmq();
  }

  /** @brief Construct using the first graph node as root. */
  Gen_Euler_RMQ_LCA(const GT &g, SA sa = SA())
    : topology_(g, nullptr, std::move(sa)), rmq_(1, Depth_Node(), Depth_Node_Min_Op())
  {
    build_rmq();
  }

  /** @brief Total number of nodes in the tree. */
  [[nodiscard]] size_t size() const noexcept
  {
    return topology_.size();
  }

  /** @brief Returns true if the tree has no nodes. */
  [[nodiscard]] bool is_empty() const noexcept
  {
    return topology_.is_empty();
  }

  /** @brief Returns the root node of the tree. */
  [[nodiscard]] Node *root() const noexcept
  {
    return topology_.root();
  }

  /** @brief Returns the internal ID of the root node. */
  [[nodiscard]] size_t root_id() const noexcept
  {
    return topology_.root_id();
  }

  /** @brief Map an internal ID [0, n-1] back to a Node pointer. */
  [[nodiscard]] Node *node_of(const size_t id) const
  {
    return topology_.node_of(id);
  }

  /** @brief Map a Node pointer to its internal ID [0, n-1]. */
  [[nodiscard]] size_t id_of(const Node *node) const
  {
    return topology_.id_of(node);
  }

  /** @brief Distance from root to node by ID. */
  [[nodiscard]] size_t depth_of_id(const size_t id) const
  {
    topology_.validate_id(id, "Gen_Euler_RMQ_LCA::depth_of_id");
    return topology_.depth()(id);
  }

  /** @brief Distance from root to node. */
  [[nodiscard]] size_t depth_of(const Node *node) const
  {
    return depth_of_id(id_of(node));
  }

  /** @brief Parent ID of node @p id, or `NONE` if root. */
  [[nodiscard]] size_t parent_id(const size_t id) const
  {
    topology_.validate_id(id, "Gen_Euler_RMQ_LCA::parent_id");
    return topology_.parent()(id);
  }

  /** @brief Parent of node, or `nullptr` if root. */
  [[nodiscard]] Node *parent_of(const Node *node) const
  {
    const size_t pid = parent_id(id_of(node));
    return pid == NONE ? nullptr : node_of(pid);
  }

  /** @brief Returns true if node @p u is an ancestor of @p v (or u == v). */
  [[nodiscard]] bool is_ancestor_id(const size_t u, const size_t v) const
  {
    return topology_.is_ancestor(u, v);
  }

  /** @brief Returns true if node @p u is an ancestor of @p v (or u == v). */
  [[nodiscard]] bool is_ancestor(const Node *u, const Node *v) const
  {
    return is_ancestor_id(id_of(u), id_of(v));
  }

  /** @brief LCA query by node ids in O(1). */
  [[nodiscard]] size_t lca_id(const size_t u, const size_t v) const
  {
    ensure_not_empty("Gen_Euler_RMQ_LCA::lca_id");
    topology_.validate_id(u, "Gen_Euler_RMQ_LCA::lca_id");
    topology_.validate_id(v, "Gen_Euler_RMQ_LCA::lca_id");

    size_t l = topology_.first()(u);
    size_t r = topology_.first()(v);
    if (l > r)
      std::swap(l, r);

    return rmq_.query(l, r).node;
  }

  /** @brief LCA query by node pointers in O(1). */
  [[nodiscard]] Node *lca(const Node *u, const Node *v) const
  {
    return node_of(lca_id(id_of(u), id_of(v)));
  }

  /** @brief Number of edges on the path between two ids in O(1). */
  [[nodiscard]] size_t distance_id(const size_t u, const size_t v) const
  {
    const size_t a = lca_id(u, v);
    return topology_.depth()(u) + topology_.depth()(v) - 2 * topology_.depth()(a);
  }

  /** @brief Number of edges on the path between two nodes in O(1). */
  [[nodiscard]] size_t distance(const Node *u, const Node *v) const
  {
    return distance_id(id_of(u), id_of(v));
  }

  /** @brief Euler tour sequence of node ids (@f$2n - 1@f$entries). */
  [[nodiscard]] const Array<size_t> &euler_tour() const noexcept
  {
    return topology_.euler();
  }

  /** @brief Euler tour size (@f$0@f$for empty tree, else@f$2n - 1@f$). */
  [[nodiscard]] size_t euler_tour_size() const noexcept
  {
    return topology_.euler_size();
  }
};

/** @brief Convenience alias for binary-lifting LCA. */
template <class GT, class SA = Dft_Show_Arc<GT>>
using Binary_Lifting_LCA = Gen_Binary_Lifting_LCA<GT, SA>;

/** @brief Convenience alias for Euler+RMQ LCA. */
template <class GT, class SA = Dft_Show_Arc<GT>>
using Euler_RMQ_LCA = Gen_Euler_RMQ_LCA<GT, SA>;
}  // namespace Aleph

#endif  // LCA_H
