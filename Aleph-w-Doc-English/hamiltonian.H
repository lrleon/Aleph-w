
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
  @file hamiltonian.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef HAMILTONIAN_H
# define HAMILTONIAN_H

# include <tpl_graph.H>

namespace Aleph 
{

/** 
  @brief Determines if a graph or digraph meets the tests
  sufficiency to be Hamiltonian.

  @details The Test_Hamiltonian_Sufficiency class performs a test
  Hamiltonian sufficiency for the graph or digraph parameter.

  The class receives three such parameters:
  - # GT: Graph
  - # SN: Filter node iterator.
  - # SA: Filter iterator arches.
  .

  The test is based on the sufficiency of Ore theorem.

  @ingroup Graphs
*/
template <class GT, 
	  class SN = Dft_Show_Node<GT>,
	  class SA = Dft_Show_Arc<GT> >
class Test_Hamiltonian_Sufficiency
{
  SN & sn;
  SA & sa;

  bool test_graph(GT & g)
  {
    assert(not g.is_digraph());

    const size_t & n = g.get_num_nodes();

    for (Node_Iterator<GT, SN> i(g, sn); i.has_curr(); i.next())
      {
	typename GT::Node * src = i.get_curr();
	const size_t & nsrc = g.get_num_arcs(src);

	i.next();

	for (Node_Iterator<GT, SN> j(i); j.has_curr(); j.next())
	  if (nsrc + g.get_num_arcs(j.get_curr()) < n)
	    return false;
      }

    return true;
  }

  bool test_digraph(GT & g)
  {
    assert(g.is_digraph());

    g.reset_counter_nodes();

    /** A first pass on arches to count degrees of input */
    for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next())
      NODE_COUNTER(it.get_tgt_node())++;

    const size_t & n = g.get_num_nodes();

    for (Node_Iterator<GT, SN> i(g, sn); i.has_curr(); i.next())
      {
	typename GT::Node * src = i.get_curr();

	for (Node_Iterator<GT, SN> j(g, sn); j.has_curr(); j.next())
	  {
	    typename GT::Node * tgt = j.get_current();

	    if (src == tgt)
	      continue;

	    if (g.get_num_arcs(src) + NODE_COUNTER(tgt) >= n)
	      continue;

/** @brief Looks for a bow src-->tgt */
	    bool terminate = true;
	    for (Node_Arc_Iterator<GT, SA> it(src, sa); it.has_curr(); 
		 it.next())
	      if (src == it.get_tgt_node())
		{
		  terminate = false;
		  break;
		}
	    
	    if (terminate)
	      return false;
	  }
      } 

    return true;
  }

public:

  Test_Hamiltonian_Sufficiency(SN && __sn = SN(), SA && __sa = SA()) 
    : sn(__sn), sa(__sa)
  {
    /* Empty */
  }

/** 
  @brief Returns true if the graph or digraph satisfies the test
  sufficiency to be Hamiltonian. 
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

# endif // HAMILTONIAN_H
