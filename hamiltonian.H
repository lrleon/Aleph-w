/* 
  This file is part of Aleph-w library

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018

  Leandro Rabindranath Leon / Alejandro Mujica

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <https://www.gnu.org/licenses/>.
*/
# ifndef HAMILTONIAN_H
# define HAMILTONIAN_H

# include <tpl_graph.H>

namespace Aleph 
{

    /** Determina si un grafo o digrafo satisface las pruebas de
	suficiencia para ser hamiltoniano.

	La clase Test_Hamiltonian_Sufficiency efectúa una prueba de
	suficiencia hamiltoniana para el grafo o digrafo parámetro.

	La clase recibe tres parámetros tipo:
	-# GT: el grafo
	-# SN: clase filtro para el iterador de nodos.
	-# SA: clase filtro para el iterador de arcos.
	.

	La prueba está basada en el teorema de suficiencia de Ore.

	@ingroup Grafos
	@author Leandro R. León
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

      // una primera pasada sobre arcos para contar grados de entrada
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

	        // buscar si hay arco src-->tgt
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
    // empty 
  }

      // retorna true si el grafo o digrafo satisface la prueba de
      // suficiencia para ser hamiltoniano.
  bool operator () (GT & g)
  {
    if (g.is_digraph())
      return test_digraph(g);
    else
      return test_graph(g);
  }
};



} // end namespace Aleph

# endif // HAMILTONIAN_H
