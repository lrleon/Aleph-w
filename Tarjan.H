
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

/** @file Tarjan.H
 *  @brief Tarjan's algorithm for strongly connected components.
 *
 *  This file implements Tarjan's algorithm for finding strongly connected
 *  components (SCCs) in directed graphs. An SCC is a maximal subset of
 *  vertices where every vertex is reachable from every other vertex.
 *
 *  ## Algorithm Overview
 *
 *  Tarjan's algorithm uses a single DFS traversal with a stack to identify
 *  SCCs. Each node is assigned:
 *  - **index**: Order of first visit
 *  - **lowlink**: Lowest index reachable from subtree
 *
 *  When lowlink[v] == index[v], v is the root of an SCC.
 *
 *  ## Key Features
 *
 *  - Single DFS pass (optimal)
 *  - Returns SCCs in reverse topological order
 *  - Can build condensation graph (DAG of SCCs)
 *  - Detects cycles in digraphs
 *
 *  ## Complexity
 *
 *  | Operation | Time | Space |
 *  |-----------|------|-------|
 *  | Find all SCCs | O(V + E) | O(V) |
 *  | Build condensation | O(V + E) | O(V + E) |
 *
 *  ## Applications
 *
 *  - Detecting cycles in directed graphs
 *  - Computing 2-SAT satisfiability
 *  - Finding strongly connected components
 *  - Building DAG condensation for topological analysis
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  List_Digraph<Node, Arc> g;
 *  // ... build digraph ...
 *
 *  DynList<DynList<Node*>> sccs;
 *  Tarjan_Connected_Components<decltype(g)> tarjan;
 *  tarjan.connected_components(g, sccs);
 *
 *  for (auto& scc : sccs) {
 *    std::cout << "SCC with " << scc.size() << " nodes\n";
 *  }
 *  ```
 *
 *  @see Kosaraju.H Alternative SCC algorithm (two DFS passes)
 *  @see topological_sort.H Topological ordering for DAGs
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

#ifndef TARJAN_H
#define TARJAN_H

#include <tpl_dynListStack.H>
#include <tpl_dynSetTree.H>
#include <htlist.H>
#include <tpl_graph_utils.H>
#include <tpl_find_path.H>
#include <ah-errors.H>

namespace Aleph {
/**
 * @class Tarjan_Connected_Components
 * @brief Computes strongly connected components (SCCs) in a directed graph
 *        using Tarjan's algorithm.
 *
 * This class implements Tarjan's algorithm for finding strongly connected
 * components in directed graphs. A strongly connected component is a maximal
 * set of vertices where every vertex is reachable from every other vertex in
 * the component.
 *
 * Tarjan's algorithm performs a single depth-first traversal to identify all
 * SCCs in O(V + E) time complexity, where V is the number of vertices and E
 * is the number of edges.
 *
 * @tparam GT The directed graph type (based on List_Graph or similar).
 * @tparam Itor The iterator template for traversing adjacent nodes/arcs.
 *              Defaults to Out_Iterator for outgoing arcs.
 * @tparam SA The arc filter class used by the internal iterator.
 *            Defaults to Dft_Show_Arc<GT> which shows all arcs.
 *
 * The class provides multiple overloaded methods for computing SCCs in
 * different output formats:
 * - As a list of subgraphs (mapped copies of original graph components)
 * - As a list of node lists (lightweight, no graph copying)
 * - As a list of component sizes (counts only)
 *
 * Additional functionality:
 * - Cycle detection: `has_cycle()`, `is_dag()`
 * - Cycle construction: `compute_cycle()`
 * - Strong connectivity testing: `test_connectivity()`
 *
 * Usage example:
 * @code
 * List_Digraph<int> g;
 * // ... build graph ...
 *
 * Tarjan_Connected_Components<List_Digraph<int>> tarjan;
 *
 * // Get SCCs as node lists
 * auto sccs = tarjan.connected_components(g);
 * for (auto & scc : sccs)
 *   {
 *     // Process each SCC
 *     for (auto node : scc)
 *       process(node);
 *   }
 *
 * // Check for cycles
 * if (tarjan.has_cycle(g))
 *   std::cout << "Graph contains a cycle" << '\n';
 * @endcode
 *
 * @note The algorithm uses node bits (Aleph::Min, Aleph::Depth_First) and
 *       node counters/cookies for bookkeeping during traversal.
 *
 * @warning This class is NOT thread-safe. Concurrent access to the same
 *          instance or to the same graph from multiple threads will result
 *          in undefined behavior.
 *
 * @see Compute_Cycle_In_Digraph
 * @ingroup Graphs
 * @author Leandro Rabindranath León (lrleon at ula dot ve)
 */
template <class GT, template <typename, class> class Itor = Out_Iterator, class SA = Dft_Show_Arc<GT>>
class Tarjan_Connected_Components
{
  SA sa_;

  GT *g_ptr_ = nullptr;

  DynListStack<typename GT::Node *> stack_;

  long df_count_ = 0;
  mutable size_t n_ = 0;  // number of nodes in the graph

  Path<GT> *path_ptr_ = nullptr;

public:
  /// Tarjan instances should not be copied (they hold internal traversal state)
  Tarjan_Connected_Components(const Tarjan_Connected_Components &) = delete;

  Tarjan_Connected_Components &operator = (const Tarjan_Connected_Components &) = delete;

  /// Move is allowed
  Tarjan_Connected_Components(Tarjan_Connected_Components &&) = default;

  Tarjan_Connected_Components &operator = (Tarjan_Connected_Components &&) = default;

  /** Constructs a Tarjan algorithm instance for computing strongly connected components.

      @param[in] __sa Arc filter functor used by the internal iterator.
                      Defaults to Dft_Show_Arc<GT> which shows all arcs.
   */
  Tarjan_Connected_Components(SA __sa = SA()) noexcept : sa_(std::move(__sa))
  { /* empty */
  }

private:
  /** @brief Functor to initialize node metadata for Tarjan traversal.
   *
   *  Resets all bits, counters (df), and sets low to -1 for each node.
   */
  struct Init_Tarjan_Node
  {
    void operator () (const GT &g, typename GT::Node *p) const noexcept
    {
      g.reset_bits(p);
      g.reset_counter(p);  // initialize df
      low<GT>(p) = -1;     // initialize low
    }
  };

  /** @brief Check if a node is currently on the traversal stack.
   *  @param p The node to check.
   *  @return true if the node is on the stack, false otherwise.
   */
  static bool is_node_in_stack(typename GT::Node *p) noexcept
  {
    assert(p != nullptr);
    return IS_NODE_VISITED(p, Aleph::Min);
  }

  /** @brief Initialize a node and push it onto the traversal stack.
   *
   *  Sets the Min and Depth_First bits, assigns df and low values,
   *  and pushes the node onto the stack.
   *
   *  @param p The node to initialize and push.
   */
  void init_node_and_push_in_stack(typename GT::Node *p)
  {
    assert(not is_node_in_stack(p));

    stack_.push(p);
    NODE_BITS(p).set_bit(Aleph::Min, true);
    NODE_BITS(p).set_bit(Aleph::Depth_First, true);
    df<GT>(p) = low<GT>(p) = df_count_++;
  }

  /** @brief Pop a node from the traversal stack.
   *  @return The popped node.
   */
  typename GT::Node *pop_from_stack()
  {
    auto ret = stack_.pop();
    NODE_BITS(ret).set_bit(Aleph::Min, false);

    return ret;
  }

  /** @brief Recursive DFS to find SCCs and build mapped subgraphs.
   *
   *  For each SCC found, creates a mapped copy subgraph and appends
   *  it to block_list.
   *
   *  @param v The current node being visited.
   *  @param[out] blk_list block list where the SCC will be put
   */
  long scc_by_blocks(typename GT::Node *v, DynList<GT> &blk_list)
  {
    init_node_and_push_in_stack(v);

    // depth-first traverse all nodes connected to v
    for (Itor<GT, SA> it(v, sa_); it.has_curr(); it.next_ne())
      if (auto w = g_ptr_->get_tgt_node(it.get_curr()); not IS_NODE_VISITED(w, Aleph::Depth_First))
        {
          // map_nodes() called inside scc_by_blocks corrupts low<GT>(w)
          // via NODE_COOKIE when w's SCC is finalized.  Save the result
          // before that corruption can propagate to v.
          const long w_low = scc_by_blocks(w, blk_list);
          low<GT>(v) = std::min(low<GT>(v), w_low);
        }
      else if (is_node_in_stack(w))
        // if on stack ==> v was visited before w
        low<GT>(v) = std::min(low<GT>(v), df<GT>(w));

    // Save low[v] before the SCC-finalization loop overwrites NODE_COOKIE
    // (via map_nodes) and NODE_COUNTER (with blk_idx) on every node in v's SCC.
    const long result = low<GT>(v);

    if (low<GT>(v) == df<GT>(v))                 // first visited node of the block?
      {                                          // yes ==> pop block nodes from stack
        const size_t blk_idx = blk_list.size();  // capture by value, not reference
        GT &blk = blk_list.append(GT());

        while (true)  // remove block from stack until v is removed
          {
            auto p = pop_from_stack();
            auto q = blk.insert_node(p->get_info());
            *q = *p;  // copy node content
            NODE_COOKIE(p) = NODE_COOKIE(q) = nullptr;
            GT::map_nodes(p, q);
            NODE_COUNTER(p) = NODE_COUNTER(q) = static_cast<long>(blk_idx);
            if (p == v)
              break;
          }
      }
    return result;
  }

  /** @brief Recursive DFS to find SCCs and collect nodes into lists.
   *
   *  For each SCC found, collects the node pointers into a list
   *  and appends it to list_list_ptr.
   *
   *  @param v The current node being visited.
   *  @param blks block list where the SCC node lists will be put
   */
  void scc_by_lists(typename GT::Node *v, DynList<DynList<typename GT::Node *>> &blks)
  {
    init_node_and_push_in_stack(v);

    // depth traversal all nodes connected to v
    for (Itor<GT, SA> it(v, sa_); it.has_curr(); it.next_ne())
      if (auto w = g_ptr_->get_tgt_node(it.get_curr()); not IS_NODE_VISITED(w, Aleph::Depth_First))
        {
          scc_by_lists(w, blks);
          low<GT>(v) = std::min(low<GT>(v), low<GT>(w));
        }
      else if (is_node_in_stack(w))
        // if on stack ==> v was visited before w
        low<GT>(v) = std::min(low<GT>(v), df<GT>(w));

    if (low<GT>(v) == df<GT>(v))  // first visited node of the block?
      {                           // yes pop out block nodes that are on stack
        auto &l = blks.append(DynList<typename GT::Node *>());
        while (true)  // remove block from stack until reaching v
          {
            auto p = pop_from_stack();
            l.append(p);
            if (p == v)
              break;
          }
      }
  }

  /** @brief Recursive DFS to find SCCs and count their sizes.
   *
   *  For each SCC found, counts the number of nodes and appends
   *  the count to list_len_ptr.
   *
   *  @param v The current node being visited.
   *  @param sizes list of block sizes in number of nodes
   */
  void scc_by_len(typename GT::Node *v, DynList<size_t> &sizes)
  {
    init_node_and_push_in_stack(v);

    // depth traverse all nodes connected to v
    for (Itor<GT, SA> it(v, sa_); it.has_curr(); it.next_ne())
      if (auto w = g_ptr_->get_tgt_node(it.get_curr()); not IS_NODE_VISITED(w, Aleph::Depth_First))
        {
          scc_by_len(w, sizes);
          low<GT>(v) = std::min(low<GT>(v), low<GT>(w));
        }
      else if (is_node_in_stack(w))
        // if on stack ==> v was visited before w
        low<GT>(v) = std::min(low<GT>(v), df<GT>(w));

    if (low<GT>(v) == df<GT>(v))  // first visited node of the block?
      {                           // yes, pop out block nodes that are on the stack
        size_t count = 0;
        while (true)  // remove block from the stack until reaching v
          {
            auto p = pop_from_stack();
            ++count;

            if (p == v)
              break;
          }
        sizes.append(count);
      }
  }

  /** @brief Initialize internal state for a Tarjan traversal.
   *
   *  Resets node metadata, clears the stack, and stores a pointer
   *  to the graph.
   *
   *  @param g The graph to traverse.
   *
   *  @note Uses const_cast internally because the algorithm modifies
   *        node metadata (bits, counters, cookies) but not the graph
   *        structure itself.
   */
  void init_tarjan(const GT &g)
  {
    Operate_On_Nodes<GT, Init_Tarjan_Node>()(g);  // initialize bits, df and low
    df_count_ = 0;                                 // visitor counter
    stack_.empty();
    n_ = g.get_num_nodes();

    g_ptr_ = &const_cast<GT &>(g);
  }

  /** @brief Recursive DFS to detect if a cycle exists starting from v.
   *
   *  Detects cycles including self-loops (cycles of length 1) and
   *  strongly connected components with 2+ nodes.
   *
   *  @param v The current node being visited.
   *  @return true if a cycle is found, false otherwise.
   */
  bool has_cycle(typename GT::Node *v)
  {
    init_node_and_push_in_stack(v);

    // depth traverse all nodes connected to v
    for (Itor<GT, SA> it(v, sa_); it.has_curr(); it.next_ne())
      {
        auto w = g_ptr_->get_tgt_node(it.get_curr());

        // Check for self-loop (a cycle of length 1)
        if (w == v)
          return true;

        if (not IS_NODE_VISITED(w, Aleph::Depth_First))
          {
            if (has_cycle(w))
              return true;

            low<GT>(v) = std::min(low<GT>(v), low<GT>(w));
          }
        else if (is_node_in_stack(w))
          // if on stack ==> v was visited before w
          low<GT>(v) = std::min(low<GT>(v), df<GT>(w));
      }

    if (low<GT>(v) == df<GT>(v))  // first visited node of block?
      {                           // yes, check if component has two or more nodes
        size_t count = 0;
        while (true)
          {
            ++count;
            if (pop_from_stack() == v)
              break;
          }

        return count >= 2;  // if count >= 2 ==> there is a cycle
      }

    return false;  // everything was covered without finding a cycle
  }

  /** @brief Build a cycle path from a strongly connected block.
   *
   *  Takes a mapped strongly connected subgraph and constructs the
   *  cycle path in path_ptr using the node mapping table.
   *
   *  @param block The strongly connected subgraph (mapped from original).
   *  @param table Bidirectional mapping between original and block nodes.
   */
  void build_path(const GT &block, DynMapAvlTree<typename GT::Node *, typename GT::Node *> &table)
  {
    // Search for a cycle in the block
    auto a = block.get_first_arc();
    auto start = block.get_tgt_node(a);
    auto end = block.get_src_node(a);
    assert(start != end);

    auto aux_path = Directed_Find_Path<GT, Itor, SA>(block, sa_).dfs(start, end);
    assert(not aux_path.is_empty());  // since it's connected it must be found

    // aux_path is about the mapped block. We need to translate it back
    // to the original graph using the mapping table.
    path_ptr_->empty();
    for (typename Path<GT>::Iterator i(aux_path); i.has_curr(); i.next_ne())
      path_ptr_->append_directed(table.find(i.get_current_node_ne()));

    path_ptr_->append_directed(path_ptr_->get_first_node());
  }

  /** @brief Recursive DFS to find and construct a cycle starting from v.
   *
   *  If a cycle is found (including self-loops), constructs it in path_ptr.
   *
   *  @param v The current node being visited.
   *  @return true if a cycle is found and stored in path_ptr, false otherwise.
   */
  bool build_cycle(typename GT::Node *v)
  {
    init_node_and_push_in_stack(v);

    // depth traverse all nodes connected to v
    for (Itor<GT, SA> it(v, sa_); it.has_curr(); it.next_ne())
      {
        auto w = g_ptr_->get_tgt_node(it.get_curr());

        // Check for self-loop (a cycle of length 1)
        if (w == v)
          {
            // Build a simple self-loop path: v -> v
            path_ptr_->empty();
            path_ptr_->init(v);
            path_ptr_->append_directed(v);
            return true;
          }

        if (not IS_NODE_VISITED(w, Aleph::Depth_First))
          {
            if (build_cycle(w))
              return true;

            low<GT>(v) = std::min(low<GT>(v), low<GT>(w));
          }
        else if (is_node_in_stack(w))
          // if on stack ==> v was visited before w
          low<GT>(v) = std::min(low<GT>(v), df<GT>(w));
      }

    if (low<GT>(v) == df<GT>(v))  // first visited node of the block?
      {
        GT blk;  // auxiliary graph

        // g node mapping to blk (cookies are busy)
        DynMapAvlTree<typename GT::Node *, typename GT::Node *> table;

        // pop nodes from stack and insert them into auxiliary block
        while (true)  // pop the component and insert into blk
          {
            auto p = pop_from_stack();
            auto q = blk.insert_node();
            *q = *p;  // copy node content
            table.insert(q, p);
            table.insert(p, q);
            if (p == v)
              break;
          }

        if (blk.get_num_nodes() == 1)
          return false;  // single node without self-loop ==> no cycle

        // finish constructing the block with the arcs
        for (typename GT::Node_Iterator j(blk); j.has_curr(); j.next_ne())
          {
            auto bsrc = j.get_curr();
            auto gsrc = table.find(bsrc);

            // traverse the arcs of gsrc
            for (Itor<GT, SA> k(gsrc, sa_); k.has_curr(); k.next_ne())
              {
                auto ga = k.get_curr();
                auto gtgt = g_ptr_->get_tgt_node(ga);
                auto ptr = table.search(gtgt);
                if (ptr == nullptr)  // arc of the block?
                  continue;

                auto ta = blk.insert_arc(bsrc, ptr->second);
                *ta = *ga;  // copy arc content
              }
          }

        build_path(blk, table);

        return true;
      }

    assert(path_ptr_->is_empty());

    return false;
  }

  /** @brief Recursive DFS to test if the graph is strongly connected.
   *
   *  Returns true if and only if all nodes belong to a single SCC.
   *  The algorithm detects early if multiple SCCs exist.
   *
   *  @param v The current node being visited.
   *  @return true if all visited nodes so far form one SCC, false otherwise.
   */
  bool is_connected(typename GT::Node *v)
  {
    init_node_and_push_in_stack(v);

    // depth-first traverse all nodes connected to v
    for (Itor<GT, SA> it(v, sa_); it.has_curr(); it.next_ne())
      {
        if (auto w = g_ptr_->get_tgt_node(it.get_curr()); not IS_NODE_VISITED(w, Aleph::Depth_First))
          {
            if (not is_connected(w))
              return false;

            low<GT>(v) = std::min(low<GT>(v), low<GT>(w));
          }
        else if (is_node_in_stack(w))
          low<GT>(v) = std::min(low<GT>(v), df<GT>(w));
      }

    if (low<GT>(v) == df<GT>(v))  // first visited node of the block?
      {                           // pop nodes from stack until v is found
        while (pop_from_stack() != v)
          ;

        return stack_.is_empty();
      }

    return true;
  }

public:
  /** Computes the strongly connected components (SCCs) of a digraph.

      connected_components() runs Tarjan's algorithm on g and stores one
      mapped subdigraph per SCC in blk_list (both nodes and arcs are mapped
      back to g). Arcs that connect different SCCs are collected in arc_list.
      The number of blocks depends on the cyclic structure: an acyclic
      digraph yields one singleton block per node, while a single strongly
      connected digraph yields exactly one block.

      The algorithm performs a single depth-first search and marks progress
      with the Aleph::Depth_First and Aleph::Min node bits.

      @param[in] g the digraph whose SCCs are computed.
      @param[out] blk_list list of mapped subdigraphs, one per SCC of g.
      @param[out] arc_list arcs connecting distinct SCCs (inter-block arcs).
      @throw bad_alloc if there is not enough memory to build a block or
      insert into a list.
  */
  void connected_components(const GT &g, DynList<GT> &blk_list, DynList<typename GT::Arc *> &arc_list)
  {
    init_tarjan(g);

    for (typename GT::Node_Iterator it(g); df_count_ < n_; it.next_ne())
      if (auto v = it.get_curr(); not IS_NODE_VISITED(v, Aleph::Depth_First))
        scc_by_blocks(v, blk_list);

    assert(stack_.is_empty());

    // traverse each partial subgraph and add its arcs
    for (typename DynList<GT>::Iterator i(blk_list); i.has_curr(); i.next_ne())
      {  // traverse all nodes of the block
        GT &blk = i.get_curr();
        for (typename GT::Node_Iterator j(blk); j.has_curr(); j.next_ne())
          {
            auto bsrc = j.get_curr();
            auto gsrc = mapped_node<GT>(bsrc);

            // traverse arcs of gsrc
            for (Itor<GT, SA> k(gsrc, sa_); k.has_curr(); k.next_ne())
              {
                auto ga = k.get_curr();
                auto gtgt = g_ptr_->get_tgt_node(ga);
                if (NODE_COUNTER(gsrc) != NODE_COUNTER(gtgt))
                  {  // inter-block arc ==> add it to arc_list
                    arc_list.append(ga);
                    continue;
                  }

                // insert and map the arc in the sub-block
                auto btgt = mapped_node<GT>(gtgt);
                auto ba = blk.insert_arc(bsrc, btgt);
                *ba = *ga;  // copy arc content
                GT::map_arcs(ga, ba);
              }
          }
      }
  }

  /** Computes the strongly connected components (SCCs) of a digraph.

      This overloaded version of connected_components() runs Tarjan's
      algorithm on g and stores each SCC as a list of node pointers in blks.

      Each inner list contains the nodes belonging to one strongly connected
      component.

      Use this function if you do not need mapped subgraph copies of the
      SCCs. Since mapping is avoided, this routine is faster and uses less
      memory than the subgraph overload; the trade-off is that it does not
      compute the arcs that interconnect the blocks.

      The algorithm performs a single depth-first search and marks progress
      with the Aleph::Depth_First and Aleph::Min node bits.

      @param[in] g the digraph whose SCCs are computed.
      @param[out] blks list of node lists; each inner list holds the nodes of
      one SCC.

      @throw bad_alloc if there is not enough memory to insert into the list.
  */
  void connected_components(const GT &g, DynList<DynList<typename GT::Node *>> &blks)
  {
    init_tarjan(g);
    for (typename GT::Node_Iterator it(g); df_count_ < n_; it.next_ne())
      if (auto v = it.get_curr(); not IS_NODE_VISITED(v, Aleph::Depth_First))
        scc_by_lists(v, blks);
  }

  [[nodiscard]] DynList<DynList<typename GT::Node *>> connected_components(const GT &g)
  {
    DynList<DynList<typename GT::Node *>> blks;
    connected_components(g, blks);
    return blks;
  }

  /** Returns the number of strongly connected components in the graph.

      This is a convenience method that computes SCCs and returns only
      the count, which is more efficient than getting the full list
      when only the count is needed.

      @param[in] g The directed graph.
      @return The number of strongly connected components.
      @throw bad_alloc if there is not enough memory.
   */
  [[nodiscard]] size_t num_connected_components(const GT &g)
  {
    DynList<size_t> sizes;
    connected_components(g, sizes);
    return sizes.size();
  }

  /** Computes the sizes of the strongly connected components (SCCs).

      This overloaded version of connected_components() runs Tarjan's
      algorithm on g and records the size (node count) of each SCC in blks.

      The algorithm performs a single depth-first search and marks progress
      with the Aleph::Depth_First and Aleph::Min node bits.

      @param[in] g the digraph whose SCCs are computed.
      @param[out] blks list of sizes; each entry is the node count of one SCC.

      @throw bad_alloc if there is not enough memory to insert into the list.
  */
  void connected_components(const GT &g, DynList<size_t> &blks)
  {
    init_tarjan(g);
    for (typename GT::Node_Iterator it(g); df_count_ < n_; it.next_ne())
      if (auto v = it.get_curr(); not IS_NODE_VISITED(v, Aleph::Depth_First))
        scc_by_len(v, blks);
  }

  /** \overload
   *
   *  Compute strongly connected components and return them as mapped subgraphs
   *  with bridges (arcs connecting different SCCs).
   *
   *  @param[in] g The directed graph.
   *  @param[out] blk_list List of strongly connected components as mapped subgraphs.
   *  @param[out] arc_list List of bridge arcs connecting different SCCs.
   */
  void operator () (const GT &g, DynList<GT> &blk_list, DynList<typename GT::Arc *> &arc_list)
  {
    connected_components(g, blk_list, arc_list);
  }

  /** \overload
   *
   *  Compute strongly connected components and return them as lists of nodes.
   *
   *  @param[in] g The directed graph.
   *  @param[out] blks List of strongly connected components, each as a list of nodes.
   */
  void operator () (const GT &g, DynList<DynList<typename GT::Node *>> &blks)
  {
    connected_components(g, blks);
  }

  DynList<DynList<typename GT::Node *>> operator () (const GT &g)
  {
    return connected_components(g);
  }

  /** \overload
   *
   *  Compute strongly connected components and return them as mapped subgraphs
   *  with bridges, using DynDlist containers.
   *
   *  @param[in] g The directed graph.
   *  @param[out] blk_list List of strongly connected components as mapped subgraphs.
   *  @param[out] arc_list List of bridge arcs connecting different SCCs.
   */
  void operator () (const GT &g, DynDlist<GT> &blk_list, DynDlist<typename GT::Arc *> &arc_list)
  {
    DynList<GT> blist;
    DynList<typename GT::Arc *> alist;
    connected_components(g, blist, alist);

    for (typename DynList<GT>::Iterator it(blist); it.has_curr(); it.next_ne())
      {
        GT &curr = it.get_curr();
        GT &block = blk_list.append(GT());
        curr.swap(block);
      }

    for (typename DynList<typename GT::Arc *>::Iterator it(alist); it.has_curr(); it.next_ne())
      arc_list.append(it.get_curr());
  }

  /** \overload
   *
   *  Compute strongly connected components and return them as lists of nodes,
   *  using DynDlist containers.
   *
   *  @param[in] g The directed graph.
   *  @param[out] blks List of strongly connected components, each as a list of nodes.
   */
  void operator () (const GT &g, DynDlist<DynDlist<typename GT::Node *>> &blks)
  {
    DynList<DynList<typename GT::Node *>> b;
    connected_components(g, b);

    for (typename DynList<DynList<typename GT::Node *>>::Iterator it(b); it.has_curr(); it.next_ne())
      {
        auto &tgt_list = blks.append(DynDlist<typename GT::Node *>());

        auto &blk = it.get_curr();
        while (not blk.is_empty())
          tgt_list.append(blk.remove_first());
      }
  }

  /** Determines whether the digraph contains at least one cycle.

      Uses Tarjan's algorithm to detect if the directed graph has any cycles.
      A directed graph has a cycle if there exists a path from a vertex back
      to itself.

      @param[in] g The directed graph to check for cycles.
      @return true if the graph contains at least one cycle, false otherwise.
      @throw bad_alloc if there is not enough memory.
      @see is_dag(), compute_cycle()
   */
  [[nodiscard]] bool has_cycle(const GT &g)
  {
    init_tarjan(g);
    for (typename GT::Node_Iterator it(g); df_count_ < n_; it.next_ne())
      if (auto v = it.get_curr(); not IS_NODE_VISITED(v, Aleph::Depth_First))
        if (has_cycle(v))
          return true;

    return false;
  }

  /** Determines whether the directed graph is acyclic (a DAG).

      A Directed Acyclic Graph (DAG) is a directed graph with no cycles.
      This method is equivalent to `!has_cycle(g)`.

      @param[in] g The directed graph to check.
      @return true if the graph is a DAG (has no cycles), false otherwise.
      @throw bad_alloc if there is not enough memory.
      @see has_cycle(), compute_cycle()
   */
  [[nodiscard]] bool is_dag(const GT &g)
  {
    return not has_cycle(g);
  }

  /** Finds and constructs a cycle in the digraph, if one exists.

      Uses Tarjan's algorithm to detect a cycle in the directed graph.
      If a cycle is found, it is stored in the provided path parameter.

      @param[in] g The directed graph to search for cycles.
      @param[out] path A Path object where the cycle will be stored if found.
                       The path will form a cycle (start node == end node).
                       If no cycle exists, path is emptied.
      @return true if a cycle was found and stored in path, false otherwise.
      @throw bad_alloc if there is not enough memory.
      @see has_cycle(), is_dag()
   */
  bool compute_cycle(const GT &g, Path<GT> &path)
  {
    init_tarjan(g);
    path_ptr_ = &path;
    path_ptr_->set_graph(g);

    for (typename GT::Node_Iterator it(g); df_count_ < n_; it.next_ne())
      if (auto v = it.get_curr(); not IS_NODE_VISITED(v, Aleph::Depth_First))  // p visited?
        if (build_cycle(v))
          return true;

    path.empty();
    return false;
  }

  /** Finds and constructs a cycle starting from a specific node, if one exists.

      Uses Tarjan's algorithm starting from the specified source node to
      detect a cycle. If a cycle is found that includes the source node,
      it is stored in the provided path parameter.

      @param[in] g The directed graph to search for cycles.
      @param[in] src The source node from which to start the cycle search.
      @param[out] path A Path object where the cycle will be stored if found.
                       The path will form a cycle containing the source node.
      @return true if a cycle was found starting from src, false otherwise.
      @throw bad_alloc if there is not enough memory.
      @see has_cycle(), compute_cycle(const GT &, Path<GT> &)
   */
  [[nodiscard]] bool compute_cycle(const GT &g, typename GT::Node *src, Path<GT> &path)
  {
    ah_domain_error_if(src == nullptr) << "compute_cycle: source node cannot be null";

    init_tarjan(g);
    path_ptr_ = &path;
    path_ptr_->set_graph(g);
    return build_cycle(src);
  }

  /** Tests whether the digraph is strongly connected.

      A directed graph is strongly connected if there is a path from every
      vertex to every other vertex. This method uses Tarjan's algorithm to
      determine strong connectivity.

      @param[in] g The directed graph to test.
      @return true if the graph is strongly connected, false otherwise.
      @throw bad_alloc if there is not enough memory.
      @see connected_components()
   */
  [[nodiscard]] bool test_connectivity(const GT &g)
  {
    init_tarjan(g);

    // In a strongly connected graph, a single DFS from any node should reach
    // all nodes. If we have to start from a second unvisited node, it means
    // the graph has multiple disconnected components.
    bool started = false;
    for (typename GT::Node_Iterator it(g); df_count_ < n_; it.next_ne())
      if (auto v = it.get_curr(); not IS_NODE_VISITED(v, Aleph::Depth_First))
        {
          if (started)  // Second DFS root → not strongly connected
            return false;
          started = true;
          if (not is_connected(v))
            return false;
        }

    assert(stack_.is_empty());

    return true;
  }

  /// @name Accessors
  /// @{

  /** Returns the arc filter used by this instance.
      @return Reference to the arc filter.
   */
  SA &get_filter() noexcept
  {
    return sa_;
  }

  /** Returns the arc filter used by this instance (const version).
      @return Const reference to the arc filter.
   */
  const SA &get_filter() const noexcept
  {
    return sa_;
  }

  /** Check if a computation has been performed.
      @return true if a graph has been processed, false otherwise.
   */
  [[nodiscard]] bool has_computation() const noexcept
  {
    return g_ptr_ != nullptr;
  }

  /** Get the graph of the last computation.
      @return Pointer to the graph, or nullptr if no computation done.
   */
  [[nodiscard]] GT *get_graph() const noexcept
  {
    return g_ptr_;
  }

  /// @}
};

/** Determines if a digraph contains a cycle and constructs it.

    Compute_Cycle_In_Digraph() takes a digraph g, determines if it
    contains a cycle, and if so, constructs a path containing the cycle
    in question.

    The class is based on Tarjan's algorithm.

    The function uses two type parameters:
    -# GT: the digraph class.
    -# SA: the arc filter used by the internal iterator.

    The class delegates to Tarjan_Connected_Components, which marks traversal
    progress with the Aleph::Depth_First and Aleph::Min node bits.

    @ingroup Graphs

    @note This class is intended for directed graphs. Behavior on undirected
          graphs is undefined.
*/
template <class GT, template <typename, class> class Itor = Out_Iterator, class SA = Dft_Show_Arc<GT>>
class Compute_Cycle_In_Digraph
{
  SA sa_;

public:
  /** Constructs a cycle computation instance with an arc filter.

      @param[in] __sa Arc filter functor.
                      Defaults to Dft_Show_Arc<GT>.
   */
  Compute_Cycle_In_Digraph(SA __sa = SA()) : sa_(std::move(__sa))
  { /* empty */
  }

  /** Invokes the computation of a cycle in a digraph.

      @param[in] g the graph on which to compute its blocks.
      @param[out] path path that defines the cycle.
      @return true if the graph contains a cycle; false otherwise.
      In the latter case the value of path is indeterminate.
      @throw bad_alloc if there is not enough memory.
  */
  [[nodiscard]] bool operator () (const GT &g, Path<GT> &path) const
  {
    Tarjan_Connected_Components<GT, Itor, SA> tarjan(sa_);

    return tarjan.compute_cycle(g, path);
  }

  /** Computes and returns a cycle in the digraph.

      @param[in] g The graph to search for cycles.
      @return A Path containing the cycle if found, empty path otherwise.
      @throw bad_alloc if there is not enough memory.
   */
  [[nodiscard]] Path<GT> operator () (const GT &g) const
  {
    Path<GT> ret(g);
    Tarjan_Connected_Components<GT, Itor, SA>(sa_).compute_cycle(g, ret);
    return ret;
  }

  /** Computes and returns a cycle starting from a specific node.

      @param[in] g The graph to search for cycles.
      @param[in] src The source node to start the search from (must not be null).
      @return A Path containing the cycle if found, empty path otherwise.
      @throw domain_error if src is null.
      @throw bad_alloc if there is not enough memory.
   */
  [[nodiscard]] Path<GT> operator () (const GT &g, typename GT::Node *src) const
  {
    Path<GT> ret(g);
    (void) Tarjan_Connected_Components<GT, Itor, SA>(sa_).compute_cycle(g, src, ret);
    return ret;
  }
};
}  // end namespace Aleph

#endif  // TARJAN_H
