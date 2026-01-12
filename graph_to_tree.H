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

/** @file graph_to_tree.H
 *  @brief Convert spanning tree graphs to Tree_Node structures.
 *
 *  This file provides utilities for converting spanning trees represented
 *  as adjacency-list graphs into Tree_Node<Key> structures, enabling the
 *  use of tree-specific algorithms and visualization tools.
 *
 *  ## Purpose
 *
 *  While spanning trees can be represented using graph structures like
 *  List_Graph, the Tree_Node representation offers advantages for:
 *  - Tree drawing and visualization (e.g., with ntreepic)
 *  - Tree-specific traversal algorithms
 *  - Simpler recursive tree operations
 *
 *  ## Key Limitations
 *
 *  Tree_Node<Key> is less versatile than graph representations:
 *  - No storage for arc/edge data (only node data)
 *  - Key type may differ from the graph's node type
 *  - Requires a Convert class to map graph nodes to tree keys
 *
 *  ## Usage
 *
 *  ```cpp
 *  // Define a converter from graph node to tree key
 *  struct MyConvert {
 *    void operator()(Graph::Node* gnode, Tree_Node<int>* tnode) {
 *      tnode->get_key() = gnode->get_info().value;
 *    }
 *  };
 *
 *  // Convert spanning tree to Tree_Node
 *  Tree_Node<int>* root = Graph_To_Tree_Node<Graph, int, MyConvert>()(g, groot);
 *  ```
 *
 *  @see Tree_Node For the target tree structure
 *  @see generate_tree.H For tree visualization
 *  @see tpl_spanning_tree.H For spanning tree algorithms
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef GRAPH_TO_TREE_H
# define GRAPH_TO_TREE_H

# include <tpl_tree_node.H>
# include <tpl_graph.H>
# include <tpl_graph_utils.H>
 # include <ah-errors.H>

namespace Aleph {

    template <class GT, typename Key, class Convert> static
Tree_Node<Key> * graph_to_tree_node(GT & g, typename GT::Node * groot);
    template <class GT, typename Key, class Convert> static void 
__graph_to_tree_node(GT & g, typename GT::Node * groot, 
                     Tree_Node<Key> * troot);

  template <class GT, typename Key, typename Convert, class SA> static inline
void __graph_to_tree_node(GT & g, typename GT::Node * groot,
                          Tree_Node<Key> * troot);

/** @brief Convert a tree graph to a Tree_Node structure.
 *
 *  Takes a tree represented as an adjacency-list graph and converts it
 *  to a Tree_Node<Key> structure rooted at the specified node.
 *
 *  This enables the use of tree-specific algorithms and visualization
 *  tools on spanning trees extracted from graphs.
 *
 *  ## Conversion Process
 *
 *  The Convert class is invoked for each node:
 *  ```cpp
 *  void Convert::operator()(GT::Node* gnode, Tree_Node<Key>* tnode)
 *  ```
 *  It should copy/transform data from the graph node to the tree node.
 *
 *  ## Limitations
 *
 *  - Tree_Node<Key> cannot store arc data (only node data)
 *  - The Key type may differ from GT::Node's data type
 *  - Requires the input graph to be acyclic (a tree)
 *
 *  @tparam GT Graph type (derived from List_Graph)
 *  @tparam Key Data type for tree nodes
 *  @tparam Convert Converter class with operator()(gnode, tnode)
 *  @tparam SA Arc filter (default: show all arcs)
 *
 *  @param g The tree graph to convert
 *  @param groot Node to use as the tree root
 *  @return Root of the new Tree_Node<Key> tree
 *
 *  @throw std::domain_error If the graph is not acyclic
 *  @throw std::bad_alloc If memory allocation fails
 *
 *  @note Uses the Convert_Tree bit to mark visited arcs.
 *  @note Performs an acyclicity check which adds O(V+E) overhead.
 *
 *  @see Tree_Node Target tree structure
 *  @see is_graph_acyclique() Acyclicity verification
 *
 *  @ingroup Graphs
 */
    template <class GT, typename Key, 
              class Convert, class SA = Dft_Show_Arc<GT>> inline
Tree_Node<Key> * graph_to_tree_node(GT & g, typename GT::Node * groot)
{ 
  ah_domain_error_if(not is_graph_acyclique<GT>(g))
    << "Graph is not a tree (not acyclique)";

  Tree_Node<Key> * troot = new Tree_Node<Key>; // apartar memoria raíz

  Convert () (groot, troot); //convertir de groot y copiar a troot

  __graph_to_tree_node <GT, Key, Convert, SA> (g, groot, troot);

  return troot;
}

/** @brief Functor class to convert a tree graph to Tree_Node structure.
 *
 *  Provides a callable interface for converting spanning trees from
 *  graph representation to Tree_Node<Key> representation.
 *
 *  This class version offers more flexibility than the free function,
 *  allowing the arc filter to be configured at construction time.
 *
 *  ## Template Parameters
 *
 *  - **GT**: Graph type (derived from List_Graph)
 *  - **Key**: Data type to store in tree nodes
 *  - **Convert**: Functor class with `operator()(gnode, tnode)` that
 *    copies data from graph nodes to tree nodes
 *  - **SA**: Arc filter class (default: show all arcs)
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  struct MyConvert {
 *    void operator()(Graph::Node* gn, Tree_Node<int>* tn) {
 *      tn->get_key() = gn->get_info().id;
 *    }
 *  };
 *
 *  Graph_To_Tree_Node<Graph, int, MyConvert> converter;
 *  auto* root = converter(g, start_node);
 *  ```
 *
 *  @see graph_to_tree_node() Free function alternative
 *  @see Tree_Node Target tree structure
 *
 *  @ingroup Graphs
 */
    template <class GT, typename Key, 
	      class Convert, 
	      class SA = Dft_Show_Arc<GT> > 
class Graph_To_Tree_Node
{
  SA        sa;
  Convert * convert = nullptr;

  void graph_to_tree(typename GT::Node * groot, Tree_Node<Key> * troot)
  {           
    typedef typename GT::Node Node;
    typedef typename GT::Arc Arc;

        // recorrer arcos de groot y construir recursivamente
    for (Node_Arc_Iterator<GT, SA> it(groot, sa); it.has_curr(); it.next_ne())
      {
	Arc * arc = it.get_current_arc_ne();
	if (IS_ARC_VISITED(arc, Convert_Tree)) 
	  continue;

	ARC_BITS(arc).set_bit(Convert_Tree, true); // arc visitado 
	Node * gtgt = it.get_tgt_node();
	Tree_Node<Key> * ttgt = new Tree_Node<Key>; 

	(*convert) (gtgt, ttgt); // asignarle la clave

	troot->insert_rightmost_child(ttgt); // insertarlo como hijo 

	graph_to_tree(gtgt, ttgt);
      }
  }

  Tree_Node<Key> * graph_to_tree(GT & g,
				 typename GT::Node * groot,
				 Convert & conv)
  { 
    ah_domain_error_if(not is_graph_acyclique<GT>(g))
      << "Graph is not a tree (not acyclique)";

    Tree_Node<Key> * troot = new Tree_Node<Key>; // apartar memoria raíz

    convert = &conv;

    (*convert) (groot, troot); //convertir de groot y copiar a troot

    graph_to_tree(groot, troot);

    return troot;
  }

public:

  Graph_To_Tree_Node(SA __sa = SA()) : sa(__sa) { /* empty */ }

/** @brief Convert a tree graph to Tree_Node structure.
   *
   *  @param g The tree graph to convert (must be acyclic)
   *  @param groot Node to use as the tree root
   *  @param conv Converter functor instance
   *  @return Root of the new Tree_Node<Key> tree
   *
   *  @throw std::domain_error If the graph is not acyclic
   *  @throw std::bad_alloc If memory allocation fails
   */
      Tree_Node<Key> * 
  operator () (GT & g, typename GT::Node * groot, Convert && conv = Convert())
  { 
    return graph_to_tree(g, groot, conv);
  }

      Tree_Node<Key> * 
  operator () (GT & g, typename GT::Node * groot, Convert & conv)
  { 
    return graph_to_tree(g, groot, conv);
  }
};


  template <class GT, typename Key, typename Convert, class SA> static inline
void __graph_to_tree_node(GT & g, typename GT::Node * groot,
                          Tree_Node<Key> * troot)
{           
  typedef typename GT::Node Node;
  typedef typename GT::Arc Arc;

      // recorrer arcos de groot y construir recursivamente
  for (Node_Arc_Iterator<GT, SA> it(groot); it.has_curr(); it.next_ne())
    {
      Arc * arc = it.get_current_arc_ne();
      if (IS_ARC_VISITED(arc, Convert_Tree)) 
        continue;

      ARC_BITS(arc).set_bit(Convert_Tree, true); // arc visitado 
      Node * gtgt = it.get_tgt_node();
      Tree_Node<Key> * ttgt = new Tree_Node<Key>; 

      Convert () (gtgt, ttgt); // asignarle la clave

      troot->insert_rightmost_child(ttgt); // insertarlo como hijo 

      __graph_to_tree_node <GT, Key, Convert, SA> (g, gtgt, ttgt);
    }
}

} // end namespace Aleph

# endif // GRAPH_TO_TREE_H
