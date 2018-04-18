
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
  @file warshall.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef WARSHALL_H
# define WARSHALL_H

# include <tpl_graph.H>
# include <tpl_matgraph.H>

namespace Aleph {

/** 
  @brief Calculation of transitive closure of an adjacency matrix.

  @details This function calculates the transitive closure of the graph g by
  Warshall algorithm. The result is stored in the matrix
  Bit mat.

  Each entry mat (i, j) indicates existence of a path between the
  source node to the destination node index i to index j. a
  zero value indicates that there is no way; a value 1
  There is at least one way.

  The procedure uses two bit arrays; of use
  inmate who is released under the procedure and the very
  matrix mat.

  @param[in] g Graph represented by a variant of List_Graph.  
  @param[out] mat Bit array where places the result.

  @throw bad_alloc If there is insufficient memory. 

  @see List_Graph Bit_Mat_Graph

  @ingroup Grafos
*/
    template <class GT, class SA = Dft_Show_Arc<GT> >
void warshall_compute_transitive_clausure(GT &                    g, 
                                          Bit_Mat_Graph<GT, SA> & mat)
{
  Bit_Mat_Graph<GT, SA> mat_prev(g);
  if (mat.get_list_graph() != &g)
    mat.set_list_graph(g);
  const size_t & n = mat.get_num_nodes();
  for (int k = 0; k < n; k++) 
    {
      for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
          mat(i, j) = mat_prev(i, j) or
                      (mat_prev(i, k) and mat_prev(k, j));

      mat_prev = mat;
    }
}

/** 
  @brief Calculation of transitive closure of an adjacency matrix.

  @details This class computes the transitive closure of a graph by
  Warshall algorithm. The result is stored in an array
  bit.

  Each entry (i, j) in the matrix indicates existence of a path
  between the source node to the destination node index i to index
  j. A value of zero indicates that there is no way; a value 1
  There is at least one way.

  @see List_Graph Bit_Mat_Graph

  @ingroup Grafos
*/
    template <class GT, class SA = Dft_Show_Arc<GT> >
class Warshall_Compute_Transitive_Clausure
{ 
public:

/** 
  @brief Summon the calculation of the transitive closure of a graph.

  @details The procedure uses two bit arrays; of use
  inmate who is released under the procedure and the very
  matrix mat.
        
  @param[in] g Graph represented by a variant of List_Graph.  
  @param[out] mat Bit array where places the result
  
  @throw bad_alloc If there is insufficient memory. 
*/
  void operator () (GT & g, Bit_Mat_Graph<GT> & mat) const
  {
    warshall_compute_transitive_clausure <GT, SA> (g, mat);
  }
};

} /* End namespace Aleph */

# endif // WARSHALL_H
