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

/** @file grid.H
 *  @brief 2D grid graph generation with 8-connectivity.
 *
 *  This file provides the Build_Grid functor for generating rectangular
 *  grid graphs where each node is connected to its 8 neighbors (including
 *  diagonals).
 *
 *  ## Grid Structure
 *
 *  ```
 *  (0,0)---(0,1)---(0,2)
 *    |\     |\     |
 *    | \    | \    |
 *  (1,0)---(1,1)---(1,2)
 *    |\     |\     |
 *    | \    | \    |
 *  (2,0)---(2,1)---(2,2)
 *  ```
 *
 *  Each interior node has 8 connections (orthogonal + diagonal).
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  List_Graph<int, double> g;
 *  Build_Grid<decltype(g)> builder;
 *  builder(g, 10, 10);  // Create 10x10 grid
 *  // g now has 100 nodes, ~320 arcs
 *  ```
 *
 *  @see random_graph.H Random graph generation
 *  @see tpl_graph.H Graph types
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 *  @author Alejandro Mujica
 */

# ifndef GRID_H
# define GRID_H

namespace Aleph 
{ 

/**
 * @brief Default no-op operation for node initialization.
 *
 * @tparam GT Graph type
 */
template <class GT>
struct Default_Operation_On_Node
{
  /**
   * @brief Called for each created node.
   *
   * @param g The graph (unused)
   * @param n The created node (unused)
   * @param row Row index of node
   * @param col Column index of node
   */
  void operator () (GT &, typename GT::Node *, const size_t &, const size_t &) { /* Empty */ }
};

/**
 * @brief Default no-op operation for arc initialization.
 *
 * @tparam GT Graph type
 */
template <class GT>
struct Default_Operation_On_Arc
{
  /**
   * @brief Called for each created arc.
   *
   * @param g The graph (unused)
   * @param a The created arc (unused)
   * @param row Row index of source node
   * @param col Column index of source node
   */
  void operator () (GT &, typename GT::Arc *, const size_t &, const size_t &) { /* Empty */ }
};

/**
 * @brief Functor to build a 2D grid graph with 8-connectivity.
 *
 * Creates a rectangular grid of nodes where each node is connected
 * to all its neighbors: left, right, up, down, and all four diagonals.
 *
 * @tparam GT Graph type (e.g., List_Graph, Array_Graph)
 * @tparam Operation_On_Node Callback for node initialization
 * @tparam Operation_On_Arc Callback for arc initialization
 *
 * ## Complexity
 *
 * - Time: O(width × height)
 * - Space: O(width × height) for nodes
 *
 * ## Node Connectivity
 *
 * | Position | Connections |
 * |----------|-------------|
 * | Corner | 3 |
 * | Edge | 5 |
 * | Interior | 8 |
 *
 * @par Example
 * @code
 * // Create grid with custom weight initialization
 * struct WeightArc {
 *   void operator()(Graph& g, Arc* a, size_t i, size_t j) {
 *     a->get_info() = 1.0;  // Unit weight
 *   }
 * };
 *
 * List_Graph<int, double> g;
 * Build_Grid<decltype(g), Default_Operation_On_Node<decltype(g)>, WeightArc> builder;
 * builder(g, 5, 5);
 * @endcode
 *
 * @ingroup Graphs
 * @author Alejandro Mujica
 */
template <
          class GT,
          class Operation_On_Node = Default_Operation_On_Node<GT>,
          class Operation_On_Arc = Default_Operation_On_Arc<GT>
         >
class Build_Grid
{
public:

  /**
   * @brief Build a grid graph.
   *
   * Creates a width × height grid of nodes with 8-connectivity.
   *
   * @param g Graph to populate (must be empty)
   * @param width Number of columns (≥ 2)
   * @param height Number of rows (≥ 2)
   *
   * @throw std::domain_error if graph is not empty
   * @throw std::length_error if width or height < 2
   *
   * @post g contains width*height nodes with grid connectivity
   */
  void operator () (GT & g, const size_t & width, const size_t & height)
  {
    ah_domain_error_if(g.get_num_nodes() != 0)
        << "There is nodes in graph";

    ah_length_error_if(width < 2 or height < 2)
        << "The minimun size must be 2 x 2";

    typename GT::Node *** map = new typename GT::Node **[height];
    for (size_t i = 0; i < height; ++i)
      {
	try
	  {
	    map[i] = new typename GT::Node *[width];
	    for (size_t j = 0; j < width; ++j)
	      {
		typename GT::Node * n = g.insert_node(typename GT::Node_Type());
		Operation_On_Node()(g, n, i, j);
		map[i][j] = n;
		if (j > 0) // Connect to left neighbor
		  {
		    typename GT::Arc * a = g.insert_arc(n, map[i][j - 1]);
		    Operation_On_Arc()(g, a, i, j);
		  }
		if (i > 0) // Connect to upper neighbor
		  {
		    typename GT::Arc * a = g.insert_arc(n, map[i - 1][j]);
		    Operation_On_Arc()(g, a, i, j);
		  }
		if (i > 0 and j > 0) // Connect to upper-left diagonal
		  {
		    typename GT::Arc * a = g.insert_arc(n, map[i - 1][j - 1]);
		    Operation_On_Arc()(g, a, i, j);
		  }
		if (j + 1 < width and i > 0) // Connect to upper-right diagonal
		  {
		    typename GT::Arc * a = g.insert_arc(n, map[i - 1][j + 1]);
		    Operation_On_Arc()(g, a, i, j);
		  }
	      }
	  }
	catch (...)
	  {
	    for (size_t k = i - 1; k >= 0; --k)
	      delete [] map[k];
	    delete [] map;
	    clear_graph(g);
	    throw;
	  }
      }

    for (size_t i = 0; i < height; ++i)
      delete [] map[i];
    delete [] map;
  }
};

}

# endif
