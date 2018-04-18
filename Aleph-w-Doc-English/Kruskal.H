
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
  @file Kruskal.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef KRUSKAL_H
# define KRUSKAL_H

# include <ahFunction.H>
# include <tpl_agraph.H>
# include <tpl_graph_utils.H>
# include <tpl_test_acyclique.H>
# include <tpl_union.H>

using namespace Aleph;

namespace Aleph {

/** 
  @brief Calculate the minimum spanning tree of a graph according to
  Kruskal algorithm.

  @details Kruskal's algorithm is recommended for graphs
  scattered.

  The procedure is configured with the following
  Specifications:
  - # GT: Graph type based on List_Graph.
  - # Distance <GT>: Read the weight class should bow
  export the following attributes:
  - # Typedef Distance <GT> :: Distance_Type: Data type
  It represents a weight in an arc.
  - # Distance <GT> Distance_Type :: operator () (typename GT :: Arc * a):
  that returns the value of the peso in the bow.
  - # Distance <GT> :: Max_Distance: Static Constant
  corresponding to the maximum distance value algorithm
  considered as infinite.
  - # Typename Distance <GT> :: Zero_Distance: Static Constant
  neutral element corresponding to the sum. Traditionally,
  in the vast majority of cases, this will be zero.
  .
  - # Compare <GT>: Class that performs the comparison between two
  weights and whose prototype is:
  @code
    - Bool operator () (const typename Distance <GT> :: Distance_Type & op1,
                      const typename Distance <GT> :: Distance_Type & op2)
  @endcode
  .
  By default, this class implements the relational operator
  less than.
  - # SA: Filter arches.

  @see Prim_Min_Spanning_Tree
  
  @ingroup Graphs
*/
 template <class GT, 
	   class Distance = Dft_Dist<GT>, 
	   class SA       = Dft_Show_Arc<GT>> 
class Kruskal_Min_Spanning_Tree 
{
  Distance & dist;
  SA      &  sa;
  bool       painted;
  
public:

/** 
  @brief Constructor.

  @param [inout] __dist Access to the distance of each arc
  @param [inout] __cmp Comparison arc distances
  @param [inout] __sa Filter iterator arches
*/
  Kruskal_Min_Spanning_Tree(Distance && __dist = Distance(),  
			    SA &&       __sa   = SA()) 
    : dist(__dist), sa(__sa), painted(false)
  { 
    /* Empty */ 
  }

  Kruskal_Min_Spanning_Tree(Distance & __dist, SA & __sa) 
    : dist(__dist), sa(__sa), painted(false)
  { 
    /* Empty */ 
  }

public:

/** 
  @brief Filter arches painted by Kruskal's algorithm
*/
      template <class G, class GT_SA>
  struct Paint_Filt
  {
    GT_SA & sa;

    Paint_Filt(GT_SA & __sa) : sa(__sa) { /* Empty */ }

    bool operator () (typename G::Arc * a) const
    {
      if (not sa(a))
	return false;

      return IS_ARC_VISITED(a, Aleph::Spanning_Tree);
    }
  };
 
private:

  struct Init_Node
  {
    long count;

    Init_Node() : count(0) { /* Empty */ }

    void operator () (const GT &, typename GT::Node * p)
    {
      NODE_COUNTER(p) = count++;
      NODE_BITS(p).set_bit(Aleph::Spanning_Tree, false);
    }
  };

  static bool arc_is_in_tree(Fixed_Relation & tree, long i, long j)
  {
    return tree.are_connected(i, j);
  }

public:

  void paint_min_spanning_tree(const GT & g)
  {
    if (g.is_digraph())
      throw std::domain_error("g is a digraph");

    g.reset_bit_arcs(Aleph::Spanning_Tree); /* Clean arches marking bits */
    Operate_On_Nodes<GT, Init_Node> () (g, Init_Node());
    
    typedef Distance_Compare<GT, Distance> DCMP;
    DCMP comp(dist);
    const_cast<GT&>(g).template sort_arcs<DCMP>(comp);
    const size_t V = g.get_num_nodes();

    Fixed_Relation tree(V);

    /** Browse ordered arcs g until number of arches
    tree equals the number of nodes g*/
    for (Arc_Iterator <GT, SA> it(g, sa); tree.get_num_blocks() > 1 and
	   it.has_curr(); it.next()) 
      {     /* Next minor arc */
	typename GT::Arc * arc = it.get_current_arc();
	long i = NODE_COUNTER(g.get_src_node(arc));
	long j = NODE_COUNTER(g.get_tgt_node(arc));
	if (arc_is_in_tree(tree, i, j))
	  continue;
	
	tree.join(i, j);
	ARC_BITS(arc).set_bit(Aleph::Spanning_Tree, true);
      }

    painted = true;
  }

  void paint_min_spanning_tree(const GT & g, GT & tree)
  {
    paint_min_spanning_tree(g);
    clear_graph(tree); /* Clean graph destination*/

    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next())
      {
	typename GT::Node * gp = it.get_curr();
	typename GT::Node * tp = tree.insert_node(gp->get_info());
	GT::map_nodes(gp, tp);
      }

    typedef Paint_Filt<GT, SA> F;
    for (Arc_Iterator<GT, F> it(g, F(sa)); it.has_curr(); it.next()) 
      {
	typename GT::Arc * ga    = it.get_current_arc();
	typename GT::Node * tsrc = mapped_node<GT>(g.get_src_node(ga));
	typename GT::Node * ttgt = mapped_node<GT>(g.get_tgt_node(ga));
	typename GT::Arc  * ta   = tree.insert_arc(tsrc, ttgt, ga->get_info());
	GT::map_arcs(ga, ta);
      }
  }

public:

/** 
  @brief Invokes the calculation of spanning tree according Kruskal.

  @param [in] g Graph to which you want to calculate the tree
  comprehensive minimum.
  @param [out] GT Graph tree where you want to save the tree
  comprehensive minimum result. This graph is cleaned before
  start algorithm.
    
  @throw bad_alloc If there is not enough memory to build
  tree. In this case the tree value is indeterminate and not clean.
*/
  void operator () (const GT & g, GT & tree) 
  {
    paint_min_spanning_tree(g, tree);
  }

/** 
  @brief Painted on the graph as the spanning tree Kruskal.

  @details the end of the algorithm, arcs g belonging to the tree
  encompassing are painted with Kruskal bit.

  @param [in] g Graph to which you want to calculate the tree
  comprehensive minimum.

  @throw bad_alloc If there is not enough memory to build
  tree. In this case the tree value is indeterminate and not clean.
*/
  void operator () (const GT & g) 
  {
    paint_min_spanning_tree(g);
  }
};
      

} /* End namespace Aleph */

# endif // KRUSKAL_H
