
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
 *  @brief Edge-weighted Link-Cut Tree using edge-as-node representation.
 *
 *  Extends the Link-Cut Tree to support edge values (weights). Each edge
 *  is internally represented as a hidden node in the underlying splay
 *  forest, placed between its two endpoint vertices. Vertex nodes carry
 *  the monoid identity so that path aggregations reflect edge values only.
 *
 *  ### Architecture
 *  This class composes with `Gen_Link_Cut_Tree<ET, EdgeMonoid, LazyTag>`,
 *  delegating all splay/access/link/cut mechanics to the proven
 *  vertex-only implementation. An edge `(u, v)` with weight `w` becomes
 *  three internal nodes linked as `u — edge_node(w) — v`, where vertex
 *  nodes hold `EdgeMonoid::identity()` (transparent to aggregation).
 *
 *  ### Typical Use Cases
 *  - **Dynamic MST**: query the max-weight edge on a cycle to decide
 *    which edge to swap (`MaxMonoid`).
 *  - **Bottleneck Paths**: find the minimum-capacity edge on a path
 *    (`MinMonoid`).
 *
 *  @tparam VT         Vertex payload type (not aggregated).
 *  @tparam ET         Edge value type (aggregated via EdgeMonoid).
 *  @tparam EdgeMonoid Associative combiner for edge aggregation.
 *  @tparam LazyTag    Lazy-tag policy for deferred edge updates.
 *
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_LINK_CUT_TREE_WITH_EDGES_H
# define TPL_LINK_CUT_TREE_WITH_EDGES_H

# include <functional>
# include <unordered_map>
# include <tpl_link_cut_tree.H>

namespace Aleph
{

  template <typename VT = int,
            typename ET = int,
            class EdgeMonoid = DefaultMonoid<ET>,
            class LazyTag = NoLazyTag<ET>>
    requires LCTMonoid<EdgeMonoid, ET> and LCTLazyTag<LazyTag, ET>
  class Gen_Link_Cut_Tree_WithEdges
  {
    using InternalLCT = Gen_Link_Cut_Tree<ET, EdgeMonoid, LazyTag>;

  public:

    /** @brief Opaque vertex handle (same type as internal LCT node). */
    using Node = typename InternalLCT::Node;

  private:

    struct EdgeKey
    {
      Node * a;
      Node * b;

      [[nodiscard]] bool operator==(const EdgeKey &) const noexcept = default;
    };

    struct EdgeKeyHash
    {
      [[nodiscard]] size_t operator()(const EdgeKey & key) const noexcept
      {
        const size_t ha = std::hash<Node *>{}(key.a);
        const size_t hb = std::hash<Node *>{}(key.b);
        return ha ^ (hb + 0x9e3779b97f4a7c15ULL + (ha << 6) + (ha >> 2));
      }
    };

    struct EdgeRec
    {
      Node * u;
      Node * v;
      Node * edge_node;
    };

    InternalLCT lct_;
    Array<EdgeRec> edge_recs_;
    std::unordered_map<EdgeKey, size_t, EdgeKeyHash> edge_idx_;
    size_t n_vertices_   = 0;
    size_t n_components_ = 0;

    [[nodiscard]] static EdgeKey make_edge_key(Node * u, Node * v) noexcept
    {
      return std::less<Node *>{}(u, v) ? EdgeKey{u, v} : EdgeKey{v, u};
    }

    size_t find_edge_idx(Node * u, Node * v) const noexcept
    {
      const auto it = edge_idx_.find(make_edge_key(u, v));
      return it == edge_idx_.end() ? edge_recs_.size() : it->second;
    }

    void remove_edge_at(size_t idx)
    {
      const auto removed = edge_recs_(idx);
      edge_idx_.erase(make_edge_key(removed.u, removed.v));

      if (idx + 1 < edge_recs_.size())
        {
          edge_recs_(idx) = edge_recs_(edge_recs_.size() - 1);
          const auto & moved = edge_recs_(idx);
          edge_idx_[make_edge_key(moved.u, moved.v)] = idx;
        }
      edge_recs_.remove_last();
    }

  public:

    Gen_Link_Cut_Tree_WithEdges() = default;

    Gen_Link_Cut_Tree_WithEdges(const Gen_Link_Cut_Tree_WithEdges &) = delete;
    Gen_Link_Cut_Tree_WithEdges & operator=(const Gen_Link_Cut_Tree_WithEdges &) = delete;
    Gen_Link_Cut_Tree_WithEdges(Gen_Link_Cut_Tree_WithEdges &&) = delete;
    Gen_Link_Cut_Tree_WithEdges & operator=(Gen_Link_Cut_Tree_WithEdges &&) = delete;

    /** @brief Create an isolated vertex.
     *
     *  The vertex node carries `EdgeMonoid::identity()` internally so
     *  that it is transparent to edge aggregation.
     *
     *  @param val Vertex payload (not used in aggregation; accepted for API convenience).
     *  @return Stable handle to the new vertex.
     */
    Node * make_vertex(const VT & = VT{})
    {
      auto * nd = lct_.make_vertex(EdgeMonoid::identity());
      ++n_vertices_;
      ++n_components_;
      return nd;
    }

    /** @brief Total number of user vertices (excluding internal edge nodes). */
    [[nodiscard]] size_t size() const noexcept { return n_vertices_; }

    /** @brief Number of connected components. */
    [[nodiscard]] size_t num_components() const noexcept
    { return n_components_; }

    /** @brief Number of edges in the forest. */
    [[nodiscard]] size_t num_edges() const noexcept
    { return edge_recs_.size(); }

    /** @brief Test whether `u` and `v` are in the same tree.
     *
     *  @throws std::invalid_argument if either handle is null.
     */
    [[nodiscard]] bool connected(Node * u, Node * v)
    {
      return lct_.connected(u, v);
    }

    /** @brief Add an edge `(u, v)` with weight `weight`.
     *
     *  Internally creates a hidden edge node `e` with value `weight`
     *  and links the chain `u — e — v` in the underlying splay forest.
     *
     *  @param u      First endpoint.
     *  @param v      Second endpoint.
     *  @param weight Edge value / weight.
     *  @throws std::invalid_argument if `u` or `v` is null, or `u == v`.
     *  @throws std::domain_error     if `u` and `v` are already connected.
     */
    void link(Node * u, Node * v, const ET & weight)
    {
      ah_invalid_argument_if(u == nullptr or v == nullptr) << "null handle";
      ah_invalid_argument_if(u == v) << "self-loop";
      ah_domain_error_if(lct_.connected(u, v)) << "already connected";

      const size_t idx = edge_recs_.size();
      const auto key = make_edge_key(u, v);

      edge_recs_.reserve(idx + 1);
      edge_idx_.reserve(idx + 1);

      edge_recs_.append({u, v, nullptr});

      try
        {
          edge_idx_.emplace(key, idx);

          auto * e = lct_.make_vertex(weight);
          lct_.link(u, e);
          lct_.link(e, v);
          edge_recs_(idx).edge_node = e;
        }
      catch (...)
        {
          edge_recs_.remove_last();
          edge_idx_.erase(key);
          throw;
        }

      --n_components_;
    }

    /** @brief Remove the edge between `u` and `v`.
     *
     *  Finds the hidden edge node and cuts both internal links
     *  `u — e` and `e — v`. The orphaned edge node is freed on
     *  destruction of the underlying LCT.
     *
     *  @throws std::invalid_argument if either handle is null.
     *  @throws std::domain_error     if no edge `(u, v)` exists.
     */
    void cut(Node * u, Node * v)
    {
      ah_invalid_argument_if(u == nullptr or v == nullptr) << "null handle";
      size_t idx = find_edge_idx(u, v);
      ah_domain_error_if(idx == edge_recs_.size()) << "edge does not exist";

      Node * e = edge_recs_(idx).edge_node;
      lct_.cut(u, e);
      lct_.cut(e, v);
      remove_edge_at(idx);
      ++n_components_;
    }

    /** @brief Aggregate edge values on the path from `u` to `v`.
     *
     *  Since vertex nodes carry `EdgeMonoid::identity()`, the returned
     *  value reflects only edge weights along the path.
     *
     *  @throws std::invalid_argument if either handle is null.
     *  @throws std::domain_error     if `u` and `v` are not connected.
     */
    [[nodiscard]] ET path_query(Node * u, Node * v)
    {
      return lct_.path_query(u, v);
    }

    /** @brief Make `x` the root of its represented tree.
     *
     *  @throws std::invalid_argument if `x` is null.
     */
    void make_root(Node * x)
    {
      lct_.make_root(x);
    }

    /** @brief Find the root of the tree containing `x`.
     *
     *  @note After rerooting via `make_root()`, the root is always a
     *        vertex node.
     *
     *  @throws std::invalid_argument if `x` is null.
     */
    [[nodiscard]] Node * find_root(Node * x)
    {
      return lct_.find_root(x);
    }

    /** @brief Number of internal nodes (vertices + edge nodes) on the
     *         path from `u` to `v`.
     */
    [[nodiscard]] size_t path_size(Node * u, Node * v)
    {
      return lct_.path_size(u, v);
    }

    /** @brief Update the weight of an existing edge `(u, v)`.
     *
     *  @throws std::invalid_argument if either handle is null.
     *  @throws std::domain_error     if no edge `(u, v)` exists.
     */
    void set_edge_val(Node * u, Node * v, const ET & new_weight)
    {
      ah_invalid_argument_if(u == nullptr or v == nullptr) << "null handle";
      size_t idx = find_edge_idx(u, v);
      ah_domain_error_if(idx == edge_recs_.size()) << "edge does not exist";

      lct_.set_val(edge_recs_(idx).edge_node, new_weight);
    }

    /** @brief Read the weight of an existing edge `(u, v)`.
     *
     *  @throws std::invalid_argument if either handle is null.
     *  @throws std::domain_error     if no edge `(u, v)` exists.
     */
    [[nodiscard]] const ET & get_edge_val(Node * u, Node * v)
    {
      ah_invalid_argument_if(u == nullptr or v == nullptr) << "null handle";
      size_t idx = find_edge_idx(u, v);
      ah_domain_error_if(idx == edge_recs_.size()) << "edge does not exist";

      return lct_.get_val(edge_recs_(idx).edge_node);
    }

    // ---- snapshot export ----

    /** @brief Data stored in each exported `Tree_Node`.
     *
     *  For the root node, `parent_edge` holds `EdgeMonoid::identity()`.
     */
    struct ExportData
    {
      Node * lct_node;    ///< Back-pointer to the original LCT vertex handle.
      ET     parent_edge; ///< Weight of the edge connecting this node to its parent.
    };

    /** @brief Export the represented tree rooted at `root` as a `Tree_Node`.
     *
     *  Materializes the forest component containing `root` into an
     *  explicit n-ary tree of `Tree_Node<ExportData>`.  Each exported
     *  node stores a back-pointer to the original LCT vertex and the
     *  weight of the edge to its parent (the root stores
     *  `EdgeMonoid::identity()`).
     *
     *  The caller owns the returned tree and must free it with
     *  `destroy_tree(result)`.
     *
     *  @param root Vertex to use as the root.
     *  @return Newly allocated `Tree_Node<ExportData> *`.
     *  @throws std::invalid_argument if `root` is null.
     *
     *  @par Complexity
     *  O(n^2) where n is the number of vertices in the component.
     */
    [[nodiscard]] Tree_Node<ExportData> * export_to_tree_node(Node * root)
    {
      ah_invalid_argument_if(root == nullptr) << "null handle";
      make_root(root);

      using TN = Tree_Node<ExportData>;

      // Build adjacency from edge records (only edges in root's component)
      // adj[i] = list of (neighbor_vertex_index, edge_weight)
      struct AdjEntry { size_t neighbor; ET weight; };

      // Collect vertices in root's component
      Array<Node *> verts;
      verts.append(root);
      for (size_t i = 0; i < edge_recs_.size(); ++i)
        {
          const auto & er = edge_recs_(i);
          if (lct_.connected(er.u, root))
            {
              // Ensure both endpoints are collected (avoid duplicates)
              bool found_u = false, found_v = false;
              for (size_t j = 0; j < verts.size(); ++j)
                {
                  if (verts(j) == er.u) found_u = true;
                  if (verts(j) == er.v) found_v = true;
                }
              if (not found_u) verts.append(er.u);
              if (not found_v) verts.append(er.v);
            }
        }

      auto find_idx = [&](Node * nd) -> size_t
      {
        for (size_t i = 0; i < verts.size(); ++i)
          if (verts(i) == nd)
            return i;
        return verts.size();
      };

      // Build adjacency lists
      size_t nv = verts.size();
      Array<Array<AdjEntry>> adj;
      for (size_t i = 0; i < nv; ++i)
        adj.append(Array<AdjEntry>());

      for (size_t i = 0; i < edge_recs_.size(); ++i)
        {
          const auto & er = edge_recs_(i);
          size_t ui = find_idx(er.u);
          size_t vi = find_idx(er.v);
          if (ui < nv and vi < nv)
            {
              ET w = lct_.get_val(er.edge_node);
              adj(ui).append({vi, w});
              adj(vi).append({ui, w});
            }
        }

      // BFS from root to build Tree_Node tree
      Array<TN *> tn(nv);
      tn.putn(nv);
      for (size_t i = 0; i < nv; ++i)
        tn(i) = nullptr;

      size_t root_idx = find_idx(root);
      tn(root_idx) = new TN({root, EdgeMonoid::identity()});

      // BFS queue (simple array-based)
      Array<size_t> queue;
      queue.append(root_idx);
      size_t qfront = 0;

      while (qfront < queue.size())
        {
          size_t ci = queue(qfront++);
          for (size_t j = 0; j < adj(ci).size(); ++j)
            {
              auto [ni, w] = adj(ci)(j);
              if (tn(ni) == nullptr)
                {
                  tn(ni) = new TN({verts(ni), w});
                  tn(ci)->insert_rightmost_child(tn(ni));
                  queue.append(ni);
                }
            }
        }

      return tn(root_idx);
    }
  };

  /** @brief Convenience alias for edge-weighted Link-Cut Tree. */
  template <typename EdgeT = int, class Monoid = DefaultMonoid<EdgeT>>
  using Link_Cut_Tree_WithEdges = Gen_Link_Cut_Tree_WithEdges<int, EdgeT, Monoid>;

} // namespace Aleph

# endif /* TPL_LINK_CUT_TREE_WITH_EDGES_H */
