
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
  @file tpl_cut_nodes.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_CUT_NODES_H
# define TPL_CUT_NODES_H

# include <tpl_graph_utils.H>

namespace Aleph {


/** 
  @brief Calculatingbrief breakpoints of a graph.

  @details The Compute_Cut_Nodes class is responsible for 
  calculating delivering everything
  nodes related to the cutting of a connected graph. For that
  takes a look in depth from a given node
  added to a dynamic list each node that contains the cutting
  graph.

  The dynamic list is provided by the user.

  Cutting a node is defined as a node such that the delete
  (Together with its arches) the graph becomes disjointed.

  The algorithm uses the bit Depth_First to mark the nodes and
  arches that have been visited. Also, to build copies
  related components mapped around a cutoff,
  Build_Subtree bit of uses.

  This class also provides methods to "paint" the various
  blocks that would separate cutoffs and for
  mapped obtain copies of related components around
  breakpoints, the graph cut and cross bows.

  It is assumed that G is connected and no check is made to
  respect.

  The class type parameters are:
  - GT: Graph.
  - SA: Filter arches arches for iterators. 

  @note If you required calculate the graph cut by
  map_cut_graph () method, then the filter class bows
  you must overload the operator () (GT & GT :: Arc *).

  Esta clase exporta varias operaciones con dependencia secuencial:
  -# cut_nodes(): Which calculates cut nodes and inserted into a list. 
  The operator () on the class is synonymous with this method.
  -# paint_subgraphs(): "Paint" the various components
  graph around the cutoff points previously
  calculated. The routine returns the number of colors, one for
  each component having the graph. The colors are placed
  in counters associated with each node.
  -# map_subgraph(): Gets a copy of a component mapped
  related associated with a given color.
  -# map_cut_graph(); Gets a mapped copy of "graph
  cut ", that is, that only the nodes formed by cutting
  graph. This method also calculates the arches crossing (the
  connecting a cut with an associated block).

  If you want to run everything at once, it provides the
  compute_blocks () method, which calculates the cut points,
  painted components and calculates the mapped copies
  related components and cross bows.

  @ingroup Grafos
*/
      template <class GT, class SA = Dft_Show_Arc<GT> >
class Compute_Cut_Nodes
{
  SA &                            sa;
  GT *                            gptr;
  DynDlist<typename GT::Node *> * list_ptr;
  long                            curr_df;
  long                            curr_color;

  enum State { Init, Cut_Nodes_Computed, Painted } state;

  void cut_nodes(typename GT::Node * p, typename GT::Arc * a)
  {
    NODE_BITS(p).set_bit(Depth_First, true); /* Paint p visited */
    low <GT> (p) = df <GT> (p) = curr_df++;  /* Assign df */

    /* Browse arches p while not encompassing g */
    bool p_is_cut_node = false;
    for (Node_Arc_Iterator <GT, SA> i(p, sa); i.has_curr(); i.next())
      {
        typename GT::Arc * arc = i.get_current_arc();
        if (arc == a) 
          continue; /* a father of arc ==> ignore it */

        typename GT::Node * tgt = i.get_tgt_node();
        if (IS_NODE_VISITED(tgt, Depth_First)) 
          { 

            if (not IS_ARC_VISITED(arc, Depth_First)) /* Is not comprehensive?*/
              if (df<GT>(tgt) < low<GT>(p)) /* sí, verify value of low */
                low<GT>(p) = df<GT>(tgt); /* Update low(p) */

            continue;
          }

        if (IS_ARC_VISITED(arc, Depth_First)) 
          continue;

        ARC_BITS(arc).set_bit(Depth_First, true); /* Mark arc */

        cut_nodes(tgt, arc);

        if (low<GT>(tgt) < low<GT>(p)) 
          low<GT>(p) = low<GT>(tgt); /* Update low(p) */

        if (low<GT>(tgt) >= df<GT>(p) and df<GT>(tgt) != 0) /* What court? */
          p_is_cut_node = true;
      }

        /* Here, p was explored recursively */
    if (p_is_cut_node)
      {
        NODE_BITS(p).set_bit(Cut, true);
        list_ptr->append(p);
      }
  }

public:

/** 
  @brief Calculate the cut points.
	
  @param[in] start Start node where you want to start calculation.
  @param[out] list Dynamic list where pointers are stored at nodes
  cutting the graph.
  
  @throw bad_alloc If no memory to insert into the dynamic list
  or if no memory for the data of internal calculation algorithm.

  The list must be provided by user and empty at the start
  calculation.
*/
  void cut_nodes(typename GT::Node *             start, 
		 DynDlist<typename GT::Node *> & list)
  {
    curr_df  = 0; /* Counter Global of Visits  */
    list_ptr = &list;

    list_ptr->empty();

    Operate_On_Nodes <GT, Init_Low <GT> > () (*gptr);

    gptr->reset_arcs();

    NODE_BITS(start).set_bit(Depth_First, true); /* Mark start */
    df <GT> (start) = curr_df++;

    int call_counter = 0; /* Count recursive calls */

    /** Scroll through the arches start while g has not been covered */
    for (Node_Arc_Iterator<GT, SA> it(start, sa); 
         it.has_curr() and curr_df < gptr->get_num_nodes(); it.next())
      {
	typename GT::Node * tgt = it.get_tgt_node();
	if (IS_NODE_VISITED(tgt, Depth_First)) 
	  continue; 

	typename GT::Arc * arc = it.get_current_arc();
	if (IS_ARC_VISITED(arc, Depth_First)) 
	  continue;

	ARC_BITS(arc).set_bit(Depth_First, true);
	cut_nodes(tgt, arc); 
	++call_counter;
      }  

    /** Following Is a hinge point?
    Yes? ==> Put in the list */
    if (call_counter > 1) 
      {    
        NODE_BITS(start).set_bit(Cut, true);
        list_ptr->append(start);
      }

    state = Cut_Nodes_Computed;
  }

private:

  void paint_subgraph(typename GT::Node * p)
  {
    assert(not is_a_cut_node <GT> (p));

    if (is_node_painted <GT> (p)) 
      return; 

    paint_node <GT> (p, curr_color);

    for (Node_Arc_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next())
      {
	typename GT::Arc * arc = it.get_current_arc();
	if (is_arc_painted <GT> (arc))
          continue;

	typename GT::Node * tgt = it.get_tgt_node();
	if (is_a_cut_node <GT> (tgt))
	  continue;

	paint_arc<GT>(arc, curr_color); 

	paint_subgraph(tgt); 
      }
  }

  void paint_from_cut_node(typename GT::Node * p)
  {
    assert(is_a_cut_node <GT> (p));

    /* Recursively paint colors with diff blocks connected to p */
    for (Node_Arc_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next())
      {
	typename GT::Arc * arc = it.get_current_arc();

	assert(not is_arc_painted <GT> (arc));

	typename GT::Node * tgt_node = it.get_tgt_node();
	if (is_a_cut_node <GT> (tgt_node)) /* Is it a cutting arc? */
	  {
	    ARC_BITS(arc).set_bit(Cut, true); /* Mark as cutting */
	    continue; /* Advance to next arc */
	  }
	else 
	  {
	    paint_arc <GT> (arc, Cross_Arc); /* Marque como de cruce */
	    if (is_node_painted <GT> (tgt_node)) 
	      continue; 
	  }

    /* Paint recursively node connected to arc */
	paint_subgraph(tgt_node);

	curr_color++; /* Change Color (sig bow in another block) */

	assert(not is_arc_painted <GT> (arc));
      }
  }

  typename GT::Node * create_and_map_node(typename GT::Node * gp, 
					  const long &        color,
					  GT &                sg)
  {
    assert(get_color<GT>(gp) == color);
    assert(not IS_NODE_VISITED(gp, Build_Subtree));

    unique_ptr<typename GT::Node> tp_auto(new typename GT::Node(gp));
    sg.insert_node(tp_auto.get());
    GT::map_nodes(gp, tp_auto.get());
    NODE_BITS(gp).set_bit(Build_Subtree, true); 

    return tp_auto.release();
  }

  void map_subgraph(GT & sg, typename GT::Node * gsrc, const long & color)
  {
    assert(get_color <GT> (gsrc) == color);

    typename GT::Node * tsrc = mapped_node<GT>(gsrc); /* gsrc en sg */

    /* Browse gsrc arches and add to the color sg interest */
    for (Node_Arc_Iterator <GT, SA> i(gsrc, sa); i.has_curr(); i.next())
      {
        typename GT::Arc * garc = i.get_current_arc();
        if (get_color<GT>(garc) != color or IS_ARC_VISITED(garc, Build_Subtree))
          continue; /* Arco is a different color or already visited */

        ARC_BITS(garc).set_bit(Build_Subtree, true); 

        typename GT::Node * gtgt = i.get_tgt_node(); 

        assert(get_color <GT> (gtgt) == color);

        typename GT::Node * ttgt = NULL; /* Image gtgt in sg */
        if (IS_NODE_VISITED(gtgt, Build_Subtree)) /* gtgt in sg? */
          ttgt = mapped_node<GT> (gtgt);
        else
	  ttgt = create_and_map_node(gtgt, color, sg);

        typename GT::Arc * tarc = sg.insert_arc(tsrc, ttgt, garc->get_info());
        GT::map_arcs(garc, tarc);

        map_subgraph(sg, gtgt, color);
      }
  }
  
public:

/** 
  @brief Constructor de calculador de nodos de corte.

  @param[in] g Graph to which you want to calculate the points cut.
  @param[in] __sa Arches filter for oiteradores on arches.
*/
  Compute_Cut_Nodes(GT & g, SA && __sa = SA()) 
    : sa(__sa), gptr(&g), state(Init)
  {
    /* Empty */ 
  }

  Compute_Cut_Nodes(GT & g, SA & __sa) 
    : sa(__sa), gptr(&g)
  {
    /* Empty */ 
  }

/** 
  @overload cut_nodes() 
*/
  void operator () (DynDlist<typename GT::Node *> & list)
  {
    cut_nodes(gptr->get_first_node(), list);
  }

/** 
  @overload cut_nodes() 
*/
  void operator () (typename GT::Node *             start,
		    DynDlist<typename GT::Node *> & list)
  {
    cut_nodes(start, list);
  }
  
/** 
  @brief Pinta related components around the cutoffs.

  @details paint_subgraphs () paints related components around
  cutoff points of a graph.

  The routine requires that before was referred to cut_nodes () method.

  "Colors" of each component are placed in attribute
  NODE_COUNTER of each node and arc. The first color starts at
  value 1.

  Wear this routine if it is not necessary or sufficient to
  obtain components using colors
  blocks. This saves the calculation and memory of having to create
  new graphs.

  @return The amount of components; that is the number of colors.

  @throw std :: logic_error If the calculation of the cutoffs
  It has not previously been invoked.
*/
  long paint_subgraphs()
  {
    if (state != Cut_Nodes_Computed)
      throw std::logic_error("Cut nodos has not been computed or the class is"
			     " another phase");

    gptr->reset_counter_nodes();
    gptr->reset_counter_arcs();
    curr_color = 1;

    /** Recorrer cada nodo de corte y pintar sus bloques */
    for (typename DynDlist<typename GT::Node*>::Iterator i(*list_ptr); 
	 i.has_curr(); i.next())
      paint_from_cut_node(i.get_curr());

    state = Painted;

    return curr_color;
  }

/** 
  @brief Gets a copy of the component mapped color.

  @details map_subgraph () draws the graph component with color color
  and builds a graph mapping in sg.

  @param [out] sg For Graph where the mapped copy.
  @param [in] color Color to be extracted.

  @throw logic_error If the graph is not painted.
  @throw domain_error If no color component
  given.
*/
  void map_subgraph(GT & sg, const long & color)
  {
    if (state != Painted)
      throw std::logic_error("Graph is not painted");

    clear_graph(sg);

    typename GT::Node * first = NULL; /* Busque primer nodo con color */

    for (typename GT::Node_Iterator it(*gptr); it.has_current(); it.next())
      if (get_color <GT> (it.get_current_node()) == color)
        first = it.get_current_node();

    if (first == NULL) /* Found the color? */
      throw std::domain_error("Color does not exist in the graph");

        /* Create first, insert sg and mapo */
    create_and_map_node(first, color, sg);
    try
      {    
	map_subgraph (sg, first, color); /* Mapee first */
      }
    catch (...)
      {
        clear_graph(sg);
      }
  }

/** 
  @brief Calculatebrief cutting graph mapping of a graph.

  @details The graph cuttingdetails cut of a graph is one single compound
  by cutting nodes.

  Cross bows are also calculated; that is, those who
  cutoffs connect with related components.

  The idea of this algorithm is used in combination with
  map_subgraph () may be obtained from the mapped copies
  original graph.

  @param [out] Graph cut cut_graph
  @param [out] cross_arc_list List arches crossing; It is
  ie arcs going from one node to a cutting
  conoxo component. These arches outside the graph cut.

  @throw logic_error If the graph has not previously painted.
*/
  void map_cut_graph(GT &                          cut_graph, 
		     DynDlist<typename GT::Arc*> & cross_arc_list)
  {
    if (state != Painted)
      throw std::logic_error("Graph is not painted");

    clear_graph(cut_graph);

    /* Scroll cutting node list and insert into cut_graph */
    for (typename DynDlist<typename GT::Node*>::Iterator it(*list_ptr);
	 it.has_curr(); it.next())
      {
	typename GT::Node * gp = it.get_current();

	assert(is_a_cut_node <GT> (gp));

	unique_ptr<typename GT::Node> tp_auto(new typename GT::Node(gp));
	cut_graph.insert_node(tp_auto.get());
	GT::map_nodes(gp, tp_auto.release());
      }

    /* Recorra arcos de g ==> cut_graph 
    = {arcos no corte} U cross_arc_list 
    = {arcos de cruce} */
    for (Arc_Iterator <GT, SA> it(*gptr, sa); it.has_curr(); it.next())
      {
	typename GT::Arc * garc = it.get_current_arc();
	if (is_a_cross_arc <GT> (garc))
	  {
	    cross_arc_list.append(garc); 
	    continue;
	  }

	if (not is_an_cut_arc <GT> (garc)) 
	  continue;

	typename GT::Node * src = mapped_node<GT>(gptr->get_src_node(garc));
	typename GT::Node * tgt = mapped_node<GT>(gptr->get_tgt_node(garc));

	assert(src != NULL and tgt != NULL);

	typename GT::Arc * arc = 
	  cut_graph.insert_arc(src, tgt, garc->get_info());
	GT::map_arcs(garc, arc);
      }
  }

/** 
  @brief Build graphsbrief mapped around the cutoff points of a graph.

  @details This routine takes a list of breakpoints previously
  calculated and constructed in one pass on the nodes
  mapped copies of related components around
  cutoffs and cutting the graph. Then it is performed
  a pass over the arches in which the arches are determined
  crossing.

  If required calculate all mapped graphs and graph
  cut, then the use of this routine is much more
  efficient than separate calls to map_subgraph () and map_cut_graph ().

  @param [out] block_list List of graphs where you saved the
  mapped copies of related components
  @param [out] cut_graph Graph cut mapping
  @param [out] list arches crossing.

  @throw bad_alloc If there is Insufficient Memory.
  @throw logic_error If nodes have not been cut
  previously calculated.
*/
  void compute_blocks(DynDlist<GT> &                block_list, 
		      GT &                          cut_graph, 
		      DynDlist<typename GT::Arc*> & cross_arc_list)
  {
    if (state < Cut_Nodes_Computed)
      throw std::logic_error("Cut nodes have not been computed");

    if (state == Cut_Nodes_Computed)
      paint_subgraphs();

    const long & num_colors = curr_color;
    /* Blocks in an arrangement for quick access
    that is empty it indicates whether or not it has been processed or not. */
    DynArray<GT*> blocks;  
    blocks.reserve(num_colors);

    /* Create list of empty color computer components i */
    for (int i = 0; i < num_colors; ++i)
      blocks.access(i) = &block_list.append(GT());

    /* Browse and copy nodes and mapping by color */
    for (typename GT::Node_Iterator it(*gptr); it.has_curr(); it.next())
      {
	typename GT::Node * p = it.get_curr();
	if (IS_NODE_VISITED(p, Build_Subtree))
	  continue;

	if (is_a_cut_node <GT> (p))
	  continue;

	const long color = get_color<GT>(p);

	GT & sg = *blocks.access(color - 1);

	create_and_map_node(p, color, sg);

	map_subgraph(sg, p, color);	
      }

    map_cut_graph(cut_graph, cross_arc_list);
  }
};

} /* End namespace Aleph */

# endif // TPL_CUT_NODES_H
