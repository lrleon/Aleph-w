
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
  @file tpl_spanning_tree.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_SPANNING_TREE_H
# define TPL_SPANNING_TREE_H

namespace Aleph {


/** 
  @brief Computes a spanning tree in depth of a graph.

  @details This class takes a "g" graph , takes a look in depth
  from a selected node and builds the spanning tree
  in the order given by the tour visit.

  The class takes two type parameters:
  -# GT: Graph type, which must be derived from List_Graph.
  -# SA: Class is responsible for displaying the arc. Internally,
  the function uses iterator filter Node_Arc_Iterator (based on
  Filter_Iterator) to traverse the arcs of each node.
  SA is the class that determines whether or not to display bow to
  traveled.

  @ingroup Grafos

  @see find_breadth_first_spanning_tree() Tree_Node
  @see graph_to_tree_node() 
*/
    template <class GT, class SA = Dft_Show_Arc<GT> > 
class Find_Depth_First_Spanning_Tree
{
  SA & sa;
  GT * gptr;
  GT * tptr;

  bool build_tree(typename GT::Node * gnode, typename GT::Arc * garc, 
		  typename GT::Node * tnode)
  {
    NODE_BITS(gnode).set_bit(Spanning_Tree, true); /* Mark node */
    ARC_BITS(garc).set_bit(Spanning_Tree, true);   /* Mark arc */

    typename GT::Node * tree_tgt_node = tptr->insert_node(gnode->get_info());
    GT::map_nodes(gnode, tree_tgt_node);

    typename GT::Arc * tarc = 
      tptr->insert_arc(tnode, tree_tgt_node, garc->get_info()); 
    GT::map_arcs(garc, tarc);

    tnode = tree_tgt_node; 

    /* Graph spanning? */ 
    if (tptr->get_num_nodes() == gptr->get_num_nodes()) 
      return true; /* Tree already contains the spanning tree */

    assert(tptr->get_num_nodes() > tptr->get_num_arcs()); /* It must be acyclic */

    for (Node_Arc_Iterator<GT, SA> i(gnode, sa); 
	 i.has_curr() and tptr->get_num_nodes() < gptr->get_num_nodes();
	 i.next())
      {
        typename GT::Arc * arc = i.get_current_arc();
        if (IS_ARC_VISITED(arc, Spanning_Tree)) 
          continue;

        typename GT::Node * arc_tgt_node = i.get_tgt_node();
        if (IS_NODE_VISITED(arc_tgt_node, Spanning_Tree))
          continue; /* Destination and visited from another arc */

        if (build_tree(arc_tgt_node, arc, tnode))
	  return false;
      }

    return false;
  }

  bool build_tree(GT & g, typename GT::Node * gnode, GT & tree)
  {
    gptr = &g;
    tptr = &tree;

    gptr->reset_nodes();
    gptr->reset_arcs(); 

    clear_graph(*tptr); /* Ensure that destination tree is empty */
  
    NODE_BITS(gnode).set_bit(Spanning_Tree, true); /* Mark gnode */
  
    typename GT::Node * tnode = tree.insert_node(gnode->get_info()); 
    GT::map_nodes(gnode, tnode);
  
    for (Node_Arc_Iterator<GT, SA> i(gnode, sa); 
	 i.has_curr() and tptr->get_num_nodes() < gptr->get_num_nodes(); 
	 i.next())
      {
	typename GT::Arc * arc = i.get_current_arc();
	if (IS_ARC_VISITED(arc, Spanning_Tree)) 
	  continue;

	typename GT::Node * arc_tgt_node = i.get_tgt_node();
	if (IS_NODE_VISITED(arc_tgt_node, Spanning_Tree))
	  continue; /* Destination and visited from another arc */

	if (build_tree(arc_tgt_node, arc, tnode))
	  return false;
      }

    return true;
  }

public:

  Find_Depth_First_Spanning_Tree(SA && __sa = SA()) : sa(__sa) { /* Empty */ }

  Find_Depth_First_Spanning_Tree(SA & __sa) : sa(__sa) { /* Empty */ }

/** 
  @brief Invake the building spanning tree in depth.

  @details After the operation the "tree" parameter contains the tree
  encompassing concerned with their arches and nodes mapped to the "g" graph.

  During the execution, the algorithm marks the nodes and
  arches visited with the spanning_tree bit.

  The order of graph visitation is not necessarily the same as the
  primitive depth_first_traversal().

  @param[in] g Graph on which you want to build the tree 
  comprehensive in depth.
  @param[out] tree Graph where to be placed the spanning tree 
  in depth.

  @return If the graph is connected and there is a comprehensive tree,
  returns a pointer to the node where the search starts at
  depth; otherwise NULL is returned.

  @throw bad_alloc If no memory to build the spanning tree.
*/
  typename GT::Node * operator () (GT & g, GT & tree)
  {
    typename GT::Node * start = g.get_first_node();
    if (not build_tree(g, start, tree)) 
      return NULL;

    return start;
  }

/** 
  @brief Invake the building spanning tree in depth
  from a given node.

	@details After the operation the "tree" parameter contains the tree
  encompassing concerned with their arches and nodes mapped to the "g" graph.

	During the execution, the algorithm marks the nodes and
  arches visited with the spanning_tree bit.
	
	The order o graph visitation is not necessarily the same as
  the primitive depth_first_traversal().

	@param[in] g Graph on which you want to build the tree 
  comprehensive in depth.
	@param[in] gnode Nodo desde el cual se desea iniciar la
	búsqueda en profundidad.
	@param[out] tree Graph where to be placed the spanning tree 
  in depth.

	@return A pointer to the node where the search starts at
  depth; otherwise NULL is returned.
	
  @throw bad_alloc If no memory to build the tree
  encompassing.
*/
  typename GT::Node * operator () (GT & g, typename GT::Node * gnode, GT & tree)
  {
    this->build_tree(g, gnode, tree);
    return (typename GT::Node *) NODE_COOKIE(gnode);
  }
};



/** 
  @brief Computes a spanning tree of a graph in depth from a node.

  @details This class takes a "g" graph , takes a look in depth
  from a selected node and builds the spanning tree
  in the order given by the tour visit.

  The class takes two type parameters:
  -# GT: Graph type, which must be derived from List_Graph.
  -# SA: Class is responsible for displaying the arc. Internally,
  the function uses iterator filter Node_Arc_Iterator (based on
  Filter_Iterator) to traverse the arcs of each node.
  SA is the class that determines whether or not to display bow to
  traveled.

  @throw bad_alloc If no memory to build the spanning tree
  or for internal queue used for travel in amplitude.


  @ingroup Grafos

  @see find_depth_first_spanning_tree() Tree_Node
  @see graph_to_tree_node() 
*/
      template <class GT, class SA = Dft_Show_Arc<GT> > 
class Find_Breadth_First_Spanning_Tree
{
  SA & sa;

  void build_tree(GT & g, typename GT::Node * gp, GT & tree)
  {
    g.reset_bit_nodes(Spanning_Tree);
    g.reset_bit_arcs(Spanning_Tree);

    clear_graph(tree); 

    unique_ptr<typename GT::Node> tp_auto(new typename GT::Node(gp));
    tree.insert_node(tp_auto.get());
    GT::map_nodes(gp, tp_auto.release());

    DynListQueue<typename GT::Arc*> q; /* Insert into queue the gp arcs */
    for (Node_Arc_Iterator<GT, SA> i(gp, sa); i.has_curr(); i.next())
      q.put(i.get_current_arc());

    NODE_BITS(gp).set_bit(Spanning_Tree, true); 

    while (not q.is_empty()) 
      {
	typename GT::Arc * garc = q.get(); 
	ARC_BITS(garc).set_bit(Spanning_Tree, true); 
	typename GT::Node * gsrc = g.get_src_node(garc);  
	typename GT::Node * gtgt = g.get_tgt_node(garc);

	if (IS_NODE_VISITED(gsrc, Spanning_Tree) and 
	    IS_NODE_VISITED(gtgt, Spanning_Tree))
	  continue; /* Los dos nodos de garc ya fueron visitados */


    /** "gtgt" visited?
    If the answer is YES, then exchange it with "gsrc" */
	if (IS_NODE_VISITED(gtgt, Spanning_Tree)) 
	  std::swap(gsrc, gtgt); 

	typename GT::Node * tsrc = mapped_node<GT>(gsrc);
	NODE_BITS(gtgt).set_bit(Spanning_Tree, true); /* visited gtgt */

    /* Crear copia de gtgt, insertarlo en tree y mapearlo */
	unique_ptr<typename GT::Node> ttgt_auto(new typename GT::Node(gtgt));
	tree.insert_node(ttgt_auto.get());
	typename GT::Node * ttgt = ttgt_auto.release();
	GT::map_nodes(gtgt, ttgt);

    /* Insertar nuevo arco en tree y mapearlo */
	typename GT::Arc * tarc = tree.insert_arc(tsrc, ttgt, garc->get_info());
	GT::map_arcs(garc, tarc);
	if (tree.get_num_nodes() == g.get_num_nodes()) /* ¿Abarca a g? */
	  break; 

    /** Insert in queued arches of "gtgt" */
	for (Node_Arc_Iterator<GT, SA> i(gtgt, sa); i.has_curr(); i.next())
	  {
	    typename GT::Arc * current_arc = i.get_current_arc();
	    if (IS_ARC_VISITED(current_arc, Spanning_Tree)) 
	      continue;

    /* Revise nodos de arcos para ver si han sido visitados */
	    if (IS_NODE_VISITED(g.get_src_node(current_arc), Spanning_Tree) and 
		IS_NODE_VISITED(g.get_tgt_node(current_arc), Spanning_Tree))
	      continue; /* Nodos ya visitados ==> no meter arco */
	    q.put(current_arc);
	  }
      }
  }

public:

  Find_Breadth_First_Spanning_Tree(SA && __sa = SA()) : sa(__sa) { /* Empty */ }

  Find_Breadth_First_Spanning_Tree(SA & __sa) : sa(__sa) { /* Empty */ }

/** 
  @brief Invokes the construction of a spanning tree the amplitude.

  @details After the operation the "tree" parameter contains the tree
  encompassing concerned with their arches and nodes mapped to the "g" graph. 

  During the execution, the algorithm marks the nodes and
  arches visited with the spanning_tree bit.

  The graph visitation order is not necessarily the same as
  the primitive breadth_first_traversal(). 

  @param[in] g Graph on which you want to build the tree 
  comprehensive in scope.
  @param[in] gnode Pointer to the source node of the search 
  amplitude.
  @param[out] tree The graph where is placed the spanning tree of 
  amplitude.
*/
    void operator () (GT & g, typename GT::Node * gnode, GT & tree) 
    {
      find_breadth_first_spanning_tree <GT, SA> (g, gnode, tree);
    }
  };


/** 
  @brief Builds a spanning tree of a graph represented with arrays.

  @details This class takes a "g" graph and spanning tree
  represented by the nodes arrays "pred" and arches
  "arcs" and built in "tree" the spanning tree.

  @ingroup Grafos
*/
      template <class GT>
class Build_Spanning_Tree
{
public:

/** 
  @brief Invokes the construction of spanning tree by arrangements.

  @param[in] g Graph on which you want to build the tree
  comprehensive alternative.
  @param[out] tree Spanning tree alternative obtained from the 
  pred and arcs arrays.
  @param[in] pred Array of pointers to nodes.
  @param[in] arcs Array of pointers to arcs.
  @param[in] with_map Indicates whether the spanning tree should 
  be mapped with the g graph.
  
  @throw bad_alloc If there is insufficient memory for build the
  spanning tree.
*/
  void operator () (GT & g, GT & tree, 
		    DynArray<typename GT::Node> * pred,
		    DynArray<typename GT::Arc> *  arcs,
		    const bool with_map = false) const
  {
    build_spanning_tree(g, tree, pred, arcs, with_map);
  }
};


} /* End namespace Aleph */

# endif /*  TPL_SPANNING_TREE_H
