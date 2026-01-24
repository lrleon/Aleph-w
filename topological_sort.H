
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


/** @file topological_sort.H
    @brief Topological sorting algorithms for directed acyclic graphs (DAGs).

    This header provides two implementations of topological sorting:

    1. `Topological_Sort`: Uses depth-first search with postfix ordering.
       Visits nodes recursively and inserts them in reverse postorder.

    2. `Q_Topological_Sort`: Uses breadth-first search (Kahn's algorithm).
       Repeatedly removes source nodes (in-degree 0) and updates degrees.
       Also provides rank-based ordering for parallel execution.

    Both algorithms assume the input is a DAG. Behavior is undefined
    if the graph contains cycles.

    @ingroup Graphs
 *  @author Leandro Rabindranath León
*/

#ifndef TOPOLOGICAL_SORT_H
#define TOPOLOGICAL_SORT_H

#include <tpl_dynListQueue.H>
#include <tpl_graph.H>

namespace Aleph {

/** @brief Computes topological ordering using depth-first search.

    This class performs topological sorting by recursively visiting
    nodes in depth-first order and inserting them in postfix order
    (after all successors have been visited).

    The resulting order is a valid topological ordering where for
    every directed edge (u, v), u appears before v in the list.

    @tparam GT The directed graph type.
    @tparam Itor Iterator type for traversing outgoing arcs (default: Out_Iterator).
    @tparam SA Arc filter/selector (default: Dft_Show_Arc<GT>).

    @ingroup Graphs
*/
template <class GT,
          template <typename, class> class Itor = Out_Iterator,
          class SA = Dft_Show_Arc<GT>>
class Topological_Sort
{
  SA & sa;
  const GT * gptr;

public:

  /** @brief Constructor with rvalue arc filter.
      @param __sa Arc filter/selector (moved).
  */
  Topological_Sort(SA && __sa = SA())
    : sa(__sa), gptr(nullptr) { /* empty */ }

  /** @brief Constructor with lvalue arc filter.
      @param __sa Arc filter/selector (referenced).
  */
  Topological_Sort(SA & __sa)
    : sa(__sa), gptr(nullptr)  { /* empty */ }

private:

  /// @brief Recursive helper for DFS-based topological sort.
  /// @param curr Current node being visited.
  /// @param list Output list accumulating sorted nodes.
  template <template <class> class List>
  void topological_sort(typename GT::Node * curr,
                        List<typename GT::Node*> & list)
  {
    assert(gptr != nullptr);

    if (IS_NODE_VISITED(curr, Depth_First))
      return;

    NODE_BITS(curr).set_bit(Depth_First, 1); // mark as visited

    const auto & n = gptr->get_num_nodes();

    // recursively visit adjacent nodes in postfix order
    for (Itor<GT,SA> it(curr,sa); it.has_curr() and list.size() < n; it.next_ne())
      topological_sort(it.get_tgt_node_ne(), list);

    list.insert(curr); // postfix insertion of node that became a sink
  }

public:

  /** @brief Compute topological ordering using DFS.

      Performs depth-first traversal of the graph, inserting nodes
      in postfix order to produce a valid topological ordering.

      @tparam List Container type for the result (e.g., DynList, DynDlist).
      @param g The directed acyclic graph.
      @return A list of nodes in topological order.

      @throw std::bad_alloc If memory allocation fails.

      @note This function does not verify that g is acyclic.
            If g contains cycles, the result is undefined.
  */
  template <template <class> class List>
  [[nodiscard]] List<typename GT::Node*> perform(const GT & g)
  {
    g.reset_bit_nodes(Depth_First); // initialize visit marks

    gptr = &g;
    List<typename GT::Node*> list;

    // traverse all nodes
    const auto & n = gptr->get_num_nodes();
    for (auto it = g.get_node_it(); it.has_curr() and list.size() < n;
         it.next_ne())
      {
        auto curr = it.get_current_node_ne();
        if (not IS_NODE_VISITED(curr, Depth_First))
          topological_sort(curr, list);
      }

    return list;
  }

  /** @brief Operator() overload for backward compatibility.
      @param g The directed acyclic graph.
      @param list Output list to receive the topological ordering.
  */
  void operator()(const GT & g, DynDlist<typename GT::Node*> & list)
  {
    perform<DynDlist>(g).swap(list);
  }
};

/** @brief Computes topological ordering using breadth-first search (Kahn's algorithm).

    This class performs topological sorting using a queue-based approach.
    It repeatedly:
    1. Finds all source nodes (in-degree 0)
    2. Removes them from the graph (conceptually)
    3. Updates in-degrees of their successors
    4. Repeats until all nodes are processed

    Also provides rank-based ordering where nodes at the same "level"
    (same topological depth) are grouped together, useful for
    identifying parallelizable tasks.

    @tparam GT The directed graph type.
    @tparam Itor Iterator type for traversing outgoing arcs (default: Out_Iterator).
    @tparam SA Arc filter/selector (default: Dft_Show_Arc<GT>).

    @ingroup Graphs
*/
template <class GT,
          template <typename, class> class Itor = Out_Iterator,
          class SA = Dft_Show_Arc<GT>>
class Q_Topological_Sort
{
  SA & sa;

public:

  /** @brief Constructor with rvalue arc filter.
      @param __sa Arc filter/selector (moved).
  */
  Q_Topological_Sort(SA && __sa = SA())
    : sa(__sa) { /* empty */ }

  /** @brief Constructor with lvalue arc filter.
      @param __sa Arc filter/selector (referenced).
  */
  Q_Topological_Sort(SA & __sa)
    : sa(__sa) { /* empty */ }

  /** @brief Compute topological ordering using BFS (Kahn's algorithm).

      Uses in-degree counting and a queue to process source nodes,
      producing a valid topological ordering.

      @tparam List Container type for the result (e.g., DynList, DynDlist).
      @param g The directed acyclic graph.
      @return A list of nodes in topological order.

      @throw std::bad_alloc If memory allocation fails.
      @throw std::domain_error If g is not a digraph.

      @note This function does not verify that g is acyclic.
            If g contains cycles, not all nodes will be in the result.
  */
  template <template <class> class List>
  [[nodiscard]] List<typename GT::Node*> perform(const GT & g)
  {
    g.reset_counter_nodes();

    List<typename GT::Node*> list;

    // traverse all arcs and count in-degrees
    for (Arc_Iterator<GT,SA> it(g, sa); it.has_curr(); it.next_ne())
      NODE_COUNTER(it.get_tgt_node_ne())++;

    // find nodes with in-degree 0 and enqueue them
    DynListQueue<typename GT::Node*> q; // queue of sources
    for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
      {
        auto p = it.get_current_node_ne();
        if (NODE_COUNTER(p) == 0) // is it a source node?
          q.put(p); // yes => enqueue it
      }

    while (not q.is_empty())
      {
        auto p = q.get(); // dequeue last source

        assert(NODE_COUNTER(p) == 0);

        list.append(p); // insert in topological order

        // decrement in-degree of each node adjacent to p
        for (Itor<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
          {
            auto tgt = it.get_tgt_node_ne();
            if (--NODE_COUNTER(tgt) == 0) // does tgt become a source?
              q.put(tgt); // yes => enqueue it
          }
      }

    return list;
  }

  /** @brief Compute rank-based topological ordering.

      Returns the topological ordering organized by ranks (levels).
      Each rank contains nodes that can be processed in parallel,
      as they have no dependencies on each other within the same rank.

      @tparam RankList Outer container type for ranks (default: DynList).
      @tparam List Inner container type for nodes in each rank (default: DynList).
      @param g The directed acyclic graph.
      @return A list of ranks, where each rank is a list of nodes at that level.

      @throw std::bad_alloc If memory allocation fails.
      @throw std::domain_error If g is not a digraph.

      @note This function does not verify that g is acyclic.
            If g contains cycles, not all nodes will be in the result.
  */
  template <template <class> class RankList = DynList,
            template <class> class List = DynList>
  [[nodiscard]] RankList<List<typename GT::Node*>> ranks(const GT & g)
  {
    g.reset_counter_nodes();

    // traverse all nodes to count in-degrees
    for (typename GT::Node_Iterator i(g); i.has_curr(); i.next_ne())
      for (Itor<GT, SA> j(i.get_current_node_ne(), sa);
           j.has_curr(); j.next_ne())
        NODE_COUNTER(j.get_tgt_node())++;

    // find nodes with in-degree 0 and enqueue them
    DynListQueue<typename GT::Node*> q; // queue of sources
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne())
      {
        auto p = it.get_current_node_ne();
        if (NODE_COUNTER(p) == 0) // is it a source node?
          q.put(p); // yes => enqueue it
      }

    RankList<List<typename GT::Node*>> ranks; // return value
    while (not q.is_empty())
      {
        List<typename GT::Node*> rank;
        DynListQueue<typename GT::Node*> aq;

        while (not q.is_empty()) // extract all nodes at level i
          {
            auto p = q.get(); // dequeue last source
            rank.append(p); // insert in current rank

            // decrement in-degree of each node adjacent to p
            for (Itor<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
              {
                auto tgt = it.get_tgt_node_ne();
                if (--NODE_COUNTER(tgt) == 0) // does tgt become a source?
                  aq.put(tgt); // yes => enqueue in auxiliary queue
              }
          }

        ranks.append(std::move(rank));
        q.swap(aq);
        assert(aq.is_empty());
      }

    return ranks;
  }

  /** @brief Operator() overload returning ranks as DynDlist of DynList.
      @param g The directed acyclic graph.
      @param list Output list to receive the rank-based ordering.
  */
  void operator()(const GT & g, DynDlist<DynList<typename GT::Node *>> & list)
  {
    auto result = this->ranks<>(g);
    list.empty();
    for (auto it = result.get_it(); it.has_curr(); it.next_ne())
      list.append(std::move(it.get_curr()));
  }

  /** @brief Operator() overload returning ranks as DynList of DynList.
      @param g The directed acyclic graph.
      @param list Output list to receive the rank-based ordering.
  */
  void operator()(const GT & g, DynList<DynList<typename GT::Node *>> & list)
  {
    this->ranks<DynList>(g).swap(list);
  }

  /** @brief Operator() overload for backward compatibility (flat list).
      @param g The directed acyclic graph.
      @param list Output list to receive the topological ordering.
  */
  void operator()(const GT & g, DynDlist<typename GT::Node*> & list)
  {
    this->perform<DynDlist>(g).swap(list);
  }
};

} // end namespace Aleph

#endif // TOPOLOGICAL_SORT_H
