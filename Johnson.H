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

/** @file Johnson.H
 *  @brief Johnson's algorithm for all-pairs shortest paths.
 *
 *  Implements Johnson's algorithm combining Bellman-Ford and Dijkstra
 *  for finding all-pairs shortest paths in sparse graphs, including
 *  graphs with negative edge weights. O(V²log V + VE) complexity.
 *
 *  @ingroup Grafos
 *  @author Leandro Rabindranath León
 */

# ifndef JOHNSON_H
# define JOHNSON_H

# include <Dijkstra.H>
# include <Bellman_Ford.H>


template <class GT,
          class Distance = Dft_Dist<GT>,
          class SA = Dft_Show_Arc<GT>>
class Jhonson
{
  void
  compute_ratio_and_diameter(Distance_Type & ratio, Node *rsrc, Node *rtgt,
                             Distance_Type & diameter, Node *dsrc, Node *dtgt)
  {}

public:
};


# endif // JOHNSON_H
