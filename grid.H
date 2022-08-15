
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
# ifndef GRID_H
# define GRID_H

namespace Aleph 
{ 

template <class GT>
struct Default_Operation_On_Node
{
  void operator () (GT &, typename GT::Node *, const size_t &, const size_t &) { /* Empty */ }
};

template <class GT>
struct Default_Operation_On_Arc
{
  void operator () (GT &, typename GT::Arc *, const size_t &, const size_t &) { /* Empty */ }
};

/**
   @author Alejandro Mujica
   @ingroup Grafos
 */
template <
          class GT,
          class Operation_On_Node = Default_Operation_On_Node<GT>,
          class Operation_On_Arc = Default_Operation_On_Arc<GT>
         >
class Build_Grid
{
public:
  void operator () (GT & g, const size_t & width, const size_t & height)
  {
    if (g.get_num_nodes() != 0)
      throw std::domain_error("There is nodes in graph");

    if (width < 2 or height < 2)
      throw std::length_error("The minimun size must be 2 x 2");

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
		if (j > 0) //Conecta con el nodo a su izquierda
		  {
		    typename GT::Arc * a = g.insert_arc(n, map[i][j - 1]);
		    Operation_On_Arc()(g, a, i, j);
		  }
		if (i > 0) //Conecta con el nodo que esta arriba
		  {
		    typename GT::Arc * a = g.insert_arc(n, map[i - 1][j]);
		    Operation_On_Arc()(g, a, i, j);
		  }
		if (i > 0 and j > 0) //Conecta con el nodo que esta arriba y a la izquierda
		  {
		    typename GT::Arc * a = g.insert_arc(n, map[i - 1][j - 1]);
		    Operation_On_Arc()(g, a, i, j);
		  }
		if (j + 1 < width and i > 0) //Conecta con el nodo que esta arriba y a la derecha
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

