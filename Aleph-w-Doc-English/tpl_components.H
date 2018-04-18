 
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
  @file tpl_components.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_COMPONENTS_H
# define TPL_COMPONENTS_H

# include <htlist.H>

namespace Aleph {


/** 
  @brief Build a mappingbrief ga subgraph of graph from one of its nodes.

  @details The Build_Subgraph classdetails walking in depth the graph ga
  g_src from a source node and builds on a mapped back
  entire graph (or subgraph if g is disconnected) seen in the
  I traveled.

  see inconnected_components () copy_graph ()

  ingroup Graphs
*/
      template <class GT, class SA = Dft_Show_Arc<GT> > 
class Build_Subgraph
{
  SA & sa;
  const GT * gptr;
  size_t count;

public:

  Build_Subgraph(SA && __sa = SA()) : sa(__sa) { /* Empty */ }

  Build_Subgraph(SA & __sa) : sa(__sa) { /* Empty */ }

private:

  void build_subgraph(GT & sg, typename GT::Node * g_src)
  {
    if (IS_NODE_VISITED(g_src, Build_Subtree)) 
      return;

    NODE_BITS(g_src).set_bit(Build_Subtree, true); /* g_src visited */
    ++count; 

    typename GT::Node * sg_src = mapped_node <GT> (g_src);
    
    /** Is g_src maped?
    Yes? ==> g_src created in the image and map subgraph sg */
    if (sg_src == NULL) 
      { 
	sg_src = sg.insert_node(g_src->get_info());
	GT::map_nodes(g_src, sg_src);
      }

    for (Node_Arc_Iterator<GT, SA> i(g_src, sa); /* Explore from g_src */
	 count < gptr->get_num_nodes() and i.has_current(); i.next())
      {
        typename GT::Arc * arc = i.get_current_arc();
        if (IS_ARC_VISITED(arc, Build_Subtree)) 
          continue; /* Next advance arc */

        ARC_BITS(arc).set_bit(Build_Subtree, true); /* arc visited */

        typename GT::Node * g_tgt  = i.get_tgt_node(); /* Target of arc */
        typename GT::Node * sg_tgt = mapped_node <GT> (g_tgt);

    /** Is maped onsg?
    NO? ==> Map and put in the subgraph sg */
        if (sg_tgt == NULL) 
          {
            sg_tgt = sg.insert_node(g_tgt->get_info()); 
            GT::map_nodes(g_tgt, sg_tgt); 
          }

    /* We subgraph nodes, insert arch and mapped */
        typename GT::Arc * sg_arc = 
	  sg.insert_arc(sg_src, sg_tgt, arc->get_info());

        GT::map_arcs(arc, sg_arc); 

        build_subgraph(sg, g_tgt);
      }
  }

      template <template <class> class List>
  void build_subgraph(List<typename GT::Node*> & l, typename GT::Node * p)
  {
    if (IS_NODE_VISITED(p, Build_Subtree)) 
      return;

    NODE_BITS(p).set_bit(Build_Subtree, true); /* g_src visited */
    ++count; 
    l.append(p);

    for (Node_Arc_Iterator<GT, SA> it(p, sa); /* Explore from g_src */
	 count < gptr->get_num_nodes() and it.has_curr(); it.next())
      {
        typename GT::Arc * arc = it.get_current_arc();
        if (IS_ARC_VISITED(arc, Build_Subtree)) 
          continue; /* Next advance arc */

        ARC_BITS(arc).set_bit(Build_Subtree, true); /* arc visited */

        build_subgraph(l, it.get_tgt_node());
      }
  }
    
  public:

/** 
  @brief Summons building a subgraph mapped.
  
  @details The method uses the bit build_subtree to mark the nodes and
  arches already visited.

  build_subgraph() it is used by the method
  inconnected_components() to map the various blocks.

  @param[in] g Graph to map
  @param[out] sg Graph empty place where the mapped copy from g_src.
  @param[in] g_src Origin node from which originates the travel and 
  mapping.
  
  @throw bad_alloc If no memory to build sg.
  @throw domain_error If sg is not an empty graph.
*/
  void operator () (const GT & g, GT & sg, typename GT::Node * g_src) 
  {
    gptr = &g;
    count = 0;
    build_subgraph (sg, g_src);
  }

/** 
  @brief Build a list of nodes reachable from src.

  @details The method uses the bit build_subtree to mark the nodes and
  arches already visited.

  build_subgraph() it is used by the method
  inconnected_components() to map the various blocks.

  @param[in] g Graph to map
  @param[out] l Empty list where to place the nodes from src.
  @param[in] src Origin node from which the route originates 
  the travel and mapping.
  
  @throw bad_alloc If no memory to build sg.
  @throw domain_error If sg is not an empty graph.
*/
  void operator () (const GT & g, DynList<typename GT::Node*> & list, 
		    typename GT::Node * src) 
  {
    gptr = &g;
    count = 0;
    build_subgraph<DynList>(list, src);
  }
}; 


/** 
  @brief Calculate the disjointed components of a graph.

  @details La clase Inconnected_Components takes a graph g,
  "supposedly unconnected" calculates its subgraphs or components
  disconnected and stored in a dynamic list of subgraphs
  mapped both nodes and arcs. The emphasis on
  Of course you connected graph character is that if the graph does not
  it was connected, then the resulting list would contain a single
  copy the graph corresponding element mapping g. If this
  were the case, then it is best done with the function
  copy_graph ().

  The class takes two type parameters:
  -# GT: Graph type, which must be derived from List_Graph.
  -# SA: Class is responsible for displaying the arc. Internally,
  the function uses iterator filter Node_Arc_Iterator (based on
  Filter_Iterator) to traverse the arcs of each node. SA is the 
  class that determines whether or not to display bow to
  traveled.

  @see copy_graph()

  @ingroup Grafos
*/
      template <class GT, class SA = Dft_Show_Arc<GT> > 
class Inconnected_Components
{
  SA & sa;

public:

  Inconnected_Components(SA && __sa = SA()) : sa(__sa) { /* Empty */ }

  Inconnected_Components(SA & __sa) : sa(__sa) { /* Empty */ }

      template <template <class> class List>
  void compute_blocks(const GT & g, List <GT> & list)
  {
    g.reset_nodes(); 
    g.reset_arcs();  
    size_t count = 0; /* Counter of visited nodes */
    for (typename GT::Node_Iterator it(g); /* Walk nodes of g */
         count < g.get_num_nodes() and it.has_curr(); it.next())
      {
        typename GT::Node * curr = it.get_current_node();
        if (IS_NODE_VISITED(curr, Build_Subtree)) 
          continue;

        GT & subgraph = list.append(GT()); /* Graph inserted on list */

        Build_Subgraph <GT, SA> build(sa);
	build(g, subgraph, curr); 
	
	count += subgraph.get_num_nodes();
      }
  }

      template <template <class> class List>
  void compute_lists(const GT & g, List<List<typename GT::Node*> > & list)
  {
    g.reset_nodes(); 
    g.reset_arcs();  
    size_t count = 0; /* Counter of visited nodes */
    for (typename GT::Node_Iterator i(g); /* Walk nodes of g */
         count < g.get_num_nodes() and i.has_curr(); i.next())
      {
        typename GT::Node * curr = i.get_current_node();
        if (IS_NODE_VISITED(curr, Build_Subtree)) 
          continue;

	 /* Create disjointed component connected subgraph curr_node */
        List<typename GT::Node*> & l = 
	  list.append(List<typename GT::Node*>()); 

        Build_Subgraph <GT, SA> build(sa);
	build(g, l, curr); 
	
	count += l.size();
      }
  }
    
/** 
  @brief Invokes the determination of unrelated components.

  @details The function uses the build_subtree bit to mark nodes and
  arches visited.

  The algorithm internally finds the components by depth searches.
    
  @param[in] g Graph on which you want to calculate their blocks.
  @param[out] list List mapped to subgraphs with g
  subgraphs g or disconnected components.
  
  @throw bad_alloc If no memory to build some block or 
  insert are listed.
*/
  void operator () (const GT & g, DynList <GT> & list)
  {
    compute_blocks<DynList>(g, list);
  }

/** 
  @brief Calculate the disconnected components by lists of lists of nodes.

  @details The function uses the build_subtree bit to mark nodes and
  arches visited.

  The algorithm internally finds the components by depth searches.
      
  @param[in] g Graph on which you want to calculate their blocks.
  @param[out] list List of lists of nodes; each list contains the
  nodes that are part of a connected component.
  
  @throw bad_alloc If no memory to build some block or 
  insert are listed.
*/
  void operator () (const GT & g, DynList<DynList<typename GT::Node*>> & list)
  {
     compute_lists<DynList>(g, list);
  }
};


} /* End namespace Aleph */

# endif //  TPL_COMPONENTS_H
