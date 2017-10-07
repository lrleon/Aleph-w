
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
  @file Bellman_Ford.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef BELLMAN_FORD_H
# define BELLMAN_FORD_H

# include <tpl_dynListQueue.H>
# include <tpl_dynSetTree.H>
# include <tpl_graph_utils.H>
# include <Tarjan.H>

namespace Aleph {
# define Nassert(p) (static_cast<Bellman_Ford_Node_Info<GT, Distance>*>(NODE_COOKIE(p)))
# define IDX(p) (Nassert(p)->idx)
# define ACU(p) (Nassert(p)->acum)

template <class GT, class Distance> struct Bellman_Ford_Node_Info
{
  int                              idx;
  typename Distance::Distance_Type acum;
};


    template <class GT, 
	      class Distance                     = Dft_Dist<GT>, 
	      template <class, class> class Ait  = Arc_Iterator,
	      template <class, class> class NAit = Node_Arc_Iterator,
	      class SA                           = Dft_Show_Arc<GT>> 
class Bellman_Ford
{
  typedef typename Distance::Distance_Type Distance_Type;

  typedef typename GT::Node Node;
  typedef typename GT::Arc Arc;

  struct Sni
  {
    Distance_Type acum;
  };

  struct Ni : public Sni
  {

/** 
  @brief Index on arrangements predecessors. 
*/
    int idx; 
  };

  Distance_Type & acum(Node * p) 
  {
    return ((Sni*) NODE_COOKIE(p))->acum;
  }

  int & idx(Node * p) 
  {
    return ((Ni*) NODE_COOKIE(p))->idx;
  }

  DynArray<typename GT::Arc*>  arcs;
  const GT & g;
  const Distance_Type Inf;
  bool painted;
  Node * s;
  SA & sa;
  Distance & dist;

/** 
  @brief Initializes unused arrangements. 
*/
  void init_simple(Node * start)
  {
    typename GT::Node_Iterator it(g);
    for (int i = 0; it.has_curr(); ++i, it.next())
      {
        typename GT::Node * p = it.get_curr();
        g.reset_bit(p, Aleph::Spanning_Tree); /* Place bit in zero */
        Sni * ptr = new Sni;
        ptr->acum = Inf;
        NODE_BITS(p).set_bit(Spanning_Tree, false); 
        NODE_COOKIE(p) = ptr;
    }
    s = start;
    acum(s) = 0;
    g.reset_arcs();
  }

  void init_with_indexes(Node * start)
  {
    arcs.reserve(g.get_num_nodes());
    typename GT::Node_Iterator it(g);
    for (int i = 0; it.has_curr(); ++i, it.next())
      {
	arcs(i) = NULL;
        typename GT::Node * p = it.get_curr();
        g.reset_bit(p, Aleph::Spanning_Tree); /* Place bit in zero */
        Ni * ptr = new Ni;
        ptr->acum = Inf;
	ptr->idx = i;
        NODE_BITS(p).set_bit(Spanning_Tree, false); 
        NODE_COOKIE(p) = ptr;
    }

    s = start;
    acum(s) = 0;

    g.reset_arcs();
  }

/** 
  @brief Frees memory associated cookies. 
*/
  template <class Info_Type>
  void uninit()
  {
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next())
      {
	Node * p = it.get_curr();
	delete (Info_Type*) NODE_COOKIE(p);
	NODE_COOKIE(p) = NULL;
      }
  }

  bool check_painted_arcs()
  {
    int num_arcs = 0;
    for (Ait<GT, SA> it(g, sa); it.has_curr(); it.next())
      if (IS_ARC_VISITED(it.get_curr(), Aleph::Spanning_Tree))
	++num_arcs;

    return num_arcs == g.get_num_nodes() - 1 or num_arcs == g.get_num_nodes();
  }

public:

  Bellman_Ford(const GT & __g, Distance & d, SA & __sa) 
    : g(const_cast<GT&>(__g)), Inf(std::numeric_limits<Distance_Type>::max()), 
      painted(false), sa(__sa), dist(d)
  {
    /* Empty */
  }

  Bellman_Ford(const GT & __g, Distance && d = Distance(), SA && __sa = SA()) 
    : Bellman_Ford(__g, d, __sa)
  {
    /* Empty */
  }

  Bellman_Ford(const GT & __g, Distance && d, SA & __sa) 
    : Bellman_Ford(__g, d, __sa)
  {
    /* Empty */
  }

  Bellman_Ford(const GT & __g, Distance & d, SA && __sa) 
    : Bellman_Ford(__g, d, __sa)
  {
    /* Empty */
  }

private:
  
  void relax_arcs()
  {
    const size_t & n = g.vsize();
    for (int i = 0; i < n - 1; ++i)
      for (Ait<GT, SA> it(g, sa); it.has_curr(); it.next()) 
	{
	  typename GT::Arc * arc = it.get_curr();
	  Node * src  = g.get_src_node(arc);
	  const Distance_Type & acum_src = acum(src);
	  if (acum_src == Inf)
	    continue;
	  
	  Node * tgt = it.get_tgt_node();
	  Distance_Type w = dist(arc);
	  Distance_Type sum = acum_src + w;
	  Distance_Type & acum_tgt = acum(tgt);
	  if (sum < acum_tgt) /* Relax arc */
	    {
	      const int & index = idx(tgt);
	      arcs(index)       = arc;
	      acum_tgt          = sum;
	    }
	}
  }

  void relax_arcs(typename GT::Node * src, DynListQueue<typename GT::Node*> & q)
  {
    for (NAit<GT, SA> it(src, sa); it.has_curr(); it.next()) 
        {
          typename GT::Arc * arc = it.get_curr();
	  Node * src  = g.get_src_node(arc);
          const Distance_Type & acum_src = acum(src);
          if (acum_src == Inf)
            continue;

          Node * tgt = g.get_tgt_node(arc);
          Distance_Type w = dist(arc);
          Distance_Type sum = acum_src + w;
          Distance_Type & acum_tgt = acum(tgt);
          if (sum < acum_tgt) /* Relax arc */
            {
              const int & index = idx(tgt);
              arcs(index)       = arc;
              acum_tgt          = sum;
	      q.put(tgt);
            }
        }
  }

  void paint_tree()
  {

/** 
  @brief Paint the nodes and arc involve 
*/
    const size_t & n = g.vsize();
    for (int i = 0; i < n; ++i)
      {
	Arc * arc = arcs(i);
	if (arc == NULL)
	  continue;
	
	ARC_BITS(arc).set_bit(Aleph::Spanning_Tree, true);
	Node * src = (Node*) arc->src_node;
	Node * tgt = (Node*) arc->tgt_node;
	NODE_BITS(src).set_bit(Aleph::Spanning_Tree, true);
	NODE_BITS(tgt).set_bit(Aleph::Spanning_Tree, true);
      }
    NODE_BITS(s).set_bit(Aleph::Spanning_Tree, true);

    assert(check_painted_arcs());

    painted = true;
  }

  bool last_relax_and_prepare_check_negative_cycle()
  {
    bool negative_cycle = false;
    for (Ait<GT, SA> it(g, sa); it.has_curr(); it.next())
      {
	Arc * arc = it.get_curr();
	Node * src  = g.get_src_node(arc);
	Distance_Type & acum_src = acum(src);
	if (acum_src == Inf)
	  continue;

	Node * tgt = g.get_tgt_node(arc);
	Distance_Type d = dist(arc);
	Distance_Type & acum_tgt = acum(tgt);
	Distance_Type sum = acum_src + d;
	if (sum < acum_tgt)
	  {
	    negative_cycle = true; 
	    const int & index = idx(tgt);
	    arcs(index)       = arc;
	    acum_tgt          = sum;
	  }
      }
    return negative_cycle;
  }

  bool last_relax_and_test_negative_cycle()
  {
    for (Ait<GT, SA> it(g, sa); it.has_curr(); it.next())
      {
	Arc * arc = it.get_curr();
	Node * src  = g.get_src_node(arc);
	Distance_Type & acum_src = acum(src);
	if (acum_src == Inf)
	  continue;

	Node * tgt = g.get_tgt_node(arc);
	Distance_Type d = dist(arc);
	Distance_Type & acum_tgt = acum(tgt);
	Distance_Type sum = acum_src + d;
	if (sum < acum_tgt)
	  return true;
      }
    return false;
  }

  void link_cookies_and_free(typename GT::Node * s)
  {
    uninit<Ni>();

/** 
  @brief Build roads invested towards the source node s 
*/
    const size_t & n = g.vsize();
    for (int i = 0; i < n; ++i)
      {
	Arc * arc = arcs(i);
	if (arc == NULL)
	  continue;

	Node * tgt = g.get_tgt_node(arc);
	NODE_COOKIE(tgt) = g.get_src_node(arc);
      }

    NODE_COOKIE(s) = NULL; /* Just in case there is a negative cycle */

    arcs.cut();
  }

public:

/** 
  @brief Paint the spanning tree of all shortest path departing
  node s with bit Min.

  @param start Node start

  @return true if negative cycle is detected, false otherwise  
*/
  bool paint_spanning_tree(Node * start)
  {
    init_with_indexes(start); 
    
    relax_arcs();
    bool negative_cycle = last_relax_and_prepare_check_negative_cycle();
    paint_tree();
    link_cookies_and_free(s);

    return negative_cycle;
  }

/** 
  @brief fastest version ever tiempoi to paint tree
  inclusive of all minimum paths.

  @param start Node start

  @return true if the graph has a negative cycle false otherwise.
*/
  bool faster_paint_spanning_tree(Node * start)
  {
    init_with_indexes(start);
    
    const size_t & n = g.get_num_nodes();

    DynListQueue<typename GT::Node*> q;

    Node __sentinel;
    Node * sentinel = &__sentinel;

    q.put(s);
    q.put(sentinel);

    bool negative_cycle = false;

    for (int i = 0; not q.is_empty() and not negative_cycle; /* Nothing */)
      {
	Node * src = q.get(); 
	if (src == sentinel) /** ¿The sentinel is removed? */
	  if (i++ == n) 
	    break;
	  else
	    q.put(sentinel);

	relax_arcs(src, q);
      }

    negative_cycle = last_relax_and_prepare_check_negative_cycle();

    paint_tree();

    link_cookies_and_free(s);

    return negative_cycle;
  }

/**
  @brief Feature that detects if the graph has a negative cycle
    
  @param start Node start
    
  @return true if the graph has a negative cycle. false otherwise.
*/
  bool has_negative_cycle(Node * start)
  {
    init_simple(start);
    s = start;
    acum(s) = 0;
    
    relax_arcs();
    bool negative_cycle = last_relax_and_test_negative_cycle();
    uninit<Sni>();

    return negative_cycle;
  }

private:

  Path<GT> search_negative_cycle_on_partial_graph()
  {
     GT aux = build_spanning_tree<GT>(arcs);

    /* We map because Tarjan algorithm modifies cookies */
     DynMapTree<Node*, Node*> table; 
     for (typename GT::Node_Iterator it(aux); it.has_curr(); it.next())
       {
	 typename GT::Node * p = it.get_curr();
	 table.insert(p, (Node*) NODE_COOKIE(p));
       }

     Path<GT> path(aux);
     if (Tarjan_Connected_Components<GT, NAit, SA>(sa).compute_cycle(aux, path))
       {
	 Path<GT> ret(g);
	 for (typename Path<GT>::Iterator it(path); it.has_current_node(); 
	      it.next())
	   ret.append_directed((Node*) table.find(it.get_current_node()));
	 return ret;
       }
     
     return Path<GT>(g);
  }

public:

/**
  @brief Run the Bellman- Ford

  @details Run the Bellman- Ford algorithm to check for
  negative cycle.

  If a negative cycle is detected, then the algorithm runs
  Tarjan on partial tree and the cycle is calculated.

  @param [in] search start node source shortest path
  
  @return A corresponding cycle path if this is
  Found. Otherwise it returns an empty road.
*/
  Path<GT> test_negative_cycle(Node * start)
  {
    init_with_indexes(start);

    relax_arcs();
    bool negative_cycle = last_relax_and_prepare_check_negative_cycle();
    if (not negative_cycle)
      {
	link_cookies_and_free(s);
	return Path<GT>(g);
      }

    Path<GT> ret = search_negative_cycle_on_partial_graph();
    if (ret.is_empty())
      WARNING("Serious inconsistency. Bellman-Ford algorithm has detected\n"
	      "a negative cycle, but Tarjan algorithm executed on partial\n"
	      "graph has not found such cycle\n\n"
	      "Be very careful, this is provably a bug");

    link_cookies_and_free(s);
    return ret;
  }

private:

  Node * create_dummy_node()
  {
    Node * start = const_cast<GT&>(g).insert_node(typename GT::Node_Type());
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next())
      {
	Node * p = it.get_curr();
	if (p == start)
	  continue;
	Arc * a = const_cast<GT&>(g).insert_arc(start, p);
	Distance::set_zero(a);
      }
    return start;
  }

  void remove_dummy_node(Node * p) { const_cast<GT&>(g).remove_node(p); }

public:

/** 
  @brief Returns the negative cylce if this exists 
*/
  Path<GT> test_negative_cycle()
  {
    Node * start = create_dummy_node();
    Path<GT> ret_val = test_negative_cycle(start);
    remove_dummy_node(start);
    return ret_val;
  }

/** 
  @brief Runs tail Bellman- Ford version
  periodically looks negative cycles.

  @details This routine runs the Bellman- Ford based on a queue
  seen nodes (which makes usually faster). From
  * V threshold iterations, the method looks at the negative cycles
  partial tree.

  @param [in] start Start node
  @param [in] it_factor Factor on the number of nodes from
  which begins to look negative cycles

  return A path which will correspond to a negative cycle
  if it is found.Or an empty road if there is no negative cycle.
*/
  Path<GT> search_negative_cycle(Node * start, double it_factor = 0.7)
  {
    init_with_indexes(start);
    
    const size_t & n = g.get_num_nodes();
    DynListQueue<typename GT::Node*> q;
    Node __sentinel;
    Node * sentinel = &__sentinel;
    q.put(s); 
    q.put(sentinel); 

    bool negative_cycle = false;
    double threshold = it_factor*n;
    Path<GT> ret;

    for (int i = 0; not q.is_empty() and not negative_cycle; /* Nothing */)
      {
	Node * src = q.get();
	if (src == sentinel) /* ¿The sentinel is removed? */
	  if (i++ == n) 
	    break;
	  else
	    q.put(sentinel);

	relax_arcs(src, q);

	if (i >= threshold)
	  {
	    ret = search_negative_cycle_on_partial_graph();
	    negative_cycle = not ret.is_empty();	      
	  }
      }

    if (negative_cycle)
      {
	link_cookies_and_free(s);
	return ret;
      }

    negative_cycle = last_relax_and_prepare_check_negative_cycle();
    if (negative_cycle)
      {
	ret = search_negative_cycle_on_partial_graph();
	if (ret.is_empty())
	  WARNING("Serious inconsistency. Bellman-Ford algorithm has detected\n"
		  "a negative cycle, but Tarjan algorithm executed on partial\n"
		  "graph has not found such cycle\n\n"
		  "Be very careful, this is provably a bug");
      }

    link_cookies_and_free(s);
    return ret;
  }

  Path<GT> search_negative_cycle(double it_factor = 0.7)
  {
    Node * start = create_dummy_node();
    Path<GT> ret_val = search_negative_cycle(start, it_factor);
    remove_dummy_node(start);
    return ret_val;
  }

/** 
  @brief Tree builds comprehensive content shortest path 
*/
  void build_tree(GT & tree, bool with_map = true)
  { 
    if (not painted and with_map)
      throw std::domain_error("Spanning tree has not been painted");

    clear_graph(tree);

    DynMapTree<Node*, Node*> table;
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next())
      {
	Node * gp = it.get_curr();
	Node * tp = tree.insert_node(gp->get_info());
	table.insert(gp, tp);
      }

    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next())
      {
	Node * gtgt = it.get_curr();
	Node * gsrc = (Node*) NODE_COOKIE(gtgt);
	if (gsrc == NULL)
	  continue; /* This is the source node of the spanning tree */
	
	Arc ** garc_ptr = g.arcs(gsrc).find_ptr([gsrc, gtgt] (Arc * a)
		{
		  return a->src_node == gsrc and a->tgt_node == gtgt;
		});
	assert(garc_ptr and IS_ARC_VISITED(*garc_ptr, Aleph::Spanning_Tree));
	Arc * garc = *garc_ptr;
	assert(garc->src_node == gsrc and garc->tgt_node == gtgt);
	Node ** tsrc_ptr = table.search(gsrc);
	Node ** ttgt_ptr = table.search(gtgt);
	assert(tsrc_ptr and ttgt_ptr);
	Arc * tarc = tree.insert_arc(*tsrc_ptr, *ttgt_ptr, garc->get_info());
	if (with_map)
	  GT::map_arcs(garc, tarc);
      }

    if (with_map)
      table.for_each([] (const std::pair<Node*, Node*> & p)
		{
		  GT::map_nodes(p.first, p.second);
		});
  }
   
  bool test_negative_cycle(typename GT::Node * s, Path<GT> & cycle)
  {
    cycle = test_negative_cycle(s);
    return not cycle.is_empty();
  }

  bool test_negative_cycle(Path<GT> & cycle)
  {
    cycle = test_negative_cycle();
    return not cycle.is_empty();
  }

  Distance_Type get_min_path(typename GT::Node * end, Path<GT> & path)
  {
    if (not painted)
      throw std::domain_error("Graph has not previously painted");
      
    return Aleph::get_min_path<GT, Distance>(s, end, path);
  }

/** 
  @brief calculate the minimum paths from a given node

  @details in AVL mapping associated weights to each node obtained
  running the Bellman-Ford algorithm from a node
  fictitious. This routine is specifically designed for the
  Johnson algorithm for calculating all minimum paths
  from a given node.

  @param [in] m A dynamic AVL mapping how Node * -> w of
  node pointer to the node associated weight.
  
  @return true if the graph does not have a negative cycle , in which case
  values for m may be used for the algorithm
  Jhonson. Returns false if a negative cycle is detected.
*/
  bool compute_nodes_weights(DynMapTree<Node*, Distance_Type> & m)
  {
    const Distance_Type zero = 0;
    s = g.insert_node(); /* Nodo auxiliar apar detectar ciclos negativos */
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next())
      {
	Node * p = it.get_curr();
	g.insert_arc(s, p, zero); /* Debe tener conversión de Arc_Type */
      }

    init_simple(s);
    
    const size_t & n = g.get_num_nodes();
    for (int i = 0; i < n - 1; ++i)
      for (Ait<GT, SA> it(g, sa); it.has_curr(); it.next())
	{
	  Arc * arc = it.get_curr();
	  Node * src  = g.get_src_node(arc);
	  Distance_Type & acum_src = acum(src);
	  if (acum_src == Inf)
	    continue;

	  Node * tgt = g.get_tgt_node(arc);
	  Distance_Type d = dist(arc);
	  Distance_Type & acum_tgt = acum(tgt);
	  Distance_Type sum = acum_src + d;
	  if (sum < acum_tgt)
	    acum_tgt = sum;
	}

    bool negative_cycle = false;
    for (Ait<GT, SA> it(g, sa); it.has_curr(); it.next())
      {
	Arc * arc = it.get_curr();
	Node * src  = g.get_src_node(arc);
	Distance_Type & acum_src = acum(src);
	if (acum_src == Inf)
	  continue;

	Node * tgt = g.get_tgt_node(arc);
	Distance_Type d = dist(arc);
	Distance_Type sum = acum_src + d;
	if (sum < acum(tgt))
	  {
	    negative_cycle = true;
	    break;
	  }
      }

    for (typename Node::Iterator it(g); it.has_curr(); it.next())
      {
	Node * p = it.get_curr();
	m.insert(p, acum(p));
      }

    uninit<Sni>();

    m.remove(s);
    g.remove_node(s);

    return negative_cycle;
  }
};

/** 
  @brief detects whether there is a negative cycle and eventually calculated.

  @details This class takes a digraph and a destination node , run the
  Bellman- Ford and if a negative cycle is detected
  then stores it in a way.

  The procedure is configured with the following
  specifications:
  - # GT: graph type based on List_Graph.
  - # Distance < GT > : read the weight class should bow
  export the following attributes:
  - # Typedef Distance < GT > :: Distance_Type : Data type
  It represents a weight in an arc.
  - # Distance < GT > Distance_Type :: operator () ( typename GT :: Arc * a) :
  that returns the value of the peso in the bow.
 .
  - # SA: Filter arches.
  @ingroup Grafos
*/
 template <class GT, 
	   class Distance                     = Dft_Dist<GT>, 
	   template <class, class> class Ait  = Arc_Iterator,
	   template <class, class> class NAit = Node_Arc_Iterator,
           class SA                           = Dft_Show_Arc<GT>> 
struct Bellman_Ford_Negative_Cycle
{ 
/** 
  @brief Summons detection and calculation of negative cycle.

  @param [in] g Graph.
  @param [out] path path where the cycle is stored in case of
  negative cycle that is detected.
  
  @return true if there is a negative cycle ; false otherwise.
  
  @throw bad_alloc If there is insufficient memory.
*/
  bool operator () (const GT & g, Path<GT> & path, 
		    Distance & d, SA & sa) const
  {
    return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).
      test_negative_cycle(path);
  }

  bool operator () (const GT & g, Path<GT> & path, 
		    Distance && d = Distance(), SA && sa = SA()) const
  {
    return Bellman_Ford<GT,Distance, Ait, NAit, SA>(g, d, sa).
      test_negative_cycle(path);
  }  

/** 
  @brief Summons detection and calculation of negative cycle.

  @param [in] g Graph.
  @param [in] s Start Node.
  @param [out] path Path where the cycle is stored in case of
  negative cycle that is detected.
  
  @return true if there is a negative cycle ; FALSE
  otherwise.

  @throw bad_alloc If there is insufficient memory.
*/
  bool operator () (const GT & g, typename GT::Node * s,
		    Path<GT> & path, Distance & d, SA & sa) const
  {
    return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).
      test_negative_cycle(s, path);
  }

  bool operator () (const GT & g, typename GT::Node * s,
		    Path<GT> & path, Distance && d = Distance(), 
		    SA && sa = SA()) const
  {
    return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).
      test_negative_cycle(s, path);
  }

  Path<GT> operator () (const GT & g, typename GT::Node * s, 
			Distance & d, SA & sa, double it_factor = 0.7) const
  {
    return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).
      search_negative_cycle(s, it_factor);
  }

  Path<GT> operator () (const GT & g, typename GT::Node * s, 
			Distance && d = Distance(), SA && sa = SA(), 
			double it_factor = 0.7) const
  {
    return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).
      search_negative_cycle(s, it_factor);
  }

  Path<GT> operator () (const GT & g, Distance & d, SA & sa, 
			double it_factor = 0.7) const
  {
    return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).
    search_negative_cycle(it_factor);
  }

  Path<GT> 
  operator () (const GT & g, Distance && d = Distance(), SA && sa = SA(), 
	       double it_factor = 0.7) const
  {
    return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).
    search_negative_cycle(it_factor);
  }
};


# undef NI
# undef IDX
# undef ACU

} /* End namespace Aleph */

# endif /* BELLMAN_FORD_H */
