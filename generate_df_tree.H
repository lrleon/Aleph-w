
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


/** @file generate_df_tree.H
 *  @brief Depth-first tree visualization with DFS numbering.
 *
 *  This file provides utilities for generating visual representations of
 *  depth-first spanning trees, including DFS discovery numbers and low-link
 *  values used in cut-vertex (articulation point) detection.
 *
 *  ## Key Features
 *
 *  - Generate tree pictures showing DFS traversal order
 *  - Display low-link values for cut-vertex analysis
 *  - Support for non-tree arcs (back edges) visualization
 *
 *  ## Data Structure
 *
 *  The `Clave` struct stores per-node information:
 *  - `key`: Original node identifier
 *  - `count`: DFS discovery number (df)
 *  - `low`: Low-link value for cut-vertex detection
 *
 *  @warning This file uses a static global counter and is NOT thread-safe.
 *           Do not use these functions concurrently from multiple threads.
 *
 *  @see tpl_cut_nodes.H For cut-vertex (articulation point) algorithms
 *  @see generate_tree.H For general tree visualization
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath Leon
 */

#ifndef GENERATE_DF_TREE_H
#define GENERATE_DF_TREE_H

#include <tpl_graph_utils.H>
#include <tpl_tree_node.H>
#include <generate_tree.H>

/// @internal Global DFS counter - NOT thread-safe
static long global_counter = 0;

/** @brief Key structure for DFS tree nodes.
 *
 *  Stores the essential information for each node in a depth-first
 *  spanning tree visualization.
 */
struct Clave
{
  int key;    ///< Original node identifier
  long count; ///< DFS discovery number (pre-order)
  long low;   ///< Low-link value (minimum reachable via back edges)
};

/** @brief Equality comparator for Clave structures.
 *
 *  Compares two Clave objects by their key field only.
 */
struct Clave_Igual
{
  bool operator () (const Clave & c1, const Clave & c2) const
  {
    return c1.key == c2.key;
  }  
};

/** @brief Converter from graph node to Tree_Node<Clave>.
 *
 *  Extracts DFS information from the original graph node
 *  and stores it in the tree representation.
 */
struct Convertir
{
  void operator () (Grafo::Node * tnode, Tree_Node<Clave> * t)
  {
    Grafo::Node * gnode = static_cast<Grafo::Node *>(NODE_COOKIE(tnode));

    Clave & clave = t->get_key();
    clave.key   = tnode->get_info().clave;
    clave.count = gnode->get_info().df;
    clave.low   = gnode->get_info().low;
  }
};

/** @brief Writer that outputs only the node key.
 *
 *  Outputs a single character representing the node identifier.
 */
struct Write_Node
{
  static const size_t Buf_Size = 512;

  std::string operator () (Tree_Node<Clave> * p)
  {
    char str[2];
    str[0] = p->get_key().key;
    str[1] = '\0';
    return std::string(str);
  }
};

/** @brief Writer that outputs node key and DFS number.
 *
 *  Outputs format: (key, df_number)
 *  Example: "(A,3)" for node A discovered at step 3.
 */
struct Write_Df
{
  static const size_t Buf_Size = 512;

  std::string operator () (Tree_Node<Clave> * p)
  {
    char buf[Buf_Size];
    snprintf(buf, Buf_Size, "(%c,%ld)", p->get_key().key, p->get_key().count);
    return std::string(buf);
  }
};

/** @brief Writer that outputs node key, DFS number, and low-link value.
 *
 *  Outputs format: key,df,low
 *  Example: "A,3,1" for node A with df=3 and low=1.
 *  If low is negative (undefined), outputs "-" instead.
 */
struct Write_Low
{
  static const size_t Buf_Size = 512;

  std::string operator () (Tree_Node<Clave> * p)
  {
    char buf[Buf_Size];

    if (p->get_key().low >= 0)
      snprintf(buf, Buf_Size, "%d,%ld,%ld", 
               p->get_key().key, p->get_key().count, p->get_key().low);
    else
      snprintf(buf, Buf_Size, "%d,%ld,-", 
               p->get_key().key, p->get_key().count);

    return std::string(buf);
  }
};

/** @brief DFS visitor that assigns discovery numbers.
 *
 *  Increments the global counter and assigns it to the node's df field.
 *  @warning Uses global state - NOT thread-safe.
 */
inline void visitar_df(Grafo &, Grafo::Node * nodo, Grafo::Arc *)
{
  nodo->get_info().df = global_counter++;
}

/** @brief DFS visitor that copies low-link values from cookies.
 *
 *  Retrieves the low-link value stored in the node's cookie field.
 */
inline void visitar_low(Grafo &, Grafo::Node * nodo, Grafo::Arc *)
{
  nodo->get_info().low = reinterpret_cast<long>(nodo->cookie);
}


/** @brief Generate a DFS tree picture with low-link values.
 *
 *  Computes and visualizes a depth-first spanning tree with:
 *  - DFS discovery numbers
 *  - Low-link values (for cut-vertex detection)
 *  - Non-tree arcs (back edges)
 *
 *  The output is suitable for tree visualization tools.
 *
 *  @tparam GT Graph type (must be derived from List_Graph)
 *  @tparam Key Key type for tree nodes (typically Clave)
 *  @param g The input graph
 *  @param src Starting node for DFS traversal
 *  @param f Output stream for the tree specification
 *
 *  @note This function also computes cut-vertices as a side effect.
 *  @warning Uses global state (global_counter) - NOT thread-safe.
 *
 *  @see compute_cut_nodes() For cut-vertex computation
 *  @see generate_tree() For tree output format
 *
 *  @ingroup Graphs
 */
template <class GT, class Key>
void write_df_low_tree(GT & g, typename GT::Node * src, std::ofstream & f)
{
  // Compute cut nodes (articulation points)
  DynDlist<typename GT::Node*> node_list = compute_cut_nodes(g);

  depth_first_traversal(g, src, &visitar_df);  // Copy df numbers
  depth_first_traversal(g, src, &visitar_low); // Copy low-link values

  Grafo tree = find_depth_first_spanning_tree(g, src);

  // Compute non-tree arcs (back edges)
  DynDlist<No_Tree_Arc> arc_list;
  generate_non_tree_arcs(g, arc_list);

  typename GT::Node * td = static_cast<typename GT::Node *>(NODE_COOKIE(src));

  Tree_Node<Key> * rd = Graph_To_Tree_Node () <Grafo, Key, Convertir>(tree, td);

  generate_tree<Tree_Node<Key>, Write_Low>(rd, f);

  write_non_tree_arcs(arc_list, rd, f);
}

#endif // GENERATE_DF_TREE_H
