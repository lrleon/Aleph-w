
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
  @file Dijkstra.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef DIJKSTRA_H
# define DIJKSTRA_H

# include <ahFunction.H>
# include <archeap.H>
# include <tpl_find_path.H>
# include <tpl_agraph.H>

namespace Aleph {

/** 
  @brief Conversion cookie to Node_Info 
*/
# define DNassert(p) ((Node_Info*) NODE_COOKIE((p)))

/** 
  @brief Access the tree node in the graph. 
*/
# define TREENODE(p) (((Tree_Node_Info*)DNassert(p))->tree_node)

/** 
  @brief Access to accumulated distance. 
*/
# define ACC(p) (DNassert(p)->dist) 
# define HEAPNODE(p) (DNassert(p)->heap_node)
# define PARENT(p) (DNassert(p)->ret_node)

# define DAassert(p) ((Arc_Info*) ARC_COOKIE(p))
# define ARC_DIST(p) (Distance () (p))
# define TREEARC(p) (((Tree_Arc_Info*)DAassert(p))->tree_arc)
# define POT(p) (DAassert(p)->pot)
# define GRAPHNODE(p) (static_cast<typename GT::Node*>(NODE_COOKIE(p)))


/** 
  @brief spanning tree calculation of all shortest paths from
  according to a given node Dijkstra's algorithm.

  @details This class handles Dijkstra's algorithm to compute a tree
  inclusive of all shortest path from a node s of a graph g.

  The algorithm uses an internal queue whose length is
  proportionate number of nodes in the graph.

  Dijkstra's algorithm is one that exhibits improved performance and is
  both recommended for most applications roads
  minimum.

  Dijkstra's algorithm does not work for graphs with negative weights.

  The class type receives the following parameters:
  - # GT: Graph type based on List_Graph
  - # Distance <GT>: Read the weight class should bow
     export the following attributes:
     - # Typedef Distance <GT> :: Distance_Type: Data type
        It represents a weight on an arc
     - # Distance <GT> Distance_Type :: operator () (typename GT :: Arc * a):
        that returns the value of the peso in the bow
    .
  - # SA: Arches filter for internal iterator
 .

  @see Find_Path_Depth_First Floyd_All_Shortest_Paths Bellman_Ford_Min_Spanning_Tree 

  @ingroup Grafos
*/
  template <class GT, 
	    class Distance = Dft_Dist<GT>,
	    template <typename, class> class Itor = Node_Arc_Iterator,
	    class SA       = Dft_Show_Arc<GT>>
class Dijkstra_Min_Paths
{
/** 
  @note Although the algorithm is basically the same , there are two approaches
  to calculate the shortest path. The first is to paint the tree
  inclusive of all shortest paths from one node
  start. Means for painting the solution is within
  the same graph and can be distinguished by brands.
      
  The other approach is to build a separate spanning tree.
      
  According to one approach or another , the graph is initialized with information
  different. Thus, classes that begin with the "Tree " prefix are
  classes regarding the solucipon building a spanning tree
  separated.
*/ 

/** 
  @brief Information to be placed in the bow for version painting. 
*/
  struct Arc_Info
  {
    typename Distance::Distance_Type pot; /** Potential arch. */
  };

/** 
  @brief information to be placed on the bow to the version that builds tree. 
*/
  struct Tree_Arc_Info : public Arc_Info
  {
    typename GT::Arc * tree_arc; /** Image in the tree. */
    typename Distance::Distance_Type pot; /** Potential arch. */
  };

/** 
  @brief Wrapper distance access
  (Distance parameter by class) 
*/
  

   typename Distance::Distance_Type operator () (typename GT::Arc *a)
    {
      Arc_Info * arc_info = (Arc_Info*) ARC_COOKIE(a);
      return arc_info->pot;
    }
  };

/** 
  @brief Put information on the node to version painting. 
*/
  struct Node_Info
  {
    typename Distance::Distance_Type dist;      /** Cumalative distance. */
    void *                           heap_node;
    void *                           ret_node;  /* Father in tree */

    Node_Info() : dist(0), heap_node(NULL), ret_node(NULL)
    { 
      /* Empty */
    }
  };

/** 
  @brief Put information in the node version
  building tree. 
*/
  struct Tree_Node_Info : public Node_Info
  {
    typename GT::Node *              tree_node; /** Node comprehensive tree. */
    typename Distance::Distance_Type dist;      /** Cumulative distance. */
    void *                           heap_node;

    Tree_Node_Info() : tree_node(NULL)
    { 
      /* Empty */
    }
  };

/** 
  @brief Acces to the heap of arch 
*/
  struct Dijkstra_Heap_Info
  {
        typedef typename 
    ArcHeap<GT, Get_Potential_Arc, Dijkstra_Heap_Info>::Node Node;

    Node *& operator () (typename GT::Node * p) 
    {
      return (Node*&) HEAPNODE(p); 
    }
  }; 

/** 
  @brief Node initialization version painting. 
*/
  struct Initialize_Node
  {
    void operator () (GT & g, typename GT::Node * p)
    {
      g.reset_bit(p, Aleph::Spanning_Tree);
      NODE_COOKIE(p) = new Node_Info; 
    }
  };

/** 
  @brief Node memory release for version painting. 
*/
  struct Destroy_Node
  {
    void operator () (GT &, typename GT::Node * p)
    {
      void * tmp = PARENT(p);
      delete DNassert(p); /* block release */
      NODE_COOKIE(p) = tmp;
    }
  };

/** 
  @brief Initialization bow for version painting. 
*/
  struct Initialize_Arc
  {
    void operator () (GT & g, typename GT::Arc * a)
    {
      g.reset_bit(a, Aleph::Spanning_Tree);
      ARC_COOKIE(a) = new Arc_Info;
      POT(a) = 0;
    }
  };

/** 
  @brief Arc freeing memory for version painting. 
*/
  struct Destroy_Arc
  {
    void operator () (GT &, typename GT::Arc * ga)
    {
      delete DAassert(ga); 
    }
  };

/** 
  @brief Memory initialization node for
  Building tree version. 
*/
  struct Initialize_Tree_Node
  {
    void operator () (GT & g, typename GT::Node * p)
    {
      g.reset_bit(p, Aleph::Spanning_Tree);
      NODE_COOKIE(p) = new Tree_Node_Info; 
    }
  };

/** 
  @brief Release memory node mapping for
  Building tree version. 
*/
  struct Destroy_Tree_Node
  {
    void operator () (GT &, typename GT::Node * p)
    {
      Tree_Node_Info * aux = (Tree_Node_Info *) DNassert(p); /* Block release */
      typename GT::Node * tp = TREENODE(p); /** Image spanning tree. */
      if (tp != NULL) /* It is this node included in the spanning tree? */
	{
	  NODE_COOKIE(p) = NODE_COOKIE(tp) = NULL;
	  GT::map_nodes (p, tp);
	}
      else
	NODE_COOKIE(p) = NULL;

      delete aux; 
    }
  };

/** 
  @brief Initialization arc for the version built tree
*/
struct Initialize_Tree_Arc
  {
    void operator () (GT & g, typename GT::Arc * a)
    {
      g.reset_bit(a, Aleph::Spanning_Tree);
      ARC_COOKIE(a) = new Tree_Arc_Info;
      POT(a) = 0;
      TREEARC(a) = NULL;
    }
  };

/** 
  @brief Freeing memory and mapping for the version built tree 
*/
  struct Destroy_Tree_Arc
  {
    void operator () (GT &, typename GT::Arc * ga)
    {
      Tree_Arc_Info * aux = (Tree_Arc_Info *) DAassert(ga); 
      typename GT::Arc * ta = TREEARC(ga);
      if (ta != NULL) /* This arc belongs to the spanning tree ? */
	{    
	  assert(IS_ARC_VISITED(ga, Aleph::Spanning_Tree));
	  GT::map_arcs (ga, ta);
	}

      delete aux;
    }
  };

  typedef Dijkstra_Heap_Info Heap_Info;

  typedef ArcHeap<GT, Get_Potential_Arc, Heap_Info> Heap;

  SA &                sa;
  Get_Potential_Arc   get_pot;
  Heap                heap;
  bool                painted;
  GT *                ptr_g;
  typename GT::Node * s;

public:

    /* Constructores */
  
/** 
  @brief Builder

  @param [in] dist Access arc distance.
  @param [in] cmp Comparison between the distances of arches.
  @param [in] __sa Filter bows for iterators.
*/
  Dijkstra_Min_Paths(Distance && dist = Distance(),  
		     SA &&       __sa = SA())
    : sa(__sa), get_pot(dist), heap(get_pot, Heap_Info()), 
      painted(false), ptr_g(NULL), s(NULL)
  {
    /* Empty */
  }

/** 
  @overload Dijkstra_Min_Paths 
*/
  Dijkstra_Min_Paths(Distance & dist, SA & __sa)
    : sa(__sa), get_pot(dist), heap(get_pot, Heap_Info()), 
      painted(false), ptr_g(NULL), s(NULL)
  {
    /* Empty */
  }

private:

      template <class IN, class IA>
  void init(GT & g, typename GT::Node * start)
  {
    heap.empty();

    ptr_g = &g;
    s     = start;

    Operate_On_Nodes<GT, IN> () (g);

    (Operate_On_Arcs <GT, IA> (sa)) (g);
  }

     template <class DN, class DA>
  void uninit()
  {
    Operate_On_Nodes <GT, DN> () (*ptr_g);

    (Operate_On_Arcs <GT, DA, SA> (sa)) (*ptr_g);
  }

public:

/** 
  @brief Calculates the spanning tree of all shortest path departing
  from the start node.

  @param [in] g Graph.
  @param [in] start Node start all minimum paths
  @param [out] tree Tree encompassing all shortest path
  beginning with start mapping using cookies to graph g.

  @throw bad_alloc If there is insufficient memory
*/
  void compute_min_paths_tree(GT & g, typename GT::Node * start, GT & tree)
  {
    init<Initialize_Tree_Node, Initialize_Tree_Arc>(g, start);

    clear_graph(tree); /* Clean comprehensive destination tree*/

    NODE_BITS(start).set_bit(Aleph::Spanning_Tree, true); 
    ACC(start)                   = 0; 
    TREENODE(start)              = tree.insert_node(start->get_info());
    NODE_COOKIE(TREENODE(start)) = start;

    for (Itor<GT, SA> it(start, sa); it.has_curr(); it.next()) 
      {
	typename GT::Arc * arc = it.get_current_arc();
	POT(arc)               = ARC_DIST(arc);
	heap.put_arc(arc, it.get_tgt_node());
      }

    const size_t & n = g.get_num_nodes();


    /** While tree encompassing not g */
    while (tree.get_num_nodes() < n)  
      {
	typename GT::Arc * garc  = heap.get_min_arc(); 
	if (IS_ARC_VISITED(garc, Aleph::Spanning_Tree))
	  continue;

	typename GT::Node * gsrc = g.get_src_node(garc);
	typename GT::Node * gtgt = g.get_tgt_node(garc);

    /* Are the two nodes visited ? */
	if (IS_NODE_VISITED(gsrc, Aleph::Spanning_Tree) and 
	    IS_NODE_VISITED(gtgt, Aleph::Spanning_Tree))
	  continue; /* Insert arc cause cycle */

	ARC_BITS(garc).set_bit(Aleph::Spanning_Tree, true);	  

	if (IS_NODE_VISITED(gtgt, Aleph::Spanning_Tree))
	  std::swap(gsrc, gtgt);

	NODE_BITS(gtgt).set_bit(Aleph::Spanning_Tree, true);

	typename GT::Node * ttgt = tree.insert_node(gtgt->get_info());
	TREENODE(gtgt)           = ttgt;

	typename GT::Arc * tarc = /* Insert new tree arch */
	  tree.insert_arc(TREENODE(gsrc), TREENODE(gtgt), garc->get_info());
	TREEARC(garc) = tarc;

	ACC(gtgt) = ACC(gsrc) + ARC_DIST(garc); /* Total dist initial node */
	const typename Distance::Distance_Type & acc = ACC(gtgt);

	
    /** For each arc and insert calculate potential heap */ 
	for (Itor<GT, SA> it(gtgt, sa); it.has_curr(); it.next()) 
	  {
	    typename GT::Arc * arc = it.get_current_arc();
	    if (IS_ARC_VISITED(arc, Aleph::Spanning_Tree))
	      continue;

	    typename GT::Node * tgt = it.get_tgt_node();
	    if (IS_NODE_VISITED(tgt, Aleph::Spanning_Tree)) 
	      continue; /* Cause cycle == > it does not get into heap  */

	    POT(arc) = acc + ARC_DIST(arc); /* Calculate potential */
	    heap.put_arc(arc, tgt);
	  }
      } 

    uninit<Destroy_Tree_Node, Destroy_Tree_Arc> ();
  }

/** 
  @brief Calculate the spanning tree part of all minimum paths
  departing from the start node and containing at start -end road.

  @details compute_partial_min_paths_tree () Constructs a comprehensive tree
  shortest path departing from the calculation start but stops
  when is the end node.

  @param [in] g Graph.
  @param [in] start Node start all minimum paths
  @param [in] end
  @param [out] tree Tree encompassing all shortest path
  beginning with start mapping using cookies to graph g.
    
  @throw bad_alloc If there is insufficient memory
*/
  void compute_partial_min_paths_tree(GT & g, 
				      typename GT::Node * start,
				      typename GT::Node * end, 
				      GT & tree)
  {
    init <Initialize_Tree_Node, Initialize_Tree_Arc> (g, start);
    clear_graph(tree);

    NODE_BITS(start).set_bit(Aleph::Spanning_Tree, true); 
    ACC(start)                   = 0; 
    TREENODE(start)              = tree.insert_node(start->get_info());
    NODE_COOKIE(TREENODE(start)) = start;

    for (Itor<GT, SA> it(start, sa); it.has_curr(); it.next()) 
      {
	typename GT::Arc * arc = it.get_current_arc();
	POT(arc)               = ARC_DIST(arc);
	heap.put_arc(arc, it.get_tgt_node());
      }

    const size_t & n = g.get_num_nodes();

    while (tree.get_num_nodes() < n and not heap.is_empty())
      {
	typename GT::Arc * garc  = heap.get_min_arc(); 

	if (IS_ARC_VISITED(garc, Aleph::Spanning_Tree))
	  continue;

	typename GT::Node * gsrc = g.get_src_node(garc);
	typename GT::Node * gtgt = g.get_tgt_node(garc);

    /* Are the two nodes visited ? */
	if (IS_NODE_VISITED(gsrc, Aleph::Spanning_Tree) and 
	    IS_NODE_VISITED(gtgt, Aleph::Spanning_Tree))
	  continue; /* Insert arc cause cycle */

	ARC_BITS(garc).set_bit(Aleph::Spanning_Tree, true);

	if (IS_NODE_VISITED(gtgt, Aleph::Spanning_Tree))
	  std::swap(gsrc, gtgt);

	typename GT::Node * ttgt = tree.insert_node(gtgt->get_info());
	TREENODE(gtgt)           = ttgt;
	NODE_BITS(gtgt).set_bit(Aleph::Spanning_Tree, true);

	typename GT::Arc * tarc = /** Insert new tree arch*/
	  tree.insert_arc(TREENODE(gsrc), TREENODE(gtgt), garc->get_info());
	TREEARC(garc)           = tarc;

    /** Are you END_NODE in spanning tree ?
    If the answer is YES, then the shortest path
    It is already in spanning tree */
	if (gtgt == end) 
	  break; 

	ACC(gtgt) = ACC(gsrc) + ARC_DIST(garc); /* Total dist initial node */
	const typename Distance::Distance_Type & acc = ACC(gtgt);

    /** Arch and insert calculate potential heap */
	for (Itor<GT, SA> it(gtgt, sa); it.has_curr(); it.next()) 
	  {
	    typename GT::Arc * arc = it.get_current_arc();
	    if (IS_ARC_VISITED(arc, Aleph::Spanning_Tree))
	      continue;

	    typename GT::Node * tgt = it.get_tgt_node();
	    if (IS_NODE_VISITED(tgt, Aleph::Spanning_Tree)) 
	      continue; /* Cause cycle == > it does not get into heap */

	    POT(arc) = acc + ARC_DIST(arc); /* Calculate potential*/
	    heap.put_arc(arc, tgt);
	  }
      }

    uninit <Destroy_Tree_Node, Destroy_Tree_Arc> ();
  }

/** 
  @brief Paint on the graph g partial shortest path tree

  @details paint on the graph g minimum partial tree roads
  They start at start and stops when the end node is.

  @param [in] g Graph.
  @param [in] start start node shortest path.
  @param [in] end end node shortest path.
      
  @throw bad_alloc If there is insufficient memory.
*/
  bool paint_partial_min_paths_tree(GT & g, 
				    typename GT::Node * start, 
				    typename GT::Node * end)
  {
    bool ret_val = false;
    init<Initialize_Node, Initialize_Arc> (g, start);

    NODE_BITS(start).set_bit(Aleph::Spanning_Tree, true); 
    ACC(start) = 0; 

    for (Itor<GT, SA> it(start, sa); it.has_curr(); it.next()) 
      {
	typename GT::Arc * arc = it.get_current_arc();
	POT(arc)               = ARC_DIST(arc);
	heap.put_arc(arc, it.get_tgt_node());
      }

    const size_t & n = g.get_num_nodes();
    size_t tn = 1; /* Number of painted node */

    while (tn < n and not heap.is_empty()) /* Tree don't cover g */
      {
	typename GT::Arc * garc  = heap.get_min_arc(); 
	if (IS_ARC_VISITED(garc, Aleph::Spanning_Tree))
	  continue;

	typename GT::Node * src = g.get_src_node(garc);
	typename GT::Node * tgt = g.get_tgt_node(garc);

    /* Are the two visited nodes ? */
	if (IS_NODE_VISITED(src, Aleph::Spanning_Tree) and 
	    IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
	  continue; /* This arc would cause a cycle */

	ARC_BITS(garc).set_bit(Aleph::Spanning_Tree, true);

	if (IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
	  std::swap(src, tgt);

	NODE_BITS(tgt).set_bit(Aleph::Spanning_Tree, true);
	PARENT(tgt) = src; 

	++tn; /* P simulates got into the spanning tree*/

    /** It is END_NODE in spanning tree ?
    If the answer is YES, then the shortest path is already painted */
	if (tgt == end)
	  {
	    ret_val = true;
	    break;
	  }

	ACC(tgt) = ACC(src) + ARC_DIST(garc); /* Total dist initial node */

	const typename Distance::Distance_Type & acc = ACC(tgt);

    /** For each arc and insert calculate potential heap */ 
	for (Itor<GT, SA> it(tgt, sa); it.has_curr(); it.next()) 
	  {
	    typename GT::Arc * a = it.get_current_arc();
	    if (IS_ARC_VISITED(a, Aleph::Spanning_Tree))
	      continue;

	    typename GT::Node * t = it.get_tgt_node();
	    if (IS_NODE_VISITED(t, Aleph::Spanning_Tree)) 
	      continue; /* Cause cycle == > it does not get into heap */

	    POT(a) = acc + ARC_DIST(a); /* Calculate potential */
	    heap.put_arc(a, t);
	  }
      }

    uninit<Destroy_Node, Destroy_Arc> ();
    painted = true;
    
    return ret_val;
  }

/** 
  @brief Pinta on the graph g minimum tree all roads
  They begin from start.
 
  @param [in] g Graph.
  @param [in] start Start node shortest path.
  
  @throw bad_alloc If there is insufficient memory.
*/
  void paint_min_paths_tree(GT & g, typename GT::Node * start)
  {
    init<Initialize_Node, Initialize_Arc> (g, start);

    NODE_BITS(start).set_bit(Aleph::Spanning_Tree, true); 
    ACC(start) = 0; 

    for (Itor<GT, SA> it(start, sa); it.has_curr(); it.next()) 
      {
	typename GT::Arc * arc = it.get_current_arc();
	POT(arc)               = ARC_DIST(arc);
	heap.put_arc(arc, it.get_tgt_node());
      }

    const size_t & n = g.get_num_nodes();
    size_t tn = 1; /* Number of nodes painted */

    while (tn < n and not heap.is_empty()) /* Tree not cover g */
      {
	typename GT::Arc * garc  = heap.get_min_arc(); 
	if (IS_ARC_VISITED(garc, Aleph::Spanning_Tree))
	  continue;

	typename GT::Node * src = g.get_src_node(garc);
	typename GT::Node * tgt = g.get_tgt_node(garc);

    /* Are the two visited nodes ? */ 
	if (IS_NODE_VISITED(src, Aleph::Spanning_Tree) and 
	    IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
	  continue; /* This arc would cause a cycle */

	ARC_BITS(garc).set_bit(Aleph::Spanning_Tree, true);

	if (IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
	  std::swap(src, tgt);

	NODE_BITS(tgt).set_bit(Aleph::Spanning_Tree, true);
	PARENT(tgt) = src; 

	++tn; /* P simulates got into the spanning tree */

	ACC(tgt) = ACC(src) + ARC_DIST(garc); /* Total dist initial node */
	const typename Distance::Distance_Type & acc = ACC(tgt);

    /** For each arc and insert calculate potential heap */
	for (Itor<GT, SA> it(tgt, sa); it.has_curr(); it.next()) 
	  {
	    typename GT::Arc * a = it.get_current_arc();
	    if (IS_ARC_VISITED(a, Aleph::Spanning_Tree))
	      continue;

	    typename GT::Node * t = it.get_tgt_node();
	    if (IS_NODE_VISITED(t, Aleph::Spanning_Tree)) 
	      continue; /* Cause cycle == > it does not get into heap*/

	    POT(a) = acc + ARC_DIST(a); /* Calculate potential */
	    heap.put_arc(a, t);
	  }
      }

    uninit<Destroy_Node, Destroy_Arc> ();
    painted = true;
  }

/** 
  @brief Extracts a shortest path to destination end on a graph
  previously painted.

  @details get_min_path () takes a graph G on which are
  painted the (total or partial) shortest path by calling
  pre paint_min_paths_tree ( ) or paint_partial_min_paths_tree ( )
  and extracts the shortest path to saving it to end path.

  The end result is indeterminate if not contained within the
  graph ; ie previously called
  paint_partial_min_paths_tree () with a different destination node to end
  and the result contains no end.

  A previous call to paint_min_paths_tree () or
  paint_partial_min_paths_tree () stores the graph and node origin.
      
  @param [in] The way destination end node.
  @param [out] path Path where the minimum will be saved.
  
  @throw bad_alloc But enough memory.
  @throw domain_error If the graph has not previously been painted
  by paint_min_paths_tree ( ) or paint_partial_min_paths_tree ( ).
*/
      typename Distance::Distance_Type 
  get_min_path(typename GT::Node * end, Path<GT> & path)
  {
    if (ptr_g == NULL)
      throw std::domain_error("Min path has not been computed");

    if (not painted)
      throw std::domain_error("Graph has not previously painted");
      
    return Aleph::get_min_path<GT, Distance>(s, end, path);
  }

/** 
  @brief Calculate the shortest path between the start and end node of the graph painted.

  @details This version lower memory consumption and probably
  also the fastest. Note that each call figures tree
  partial shortest path from start.

  Use get_min_path () if you already calculated the spanning tree and wishes
  simply get the shortest path.

  @param [in] g Graph.
  @param [in] start Node beginning of the road.
  @param [in] end End node path.
  @param [in] min_path Minimum Way.

  @return The total cost of the path from start to end.

  @throw bad_alloc If there is insufficient memory
  @see get_min_parth
*/
      typename Distance::Distance_Type 
  find_min_path(GT & g, 
		typename GT::Node * start, typename GT::Node * end, 
		Path<GT> & min_path)
  {
    min_path.empty();
    if (paint_partial_min_paths_tree(g, start, end))
      return get_min_path(end, min_path);

    return numeric_limits<typename Distance::Distance_Type>::max(); 
  }
  
/** 
  @brief calculates the spanning tree of all shortest paths from
  according to a given node Dijkstra's algorithm.

  @details Tree comprehensive tree of all resulting shortest path
  start_node is completely mapped with g. Therefore, a
  path search in depth, by
  Find_Path_Depth_First , you find and build on tree
  any shortest path from any node start_node up.

  @param [in] g Graph to which you want to calculate the minimum spanning tree.
  @param [in] s Node start of the shortest path.
  @param [out] tree Graph where you want to save the tree
  comprehensive result of all shortest path departing
  start_node from the node. This graph is cleaned before
  start algorithm.
  
  @throw bad_alloc If there is not enough memory to build
  tree; either because tree or the internal queue fails.
  In this case the tree is indeterminate value and is not clean.
*/
  void operator () (GT & g, typename GT::Node * s, GT & tree)
  {
    compute_min_paths_tree(g, s, tree);
  }

/** 
  @brief Extract the shortest path tree (partial or total) and puts it in tree.

  @details copy_painted_min_paths_tree () takes a graph G on which are
  painted the (total or partial) shortest path by calling
  pre paint_min_paths_tree ( ) or paint_partial_min_paths_tree ( )
  and extracts a copy to the parameter tree.

  @param [in] g Graph previously painted where roads are minimal.
  @param [out] Graph tree where you want to copy the spanning tree
  
  @throw bad_alloc If there is insufficient memory.
*/
      typename Distance::Distance_Type 
  copy_painted_min_paths_tree(GT & g, GT & tree)
  {
    if (not painted)
      throw std::domain_error("Graph has not previously painted");

    using Paint_Filt = Painted_Min_Spanning_Tree<GT, Distance>;
    Paint_Filt painted;
    (Copy_Graph<GT, Dft_Show_Node<GT>, Paint_Filt> (painted)) (tree, g);

    return painted.dist;
  }

/** 
  @overload find_min_path() 
*/
  typename Distance::Distance_Type operator () (GT & g, 
						typename GT::Node * s, 
						typename GT::Node * e,
						Path<GT> & path)
  {
    return find_min_path (g, s, e, path);
  }

/** 
  @brief Clase totalizadora de distancias. 
*/
  struct Total
  {
    typename  Distance::Distance_Type dist;
    
    Total() : dist(0) { /* Empty */ }

    bool operator () (typename GT::Arc * a)
    {
      dist += ARC_DIST(a);

      return true;
    }
  };

/** 
  @brief Extracts a shortest path to destination end on a tree
  shortest path previously calculated.

  @details get_min_path () takes a tree tree, previously built by
  compute_min_paths_tree ( ) or compute_partial_min_paths_tree ( )
  and extracts the shortest path to saving it to end path.

  The end result is indeterminate if not contained within the
  tree ; ie previously called
  compute_partial_min_paths_tree () with a different destination node to end
  and the result contains no end.

  @param [in] tree Tree shortest path which it should have been
  previously calculated by compute_min_paths_tree ( ) or
  compute_partial_min_paths_tree ( ).
  @param [in] the way destination end node.
  @param [out] path path where the minimum will be saved.
  
  @throw bad_alloc  But enough memory.
  @throw domain_error If the graph has not previously been painted
  by paint_min_paths_tree ( ) or paint_partial_min_paths_tree ( ).
*/
      typename Distance::Distance_Type 
  get_min_path(GT & tree, typename GT::Node * end, Path<GT> & path)
  {
    if (ptr_g == NULL)
      throw std::domain_error("Min path has not been computed");

    typename GT::Node * ts = mapped_node<GT>(s);
    typename GT::Node * te = mapped_node<GT>(end);

    Path<GT> tree_path(tree);
    Total total;
    (Find_Path_Depth_First<GT, Itor, Total> (total)) (tree, ts, te, tree_path); 

    path.clear_path();
    path.init(s);
    typename Path<GT>::Iterator it(tree_path);
    for (it.next(); it.has_curr(); it.next())
      path.append(mapped_node<GT>(it.get_current_node()));

    return total.dist;
  }
};



# undef DNI
# undef PARENT
# undef TREENODE
# undef ACC
# undef HEAPNODE
# undef DAI
# undef ARC_DIST
# undef TREEARC
# undef POT
# undef GRAPHNODE

} /* End namespace Aleph */

# endif // DIJKSTRA_H
