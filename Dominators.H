
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
 *  @brief Dominator and post-dominator tree computation via the
 *         Lengauer-Tarjan algorithm.
 *
 *  This file implements the Lengauer-Tarjan algorithm for computing
 *  **immediate dominators** and the **dominator tree** of a directed graph
 *  from a given root node, in O(E * alpha(V)) time.  It also provides
 *  **post-dominator** computation by running the same algorithm on the
 *  reversed graph from a designated exit node.
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
    Array<DynList<long>> pred; // predecessors[i] = DFS numbers of predecessors of node i

    /** @brief Iterative DFS to number nodes and build DFS tree.
     *
     *  @param v Current node being visited.
     *  @param p DFS number of v's parent (-1 for root).
     */
    void dfs(Node *v, long p)
    {
      struct Pending_Frame
      {
        Node *node = nullptr;
        long parent = -1;
      };

      Array<Pending_Frame> stack;
      stack.reserve(gptr->get_num_nodes());
      stack.append(Pending_Frame{v, p});
      NODE_COUNTER(v) = -2; // discovered but not yet numbered

      while (not stack.is_empty())
        {
          const Pending_Frame frame = stack.remove_last();
          Node *curr = frame.node;

          const long v_num = num_reachable++;
          NODE_COUNTER(curr) = v_num;
          vertex[v_num] = curr;
          par[v_num] = frame.parent;
          semi[v_num] = v_num;
          label_arr[v_num] = v_num;
          anc[v_num] = -1;

          // Push children in reverse order so DFS visitation order
          // matches the recursive implementation.
          Array<Node *> children;
          for (Out_Iterator<GT, SA> it(curr, sa); it.has_curr(); it.next_ne())
            {
              Node *w = gptr->get_tgt_node(it.get_curr());
              if (NODE_COUNTER(w) != -1)
                continue;
              NODE_COUNTER(w) = -2; // mark discovered to avoid duplicate pushes
              children.append(w);
            }

          for (long i = static_cast<long>(children.size()) - 1; i >= 0; --i)
            stack.append(Pending_Frame{children[static_cast<size_t>(i)], v_num});
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
    void compress(const long v)
    {
      if (anc[v] == -1 or anc[anc[v]] == -1)
        return;

      Array<long> path;
      for (long x = v; anc[x] != -1 and anc[anc[x]] != -1; x = anc[x])
        path.append(x);

      for (long i = static_cast<long>(path.size()) - 1; i >= 0; --i)
        {
          const long x = path[static_cast<size_t>(i)];
          if (semi[label_arr[anc[x]]] < semi[label_arr[x]])
            label_arr[x] = label_arr[anc[x]];
          anc[x] = anc[anc[x]];
        }
    }

    /** @brief EVAL operation for the Union-Find forest.
     *
     *  Returns the node with minimum semi-dominator on the path
     *  from v to the root of v's tree in the forest.
     *
     *  @param v The node (DFS number) to evaluate.
     *  @return DFS number of the node with minimum semi.
     */
    [[nodiscard]] long eval(const long v)
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
      dfs(root, -1); // dfs updates num_reachable

      if (num_reachable <= 1)
        {
          if (num_reachable == 1)
            idom_arr[0] = -1; // root has no dominator
          is_computed = true;
          return;
        }

      // Build predecessor lists by iterating over all arcs.
      // In_Iterator does not work on List_Digraph because arcs are
      // only stored in the source node's adjacency list.
      pred = Array<DynList<long>>(static_cast<size_t>(num_reachable), DynList<long>());
      for (typename GT::Arc_Iterator ait(g); ait.has_curr(); ait.next_ne())
        {
          auto arc = ait.get_curr();
          if (not sa(arc))
            continue;
          long s = NODE_COUNTER(gptr->get_src_node(arc));
          long t = NODE_COUNTER(gptr->get_tgt_node(arc));
          if (s >= 0 and s < num_reachable and t >= 0 and t < num_reachable)
            pred[t].append(s);
        }

      // Allocate buckets (one per reachable node)
      Array<DynList<long>> bucket(static_cast<size_t>(num_reachable), DynList<long>());

      // Phase 2: Semi-dominators (process in reverse DFS order)
      for (long w = num_reachable - 1; w >= 1; --w)
        {
          // For each predecessor of w
          for (auto pit = pred[w].get_it(); pit.has_curr(); pit.next_ne())
            {
              const long v_num = pit.get_curr();
              const long u = eval(v_num);
              if (semi[u] < semi[w])
                semi[w] = semi[u];
            }

          bucket[semi[w]].append(w);
          anc[w] = par[w]; // LINK(parent[w], w)

          // Process bucket of parent[w]
          while (not bucket[par[w]].is_empty())
            {
              const long v = bucket[par[w]].remove_first();
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

      // For each node with 2+ predecessors, walk up from each pred
      for (long b = 0; b < num_reachable; ++b)
        {
          if (pred[b].size() < 2)
            continue;

          Node *bnode = vertex[b];

          // Walk up from each predecessor to idom(b)
          for (auto it = pred[b].get_it(); it.has_curr(); it.next_ne())
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

  // =========================================================================
  // Post-dominator computation (Lengauer-Tarjan on the reversed graph)
  // =========================================================================

  /** @class Lengauer_Tarjan_Post_Dominators
   *  @brief Computes post-dominator tree and post-dominance frontiers of a
   *         digraph using the Lengauer-Tarjan algorithm on the reversed graph.
   *
   *  A node `d` **post-dominates** `v` if every path from `v` to a
   *  designated exit node passes through `d`.  The **post-dominator tree**
   *  is the dominator tree of the reversed graph rooted at the exit node.
   *
   *  This class internally inverts the digraph with `invert_digraph()` and
   *  runs `Lengauer_Tarjan_Dominators` on the result.  Results are
   *  transparently mapped back to the original graph nodes.
   *
   *  The class caches its computation: calling multiple methods with
   *  the same graph and exit node reuses the previous result without
   *  recomputing.
   *
   *  @tparam GT The directed graph type.
   *  @tparam SA Arc filter class for the internal iterator.
   *
   *  ## Complexity
   *
   *  | Operation | Time | Space |
   *  |-----------|------|-------|
   *  | Post-dominator tree | O(E * alpha(V)) | O(V + E) |
   *  | Post-dominance frontiers | O(V + E) additional | O(V + |PDF|) |
   *
   *  ## Applications
   *
   *  - Control dependence graph (CDG) construction
   *  - Program dependence graph (PDG) construction
   *  - Compiler optimization and code motion
   *
   *  @note This class creates an internal copy of the reversed graph.
   *  @warning This class is NOT thread-safe.
   *
   *  @ingroup Graphs
   *  @see Lengauer_Tarjan_Dominators
   */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  class Lengauer_Tarjan_Post_Dominators
  {
    SA sa;

    using Node = typename GT::Node;
    using Arc = typename GT::Arc;

    GT rev;
    Lengauer_Tarjan_Dominators<GT, SA> dom;

    GT *gptr = nullptr;
    Node *exit_ptr = nullptr;
    bool is_computed = false;

    // Saved node mappings (orig ↔ rev) to avoid NODE_COOKIE conflicts
    DynMapTree<Node*, Node*> orig_to_rev;
    DynMapTree<Node*, Node*> rev_to_orig;

    /** @brief Map original node to reversed graph node. */
    [[nodiscard]] Node * to_rev(Node *orig) const
    {
      auto *p = orig_to_rev.search(orig);
      return p ? p->second : nullptr;
    }

    /** @brief Map reversed graph node to original node. */
    [[nodiscard]] Node * to_orig(Node *rev_n) const
    {
      auto *p = rev_to_orig.search(rev_n);
      return p ? p->second : nullptr;
    }

    /** @brief Ensure computation is up to date.
     *
     *  Inverts the graph and runs dominator computation on the reversed
     *  graph from the exit node.  If the computation was already done
     *  for the same graph and exit node, this is a no-op.
     *
     *  @param g The directed graph.
     *  @param exit_node Exit node (must not be null).
     */
    void ensure_computed(const GT & g, Node *exit_node)
    {
      ah_domain_error_if(exit_node == nullptr)
        << "Lengauer_Tarjan_Post_Dominators: exit node cannot be null";

      if (is_computed and gptr == &g and exit_ptr == exit_node)
        return;

      gptr = &const_cast<GT &>(g);
      exit_ptr = exit_node;
      is_computed = false;

      // Step 1: Invert the digraph (sets NODE_COOKIE for mapping)
      rev = invert_digraph(g);

      // Step 2: Save orig↔rev mappings before dom overwrites anything
      orig_to_rev = DynMapTree<Node*, Node*>();
      rev_to_orig = DynMapTree<Node*, Node*>();
      for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
        {
          auto orig = it.get_curr();
          auto rev_n = mapped_node<GT>(orig);
          orig_to_rev.insert(orig, rev_n);
          rev_to_orig.insert(rev_n, orig);
        }

      // Step 3: Run forward dominators on reversed graph from exit
      //         (discard return value; we only need the cached state)
      auto rev_exit = to_rev(exit_node);
      (void) dom.compute_idom(rev, rev_exit);

      is_computed = true;
    }

  public:
    /** @brief Construct a post-dominator computation instance.
     *
     *  @param __sa Arc filter functor. Defaults to Dft_Show_Arc<GT>.
     */
    Lengauer_Tarjan_Post_Dominators(SA __sa = SA()) noexcept
      : sa(std::move(__sa)), dom(sa)
    { /* empty */
    }

    /// Post-dominator instances should not be copied (they hold internal state)
    Lengauer_Tarjan_Post_Dominators(const Lengauer_Tarjan_Post_Dominators &) = delete;
    Lengauer_Tarjan_Post_Dominators &
    operator=(const Lengauer_Tarjan_Post_Dominators &) = delete;

    /// Move is allowed
    Lengauer_Tarjan_Post_Dominators(Lengauer_Tarjan_Post_Dominators &&) = default;
    Lengauer_Tarjan_Post_Dominators &
    operator=(Lengauer_Tarjan_Post_Dominators &&) = default;

    /** @brief Compute immediate post-dominators.
     *
     *  Returns a list of (node, ipdom) pairs for every node that can
     *  reach the exit node.  The exit node's ipdom is nullptr.
     *
     *  @param g The directed graph.
     *  @param exit_node Exit node for post-dominator computation.
     *  @return List of (node, immediate_post_dominator) pairs.
     *  @throws std::domain_error If exit_node is null.
     */
    [[nodiscard]] DynList<std::pair<Node*, Node*>>
    compute_ipdom(const GT & g, Node *exit_node)
    {
      ensure_computed(g, exit_node);

      auto rev_exit = to_rev(exit_node);
      auto rev_idoms = dom.compute_idom(rev, rev_exit);

      DynList<std::pair<Node*, Node*>> result;
      for (auto it = rev_idoms.get_it(); it.has_curr(); it.next_ne())
        {
          auto [rev_n, rev_id] = it.get_curr();
          auto orig_n = to_orig(rev_n);
          auto orig_id = rev_id ? to_orig(rev_id) : nullptr;
          result.append(std::make_pair(orig_n, orig_id));
        }
      return result;
    }

    /** @brief Build the post-dominator tree as a separate graph.
     *
     *  Creates a new directed graph `tree` where each node that can
     *  reach the exit in the original graph is represented, and arcs go
     *  from ipdom to child.  Nodes are mapped via NODE_COOKIE between
     *  the original graph and the tree.
     *
     *  @param g The directed graph.
     *  @param exit_node Exit node.
     *  @param[out] tree Output post-dominator tree (cleared first).
     *  @throws std::domain_error If exit_node is null.
     */
    void build_tree(const GT & g, Node *exit_node, GT & tree)
    {
      ensure_computed(g, exit_node);

      clear_graph(tree);

      auto rev_exit = to_rev(exit_node);
      auto rev_idoms = dom.compute_idom(rev, rev_exit);

      if (rev_idoms.is_empty())
        return;

      // Create tree nodes with mapping to original graph
      DynMapTree<Node*, Node*> orig_to_tree;
      for (auto it = rev_idoms.get_it(); it.has_curr(); it.next_ne())
        {
          auto [rev_n, rev_id] = it.get_curr();
          auto orig = to_orig(rev_n);
          auto tn = tree.insert_node(orig->get_info());
          GT::map_nodes(orig, tn);
          orig_to_tree.insert(orig, tn);
        }

      // Create arcs: ipdom(v) -> v
      for (auto it = rev_idoms.get_it(); it.has_curr(); it.next_ne())
        {
          auto [rev_n, rev_id] = it.get_curr();
          if (rev_id == nullptr)
            continue; // exit node has no ipdom
          auto orig = to_orig(rev_n);
          auto orig_id = to_orig(rev_id);
          tree.insert_arc(orig_to_tree.find(orig_id),
                          orig_to_tree.find(orig));
        }
    }

    /** @brief Get the immediate post-dominator of a specific node.
     *
     *  @param g The directed graph.
     *  @param exit_node Exit node.
     *  @param node The node whose ipdom is queried.
     *  @return Pointer to the immediate post-dominator, or nullptr if
     *          node is the exit or cannot reach the exit.
     *  @throws std::domain_error If exit_node or node is null.
     */
    [[nodiscard]] Node * get_ipdom(const GT & g, Node *exit_node, Node *node)
    {
      ah_domain_error_if(node == nullptr)
        << "Lengauer_Tarjan_Post_Dominators::get_ipdom: node cannot be null";
      ensure_computed(g, exit_node);

      auto rev_n = to_rev(node);
      if (rev_n == nullptr)
        return nullptr; // node not in graph

      auto rev_exit = to_rev(exit_node);
      auto rev_idom = dom.get_idom(rev, rev_exit, rev_n);
      return rev_idom ? to_orig(rev_idom) : nullptr;
    }

    /** @brief Test whether node d post-dominates node v.
     *
     *  Post-dominance is reflexive: every node post-dominates itself.
     *
     *  @param g The directed graph.
     *  @param exit_node Exit node.
     *  @param d Potential post-dominator.
     *  @param v Potential post-dominated node.
     *  @return true if d post-dominates v, false otherwise.
     *  @throws std::domain_error If exit_node is null.
     */
    [[nodiscard]] bool
    post_dominates(const GT & g, Node *exit_node, Node *d, Node *v)
    {
      ensure_computed(g, exit_node);

      if (d == v)
        return true; // reflexive

      auto rev_d = to_rev(d);
      auto rev_v = to_rev(v);
      if (rev_d == nullptr or rev_v == nullptr)
        return false;

      auto rev_exit = to_rev(exit_node);
      return dom.dominates(rev, rev_exit, rev_d, rev_v);
    }

    /** @brief Compute post-dominance frontiers for all reachable nodes.
     *
     *  The post-dominance frontier PDF(x) of a node x is the set of
     *  nodes y where x post-dominates a successor of y but does not
     *  strictly post-dominate y.  This is the key data structure for
     *  constructing the control dependence graph (CDG).
     *
     *  @param g The directed graph.
     *  @param exit_node Exit node.
     *  @return Map from each reachable node to its post-dominance frontier.
     *  @throws std::domain_error If exit_node is null.
     */
    [[nodiscard]] DynMapTree<Node*, DynList<Node*>>
    post_dominance_frontiers(const GT & g, Node *exit_node)
    {
      ensure_computed(g, exit_node);

      auto rev_exit = to_rev(exit_node);
      auto rev_df = dom.dominance_frontiers(rev, rev_exit);

      DynMapTree<Node*, DynList<Node*>> result;
      for (auto it = rev_df.get_it(); it.has_curr(); it.next_ne())
        {
          auto & [rev_n, rev_frontier] = it.get_curr();
          auto orig = to_orig(rev_n);
          DynList<Node*> orig_frontier;
          for (auto fi = rev_frontier.get_it(); fi.has_curr(); fi.next_ne())
            orig_frontier.append(to_orig(fi.get_curr()));
          result.insert(orig, std::move(orig_frontier));
        }
      return result;
    }

    /** @brief Build post-dominator tree (operator() alias).
     *
     *  @param g The directed graph.
     *  @param exit_node Exit node.
     *  @param[out] tree Output post-dominator tree.
     */
    void operator()(const GT & g, Node *exit_node, GT & tree)
    {
      build_tree(g, exit_node, tree);
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

    /** @brief Get the exit node of the last computation. */
    [[nodiscard]] Node * get_exit() const noexcept { return exit_ptr; }

    /// @}
  };


  // =========================================================================
  // Post-dominator free functions
  // =========================================================================

  /** @brief Compute immediate post-dominators of a digraph from an exit node.
   *
   *  Convenience free function wrapping Lengauer_Tarjan_Post_Dominators.
   *
   *  @tparam GT Graph type (must be directed).
   *  @tparam SA Arc filter type.
   *  @param g The directed graph.
   *  @param exit_node Exit node.
   *  @param sa Arc filter.
   *  @return List of (node, ipdom) pairs. Exit node's ipdom is nullptr.
   *
   *  @ingroup Graphs
   *  @see Lengauer_Tarjan_Post_Dominators
   */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  [[nodiscard]] DynList<std::pair<typename GT::Node*, typename GT::Node*>>
  compute_post_dominators(const GT & g, typename GT::Node *exit_node,
                          SA sa = SA())
  {
    return Lengauer_Tarjan_Post_Dominators<GT, SA>(sa)
      .compute_ipdom(g, exit_node);
  }

  /** @brief Build the post-dominator tree of a digraph.
   *
   *  Convenience free function wrapping Lengauer_Tarjan_Post_Dominators.
   *
   *  @tparam GT Graph type (must be directed).
   *  @tparam SA Arc filter type.
   *  @param g The directed graph.
   *  @param exit_node Exit node.
   *  @param[out] tree Output post-dominator tree (cleared first). Nodes are
   *              mapped to the original graph via GT::map_nodes.
   *  @param sa Arc filter.
   *
   *  @ingroup Graphs
   *  @see Lengauer_Tarjan_Post_Dominators
   */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  void build_post_dominator_tree(const GT & g, typename GT::Node *exit_node,
                                 GT & tree, SA sa = SA())
  {
    Lengauer_Tarjan_Post_Dominators<GT, SA>(sa)
      .build_tree(g, exit_node, tree);
  }

  /** @brief Compute post-dominance frontiers of a digraph.
   *
   *  Convenience free function wrapping Lengauer_Tarjan_Post_Dominators.
   *
   *  @tparam GT Graph type (must be directed).
   *  @tparam SA Arc filter type.
   *  @param g The directed graph.
   *  @param exit_node Exit node.
   *  @param sa Arc filter.
   *  @return Map from each reachable node to its post-dominance frontier.
   *
   *  @ingroup Graphs
   *  @see Lengauer_Tarjan_Post_Dominators
   */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  [[nodiscard]] DynMapTree<typename GT::Node*, DynList<typename GT::Node*>>
  compute_post_dominance_frontiers(const GT & g,
                                   typename GT::Node *exit_node,
                                   SA sa = SA())
  {
    return Lengauer_Tarjan_Post_Dominators<GT, SA>(sa)
      .post_dominance_frontiers(g, exit_node);
  }

} // end namespace Aleph

# endif // DOMINATORS_H
