
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
  @file Floyd.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef FLOYD_H
# define FLOYD_H

# include <sstream>
# include <ahFunction.H>
# include <tpl_indexArc.H>
# include <tpl_dynMat.H>

namespace Aleph
{

/** 
  @brief Calculate the cost matrix of shortest paths
  between all pairs of nodes of a graph the path matrix g
  minimum according to the algorithm of Floyd-Warshall.

  @details This class uses the Floyd-Warshall algorithm for
  calculate two matrices Ady_Mat type specified as follows:
  - # Dist: minimum cost matrix between all pairs of nodes.
  Each entry dist (i, j) stores the minimum total cost to go from node
  with index i to the node j index. n
  Recall that a node index in the array can dist
  dist calculated by (node), where node is a pointer to
  node in a representation derived enlzadas lists
  type of graph
  - # Path: shortest path matrix. Each entry path (i, j)
  k stores the node that allowed the algorithm Floyd-Warshall
  find the minimum value of dist (i, j). Thus, inspections
  subsequent to dist (k, j) allow to find and build the road
  to node j. n
  The find_min_path function (g, i, j, path) performs the above function.

  The Floyd-Warshall algorithm handles negative weights, but not
  It operates correctly if the graph contains negative cycles.
  Wear the Bellman-Ford q_bellman_ford_min_spanning_tree ()
  if their presence is suspected.

  The algorithm uses two additional internal matrices.

  The procedure is configured with the following specifications:
  - # GT: type graph
  - # Distance <GT>: Class bow weight reading
  you must export the following attributes:
    - # Typedef Distance <GT> :: Distance_Type: Data type representing a
    weight in an arc.
    - # Distance <GT> Distance_Type :: operator () (typename GT :: Arc * a):
    that returns the value of the peso in the bow.
    - # Distance <GT> :: Max_Distance: Static Constant
    corresponding to the maximum distance value algorithm
    considered as infinite.
    - # Typename Distance <GT> :: Zero_Distance: Static Constant
    neutral element corresponding to the sum. Traditionally,
    in the vast majority of cases, this will be zero.
  -# SA: Filtro de arcos.
 .

  @see dijkstra_min_spanning_tree() dijkstra_min_path() 
  find_path_depth_first() find_min_path() bellman_ford_min_spanning_tree() 
  q_bellman_ford_min_spanning_tree() 
  
  @ingroup Graphs
*/
    template <class GT, 
	      class Distance = Dft_Dist<GT>, 
              class SA       = Dft_Show_Arc<GT>> 
class Floyd_All_Shortest_Paths
{
  typedef typename GT::Node Node;
  typedef typename GT::Arc Arc;
  typedef typename Distance::Distance_Type Distance_Type;

  DynArray<Node*> nodes;
  GT & g;
  const long n;
  const Distance_Type Inf;
  bool computed;
  DynMatrix<long> * path_ptr;
  SA & sa;

public:

/** 
  @brief Returns the node corresponding to the index i in a matrix
  adjacency to the Floyd-Warshall algorithm 
*/
  Node * select_node(long i)
  {
    return random_select<Node*>(nodes, i);
  }  

/** 
  @brief Returns the index into an adjacency matrix
  I would have to have the p node 
*/
  int index_node(Node * p)
  {
    return random_search<Node*>(nodes, p);
  }

private:

  void init()
  {
    if (not g.is_digraph())
      throw std::domain_error("Floyd algorithm only operates on digraphs");

    int i = 0;
    nodes.reserve(g.get_num_nodes());
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next())
      nodes.access(i++) = it.get_curr();
  }

public:

  Floyd_All_Shortest_Paths(GT & __g, SA && __sa = SA()) 
    : g(__g), n(g.get_num_nodes()), 
      Inf(std::numeric_limits<Distance_Type>::max()), computed(false), sa(__sa)
  {
    init();
  }

  Floyd_All_Shortest_Paths(GT & __g, const SA & __sa) 
    : g(__g), n(g.get_num_nodes()), 
      Inf(std::numeric_limits<Distance_Type>::max()), computed(false), sa(__sa)
  {
    init();
  }

  string entry(const Distance_Type & e)
  {
    if (e == Inf)
      return "Inf";

    std::stringstream ss;
    ss << e;
    return ss.str();
  }

  static void print(DynMatrix<Distance_Type> & dist)
  {
    Distance_Type Inf = std::numeric_limits<Distance_Type>::max();
    const int n = dist.rows();
    for (int i = 0; i < n; ++i)
      {
	for (int j = 0; j < n; ++j)
	  if (dist.access(i,j) == Inf)
	    cout << "inf ";
	  else
	    cout << dist.access(i,j) << " ";
	cout << endl;
      }
    cout << endl;
  }

/** 
  @brief invokes the calculation of minimum costs and matrices
  their paths between all pairs of nodes.

  @param [in] g Graph to which you want to calculate the minimum spanning tree.
  @param [in] dist Matrix where the minimum costs are stored.
  @param [out] Matrix index path shortest path that allows recovery.
  
  @throw bad_alloc If there is not enough memory to build the internal matrices
*/
  bool operator () (DynMatrix<Distance_Type> & dist, DynMatrix<long> & path) 
  {
    dist.allocate();
    path.allocate();

    {    
/** 
  @brief initializes matrices 
*/
      IndexArc<GT, Rand_Tree, SA> arcs(g, true, sa);

      for (long i = 0; i < n; ++i)
	{
	  Node * src = select_node(i);
	  for (long j = 0; j < n; ++j)
	    {
	      if (i == j)
		{
		  dist(i, j) = 0;
		  continue;
		}

	      Node * tgt = select_node(j);
	      Arc * arc  = arcs.search(src, tgt);
	      if (arc == NULL)
		{
		  dist(i, j) = Inf;
		  continue;
		}

	      dist(i, j) = Distance () (arc);
	      path(i, j) = j;
	    }
	}
    }

    for (long k = 0; k < n; ++k)
      for (long i = 0; i < n; ++i)
	{
	  const Distance_Type & dik = dist(i, k);
	  for (long j = 0; j < n; ++j)
	    { 
	      const Distance_Type & dij = dist(i, j);
	      if (dik == Inf) 
		continue;

	      const Distance_Type & dkj = dist(k, j);
	      if (dkj == Inf)
		continue;

/** 
  @brief Calculate new distance k through
*/
	      Distance_Type new_dist = dik + dkj;
	      if (new_dist < dij) 
		{
		  dist(i, j) = new_dist; /** Upgrade shorter distance. */
		  path(i, j) = path(i, k);
		}

    /* Negative cycle? */
	      if (dist(i, i) < 0) 
		return true;
	    }
	}

    path_ptr = &path;
    computed = true;

    return false;
  }

  void get_min_path(const long src_idx, const long tgt_idx,
		     Path<GT> & path)
  {
    if (not computed)
      throw std::domain_error("Floyd algorithm has not previously ran");
      
    typename GT::Node * src = select_node(src_idx);
    path.set_graph(g, src);
    for (int i = src_idx, j; j != tgt_idx; i = j)
      {
	j = (*path_ptr)(i, tgt_idx);
	typename GT::Node * p = select_node(j);
	path.append(p);
      }
  } 

  void get_min_path (typename GT::Node * src, 
		     typename GT::Node * tgt,
		     Path<GT> &          path) const
  {
    const long src_idx = index_node(src);
    const long tgt_idx = index_node(tgt);

    get_min_path(src_idx, tgt_idx, path);
  }
};

} /* End namespace Aleph */

# endif /* FLOYD_H */
