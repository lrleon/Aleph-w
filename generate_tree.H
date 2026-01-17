
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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


/** @file generate_tree.H
 *  @brief Tree visualization and output generation.
 *
 *  Provides functions for generating visual representations of
 *  tree structures including DOT format for Graphviz.
 *
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

# ifndef GENERATE_TREE_H
# define GENERATE_TREE_H

# include <fstream>
# include <tpl_tree_node.H>
# include <tpl_binNodeUtils.H>
# include <ah-errors.H>

namespace Aleph
{
  template <class Node>
  struct Dft_Write
  {
    std::string operator ()(Node *p)
    {
      return to_str(p->get_key());
    }
  };

  /*
    Write debe exportar const std::string Write::operator () (Node *)

    Que se encarga de convertir el valor de clave contenido en el nodo a
    un std::string imprimible
  */

  template <typename Node, class Write>
  static
  void __generate_tree(Node *node,
                       int deway[],
                       const size_t & current_level,
                       const size_t & size,
                       std::ostream & output)
  {
    ah_overflow_error_if(current_level >= size)
      << "Allocated size for deway array has been exceeded";

    // imprimir número de Deway del padre
    output << "Node ";
    for (size_t i = 0; i < current_level; ++i)
      {
        output << deway[i];

        if (i < current_level - 1)
          output << ".";
      }
    output << " \"" << Write()(node) << "\" " << std::endl;

    Node *child = static_cast<Node *>(node->get_left_child());
    for (int i = 0; child != nullptr;
         i++, child = static_cast<Node *>(child->get_right_sibling()))
      {
        deway[current_level + 1] = i;
        __generate_tree<Node, Write>
            (child, deway, current_level + 1, size, output);
      }
  }


  constexpr size_t Max_Tree_Node_Depth = 1024;

  /** @brief Generate a tree specification for the ntreepic drawing tool.
     *
     *  Produces a text specification that can be used with the ntreepic
     *  program to generate visual representations of tree structures.
     *
     *  The output format uses Dewey decimal notation to identify nodes:
     *  - Root is at position 0
     *  - First child of root is 0.0, second is 0.1, etc.
     *  - Children are numbered left-to-right
     *
     *  @tparam Node Tree node type (must be Tree_Node or compatible)
     *  @tparam Write Functor that converts node key to string for display.
     *          Must provide: `std::string operator()(Node*)`
     *
     *  @param root Root of the tree to draw
     *  @param out Output stream for the drawing specification
     *  @param tree_number Internal use - tree index in a forest (default: 0)
     *
     *  @see generate_forest() For drawing multiple trees
     *  @see Dft_Write Default writer using to_str()
     *
     *  @ingroup Trees
     */
  template <typename Node, class Write = Dft_Write<Node>>
  void generate_tree(Node *root,
                     std::ostream & out,
                     const int & tree_number = 0)
  {
    out << "Root \"" << Write()(root) << "\" " << std::endl;

    const auto deway = new int [Max_Tree_Node_Depth];

    constexpr int level = 0; // Este es el nivel de partida

    deway[level] = tree_number; // nivel 0 en el número del árbol

    Node *child = static_cast<Node *>(root->get_left_child());
    for (int i = 0; child != nullptr;
         i++, child = static_cast<Node *>(child->get_right_sibling()))
      {
        deway[1] = i;
        __generate_tree<Node, Write>(child, deway, level + 1,
                                     Max_Tree_Node_Depth, out);
      }

    delete [] deway;
  }

  /** @brief Generate a forest specification for the ntreepic drawing tool.
     *
     *  Produces a text specification for drawing a forest (collection of
     *  trees linked as siblings). Each tree is numbered starting from 0.
     *
     *  The forest is represented as siblings of the first root node:
     *  - First tree: root
     *  - Second tree: root->get_right_sibling()
     *  - And so on...
     *
     *  @tparam Node Tree node type (must be Tree_Node or compatible)
     *  @tparam Write Functor that converts node key to string for display.
     *          Must provide: `std::string operator()(Node*)`
     *
     *  @param root Root of the first tree in the forest
     *  @param out Output stream for the drawing specification
     *
     *  @see generate_tree() For drawing a single tree
     *
     *  @ingroup Trees
     */
  template <typename Node, class Write = Dft_Write<Node>>
  void generate_forest(Node *root, std::ostream & out)
  {
    Node *tree = root;

    for (int i = 0; tree != nullptr; i++, tree = tree->get_right_sibling())
      generate_tree<Node, Write>(tree, out, i);
  }


  /** @brief Generate a binary tree specification for the btreepic drawing tool.
     *
     *  Produces a text specification for drawing binary trees using the
     *  btreepic program. The output contains both prefix and infix
     *  traversal sequences.
     *
     *  Output format:
     *  ```
     *  start-prefix <prefix traversal>
     *  start_infix <infix traversal>
     *  ```
     *
     *  @tparam Node Binary tree node type
     *  @tparam Write Functor that writes node content to output stream.
     *          Invoked as: `Write()(node)` during traversals
     *
     *  @param root Root of the binary tree to draw
     *  @param out Output stream for the drawing specification
     *
     *  @see generate_tree() For general (non-binary) trees
     *
     *  @ingroup Trees
     */
  template <typename Node, class Write>
  void generate_btree(Node *root, std::ostream & out)
  {
    out << "start-prefix ";
    For_Each_Preorder<Node>()(root, Write());
    out << std::endl
        << "start_infix ";
    For_Each_In_Order<Node>()(root, Write());
    out << std::endl;
  }
} // end namespace Aleph

# endif // GENERATE_TREE_H
