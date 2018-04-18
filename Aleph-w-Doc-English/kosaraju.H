
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
  @file kosaraju.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef KOSARAJU_H
# define KOSARAJU_H

# include <tpl_graph_utils.H>

namespace Aleph {

     template <class GT, class SA> inline static
void __dfp(GT & g, typename GT::Node * p, DynArray<typename GT::Node*> & df)
{
  if (IS_NODE_VISITED(p, Depth_First))
    return;

  NODE_BITS(p).set_bit(Depth_First, true); 

    /** Walking in deep arcs p */
  for (Node_Arc_Iterator<GT,SA> it(p); it.has_current(); it.next())
    {
      typename GT::Arc * a = it.get_current_arc();
      if (IS_ARC_VISITED(a, Depth_First))
        continue;

      ARC_BITS(a).set_bit(Depth_First, true); 

      __dfp<GT,SA>(g, it.get_tgt_node(), df);
    }
  df.append(p); /* Asignation of df(p) in suffix */
  NODE_COUNTER(p) = df.size();
}

/** 
  @brief Walk the graph invested and inserts achievable nodes in the sub-graph blk.
  Each block will place the color
*/
     template <class GT, class SA> inline static
void __dfp(GT & g, typename GT::Node * p, GT & blk, const int & color)
{
  if (IS_NODE_VISITED(p, Depth_First))
    return;

  NODE_BITS(p).set_bit(Depth_First, true); 

  typename GT::Node * q = blk.insert_node(p->get_info());
  NODE_COUNTER(q) = color;
  GT::map_nodes(p, q);

  for (Node_Arc_Iterator<GT,SA> it(p); it.has_current(); it.next())
    {
      typename GT::Arc * a = it.get_current_arc();

      if (IS_ARC_VISITED(a, Depth_First))
        continue;
      ARC_BITS(a).set_bit(Depth_First, true); 

      __dfp<GT,SA>(g, it.get_tgt_node(), blk, color);
    }
}

/** 
  @brief Calculates strongly connected components of a graph according to
  Kosaraju algorithm.

  @details kosaraju_connected_components () Takes a digraph g,
  "Supposedly weakly related" calculates its sub-graphs or components
  disconnected and stored in a dynamic list of subdigrafos list
  mapped both nodes and arcs. The emphasis on
  Of course you connected graph character is that if the graph does not
  it was connected, then the resulting list would contain a single
  copy the digraph corresponding element mapping g. If this
  were the case, then it is best done with the function
  copy_graph ().

  The function is based on the algorithm of Tarjan.

  The function takes two type parameters:
  - # GT: Class digraph.
  - # SA: Arcs filter employing internal iterator.
  .

  The function uses the bit to mark nodes and build_subtree
  arches visited.

  The algorithm finds the components internally by
  depth searches.

  @param [in] g Graph.
  @param [out] blk_list List mapped sub-graphs ag
  corresponding to the strongly connected components of g.
  @param [out] arc_list List arcs crossing between components.

  @see tarjan_connected_components ()
  
  @ingroup Graphs
*/
     template <class GT, class SA> inline 
void kosaraju_connected_components(GT &                           g, 
                                   DynDlist <GT> &                blk_list,
                                   DynDlist<typename GT::Arc *> & arc_list)
{
  g.reset_nodes();
  g.reset_arcs();

  DynArray<typename GT::Node*> df; /* df arrangement by suffix */

  /** Walking in depth and mark suffix nodes */
  for (Node_Iterator<GT> it(g); it.has_current() and df.size() < g.vsize();
       it.next())
    __dfp<GT,SA>(g, it.get_current(), df);

  GT gi; 
  invert_digraph<GT,SA>(g, gi); /* gi is the inverted graph of g */

  DynArray<GT*> array; /* Arrangement of sub-graphs */

  for (int i = df.size() - 1, color = 0; i >= 0; i--)
    {
      typename GT::Node * gp = df.access(i);
      typename GT::Node * bp = mapped_node<GT>(gp);

      if (IS_NODE_VISITED(bp, Depth_First))
        continue;
      
      GT & blk = blk_list.append(GT());
      array[color] = &blk;
      
      __dfp<GT,SA>(gi, bp, blk, color++); /* Runs the reverse graph*/
                                             
      assert(NODE_COLOR(mapped_node<GT>(bp)) == color - 1); /* Insert nodes in blk */
      
    }

  for (Arc_Iterator<GT,SA> it(g); it.has_current(); it.next())
    {
      typename GT::Arc  * a  = it.get_current();
      typename GT::Node * gs = g.get_src_node(a);
      typename GT::Node * gt = g.get_tgt_node(a);

      typename GT::Node * bs = mapped_node<GT>(mapped_node<GT>(gs));
      typename GT::Node * bt = mapped_node<GT>(mapped_node<GT>(gt));

      const long & color = NODE_COLOR(bs);

      if (color == NODE_COLOR(bt))
        {
          typename GT::Arc * ba = array.access(color)->insert_arc(bs, bt);
          GT::map_arcs(a, ba);
        }
      else 
        arc_list.append(a); 
    }
} 


     template <class GT> inline 
void kosaraju_connected_components(GT &                          g, 
                                   DynDlist <GT> &               blk_list,
                                   DynDlist<typename GT::Arc*> & arc_list)
{
  kosaraju_connected_components<GT,Dft_Show_Arc<GT> >
    (g, blk_list, arc_list);
}


/** 
  @brief Walk the graph invested and inserts the nodes in the list achievable list.
  Each node will place the color 
*/
     template <class GT, class SA> inline static
void __dfp(GT & g, typename GT::Node * p, DynDlist<typename GT::Node *> & list)
{
  if (IS_NODE_VISITED(p, Depth_First))
    return;

  NODE_BITS(p).set_bit(Depth_First, true); 

  list.append(mapped_node<GT>(p));

  for (Node_Arc_Iterator<GT,SA> it(p); it.has_current(); it.next())
    {
      typename GT::Arc * a = it.get_current_arc();

      if (IS_ARC_VISITED(a, Depth_First))
        continue;
      ARC_BITS(a).set_bit(Depth_First, true); 

      __dfp<GT,SA>(g, it.get_tgt_node(), list);
    }
}

/** 
  @brief Calculates strongly connected components of a graph according to
   Kosaraju algorithm.

  @details kosaraju_connected_components () Takes a digraph g,
  "supposedly weakly related" calculates its subgraphs or components
  disconnected and stored in a dynamic list of sub-digraphs list
  mapped both nodes and arcs. The emphasis on
  Of course you connected graph character is that if the graph does not
  it was connected, then the resulting list would contain a single
  copy the digraph corresponding element mapping g. If this
  were the case, then it is best done with the function
  copy_graph ().

  The function is based on Kosaraju algorithm, which is not the
  most efficient it is known but it is the simplest.
  The function takes two type parameters:
  -# GT: Type of digraph.
  -# SA: arcs filter employing internal iterator.
  .

  The function uses the bit to mark nodes and build_subtree
  arches visited.

  The algorithm finds the components internally by
  depth searches.

  The difference and advantage of this routine about the version
  calculates the direct blocks is its speed; the block are
  included in lists of lists of nodes and not on list
  sub-graphs.

  @param [in] g Graph.
  @param [out] list List lists nodes that make up the
  components. Each list is a list of component nodes.

  @see tarjan_connected_components ()

  @ingroup Graphs
*/
    template <class GT, class SA> inline 
void kosaraju_connected_components(GT & g, 
                                   DynDlist<DynDlist<typename GT::Node*> > & list)
{
  g.reset_nodes();
  g.reset_arcs();
  DynArray<typename GT::Node*> df; /** Df arrangement by suffix */

    /** Walking in depth and mark suffix nodes */
  for (Node_Iterator<GT> it(g); it.has_current() and df.size() < g.vsize();
       it.next())
    __dfp<GT,SA>(g, it.get_current(), df);

  GT gi; 
  invert_digraph<GT,SA>(g, gi); /* gi is the inverted graph of g*/

  for (int i = df.size() - 1; i >= 0; i--)
    {
      typename GT::Node * gp = df.access(i);
      typename GT::Node * bp = mapped_node<GT>(gp);
      if (IS_NODE_VISITED(bp, Depth_First))
        continue;

      DynDlist<typename GT::Node*> & blk = 
        list.append(DynDlist<typename GT::Node*>());
      
      __dfp<GT,SA>(gi, bp, blk);
    }
} 


     template <class GT> inline 
void kosaraju_connected_components(GT & g, 
                                   DynDlist<DynDlist<typename GT::Node*> > & list)

{
  kosaraju_connected_components<GT,Dft_Show_Arc<GT> > (g, list);
}

/** 
  @brief Calculates strongly connected components of a graph according to
  Kosaraju algorithm.

  @details Kosaraju_Connected_Components () takes a digraph g,
  "supposedly weakly related" calculates its subgraphs or components
  disconnected and stored in a dynamic list of sub-digraphs list
  mapped both nodes and arcs. The emphasis on
  Of course you connected graph character is that if the graph does not
  it was connected, then the resulting list would contain a single
  copy the digraph corresponding element mapping g. If this
  were the case, then it is best done with the function
  copy_graph ().

  The class is based on the algorithm Kosaraju.

  The function takes two type parameters:
  - # GT: Class digraph.
  - # SA: Arcs filter employing internal iterator.
  .
  
  The function uses the bit to mark nodes and build_subtree
  arcs visited.

  The algorithm finds the components internally by
  depth searches.
    
  @see Tarjan_Connected_Components ()
    
  @ingroup Graphs
*/
    template <class GT, class SA = Dft_Show_Arc<GT> > 
class Kosaraju_Connected_Components
{
public:

/** 
  @brief Computes the strongly connected components of a graph according to
  Kosaraju algorithm.

  @param [in] g Graph.
  @param [out] blk_list List mapped sub-graphs ag
  corresponding to the strongly connected components of g.
  @param [out] arc_list List arches crossing between components.
*/
  void operator () (GT &                           g, 
                    DynDlist <GT> &                blk_list, 
                    DynDlist<typename GT::Arc *> & arc_list)
  {
    kosaraju_connected_components<GT, SA> (g, blk_list, arc_list);
  }

/** 
  @brief Calculates strongly connected components of a graph according to
  Kosaraju algorithm.

  @param [in] Graph g.
  @param [out] Lists nodes that make up the components.
  Each list is a list of component nodes.
*/
  void operator () (GT & g, DynDlist<DynDlist<typename GT::Node*> > & list)
  {
    kosaraju_connected_components<GT, SA> (g, list);
  }
};

} /* End namespace Aleph */

# endif // KOSARAJU_H
