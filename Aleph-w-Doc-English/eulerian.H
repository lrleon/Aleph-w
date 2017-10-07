
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
  @file eulerian.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef EULERIAN_H
# define EULERIAN_H

# include <tpl_graph.H>

namespace Aleph
{

/** 
  @brief Determines if a graph or digraph is Eulerian

  @details The Test_Eulerian class performed checks whether the graph or digraph is
  Euler; that is, if it contains an Eulerian cycle.

  The class receives three such parameters:
  - # GT: Graph
  - # SN: Filter node iterator.
  - # SA: Filter iterator arches.
  
  @ingroup Grafos
*/
template <class GT, 
	  class SN = Dft_Show_Node<GT>,
	  class SA = Dft_Show_Arc<GT> >
class Test_Eulerian
{
  SN & sn;
  SA & sa;

  bool test_graph(GT & g)
  {
    assert(not g.is_digraph());

    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next())
      if ((g.get_num_arcs(it.get_curr()) % 2) == 1)
	return false;

    return true;
  }

  bool test_digraph(GT & g)
  {
    assert(g.is_digraph());

    g.reset_counter_nodes();

/** 
  @brief A first pass on arches to count entry grades. 
*/
    for (Arc_Iterator<GT, SA> it(g, sa); it.has_current(); it.next())
      NODE_COUNTER(it.get_tgt_node())++;

    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next())
      {
	typename GT::Node * p = it.get_curr();

	if (g.get_num_arcs(p) != NODE_COUNTER(p))
	  return false;
      }

    return true;
  }

public:

  Test_Eulerian(SN && __sn = SN(), SA && __sa = SA()) 
    : sn(__sn), sa(__sa)
  {
    /* Empty */ 
  }

/** 
  @brief Returns true if the graph the Eulerian. 
*/
  bool operator () (GT & g)
  {
    if (g.is_digraph())
      return test_digraph(g);
    else
      return test_graph(g);
  }
};


} /* End namespace Aleph */

# endif // EULERIAN_H
