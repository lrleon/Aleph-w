
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
  @file tpl_graph_utils.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_GRAPH_UTILS_H
# define TPL_GRAPH_UTILS_H


# include <limits>
# include <tpl_agraph.H> 
# include <tpl_dynListQueue.H>

using namespace Aleph;

namespace Aleph {

    template <class GT, class SA> inline static bool 
__depth_first_traversal(GT & g, typename GT::Node * node,
			typename GT::Arc *  arc,
			bool (*visit)(GT & g, typename GT::Node *, 
				      typename GT::Arc *),
			size_t & count);

/** 
  @brief Generic depth-first traversal of a graph.

  @details This feature runs in the graph depth "g" from the start_node 
  node. For each node visited, it invokes a function whose
  prototype should be:

  bool visitar(GT & g, typename GT::Node * p, typename GT::Arc * a)
    
  Whose parameters are described below:
  -# g: Graph
  -# p: Current node you are visiting in depth
  -# a: Archway leading to the current p node
  .

  The function takes two type parameters:
  -# GT: Graph type, which must be derived from List_Graph.
  -# SA: Class is responsible for displaying the arc. Internally,
  the function uses iterator filter Node_Arc_Iterator (based on
  Filter_Iterator) to traverse the arcs of each node.
  SA is the class that determines whether or not to display bow to
  traveled. Through this class, the in-depth tour
  can specialize to process the arches according to some criterion;
  arcs of a residual waste flow network, for example
  .

  If the function returns true, then the tour stops; ie, 
  not the path on the graph continues. If the function
  returns false, then the tour continues in amplitude
  and would stop once all the arcs of the graph have been seen.

  The route uses the depth_first bit, which is initiated to
  first algorithm.

  A version without the parameter SA type specializes show all
  arches.

  @param[in] g Graph to explore.
  @param[in] start_node Node where the tour starts.
  @param[in] visit Function of visit.
  @param[in] sa Filter of arches.

  @return The number of nodes that were visited
    
  @ingroup Grafos

  @see breadth_first_traversal() test_connectivity() Filter_Iterator
*/
    template <class GT, class SA = Dft_Show_Arc<GT>> inline size_t 
depth_first_traversal(GT & g, typename GT::Node * start_node,
		      bool (*visit)(GT & g, typename GT::Node *, 
				    typename GT::Arc *),
		      SA sa = SA())
{
  g.reset_bit_nodes(Depth_First); /* Restart Depth_First nodes */
  g.reset_bit_arcs(Depth_First);  /* Restart Depth_First arches */
  size_t counter = 0; /* Initially it has not visited any node */

  __depth_first_traversal<GT,SA>(g, start_node, NULL, visit, counter, sa);

  return counter;
}

/** 
  @brief Generic depth-first traversal of a graph.

  @details This feature runs in the graph depth "g" from any node.

  The start node is undetermined and should not be done
  assumptions about what will be this node.

  For each node visited, it invokes a function whose
  prototype should be:

  bool visitar(GT & g, typename GT::Node * p, typename GT::Arc * a)

  Whose parameters are described below:
  -# g: Graph.
  -# p: Current node you are visiting in depth.
  -# a: Archway leading to the current node p.

  If the function returns true, then the tour stops; ie, 
  not the path on the graph continues. If the function
  returns false, then the tour continues in amplitude
  and would stop once all the arcs of the graph have been seen.

  The tour uses the Bit_Fields::depth_first bit. This bit
  is initiated at the beginning of the algorithm.
  

  @param[in] Graph to explore.
  @param[in] visit Function of visit.
  @param[in] sa Filter of arches.

  @return The number of nodes that were visited

  @see breadth_first_traversal() test_connectivity()

  @ingroup Grafos
*/
  template <class GT, class SA = Dft_Show_Arc<GT>> inline 
size_t depth_first_traversal(GT & g, 
			     bool (*visit)(GT &, typename GT::Node *, 
					   typename GT::Arc *),
			     SA sa = SA())
{
  return depth_first_traversal <GT, SA> (g, g.get_first_node(), visit, sa);
}

/** 
  @brief Operation of visit default. It does nothing.

  @ingroup Grafos
*/
  template <class GT>
struct Default_Visit_Op
{
  bool operator () (GT &, typename GT::Node *, typename GT::Arc *)
  {
    return false;
  }
};

/** 
  @brief Generic depth-first traversal of a graph.

  @details This class runs in depth the g graph. Each node
  visited invokes a function whose prototype should be:

  bool visitar(GT & g, typename GT::Node * p, typename GT::Arc * a)

  Whose parameters are described below:
  -# g: Graph.
  -# p: Current node you are visiting in depth.
  -# a: Archway leading to the current node p.
  .

  The class takes two type parameters:
  -# GT: Graph type, which must be derived from List_Graph.
  -# Op: Operation "visit" to make every time you see a new node.
  -# SA: Class is responsible for displaying the arc. Internally,
  the function uses iterator filter Node_Arc_Iterator (based on
  Filter_Iterator) to traverse the arcs of each node. SA is the 
  class that determines whether or not to display bow to
  traveled. Through this class, the in-depth tour
  can specialize to process the arches according to some criterion;
  arcs of a residual waste flow network, for example.
  .

  The operation of visit has the following structure:

  @code
  struct Operation
  {
    bool operator () (GT & g,
                      typename GT::Node * node, 
                      typename GT::Arc * arc)
    {
      // Return true if the journey must be stopped; 
      // false otherwise
    }
  };
  @endcode
  
  The first parameter is the g graph. The second node is the node
  current visit. Finally, the third
  arc is the arc from which node is reached. This value is
  NULL in the case of the first node visit.

  If the function returns true, then the tour stops; ie, 
  not the path on the graph continues. If the function
  returns false, then the tour continues in amplitude
  and would stop once all the arcs of the graph have been seen.

  The route uses the depth_first bit, which is initiated to
  first algorithm.

  @ingroup Grafos

  @see breadth_first_traversal() test_connectivity() Filter_Iterator
*/
      template <class GT, 
                class Operation = Default_Visit_Op<GT>, 
                class SA        = Dft_Show_Arc<GT>> 
class Depth_First_Traversal
{
  Operation * op;
  SA        & sa;
  size_t      count;
  GT *        g_ptr;

private:

  bool __dft(typename GT::Node * node, typename GT::Arc * arc = NULL)
  {
    if (IS_NODE_VISITED(node, Depth_First)) 
      return false; 

    NODE_BITS(node).set_bit(Depth_First, true); /* Mark the node as visited */
    count++;

    if ((*op)(*g_ptr, node, arc)) 
      return true; /* Yes, invoke it */

    if (count == g_ptr->get_num_nodes()) /* Are all the nodes were visited? */
      return true;

    /* Browse arches node recursively */
    for (Node_Arc_Iterator<GT, SA> it(node, sa); it.has_current(); it.next())
      {
	typename GT::Arc * arc = it.get_current_arc();
	if (IS_ARC_VISITED(arc, Depth_First)) 
	  continue;

	ARC_BITS(arc).set_bit(Depth_First, true); /* Visited */
	if (__dft (it.get_tgt_node(), arc))
	  return true; /* Already fully explored it.get_tgt_node() */
      }

    return false; /* Return and continue exploring */
  }

  size_t dft(GT & g, typename GT::Node * start_node, Operation & __op)
  {
    op = &__op;
    g_ptr = &g;
    
    g_ptr->reset_bit_nodes(Depth_First); /* Restart Depth_First nodes */
    g_ptr->reset_bit_arcs(Depth_First);  /* Restart Depth_First arches */

    count = 0;

    __dft(start_node);

    return count;
  }

public:

/** 
  @brief Functor Depth_First_Traversal builder.

  @details The parameter is the filter of arches of the inner iterator. 
*/
  Depth_First_Traversal(SA && __sa = SA()) 
    : sa(__sa) 
  { 
    /* Empty */ 
  }

/** 
  @brief Functor Depth_First_Traversal builder.
  
  @details The parameter is the filter of arches of the inner iterator. 
*/
  Depth_First_Traversal(SA & __sa) 
    : sa(__sa) 
  { 
    /* Empty */ 
  }

/** 
  @brief Invokes the in-depth tour.

  @param[in] g Graph to explore.
  @param[in] __op Operation of visit.

  @return The number of nodes that were visited
*/
  size_t operator () (GT & g, Operation && op = Operation()) 
  {
    return dft(g, g.get_first_node(), op);
  }

  size_t operator () (GT & g, Operation & op) 
  {
    return dft(g, g.get_first_node(), op);
  }

/** 
  @brief Invokes the depth tour start from a particular node.

  @param[in] g Graph to explore.
  @param[in] sn Node where the tour starts.
  @param[in] __op Operation of visit.
  
  @return The number of nodes that were visited
*/
  size_t operator () (GT & g, typename GT::Node * sn, 
		      Operation && op = Operation())
  {
    return dft(g, sn, op);
  }

  size_t operator () (GT & g, typename GT::Node * sn, Operation & op)
  {
    return dft(g, sn, op);
  }
};


  template <class GT, class SA> inline static bool
__depth_first_traversal(GT & g, typename GT::Node * node, 
			typename GT::Arc *  arc,
			bool (*visit)(GT & g, typename GT::Node *, 
				      typename GT::Arc *),
			size_t & count,
			SA sa = SA())
{
  if (IS_NODE_VISITED(node, Depth_First)) 
    return false; 

  NODE_BITS(node).set_bit(Depth_First, true); /* Mark the node as visited */
  count++;

  if (visit != NULL) /* Check is there function of visit */
    if ((*visit)(g, node, arc)) 
      return true; /* Yes, invoke it */

  if (count == g.get_num_nodes()) /* Are all the nodes were visited? */
    return true;

    /** Browse recursively the arches of node */
  for (Node_Arc_Iterator<GT, SA> it(node, sa); it.has_current(); it.next())
    {
      typename GT::Arc * arc = it.get_current_arc();
      if (IS_ARC_VISITED(arc, Depth_First)) 
	continue;

      ARC_BITS(arc).set_bit(Depth_First, true); /* Visited */
      if (__depth_first_traversal<GT, SA>(g, it.get_tgt_node(), arc,
					  visit, count, sa))
	return true; /* Already fully explored it.get_tgt_node() */
    }

  return false; /* Return and continue exploring */
} 

/** 
  @brief Tour generic in amplitude of a graph.

  @details This feature runs in the graph depth "g" from the start_node 
  node. For each node visited, it invokes a function whose
  prototype should be:

  bool visitar(GT & g, typename GT::Node * p, typename GT::Arc * a)
  
  Whose parameters are described below:
  -# g: Graph.
  -# p: Current node being visited in amplitude.
  -# a: Archway leading to the current node p.

  If the function returns true, then the tour stops; ie, 
  not the path on the graph continues. If the function
  returns false, then the tour continues in amplitude
  and would stop once all the arcs of the graph have been seen.

  The function takes two type parameters:
  -# GT: Graph type, which must be derived from List_Graph.
  -# SA: Class is responsible for displaying the arc. Internally,
  the function uses iterator filter Node_Arc_Iterator (based on
  Filter_Iterator) to traverse the arcs of each node. SA is the 
  class that determines whether or not to display bow to
  traveled.

  The route uses the breadth_first bit, which is initiated to
  first of algorithm.

  @param[in] g the Graph to explore.
  @param[in] start Node where the tour starts.
  @param[in] visit Function of visit.
  
  @return The number of nodes that were visited
  
  @throw bad_alloc If no memory for internal queue used the algorithm
  
  @ingroup Grafos
  
  @see depth_first_traversal() test_connectivity()
*/
  template <class GT, class SA = Dft_Show_Arc<GT>> inline size_t
breadth_first_traversal(GT & g, typename GT::Node * start,
			bool (*visit)(GT &, typename GT::Node *, 
				      typename GT::Arc *) )
{
  g.reset_bit_nodes(Breadth_First); 
  g.reset_bit_arcs(Breadth_First);  
  DynListQueue<typename GT::Arc*> q; /* Pending arches queue */

    /* Enter to the queue the arches of start node */
  for (Node_Arc_Iterator<GT, SA> it(start); it.has_current(); it.next())
    q.put(it.get_current_arc());

  NODE_BITS(start).set_bit(Breadth_First, true); 
  size_t node_counter = 1; /* Counter of visited nodes */

  if (visit != NULL)
    if ((*visit)(g, start, NULL))
      return 1;

    /** While remaining arcs in queue and subtract nodes for visiting */
  while (not q.is_empty() and node_counter < g.get_num_nodes()) 
    {
      typename GT::Arc * arc = q.get();/* Remove of nearest tail */
      ARC_BITS(arc).set_bit(Breadth_First, true); 

      typename GT::Node * src = g.get_src_node(arc); 
      typename GT::Node * tgt = g.get_tgt_node(arc);
      if (IS_NODE_VISITED(src, Breadth_First) and 
	  IS_NODE_VISITED(tgt, Breadth_First)) 
	continue; 

      typename GT::Node * visit_node = /* Next to visit */
	IS_NODE_VISITED(src, Breadth_First) ? tgt : src;

      if (visit != NULL)
	if ((*visit)(g, visit_node, arc))
	  break;

      NODE_BITS(visit_node).set_bit(Breadth_First, true); 
      node_counter++;

    /** Insert node in queue the arches of node recently visited */
      for (Node_Arc_Iterator<GT, SA> it(visit_node); it.has_curr(); it.next())
	{
	  typename GT::Arc * curr_arc = it.get_current_arc();
            if (IS_ARC_VISITED(curr_arc, Breadth_First)) 
              continue; 

    /* Check arches nodes to see if they have been visited */
            if (IS_NODE_VISITED(g.get_src_node(curr_arc), Breadth_First) and 
                IS_NODE_VISITED(g.get_tgt_node(curr_arc), Breadth_First))
              continue; /* Nodes already visited */

            q.put(curr_arc);
          }
    }

    return node_counter;
  }

/** 
  @brief Tour generic in amplitude of a graph.

  @details This feature runs in the graph depth "g" from any node.

  The start node is undetermined and should not be done
  assumptions about what will be this node.

  For each node visited, it invokes a function whose
  prototype should be:

  bool visitar(GT & g, typename GT::Node * p, typename GT::Arc * a)

  Whose parameters are described below:
  -# g: Graph.
  -# p: Current node being visited in amplitude.
  -# a: Archway leading to the current node p.

  If the function returns true, then the tour stops; ie, 
  not the path on the graph continues. If the function
  returns false, then the tour continues in amplitude
  and would stop once all the arcs of the graph have been seen.

  The function takes two type parameters:
  -# GT: Graph type, which must be derived from List_Graph.
  -# SA: Class is responsible for displaying the arc. Internally,
  the function uses iterator filter Node_Arc_Iterator (based on
  Filter_Iterator) to traverse the arcs of each node. SA is the 
  class that determines whether or not to display bow to
  traveled.

  The route uses the breadth_first bit. This bit is initiated to
  first of algorithm.

  @param[in] g Graph to explore.
  @param[in] visit Function of visit.

  @return The number of nodes that were visited

  @throw bad_alloc If no memory for internal queue used the algorithm

  @see breadth_first_traversal() test_connectivity()

  @ingroup Grafos
*/
  template <class GT, class SA = Dft_Show_Arc<GT>> inline size_t
breadth_first_traversal(GT &                g, 
			bool (*visit)(GT &, typename GT::Node *, 
				      typename GT::Arc *)
			)
{
  return breadth_first_traversal<GT, SA>(g, g.get_first_node(), visit);
}

/** 
  @brief Tour generic in amplitude of a graph.

  @details This class runs in amplitude the g graph. Each node
  visited, it invokes a function whose prototype should be:

  bool visitar(GT & g, typename GT::Node * p, typename GT::Arc * a)
  
  Whose parameters are described below:
  -# g: Graph.
  -# p: Current node being visited in amplitude.
  -# a: Archway leading to the current node p.

  If the function returns true, then the tour stops; ie, 
  not the path on the graph continues. If the function
  returns false, then the tour continues in amplitude
  and would stop once all the arcs of the graph have been seen.

  The class takes two type parameters:
  -# GT: Graph type, which must be derived from List_Graph.
  -# SA: Class is responsible for displaying the arc. Internally,
  the function uses iterator filter Node_Arc_Iterator (based on
  Filter_Iterator) to traverse the arcs of each node. SA is the 
  class that determines whether or not to display bow to
  traveled.

  The route uses the breadth_first bit, which is initiated to
  first of algorithm.

  @ingroup Grafos
  
  @see depth_first_traversal() test_connectivity()
*/
      template <class GT, 
                class Operation = Default_Visit_Op<GT>, 
                class SA        = Dft_Show_Arc<GT>> 
class Breadth_First_Traversal
{
  SA     & sa;
  size_t   count;

  size_t bft(GT & g, typename GT::Node * start, Operation & op)
  {
    g.reset_bit_nodes(Breadth_First); 
    g.reset_bit_arcs(Breadth_First);  
    DynListQueue<typename GT::Arc*> q; /* Pending arches queue */

    /** Enter to the queue the arches of start node */
    for (Node_Arc_Iterator<GT, SA> it(start); it.has_current(); it.next())
      q.put(it.get_current_arc());

    NODE_BITS(start).set_bit(Breadth_First, true); 
    count = 1; /* Counter of visited nodes */

    if (op (g, start, NULL))
      return 1;

    /** While remaining arcs in queue and take from all visit */
    while (not q.is_empty() and count < g.get_num_nodes()) 
      {
	typename GT::Arc * arc = q.get();/* Remove of nearest tail */
	ARC_BITS(arc).set_bit(Breadth_First, true); 

	typename GT::Node * src = g.get_src_node(arc); 
	typename GT::Node * tgt = g.get_tgt_node(arc);

	if (IS_NODE_VISITED(src, Breadth_First) and
	    IS_NODE_VISITED(tgt, Breadth_First))
	  continue;

	typename GT::Node * curr = /* Next to visit */
	  IS_NODE_VISITED(src, Breadth_First) ? tgt : src;

	if (op (g, curr, arc))
	  break;

	NODE_BITS(curr).set_bit(Breadth_First, true); 
	count++;

  	/** Insert in arches queue of node recently visited */
	for (Node_Arc_Iterator<GT, SA> it(curr); it.has_current(); it.next())
	  {
	    typename GT::Arc * curr_arc = it.get_current_arc();
	    if (IS_ARC_VISITED(curr_arc, Breadth_First)) 
	      continue; 

	  /* Check arches nodes to see if they have been visited */
	    if (IS_NODE_VISITED(g.get_src_node(curr_arc), Breadth_First) and 
		IS_NODE_VISITED(g.get_tgt_node(curr_arc), Breadth_First))
	      continue; /* Nodes already visited */

	    q.put(curr_arc);
	  }
      }

    return count;
  }  

public:

/** 
  @brief Functor Breadth_First_Traversal builder.
  The parameter is the arc filter of the inner iterator. 
*/
  Breadth_First_Traversal(SA && __sa = SA()) : sa(__sa) { /* Empty */ }

  Breadth_First_Traversal(SA & __sa) : sa(__sa) { /* Empty */ }

/** 
  @brief Invokes the tour in amplitude.
            
  @param[in] g Graph to explore.
  @param[in] op Operation of visit.

  @return The number of nodes that were visited

  @throw bad_alloc If no memory for internal queue used the algorithm
*/
  size_t operator () (GT & g, Operation && op = Operation())
  {
    return bft (g, g.get_first_node(), op);
  }

  size_t operator () (GT & g, Operation & op)
  {
    return bft (g, g.get_first_node(), op);
  }

/** 
  @brief Invokes the tour by amplitude from a start node.

  @param[in] g Graph to explore.
  @param[in] p Node where the tour starts.
  @param[in] op Operation of visit.
  
  @return The number of nodes that were visited
  
  @throw bad_alloc If no memory for internal queue used the algorithm
*/
  size_t operator () (GT & g, typename GT::Node * p, 
		      Operation && op = Operation())
  {
    return bft(g, p, op);
  }

  size_t operator () (GT & g, typename GT::Node * p, Operation & op)
  {
    return bft(g, p, op);
  }
};


      template <class GT> inline static
void __insert_in_queue(GT & g, DynListQueue<Path<GT> *> & q,
		       typename GT::Node * node, typename GT::Arc * arc,
		       Path<GT> * path_ptr = NULL)
{
  unique_ptr<Path<GT>> path_auto;
  if (path_ptr == NULL) /* Start or copy? */
    path_auto = unique_ptr<Path<GT>>(new Path<GT>(g, node)); /* Start */
  else
    path_auto = unique_ptr<Path<GT>>(new Path<GT>(*path_ptr));/* Copy */

  path_auto->append(arc);     /* Add new arc */
  q.put(path_auto.get());     /* Insert the new path in queue */
  path_auto.release();
}

/** 
  @brief Searches in amplitude a path between start_node and end_node;
  built the road viewed if you find the way.

  @details find_path_breadth_first() amplitude searches a path between
  start_node and end_node, while building a road going
  to the destination node. If there is a path, then the
  method returns true and the "path" parameter on the way home
  question; otherwise, the function returns false and value
  path is undetermined.

  The function takes two type parameters:
  -# GT: Graph type, which must be derived from List_Graph.
  -# SA: Class is responsible for displaying the arc. Internally,
  the function uses iterator filter Node_Arc_Iterator (based on
  Filter_Iterator) to traverse the arcs of each node.
  SA is the class that determines whether or not to display bow to
  traveled.
  .

  @param[in] g Graph on which you want to find the road.
  @param[in] start A pointer at the beginning node of the road.
  @param[in] end Pointer to the destination node of the road. 
  @param[out] path The road seen during the search in amplitude;
  only it makes sense if the return value is true.

  @throw bad_alloc If no memory is to continue building the road
  or the internal queue for travel in amplitude.

  @see find_path_depth_first()
  @see dijkstra_min_spanning_tree() dijkstra_min_path()
  @see bellman_ford_min_spanning_tree() q_bellman_ford_min_spanning_tree()

  @ingroup Grafos
*/
    template <class GT, class SA = Dft_Show_Arc<GT>> inline 
bool find_path_breadth_first(GT & g, typename GT::Node * start, 
			     typename GT::Node * end, Path<GT> & path)
{
  if (not path.inside_graph(g))
    throw std::invalid_argument("Path does not belong to graph");

  path.clear_path(); /* Limpiamos cualquier cosa que esté en path */
  g.reset_nodes();
  g.reset_arcs(); 
  
  DynListQueue<typename GT::Arc*> q; /* Cola partial paths */

    /** Insert initial arcs coming out from start */
  for (Node_Arc_Iterator<GT, SA> i(start); i.has_curr(); i.next())
    q.put(i.get_current_arc());
        
  NODE_BITS(start).set_bit(Find_Path, true); /* Mark it as visited */

  bool path_found = false;

    /** While remaining arches to visit */
  while (not q.is_empty()) 
    {
      typename GT::Arc * arc = q.get(); 
      typename GT::Node * src = g.get_src_node(arc); 
      typename GT::Node * tgt = g.get_tgt_node(arc);

      if (IS_NODE_VISITED(src, Find_Path) and IS_NODE_VISITED(tgt, Find_Path))
	continue;
      
      if (IS_NODE_VISITED(tgt, Find_Path))
	std::swap(src, tgt);

      ARC_BITS(arc).set_bit(Find_Path, true); 
      NODE_BITS(tgt).set_bit(Find_Path, true);
      NODE_COOKIE(tgt) = src;
        
      if (tgt == end) /* Path Found? */
	{
	  path_found = true;
	  break;
	}

      for (Node_Arc_Iterator<GT,SA> i(tgt); i.has_curr(); i.next())
	{
	  typename GT::Arc * a = i.get_current_arc();

    /** Arc visited?
    If the answer is YES, then advance to the next */
	  if (IS_ARC_VISITED(a, Find_Path)) 
	    continue; 

	  /** Check arch nodes to see if they have been visited
    If the answer is YES, then do not put into arc */
	  if (IS_NODE_VISITED(g.get_src_node(a), Find_Path) and 
	      IS_NODE_VISITED(g.get_tgt_node(a), Find_Path))
	    continue;

	  q.put(a);
	}
    }
  
  if (not path_found)
    return false;

  q.empty();
  path.insert(end);
  typename GT::Node * p = end;
  while (p != start)
    {
      p = (typename GT::Node *) NODE_COOKIE(p);
      path.insert(p);
    }

  return true;
}


/** 
  @brief Returns true if the g graph is connected.

  @details This function performs a test connectivity of the g graph. The
  test calls for an in-depth tour.

  The function checks the number of arcs. If this amount is
  less than the number of nodes, then the graph is considered
  disjointed.

  The function takes two type parameters:
  -# GT: Graph type, which must be derived from List_Graph.
  -# SA: Class is responsible for displaying the arc. Internally,
  the function uses iterator filter Node_Arc_Iterator (based on
  Filter_Iterator) to traverse the arcs of each node.
  SA is the class that determines whether or not to display bow to
  traveled.

  @param[in] g Graph or digraph to verify.

  @return true if the graph is connected; false otherwise.

  @note Because testing with the number of arcs, this function
  is incorrect to multigraphs.

  @throw domain_error If the routine is invoked on a digraph.

  @ingroup Grafos

  @see depth_first_traversal() is_reachable()
*/
    template <class GT, class SA = Dft_Show_Arc<GT>> inline 
bool test_connectivity(GT & g)
{
  if (g.is_digraph()) /* Only is valid for graphs, digraphs not */
    throw std::domain_error("test_connectivity() does not work on digraphs");

  if (g.get_num_arcs() < g.get_num_nodes() - 1) 
    return false; 

  return depth_first_traversal<GT, SA>(g, NULL) == g.get_num_nodes();
}


      template <class GT, class SA> inline static
  bool __test_cycle(GT & g, typename GT::Node * src_node, 
                    typename GT::Node * curr_node);

/** 
  @brief Returns true if there is a cycle since src_node node.

  @details test_for_cycle() explores in depth the g graph from node start_node 
  and see if there is any cycle from him.

  The test_cycle bit is used and initiated at the beginning of the 
  algorithm to mark the visited nodes and arcs.

  @note The routine only checks existence cycle, it says nothing
  all about the composition of the cycle.

  @param[in] g Graph to check.
  @param[in] src_node Node being that you want check.
  @return true if there is a cycle; false otherwise.

  @ingroup Grafos
*/
      template <class GT, class SA = Dft_Show_Arc<GT>> inline 
  bool test_for_cycle(GT & g, typename GT::Node * src_node)
  {
    g.reset_bit_nodes(Test_Cycle); /* Restart Test_Cycle to nodes */
    g.reset_bit_arcs(Test_Cycle);  /* Restart Test_Cycle to arches */

    /** Exploring recursively by adjacent arcs to src_node */
    for (Node_Arc_Iterator<GT, SA> it(src_node); it.has_current(); it.next())
      {
        typename GT::Arc * arc = it.get_current_arc();
        if (IS_ARC_VISITED(arc, Test_Cycle)) 
          continue;

        ARC_BITS(arc).set_bit(Test_Cycle, true); /* Paint arc */

        if (__test_cycle<GT,SA>(g, src_node, it.get_tgt_node()))
          return true; /* Cycle detected */
      }
    /* We have explored all the roads from src_node
    without finding again src_node ==> no cycle */
    return false;
  }


      template <class GT, class SA> inline static
  bool __test_cycle(GT & g, typename GT::Node * src_node, 
                    typename GT::Node * curr_node)
  {
    if (src_node == curr_node) 
      return true; /* Cycle detected! */

    if (IS_NODE_VISITED(curr_node, Test_Cycle)) 
      return false;

    NODE_BITS(curr_node).set_bit(Test_Cycle, true); /* Check node */

    /** Find ways from current_node to see if it src_node */
    for (Node_Arc_Iterator<GT, SA> it(curr_node); it.has_current(); it.next())
      {
        typename GT::Arc * arc = it.get_current_arc();
        if (IS_ARC_VISITED(arc, Test_Cycle)) 
          continue;

        ARC_BITS(arc).set_bit(Test_Cycle, true); /* Mark arc */

        if (__test_cycle<GT,SA>(g, src_node, it.get_tgt_node()))
          return true; /* Cycle from the current arc found */
      }
    /* At this point we have explored ways from curr_node
    without finding src_node ==> no exists cycle by curr_node */
    return false; 
  }  

      template <class GT, class SA> inline static
  bool __is_graph_acyclique(GT & g, typename GT::Node * curr_node)
  {
    if (IS_NODE_VISITED(curr_node, Test_Cycle)) 
      return false;

    NODE_BITS(curr_node).set_bit(Test_Cycle, true); /* Mark node */

    for (Node_Arc_Iterator<GT, SA> i(curr_node); i.has_current(); i.next())
      {
        typename GT::Arc * arc = i.get_current_arc();
        if (IS_ARC_VISITED(arc, Test_Cycle)) 
          continue; 

        ARC_BITS(arc).set_bit(Test_Cycle, true); 

        if (not __is_graph_acyclique<GT,SA>(g, i.get_tgt_node())) 
          return false;
      }
    /* All the tours arches without finding cycle, then
    the graph is acyclic through curr_node */
    return true; 
  }

/** 
  @brief Returns true if the graph is acyclic (it contains no cycles).

  @details is_graph_acyclique(g, start_node) explores in depth in g graph, a
  from start_node node, and checks if the graph is acyclic; It is
  ie, containing no cycle.

  The function takes two type parameters:
  -# GT: Graph type, which must be derived from List_Graph.
  -# SA: Class is responsible for displaying the arc. Internally,
  the function uses iterator filter Node_Arc_Iterator (based on
  Filter_Iterator) to traverse the arcs of each node.
  SA is the class that determines whether or not to display bow to
  traveled.
  . 

  The function uses the is_acyclique bit and restarts at the beginning of
  algorithm to mark the nodes and arcs already visited.

  If g is a graph (not a digraph) then the algorithm compares the
  arch with the amount of nodes. If the graph has fewer arcs
  that node, then it follows that the graph is acyclic.
  Consequently, this technique does not work for multigraphs.
      
  @param[in] g Graph to test.
  @param[in] start_node Node from which to start search in depth.

  @return true if the graph does not contain any cycle; false otherwise.

  @note Because the initial test with the number of arcs, this routine
  is preferable that test_for_cycle(), still under the knowledge that the
  graph is related, since the latter always performs routine
  in-depth exploration while that is_graph_acyclique() no.

  @ingroup Grafos
*/
    template <class GT, class SA = Dft_Show_Arc<GT>> inline 
bool is_graph_acyclique(GT & g, typename GT::Node * start_node)
{
  if (g.is_digraph())
    throw std::domain_error("is_graph_acyclique() does not work for digraps");
  
  if (g.get_num_arcs() >= g.get_num_nodes()) 
    return false; 

  g.reset_bit_arcs(Test_Cycle);
  g.reset_bit_nodes(Test_Cycle);

  return __is_graph_acyclique<GT,SA>(g, start_node);
}

/** 
  @brief Returns true if the graph is acyclic (it contains no cycles).

  @details is_graph_acyclique(g) explores in depth and verified in the g graph
  if the graph is acyclic; ie containing no cycle.

  The function uses the is_acyclique bit and restarts at the beginning of
  algorithm to mark the nodes and arcs already visited.

  If g is a graph (not a digraph) then the algorithm compares the
  arch amount with the of nodes. If the graph has fewer arcs
  that node, then it follows that the graph is acyclic.
  Consequently, this technique does not work for multigraphs.

  @param[in] g Graph to test.
  
  @return true if the graph does not contain any cycle; false otherwise.
  
  @note Because the initial test with the number of arcs, this routine
  is preferable that test_for_cycle(), still under the knowledge that the
  graph is related, since the latter always performs routine
  in-depth exploration while that is_graph_acyclique() no.
  
  @see test_for_cycle() has_cycle()
  
  @ingroup Grafos
*/
    template <class GT, class SA = Dft_Show_Arc<GT>> inline 
bool is_graph_acyclique(GT & g)
{
  if (g.is_digraph())
    throw std::domain_error("is_graph_acyclique() does not work for digraps");

  if (g.get_num_arcs() >= g.get_num_nodes()) 
    return false; 

  g.reset_bit_arcs(Test_Cycle);
  g.reset_bit_nodes(Test_Cycle);

  for (Node_Iterator<GT> it(g); it.has_current(); it.next()) 
    {
      typename GT::Node * current_node = it.get_current_node();
      if (IS_NODE_VISITED(current_node, Test_Cycle)) 
	continue; 

      if (not __is_graph_acyclique<GT,SA>(g, current_node)) 
	return false;
    }

  return true;
}

/** 
  @brief Returns true if the graph is not acyclic (containing at least one cycle).

  @details has_clique(g) explores in depth into g graph and verified
  if the graph has at least one cycle.

  The function is denied the is_graph_acyclique().

  The function takes two type parameters:
  -# GT: Graph type, which must be derived from List_Graph.
  -# SA: Class is responsible for displaying the arc. Internally,
  the function uses iterator filter Node_Arc_Iterator (based on
  Filter_Iterator) to traverse the arcs of each node.
  SA is the class that determines whether or not to display bow to
  traveled.

  A specialization dispenses the parameter SA type and shows
  all the arcs of the graph without filtering.

  If g is a graph (not a digraph) then the algorithm compares the
  arch amount with the of nodes. If the graph has fewer arcs
  that node, then it follows that the graph is acyclic.
  Consequently, this technique does not work for multigraphs.

  @param[in] g Graph testing in depth.

  @return true if the graph does not contain any cycle; false otherwise.

  @note Because the initial test with the number of arcs, this routine
  is preferable that test_for_cycle(), still under the knowledge that the
  graph is related, since the latter always performs routine
  in-depth exploration while that is_graph_acyclique() no.

  @see test_for_cycle() is_graph_acyclique()

  @ingroup Grafos
*/
      template <class GT, class SA = Dft_Show_Arc<GT>> 
inline bool has_cycle(GT & g)
{
  return not is_graph_acyclique<GT,SA>(g);
}

 
      template <class GT, class SA> inline static
bool __test_for_path(GT & g, typename GT::Node * curr_node,
                     typename GT::Node * end_node);

/** 
  @brief Returns true if there is a path between the node start_node and end_node.

  @details test_for_path() explores in depth the g graph from
  start_node node in finding a path that derive in
  end_node. If you see end_node during scanning, then
  it concluded that there is a way and returns true. Whether
  runs entirely to g graph without seeing end_node, then the function
  returns false.

  The function takes two type parameters:
  -# GT: Graph type, which must be derived from List_Graph.
  -# SA: Class is responsible for displaying the arc. Internally,
  the function uses iterator filter Node_Arc_Iterator (based on
  Filter_Iterator) to traverse the arcs of each node. SA is the 
  class that determines whether or not to display bow to
  traveled.

  The test_path bit is used to mark the nodes and arcs
  visited during the search.

  @param[in] g Graph searching road.
  @param[in] start_node Pointer to the origin node of the road.
  @param[in] end_node Pointer to destination node of road.

  @return true if there is a path between start_node and end_node.

  @see find_path_depth_first() find_path_breadth_first()

  @ingroup Grafos
*/
      template <class GT, class SA = Dft_Show_Arc<GT>> inline 
bool test_for_path(GT& g, typename GT::Node * start_node, 
		   typename GT::Node * end_node)
{
    /* If the graph is connected, then there way */
  if (not g.is_digraph() and g.get_num_arcs() >= g.get_num_nodes()) 
    return true;

  g.reset_bit_nodes(Find_Path); 
  g.reset_bit_arcs(Find_Path);

    /* Search recursively adjacent arcs start_node */
  for (Node_Arc_Iterator<GT, SA> i(start_node); i.has_current(); i.next())
    {
      typename GT::Arc * arc = i.get_current_arc();
      ARC_BITS(arc).set_bit(Find_Path, true); /* Mark arc */
      if (__test_for_path<GT, SA>(g, i.get_tgt_node(), end_node)) 
	return true; 
    }
    /* All start_node arches have been explored without
     end_node way to find, there is no way entoces */
  return false;
}


      template <class GT, class SA> inline static
bool __test_for_path(GT & g, typename GT::Node * curr_node, 
		     typename GT::Node * end_node)  
{
  if (curr_node == end_node) 
    return true; /* end_node reached? */

    /** curr_node visited??
    Yes? ==> don't explore */
  if (IS_NODE_VISITED(curr_node, Find_Path)) 
    return false; 

  NODE_BITS(curr_node).set_bit(Find_Path, true); /* Paint curr_node */

    /** Search recursively through arches of curr_node */
  for (Node_Arc_Iterator<GT, SA> i(curr_node); i.has_current(); i.next())
    { 
      typename GT::Arc * arc = i.get_current_arc();
      if (IS_ARC_VISITED(arc, Find_Path)) 
	continue; 

      ARC_BITS(arc).set_bit(Find_Path, true); /* Paint arc */
      if (__test_for_path<GT, SA>(g, i.get_tgt_node(), end_node)) 
	return true;
    }

    /* All adjacent arches curr_node explored without find end_node, 
    then there is no way to curr_node */
  return false;
}

      template <class GT, class SA = Dft_Show_Arc<GT>> inline 
  void inconnected_components(GT & g, DynList<GT> & list);

      template <class GT, class SA = Dft_Show_Arc<GT>> inline 
  void build_subgraph(GT & g, GT & sg, 
                      typename GT::Node * g_src, size_t & node_count); 

    
/** 
  @brief Calculate the disjointed components of a graph.
          
  @details inconnected_components() takes a g graph,
  "supposedly unconnected" calculates its subgraphs or components
  disconnected and stored in a dynamic list of subgraphs list
  mapped both the nodes and the arcs. The emphasis on
  of course you connected graph character is that if the graph does not
  it was connected, then the resulting list would contain a single
  copy the graph corresponding element mapping g graph. If this
  were the case, then it is best done with the function
  copy_graph().

  The function takes two type parameters:
  -# GT: Graph type, which must be derived from List_Graph.
  -# SA: Class is responsible for displaying the arc. Internally,
  the function uses iterator filter Node_Arc_Iterator (based on
  Filter_Iterator) to traverse the arcs of each node.
  SA is the class that determines whether or not to display bow to
  traveled.

  A specialization dispenses the parameter SA type and shows
  all the arcs of the graph without filtering.

  The function uses the bit build_subtree to mark nodes and
  arches visited.

  The algorithm internally finds the components by depth searches.

  @param[in] g Graph on which you want to calculate their blocks.
  @param[out] list List of mapped subgraphs to "g" 
  with the subgraphs or g disjointed components.
  
  @throw bad_alloc If no memory to build some block or 
  insert are listed.

  @see copy_graph()

  @ingroup Grafos
*/
      template <class GT, class SA> inline 
void inconnected_components(GT & g, DynList<GT> & list)
{
  g.reset_nodes(); 
  g.reset_arcs();  
  size_t count = 0; /* Counter of visited nodes */
  for (typename GT::Node_Iterator i(g); /* Walk nodes of g */
       count < g.get_num_nodes() and i.has_current(); i.next())
    {
      typename GT::Node * curr = i.get_current_node();
      if (IS_NODE_VISITED(curr, Build_Subtree)) 
	continue;

    /* Create subgraph disjointed component connected by curr_node */
      list.append(GT()); /* Create subgraph and inserts in list */
      GT & subgraph = list.get_last(); /* Graph inserted in list */

      build_subgraph <GT, SA> (g, subgraph, curr, count); 
    }
}


/** 
  @brief Build a mapping subgraph of g graph from one of its nodes.

  @details The build_subgraph() method iterates through in-depth g graph
  from the g_src source node and builds on a mapped copy of "sg"
  all the graph (or subgraph if g is disconnected) seen in the
  I traveled.

  The node_count parameter is a counter that is incremented to
  number of nodes that were visited and mapped.

  The method uses the bit build_subtree to mark the nodes and
  arches already visited.

  build_subgraph() it is used by the method
  inconnected_components() to map the various blocks.

  @param[in] g Graph to map
  @param[out] sg Graph empty place where the mapped copy from g_src.
  @param[in] g_src Origin node from which the route originates and mapping.
  @param[in,out] node_count Current number of visited nodes
  before the call; this value is increased to the total amount
  of nodes that managed to visit build_subgraph().

  @throw bad_alloc If no memory to build sg.

  @see inconnected_components() copy_graph()

  @ingroup Grafos
*/
    template <class GT, class SA> inline 
void build_subgraph(GT & g, GT & sg, 
		    typename GT::Node * g_src, size_t & node_count)
{
  if (IS_NODE_VISITED(g_src, Build_Subtree)) 
    return;

  NODE_BITS(g_src).set_bit(Build_Subtree, true); /* g_src visited */
  ++node_count; 

  typename GT::Node * sg_src = mapped_node <GT> (g_src);

    /** Are you mapped g_src?
    If the answer is NO, then create image
    of g_src in the subgraph of "sg" and map */
  if (sg_src == NULL) 
    {     
      sg_src = sg.insert_node(g_src->get_info());
      GT::map_nodes(g_src, sg_src);
    }

  for (Node_Arc_Iterator<GT, SA> i(g_src); /* Explore from g_src */
       node_count < g.get_num_nodes() and i.has_current(); i.next())
    {
      typename GT::Arc * arc = i.get_current_arc();
      if (IS_ARC_VISITED(arc, Build_Subtree)) 
	continue; /* Next advance arc */

      ARC_BITS(arc).set_bit(Build_Subtree, true); /* arc visited */
      typename GT::Node * g_tgt  = i.get_tgt_node(); /* Destiny of arc */
      typename GT::Node * sg_tgt = mapped_node <GT> (g_tgt);

    /** Are you mapped in sg?
    If the answer is NO, then you have to map it and
    insert the subgraph sg */
      if (sg_tgt == NULL) 
	{    
    
	  sg_tgt = sg.insert_node(g_tgt->get_info()); 
	  GT::map_nodes(g_tgt, sg_tgt); 
	}

    /* We subgraph nodes, insert arch and maps */
      typename GT::Arc * sg_arc = 
	sg.insert_arc(sg_src, sg_tgt, arc->get_info());
      GT::map_arcs(arc, sg_arc); 

      build_subgraph<GT,SA>(g, sg, g_tgt, node_count);
    }
}

  
    template <class GT, class SA> inline static
bool __find_depth_first_spanning_tree(GT &                g, 
				      typename GT::Node * gnode, 
				      typename GT::Arc *  garc, 
				      GT &                tree,
				      typename GT::Node * tnode);

/** 
  @brief Computes a spanning tree of a graph in depth from
  a node.

  @details This function takes a g graph, takes a look at
  depth from the gnode node and and builds the 
  tree comprehensive in the order given by the tour visit.

  The function takes two type parameters:
  -# GT: Graph type, which must be derived from List_Graph.
  -# SA: Class is responsible for displaying the arc. Internally,
  the function uses iterator filter Node_Arc_Iterator (based on
  Filter_Iterator) to traverse the arcs of each node.
  SA is the class that determines whether or not to display bow to
  traveled.

  A specialization dispenses the parameter SA type and shows
  all the arcs of the graph without filtering.

  After the operation the parameter "tree" contains the tree
  encompassing concerned with their aches and mapped nodes to the g graph.

  During the execution, the algorithm marks the nodes and
  arches visited with the spanning_tree bit.

  The graph visitation order is not necessarily the same as
  the primitive depth_first_traversal().

  @param[in] g Graph on which you want to build the spanning tree in depth.
  @param[in] gnode Pointer to the source node of the depth search.
  @param[out] tree Graph where is placed the spanning tree of depth. 

  @return true if the graph is connected and there spanning tree;
  false otherwise.

  @throw bad_alloc If no memory to build the spanning tree.

  @ingroup Grafos

  @see find_breadth_first_spanning_tree() Tree_Node
  @see graph_to_tree_node() 
*/
      template <class GT, class SA = Dft_Show_Arc<GT>> inline
void find_depth_first_spanning_tree(GT & g, typename GT::Node * gnode, 
				    GT & tree)
{
  g.reset_nodes();
  g.reset_arcs(); 

  clear_graph(tree); /* Ensure that destination tree is empty */
  
  NODE_BITS(gnode).set_bit(Spanning_Tree, true); /* Mark gnode */
  
  typename GT::Node * tnode = tree.insert_node(gnode->get_info()); 
  GT::map_nodes(gnode, tnode);
  
  for (Node_Arc_Iterator<GT, SA> i(gnode); i.has_current(); i.next())
    {
      typename GT::Arc * arc = i.get_current_arc();
      if (IS_ARC_VISITED(arc, Spanning_Tree)) 
	continue;

      typename GT::Node * arc_tgt_node = i.get_tgt_node();
      if (IS_NODE_VISITED(arc_tgt_node, Spanning_Tree))
	continue; /* Destination and visited from another arc */

      if (__find_depth_first_spanning_tree<GT,SA>(g, arc_tgt_node, 
						  arc, tree, tnode))
	return; /*  Tree is calculated */
    }
}

/** 
  @brief Computes a spanning tree in depth of a graph.

  @details This function takes a g graph, takes a look at
  depth from the selected node function and builds the tree
  comprehensive in the order given by the tour visit.

  The function takes two type parameters:
  -# GT: Graph type, which must be derived from List_Graph.
  -# SA: Class is responsible for displaying the arc. Internally,
  the function uses iterator filter Node_Arc_Iterator (based on
  Filter_Iterator) to traverse the arcs of each node.
  SA is the class that determines whether or not to display bow to
  traveled.

  After the operation the parameter "tree" contains the tree
  encompassing concerned with their aches and mapped nodes to the g graph.

  During the execution, the algorithm marks the nodes and
  arches visited with the spanning_tree bit.

  The graph visitation order is not necessarily the same as
  the primitive depth_first_traversal().

  @param[in] g Graph on which you want to build the spanning tree in depth.
  @param[out] tree Graph where is placed the spanning tree of depth. 

  @throw bad_alloc If no memory to build the spanning tree.

  @ingroup Grafos

  @see find_breadth_first_spanning_tree() Tree_Node
  @see graph_to_tree_node() 
*/
      template <class GT, class SA = Dft_Show_Arc<GT>> inline
void find_depth_first_spanning_tree(GT & g, GT & tree)
{
  typename GT::Node * start_node = g.get_first_node();
  find_depth_first_spanning_tree<GT,SA>(g, start_node, tree);
}

  
      template <class GT, class SA> inline static
bool __find_depth_first_spanning_tree(GT & g, typename GT::Node * gnode, 
				      typename GT::Arc *  garc, 
				      GT & tree, typename GT::Node * tnode)
{
  NODE_BITS(gnode).set_bit(Spanning_Tree, true); /* Mark node */
  ARC_BITS(garc).set_bit(Spanning_Tree, true);   /* Mark arc */

  typename GT::Node * tree_tgt_node = tree.insert_node(gnode->get_info());
  GT::map_nodes(gnode, tree_tgt_node);

  typename GT::Arc * tarc = 
    tree.insert_arc(tnode, tree_tgt_node, garc->get_info()); 
  GT::map_arcs(garc, tarc);

  tnode = tree_tgt_node; 
  if (tree.get_num_nodes() == g.get_num_nodes()) /* Graph encompassed? */ 
    return true; /* Tree already contains the spanning tree */

  assert(tree.get_num_nodes() > tree.get_num_arcs()); /* It must be acyclic */

  for (Node_Arc_Iterator<GT, SA> i(gnode); i.has_current(); i.next())
    {
      typename GT::Arc * arc = i.get_current_arc();
      if (IS_ARC_VISITED(arc, Spanning_Tree)) 
	continue;

      typename GT::Node * arc_tgt_node = i.get_tgt_node();
      if (IS_NODE_VISITED(arc_tgt_node, Spanning_Tree))
	continue; /* Destination and visited from another arc */

      if (__find_depth_first_spanning_tree<GT,SA>(g, arc_tgt_node, 
						  arc, tree, tnode))
	return false; /* the tree is all ready calculated */
    }

  return false;
}

/** 
  @brief Calculates an amplitude spanning tree of a graph from a node.

  @details This function takes a g graph, takes a look at
  amplitude from the gnode node and builds tree
  comprehensive in the order given by the tour visit.

  The function takes two type parameters:
  -# GT: Graph type, which must be derived from List_Graph.
  -# SA: Class is responsible for displaying the arc. Internally,
  the function uses iterator filter Node_Arc_Iterator (based on
  Filter_Iterator) to traverse the arcs of each node.
  SA is the class that determines whether or not to display bow to
  traveled.

  A specialization dispenses the parameter SA type and shows
  all the arcs of the graph without filtering.

  After the operation the parameter "tree" contains the tree
  encompassing concerned with their aches and mapped nodes to the g graph.

  During the execution, the algorithm marks the nodes and
  arches visited with the spanning_tree bit.

  The graph visitation order is not necessarily the same as
  the primitive breadth_first_traversal(). 

  @param[in] g Graph on which you want to build the spanning tree amplitude.
  @param[in] gp Pointer to the source node of the amplitude  search.
  @param[out] tree The graph where is placed the spanning tree of amplitude.

  @throw bad_alloc If no memory to build the spanning tree
  or for internal queue used for travel in amplitude.

  @ingroup Grafos

  @see find_depth_first_spanning_tree() Tree_Node
  @see graph_to_tree_node() 
*/
      template <class GT, class SA = Dft_Show_Arc<GT>> inline
void find_breadth_first_spanning_tree(GT & g, typename GT::Node * gp, GT & tree)
{
  g.reset_bit_nodes(Spanning_Tree);
  g.reset_bit_arcs(Spanning_Tree);
  clear_graph(tree); 
  unique_ptr<typename GT::Node> tp_auto(new typename GT::Node(gp));
  tree.insert_node(tp_auto.get());
  GT::map_nodes(gp, tp_auto.release());

  DynListQueue<typename GT::Arc*> q; /* Insert into queue the gp arcs */
  for (Node_Arc_Iterator<GT, SA> i(gp); i.has_current(); i.next())
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
	continue; /* Both nodes have already been visited garc */

    /** gtgt visited?
    If the answer is YES, then exchange it with gsrc */
      if (IS_NODE_VISITED(gtgt, Spanning_Tree)) 
	std::swap(gsrc, gtgt); 

      typename GT::Node * tsrc = mapped_node<GT>(gsrc);
      NODE_BITS(gtgt).set_bit(Spanning_Tree, true); 
    /* visited gtgt */

    /* Back up gtgt, insert tree and mapping */
      unique_ptr<typename GT::Node> ttgt_auto(new typename GT::Node(gtgt));
      tree.insert_node(ttgt_auto.get());
      typename GT::Node * ttgt = ttgt_auto.release();
      GT::map_nodes(gtgt, ttgt);

    /* Insert new tree arc and mapping*/
      typename GT::Arc * tarc = tree.insert_arc(tsrc, ttgt, garc->get_info());
      GT::map_arcs(garc, tarc);
      if (tree.get_num_nodes() == g.get_num_nodes()) /* ¿Covers a g? */
	break; 

    /** Insert queued arches gtgt */
      for (Node_Arc_Iterator<GT, SA> i(gtgt); i.has_current(); i.next())
	{
	  typename GT::Arc * current_arc = i.get_current_arc();
	  if (IS_ARC_VISITED(current_arc, Spanning_Tree)) 
	    continue;

    /** Check arches nodes to see if they have been visited
    If the answer is YES, then put in an arc */
	  if (IS_NODE_VISITED(g.get_src_node(current_arc),Spanning_Tree) and 
	      IS_NODE_VISITED(g.get_tgt_node(current_arc),Spanning_Tree))
	    continue; 
	  q.put(current_arc);
	}
    }
}

/** 
  @brief Builds a spanning tree of a graph represented with arrays.

  @details build_spanning_tree() takes a g graph and a spanning tree
  represented by settling "arcs" arcs. Then tree built on the 
  spanning tree.

  @param[in] g Graph on which you want to build the alternate spanning tree.
  @param[out] tree Spanning tree alternative obtained from the pred and arcs arrays.
  @param[in] arcs Array of pointers to arcs.
  @param[in] with_map Indicates whether the spanning tree should be mapped with the g graph.

  @throw bad_alloc If there is insufficient memory for build the 
  spanning tree.

  @ingroup Grafos
*/
      template <class GT>
void build_spanning_tree(GT & g, GT & tree, DynArray<typename GT::Arc> * arcs,
			 const bool with_map = false)
{
  tree.clear();
  for (int i = 0; i < g.get_num_nodes(); ++i)
    {
      typename GT::Arc * garc = arcs[i];
      if (garc == NULL) 
	continue;

      typename GT::Node * gsrc = g.get_src_node(garc);
      typename GT::Node * tsrc = NULL;
      if (IS_NODE_VISITED(gsrc, Spanning_Tree))
	tsrc = mapped_node <GT> (gsrc);
      else
	{
	  NODE_BITS(gsrc).set_bit(Spanning_Tree, true); 
	  tsrc = tree.insert_node(gsrc->get_info());
	}

      typename GT::Node * gtgt = g.get_tgt_node(garc);
      typename GT::Node * ttgt = NULL;
      if (IS_NODE_VISITED(gtgt, Spanning_Tree))
	ttgt = mapped_node <GT> (gtgt);
      else
	{
	  NODE_BITS(gtgt).set_bit(Spanning_Tree, true);
	  ttgt = tree.insert_node(gtgt->get_info());
	}

      typename GT::Arc * tarc = tree.insert_arc(tsrc, ttgt);
      *tarc = *garc;

      ARC_BITS(garc).set_bit(Min, true);
      if (with_map)
	{
	  GT::map_nodes(gsrc, tsrc);
	  GT::map_nodes(gtgt, ttgt);
	  GT::map_arcs(garc, tarc);
	}
    }
}

template <class GT>
GT build_spanning_tree(DynArray<typename GT::Arc*> & arcs)
{
  using Node = typename GT::Node;
  using Arc  = typename GT::Arc;

  GT ret;
  DynMapTree<Node*, Node*> table;
  arcs.for_each([&table, &ret] (Arc * ga)
    {
      if (ga == NULL)
	return;

      Node * gsrc = (Node*) ga->src_node;
      Node * gtgt = (Node*) ga->tgt_node;

      Node * tsrc;
      Node ** tsrc_ptr = table.search(gsrc);
      if (tsrc_ptr)
	tsrc = *tsrc_ptr;
      else
	{
	  tsrc = ret.insert_node(gsrc->get_info()); /* delete get_info() */
	  table.insert(gsrc, tsrc);
	  NODE_COOKIE(tsrc) = gsrc;
	}
	
      Node * ttgt;
      Node ** ttgt_ptr = table.search(gtgt);
      if (ttgt_ptr)
	ttgt = *ttgt_ptr;
      else
	{
	  ttgt = ret.insert_node(gtgt->get_info());
	  table.insert(gtgt, ttgt);
	  NODE_COOKIE(ttgt) = gtgt;
	}

      Arc * ta = ret.insert_arc(tsrc, ttgt);
      *ta = *ga;
      ARC_COOKIE(ta) = ga;
    });

  return ret;
}

    template <class GT> inline static 
long & df(typename GT::Node * p)
{
  return NODE_COUNTER(p);
}

    template <class GT> inline static 
long & low(typename GT::Node * p)
{
  return reinterpret_cast<long&>(NODE_COOKIE(p));
}

    template <class GT> 
struct Init_Low
{
  void operator () (GT & g, typename GT::Node * node) 
  {
    g.reset_counter(node); /* Inicializes df */
    low <GT> (node) = -1;  /* Inicializes low */
  }
};

      template <class GT, class SA> inline static
void __compute_cut_nodes(GT & g, DynList<typename GT::Node *> & list, 
			 typename GT::Node * p, typename GT::Arc * a,
			 long & curr_df);

/** 
  @brief Calculate the cut points of a graph.

  details compute_cut_nodes () takes a connected graph g, makes a tour
  depth from a start node and added to a list
  dynamic cutting list each node that contains the graph.

  Cutting a node is defined as a node such that the delete
  (together with its arches) the graph becomes disjointed.

  The algorithm uses the bit depth_first to mark the nodes and
  arches that have been visited.

  The result of this function can be combined with
  paint_subgraphs () to paint the various blocks
  would separate cutoffs. In turn, block mapping and
  nodes and arcs of the graph cut can be obtained
  by map_subgraph calls () and map_cut_graph () on a
  graph previously painted.

  It is assumed that G is connected and no check is made to
  respect. 

  @param [in] g Graph to calculate cutoffs.
  @param [in] start Node origin where start the tour in depth.
  @param [out] list dynamic list where pointers are saved
  Cutting nodes of the graph.

  @throw bad_alloc If no memory to insert into the dynamic list
  or if no memory for the data of internal calculation algorithm.

  @see paint_subgraphs() map_subgraph() map_cut_graph()

  @ingroup Grafos
*/
      template <class GT, class SA = Dft_Show_Arc<GT>>
void compute_cut_nodes(GT & g, typename GT::Node * start,
		       DynList<typename GT::Node *> & list)
{
  Operate_On_Nodes <GT, Init_Low <GT>> () (g);
  g.reset_arcs();
  long current_df = 0; /* Counter Global of Visits  */
  NODE_BITS(start).set_bit(Depth_First, true); /* Mark start */
  df <GT> (start) = current_df++;
  int call_counter = 0; /* Counter recursive calls */
  
    /** Scroll through the arches start while g has not been covered */
  for (Node_Arc_Iterator<GT, SA> i(start); 
       i.has_current() and current_df < g.get_num_nodes(); i.next())
    {
      typename GT::Node * tgt = i.get_tgt_node();
      if (IS_NODE_VISITED(tgt, Depth_First)) 
        continue; 

      typename GT::Arc * arc = i.get_current_arc();
      if (IS_ARC_VISITED(arc, Depth_First)) 
        continue;

      ARC_BITS(arc).set_bit(Depth_First, true);
      __compute_cut_nodes <GT, SA> (g, list, tgt, arc, current_df); 
      ++call_counter;
    }

    /* Is root an articulation point? */
    if (call_counter > 1) 
      {
        NODE_BITS(start).set_bit(Cut, true);
        list.append(start);
      }
  }
      
      template <class GT, class SA> inline static
void __compute_cut_nodes(GT & g, DynList<typename GT::Node *> & list, 
			 typename GT::Node * p, typename GT::Arc * a, 
			 long & curr_df)
{
  NODE_BITS(p).set_bit(Depth_First, true); /* Paint p visited */
  low <GT> (p) = df <GT> (p) = curr_df++;  /* Assign df */

    
  bool p_is_cut_node = false;
  
    /** Recorrer arcos de p mientras no se abarque a g */
  for (Node_Arc_Iterator <GT, SA> i(p); i.has_current(); i.next())
    {
      typename GT::Arc * arc = i.get_current_arc();
      if (arc == a) 
	continue; /* a es el padre de arc, entonces ignorarlo */

      typename GT::Node * tgt = i.get_tgt_node();
      if (IS_NODE_VISITED(tgt, Depth_First)) 
	{ 
	  if (not IS_ARC_VISITED(arc, Depth_First)) /* No abarcador? */
	    if (df<GT>(tgt) < low<GT>(p)) /* Sí, verificar valor low */
	      low<GT>(p) = df<GT>(tgt); /* Update low(p) */

	  continue;
	}

      if (IS_ARC_VISITED(arc, Depth_First)) 
	continue;

      ARC_BITS(arc).set_bit(Depth_First, true); /* Mark arc */

      __compute_cut_nodes<GT, SA>(g, list, tgt, arc, curr_df);

      if (low<GT>(tgt) < low<GT>(p)) 
	low<GT>(p) = low<GT>(tgt); /* Update low(p) */

      if (low<GT>(tgt) >= df<GT>(p) and df<GT>(tgt) != 0) /* ¿De corte? */
	p_is_cut_node = true;
    }

    /* Here, p was explored recursively */
  if (p_is_cut_node)
    {
      NODE_BITS(p).set_bit(Cut, true);
      list.append(p);
    }
}

const long Cross_Arc = -1;

    template <class GT> inline static 
bool is_a_cross_arc(typename GT::Arc * a) 
{
  return ARC_COUNTER(a) == Cross_Arc; 
}

     template <class GT> inline static 
bool is_a_cut_node(typename GT::Node * p)
{
  return NODE_BITS(p).get_bit(Cut);
}

      template <class GT> inline static 
bool is_an_cut_arc(typename GT::Arc * a)
{
  return ARC_BITS(a).get_bit(Cut);
}
      template <class GT> inline static 
bool is_node_painted(typename GT::Node * p)
{
  return NODE_COUNTER(p) > 0;
}

      template <class GT> inline static 
bool is_arc_painted(typename GT::Arc * arc)
{
  return ARC_COUNTER(arc) > 0;
}

      template <class GT> inline static 
void paint_node(typename GT::Node * p, const long & color)
{
  NODE_COUNTER(p) = color;
}

      template <class GT> inline static 
void paint_arc(typename GT::Arc * a, const long & color)
{
  ARC_COUNTER(a) = color;
}

      template <class GT> inline static 
const long & get_color(typename GT::Node * p)
{
  return NODE_COUNTER(p);
}

      template <class GT> inline static 
const long & get_color(typename GT::Arc * a)
{
  return ARC_COUNTER(a);
}

      template <class GT, class SA> inline static
void __paint_subgraph(GT &                g,
		      typename GT::Node * p, /* Current Node */
		      typename GT::Arc *  a, /* Archway leading to p */
		      const long &        current_color);

      template <class GT, class SA> inline static
void __paint_subgraph(GT & g, typename GT::Node * p, const long & current_color)
{
  assert(not is_a_cut_node <GT> (p));

  if (is_node_painted <GT> (p)) 
    return; 

  paint_node <GT> (p, current_color);

  for (Node_Arc_Iterator<GT, SA> it(p); it.has_current(); it.next())
    {
      typename GT::Arc * arc = it.get_current_arc();
      if (is_arc_painted <GT> (arc))
          continue;

      typename GT::Node * tgt = it.get_tgt_node();
      if (is_a_cut_node <GT> (tgt))
	continue;

      paint_arc <GT> (arc, current_color); 

      __paint_subgraph <GT, SA> (g, tgt, current_color); 
    }
}

    template <class GT, class SA> inline static
void __paint_from_cut_node(GT & g, typename GT::Node * p, long & current_color)
{
  assert(is_a_cut_node <GT> (p));

    /** Diff recursively with paint colors blocks connected to p */
  for (Node_Arc_Iterator<GT, SA> it(p); it.has_current(); it.next())
    {
      typename GT::Arc * arc = it.get_current_arc();

      assert(not is_arc_painted <GT> (arc));

      typename GT::Node * tgt_node = it.get_tgt_node();
      if (is_a_cut_node <GT> (tgt_node)) /* Is it a cutting arc?*/
	{
	  ARC_BITS(arc).set_bit(Cut, true); /* Mark as cutting */
	  continue; /* Advance to next arc */
	}
      else 
	{
	  paint_arc <GT> (arc, Cross_Arc); /* Mark as crossing */
	  if (is_node_painted <GT> (tgt_node)) 
	    continue; 
	}

    /* Paint recursively node connected to arc */
      __paint_subgraph <GT, SA> (g, tgt_node, current_color);

      current_color++; /* Change Color (sig bow in another block) */

      assert(not is_arc_painted <GT> (arc));
    }
}

/** 
  @brief Paints a connected graph nodes according to their court.

  @details paint_subgraphs () takes a list of nodes court
  cut_node_list on the graph g, previously calculated
  compute_cut_nodes (), and conducts tours in depth from
  each node cutting blocks by painting the counter
  present in each node and arc.
      
  The color is defined as the value of the counter node and
  arc. The colors begin from the value 1 and will
  many colors as the number of blocks separated by
  breakpoints of the graph. The colors of all nodes and
  graph arcs are reset to zero before running
  algorithm.

  No checks on content are made
  cut_node_list and connectivity graph.

  paint_subgraphs () uses the bit to distinguish cut nodes and
  cutting arches. Similarly, to identify nodes and arcs
  and mapped the process used to mark the build_subtree bit.

  Cutting nodes must have previously calculated
  compute_cut_nodes by ().

  Typically, once the graph painted, their blocks
  They can be distinguished by colors and this can be
  enough for many algorithms; the planarity or
  drawn, for example. If more security is desired, in
  expense of a little time and space, then they can
  copying the blocks mapped accomplished by
  map_subgraph () specifying a color. Similarly, the
  cutting subgraph (nodes and arcs cutting) can
  map_cut_graph obtained by () and arches
  intersection (adjacent arcs to a cutoff).

  If the protocol is respected, then paint_subgraphs () no
   must fail, since no memory away.

  @param [in] g Graph to paint.
  @param [in] cut_node_list list of pointers to nodes
  previously calculated (ideally cut by
  compute_cut_node ()).

  @return Total colors used to paint the blocks;
  this is equivalent to the total number of blocks having
  the graph as the delimitation of the breakpoints.

  @see compute_cut_nodes() map_subgraph()  map_cut_graph()
 
  @ingroup Grafos
*/
    template <class GT, class SA = Dft_Show_Arc<GT>> inline long 
paint_subgraphs(GT & g, const DynList<typename GT::Node*> & cut_node_list)
{
  g.reset_counter_nodes();
  g.reset_counter_arcs();
  long current_color = 1;

    /* Cutting through each node and paint their blocks */
  for (typename DynList<typename GT::Node*>::Iterator 
	 i(cut_node_list); i.has_current(); i.next())
    __paint_from_cut_node<GT,SA>(g, i.get_current(), current_color);

  return current_color;
}

         

       template <class GT, class SA> inline static
void __map_subgraph(GT & g, GT & sg, typename GT::Node * gsrc, 
		    const long & color)
{
  assert(get_color <GT> (gsrc) == color);

  typename GT::Node * tsrc = mapped_node<GT>(gsrc); /* gsrc en sg */

    /** Browse gsrc arches and add to the color sg interest */
  for (Node_Arc_Iterator <GT, SA> i(gsrc); i.has_current(); i.next())
    {
      typename GT::Arc * garc = i.get_current_arc();
      if (get_color<GT>(garc) != color or IS_ARC_VISITED(garc, Build_Subtree))
	continue; /* Arco is a different color or already visited */

      ARC_BITS(garc).set_bit(Build_Subtree, true); 

      typename GT::Node * gtgt = i.get_tgt_node(); 

      assert(get_color <GT> (gtgt) == color);

      typename GT::Node * ttgt = NULL; /* Image gtgt on sg */
      if (IS_NODE_VISITED(gtgt, Build_Subtree)) /* gtgt on sg? */
	ttgt = mapped_node<GT> (gtgt);
      else
	{     
    /* gtgt is not in sg ==> copy and map */
	  unique_ptr<typename GT::Node> ttgt_auto(new typename GT::Node(gtgt));
	  sg.insert_node(ttgt_auto.get());
	  GT::map_nodes(gtgt, ttgt_auto.get());
	  NODE_BITS(gtgt).set_bit(Build_Subtree, true); 
	  ttgt = ttgt_auto.release(); 
	}

      typename GT::Arc * tarc = sg.insert_arc(tsrc, ttgt, garc->get_info());
      GT::map_arcs(garc, tarc);

      __map_subgraph<GT, SA> (g, sg, gtgt, color);
    }
}

/** 
  @brief Copy a subgraph mapped according to a color.
          
  @details map_subgraph () takes a graph G, a color (an integer to compare
  with meter arc-node) and travels deep into
  search for the nodes and arcs whose color matches the given
  in parameter. The nodes and arcs with the color in question are
  copied and mapped in the subgraph sg.

  map_subgraph () does not necessarily walking in all the graph,
  because it does not explore in depth the nodes with different color
  in the given parameter or delves arches with another
  color. In fact, the routine assumes that color corresponds to
  a connected subgraph g.

  map_subgraph () is intended to extract a subgraph block
  of a cut previously painted by
  paint_subgraphs (). However eventually map_subgraph ()
  It can be used independently of the breakpoints. Enough
  with nodes and arcs are painted and form a component
  related to the function to operate and remove the subgraph.
  @param [in] g Graph on Which the subgraph extract specific color.
  @param [out] sg Graph where to place the copy of the block by color;
  This graph is cleared before beginning the procedure.
  @param [in] Color to be extracted.

  @throw bad_alloc If no memory to construct the subgraph sg;
  In this case, sg is empty, regardless of the state
  that the exception has occurred.

  @see paint_subgraphs() compute_cut_nodes() map_cut_graph()

  @ingroup Grafos
*/
      template <class GT, class SA = Dft_Show_Arc<GT>>
void map_subgraph(GT & g, GT & sg, const long & color)
{
  clear_graph(sg);
  typename GT::Node * first = NULL; /* Busque primer nodo con color */
  for (typename GT::Node_Iterator it(g); it.has_current(); it.next())
    if (get_color <GT> (it.get_current_node()) == color)
      first = it.get_current_node();

  if (first == NULL) /* Color found? */
    throw std::domain_error("Color does not exist in the graph");

    /* Create first, put in sg and map */
  unique_ptr<typename GT::Node> auto_tsrc(new typename GT::Node(first));
  sg.insert_node(auto_tsrc.get());
  GT::map_nodes(first, auto_tsrc.release());
  NODE_BITS(first).set_bit(Build_Subtree, true);

  try
    {    
      __map_subgraph <GT, SA> (g, sg, first, color); /* Mapee first */
    }
  catch (...)
    {
      clear_graph(sg);
    }
}


/** 
  @brief Makes a copy of the graph mapped section of a graph.

  @details map_cut_graph () takes a graph and list of nodes -the court
  which it should have been previously calculated by
  compute_cut_nodes () - and makes a copy of the graph mapped
  cut.

  Cutting the graph it is defined by one consisting of nodes and
  cutting arches. A cutting arc is defined as an arc
  two nodes related to cutting.

  The arches that involve cutting a node to another node part
  a block are called "arcs crossing". By their nature, these
  Bows can be neither in blocks nor in graph
  cut. To deal with this situation, the fourth parameter,
  called cross_arc_list keeps a list of pointers to the arches
  crossing.

  The procedure requires that the nodes and arcs are marked by
  their corresponding bits, and they are also
  painted. This requires pre compute_cut_nodes calls ()
  and then paint_subgraphs (). The results are indeterminate
  If this protocol is not followed.


  @param [in] g Graph to which you want to copy and map their
  Cutting nodes and arcs and arcs crossing.
  @param [in] cut_node_list list with nodes which cut
  It should be the result of the call
  compute_cut_nodes ().
  @param [out] cut_graph Graph court where the graph is copied; It is
  mean graph composed of nodes and arcs cutting. east
  graph is often disjointed.
  @param [out] pointers cross_arc_list List arches
  crossing; that is, the arcs involving cutting a node and
  to a node that is part of a block.

  @throw bad_alloc If no memory to build cut_graph or
  cross_arc_list inserted into the list.

  @see map_subgraph() compute_cut_nodes() paint_subgraphs()

  @ingroup Grafos
*/
      template <class GT, class SA = Dft_Show_Arc<GT>>
void map_cut_graph(GT & g, DynList<typename GT::Node*> & cut_node_list,
                   GT & cut_graph, 
                   DynList<typename GT::Arc*> &  cross_arc_list)
{
  clear_graph(cut_graph);

    /** Recorra lista de nodos de corte e insértelos en cut_graph */
  for (typename DynList<typename GT::Node*>::Iterator 
	 it(cut_node_list); it.has_curr(); it.next())
    {
      typename GT::Node * gp = it.get_current();

      assert(is_a_cut_node <GT> (gp));

      unique_ptr<typename GT::Node> tp_auto(new typename GT::Node(gp));
      cut_graph.insert_node(tp_auto.get());
      GT::map_nodes(gp, tp_auto.release());
    }

    /* Srcroll arches de g ==> cut_graph 
    = {not cut arches} U cross_arc_list 
    = {Arches corssing} */
  for (Arc_Iterator <GT, SA> it(g); it.has_current(); it.next())
    {
      typename GT::Arc * garc = it.get_current_arc();
      if (is_a_cross_arc <GT> (garc))
	{
	  cross_arc_list.append(garc); 
	  continue;
	}

      if (not is_an_cut_arc <GT> (garc)) 
	continue;

      typename GT::Node * src = mapped_node<GT>(g.get_src_node(garc));
      typename GT::Node * tgt = mapped_node<GT>(g.get_tgt_node(garc));

      assert(src != NULL and tgt != NULL);

      typename GT::Arc * arc = cut_graph.insert_arc(src, tgt, garc->get_info());
      GT::map_arcs(garc, arc);
    }
}


/** 
  @brief comparison Warpper distances or weights of two arcs.

  @details algorithms used by Dijkstra and Prim in combination with a Heap arches.

  @ingroup Grafos
*/
    template <class GT, class Distance> 
struct Distance_Compare
{
  Distance & dist;

  Distance_Compare(Distance && __dist = Distance())
    : dist(__dist)
  {
    /* Empty */
  }

  Distance_Compare(Distance & __dist)
    : dist(__dist)
  {
    /* Empty */
  }

  bool operator () (typename GT::Arc * a1, typename GT::Arc * a2)
  {
    return dist(a1) < dist(a2);
  }
};

/** 
  @brief Returns true if there is tgt node is reachable from src.

  @details is_reachable () explores in depth the graph from node ga
  src in finding a path that derive in tgt. Yes
  during scanning is to tgt, then it follows
  there is a way and returns true. If it runs
  ag entirely without seeing tgt, then the function returns
  false.

  Test_path bit is used to mark the nodes and arcs
  visited during the search. in fact, this routine underlies
  on Test_For_Path.

  @param[in] g Graph searching road.
  @param[in] src Pointer to the origin node of the road.
  @param[in] tgt Pointer to destination node of road.

  @return true if there is a path between src and tgt.

  @see Find_Path_Depth_First Find_Path_Breadth_First Test_For_Path Test_Connectivity 

  @ingroup Grafos

  @throw domain_error If g is a graph (not a digraph)
*/
      template <class GT, class SA = Dft_Show_Arc<GT>>
bool is_reachable(GT & g, typename GT::Node * src, typename GT::Node * tgt)
{
  if (not g.is_digraph())
    throw std::domain_error("g is not a digraph");

  return test_for_path <GT, SA> (g, src, tgt);
}

/** 
  @brief Returns true if there is tgt node is reachable from src.

  @details Is_Reachable explores in depth a graph from one node
  origin in search of a way that derive a destination node. Yes
  during scanning is the destination node, then it is concluded
  there is a way and returns true. If it runs
  ag entirely without seeing the destination node, then the function returns
  false.

  The test_path bit is used to mark the nodes and arcs
  visited during the search.

  @see find_path_depth_first() find_path_breadth_first() test_for_path() test_connectivity() 

  @ingroup Grafos
*/
      template <class GT, class SA = Dft_Show_Arc<GT>>
  class Is_Reachable
  { 
  public:

/** 
  @brief Invokes the reachability test.

  @param[in] g Graph searching road.
  @param[in] src Pointer to the origin node of the road.
  @param[in] tgt Pointer to destination node of road.

  @return true if there is a path between src and tgt.

  @throw domain_error If g is a graph (not a digraph)
*/
    bool operator () (GT & g, typename GT::Node * src, 
                      typename GT::Node * tgt) const
    {
      return is_reachable<GT, SA> (g, src, tgt);
    }
  };

/** 
  @brief Calculates the inversebrief digraph and maps.

  @details details invert_digraph (sg, gi) calculates the inverse of the digraph digraph
  sg. The result is placed in the digraph gi, which is
  cleaned at the start of the algorithm.

  Sg reverse digraph is one with the same nodes but their
  directed arcs are reversed.

  After the reverse call graph is mapped to the original.

  @param[in] g Digraph to invest.
  @param[in] gi Digraph invested of sg.

  @throw bad_alloc If no memory to create a gi.
  @throw domain_error If sg and gi are not digraphs.

  @ingroup Grafos
*/
      template <class GT, class SA = Dft_Show_Arc<GT>> 
void invert_digraph(GT & g, GT & gi, SA && sa = SA())
{
  if (not g.is_digraph())
    throw std::domain_error("g is not a digraph");

  if (not gi.is_digraph())
    throw std::domain_error("gi is not a digraph");

  clear_graph(gi);
  g.reset_nodes();

    /** Browse all arcs of digraph g */
  for (Arc_Iterator<GT, SA> it(g, sa); it.has_current(); it.next())
    {
      typename GT::Arc * arc   = it.get_current();

    /* Process origin node */
      typename GT::Node * ssrc = g.get_src_node(arc); 
      typename GT::Node * rsrc = mapped_node<GT> (ssrc);

    /** Is already created ssrc in gi?
    If the answer is NO, I create, insert and mapping */
      if (rsrc == NULL) 
	{     
	  unique_ptr<typename GT::Node> rsrc_auto(new typename GT::Node(ssrc));
	  gi.insert_node(rsrc_auto.get());
	  GT::map_nodes(ssrc, rsrc_auto.get());
	  rsrc = rsrc_auto.release(); 
	}

    /* Process origin node */
      typename GT::Node * stgt = g.get_tgt_node(arc);
      typename GT::Node * rtgt = mapped_node<GT> (stgt);

    /** Is already created ssrc in gi?
    If the answer is NO, I create, insert and mapping */
      if (rtgt == NULL) 
	{     
	  unique_ptr<typename GT::Node> rtgt_auto(new typename GT::Node(stgt));
	  gi.insert_node(rtgt_auto.get());
	  GT::map_nodes(stgt, rtgt_auto.get());
	  rtgt = rtgt_auto.release(); 
	}

      typename GT::Arc * ai = gi.insert_arc(rtgt, rsrc, arc->get_info());
      GT::map_arcs(arc, ai);
    }

  assert(g.get_num_arcs() == gi.get_num_arcs() and 
    g.get_num_nodes() == gi.get_num_nodes());
}


/** 
  @brief Calculates the inverse digraph and maps.

  @ingroup Grafos
*/
      template <class GT, class SA = Dft_Show_Arc<GT>>
class Invert_Digraph
{
  SA & sa;

public:

  Invert_Digraph(SA && __sa = SA()) : sa(__sa) { /* Empty */ }

  Invert_Digraph(SA & __sa) : sa(__sa) { /* Empty */ }
  

/** 
  @brief Iinvokes the inverse calculation digraph

  @param[in] g Digraph to invest.
  @param[in] gi Digraph invested of g.
  
  @throw bad_alloc If no memory to create a gi.
  @throw domain_error If g and gi are not digraphs.
*/
  void operator () (GT & g, GT & gi) const
  {
    invert_digraph <GT, SA> (g, gi, sa);
  }
};

/** 
  @brief Class default remote access.

  @details assumes that the distance or weight of the bow is the value obtained by
  get_info () on the arc.

  @ingroup Grafos
*/
    template <class GT>
class Dft_Dist
{
public:

  typedef typename GT::Arc_Type Distance_Type;

  static const Distance_Type Zero_Distance;

  static const Distance_Type Max_Distance;

  Distance_Type & operator () (typename GT::Arc * a) const
  {
    return a->get_info();
  }

  Distance_Type & operator () (typename GT::Arc * a, typename GT::Node*) const
  {
    return a->get_info();
  }

  static void set_zero(typename GT::Arc * a) { a->get_info() = 0; }
};

template <class GT>
const typename Dft_Dist<GT>::Distance_Type Dft_Dist<GT>::Max_Distance =
  std::numeric_limits<typename Dft_Dist<GT>::Distance_Type>::max();

template <class GT>
const typename Dft_Dist<GT>::Distance_Type Dft_Dist<GT>::Zero_Distance = 0.0;

template <class GT, class Distance = Dft_Dist<GT>>
typename Distance::Distance_Type 
get_min_path(typename GT::Node * s, typename GT::Node * end, 
	     Path<GT> & path)
{
  typename Distance::Distance_Type dist = 0.0;
  path.clear_path();
  path.insert(end);

  typename GT::Node * p = end;
  do
    {
      p = (typename GT::Node *) NODE_COOKIE(p);
      path.insert(p);
      dist = dist + Distance() (path.get_first_arc());
    }
  while (p != s);

  return dist;
}

/**
  @brief Calculate the total cost of all the arcs of the graph.

  @details generally used to calculate costs trees
  encompassing.

  @ingroup Grafos
*/
template <class GT, 
	  class Distance = Dft_Dist<GT>, 
	  class SA       = Dft_Show_Arc<GT>>
class Total_Cost
{
  Distance & dist;
  SA &       sa;

public:

  Total_Cost(Distance && __dist = Distance(), SA && __sa = SA()) 
    : dist(__dist), sa(__sa)
  {
    /* Empty */
  }

  Total_Cost(Distance & __dist, SA & __sa) : dist(__dist), sa(__sa)
  {
    /* Empty */
  }

/** 
  @brief Invokes the calculation of the total cost 
*/
  typename Distance::Distance_Type total_cost(GT & g)
  {
    typename Distance::Distance_Type sum = 0;
  
    /* Browse all arcs and add its weight */
    for (Arc_Iterator <GT, SA> it(g, sa); it.has_curr(); it.next())
      sum += dist(it.get_current_arc());

    return sum;
  }

/** 
  @overload total_cost 
*/
  typename Distance::Distance_Type operator () (GT & g)
  {
    return total_cost (g);
  }

  bool operator () (typename GT::Arc * a)
  {
    if (not sa(a))
      return false;

    dist += dist(a);
    return true;
  }
};



} /* End namespace Aleph */

# endif /* TPL_GRAPH_UTILS_H
