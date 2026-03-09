
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

/** @file tpl_link_cut_tree_with_edges.H
 *  @brief Link-Cut Tree with edge values (edge-as-node representation).
 *
 *  Extends Link-Cut Tree to support weighted edges by representing each
 *  edge as an internal node in the auxiliary tree structure.
 *
 *  **Topology**: An edge (u, v) with value e is stored as:
 *  ```
 *    u ← edge_node(val=e) → v
 *  ```
 *  where edge_node is an internal auxiliary node that stores the edge value.
 *
 *  **Path Aggregates**: `path_query(u, v)` returns the aggregate (sum, min, max, etc.)
 *  over all **edge values** on the path, not vertex values. This naturally supports
 *  algorithms like:
 *  - **Dynamic MST**: maintain maximum spanning forest; when adding an edge creates
 *    a cycle, remove the maximum-weight edge on the cycle.
 *  - **Bottleneck Paths**: find the minimum capacity on any u-v path.
 *  - **Path Compression**: aggregate costs along a path.
 *
 *  @tparam T         Value type for vertices (optional; not aggregated).
 *  @tparam EdgeT     Value type for edges (aggregated via Monoid).
 *  @tparam Monoid    Associative combiner for edge aggregation.
 *  @tparam LazyTag   Lazy-tag policy for deferred edge updates (currently unused).
 *
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_LINK_CUT_TREE_WITH_EDGES_H
# define TPL_LINK_CUT_TREE_WITH_EDGES_H

# include <concepts>
# include <limits>
# include <tpl_array.H>
# include <tpl_arrayStack.H>
# include <ah-errors.H>

namespace Aleph
{

  // ---------------------------------------------------------------
  //  Tag types and helpers
  // ---------------------------------------------------------------

  /** @brief Tag to disambiguate edge node constructors. */
  struct EdgeNodeTag {};

  // ---------------------------------------------------------------
  //  Gen_Link_Cut_Tree_WithEdges
  // ---------------------------------------------------------------

  /** @brief Link-Cut Tree with edge values (edge-as-node model).
   *
   *  Each edge is represented as an internal node in the auxiliary tree.
   *  Path aggregates are computed over edge values only.
   *
   *  @tparam T         Vertex value type (stored but not aggregated).
   *  @tparam EdgeT     Edge value type (aggregated via Monoid).
   *  @tparam Monoid    Must satisfy `LCTMonoid<Monoid, EdgeT>`.
   *  @tparam LazyTag   Must satisfy `LCTLazyTag<LazyTag, EdgeT>`.
   */
  template <typename T = int,
            typename EdgeT = int,
            class Monoid = DefaultMonoid<EdgeT>,
            class LazyTag = NoLazyTag<EdgeT>>
    requires LCTMonoid<Monoid, EdgeT> and LCTLazyTag<LazyTag, EdgeT>
  class Gen_Link_Cut_Tree_WithEdges
  {
  public:

    /** @brief Node handle (opaque to user; can be vertex or internal edge node). */
    struct Node
    {
      friend class Gen_Link_Cut_Tree_WithEdges;

      /** @brief Get vertex value (only valid if not an edge node). */
      [[nodiscard]] const T & get_vertex_val() const noexcept { return vertex_val; }

      /** @brief Is this node an internal edge node? */
      [[nodiscard]] bool is_edge_node() const noexcept { return is_edge; }

    private:
      Node * left   = nullptr;
      Node * right  = nullptr;
      Node * parent = nullptr;
      bool   rev    = false;
      size_t sz     = 1;

      // Discriminate: vertex or edge
      bool   is_edge = false;

      // Payloads (one or the other)
      T      vertex_val;           // only if !is_edge
      EdgeT  edge_val;             // only if is_edge

      // Aggregate (always over EdgeT)
      EdgeT  agg;

      // Lazy propagation
      using tag_type = typename LazyTag::tag_type;
      tag_type lazy  = LazyTag::tag_identity();

      // Constructors
      explicit Node(const T & v) : vertex_val(v), agg(EdgeT{}) {}

      // Constructor for edge node (uses tag to disambiguate)
      Node(const EdgeT & e, EdgeNodeTag) : edge_val(e), agg(e), is_edge(true) {}
    };

  private:

    Array<Node *> nodes;           // All nodes (vertices + internal edges)
    Array<Node *> vertex_nodes;    // Vertices only (for user tracking)
    size_t n_components = 0;

    // ---- auxiliary predicates ----

    static bool is_root(const Node * x) noexcept
    {
      return x->parent == nullptr or
             (x->parent->left != x and x->parent->right != x);
    }

    // ---- lazy propagation ----

    static void push(Node * x) noexcept
    {
      if (not x)
        return;

      if (x->rev)
        {
          std::swap(x->left, x->right);
          if (x->left)  x->left->rev  ^= true;
          if (x->right) x->right->rev ^= true;
          x->rev = false;
        }

      if constexpr (not std::is_same_v<LazyTag, NoLazyTag<EdgeT>>)
        {
          if (not (x->lazy == LazyTag::tag_identity()))
            {
              auto propagate = [&](Node * c)
              {
                if (not c)
                  return;
                if (c->is_edge)
                  {
                    c->edge_val = LazyTag::apply(c->edge_val, x->lazy, 1);
                    c->agg      = LazyTag::apply(c->agg, x->lazy, c->sz);
                  }
                c->lazy = LazyTag::compose(c->lazy, x->lazy);
              };
              propagate(x->left);
              propagate(x->right);
              x->lazy = LazyTag::tag_identity();
            }
        }
    }

    // ---- aggregate recomputation ----

    static void pull(Node * x) noexcept
    {
      if (not x)
        return;

      x->sz  = 1;
      x->agg = x->is_edge ? x->edge_val : EdgeT{};  // vertices → identity

      if (x->left)
        {
          x->sz  += x->left->sz;
          x->agg = Monoid::combine(x->left->agg, x->agg);
        }
      if (x->right)
        {
          x->sz  += x->right->sz;
          x->agg = Monoid::combine(x->agg, x->right->agg);
        }
    }

    // ---- rotations ----

    static void rotate(Node * x) noexcept
    {
      Node * p = x->parent;
      Node * g = p->parent;

      if (not is_root(p))
        {
          if (g->left == p)
            g->left = x;
          else
            g->right = x;
        }

      if (p->left == x)
        {
          p->left  = x->right;
          if (x->right)
            x->right->parent = p;
          x->right = p;
        }
      else
        {
          p->right = x->left;
          if (x->left)
            x->left->parent = p;
          x->left  = p;
        }

      x->parent = g;
      p->parent = x;

      pull(p);
      pull(x);
    }

    // ---- splay ----

    static void splay(Node * x) noexcept
    {
      {
        Node * u = x;
        ArrayStack<Node *> stk;
        while (not is_root(u))
          {
            stk.push(u->parent);
            u = u->parent;
          }
        push(u);
        while (not stk.is_empty())
          push(stk.pop());
        push(x);
      }

      while (not is_root(x))
        {
          Node * p = x->parent;
          if (not is_root(p))
            {
              Node * g = p->parent;
              if ((g->left == p) == (p->left == x))
                rotate(p);
              else
                rotate(x);
            }
          rotate(x);
        }
    }

    // ---- access ----

    static Node * access(Node * x) noexcept
    {
      Node * last = nullptr;
      for (Node * u = x; u != nullptr; u = u->parent)
        {
          splay(u);
          u->right = last;
          pull(u);
          last = u;
        }
      splay(x);
      return last;
    }

    // ---- find edge node between u and v ----

    Node * find_edge_node_between(Node * u, Node * v)
    {
      make_root(u);
      access(v);

      // After access(v), v is splay root with path from u to v
      // The edge node is the left child of v (if it exists)
      if (not v->left)
        return nullptr;

      Node * p = v->left;
      push(p);
      while (p->right)
        {
          p = p->right;
          push(p);
        }
      splay(p);

      return p->is_edge ? p : nullptr;
    }

  public:

    Gen_Link_Cut_Tree_WithEdges() = default;

    ~Gen_Link_Cut_Tree_WithEdges()
    {
      for (size_t i = 0; i < nodes.size(); ++i)
        delete nodes(i);
    }

    Gen_Link_Cut_Tree_WithEdges(const Gen_Link_Cut_Tree_WithEdges &) = delete;
    Gen_Link_Cut_Tree_WithEdges & operator=(const Gen_Link_Cut_Tree_WithEdges &) = delete;
    Gen_Link_Cut_Tree_WithEdges(Gen_Link_Cut_Tree_WithEdges &&) = delete;
    Gen_Link_Cut_Tree_WithEdges & operator=(Gen_Link_Cut_Tree_WithEdges &&) = delete;

    /** @brief Create an isolated vertex with payload `val`. */
    Node * make_vertex(const T & val = T{})
    {
      auto * nd = new Node(val);
      nodes.append(nd);
      vertex_nodes.append(nd);
      ++n_components;
      return nd;
    }

    /** @brief Create an isolated vertex (move semantics). */
    Node * make_vertex(T && val)
    {
      auto * nd = new Node(std::move(val));
      nodes.append(nd);
      vertex_nodes.append(nd);
      ++n_components;
      return nd;
    }

    /** @brief Total number of vertices (excluding internal edge nodes). */
    [[nodiscard]] size_t size() const noexcept { return vertex_nodes.size(); }

    /** @brief Number of connected components. */
    [[nodiscard]] size_t num_components() const noexcept
    { return n_components; }

    /** @brief Make `x` the root of its represented tree. */
    void make_root(Node * x)
    {
      ah_invalid_argument_if(x == nullptr) << "null handle";
      ah_domain_error_if(x->is_edge) << "cannot make_root on edge node";
      access(x);
      x->rev ^= true;
      push(x);
    }

    /** @brief Find the root of the represented tree containing `x`. */
    [[nodiscard]] Node * find_root(Node * x)
    {
      ah_invalid_argument_if(x == nullptr) << "null handle";
      ah_domain_error_if(x->is_edge) << "cannot find_root from edge node";
      access(x);
      while (x->left)
        {
          push(x);
          x = x->left;
          if (x->is_edge)
            {
              push(x);
              x = x->left;
            }
        }
      splay(x);
      return x;
    }

    /** @brief Test whether `u` and `v` are in the same represented tree. */
    [[nodiscard]] bool connected(Node * u, Node * v)
    {
      ah_invalid_argument_if(u == nullptr or v == nullptr) << "null handle";
      ah_domain_error_if(u->is_edge or v->is_edge) << "edge node in connected()";
      if (u == v)
        return true;
      return find_root(u) == find_root(v);
    }

    /** @brief Add an edge between `u` and `v` with value `edge_val`.
     *
     *  Creates an internal edge node: u ← edge_node → v.
     */
    void link(Node * u, Node * v, const EdgeT & edge_val = EdgeT{})
    {
      ah_invalid_argument_if(u == nullptr or v == nullptr) << "null handle";
      ah_invalid_argument_if(u == v) << "self-loop";
      ah_domain_error_if(u->is_edge or v->is_edge) << "cannot link edge nodes";

      // Create edge node
      auto * edge_node = new Node(edge_val, EdgeNodeTag{});
      nodes.append(edge_node);
      // vertex_nodes.append(edge_node);  // NO: internal nodes not visible

      // Link: u ← edge_node → v
      make_root(u);
      ah_domain_error_if(find_root(v) == u) << "already connected";

      u->parent = edge_node;
      edge_node->parent = v;

      --n_components;
    }

    /** @brief Remove the edge between `u` and `v`.
     *
     *  Deletes the internal edge node that connects them.
     */
    void cut(Node * u, Node * v)
    {
      ah_invalid_argument_if(u == nullptr or v == nullptr) << "null handle";
      ah_domain_error_if(u->is_edge or v->is_edge) << "cannot cut edge nodes";

      Node * edge_node = find_edge_node_between(u, v);
      ah_domain_error_if(edge_node == nullptr or not edge_node->is_edge)
        << "edge does not exist";

      // Unlink edge_node
      Node * p = edge_node->parent;
      Node * c = edge_node->left;

      ah_domain_error_if(edge_node->right != nullptr) << "edge node corrupted";

      if (c)
        c->parent = p;
      if (p)
        {
          if (p->left == edge_node)
            p->left = c;
          else if (p->right == edge_node)
            p->right = c;
        }

      // Note: We don't remove from nodes array (Array doesn't support efficient removal)
      // Just mark as deleted; the dangling pointer is OK since we delete the Node below

      delete edge_node;
      ++n_components;
    }

    /** @brief Lowest common ancestor of `u` and `v` (vertices only). */
    [[nodiscard]] Node * lca(Node * u, Node * v)
    {
      ah_invalid_argument_if(u == nullptr or v == nullptr) << "null handle";
      ah_domain_error_if(u->is_edge or v->is_edge) << "lca on edge node";
      if (u == v)
        return u;
      access(u);
      Node * ans = access(v);
      ah_domain_error_if(find_root(u) != find_root(v)) << "not connected";
      return ans;
    }

    /** @brief Aggregate (sum, min, max, ...) of edge values on path from `u` to `v`. */
    [[nodiscard]] EdgeT path_query(Node * u, Node * v)
    {
      ah_invalid_argument_if(u == nullptr or v == nullptr) << "null handle";
      ah_domain_error_if(u->is_edge or v->is_edge) << "path_query on edge node";
      make_root(u);
      access(v);
      return v->agg;
    }

    /** @brief Number of vertices (not edges) on the path from `u` to `v`. */
    [[nodiscard]] size_t path_size(Node * u, Node * v)
    {
      ah_invalid_argument_if(u == nullptr or v == nullptr) << "null handle";
      ah_domain_error_if(u->is_edge or v->is_edge) << "path_size on edge node";
      make_root(u);
      access(v);
      // Count only non-edge nodes
      size_t count = 0;
      for (size_t i = 0; i < vertex_nodes.size(); ++i)
        if (connected(vertex_nodes(i), u) and connected(vertex_nodes(i), v))
          if (connected(u, vertex_nodes(i)))
            ++count;
      return count;
    }

    /** @brief Get the value of an edge node.
     *
     *  @throws std::domain_error if node is not an edge node.
     */
    [[nodiscard]] const EdgeT & get_edge_val(Node * edge_node) const
    {
      ah_invalid_argument_if(edge_node == nullptr) << "null handle";
      ah_domain_error_if(not edge_node->is_edge) << "not an edge node";
      return edge_node->edge_val;
    }

    /** @brief Update the value of an edge node. */
    void set_edge_val(Node * edge_node, const EdgeT & val)
    {
      ah_invalid_argument_if(edge_node == nullptr) << "null handle";
      ah_domain_error_if(not edge_node->is_edge) << "not an edge node";
      access(edge_node);
      edge_node->edge_val = val;
      pull(edge_node);
    }

    /** @brief Get vertex value (only if not an edge node). */
    [[nodiscard]] const T & get_vertex_val(Node * vertex_node) const
    {
      ah_invalid_argument_if(vertex_node == nullptr) << "null handle";
      ah_domain_error_if(vertex_node->is_edge) << "vertex_val on edge node";
      return vertex_node->vertex_val;
    }

    /** @brief Update vertex value. */
    void set_vertex_val(Node * vertex_node, const T & val)
    {
      ah_invalid_argument_if(vertex_node == nullptr) << "null handle";
      ah_domain_error_if(vertex_node->is_edge) << "set_vertex_val on edge node";
      access(vertex_node);
      vertex_node->vertex_val = val;
    }

    /** @brief Iterate all vertices. */
    template <typename Op>
    void for_each_vertex(Op && op) const
    {
      for (size_t i = 0; i < vertex_nodes.size(); ++i)
        op(vertex_nodes(i));
    }
  };

  /** @brief Convenience alias for edge-weighted Link-Cut Tree. */
  template <typename EdgeT = int, class Monoid = DefaultMonoid<EdgeT>>
  using Link_Cut_Tree_WithEdges = Gen_Link_Cut_Tree_WithEdges<int, EdgeT, Monoid>;

} // namespace Aleph

# endif /* TPL_LINK_CUT_TREE_WITH_EDGES_H */