
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
  @file tpl_test_connectivity.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_TEST_CONNECTIVITY_H
# define TPL_TEST_CONNECTIVITY_H

# include <tpl_graph_utils.H>


/** 
  @brief Determines if a graph g is connected.

  @details This class performs a connectivity test of the g graph. The
  test calls for a depth-first traversal.

  The class checks the number of arcs.
  If this amount is less than the number of nodes,
  then the graph is considered disjointed.

  The function takes two type parameters:
  -# GT: Graph type, which must be derived from List_Graph.
  -# SA: Class is responsible for displaying the arc. Internally,
  the function uses iterator filter Node_Arc_Iterator (based on
  Filter_Iterator) to traverse the arcs of each node.
  SA is the class that determines whether or not to display bow to
  traveled.
  .

  @ingroup Grafos

  @see depth_first_traversal() is_reachable()
*/
    template <class GT, class SA = Dft_Show_Arc<GT> > 
class Test_Connectivity
{
public:

/** 
  @brief Invokes the connectivity test.

  @details This version is used when required the filter
  of arches maintain a state.

  @param[in] g Graph or digraph to verify.
  @param[in] sa Filter of graph arcs.

  @return true if the graph is connected; false otherwise.

  @note Because testing with the number of arcs, this function
  is incorrect to multigraphs.

  @throw domain_error if the routine is invoked on a digraph.
*/
  bool operator () (GT & g, SA && sa = SA()) const
  {
    if (g.is_digraph()) /* Only is valid for graphs, digraphs not */
      throw std::domain_error("test_connectivity() does not work on digraphs");

    if (g.get_num_arcs() < g.get_num_nodes() - 1) 
      return false;

    Depth_First_Traversal <GT, Default_Visit_Op<GT>, SA> traversal(sa);
 
    return traversal(g) == g.get_num_nodes();
  }

/** 
  @brief Invokes the connectivity test.

  @details This version is used when required the filter
  of arches maintain a state.

  @param[in] g Graph or digraph to verify.
  @param[in] sa Filter of graph arcs.

  @return true if the graph is connected; false otherwise.
  @note Because testing with the number of arcs, this function
  is incorrect to multigraphs.

  @throw domain_error If the routine is invoked on a digraph.
*/
  bool operator () (GT & g, SA & sa) const
  {
    if (g.is_digraph()) /* Only is valid for graphs, digraphs not */
      throw std::domain_error("test_connectivity() does not work on digraphs");

    if (g.get_num_arcs() < g.get_num_nodes() - 1) 
      return false;

    Depth_First_Traversal <GT, Default_Visit_Op<GT>, SA> traversal(sa);
 
    return traversal(g) == g.get_num_nodes();
  }
};



# endif // TPL_TEST_CONNECTIVITY_H
