/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

/** @file tpl_graph_indexes.H
 *  @brief Graph node and arc indexing.
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_GRAPH_INDEXES_H
# define TPL_GRAPH_INDEXES_H

# include <tpl_dynSetTree.H>
# include <tpl_graph.H>

using namespace Aleph;

namespace Aleph
{

/**
 * @brief Default node comparison class for Nodes_Index.
 *
 * Compares nodes based on their info field.
 *
 * @tparam GT Graph type
 */
template <class GT>
struct Dft_Node_Cmp
{
  /**
   * @brief Comparison operator.
   *
   * @param p1 First node to compare
   * @param p2 Second node to compare
   * @return True if p1's info is less than p2's info
   */
  bool operator () (typename GT::Node * p1, typename GT::Node * p2) const
  {
    return p1->get_info() < p2->get_info();
  }
};

/**
 * @brief Default arc comparison class for Arcs_Index.
 *
 * Compares arcs based on their source and target nodes.
 *
 * @tparam GT Graph type
 */
template <class GT>
struct Dft_Arc_Cmp
{
  /**
   * @brief Comparison operator.
   *
   * @param a1 First arc to compare
   * @param a2 Second arc to compare
   * @return True if a1's source node is less than a2's source node, or if
   *         source nodes are equal and a1's target node is less than a2's target node
   */
  bool operator () (typename GT::Arc * a1, typename GT::Arc * a2) const
  {
    if (a1->src_node < a2->src_node)
      return true;

    return not (a2->src_node < a1->src_node) and a1->tgt_node < a2->tgt_node;
  }
};

/**
 * @brief Builds a node index for fast lookup and retrieval.
 *
 * Nodes_Index indexes the nodes of a graph by a user-defined key.
 *
 * Template parameters:
 * - GT: Graph type based on List_Graph
 * - Compare: Comparison class for indexing key (default compares node info)
 * - Tree: Binary search tree type for internal storage (default Treap)
 * - SN: Node iterator filter class
 *
 * @note The index maintains pointers to graph nodes. Node removal from the graph
 *       without removing from the index will cause undefined behavior.
 * @see Arcs_Index Index_Graph
 * @ingroup Graphs
 */
template <class GT, class Compare = Dft_Node_Cmp<GT>,
          template <typename, class> class Tree = Treap,
          class SN = Dft_Show_Node<GT>>
class Nodes_Index : public DynSetTree<typename GT::Node *, Tree, Compare>
{
  /**
   * @brief Typedef for the graph's node type.
   */
  typedef typename GT::Node GT_Node;
  
  /**
   * @brief Typedef for the graph's node info type.
   */
  typedef typename GT::Node_Type GT_Node_Info;
  
  /**
   * @brief Typedef for the internal tree type.
   */
  typedef DynSetTree <GT_Node *, Tree, Compare> Tree_Type;

  /**
   * @brief Reference to the graph being indexed.
   */
  GT & g;
  
  /**
   * @brief Reference to the node iterator filter object.
   */
  SN & sn;

  /**
   * @brief Initialize the index by inserting all existing graph nodes
   */
  void init()
  {
    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next_ne())
      this->insert(it.get_curr());
  }

public:
  /**
   * @brief Constructor that initializes the index with existing graph nodes
   * @param _g Reference to the graph
   * @param cmp Comparison function object
   * @param _sn Node iterator filter
   */
  Nodes_Index(GT & _g, Compare & cmp, SN & _sn)
    : Tree_Type(cmp), g(_g), sn(_sn)
  {
    init();
  }

  /**
   * @brief Constructor with rvalue references for comparison and filter
   * @param _g Reference to the graph
   * @param cmp Comparison function object
   * @param _sn Node iterator filter
   */
  Nodes_Index(GT & _g, Compare && cmp = Compare(), SN && _sn = SN())
    : Tree_Type(cmp), g(_g), sn(_sn)
  {
    init();
  }

  /**
   * @brief Inserts a node into the graph and index.
   *
   * @param p Node to insert
   * @return Pointer to the inserted node, or nullptr if insertion failed
   */
  GT_Node * insert_in_graph(GT_Node * p)
  {
    g.insert_node(p);

    if (this->insert(p) == nullptr)
      {
        g.remove_node(p);
        return nullptr;
      }

    return p;
  }

  /**
   * @brief Searches for a node in the index and graph, inserting it if not found.
   *
   * @param p Node to search for or insert
   * @return Pointer to the found or inserted node
   */
  GT_Node * search_or_insert_in_graph(GT_Node * p)
  {
    g.insert_node(p);
    GT_Node ** ptr_node = this->search_or_insert(p);
    GT_Node * q = *ptr_node;

    if (p != q)
      g.remove_node(p);

    return q;
  }

  /**
   * @brief Inserts a node with generic content into the graph and index.
   *
   * @param info Node content
   * @return Pointer to the inserted node, or nullptr if insertion failed
   * @throw bad_alloc if memory allocation fails
   */
  GT_Node * insert_in_graph(const GT_Node_Info & info)
  {
    GT_Node * p = g.insert_node(info);
    
    if (this->insert(p) == nullptr)
      {
        g.remove_node(p);
        return nullptr;
      }

    return p;
  }

  /**
   * @brief Searches for a node with generic content in the index and graph, inserting it if not found.
   *
   * @param info Node content
   * @return Pointer to the found or inserted node
   * @throw bad_alloc if memory allocation fails
   */
  GT_Node * search_or_insert_in_graph(const GT_Node_Info & info)
  {
    GT_Node * p = g.insert_node(info);
    GT_Node ** ptr_node = this->search_or_insert(p);
    GT_Node * q = *ptr_node;

    if (p != q)
      g.remove_node(p);

    return q;
  }

  /**
   * @brief Inserts a node with generic content into the graph and index (rvalue overload).
   *
   * @param info Node content
   * @return Pointer to the inserted node, or nullptr if insertion failed
   * @throw bad_alloc if memory allocation fails
   */
  GT_Node * insert_in_graph(const GT_Node_Info && info = GT_Node_Info())
  {
    return insert_in_graph(info);
  }

  /**
   * @brief Searches for a node by its content.
   *
   * @param p Node to search for
   * @return Pointer to the found node, or nullptr if not found
   */
  GT_Node * search(GT_Node * p)
  {
    GT_Node ** ret_val = Tree_Type::search(p);

    if (ret_val == nullptr)
      return nullptr;

    return *ret_val;
  }

  /**
   * @brief Searches for a node by its content (info overload).
   *
   * @param info Node content
   * @return Pointer to the found node, or nullptr if not found
   */
  GT_Node * search(const GT_Node_Info & info)
  {
    GT_Node dummy_node(info);
    GT_Node * dummy_node_ptr = &dummy_node;

    return search(dummy_node_ptr);
  }

  /**
   * @brief Removes a node from the graph and index.
   *
   * @param p Node to remove
   * @throw domain_error if the node is not found in the index
   */
  void remove_from_graph(GT_Node * p)
  {
    Tree_Type::find(p); // throws exception if p is not in the index
    Tree_Type::remove(p);
    g.remove_node(p);
  }
};

/**
 * @brief Builds an arc index for fast lookup by source/target nodes.
 *
 * Arcs_Index indexes graph arcs based on the node pair they connect,
 * enabling logarithmic-time arc existence checks and retrieval.
 *
 * Template parameters:
 * - GT: Graph type based on List_Graph
 * - Compare: Comparison class for arcs (default compares src/tgt nodes)
 * - Tree: Binary search tree type for internal storage (default Treap)
 * - SA: Arc iterator filter class
 *
 * @warning Requires GT::Arc(void*, void*, GT::Arc_Type) constructor
 * @warning Default constructor GT::Arc_Type() must exist
 * @note For undirected graphs, searches will check both directions
 * @see Nodes_Index Index_Graph
 * @ingroup Graphs
 */
template <class GT, class Compare = Dft_Arc_Cmp<GT>,
          template <typename, class> class Tree = Treap,
          class SA = Dft_Show_Arc<GT>>
class Arcs_Index : public DynSetTree<typename GT::Arc *, Tree, Compare>
{
  /**
   * @brief Typedef for the graph's node type.
   */
  typedef typename GT::Node GT_Node;
  
  /**
   * @brief Typedef for the graph's node info type.
   */
  typedef typename GT::Node_Type GT_Node_Info;
  
  /**
   * @brief Typedef for the graph's arc type.
   */
  typedef typename GT::Arc GT_Arc;
  
  /**
   * @brief Typedef for the graph's arc info type.
   */
  typedef typename GT::Arc_Type GT_Arc_Info;
  
  /**
   * @brief Typedef for the internal tree type.
   */
  typedef DynSetTree <typename GT::Arc *, Tree, Compare> Tree_Type;

  /**
   * @brief Reference to the graph being indexed.
   */
  GT & g;
  
  /**
   * @brief Reference to the arc iterator filter object.
   */
  SA & sa;

  /**
   * @brief Initializes the index by inserting all arcs from the graph.
   */
  void init()
  {
    for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
      this->insert(it.get_curr());
  }

public:
  /**
   * @brief Constructor.
   *
   * @param _g Graph to index
   * @param cmp Comparison object for arcs
   * @param _sa Arc iterator filter object
   */
  Arcs_Index(GT & _g, Compare & cmp, SA & _sa)
    : Tree_Type(cmp), g(_g), sa(_sa)
  {
    init();
  }

  /**
   * @brief Constructor with default comparison and filter objects.
   *
   * @param _g Graph to index
   * @param cmp Comparison object for arcs (default constructed)
   * @param _sa Arc iterator filter object (default constructed)
   */
  Arcs_Index(GT & _g, Compare && cmp = Compare(), SA && _sa = SA())
    : Tree_Type(cmp), g(_g), sa(_sa)
  {
    init();
  }

  /**
   * @brief Inserts an arc into the graph and index.
   *
   * @param src Source node
   * @param tgt Target node
   * @param info Arc content
   * @return Pointer to the inserted arc, or nullptr if insertion failed
   * @throw bad_alloc if memory allocation fails
   */
  GT_Arc * insert_in_graph(GT_Node * src, GT_Node * tgt,
                           const GT_Arc_Info & info)
  {
    GT_Arc * a = g.insert_arc(src, tgt, info);
    
    if (this->insert(a) == nullptr)
      {
        g.remove_arc(a);
        return nullptr;
      }

    return a;
  }

  /**
   * @brief Inserts an arc into the graph and index (rvalue overload).
   *
   * @param src Source node
   * @param tgt Target node
   * @param info Arc content
   * @return Pointer to the inserted arc, or nullptr if insertion failed
   * @throw bad_alloc if memory allocation fails
   */
  GT_Arc * insert_in_graph(GT_Node * src, GT_Node * tgt,
                           const GT_Arc_Info && info = GT_Arc_Info())
  {
    return insert_in_graph(src, tgt, info);
  }

  /**
   * @brief Searches for an arc by its source, target, and content.
   *
   * @param src Source node
   * @param tgt Target node
   * @param info Arc content
   * @return Pointer to the found arc, or nullptr if not found
   */
  GT_Arc * search(GT_Node * src, GT_Node * tgt, const GT_Arc_Info & info)
  {
    // Create persistent key for search (avoid stack allocation)
    auto * key = new GT_Arc(info);
    key->src_node = src;
    key->tgt_node = tgt;
    
    GT_Arc ** ret_val = Tree_Type::search(key);
    
    if (ret_val != nullptr)
      {
        GT_Arc* found = *ret_val;
        delete key;
        return found;
      }
    
    if (g.is_digraph())
      {
        delete key;
        return nullptr;
      }
    
    // Try reverse direction for undirected graphs
    std::swap(key->src_node, key->tgt_node);
    ret_val = Tree_Type::search(key);
    
    if (ret_val == nullptr)
      {
        delete key;
        return nullptr;
      }
    
    // Validate arc direction consistency
    GT_Arc* found = *ret_val;
    assert(((src == found->src_node) && (tgt == found->tgt_node)) || 
           ((tgt == found->src_node) && (src == found->tgt_node)));
    
    delete key;
    return found;
  }

  /**
   * @brief Searches for an arc by its source, target, and content (rvalue overload).
   *
   * @param src Source node
   * @param tgt Target node
   * @param info Arc content
   * @return Pointer to the found arc, or nullptr if not found
   */
  GT_Arc * search(GT_Node * src, GT_Node * tgt,
                  const GT_Arc_Info && info = GT_Arc_Info())
  {
    return search(src, tgt, info);
  }

  /**
   * @brief Removes an arc from the graph and index.
   *
   * @param a Arc to remove
   * @throw domain_error if the arc is not found in the index
   */
  void remove_from_graph(GT_Arc * a)
  {
    Tree_Type::find(a);
    Tree_Type::remove(a);
    g.remove_arc(a);
  }

};

} // End namespace Aleph

# endif // GRAPH_INDEXES_H
