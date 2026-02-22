
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


/** @file Dominators.H
 *  @brief Dominator tree computation via the Lengauer-Tarjan algorithm.
 *
 *  This file implements the Lengauer-Tarjan algorithm for computing
 *  **immediate dominators** and the **dominator tree** of a directed graph
 *  from a given root node, in O(E * alpha(V)) time.
 *
 *  ## Definitions
 *
 *  - A node `d` **dominates** `v` if every path from the root to `v`
 *    passes through `d`.
 *  - The **immediate dominator** (idom) of `v` is the closest strict
 *    dominator of `v`.
 *  - The **dominator tree** has an edge from idom(v) to v for each
 *    non-root reachable node.
 *  - The **dominance frontier** DF(x) is the set of nodes y such that
 *    x dominates a predecessor of y but does not strictly dominate y.
 *    Dominance frontiers are essential for SSA construction in compilers.
 *
 *  ## Algorithm Overview
 *
 *  The Lengauer-Tarjan algorithm uses a single DFS traversal followed by
 *  semi-dominator computation with a Union-Find structure with path
 *  compression:
 *
 *  1. **DFS**: Number nodes in DFS order, record DFS tree parent.
 *  2. **Semi-dominators**: Process nodes in reverse DFS order. For each
 *     node, examine predecessors via incoming arcs and compute the
 *     semi-dominator using EVAL/LINK on the Union-Find forest.
 *  3. **Immediate dominators**: Forward pass resolving deferred idom values.
 *
 *  ## Complexity
 *
 *  | Operation | Time | Space |
 *  |-----------|------|-------|
 *  | Dominator tree | O(E * alpha(V)) | O(V) |
 *  | Dominance frontiers | O(V + E) additional | O(V + |DF|) |
 *
 *  ## Applications
 *
 *  - SSA form construction in optimizing compilers
 *  - Control-flow analysis and optimization
 *  - Program dependence graph construction
 *  - Loop detection and nesting analysis
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  List_Digraph<Graph_Node<std::string>, Graph_Arc<int>> cfg;
 *  // ... build control-flow graph ...
 *  auto root = cfg.get_first_node();
 *
 *  // Get immediate dominators as (node, idom) pairs
 *  auto idoms = compute_dominators(cfg, root);
 *
 *  // Build dominator tree as a separate graph
 *  decltype(cfg) dom_tree;
 *  build_dominator_tree(cfg, root, dom_tree);
 *
 *  // Compute dominance frontiers for SSA construction
 *  auto df = compute_dominance_frontiers(cfg, root);
 *  ```
 *
 *  @see Tarjan.H Strongly connected components (same DFS-based family)
 *  @see topological_sort.H Topological ordering for DAGs
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef DOMINATORS_H
# define DOMINATORS_H

# include <utility>
# include <tpl_graph.H>
# include <tpl_graph_utils.H>
# include <tpl_array.H>
# include <htlist.H>
# include <ah-errors.H>
# include <tpl_dynSetTree.H>

namespace Aleph
{
  /** @class Lengauer_Tarjan_Dominators
   *  @brief Computes dominator tree and dominance frontiers of a digraph
   *         using the Lengauer-Tarjan algorithm.
   *
   *  This class implements the Lengauer-Tarjan algorithm for computing
   *  immediate dominators in a directed graph from a given root node.
   *  The algorithm runs in O(E * alpha(V)) time using a Union-Find
   *  structure with path compression.
   *
   *  The class caches its computation: calling multiple methods with
   *  the same graph and root reuses the previous result without
   *  recomputing.
   *
   *  @tparam GT The directed graph type.
   *  @tparam SA Arc filter class for the internal iterator.
   *             Defaults to Dft_Show_Arc<GT> which shows all arcs.
   *
   *  @note This class uses `NODE_COUNTER` on graph nodes internally.
   *        After computation, node counters contain DFS numbers.
   *  @note The graph must be a digraph. Behavior on undirected graphs is
   *        undefined.
   *
   *  @warning This class is NOT thread-safe.
   *
   *  @ingroup Graphs
   *  @see Tarjan_Connected_Components
   */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  class Lengauer_Tarjan_Dominators
  {
    SA sa;

    using Node = typename GT::Node;
    using Arc = typename GT::Arc;

    GT *gptr = nullptr;
    Node *root_ptr = nullptr;
    bool is_computed = false;
    long num_reachable = 0;

    // Internal arrays indexed by DFS number
    Array<Node*> vertex;     // DFS number -> node pointer
    Array<long> par;         // DFS parent (as DFS number)
    Array<long> semi;        // semi-dominator (as DFS number)
    Array<long> idom_arr;    // immediate dominator (as DFS number)
    Array<long> anc;         // Union-Find ancestor
    Array<long> label_arr;   // Union-Find label (best semi on path)

    /** @brief Recursive DFS to number nodes and build DFS tree.
     *
     *  @param v Current node being visited.
     *  @param p DFS number of v's parent (-1 for root).
     */
    void dfs(Node *v, long p)
    {
      long v_num = num_reachable++;
      NODE_COUNTER(v) = v_num;
      vertex[v_num] = v;
      par[v_num] = p;
      semi[v_num] = v_num;
      label_arr[v_num] = v_num;
      anc[v_num] = -1;

      for (Out_Iterator<GT, SA> it(v, sa); it.has_curr(); it.next_ne())
        {
          auto w = gptr->get_tgt_node(it.get_curr());
          if (NODE_COUNTER(w) == -1)
            dfs(w, v_num);
        }
    }

    /** @brief Path compression for Union-Find.
     *
     *  Updates label[v] to hold the node with the minimum
     *  semi-dominator on the path from v to the tree root
     *  in the current forest.
     *
     *  @param v The node (DFS number) to compress.
     */
    void compress(long v)
    {
      if (anc[anc[v]] == -1)
        return;
      compress(anc[v]);
      if (semi[label_arr[anc[v]]] < semi[label_arr[v]])
        label_arr[v] = label_arr[anc[v]];
      anc[v] = anc[anc[v]];
    }

    /** @brief EVAL operation for the Union-Find forest.
     *
     *  Returns the node with minimum semi-dominator on the path
     *  from v to the root of v's tree in the forest.
     *
     *  @param v The node (DFS number) to evaluate.
     *  @return DFS number of the node with minimum semi.
     */
    [[nodiscard]] long eval(long v)
    {
      if (anc[v] == -1)
        return v;
      compress(v);
      return label_arr[v];
    }

    /** @brief Core computation of dominators.
     *
     *  Performs DFS from root, computes semi-dominators in reverse DFS
     *  order using the Union-Find forest, then resolves immediate
     *  dominators in a forward pass.
     *
     *  @param g The directed graph.
     *  @param root The root node for dominator computation.
     */
    void do_compute(const GT & g, Node *root)
    {
      gptr = &const_cast<GT &>(g);
      root_ptr = root;
      is_computed = false;
      num_reachable = 0;

      const size_t N = g.get_num_nodes();
      if (N == 0)
        {
          is_computed = true;
          return;
        }

      // Allocate internal arrays
      vertex = Array<Node*>(N, nullptr);
      par = Array<long>(N, -1L);
      semi = Array<long>(N, -1L);
      idom_arr = Array<long>(N, -1L);
      anc = Array<long>(N, -1L);
      label_arr = Array<long>(N, -1L);

      // Mark all nodes as unvisited
      gptr->for_each_node([](Node *p) { NODE_COUNTER(p) = -1; });

      // Phase 1: DFS from root
      dfs(root, -1);

      if (num_reachable <= 1)
        {
          if (num_reachable == 1)
            idom_arr[0] = -1; // root has no dominator
          is_computed = true;
          return;
        }

      // Allocate buckets (one per reachable node)
      Array<DynList<long>> bucket(num_reachable, DynList<long>());

      // Phase 2: Semi-dominators (process in reverse DFS order)
      for (long w = num_reachable - 1; w >= 1; --w)
        {
          Node *wnode = vertex[w];

          // For each predecessor of w (incoming arcs)
          for (In_Iterator<GT, SA> it(wnode, sa); it.has_curr(); it.next_ne())
            {
              Node *pred = gptr->get_src_node(it.get_curr());
              long v_num = NODE_COUNTER(pred);
              if (v_num < 0 or v_num >= num_reachable)
                continue; // predecessor not reachable from root

              long u = eval(v_num);
              if (semi[u] < semi[w])
                semi[w] = semi[u];
            }

          bucket[semi[w]].append(w);
          anc[w] = par[w]; // LINK(parent[w], w)

          // Process bucket of parent[w]
          while (not bucket[par[w]].is_empty())
            {
              long v = bucket[par[w]].remove_first();
              long u = eval(v);
              idom_arr[v] = (semi[u] == semi[v]) ? par[w] : u;
            }
        }

      // Phase 3: Resolve deferred idom assignments
      for (long w = 1; w < num_reachable; ++w)
        {
          if (idom_arr[w] != semi[w])
            idom_arr[w] = idom_arr[idom_arr[w]];
        }

      idom_arr[0] = -1; // root has no dominator
      is_computed = true;
    }

    /** @brief Ensure computation is up to date.
     *
     *  If the computation was already done for the same graph and root,
     *  this is a no-op. Otherwise, recomputes.
     *
     *  @param g The directed graph.
     *  @param root Root node (must not be null).
     */
    void ensure_computed(const GT & g, Node *root)
    {
      ah_domain_error_if(root == nullptr)
        << "Lengauer_Tarjan_Dominators: root cannot be null";

      if (is_computed and gptr == &g and root_ptr == root)
        return;

      do_compute(g, root);
    }

  public:
    /** @brief Construct a dominator computation instance.
     *
     *  @param __sa Arc filter functor. Defaults to Dft_Show_Arc<GT>.
     */
    Lengauer_Tarjan_Dominators(SA __sa = SA()) noexcept
      : sa(std::move(__sa))
    { /* empty */
    }

    /// Dominator instances should not be copied (they hold internal state)
    Lengauer_Tarjan_Dominators(const Lengauer_Tarjan_Dominators &) = delete;
    Lengauer_Tarjan_Dominators & operator=(const Lengauer_Tarjan_Dominators &) = delete;

    /// Move is allowed
    Lengauer_Tarjan_Dominators(Lengauer_Tarjan_Dominators &&) = default;
    Lengauer_Tarjan_Dominators & operator=(Lengauer_Tarjan_Dominators &&) = default;

    /** @brief Compute immediate dominators.
     *
     *  Returns a list of (node, idom) pairs for every node reachable
     *  from the root. The root's idom is nullptr.
     *
     *  @param g The directed graph.
     *  @param root Root node for dominator computation.
     *  @return List of (node, immediate_dominator) pairs.
     *  @throws std::domain_error If root is null.
     */
    [[nodiscard]] DynList<std::pair<Node*, Node*>>
    compute_idom(const GT & g, Node *root)
    {
      ensure_computed(g, root);

      DynList<std::pair<Node*, Node*>> result;
      for (long i = 0; i < num_reachable; ++i)
        {
          Node *idom_node = (idom_arr[i] >= 0) ? vertex[idom_arr[i]] : nullptr;
          result.append(std::make_pair(vertex[i], idom_node));
        }
      return result;
    }

    /** @brief Build the dominator tree as a separate graph.
     *
     *  Creates a new directed graph `tree` where each reachable node
     *  from the original graph is represented, and arcs go from idom
     *  to child. Nodes are mapped via NODE_COOKIE between the original
     *  graph and the tree (bidirectional mapping via GT::map_nodes).
     *
     *  @param g The directed graph.
     *  @param root Root node.
     *  @param[out] tree Output dominator tree (cleared first).
     *  @throws std::domain_error If root is null.
     */
    void build_tree(const GT & g, Node *root, GT & tree)
    {
      ensure_computed(g, root);

      clear_graph(tree);

      if (num_reachable == 0)
        return;

      // Create tree nodes and establish mapping
      Array<Node*> tree_nodes(num_reachable, nullptr);
      for (long i = 0; i < num_reachable; ++i)
        {
          auto tn = tree.insert_node(vertex[i]->get_info());
          GT::map_nodes(vertex[i], tn);
          tree_nodes[i] = tn;
        }

      // Create arcs: idom(v) -> v
      for (long i = 1; i < num_reachable; ++i)
        {
          if (idom_arr[i] >= 0)
            tree.insert_arc(tree_nodes[idom_arr[i]], tree_nodes[i]);
        }
    }

    /** @brief Get the immediate dominator of a specific node.
     *
     *  @param g The directed graph.
     *  @param root Root node.
     *  @param node The node whose idom is queried.
     *  @return Pointer to the immediate dominator, or nullptr if node
     *          is the root or unreachable.
     *  @throws std::domain_error If root or node is null.
     */
    [[nodiscard]] Node * get_idom(const GT & g, Node *root, Node *node)
    {
      ah_domain_error_if(node == nullptr)
        << "Lengauer_Tarjan_Dominators::get_idom: node cannot be null";
      ensure_computed(g, root);

      long v = NODE_COUNTER(node);
      if (v < 0 or v >= num_reachable or vertex[v] != node)
        return nullptr; // unreachable node

      return (idom_arr[v] >= 0) ? vertex[idom_arr[v]] : nullptr;
    }

    /** @brief Test whether node d dominates node v.
     *
     *  Dominance is reflexive: every node dominates itself.
     *
     *  @param g The directed graph.
     *  @param root Root node.
     *  @param d Potential dominator.
     *  @param v Potential dominated node.
     *  @return true if d dominates v, false otherwise.
     *  @throws std::domain_error If root is null.
     */
    [[nodiscard]] bool dominates(const GT & g, Node *root, Node *d, Node *v)
    {
      ensure_computed(g, root);

      if (d == v)
        return true; // reflexive

      long d_num = NODE_COUNTER(d);
      long v_num = NODE_COUNTER(v);

      if (d_num < 0 or d_num >= num_reachable or vertex[d_num] != d)
        return false;
      if (v_num < 0 or v_num >= num_reachable or vertex[v_num] != v)
        return false;

      // Walk up the idom chain from v
      long curr = idom_arr[v_num];
      while (curr != -1)
        {
          if (curr == d_num)
            return true;
          curr = idom_arr[curr];
        }
      return false;
    }

    /** @brief Compute dominance frontiers for all reachable nodes.
     *
     *  The dominance frontier DF(x) of a node x is the set of nodes y
     *  where x dominates a predecessor of y but does not strictly
     *  dominate y. This is the key data structure for placing
     *  phi-functions in SSA form.
     *
     *  Uses the standard algorithm: for each join point (node with 2+
     *  predecessors), walk up from each predecessor to its idom,
     *  adding the join point to each visited node's frontier.
     *
     *  @param g The directed graph.
     *  @param root Root node.
     *  @return Map from each reachable node to its dominance frontier.
     *  @throws std::domain_error If root is null.
     */
    [[nodiscard]] DynMapTree<Node*, DynList<Node*>>
    dominance_frontiers(const GT & g, Node *root)
    {
      ensure_computed(g, root);

      DynMapTree<Node*, DynList<Node*>> df;

      // Initialize empty frontiers for all reachable nodes
      for (long i = 0; i < num_reachable; ++i)
        df.insert(vertex[i], DynList<Node*>());

      // For each node, if it has 2+ reachable predecessors, walk up
      for (long b = 0; b < num_reachable; ++b)
        {
          Node *bnode = vertex[b];

          // Collect reachable predecessors
          DynList<long> preds;
          for (In_Iterator<GT, SA> it(bnode, sa); it.has_curr(); it.next_ne())
            {
              Node *pred = gptr->get_src_node(it.get_curr());
              long p = NODE_COUNTER(pred);
              if (p >= 0 and p < num_reachable and vertex[p] == pred)
                preds.append(p);
            }

          if (preds.size() < 2)
            continue;

          // Walk up from each predecessor to idom(b)
          for (auto it = preds.get_it(); it.has_curr(); it.next_ne())
            {
              long runner = it.get_curr();
              while (runner != idom_arr[b] and runner != -1)
                {
                  df.find(vertex[runner]).append(bnode);
                  runner = idom_arr[runner];
                }
            }
        }

      return df;
    }

    /** @brief Build dominator tree (operator() alias).
     *
     *  @param g The directed graph.
     *  @param root Root node.
     *  @param[out] tree Output dominator tree.
     */
    void operator()(const GT & g, Node *root, GT & tree)
    {
      build_tree(g, root, tree);
    }

    /// @name Accessors
    /// @{

    /** @brief Get the arc filter. */
    [[nodiscard]] SA & get_filter() noexcept { return sa; }

    /** @brief Get the arc filter (const). */
    [[nodiscard]] const SA & get_filter() const noexcept { return sa; }

    /** @brief Check if a computation has been performed. */
    [[nodiscard]] bool has_computation() const noexcept { return is_computed; }

    /** @brief Get the graph of the last computation. */
    [[nodiscard]] GT * get_graph() const noexcept { return gptr; }

    /** @brief Get the root of the last computation. */
    [[nodiscard]] Node * get_root() const noexcept { return root_ptr; }

    /** @brief Get the number of reachable nodes in the last computation. */
    [[nodiscard]] long get_num_reachable() const noexcept
    {
      return num_reachable;
    }

    /// @}
  };


  // =========================================================================
  // Free functions
  // =========================================================================

  /** @brief Compute immediate dominators of a digraph from a root node.
   *
   *  Convenience free function wrapping Lengauer_Tarjan_Dominators.
   *
   *  @tparam GT Graph type (must be directed).
   *  @tparam SA Arc filter type.
   *  @param g The directed graph.
   *  @param root Root node.
   *  @param sa Arc filter.
   *  @return List of (node, idom) pairs. Root's idom is nullptr.
   *
   *  @ingroup Graphs
   *  @see Lengauer_Tarjan_Dominators
   */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  [[nodiscard]] DynList<std::pair<typename GT::Node*, typename GT::Node*>>
  compute_dominators(const GT & g, typename GT::Node *root, SA sa = SA())
  {
    return Lengauer_Tarjan_Dominators<GT, SA>(sa).compute_idom(g, root);
  }

  /** @brief Build the dominator tree of a digraph.
   *
   *  Convenience free function wrapping Lengauer_Tarjan_Dominators.
   *
   *  @tparam GT Graph type (must be directed).
   *  @tparam SA Arc filter type.
   *  @param g The directed graph.
   *  @param root Root node.
   *  @param[out] tree Output dominator tree (cleared first). Nodes are
   *              mapped to the original graph via GT::map_nodes.
   *  @param sa Arc filter.
   *
   *  @ingroup Graphs
   *  @see Lengauer_Tarjan_Dominators
   */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  void build_dominator_tree(const GT & g, typename GT::Node *root,
                            GT & tree, SA sa = SA())
  {
    Lengauer_Tarjan_Dominators<GT, SA>(sa).build_tree(g, root, tree);
  }

  /** @brief Compute dominance frontiers of a digraph.
   *
   *  Convenience free function wrapping Lengauer_Tarjan_Dominators.
   *
   *  @tparam GT Graph type (must be directed).
   *  @tparam SA Arc filter type.
   *  @param g The directed graph.
   *  @param root Root node.
   *  @param sa Arc filter.
   *  @return Map from each reachable node to its dominance frontier.
   *
   *  @ingroup Graphs
   *  @see Lengauer_Tarjan_Dominators
   */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  [[nodiscard]] DynMapTree<typename GT::Node*, DynList<typename GT::Node*>>
  compute_dominance_frontiers(const GT & g, typename GT::Node *root,
                              SA sa = SA())
  {
    return Lengauer_Tarjan_Dominators<GT, SA>(sa).dominance_frontiers(g, root);
  }

} // end namespace Aleph

# endif // DOMINATORS_H
