
/*
  This file is part of Aleph-w system

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014
  Leandro Rabindranath León
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met: 

  1. Redistributions of source code must retain the above copyright 
     notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright 
     notice, this list of conditions and the following disclaimer in the 
     documentation and/or other materials provided with the distribution.

  3. All advertising materials mentioning features or use of this software
     must display the following acknowledgement:

     Copyright (c) 2002-2014 Leandro Rabindranath León. See details of 
     licence.     

     This product includes software developed by the Hewlett-Packard
     Company, Free Software Foundation and Silicon Graphics Computer
     Systems, Inc. 

  4. Neither the name of the ULA nor the names of its contributors may
     be used to endorse or promote products derived from this software
     without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY Leandro Rabindranath León ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Aleph-w is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.

  I request users of this software to return to 

  Leandro Rabindranath Leon
  CEMISID 
  Ed La Hechicera 
  3er piso, ala sur
  Facultad de Ingenieria 
  Universidad de Los Andes 
  Merida - REPÚBLICA BOLIVARIANA DE VENEZUELA    or

  leandro.r.leon@gmail.com

  any improvements or extensions that they make and grant me the rights
  to redistribute these changes.  
*/

/**
  @file grid.H
  @date 2002 - 2014
  @author Leandro León & Alejandro Mujica (Source Code), Julio Criollo (Documentation)
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

/** @ingroup Graphs */
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
		if (j > 0) /* Connect to the node on the left */
		  {
		    typename GT::Arc * a = g.insert_arc(n, map[i][j - 1]);
		    Operation_On_Arc()(g, a, i, j);
		  }
		if (i > 0) /*Connect to the node that is up */
		  {
		    typename GT::Arc * a = g.insert_arc(n, map[i - 1][j]);
		    Operation_On_Arc()(g, a, i, j);
		  }
		if (i > 0 and j > 0) /* Connect to the node that is above and to the left */
		  {
		    typename GT::Arc * a = g.insert_arc(n, map[i - 1][j - 1]);
		    Operation_On_Arc()(g, a, i, j);
		  }
		if (j + 1 < width and i > 0) /* Connect to the node that is up and right */
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
