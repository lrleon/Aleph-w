
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
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

            @param g the graph or digraph to verify
            @param sa the arc filter for the graph (rvalue reference, default: Dft_Show_Arc<GT>)
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

    @param g the graph or digraph to verify
    @param sa the arc filter for the graph (lvalue reference)
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
