
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


/** @file tpl_indexGraph.H
 *  @brief Graph indexing utilities for O(log n) node/arc lookup.
 *
 *  @par Example: Fast node lookup by value
 *  @code
 *  using GT = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
 *  GT g;
 *  Index_Graph<GT> idx(g);
 *  
 *  // Insert nodes
 *  auto n1 = idx.insert_node(100);
 *  auto n2 = idx.insert_node(200);
 *  auto n3 = idx.insert_node(300);
 *  
 *  // O(log n) search by value
 *  auto found = idx.search_node(200);
 *  if (found)
 *    cout << "Found node: " << found->get_info() << endl;
 *  @endcode
 *
 *  @par Example: Fast arc lookup
 *  @code
 *  idx.insert_arc(n1, n2, 12);
 *  
 *  // O(log n) arc search
 *  auto arc = idx.search_arc(n1, n2);
 *  if (arc)
 *    cout << "Arc weight: " << arc->get_info() << endl;
 *  @endcode
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_INDEXGRAPH
# define TPL_INDEXGRAPH

# include <tpl_indexNode.H>
# include <tpl_indexArc.H>

# include <ah-errors.H>

namespace Aleph
{

  /** Builds node and arc indices for fast lookup and retrieval.

      Index_Graph indexes nodes and arcs for the sake of their
      fast retrieval.

      To make it easier and safer to use, Index_Graph offers the
      classic topological operations of a graph: insert_node(),
      insert_arc(), etc.

      The class takes the following type parameters:
      -# GT: the graph type, based on List_Graph
      -# Compare: comparison class for the nodes' indexing key. This
      class's contract is to implement operator () like this:
      \code
          template <class GT>
          struct Dft_Node_Cmp
          {
            bool
            operator () (typename GT::Node * p1, typename GT::Node * p2) const
            {
              // access the nodes and compare according to the desired field
            }
         };
      \endcode

      By default this class is programmed to compare the value
      returned by get_info() on each node. For that, the < operator
      of type GT::Node_Type must be implemented
      -# Tree: the type of binary search tree used internally to
      index the keys. Treaps are used by default
      .

      @see IndexArc IndexNode

      @ingroup Graphs
      @author Leandro Rabindranath León (lrleon at ula dot ve)
      @author Alejandro Mujica (aledrums at gmail dot com)
  */
template <class GT, class Compare = Dft_Node_Cmp<GT>,
          template <class /* Key */, class /* Compare */> class Tree = Treap>
class Index_Graph
{
private:

  typedef typename GT::Arc GT_Arc;
  typedef typename GT::Node GT_Node;
  typedef typename GT::Arc_Type GT_Arc_Type;
  typedef typename GT::Node_Type GT_Node_Type;

  IndexNode<GT, Compare, Tree> idx_node;
  IndexArc<GT, Tree>           idx_arc;

public:

      /// Creates an index of the graph: nodes and arcs are indexed.
  Index_Graph(GT & g) : idx_node(g), idx_arc(g)
  {
    // empty
  }

  /** Creates a new node and inserts it into the graph and the index.

      @param[in] info information to be copied into the node.
      @return pointer to the new node.
      @throw bad_alloc if there is not enough memory.
   */
  GT_Node * insert_node(const GT_Node_Type & info)
  {
    return idx_node.insert_in_graph(info);
  }

  /** Creates a new arc between two nodes and inserts it into the graph
      and the index.

      @param[in] src source node.
      @param[in] tgt target node.
      @param[in] info information to be copied into the arc. By default
      it is whatever the GT_Arc_Type() constructor gives.
      @return pointer to the new arc.
      @throw bad_alloc if there is not enough memory.
   */
  GT_Arc * insert_arc(GT_Node * src, GT_Node * tgt,
                      const GT_Arc_Type & info = GT_Arc_Type())
  {
    ah_domain_error_if(idx_node.search(src) == nullptr)
        << "src node not in index";

    ah_domain_error_if(idx_node.search(tgt) == nullptr)
        << "tgt node not in index";

    return idx_arc.insert_in_graph(src, tgt, info);
  }

  /** Looks up a node in the index.

      @param[in] p pointer to the node.
      @return pointer to the node if it is found in the index; nullptr
      otherwise.
  */
  GT_Node * search_node(GT_Node * p)
  {
    return idx_node.search(p);
  }

  /** Looks up a node in the index.

      @param[in] info information by which the node will be searched for.
      @return pointer to the node if it is found in the index; nullptr
      otherwise.
      @warning Keep in mind that the lookup is performed according to
      the implementation of the Compare comparison class passed at
      instantiation time.
  */
  GT_Node * search_node(const GT_Node_Type & info)
  {
    return idx_node.search(info);
  }

  /** Looks up an arc in the index given its two nodes.

      @param[in] src pointer to the source node.
      @param[in] tgt pointer to the target node.
      @return pointer to the arc if it is found in the index; nullptr
      otherwise.
  */
  GT_Arc * search_arc(GT_Node * src, GT_Node * tgt)
  {
    return idx_arc.search(src, tgt);
  }

  /** Removes node p from the graph and from the index.

      @param[in] p pointer to the node to remove
  */
  void remove_node(GT_Node * p)
  {
    // remove from the index the arcs emanating from the node (they
    // will be removed by the node's removal from the graph)
    for (typename GT::Node_Arc_Iterator it(p); it.has_curr(); it.next_ne())
      idx_arc.remove(it.get_curr());

    return idx_node.remove_from_graph(p);
  }

  /// Removes arc a from the graph and from the index
  void remove_arc(GT_Arc * a)
  {
    return idx_arc.remove_from_graph(a);
  }

  /// Returns the number of arcs the index contains.
  size_t get_num_arcs() const { return idx_arc.size(); }

  /// Returns the number of nodes the index contains.
  size_t get_num_nodes() const { return idx_node.size(); }
};

  template <AlephGraph GT> inline
  bool are_equal(const GT & g1, const GT & g2)
  {
    if (g1.vsize() != g2.vsize() or g1.esize() != g2.esize())
      return false;

    {
      IndexNode<GT> t2(const_cast<GT&>(g2));
      if (not g1.all_nodes([&t2] (auto p)
                           {
                             auto q = t2.search(p);
                             if (q == nullptr)
                               return false;
                             GT::map_nodes(p, q);
                             return true;
                           }))
        return false;
    }

    IndexArc<GT> t2(const_cast<GT&>(g2));
    return g1.all_arcs([idx = &t2, &g1] (auto a)
                       {
                         auto s1 = g1.get_src_node(a);
                         auto t1 = g1.get_tgt_node(a);
                         auto s2 = mapped_node<GT>(s1);
                         auto t2 = mapped_node<GT>(t1);
                         auto a2 = idx->search(s2, t2);
                         if (a2 == nullptr)
                           return false;
                         return a2->get_info() == a->get_info();
                       });
  }

}

# endif // TPL_INDEXGRAPH


