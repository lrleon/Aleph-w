/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

/** @file tpl_test_connectivity.H
 *  @brief Graph connectivity testing.
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_TEST_CONNECTIVITY_H
# define TPL_TEST_CONNECTIVITY_H

# include <tpl_graph_utils.H>
 # include <ah-errors.H>


       /** Determines if a graph g is connected.

           This class performs a connectivity test on graph g. The test
           uses a depth-first traversal.

           The class verifies the number of arcs. If this number is less than
           the number of nodes minus one, then the graph is considered
           disconnected.

           The function takes two type parameters:
           -# GT: the graph type, which must be derived from List_Graph.
           -# SA: class responsible for showing the arc. Internally, the
           function uses the filtered iterator Node_Arc_Iterator (based
           on Filter_Iterator) to traverse the arcs of each node. SA is
           the class that determines whether the arc should be shown during
           traversal.

           @ingroup Graphs
           @see depth_first_traversal() is_reachable()
        */
    template <class GT, class SA = Dft_Show_Arc<GT> > 
class Test_Connectivity
{
public:

        /** Invokes the connectivity test.

            This version is used when the arc filter needs to maintain state.

            @param[in] g the graph or digraph to verify.
            @param[in] sa the arc filter for the graph (default: Dft_Show_Arc<GT>)
            @return true if the graph is connected; false otherwise.
            @note Due to the test with the number of arcs, this function
            is incorrect for multigraphs.
            @throw domain_error if the routine is invoked on a digraph.
         */
  bool operator () (GT & g, SA && sa = SA()) const
  {
    ah_domain_error_if(g.is_digraph())
      << "test_connectivity() does not work on digraphs";

    if (g.get_num_arcs() < g.get_num_nodes() - 1)
      return false;

    Depth_First_Traversal <GT, Default_Visit_Op<GT>, SA> traversal(sa);

    return traversal(g) == g.get_num_nodes();
  }

/** Invokes the connectivity test.

    This version is used when the arc filter needs to maintain state.

    @param[in] g the graph or digraph to verify.
    @param[in] sa the arc filter for the graph (by reference)
    @return true if the graph is connected; false otherwise.
    @note Due to the test with the number of arcs, this function
    is incorrect for multigraphs.
    @throw domain_error if the routine is invoked on a digraph.
 */
  bool operator () (GT & g, SA & sa) const
  {
    ah_domain_error_if(g.is_digraph())
      << "test_connectivity() does not work on digraphs";

    if (g.get_num_arcs() < g.get_num_nodes() - 1) 
      return false;

    Depth_First_Traversal <GT, Default_Visit_Op<GT>, SA> traversal(sa);
 
    return traversal(g) == g.get_num_nodes();
  }
};



# endif // TPL_TEST_CONNECTIVITY_H
