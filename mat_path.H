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
# ifndef MAT_PATH_H
# define MAT_PATH_H

# include <tpl_matgraph.H>

namespace Aleph {

// Forward declaration
template <class Mat>
void find_min_path(Mat & p, long src_index, long tgt_index,
                   Path<typename Mat::Graph_Type> & path);

/** @brief Build a minimum path from Floyd-Warshall path matrix using node pointers.
 *
 *  Takes a path matrix p computed by Floyd-Warshall algorithm and two node
 *  pointers (source and target) from the adjacency list representation,
 *  then constructs the minimum path.
 *
 *  This function should be called after floyd_all_shortest_paths() which
 *  computes the path matrix p.
 *
 *  @warning Results are undefined if the matrix p is invalid.
 *
 *  @tparam Mat Matrix type from Map_Matrix_Graph.
 *  @param[in] p Path index matrix from Floyd-Warshall.
 *  @param[in] src_node Source node pointer in adjacency list representation.
 *  @param[in] tgt_node Target node pointer in adjacency list representation.
 *  @param[out] path The minimum path in the adjacency list graph.
 *  @throw std::bad_alloc If insufficient memory to build the path.
 *  @see floyd_all_shortest_paths()
 *  @ingroup Grafos
 */
template <class Mat>
void find_min_path(Mat & p, typename Mat::Node * src_node, 
                   typename Mat::Node * tgt_node,
                   Path<typename Mat::Graph_Type> & path)
{
  find_min_path(p, p(src_node), p(tgt_node), path);
}

/** @brief Functor wrapper for find_min_path.
 *  @tparam Mat Matrix type from Map_Matrix_Graph.
 *  @ingroup Grafos
 */
template <class Mat>
class Find_Min_Path
{
public:

  void operator()(Mat & p, 
                  typename Mat::Node * src_node, 
                  typename Mat::Node * tgt_node,
                  Path<typename Mat::Graph_Type> & path) const
  {
    find_min_path<Mat>(p, src_node, tgt_node, path);
  }

  void operator()(Mat & p, 
                  long src_index, long tgt_index,
                  Path<typename Mat::Graph_Type> & path) const
  {
    find_min_path<Mat>(p, src_index, tgt_index, path);
  }
};

/** @brief Build a minimum path from Floyd-Warshall path matrix using indices.
 *
 *  Takes a path matrix p computed by Floyd-Warshall algorithm and two
 *  matrix indices (source and target), then constructs the minimum path
 *  in the adjacency list representation.
 *
 *  This function should be called after floyd_all_shortest_paths() which
 *  computes the path matrix p.
 *
 *  @warning Results are undefined if the matrix p is invalid.
 *
 *  An overloaded version takes node pointers instead of matrix indices.
 *
 *  @tparam Mat Matrix type from Map_Matrix_Graph.
 *  @param[in] p Path index matrix from Floyd-Warshall.
 *  @param[in] src_idx Source node index in the path matrix.
 *  @param[in] tgt_idx Target node index in the path matrix.
 *  @param[out] path The minimum path in the adjacency list graph.
 *  @throw std::bad_alloc If insufficient memory to build the path.
 *  @see floyd_all_shortest_paths()
 *  @ingroup Grafos
 */
template <class Mat>
void find_min_path(Mat & p, long src_idx, long tgt_idx,
                   Path<typename Mat::Graph_Type> & path)
{
  using GT = typename Mat::Graph_Type;
  GT & g = p.get_list_graph();
  typename GT::Node * src = p(src_idx);
  path.set_graph(g, src);
  
  for (long i = src_idx, j = p(i, tgt_idx); ; i = j, j = p(i, tgt_idx))
    {
      path.append(p(j));
      if (j == tgt_idx)
        break;
    }
}

} // end namespace Aleph

# endif // MAT_PATH_H
