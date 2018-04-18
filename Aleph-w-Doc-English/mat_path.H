
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
  @file mat_path.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef MAT_PATH_H
# define MAT_PATH_H

# include <tpl_matgraph.H>

    template <class Mat>
void find_min_path(Mat & p, const long & src_index, const long & tgt_index,
                   Path<typename Mat::Graph_Type> & path);
/** 
  @brief builds a shortest path from one node to another from a
  path matrix calculated by the Floyd-Warshall algorithm.

  @details This procedure takes p matrix containing indices
  shortest path calculated by the Floyd-Warshall algorithm,
  two nodes corresponding to source and destination pointers
  respectively in the adjacency list representation
  used when invoked floyd_all_shortest_paths (), and calculates
  the shortest path.

  This procedure is designed to be named after
  floyd_all_shortest_paths run (), which calculates the matrix p.

  Results are completely indeterminate if the matrix is p invalid. 

  @param [in] p Matrix indexes shortest path.
  @param [in] src_node Pointer to the source node within the
  adjacency lists representation.
  @param [in] tgt_node Pointer to the destination node within the
  adjacency lists representation.
  @param [out] path Shortest path in the graph represented with
  adjacency lists used when you ran
  floyd_all_shortest_paths ().
   
  @throw bad_alloc If there is not enough memory to build the road.
    
  @see floyd_all_shortest_paths ()
    
  @ingroup Graphs
*/
    template <class Mat>
void find_min_path(Mat & p, typename Mat::Node * src_node, 
                   typename Mat::Node * tgt_node,
                   Path<typename Mat::Graph_Type> & path)
{
  find_min_path(p, p(src_node), p(tgt_node, path));
}

    template <class Mat>
class Find_Min_Path
{
public:

  void operator () (Mat &                                 p, 
                    typename Mat::Node *                  src_node, 
                    typename Mat::Node *                  tgt_node,
                    Path<typename Mat::Graph_Type> & path) const
  {
    find_min_path <Mat> (p, src_node, tgt_node, path);
  }

  void operator () (Mat &                                 p, 
                    const long & src_index, const long &  tgt_index,
                    Path<typename Mat::Graph_Type> & path) const
  {
    find_min_path <Mat> (p, src_index, tgt_index, path);
  }
};

/** 
  @brief builds a shortest path from one node to another from a
  path matrix calculated by the Floyd-Warshall algorithm.

  @details This procedure takes p matrix containing indices
  shortest path calculated by the Floyd-Warshall algorithm,
  two integers i and j in the matrix corresponding to indices
  source and destination node respectively, and calculates the road
  minimum representation in the adjacency lists used in the
  Floyd-Warshall algorithm.

  This procedure is designed to be named after
  floyd_all_shortest_paths run (), which calculates the matrix
  p.

  Results are completely indeterminate if the matrix is p
  invalid.

  An overloaded version of this method takes as parameters
  node pointers, instead of the matrix indices.

  @param [in] p Matrix indexes shortest path.
  @param [in] src_idx Index source node within the matrix of
  minimal roads.
  @param [in] Index tgt_idx Destination node within the matrix
  minimal roads.
  @param [out] path Shortest path in the graph represented with
  adjacency lists used when you ran
  floyd_all_shortest_paths ().
    
  @throw bad_alloc If there is not enough memory to build the
  way.
    
  @see floyd_all_shortest_paths ()
    
  @ingroup Graphs
*/
    template <class Mat> void 
find_min_path(Mat & p, const long & src_idx, const long & tgt_idx,
              Path<typename Mat::Graph_Type> & path)
{
  typedef typename Mat::Graph_Type GT;
  GT & g                  = p.get_list_graph();
  typename GT::Node * src = p(src_idx);
  path.set_graph(g, src);
  for (int i = src_idx, j; j != tgt_idx; i = j)
    {
      j = p(i, tgt_idx);
      path.append(p(j));
    }
}

# endif // MAT_PATH_H
