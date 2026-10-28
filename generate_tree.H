
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
# include <memory>
# include <string>
# include <tpl_dfs_stack.H>
# include <tpl_tree_node.H>
# include <tpl_binNodeUtils.H>
# include <ah-errors.H>

namespace Aleph
{
  inline std::string escape_dot_label(const std::string & text)
  {
    std::string escaped;
    escaped.reserve(text.size());

    for (const char ch : text)
      switch (ch)
        {
        case '\\':
          escaped += "\\\\";
          break;
        case '"':
          escaped += "\\\"";
          break;
        case '\n':
          escaped += "\\n";
          break;
        case '\r':
          escaped += "\\r";
          break;
        case '\t':
          escaped += "\\t";
          break;
        default:
          escaped += ch;
        }

    return escaped;
  }

  template <class Node>
  struct Dft_Write
  {
    std::string operator ()(Node *p)
    {
      return to_str(p->get_key());
    }
  };

  /*
    Write must give std::string Write::operator () (Node *), which turns
    the key of the node into a printable std::string.

    The nodes go in preorder, as a recursion would take them; deway holds
    the positions of the nodes of the path. A path can be as long as the
    tree is deep, too long for recursion: the parent of a node is found
    through its left siblings and its first child.
  */

  template <typename Node, class Write>
  static
  void __generate_tree(Node *node,
                       int deway[],
                       const size_t & current_level,
                       const size_t & size,
                       std::ostream & output)
  {
    Node *p = node;
    size_t level = current_level;
    while (true)
      {
        // the Dewey number of the parent
        output << "Node ";
        for (size_t i = 0; i < level; ++i)
          {
            output << deway[i];

            if (i < level - 1)
              output << ".";
          }
        output << " \"" << Write()(p) << "\" " << std::endl;

        if (auto *child = static_cast<Node *>(p->get_left_child()))
          {
            ah_overflow_error_if(level + 1 >= size)
              << "Allocated size for deway array has been exceeded";

            deway[level + 1] = 0;
            p = child;
            ++level;
            continue;
          }

        // the right sibling of p, or of its lowest ancestor below node
        // that has one
        while (p != node and p->is_rightmost())
          {
            p = static_cast<Node *>(p->get_parent());
            --level;
          }

        if (p == node)
          return;

        p = static_cast<Node *>(p->get_right_sibling());
        ++deway[level];
      }
  }


  /// The depth of the trees that generate_tree() wrote before 7.3.2; it
  /// writes trees of any depth now
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
     *  @throw std::bad_alloc if there is not enough memory; @p out may
     *         then hold part of the tree
     *
     *  @note The Dewey number of each node has as many parts as its
     *        depth, so that a deep tree gives a long specification.
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

    // the Dewey numbers, as long as the tree is deep
    const size_t size = compute_height(root) + 1;
    const auto deway = std::make_unique<int[]>(size);

    constexpr int level = 0; // the level where it starts

    deway[level] = tree_number; // level 0 is the number of the tree

    Node *child = static_cast<Node *>(root->get_left_child());
    for (int i = 0; child != nullptr;
         i++, child = static_cast<Node *>(child->get_right_sibling()))
      {
        deway[1] = i;
        __generate_tree<Node, Write>(child, deway.get(), level + 1,
                                     size, out);
      }
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

  /** @brief Generate a Graphviz DOT specification for a tree.
     *
     *  Produces a DOT specification that can be rendered using Graphviz
     *  tools (dot, neato, etc.) to visualize the tree structure.
     *
     *  @tparam Node Tree node type (e.g. Tree_Node)
     *  @tparam Write Functor that converts node key to string for the node label.
     *          Must provide: `std::string operator()(Node*)`
     *
     *  @param root Root of the tree to draw
     *  @param out Output stream for the DOT specification
     *
     *  @see Dft_Write Default writer using to_str()
     *
     *  @ingroup Trees
     */
  template <typename Node, class Write = Dft_Write<Node>>
  void generate_tree_graphviz(Node *root, std::ostream & out)
  {
    out << "digraph Tree {" << std::endl;
    out << "  node [shape=circle, fontname=\"Helvetica\"];" << std::endl;
    out << "  edge [fontname=\"Helvetica\"];" << std::endl;

    if (root == nullptr)
      {
        out << "}" << std::endl;
        return;
      }

    // In preorder, as a recursion would go. The ancestors of the node wait
    // in a stack, to give the arcs to the right siblings of their children:
    // a path can be as long as the tree is deep, too long for recursion.
    Dfs_Stack<Node *> path;
    Node * p = root;
    while (true)
      {
        out << "  \"" << p << "\" [label=\""
            << escape_dot_label(Write()(p)) << "\"];" << std::endl;

        if (auto * child = static_cast<Node *>(p->get_left_child()))
          {
            out << "  \"" << p << "\" -> \"" << child << "\";" << std::endl;
            path.push(p);
            p = child;
            continue;
          }

        // the right sibling of p, or of its lowest ancestor that has one
        while (not path.is_empty() and p->is_rightmost())
          {
            p = path.top();
            path.pop();
          }

        if (path.is_empty())
          break;

        auto * sibling = static_cast<Node *>(p->get_right_sibling());
        out << "  \"" << path.top() << "\" -> \"" << sibling << "\";" << std::endl;
        p = sibling;
      }

    out << "}" << std::endl;
  }
} // end namespace Aleph

# endif // GENERATE_TREE_H
