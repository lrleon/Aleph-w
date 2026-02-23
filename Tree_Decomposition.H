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


/** @file Tree_Decomposition.H
 *  @brief Heavy-Light and Centroid decomposition on rooted trees represented as Aleph graphs.
 *
 *  This header provides two complementary tree decomposition engines:
 *
 *  1. **Heavy-Light Decomposition (HLD)**
 *     - `Gen_Heavy_Light_Decomposition<GT, SA>`
 *     - `Gen_HLD_Path_Query<GT, T, Op, SA>`
 *
 *     HLD maps each tree node to a position in a base array. Any path query
 *     becomes a small number of contiguous segments (O(log n) segments), which
 *     can be aggregated with a range-query structure such as `Gen_Segment_Tree`.
 *
 *  2. **Centroid Decomposition**
 *     - `Gen_Centroid_Decomposition<GT, SA>`
 *
 *     Centroid decomposition builds a centroid tree in O(n log n), exposing,
 *     for each node, its chain of centroid ancestors and the distance to each
 *     ancestor. This is ideal for dynamic nearest/farthest marked-node queries,
 *     distance-constrained updates, and other "niche but powerful" tree tricks.
 *
 *  ## Problem-oriented view
 *
 *  Use this header when your tree workload is one of these:
 *
 *  - **Online path aggregates with node updates**:
 *    many queries of the form `query(path(u, v))` plus point updates.
 *    Typical examples: path risk score, path maintenance sum, path min/max.
 *
 *  - **Online subtree aggregates with node updates**:
 *    many queries of the form `query(subtree(u))` plus point updates.
 *    Typical examples: total load/cost under a supervisor/rooted region.
 *
 *  - **Distance to dynamic marked set** (unweighted edge distance):
 *    repeatedly mark/activate nodes and query nearest/farthest/threshold
 *    distance from arbitrary nodes. Centroid chains are the natural primitive.
 *
 *  ## Which engine should you choose?
 *
 *  - Choose **HLD** when the central operation is an associative fold over
 *    tree paths/subtrees and you want a segment-tree style interface.
 *  - Choose **Centroid decomposition** when the central operation is distance
 *    to a dynamic set of marked nodes.
 *  - If you only need ancestor/LCA/distance queries, prefer `LCA.H`.
 *  - If queries are fully offline and not monoidal, consider `tpl_mo_on_trees.H`.
 *
 *  ## Notes and limits
 *
 *  - Distances exposed by centroid annotations are measured in **number of edges**.
 *  - Topology validation is strict and performed at construction time.
 *  - For range updates on paths/subtrees, reuse HLD decomposition with a lazy
 *    segment tree policy.
 *
 *  ## Graph support
 *
 *  Both engines work on Aleph graph backends:
 *  - `List_Graph`
 *  - `List_SGraph`
 *  - `Array_Graph`
 *
 *  and accept an optional arc filter `SA` to project a tree subgraph from a
 *  larger graph.
 *
 *  ## Validation guarantees
 *
 *  Construction validates that the filtered graph is a **simple undirected tree**:
 *  - no self-loops
 *  - no parallel edges
 *  - exactly `n-1` edges
 *  - connected and acyclic
 *
 *  If validation fails, a domain error is raised with an explanatory message.
 *
 *  @see tpl_segment_tree.H
 *  @see LCA.H
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef TREE_DECOMPOSITION_H
# define TREE_DECOMPOSITION_H

# include <algorithm>
# include <concepts>
# include <cstddef>
# include <limits>
# include <type_traits>
# include <utility>

# include <ah-errors.H>
# include <ahFunction.H>
# include <tpl_array.H>
# include <tpl_dynListStack.H>
# include <tpl_dynMapOhash.H>
# include <tpl_dynMapTree.H>
# include <tpl_graph.H>
# include <tpl_segment_tree.H>

namespace Aleph
{
  namespace tree_decomposition_detail
  {
    /** @internal
     *  @brief Canonical half-open range in the HLD base array.
     */
    struct Id_Range
    {
      size_t left = 0;
      size_t right = 0;
    };

    /** @internal
     *  @brief Shared rooted-tree topology builder/validator for decomposition engines.
     *
     *  Responsibilities:
     *  - map graph nodes to contiguous IDs [0, n-1]
     *  - build simple undirected adjacency from filtered arcs
     *  - validate the filtered graph is a tree
     */
    template <class GT, class SA>
    class Rooted_Tree_Topology
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

      static Pair_Key normalize_pair(size_t u, size_t v) noexcept
      {
        if (u > v)
          std::swap(u, v);
        return std::make_pair(u, v);
      }

      void check_id(const size_t id, const char * where) const
      {
        ah_out_of_range_error_if(id >= n_)
          << where << ": id=" << id << " is out of range [0, " << n_ << ")";
      }

      void index_nodes()
      {
        n_ = graph_->get_num_nodes();

        if (n_ == 0)
          {
            ah_domain_error_if(root_ != nullptr)
              << "Rooted_Tree_Topology: root node provided but graph is empty";
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
          << "Rooted_Tree_Topology: failed to index all graph nodes";

        if (root_ == nullptr)
          root_ = id_to_node_(0);

        ah_domain_error_if(node_to_id_.search(root_) == nullptr)
          << "Rooted_Tree_Topology: root node does not belong to graph";

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
              << "Rooted_Tree_Topology: arc endpoint is not indexed";

            const size_t u = src_item->second;
            const size_t v = tgt_item->second;

            ah_domain_error_if(u == v)
              << "Rooted_Tree_Topology: self-loop detected in filtered graph";

            const Pair_Key key = normalize_pair(u, v);
            ah_domain_error_if(unique_edges.search(key) != nullptr)
              << "Rooted_Tree_Topology: parallel/duplicate edge detected in filtered graph";

            unique_edges.insert(key, 1);
            ++edge_count;
          }

        ah_domain_error_if(edge_count != n_ - 1)
          << "Rooted_Tree_Topology: filtered graph is not a tree (expected "
          << (n_ - 1) << " edges, got " << edge_count << ")";

        for (typename DynMapTree<Pair_Key, char>::Iterator it(unique_edges);
             it.has_curr(); it.next_ne())
          {
            const auto & [fst, snd] = it.get_curr();
            (void) snd;

            const size_t u = fst.first;
            const size_t v = fst.second;
            adjacency_(u).append(v);
            adjacency_(v).append(u);
          }
      }

      void validate_connected_acyclic() const
      {
        if (n_ == 0)
          return;

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
        visited(root_id_) = 1;
        size_t visited_count = 1;
        stack.push({root_id_, NONE, 0});

        while (not stack.is_empty())
          {
            auto & fr = stack.top();

            if (fr.next_child == adjacency_(fr.node).size())
              {
                (void) stack.pop();
                continue;
              }

            const size_t nxt = adjacency_(fr.node)(fr.next_child++);
            if (nxt == fr.parent)
              continue;

            ah_domain_error_if(visited(nxt))
              << "Rooted_Tree_Topology: filtered graph is not acyclic";

            visited(nxt) = 1;
            ++visited_count;
            stack.push({nxt, fr.node, 0});
          }

        ah_domain_error_if(visited_count != n_)
          << "Rooted_Tree_Topology: filtered graph is not connected";
      }

    public:
      Rooted_Tree_Topology(const GT & g, Node * root, SA sa = SA())
        : graph_(&g), sa_(std::move(sa)), root_(root)
      {
        index_nodes();
        build_simple_adjacency();
        validate_connected_acyclic();
      }

      [[nodiscard]] size_t size() const noexcept { return n_; }
      [[nodiscard]] bool is_empty() const noexcept { return n_ == 0; }
      [[nodiscard]] Node * root() const noexcept { return root_; }
      [[nodiscard]] size_t root_id() const noexcept { return root_id_; }

      [[nodiscard]] const Array<Node *> & id_to_node() const noexcept
      {
        return id_to_node_;
      }

      [[nodiscard]] const Array<Array<size_t>> & adjacency() const noexcept
      {
        return adjacency_;
      }

      [[nodiscard]] size_t id_of(const Node * node) const
      {
        ah_invalid_argument_if(node == nullptr)
          << "Rooted_Tree_Topology::id_of: null node";

        const auto * item = node_to_id_.search(const_cast<Node *>(node));
        ah_domain_error_if(item == nullptr)
          << "Rooted_Tree_Topology::id_of: node does not belong to graph";

        return item->second;
      }

      [[nodiscard]] Node * node_of(const size_t id) const
      {
        check_id(id, "Rooted_Tree_Topology::node_of");
        return id_to_node_(id);
      }

      void validate_id(const size_t id, const char * where) const
      {
        check_id(id, where);
      }
    };
  } // namespace tree_decomposition_detail


  /** @brief Heavy-Light Decomposition over a rooted tree represented as an Aleph graph.
   *
   *  HLD decomposes each root-to-leaf path into heavy and light edges so that
   *  any path between two nodes can be split into O(log n) contiguous segments
   *  over a base array.
   *
   *  Build complexity: O(n)
   *  Path decomposition complexity: O(log n) segments
   *
   *  @tparam GT Graph type.
   *  @tparam SA Arc filter type.
   *
   *  @ingroup Graphs
   */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  class Gen_Heavy_Light_Decomposition
  {
  public:
    using Node = typename GT::Node;
    using Range = tree_decomposition_detail::Id_Range;

  private:
    using Topology = tree_decomposition_detail::Rooted_Tree_Topology<GT, SA>;
    static constexpr size_t NONE = Topology::NONE;

    Topology topology_;

    Array<size_t> parent_;
    Array<size_t> depth_;
    Array<size_t> subtree_size_;
    Array<size_t> heavy_;
    Array<size_t> head_;
    Array<size_t> pos_;
    Array<size_t> inv_pos_;

    void ensure_not_empty(const char * where) const
    {
      ah_domain_error_if(is_empty()) << where << ": tree is empty";
    }

    void build_decomposition()
    {
      const size_t n = size();
      if (n == 0)
        return;

      parent_ = Array<size_t>::create(n);
      depth_ = Array<size_t>::create(n);
      subtree_size_ = Array<size_t>::create(n);
      heavy_ = Array<size_t>::create(n);
      head_ = Array<size_t>::create(n);
      pos_ = Array<size_t>::create(n);
      inv_pos_ = Array<size_t>::create(n);

      for (size_t i = 0; i < n; ++i)
        {
          parent_(i) = NONE;
          depth_(i) = 0;
          subtree_size_(i) = 0;
          heavy_(i) = NONE;
          head_(i) = NONE;
          pos_(i) = NONE;
          inv_pos_(i) = NONE;
        }

      Array<char> visited = Array<char>::create(n);
      for (size_t i = 0; i < n; ++i)
        visited(i) = 0;

      Array<size_t> order;
      order.reserve(n);

      struct Frame
      {
        size_t node;
        size_t parent;
        size_t next_child;
      };

      const size_t root_id = topology_.root_id();
      parent_(root_id) = NONE;
      depth_(root_id) = 0;
      visited(root_id) = 1;

      size_t visited_count = 1;
      order.append(root_id);

      DynListStack<Frame> stack;
      stack.push({root_id, NONE, 0});

      while (not stack.is_empty())
        {
          auto & fr = stack.top();

          if (fr.next_child == topology_.adjacency()(fr.node).size())
            {
              (void) stack.pop();
              continue;
            }

          const size_t nxt = topology_.adjacency()(fr.node)(fr.next_child++);
          if (nxt == fr.parent)
            continue;

          ah_domain_error_if(visited(nxt))
            << "Gen_Heavy_Light_Decomposition: filtered graph is not acyclic";

          visited(nxt) = 1;
          ++visited_count;

          parent_(nxt) = fr.node;
          depth_(nxt) = depth_(fr.node) + 1;
          order.append(nxt);

          stack.push({nxt, fr.node, 0});
        }

      ah_domain_error_if(visited_count != n)
        << "Gen_Heavy_Light_Decomposition: filtered graph is not connected";

      for (size_t i = order.size(); i-- > 0;)
        {
          const size_t u = order(i);
          subtree_size_(u) = 1;
          heavy_(u) = NONE;

          for (size_t j = 0; j < topology_.adjacency()(u).size(); ++j)
            {
              const size_t v = topology_.adjacency()(u)(j);
              if (parent_(v) != u)
                continue;

              subtree_size_(u) += subtree_size_(v);
              if (heavy_(u) == NONE or subtree_size_(v) > subtree_size_(heavy_(u)))
                heavy_(u) = v;
            }
        }

      size_t next_pos = 0;
      DynListStack<std::pair<size_t, size_t>> chain_stack;
      chain_stack.push({root_id, root_id});

      while (not chain_stack.is_empty())
        {
          const auto [chain_head, chain_start] = chain_stack.pop();

          size_t u = chain_start;
          while (u != NONE)
            {
              head_(u) = chain_head;
              pos_(u) = next_pos;
              inv_pos_(next_pos) = u;
              ++next_pos;

              for (size_t j = 0; j < topology_.adjacency()(u).size(); ++j)
                {
                  const size_t v = topology_.adjacency()(u)(j);
                  if (parent_(v) == u and v != heavy_(u))
                    chain_stack.push({v, v});
                }

              u = heavy_(u);
            }
        }

      ah_runtime_error_unless(next_pos == n)
        << "Gen_Heavy_Light_Decomposition: invalid decomposition size";
    }

  public:
    /** @brief Construct using an explicit root node.
     *
     *  @param[in] g Graph containing the rooted tree.
     *  @param[in] root Root node pointer.
     *  @param[in] sa Arc filter.
     */
    Gen_Heavy_Light_Decomposition(const GT & g, Node * root, SA sa = SA())
      : topology_(g, root, std::move(sa))
    {
      build_decomposition();
    }

    /** @brief Construct using the first graph node as root.
     *
     *  @param[in] g Graph containing the rooted tree.
     *  @param[in] sa Arc filter.
     */
    Gen_Heavy_Light_Decomposition(const GT & g, SA sa = SA())
      : topology_(g, nullptr, std::move(sa))
    {
      build_decomposition();
    }

    [[nodiscard]] size_t size() const noexcept { return topology_.size(); }
    [[nodiscard]] bool is_empty() const noexcept { return topology_.is_empty(); }

    [[nodiscard]] Node * root() const noexcept { return topology_.root(); }
    [[nodiscard]] size_t root_id() const noexcept { return topology_.root_id(); }

    [[nodiscard]] Node * node_of(const size_t id) const
    {
      return topology_.node_of(id);
    }

    [[nodiscard]] size_t id_of(const Node * node) const
    {
      return topology_.id_of(node);
    }

    [[nodiscard]] size_t parent_id(const size_t id) const
    {
      topology_.validate_id(id, "Gen_Heavy_Light_Decomposition::parent_id");
      return parent_(id);
    }

    [[nodiscard]] Node * parent_of(const Node * node) const
    {
      const size_t pid = parent_id(id_of(node));
      return pid == NONE ? nullptr : node_of(pid);
    }

    [[nodiscard]] size_t depth_of_id(const size_t id) const
    {
      topology_.validate_id(id, "Gen_Heavy_Light_Decomposition::depth_of_id");
      return depth_(id);
    }

    [[nodiscard]] size_t depth_of(const Node * node) const
    {
      return depth_of_id(id_of(node));
    }

    [[nodiscard]] size_t subtree_size_of_id(const size_t id) const
    {
      topology_.validate_id(id, "Gen_Heavy_Light_Decomposition::subtree_size_of_id");
      return subtree_size_(id);
    }

    [[nodiscard]] size_t subtree_size_of(const Node * node) const
    {
      return subtree_size_of_id(id_of(node));
    }

    [[nodiscard]] size_t heavy_child_id(const size_t id) const
    {
      topology_.validate_id(id, "Gen_Heavy_Light_Decomposition::heavy_child_id");
      return heavy_(id);
    }

    [[nodiscard]] Node * heavy_child(const Node * node) const
    {
      const size_t cid = heavy_child_id(id_of(node));
      return cid == NONE ? nullptr : node_of(cid);
    }

    [[nodiscard]] size_t head_id(const size_t id) const
    {
      topology_.validate_id(id, "Gen_Heavy_Light_Decomposition::head_id");
      return head_(id);
    }

    [[nodiscard]] Node * head_of(const Node * node) const
    {
      return node_of(head_id(id_of(node)));
    }

    [[nodiscard]] size_t position_of_id(const size_t id) const
    {
      topology_.validate_id(id, "Gen_Heavy_Light_Decomposition::position_of_id");
      return pos_(id);
    }

    [[nodiscard]] size_t position_of(const Node * node) const
    {
      return position_of_id(id_of(node));
    }

    /** @brief Base-array range of a full subtree (inclusive endpoints). */
    [[nodiscard]] Range subtree_range_id(const size_t id) const
    {
      topology_.validate_id(id, "Gen_Heavy_Light_Decomposition::subtree_range_id");
      const size_t l = pos_(id);
      const size_t r = l + subtree_size_(id) - 1;
      return Range{l, r};
    }

    /** @brief Base-array range of a full subtree (inclusive endpoints). */
    [[nodiscard]] Range subtree_range(const Node * node) const
    {
      return subtree_range_id(id_of(node));
    }

    /** @brief True iff `u` is ancestor of `v` in the rooted tree. */
    [[nodiscard]] bool is_ancestor_id(const size_t u, const size_t v) const
    {
      const Range r = subtree_range_id(u);
      const size_t pv = position_of_id(v);
      return r.left <= pv and pv <= r.right;
    }

    /** @brief True iff `u` is ancestor of `v` in the rooted tree. */
    [[nodiscard]] bool is_ancestor(const Node * u, const Node * v) const
    {
      return is_ancestor_id(id_of(u), id_of(v));
    }

    /** @brief Lowest common ancestor in O(log n). */
    [[nodiscard]] size_t lca_id(size_t u, size_t v) const
    {
      ensure_not_empty("Gen_Heavy_Light_Decomposition::lca_id");
      topology_.validate_id(u, "Gen_Heavy_Light_Decomposition::lca_id");
      topology_.validate_id(v, "Gen_Heavy_Light_Decomposition::lca_id");

      while (head_(u) != head_(v))
        {
          if (depth_(head_(u)) >= depth_(head_(v)))
            u = parent_(head_(u));
          else
            v = parent_(head_(v));
        }

      return (depth_(u) <= depth_(v)) ? u : v;
    }

    /** @brief Lowest common ancestor in O(log n). */
    [[nodiscard]] Node * lca(const Node * u, const Node * v) const
    {
      return node_of(lca_id(id_of(u), id_of(v)));
    }

    /** @brief Distance (number of edges) between two node IDs. */
    [[nodiscard]] size_t distance_id(const size_t u, const size_t v) const
    {
      const size_t a = lca_id(u, v);
      return depth_(u) + depth_(v) - 2 * depth_(a);
    }

    /** @brief Distance (number of edges) between two nodes. */
    [[nodiscard]] size_t distance(const Node * u, const Node * v) const
    {
      return distance_id(id_of(u), id_of(v));
    }

    /** @brief Enumerate path segments from `u` to `v` in path order.
     *
     *  The callback receives `(l, r, reversed)` where `[l, r]` is a contiguous
     *  segment in the HLD base array, and:
     *  - `reversed == true`  means the path traverses that segment as `r..l`
     *  - `reversed == false` means the path traverses that segment as `l..r`
     *
     *  This is useful for non-commutative path folds where segment direction
     *  matters.
     */
    template <class F>
    void for_each_path_segment_id(size_t u, size_t v, F && visit) const
    {
      ensure_not_empty("Gen_Heavy_Light_Decomposition::for_each_path_segment_id");
      topology_.validate_id(u, "Gen_Heavy_Light_Decomposition::for_each_path_segment_id");
      topology_.validate_id(v, "Gen_Heavy_Light_Decomposition::for_each_path_segment_id");

      DynListStack<Range> down_segments;

      while (head_(u) != head_(v))
        {
          if (depth_(head_(u)) >= depth_(head_(v)))
            {
              visit(pos_(head_(u)), pos_(u), true);
              u = parent_(head_(u));
            }
          else
            {
              down_segments.push(Range{pos_(head_(v)), pos_(v)});
              v = parent_(head_(v));
            }
        }

      if (depth_(u) >= depth_(v))
        visit(pos_(v), pos_(u), true);
      else
        down_segments.push(Range{pos_(u), pos_(v)});

      while (not down_segments.is_empty())
        {
          const Range seg = down_segments.pop();
          visit(seg.left, seg.right, false);
        }
    }

    /** @brief Enumerate path segments from `u` to `v` in path order. */
    template <class F>
    void for_each_path_segment(const Node * u, const Node * v, F && visit) const
    {
      for_each_path_segment_id(id_of(u), id_of(v), std::forward<F>(visit));
    }

    /** @brief Enumerate path segments ignoring direction.
     *
     *  Useful when the query operation is commutative.
     */
    template <class F>
    void for_each_path_segment_undirected_id(const size_t u,
                                             const size_t v,
                                             F && visit) const
    {
      for_each_path_segment_id(u, v,
                               [&](const size_t l, const size_t r, const bool)
                               {
                                 visit(l, r);
                               });
    }
  };


  /** @brief Segment-tree-powered path/subtree queries over an HLD layout.
   *
   *  This class combines `Gen_Heavy_Light_Decomposition` with
   *  `Gen_Segment_Tree` for convenient dynamic queries.
   *
   *  Supported operations:
   *  - `query_path(u, v)`
   *  - `query_subtree(u)`
   *  - `update_node(u, delta)`
   *  - `set_node(u, value)`
   *  - `get_node(u)`
   *
   *  @note `query_path()` combines segment answers with `Op` and therefore
   *        assumes this fold is valid for your query semantics. For
   *        non-commutative path queries, use `for_each_path_segment_id()` from
   *        `Gen_Heavy_Light_Decomposition` and handle direction explicitly.
   *
   *  @tparam GT Graph type.
   *  @tparam T Segment value type.
   *  @tparam Op Associative operation.
   *  @tparam SA Arc filter type.
   *
   *  @ingroup Graphs
   */
  template <class GT,
            typename T = typename GT::Node::Node_Type,
            class Op = Aleph::plus<T>,
            class SA = Dft_Show_Arc<GT>>
  class Gen_HLD_Path_Query
  {
  public:
    using Node = typename GT::Node;

  private:
    Gen_Heavy_Light_Decomposition<GT, SA> hld_;
    Gen_Segment_Tree<T, Op> segment_tree_;
    T identity_;
    Op op_;

    void ensure_not_empty(const char * where) const
    {
      ah_domain_error_if(is_empty()) << where << ": tree is empty";
    }

    template <class Getter>
      requires std::invocable<Getter, Node *> and
               std::convertible_to<std::invoke_result_t<Getter, Node *>, T>
    static Array<T> build_base_array(const Gen_Heavy_Light_Decomposition<GT, SA> & hld,
                                     Getter getter)
    {
      const size_t n = hld.size();
      if (n == 0)
        return Array<T>();

      auto base = Array<T>::create(n);
      for (size_t id = 0; id < n; ++id)
        {
          const size_t p = hld.position_of_id(id);
          base(p) = static_cast<T>(getter(hld.node_of(id)));
        }

      return base;
    }

    static Array<T> build_base_from_node_info(const Gen_Heavy_Light_Decomposition<GT, SA> & hld)
    {
      return build_base_array(hld,
                              [](Node * p) -> T
                              {
                                return static_cast<T>(p->get_info());
                              });
    }

  public:
    /** @brief Construct from graph/root using `node->get_info()` as initial values. */
    Gen_HLD_Path_Query(const GT & g,
                       Node * root,
                       const T & identity,
                       Op op = Op(),
                       SA sa = SA())
      : hld_(g, root, std::move(sa)),
        segment_tree_(build_base_from_node_info(hld_), identity, op),
        identity_(identity),
        op_(op)
    {
      // Empty
    }

    /** @brief Construct from graph (first node as root) using `node->get_info()`. */
    Gen_HLD_Path_Query(const GT & g,
                       const T & identity,
                       Op op = Op(),
                       SA sa = SA())
      : hld_(g, std::move(sa)),
        segment_tree_(build_base_from_node_info(hld_), identity, op),
        identity_(identity),
        op_(op)
    {
      // Empty
    }

    /** @brief Construct with custom node-to-value getter. */
    template <class Getter>
      requires std::invocable<Getter, Node *> and
               std::convertible_to<std::invoke_result_t<Getter, Node *>, T>
    Gen_HLD_Path_Query(const GT & g,
                       Node * root,
                       Getter getter,
                       const T & identity,
                       Op op = Op(),
                       SA sa = SA())
      : hld_(g, root, std::move(sa)),
        segment_tree_(build_base_array(hld_, getter), identity, op),
        identity_(identity),
        op_(op)
    {
      // Empty
    }

    /** @brief Construct with custom getter and implicit root (first node). */
    template <class Getter>
      requires std::invocable<Getter, Node *> and
               std::convertible_to<std::invoke_result_t<Getter, Node *>, T>
    Gen_HLD_Path_Query(const GT & g,
                       Getter getter,
                       const T & identity,
                       Op op = Op(),
                       SA sa = SA())
      : hld_(g, std::move(sa)),
        segment_tree_(build_base_array(hld_, getter), identity, op),
        identity_(identity),
        op_(op)
    {
      // Empty
    }

    [[nodiscard]] const Gen_Heavy_Light_Decomposition<GT, SA> & decomposition() const noexcept
    {
      return hld_;
    }

    [[nodiscard]] size_t size() const noexcept { return hld_.size(); }
    [[nodiscard]] bool is_empty() const noexcept { return hld_.is_empty(); }

    /** @brief Path query between two node IDs in O(log^2 n). */
    [[nodiscard]] T query_path_id(const size_t u, const size_t v) const
    {
      ensure_not_empty("Gen_HLD_Path_Query::query_path_id");

      T ret = identity_;
      hld_.for_each_path_segment_id(u, v,
                                    [&](const size_t l,
                                        const size_t r,
                                        const bool)
                                    {
                                      ret = op_(ret, segment_tree_.query(l, r));
                                    });
      return ret;
    }

    /** @brief Path query between two nodes in O(log^2 n). */
    [[nodiscard]] T query_path(const Node * u, const Node * v) const
    {
      return query_path_id(hld_.id_of(u), hld_.id_of(v));
    }

    /** @brief Subtree query for node ID in O(log n). */
    [[nodiscard]] T query_subtree_id(const size_t id) const
    {
      ensure_not_empty("Gen_HLD_Path_Query::query_subtree_id");
      const auto range = hld_.subtree_range_id(id);
      return segment_tree_.query(range.left, range.right);
    }

    /** @brief Subtree query for node in O(log n). */
    [[nodiscard]] T query_subtree(const Node * node) const
    {
      return query_subtree_id(hld_.id_of(node));
    }

    /** @brief Point update: `a[id] = Op(a[id], delta)`. */
    void update_node_id(const size_t id, const T & delta)
    {
      ensure_not_empty("Gen_HLD_Path_Query::update_node_id");
      segment_tree_.update(hld_.position_of_id(id), delta);
    }

    /** @brief Point update: `a[node] = Op(a[node], delta)`. */
    void update_node(const Node * node, const T & delta)
    {
      update_node_id(hld_.id_of(node), delta);
    }

    /** @brief Point assignment: `a[id] = value`. */
    void set_node_id(const size_t id, const T & value)
    {
      ensure_not_empty("Gen_HLD_Path_Query::set_node_id");
      segment_tree_.set(hld_.position_of_id(id), value);
    }

    /** @brief Point assignment: `a[node] = value`. */
    void set_node(const Node * node, const T & value)
    {
      set_node_id(hld_.id_of(node), value);
    }

    /** @brief Return current value at node ID. */
    [[nodiscard]] T get_node_id(const size_t id) const
    {
      ensure_not_empty("Gen_HLD_Path_Query::get_node_id");
      return segment_tree_.get(hld_.position_of_id(id));
    }

    /** @brief Return current value at node. */
    [[nodiscard]] T get_node(const Node * node) const
    {
      return get_node_id(hld_.id_of(node));
    }
  };


  /** @brief Centroid decomposition over a rooted tree represented as an Aleph graph.
   *
   *  Decomposes a tree recursively by centroids, building a centroid tree.
   *
   *  For each original node `u`, the class stores:
   *  - `centroid_ancestors_of_id(u)`
   *  - `centroid_distances_of_id(u)`
   *
   *  where element `k` in both arrays corresponds to the same centroid ancestor.
   *
   *  Build complexity: O(n log n)
   *  Storage complexity: O(n log n)
   *
   *  @tparam GT Graph type.
   *  @tparam SA Arc filter type.
   *
   *  @ingroup Graphs
   */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  class Gen_Centroid_Decomposition
  {
  public:
    using Node = typename GT::Node;

  private:
    using Topology = tree_decomposition_detail::Rooted_Tree_Topology<GT, SA>;
    static constexpr size_t NONE = Topology::NONE;

    Topology topology_;

    Array<char> removed_;
    Array<size_t> centroid_parent_;
    Array<size_t> centroid_level_;
    size_t centroid_root_ = NONE;

    Array<size_t> local_parent_;
    Array<size_t> local_subtree_;

    Array<Array<size_t>> centroid_ancestors_;
    Array<Array<size_t>> centroid_distances_;

    void ensure_not_empty(const char * where) const
    {
      ah_domain_error_if(is_empty()) << where << ": tree is empty";
    }

    void init_storage()
    {
      if (is_empty())
        return;

      const size_t n = size();
      removed_ = Array<char>::create(n);
      centroid_parent_ = Array<size_t>::create(n);
      centroid_level_ = Array<size_t>::create(n);
      local_parent_ = Array<size_t>::create(n);
      local_subtree_ = Array<size_t>::create(n);
      centroid_ancestors_ = Array<Array<size_t>>::create(n);
      centroid_distances_ = Array<Array<size_t>>::create(n);

      for (size_t i = 0; i < n; ++i)
        {
          removed_(i) = 0;
          centroid_parent_(i) = NONE;
          centroid_level_(i) = 0;
          local_parent_(i) = NONE;
          local_subtree_(i) = 0;
          centroid_ancestors_(i) = Array<size_t>();
          centroid_distances_(i) = Array<size_t>();
        }

      centroid_root_ = NONE;
    }

    Array<size_t> collect_component_nodes(const size_t start)
    {
      Array<size_t> nodes;
      DynListStack<size_t> stack;

      local_parent_(start) = NONE;
      stack.push(start);

      while (not stack.is_empty())
        {
          const size_t u = stack.pop();
          nodes.append(u);

          for (size_t i = 0; i < topology_.adjacency()(u).size(); ++i)
            {
              const size_t v = topology_.adjacency()(u)(i);
              if (removed_(v))
                continue;

              if (v == local_parent_(u))
                continue;

              local_parent_(v) = u;
              stack.push(v);
            }
        }

      return nodes;
    }

    size_t choose_centroid(const Array<size_t> & nodes)
    {
      const size_t total = nodes.size();
      ah_runtime_error_unless(total > 0)
        << "Gen_Centroid_Decomposition: empty component while choosing centroid";

      for (size_t i = total; i-- > 0;)
        {
          const size_t u = nodes(i);
          local_subtree_(u) = 1;

          for (size_t j = 0; j < topology_.adjacency()(u).size(); ++j)
            {
              const size_t v = topology_.adjacency()(u)(j);
              if (removed_(v))
                continue;

              if (local_parent_(v) == u)
                local_subtree_(u) += local_subtree_(v);
            }
        }

      size_t centroid = nodes(0);
      size_t best_balance = total + 1;

      for (size_t i = 0; i < total; ++i)
        {
          const size_t u = nodes(i);

          size_t max_part = total - local_subtree_(u);
          for (size_t j = 0; j < topology_.adjacency()(u).size(); ++j)
            {
              const size_t v = topology_.adjacency()(u)(j);
              if (removed_(v))
                continue;

              if (local_parent_(v) == u)
                max_part = std::max(max_part, local_subtree_(v));
            }

          if (max_part < best_balance)
            {
              best_balance = max_part;
              centroid = u;
            }
        }

      return centroid;
    }

    void append_centroid_annotations(const size_t centroid)
    {
      struct Frame
      {
        size_t node;
        size_t parent;
        size_t dist;
      };

      DynListStack<Frame> stack;
      stack.push({centroid, NONE, 0});

      while (not stack.is_empty())
        {
          const Frame fr = stack.pop();
          centroid_ancestors_(fr.node).append(centroid);
          centroid_distances_(fr.node).append(fr.dist);

          for (size_t i = 0; i < topology_.adjacency()(fr.node).size(); ++i)
            {
              const size_t v = topology_.adjacency()(fr.node)(i);
              if (removed_(v) or v == fr.parent)
                continue;

              stack.push({v, fr.node, fr.dist + 1});
            }
        }
    }

    void build_centroid_tree()
    {
      const size_t n = size();
      if (n == 0)
        return;

      init_storage();

      struct Task
      {
        size_t start;
        size_t parent_centroid;
        size_t level;
      };

      DynListStack<Task> tasks;
      tasks.push({topology_.root_id(), NONE, 0});

      size_t centroid_count = 0;

      while (not tasks.is_empty())
        {
          const Task task = tasks.pop();
          if (removed_(task.start))
            continue;

          const Array<size_t> nodes = collect_component_nodes(task.start);
          const size_t centroid = choose_centroid(nodes);

          append_centroid_annotations(centroid);

          centroid_parent_(centroid) = task.parent_centroid;
          centroid_level_(centroid) = task.level;
          if (task.parent_centroid == NONE)
            centroid_root_ = centroid;

          removed_(centroid) = 1;
          ++centroid_count;

          for (size_t i = 0; i < topology_.adjacency()(centroid).size(); ++i)
              if (const size_t v = topology_.adjacency()(centroid)(i); not removed_(v))
                tasks.push({v, centroid, task.level + 1});
        }

      ah_runtime_error_unless(centroid_count == n)
        << "Gen_Centroid_Decomposition: invalid centroid tree size";
    }

  public:
    /** @brief Construct using an explicit root node. */
    Gen_Centroid_Decomposition(const GT & g, Node * root, SA sa = SA())
      : topology_(g, root, std::move(sa))
    {
      build_centroid_tree();
    }

    /** @brief Construct using the first graph node as root. */
    Gen_Centroid_Decomposition(const GT & g, SA sa = SA())
      : topology_(g, nullptr, std::move(sa))
    {
      build_centroid_tree();
    }

    [[nodiscard]] size_t size() const noexcept { return topology_.size(); }
    [[nodiscard]] bool is_empty() const noexcept { return topology_.is_empty(); }

    [[nodiscard]] Node * root() const noexcept { return topology_.root(); }
    [[nodiscard]] size_t root_id() const noexcept { return topology_.root_id(); }

    [[nodiscard]] Node * node_of(const size_t id) const
    {
      return topology_.node_of(id);
    }

    [[nodiscard]] size_t id_of(const Node * node) const
    {
      return topology_.id_of(node);
    }

    /** @brief Root centroid ID of the centroid tree (or `NONE` if empty). */
    [[nodiscard]] size_t centroid_root_id() const noexcept
    {
      return centroid_root_;
    }

    /** @brief Root centroid node of the centroid tree (or `nullptr` if empty). */
    [[nodiscard]] Node * centroid_root() const noexcept
    {
      return centroid_root_ == NONE ? nullptr : node_of(centroid_root_);
    }

    /** @brief Parent centroid ID of `id` (or `NONE` if centroid root). */
    [[nodiscard]] size_t centroid_parent_id(const size_t id) const
    {
      topology_.validate_id(id, "Gen_Centroid_Decomposition::centroid_parent_id");
      return centroid_parent_(id);
    }

    /** @brief Parent centroid of `node` (or `nullptr` if centroid root). */
    [[nodiscard]] Node * centroid_parent(const Node * node) const
    {
      const size_t pid = centroid_parent_id(id_of(node));
      return pid == NONE ? nullptr : node_of(pid);
    }

    /** @brief Level of centroid `id` in centroid tree (root level = 0). */
    [[nodiscard]] size_t centroid_level_of_id(const size_t id) const
    {
      topology_.validate_id(id, "Gen_Centroid_Decomposition::centroid_level_of_id");
      return centroid_level_(id);
    }

    /** @brief Level of centroid node in centroid tree (root level = 0). */
    [[nodiscard]] size_t centroid_level_of(const Node * node) const
    {
      return centroid_level_of_id(id_of(node));
    }

    /** @brief Maximum centroid-tree level. */
    [[nodiscard]] size_t max_centroid_level() const noexcept
    {
      size_t ret = 0;
      for (size_t i = 0; i < size(); ++i)
        ret = std::max(ret, centroid_level_(i));
      return ret;
    }

    /** @brief Number of centroid ancestors stored for node ID. */
    [[nodiscard]] size_t centroid_path_length_of_id(const size_t id) const
    {
      topology_.validate_id(id, "Gen_Centroid_Decomposition::centroid_path_length_of_id");
      return centroid_ancestors_(id).size();
    }

    /** @brief Number of centroid ancestors stored for node. */
    [[nodiscard]] size_t centroid_path_length_of(const Node * node) const
    {
      return centroid_path_length_of_id(id_of(node));
    }

    /** @brief Centroid ancestors of node ID from centroid root down to local centroid. */
    [[nodiscard]] const Array<size_t> & centroid_ancestors_of_id(const size_t id) const
    {
      topology_.validate_id(id, "Gen_Centroid_Decomposition::centroid_ancestors_of_id");
      return centroid_ancestors_(id);
    }

    /** @brief Distances to centroid ancestors (aligned with `centroid_ancestors_of_id`). */
    [[nodiscard]] const Array<size_t> & centroid_distances_of_id(const size_t id) const
    {
      topology_.validate_id(id, "Gen_Centroid_Decomposition::centroid_distances_of_id");
      return centroid_distances_(id);
    }

    /** @brief Centroid ancestors of node from centroid root down to local centroid. */
    [[nodiscard]] const Array<size_t> & centroid_ancestors_of(const Node * node) const
    {
      return centroid_ancestors_of_id(id_of(node));
    }

    /** @brief Distances to centroid ancestors (aligned with `centroid_ancestors_of`). */
    [[nodiscard]] const Array<size_t> & centroid_distances_of(const Node * node) const
    {
      return centroid_distances_of_id(id_of(node));
    }

    /** @brief True iff `ancestor` is in the centroid-ancestor chain of `node`. */
    [[nodiscard]] bool is_centroid_ancestor_id(const size_t ancestor,
                                               const size_t node) const
    {
      topology_.validate_id(ancestor, "Gen_Centroid_Decomposition::is_centroid_ancestor_id");
      topology_.validate_id(node, "Gen_Centroid_Decomposition::is_centroid_ancestor_id");

      const auto & chain = centroid_ancestors_(node);
      for (size_t i = 0; i < chain.size(); ++i)
        if (chain(i) == ancestor)
          return true;

      return false;
    }

    /** @brief Distance from node ID to a centroid ancestor ID.
     *
     *  Returns `NONE` if `centroid` is not in node's centroid-ancestor chain.
     */
    [[nodiscard]] size_t distance_to_centroid_id(const size_t node,
                                                 const size_t centroid) const
    {
      topology_.validate_id(node, "Gen_Centroid_Decomposition::distance_to_centroid_id");
      topology_.validate_id(centroid, "Gen_Centroid_Decomposition::distance_to_centroid_id");

      const auto & chain = centroid_ancestors_(node);
      const auto & dists = centroid_distances_(node);

      for (size_t i = 0; i < chain.size(); ++i)
        if (chain(i) == centroid)
          return dists(i);

      return NONE;
    }

    /** @brief Visit each centroid ancestor of node ID.
     *
     *  Callback signature: `f(centroid_id, distance, index)`.
     */
    template <class F>
    void for_each_centroid_ancestor_id(const size_t node, F && f) const
    {
      ensure_not_empty("Gen_Centroid_Decomposition::for_each_centroid_ancestor_id");
      topology_.validate_id(node, "Gen_Centroid_Decomposition::for_each_centroid_ancestor_id");

      const auto & chain = centroid_ancestors_(node);
      const auto & dists = centroid_distances_(node);

      ah_runtime_error_unless(chain.size() == dists.size())
        << "Gen_Centroid_Decomposition: invalid centroid annotation state";

      for (size_t i = 0; i < chain.size(); ++i)
        f(chain(i), dists(i), i);
    }

    /** @brief Visit each centroid ancestor of node.
     *
     *  Callback signature: `f(centroid_id, distance, index)`.
     */
    template <class F>
    void for_each_centroid_ancestor(const Node * node, F && f) const
    {
      for_each_centroid_ancestor_id(id_of(node), std::forward<F>(f));
    }
  };


  /** @brief Convenience alias for heavy-light decomposition. */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  using Heavy_Light_Decomposition = Gen_Heavy_Light_Decomposition<GT, SA>;

  /** @brief Convenience alias for segment-tree-backed HLD queries. */
  template <class GT,
            typename T = typename GT::Node::Node_Type,
            class Op = Aleph::plus<T>,
            class SA = Dft_Show_Arc<GT>>
  using HLD_Path_Query = Gen_HLD_Path_Query<GT, T, Op, SA>;

  /** @brief Convenience alias for centroid decomposition. */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  using Centroid_Decomposition = Gen_Centroid_Decomposition<GT, SA>;

} // namespace Aleph

# endif // TREE_DECOMPOSITION_H
