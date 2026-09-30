
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


/** @file tpl_test_path.H
 *  @brief Path existence testing.
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef TEST_PATH_H
# define TEST_PATH_H

# include <ah-graph-concepts.H>

# include <tpl_graph.H>

namespace Aleph {


      /** Checks whether a path exists between two nodes.

          Test_For_Path explores graph g depth-first starting from a
          start node, searching for a path that leads to a destination one.

          The class takes two type parameters:
          -# GT: the graph type, which must be derived from List_Graph.
          -# SA: class in charge of showing the arc. Internally, the
          function uses the filter iterator Node_Arc_Iterator (based
          on Filter_Iterator) to traverse the arcs of each node. SA is
          the class that determines whether or not the arc should be
          shown to the traversal.

          The test_path bit is used to mark the nodes and arcs
          visited during the search.

         @see find_path_depth_first() find_path_breadth_first()
         @ingroup Graphs
       */
      template <AlephGraph GT, ArcFilter<GT> SA = Dft_Show_Arc<GT> > 
class Test_For_Path
{
  SA sa;
  typename GT::Node * tgt = nullptr;

  bool test_path(typename GT::Node * curr)  
  {
    if (curr == tgt) 
      return true; // tgt was reached

    if (IS_NODE_VISITED(curr, Find_Path)) // was curr_node visited?
      return false; // yes, do not explore

    NODE_BITS(curr).set_bit(Find_Path, true); // paint curr_node

    // recursively search through curr's arcs 
    for (Node_Arc_Iterator<GT, SA> i(curr, sa); i.has_curr(); i.next_ne())
      { 
        typename GT::Arc * arc = i.get_current_arc_ne();
        if (IS_ARC_VISITED(arc, Find_Path)) 
          continue; 

        ARC_BITS(arc).set_bit(Find_Path, true); // paint arc
        if (test_path(i.get_tgt_node())) 
          return true;
      }

    // all of curr_node's adjacent arcs were explored without
    // finding end_node ==> there is no path through curr_node
    return false;
  }

  bool test_path(const GT & g, typename GT::Node * src, typename GT::Node * dest)
  {      // if the graph is connected ==> a path exists
    if (not g.is_digraph() and g.get_num_arcs() >= g.get_num_nodes()) 
      return true;

    g.reset_bit_nodes(Find_Path); 
    g.reset_bit_arcs(Find_Path);

    tgt = dest;

        // recursively search through src's adjacent arcs
    for (Node_Arc_Iterator<GT, SA> i(src, sa); i.has_curr(); i.next_ne())
      {
        typename GT::Arc * arc = i.get_current_arc_ne();
        ARC_BITS(arc).set_bit(Find_Path, true); // mark arc
        if (test_path(i.get_tgt_node())) 
          return true; 
      }

    // all of start_node's arcs have been explored without
    // finding a path to end_node ==> no path exists
    return false;
  }

public:

  Test_For_Path(SA __sa = SA()) : sa(__sa) { /* empty */ }

    /** Invokes the test for a path's existence between two nodes.

         @param[in] g the graph to search a path in.
         @param[in] start_node pointer to the path's source node.
         @param[in] end_node pointer to the path's destination node.
         @return true if a path exists between start_node and end_node.
    */
  bool operator () (const GT& g, typename GT::Node * start_node, 
		    typename GT::Node * end_node) 
  {
    return test_path(g, start_node, end_node);
  }
};



} // end namespace Aleph

# endif // TEST_PAT_H
