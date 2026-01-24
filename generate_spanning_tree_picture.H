
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


/** @file generate_spanning_tree_picture.H
 *  @brief Spanning tree visualization with highlighted tree/non-tree edges.
 *
 *  This file provides utilities for generating visual representations of
 *  spanning trees where tree edges are distinguished from non-tree edges
 *  through shading/highlighting.
 *
 *  ## Key Features
 *
 *  - Visualize spanning trees with clear distinction between:
 *    - Tree edges (spanning tree arcs)
 *    - Non-tree edges (cross/back edges)
 *  - Support for both cross-layout and net-layout visualizations
 *  - Compatible with graphpic visualization tools
 *
 *  ## Shading Convention
 *
 *  The shading functors use the node/arc cookie field to determine
 *  if an element belongs to the spanning tree:
 *  - Cookie == nullptr: Not in spanning tree (normal display)
 *  - Cookie != nullptr: In spanning tree (shadowed/highlighted)
 *
 *  @see generate_graph.H For underlying graph visualization
 *  @see tpl_spanning_tree.H For spanning tree algorithms
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

#ifndef GENERATE_SPANNING_TREE_PICTURE_H
#define GENERATE_SPANNING_TREE_PICTURE_H

#include <generate_graph.H>

/** @brief Shading functor for spanning tree nodes.
 *
 *  Returns a graphpic shading command if the node belongs to the
 *  spanning tree (cookie != nullptr).
 *
 *  @tparam GT Graph type
 */
template <typename GT> 
struct Shade_Span_Node
{
  std::string operator () (typename GT::Node * p) const
  {
    return NODE_COOKIE(p) == nullptr ? "" : "SHADOW-NODE";
  }
};

/** @brief Shading functor for spanning tree arcs.
 *
 *  Returns a graphpic shading command based on whether the arc
 *  belongs to the spanning tree.
 *  - Tree arcs: "SHADOW-ARC" (highlighted)
 *  - Non-tree arcs: "ARC" (normal)
 *
 *  @tparam GT Graph type
 */
template <typename GT> 
struct Shade_Span_Arc
{
  std::string operator () (typename GT::Arc * a) const
  {
    return ARC_COOKIE(a) == nullptr ? "ARC" : "SHADOW-ARC"; 
  }
};

/** @brief Generate a cross-layout visualization of a spanning tree.
 *
 *  Creates a graphpic specification with nodes arranged in a cross
 *  pattern, with spanning tree edges highlighted.
 *
 *  @tparam GT Graph type
 *  @tparam Write_Node Functor to convert node content to string
 *  @tparam Write_Arc Functor to convert arc content to string
 *  @tparam SA Arc filter class
 *
 *  @param g The graph containing the spanning tree
 *  @param nodes_by_level Number of nodes per level in the layout
 *  @param xdist Horizontal distance between nodes
 *  @param ydist Vertical distance between levels
 *  @param output Output stream for the graphpic specification
 *
 *  @see generate_cross_graph() For the underlying layout function
 *
 *  @ingroup Graphs
 */
template <typename GT, class Write_Node, class Write_Arc, class SA>
void generate_cross_spanning_tree(GT &            g, 
                                  const size_t &  nodes_by_level, 
                                  const double &  xdist,
                                  const double &  ydist,
                                  std::ofstream & output)
{
  generate_cross_graph
    <GT, Write_Node, Write_Arc, Shade_Span_Node<GT>, Shade_Span_Arc<GT>, SA>
      (g, nodes_by_level, xdist, ydist, output);
}

/// @brief Overload with default arc filter.
/// @see generate_cross_spanning_tree()
template <typename GT, class Write_Node, class Write_Arc>
void generate_cross_spanning_tree(GT &            g, 
                                  const size_t &  nodes_by_level, 
                                  const double &  xdist,
                                  const double &  ydist,
                                  std::ofstream & output)
{
  using DSA = Dft_Show_Arc<GT>;
  generate_cross_spanning_tree<GT, Write_Node, Write_Arc, DSA>
    (g, nodes_by_level, xdist, ydist, output);
}

/** @brief Generate a net-layout visualization of a spanning tree.
 *
 *  Creates a graphpic specification with nodes arranged in a regular
 *  grid (net) pattern, with spanning tree edges highlighted.
 *
 *  @tparam GT Graph type
 *  @tparam Write_Node Functor to convert node content to string
 *  @tparam Write_Arc Functor to convert arc content to string
 *  @tparam SA Arc filter class
 *
 *  @param g The graph containing the spanning tree
 *  @param nodes_by_level Number of nodes per level in the layout
 *  @param xdist Horizontal distance between nodes
 *  @param ydist Vertical distance between levels
 *  @param output Output stream for the graphpic specification
 *
 *  @see generate_net_graph() For the underlying layout function
 *
 *  @ingroup Graphs
 */
template <typename GT, class Write_Node, class Write_Arc, class SA>
void generate_net_spanning_tree(GT &            g, 
                                const size_t &  nodes_by_level, 
                                const double &  xdist,
                                const double &  ydist,
                                std::ofstream & output)
{
  generate_net_graph
    <GT, SA, Write_Node, Write_Arc, Shade_Span_Node<GT>, Shade_Span_Arc<GT>>
      (g, nodes_by_level, xdist, ydist, output);
}

/// @brief Overload with default arc filter.
/// @see generate_net_spanning_tree()
template <typename GT, class Write_Node, class Write_Arc>
void generate_net_spanning_tree(GT &            g, 
                                const size_t &  nodes_by_level, 
                                const double &  xdist,
                                const double &  ydist,
                                std::ofstream & output)
{
  using DSA = Dft_Show_Arc<GT>;
  generate_net_spanning_tree<GT, Write_Node, Write_Arc, DSA>
    (g, nodes_by_level, xdist, ydist, output);
}

#endif // GENERATE_SPANNING_TREE_PICTURE_H
