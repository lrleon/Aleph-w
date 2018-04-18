
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
  @file tpl_bipartite.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_BIPARTITE_H
# define TPL_BIPARTITE_H

# include <tpl_dynDlist.H>
# include <tpl_netgraph.H>

namespace Aleph {

/** @brief Enumeración de los colores de bipártito */
enum Bipartite_Color { Bp_White, Bp_Red, Bp_Blue };

    template <class GT> 
static long & color(typename GT::Node * p)
{
  return NODE_COUNTER(p);
}

    template <class GT> 
static long & color(typename GT::Arc * a)
{
  return ARC_COUNTER(a);
}

/** 
  @brief Take a bipartite graph and calculates the sets of partition.
	
  @details A graph is bipartite if it can be divided into two subset l and r
  such that every node of l simply arcs to nodes r and vice versa.
  
  compute_bipartite(g,l,r) take a bipartite graph g and calculates
  bipartition sets l and r appointed by the parameters,
  respectively.

  @param[in] g Bipartite graph.
  @param[out] l Set partition.
  @param[out] r Set partition.
	
  @throw domain_error If during the calculation is determined 
  that the bipartite graph is not.
	@throw bad_alloc If there is insufficient memory.
	
  @ingroup Grafos 
*/
     template <class GT, class SA = Dft_Show_Arc<GT> > 
void compute_bipartite(const GT &                      g, 
		       DynDlist<typename GT::Node *> & l,
		       DynDlist<typename GT::Node *> & r)
{
  g.reset_nodes(); /* Initializes counters in White */
  g.reset_arcs();

  DynDlist<typename GT::Node *> red, blue;
  typename GT::Node * p = g.get_first_node();
  color<GT>(p) = Bp_Red;
  red.put(p); l.put(p);  

  while (true)
    if (not red.is_empty()) /* Does not looked red arcs? */
      {
	typename GT::Node * p = red.get();
	for (Node_Arc_Iterator<GT, SA> it(p); it.has_current(); it.next())
	  {
 	    typename GT::Arc * a = it.get_current();
 	    if (color<GT>(a) == Bp_Red)
 	      throw std::domain_error("Graph is not bipartite");
 	    else if (color<GT>(a) == Bp_Blue)
 	      continue;

 	    color<GT>(a) = Bp_Red; 
	    typename GT::Node * q = it.get_tgt_node();
	    if (color<GT>(q) == Bp_Red)
	      throw std::domain_error("Graph is not bipartite");	      
	    else if (color<GT>(q) == Bp_Blue)
	      continue;

	    color<GT>(q) = Bp_Blue;
	    blue.put(q); r.put(q);
	  }
      }
    else if (not blue.is_empty()) /* Does not looked blue with arcs? */
      {
	typename GT::Node * p = blue.get();
	for (Node_Arc_Iterator<GT, SA> it(p); it.has_current(); it.next())
	  {
 	    typename GT::Arc * a = it.get_current();
 	    if (color<GT>(a) == Bp_Blue)
 	      throw std::domain_error("Graph is not bipartite");
 	    else if (color<GT>(a) == Bp_Red)
 	      continue;

 	    color<GT>(a) = Bp_Blue;

	    typename GT::Node * q = it.get_tgt_node();
	    if (color<GT>(q) == Bp_Blue)
	      throw std::domain_error("Graph is not bipartite");
	    else if (color<GT>(q) == Bp_Red)
	      continue;

	    color<GT>(q) = Bp_Red;
	    red.put(q); l.put(q);
	  }
      }
    else
      break;
}

/** 
  @brief Class taking a bipartite graph and calculates the sets of partition.
	
  @details A graph is bipartite if it can be divided into two subsets l and r
  such that every node of l simply arcs to nodes r and vice versa.
       
	@ingroup Grafos
*/
    template <class GT, class SA = Dft_Show_Arc<GT> > 
class Compute_Bipartite
{
public:
  
/** 
  @brief Calculate sets partition of a bipartite graph.

  @param[in] g Bipartite graph.
  @param[out] l Set partition.
  @param[out] r Set partition.
	
  @throw domain_error If during the calculation is determined 
  that the bipartite graph is not.
	@throw bad_alloc If there is insufficient memory.
*/
  void operator () (const GT &                      g, 
		    DynDlist<typename GT::Node *> & l,
		    DynDlist<typename GT::Node *> & r)
  {
    compute_bipartite <GT, SA> (g, l, r);
  }
};

/** 
  @brief Calculate the maximum cardinality matching of a bipartite graph.

  @details compute_maximum_cardinality_bipartite_matching(g,matching)
  receive a bipartite graph g and calculates the maximum matching
  bipartite matching on the list.

  The procedure computes the bipartition sets, then
  builds a network equivalent unit trained on it
  invokes a flow maximization algorithm.

  The routine operates two type parameters:
  -# GT Bipartite graph type
  -# Max_Flow Maximization algorithm flow used
  to perform the calculation.
  .

  @param[in] g Bipartite graph.
  @param[out] matching List of components arches of pairing.

  @throw bad_alloc If there is insufficient memory.

  @ingroup Grafos 
*/
     template <class GT, 
	       template <class> class Max_Flow = Ford_Fulkerson_Maximum_Flow, 
	       class SA = Dft_Show_Arc<GT>> 
void compute_maximum_cardinality_bipartite_matching
  (const GT & g, DynDlist<typename GT::Arc *> & matching)
{
  DynDlist<typename GT::Node *> l, r;

  compute_bipartite(g, l, r);

  typedef Net_Graph<Net_Node<Empty_Class>, Net_Arc<Empty_Class> > AN;
  AN net;

    /** Browse nodes g and insert image net */
  for (Node_Iterator<GT> it(g); it.has_current(); it.next())
  {
    typename GT::Node * p = it.get_current();
    NODE_COOKIE(p) = net.insert_node();
    NODE_COOKIE((typename GT::Node *)NODE_COOKIE(p)) = p;
  }

  typename AN::Node * source = net.insert_node();

    /** Browse nodes l, tie them to source and insert their arcs */
  for (typename DynDlist<typename GT::Node*>::Iterator i(l); 
       i.has_current(); i.next())
    {
      typename GT::Node * p = i.get_current();
      typename AN::Node * src = mapped_node<GT, AN> (p);
      net.insert_arc(source, src, 1);

      for (Node_Arc_Iterator<GT, SA> j(p); j.has_current(); j.next())
	{
	  typename GT::Arc * arc = j.get_current_arc();
	  typename AN::Node * tgt = mapped_node <GT, AN> (g.get_tgt_node(arc));
	  typename AN::Arc * a = net.insert_arc(src, tgt, 1);
	  ARC_COOKIE(arc) = a;
	  ARC_COOKIE(a) = arc;
	}
    }

  typename AN::Node * sink = net.insert_node();

    /* Browse r nodes and tie the drain */
  for (typename DynDlist<typename GT::Node*>::Iterator it(r); 
       it.has_current(); it.next())
    {
      typename GT::Node * p = it.get_current();
      net.insert_arc(mapped_node<GT, AN> (p), sink, 1);
    }

  Max_Flow <AN> () (net); 

  for (Arc_Iterator<AN> it(net); it.has_current(); it.next())
    {
      typename AN::Arc * a = it.get_current();
      if (a->flow == 0) 
	continue;

      typename GT::Arc * arc = mapped_arc <AN, GT> (a);
      if (arc == NULL)
	continue;

      matching.append(arc);
    }
}

/** 
  @brief Class pairing calculation maximum 
  cardinality of a bipartite graph.

  @details The class manages two type parameters:
  -# GT Bipartite graph type
  -# Max_Flow Maximization algorithm flow used
  to perform the calculation. By default algorithm is used
  Ford_Fulkerson
  .

  @ingroup Grafos 
*/
     template <class GT, 
	       template <class> class Max_Flow = Ford_Fulkerson_Maximum_Flow,
	       class SA = Dft_Show_Arc<GT> > 
class Compute_Maximum_Cardinality_Bipartite_Matching
{
public:

/** 
  @brief Calculate the maximum cardinality matching of a bipartite graph.

  @details compute_maximum_cardinality_bipartite_matching(g,matching)
  receive a bipartite graph g and calculates the maximum matching
  bipartite matching on the list.

  The procedure computes the bipartition sets, then
  builds a network equivalent unit trained on it
  invokes a flow maximization algorithm.

  @param[in] g Bipartite graph.
  @param[out] matching List of components arches of pairing.
  
  @throw bad_alloc If there is insufficient memory.
*/
  void operator () (const GT & g, DynDlist<typename GT::Arc *> & matching)
  {
    compute_maximum_cardinality_bipartite_matching <GT, Max_Flow, SA>
      (g, matching);
  }
};


} /* End namespace Aleph */

# endif //  TPL_BIPARTITE_H
