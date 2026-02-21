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


/** @file HLD.H
 *  @brief Heavy-Light Decomposition for tree path queries on Aleph graphs.
 *
 *  Heavy-Light Decomposition (HLD) partitions a rooted tree into
 *  @e heavy chains such that any root-to-node path crosses at most
 *  @f$O(\log n)@f$ chains.  Combined with a segment tree, this yields
 *  efficient path queries and point updates.
 *
 *  @par Concept Visualization
 *  @code
 *         [0] (Root)          Heavy edges: 0-2, 2-5, 5-7
 *        /   \                Light edges: 0-1, 1-3, 1-4, 2-6, 5-8
 *      [1]   [2]
 *      / \   / \              path_query(3, 8) crosses O(log n) chains
 *    [3] [4][5] [6]           subtree_query(2) covers contiguous segment
 *          / \
 *        [7] [8]
 *  @endcode
 *
 *  This header provides a generic HLD engine parameterised by the
 *  graph type, value type, and an associative binary operation:
 *
 *  ### `Gen_HLD<GT, T, Op, SA>`
 *  - **Preprocessing**: @f$O(n \log n)@f$ time and space (segment tree build).
 *  - **Path query**: @f$O(\log^2 n)@f$ time.
 *  - **Subtree query**: @f$O(\log n)@f$ time.
 *  - **Point update**: @f$O(\log n)@f$ time.
 *  - **LCA query**: @f$O(\log n)@f$ time (free from chain walk).
 *
 *  Three convenience aliases are provided:
 *  - `HLD_Sum<GT, T, SA>` — path/subtree sum queries.
 *  - `HLD_Max<GT, T, SA>` — path/subtree max queries.
 *  - `HLD_Min<GT, T, SA>` — path/subtree min queries.
 *
 *  @note The binary operation @p Op must be **associative** and
 *  **commutative** for correct path queries.  Non-commutative operations
 *  require directional accumulation (not currently supported).
 *
 *  @par Example: Path sum queries
 *  @code
 *  using G = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
 *  G g;
 *  // ... build a tree in g ...
 *  auto node_val = [](G::Node * p) { return p->get_info(); };
 *  HLD_Sum<G, int> hld(g, root, node_val);
 *
 *  int s = hld.path_query(u, v);     // sum on path u..v
 *  hld.point_update(u, 42);          // set value of u to 42
 *  int sub = hld.subtree_query(v);   // sum of subtree rooted at v
 *  @endcode
 *
 *  @see tpl_segment_tree.H
 *  @see LCA.H
 *  @see tpl_graph.H
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

#ifndef HLD_H
#define HLD_H

# include <algorithm>
# include <cstddef>
# include <limits>
# include <utility>

# include <ah-errors.H>
# include <tpl_array.H>
# include <tpl_dynListStack.H>
# include <tpl_dynMapOhash.H>
# include <tpl_dynMapTree.H>
# include <tpl_graph.H>
# include <tpl_segment_tree.H>

namespace Aleph
{
  namespace hld_detail
  {
    /** @internal
     *  @brief Tree topology data for HLD with heavy-child-first DFS order.
     *
     *  Performs node indexing, adjacency validation, subtree size
     *  computation, heavy-child reordering, and a second DFS to assign
     *  flat-array positions such that each heavy chain maps to a
     *  contiguous segment.
     */
    template <class GT, class SA>
    class HLD_Tree_Data
    {
    public:
      using Node = typename GT::Node;
      using Arc = typename GT::Arc;

      static constexpr size_t NONE = std::numeric_limits<size_t>::max();

    private:
      using Pair_Key = std::pair<size_t, size_t>;

      const GT * graph_ = nullptr;
      SA sa_;
      Node * root_ = nullptr;
      size_t root_id_ = NONE;

      size_t n_ = 0;

      Array<Node *> id_to_node_;
      MapOLhash<Node *, size_t> node_to_id_;

      Array<Array<size_t>> adjacency_;
      Array<size_t> parent_;
      Array<size_t> depth_;
      Array<size_t> subtree_size_;

      // HLD-specific arrays
      Array<size_t> pos_;         // flat position in segment tree
      Array<size_t> chain_head_;  // head of the chain containing this node
      Array<size_t> tin_;         // entry time (same as pos_ for HLD)
      Array<size_t> tout_;        // exit time

      size_t num_chains_ = 0;

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
              << "HLD_Tree_Data: root node provided but graph is empty";
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
            Node * p = it.get_curr();
            id_to_node_(next_id) = p;
            node_to_id_.insert(p, next_id);
            ++next_id;
          }

        ah_runtime_error_unless(next_id == n_)
          << "HLD_Tree_Data: failed to index all graph nodes";

        if (root_ == nullptr)
          root_ = id_to_node_(0);

        ah_domain_error_if(node_to_id_.search(root_) == nullptr)
          << "HLD_Tree_Data: root node does not belong to graph";

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
            Arc * a = it.get_curr_ne();
            Node * src = graph_->get_src_node(a);
            Node * tgt = graph_->get_tgt_node(a);

            const auto * src_item = node_to_id_.search(src);
            const auto * tgt_item = node_to_id_.search(tgt);

            ah_runtime_error_unless(src_item != nullptr and tgt_item != nullptr)
              << "HLD_Tree_Data: arc endpoint is not indexed";

            const size_t u = src_item->second;
            const size_t v = tgt_item->second;

            ah_domain_error_if(u == v)
              << "HLD_Tree_Data: self-loop detected in filtered graph";

            const Pair_Key key = normalize_pair(u, v);
            ah_domain_error_if(unique_edges.search(key) != nullptr)
              << "HLD_Tree_Data: parallel/duplicate edge detected in filtered graph";

            unique_edges.insert(key, 1);
            ++edge_count;
          }

        ah_domain_error_if(edge_count != n_ - 1)
          << "HLD_Tree_Data: filtered graph is not a tree (expected "
          << (n_ - 1) << " edges, got " << edge_count << ")";

        for (typename DynMapTree<Pair_Key, char>::Iterator it(unique_edges);
             it.has_curr(); it.next_ne())
          {
            const auto & [fst, snd] = it.get_curr();
            const size_t u = fst.first;
            const size_t v = fst.second;
            adjacency_(u).append(v);
            adjacency_(v).append(u);
          }
      }

      void compute_sizes_and_parents()
      {
        if (n_ == 0)
          return;

        parent_ = Array<size_t>::create(n_);
        depth_ = Array<size_t>::create(n_);
        subtree_size_ = Array<size_t>::create(n_);

        for (size_t i = 0; i < n_; ++i)
          {
            parent_(i) = NONE;
            subtree_size_(i) = 1;
          }

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

        size_t visited_count = 1;
        visited(root_id_) = 1;
        depth_(root_id_) = 0;
        parent_(root_id_) = NONE;

        stack.push({root_id_, NONE, 0});

        while (not stack.is_empty())
          {
            auto & fr = stack.top();

            if (fr.next_child == adjacency_(fr.node).size())
              {
                // All children processed; accumulate size into parent
                (void) stack.pop();
                if (not stack.is_empty())
                  subtree_size_(stack.top().node) += subtree_size_(fr.node);
                continue;
              }

            const size_t nxt = adjacency_(fr.node)(fr.next_child++);
            if (nxt == fr.parent)
              continue;

            ah_domain_error_if(visited(nxt))
              << "HLD_Tree_Data: filtered graph is not acyclic";

            visited(nxt) = 1;
            ++visited_count;

            parent_(nxt) = fr.node;
            depth_(nxt) = depth_(fr.node) + 1;

            stack.push({nxt, fr.node, 0});
          }

        ah_domain_error_if(visited_count != n_)
          << "HLD_Tree_Data: filtered graph is not connected";
      }

      void reorder_adjacency_heavy_first()
      {
        if (n_ == 0)
          return;

        for (size_t u = 0; u < n_; ++u)
          {
            auto & adj = adjacency_(u);
            if (adj.size() <= 1)
              continue;

            // Find the child with the largest subtree_size
            size_t best_idx = NONE;
            size_t best_size = 0;

            for (size_t i = 0; i < adj.size(); ++i)
              {
                const size_t v = adj(i);
                if (v == parent_(u))
                  continue;
                if (subtree_size_(v) > best_size)
                  {
                    best_size = subtree_size_(v);
                    best_idx = i;
                  }
              }

            if (best_idx == NONE)
              continue;

            // Swap the heavy child to position 0 (or the first non-parent)
            // Find the first non-parent position
            size_t first_child_pos = 0;
            if (adj(0) == parent_(u))
              first_child_pos = 1;

            if (best_idx != first_child_pos)
              std::swap(adj(best_idx), adj(first_child_pos));
          }
      }

      void compute_hld_positions()
      {
        if (n_ == 0)
          return;

        pos_ = Array<size_t>::create(n_);
        chain_head_ = Array<size_t>::create(n_);
        tin_ = Array<size_t>::create(n_);
        tout_ = Array<size_t>::create(n_);

        struct Frame
        {
          size_t node;
          size_t parent;
          size_t next_child;
          size_t head;
        };

        DynListStack<Frame> stack;

        size_t timer = 0;
        num_chains_ = 0;

        // Root starts a new chain
        pos_(root_id_) = timer;
        tin_(root_id_) = timer;
        chain_head_(root_id_) = root_id_;
        ++timer;
        ++num_chains_;

        stack.push({root_id_, NONE, 0, root_id_});

        while (not stack.is_empty())
          {
            auto & fr = stack.top();

            if (fr.next_child == adjacency_(fr.node).size())
              {
                tout_(fr.node) = timer - 1;
                (void) stack.pop();
                continue;
              }

            const size_t nxt = adjacency_(fr.node)(fr.next_child);
            ++fr.next_child;

            if (nxt == fr.parent)
              continue;

            // Determine if this is a heavy or light child
            // The heavy child is the first non-parent child in adjacency
            // (after reordering)
            bool is_heavy = false;
            {
              const auto & adj = adjacency_(fr.node);
              size_t first_child_pos = 0;
              if (adj.size() > 0 and adj(0) == fr.parent)
                first_child_pos = 1;
              if (first_child_pos < adj.size() and adj(first_child_pos) == nxt)
                is_heavy = true;
            }

            if (is_heavy)
              {
                chain_head_(nxt) = fr.head;
              }
            else
              {
                chain_head_(nxt) = nxt;
                ++num_chains_;
              }

            pos_(nxt) = timer;
            tin_(nxt) = timer;
            ++timer;

            stack.push({nxt, fr.node, 0, chain_head_(nxt)});
          }
      }

      void check_id(const size_t id, const char * where) const
      {
        ah_out_of_range_error_if(id >= n_)
          << where << ": id=" << id << " is out of range [0, " << n_ << ")";
      }

    public:
      HLD_Tree_Data(const GT & g, Node * root, SA sa = SA())
        : graph_(&g), sa_(std::move(sa)), root_(root)
      {
        index_nodes();
        build_simple_adjacency();
        compute_sizes_and_parents();
        reorder_adjacency_heavy_first();
        compute_hld_positions();
      }

      [[nodiscard]] size_t size() const noexcept { return n_; }
      [[nodiscard]] bool is_empty() const noexcept { return n_ == 0; }
      [[nodiscard]] Node * root() const noexcept { return root_; }
      [[nodiscard]] size_t root_id() const noexcept { return root_id_; }

      [[nodiscard]] const Array<Node *> & id_to_node() const noexcept { return id_to_node_; }
      [[nodiscard]] const Array<size_t> & parent() const noexcept { return parent_; }
      [[nodiscard]] const Array<size_t> & depth() const noexcept { return depth_; }
      [[nodiscard]] const Array<size_t> & subtree_size() const noexcept { return subtree_size_; }
      [[nodiscard]] const Array<size_t> & pos() const noexcept { return pos_; }
      [[nodiscard]] const Array<size_t> & chain_head() const noexcept { return chain_head_; }
      [[nodiscard]] const Array<size_t> & tin() const noexcept { return tin_; }
      [[nodiscard]] const Array<size_t> & tout() const noexcept { return tout_; }
      [[nodiscard]] size_t num_chains() const noexcept { return num_chains_; }

      [[nodiscard]] size_t id_of(const Node * node) const
      {
        ah_invalid_argument_if(node == nullptr)
          << "HLD_Tree_Data::id_of: null node";

        const auto * item = node_to_id_.search(const_cast<Node *>(node));
        ah_domain_error_if(item == nullptr)
          << "HLD_Tree_Data::id_of: node does not belong to graph";

        return item->second;
      }

      [[nodiscard]] Node * node_of(const size_t id) const
      {
        check_id(id, "HLD_Tree_Data::node_of");
        return id_to_node_(id);
      }

      void validate_id(const size_t id, const char * where) const
      {
        check_id(id, where);
      }
    };
  } // namespace hld_detail


  /** @brief Heavy-Light Decomposition with segment tree for path/subtree queries.
   *
   *  Decomposes a rooted tree into heavy chains and maintains a segment tree
   *  over the flattened HLD order.  Supports path queries, subtree queries,
   *  point updates, LCA computation, and distance queries.
   *
   *  @tparam GT  Graph type (`List_Graph`, `List_SGraph`, `Array_Graph`).
   *  @tparam T   Value type stored at each node.
   *  @tparam Op  Associative and commutative binary functor over `T`.
   *  @tparam SA  Arc filter type (default: `Dft_Show_Arc<GT>`).
   *
   *  @par Complexity
   *  | Operation       | Time               |
   *  |----------------|--------------------|
   *  | Construction    | O(n log n)          |
   *  | path_query      | O(log^2 n)          |
   *  | subtree_query   | O(log n)            |
   *  | point_update    | O(log n)            |
   *  | lca             | O(log n)            |
   *  | distance        | O(log n)            |
   *
   *  @ingroup Graphs
   */
  template <class GT, typename T, class Op, class SA = Dft_Show_Arc<GT>>
    requires SegmentTreeOp<Op, T>
  class Gen_HLD
  {
  public:
    using Node = typename GT::Node;

  private:
    using Topology = hld_detail::HLD_Tree_Data<GT, SA>;
    static constexpr size_t NONE = Topology::NONE;

    Topology topology_;
    T identity_;
    Op op_;
    Gen_Segment_Tree<T, Op> seg_;

    [[nodiscard]] size_t n() const noexcept { return topology_.size(); }

    void ensure_not_empty(const char * where) const
    {
      ah_domain_error_if(is_empty()) << where << ": tree is empty";
    }

    /** @internal Build the segment tree from HLD-ordered node values. */
    template <class NodeValueFn>
    void build_segment_tree(NodeValueFn && node_value)
    {
      if (is_empty())
        return;

      auto flat = Array<T>::create(n());
      for (size_t i = 0; i < n(); ++i)
        flat(i) = identity_;

      for (size_t id = 0; id < n(); ++id)
        {
          const size_t p = topology_.pos()(id);
          flat(p) = node_value(topology_.id_to_node()(id));
        }

      Gen_Segment_Tree<T, Op> real_seg(flat, identity_, op_);
      seg_.swap(real_seg);
    }

  public:
    /** @brief Construct HLD from graph, root, and node value functor.
     *
     *  @param[in] g          Tree topology (filtered by @p sa).
     *  @param[in] root       Root node pointer (must belong to @p g).
     *  @param[in] node_value Callable `Node* -> T` providing node values.
     *  @param[in] identity   Identity element of the monoid `(T, Op)`.
     *  @param[in] oper       Associative binary functor.
     *  @param[in] sa         Arc filter.
     *  @throw ah_domain_error if the filtered graph is not a tree.
     */
    template <class NodeValueFn>
    Gen_HLD(const GT & g, Node * root, NodeValueFn && node_value,
            const T & identity, Op oper = Op(), SA sa = SA())
      : topology_(g, root, std::move(sa)),
        identity_(identity),
        op_(oper),
        seg_(1, identity, identity, oper)
    {
      build_segment_tree(std::forward<NodeValueFn>(node_value));
    }

    /** @brief Construct with first graph node as root. */
    template <class NodeValueFn>
    Gen_HLD(const GT & g, NodeValueFn && node_value,
            const T & identity, Op oper = Op(), SA sa = SA())
      : topology_(g, nullptr, std::move(sa)),
        identity_(identity),
        op_(oper),
        seg_(1, identity, identity, oper)
    {
      build_segment_tree(std::forward<NodeValueFn>(node_value));
    }

    /** @brief Total number of nodes in the tree. */
    [[nodiscard]] size_t size() const noexcept { return topology_.size(); }

    /** @brief Returns true if the tree has no nodes. */
    [[nodiscard]] bool is_empty() const noexcept { return topology_.is_empty(); }

    /** @brief Returns the root node. */
    [[nodiscard]] Node * root() const noexcept { return topology_.root(); }

    /** @brief Returns the internal ID of the root node. */
    [[nodiscard]] size_t root_id() const noexcept { return topology_.root_id(); }

    /** @brief Map an internal ID back to a Node pointer. */
    [[nodiscard]] Node * node_of(const size_t id) const
    {
      return topology_.node_of(id);
    }

    /** @brief Map a Node pointer to its internal ID. */
    [[nodiscard]] size_t id_of(const Node * node) const
    {
      return topology_.id_of(node);
    }

    /** @brief HLD flat-array position of node @p id. */
    [[nodiscard]] size_t position(const size_t id) const
    {
      topology_.validate_id(id, "Gen_HLD::position");
      return topology_.pos()(id);
    }

    /** @brief Head of the chain containing node @p id. */
    [[nodiscard]] size_t chain_head_id(const size_t id) const
    {
      topology_.validate_id(id, "Gen_HLD::chain_head_id");
      return topology_.chain_head()(id);
    }

    /** @brief Depth of node @p id from root. */
    [[nodiscard]] size_t depth_of_id(const size_t id) const
    {
      topology_.validate_id(id, "Gen_HLD::depth_of_id");
      return topology_.depth()(id);
    }

    /** @brief Depth of node from root. */
    [[nodiscard]] size_t depth_of(const Node * node) const
    {
      return depth_of_id(id_of(node));
    }

    /** @brief Subtree size of node @p id. */
    [[nodiscard]] size_t subtree_size_of_id(const size_t id) const
    {
      topology_.validate_id(id, "Gen_HLD::subtree_size_of_id");
      return topology_.subtree_size()(id);
    }

    /** @brief Subtree size of node. */
    [[nodiscard]] size_t subtree_size_of(const Node * node) const
    {
      return subtree_size_of_id(id_of(node));
    }

    /** @brief Parent ID of node @p id, or `NONE` if root. */
    [[nodiscard]] size_t parent_id(const size_t id) const
    {
      topology_.validate_id(id, "Gen_HLD::parent_id");
      return topology_.parent()(id);
    }

    /** @brief Parent of node, or `nullptr` if root. */
    [[nodiscard]] Node * parent_of(const Node * node) const
    {
      const size_t pid = parent_id(id_of(node));
      return pid == NONE ? nullptr : node_of(pid);
    }

    /** @brief Number of heavy chains in the decomposition. */
    [[nodiscard]] size_t num_chains() const noexcept
    {
      return topology_.num_chains();
    }

    // ------------------------------------------------------------------
    // Path queries
    // ------------------------------------------------------------------

    /** @brief Path query by node IDs in O(log^2 n).
     *
     *  Combines all node values on the path from @p u to @p v
     *  using the binary operation @p Op.
     *
     *  @param[in] u Source node ID.
     *  @param[in] v Target node ID.
     *  @return Combined value along the path.
     */
    [[nodiscard]] T path_query_id(size_t u, size_t v) const
    {
      ensure_not_empty("Gen_HLD::path_query_id");
      topology_.validate_id(u, "Gen_HLD::path_query_id");
      topology_.validate_id(v, "Gen_HLD::path_query_id");

      T result = identity_;

      while (topology_.chain_head()(u) != topology_.chain_head()(v))
        {
          // Move the deeper chain head up
          if (topology_.depth()(topology_.chain_head()(u)) <
              topology_.depth()(topology_.chain_head()(v)))
            std::swap(u, v);

          // Query the segment from chain_head(u) to u
          const size_t head = topology_.chain_head()(u);
          result = op_(result, seg_.query(topology_.pos()(head),
                                          topology_.pos()(u)));
          u = topology_.parent()(head);
        }

      // u and v are now on the same chain
      if (topology_.depth()(u) > topology_.depth()(v))
        std::swap(u, v);

      result = op_(result, seg_.query(topology_.pos()(u),
                                      topology_.pos()(v)));
      return result;
    }

    /** @brief Path query by node pointers in O(log^2 n). */
    [[nodiscard]] T path_query(const Node * u, const Node * v) const
    {
      return path_query_id(id_of(u), id_of(v));
    }

    /** @brief Edge-weighted path query by node IDs in O(log^2 n).
     *
     *  Like path_query_id but skips the LCA node value, useful when
     *  values represent edges (stored on the child node).
     */
    [[nodiscard]] T path_query_edges_id(size_t u, size_t v) const
    {
      ensure_not_empty("Gen_HLD::path_query_edges_id");
      topology_.validate_id(u, "Gen_HLD::path_query_edges_id");
      topology_.validate_id(v, "Gen_HLD::path_query_edges_id");

      T result = identity_;

      while (topology_.chain_head()(u) != topology_.chain_head()(v))
        {
          if (topology_.depth()(topology_.chain_head()(u)) <
              topology_.depth()(topology_.chain_head()(v)))
            std::swap(u, v);

          const size_t head = topology_.chain_head()(u);
          result = op_(result, seg_.query(topology_.pos()(head),
                                          topology_.pos()(u)));
          u = topology_.parent()(head);
        }

      if (topology_.depth()(u) > topology_.depth()(v))
        std::swap(u, v);

      // Skip the LCA node (u) for edge-weighted queries
      if (u != v)
        result = op_(result, seg_.query(topology_.pos()(u) + 1,
                                        topology_.pos()(v)));

      return result;
    }

    /** @brief Edge-weighted path query by node pointers. */
    [[nodiscard]] T path_query_edges(const Node * u, const Node * v) const
    {
      return path_query_edges_id(id_of(u), id_of(v));
    }

    // ------------------------------------------------------------------
    // Subtree queries
    // ------------------------------------------------------------------

    /** @brief Subtree query by node ID in O(log n).
     *
     *  Combines all values in the subtree rooted at @p v.
     */
    [[nodiscard]] T subtree_query_id(const size_t v) const
    {
      ensure_not_empty("Gen_HLD::subtree_query_id");
      topology_.validate_id(v, "Gen_HLD::subtree_query_id");

      const size_t l = topology_.pos()(v);
      const size_t r = l + topology_.subtree_size()(v) - 1;
      return seg_.query(l, r);
    }

    /** @brief Subtree query by node pointer in O(log n). */
    [[nodiscard]] T subtree_query(const Node * v) const
    {
      return subtree_query_id(id_of(v));
    }

    // ------------------------------------------------------------------
    // Point update
    // ------------------------------------------------------------------

    /** @brief Point update by node ID: set value to @p new_value.
     *
     *  @param[in] v         Node ID.
     *  @param[in] new_value New value for the node.
     */
    void point_update_id(const size_t v, const T & new_value)
    {
      ensure_not_empty("Gen_HLD::point_update_id");
      topology_.validate_id(v, "Gen_HLD::point_update_id");

      seg_.set(topology_.pos()(v), new_value);
    }

    /** @brief Point update by node pointer: set value to @p new_value. */
    void point_update(const Node * v, const T & new_value)
    {
      point_update_id(id_of(v), new_value);
    }

    // ------------------------------------------------------------------
    // LCA queries (free from chain walk)
    // ------------------------------------------------------------------

    /** @brief LCA query by node IDs in O(log n). */
    [[nodiscard]] size_t lca_id(size_t u, size_t v) const
    {
      ensure_not_empty("Gen_HLD::lca_id");
      topology_.validate_id(u, "Gen_HLD::lca_id");
      topology_.validate_id(v, "Gen_HLD::lca_id");

      while (topology_.chain_head()(u) != topology_.chain_head()(v))
        {
          if (topology_.depth()(topology_.chain_head()(u)) <
              topology_.depth()(topology_.chain_head()(v)))
            std::swap(u, v);

          u = topology_.parent()(topology_.chain_head()(u));
        }

      return topology_.depth()(u) <= topology_.depth()(v) ? u : v;
    }

    /** @brief LCA query by node pointers in O(log n). */
    [[nodiscard]] Node * lca(const Node * u, const Node * v) const
    {
      return node_of(lca_id(id_of(u), id_of(v)));
    }

    // ------------------------------------------------------------------
    // Distance queries
    // ------------------------------------------------------------------

    /** @brief Number of edges between two nodes by IDs in O(log n). */
    [[nodiscard]] size_t distance_id(const size_t u, const size_t v) const
    {
      const size_t a = lca_id(u, v);
      return topology_.depth()(u) + topology_.depth()(v)
             - 2 * topology_.depth()(a);
    }

    /** @brief Number of edges between two nodes in O(log n). */
    [[nodiscard]] size_t distance(const Node * u, const Node * v) const
    {
      return distance_id(id_of(u), id_of(v));
    }

    // ------------------------------------------------------------------
    // Decomposition accessors (for power users)
    // ------------------------------------------------------------------

    /** @brief Read-only access to the flat-position array.
     *
     *  `position_array()(id)` gives the segment-tree index of node @p id.
     */
    [[nodiscard]] const Array<size_t> & position_array() const noexcept
    {
      return topology_.pos();
    }

    /** @brief Read-only access to the chain-head array.
     *
     *  `chain_head_array()(id)` gives the ID of the head of the chain
     *  containing node @p id.
     */
    [[nodiscard]] const Array<size_t> & chain_head_array() const noexcept
    {
      return topology_.chain_head();
    }

    /** @brief Read-only access to the depth array. */
    [[nodiscard]] const Array<size_t> & depth_array() const noexcept
    {
      return topology_.depth();
    }

    /** @brief Read-only access to the subtree-size array. */
    [[nodiscard]] const Array<size_t> & subtree_size_array() const noexcept
    {
      return topology_.subtree_size();
    }

    /** @brief Read-only access to the parent array. */
    [[nodiscard]] const Array<size_t> & parent_array() const noexcept
    {
      return topology_.parent();
    }

    /** @brief Current node value at HLD position @p pos in the segment tree. */
    [[nodiscard]] T get_value_at_id(const size_t id) const
    {
      topology_.validate_id(id, "Gen_HLD::get_value_at_id");
      return seg_.get(topology_.pos()(id));
    }

    /** @brief Current node value at node pointer @p v. */
    [[nodiscard]] T get_value(const Node * v) const
    {
      return get_value_at_id(id_of(v));
    }
  };


  // ====================================================================
  // Convenience aliases
  // ====================================================================

  /** @brief HLD with path/subtree sum queries.
   *
   *  @tparam GT Graph type.
   *  @tparam T  Arithmetic value type (identity = `T()`).
   *  @tparam SA Arc filter type.
   *
   *  @ingroup Graphs
   */
  template <class GT, typename T, class SA = Dft_Show_Arc<GT>>
  struct HLD_Sum : public Gen_HLD<GT, T, Aleph::plus<T>, SA>
  {
    using Base = Gen_HLD<GT, T, Aleph::plus<T>, SA>;
    using Node = typename GT::Node;

    template <class NodeValueFn>
    HLD_Sum(const GT & g, Node * root, NodeValueFn && nv, SA sa = SA())
      : Base(g, root, std::forward<NodeValueFn>(nv),
             T(), Aleph::plus<T>(), std::move(sa))
    {}

    template <class NodeValueFn>
    HLD_Sum(const GT & g, NodeValueFn && nv, SA sa = SA())
      : Base(g, std::forward<NodeValueFn>(nv),
             T(), Aleph::plus<T>(), std::move(sa))
    {}
  };

  /** @brief HLD with path/subtree max queries.
   *
   *  @tparam GT Graph type.
   *  @tparam T  Totally ordered value type (identity = `lowest()`).
   *  @tparam SA Arc filter type.
   *
   *  @ingroup Graphs
   */
  template <class GT, typename T, class SA = Dft_Show_Arc<GT>>
  struct HLD_Max : public Gen_HLD<GT, T, Max_Op<T>, SA>
  {
    using Base = Gen_HLD<GT, T, Max_Op<T>, SA>;
    using Node = typename GT::Node;

    template <class NodeValueFn>
    HLD_Max(const GT & g, Node * root, NodeValueFn && nv, SA sa = SA())
      : Base(g, root, std::forward<NodeValueFn>(nv),
             std::numeric_limits<T>::lowest(), Max_Op<T>(), std::move(sa))
    {}

    template <class NodeValueFn>
    HLD_Max(const GT & g, NodeValueFn && nv, SA sa = SA())
      : Base(g, std::forward<NodeValueFn>(nv),
             std::numeric_limits<T>::lowest(), Max_Op<T>(), std::move(sa))
    {}
  };

  /** @brief HLD with path/subtree min queries.
   *
   *  @tparam GT Graph type.
   *  @tparam T  Totally ordered value type (identity = `max()`).
   *  @tparam SA Arc filter type.
   *
   *  @ingroup Graphs
   */
  template <class GT, typename T, class SA = Dft_Show_Arc<GT>>
  struct HLD_Min : public Gen_HLD<GT, T, Min_Op<T>, SA>
  {
    using Base = Gen_HLD<GT, T, Min_Op<T>, SA>;
    using Node = typename GT::Node;

    template <class NodeValueFn>
    HLD_Min(const GT & g, Node * root, NodeValueFn && nv, SA sa = SA())
      : Base(g, root, std::forward<NodeValueFn>(nv),
             std::numeric_limits<T>::max(), Min_Op<T>(), std::move(sa))
    {}

    template <class NodeValueFn>
    HLD_Min(const GT & g, NodeValueFn && nv, SA sa = SA())
      : Base(g, std::forward<NodeValueFn>(nv),
             std::numeric_limits<T>::max(), Min_Op<T>(), std::move(sa))
    {}
  };

} // namespace Aleph

#endif // HLD_H
