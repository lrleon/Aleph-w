
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


/** @file tpl_find_path.H
 *  @brief Path finding algorithms in graphs.
 *
 *  Finds paths between nodes using DFS, BFS, or specialized
 *  algorithms. For weighted shortest paths, see Dijkstra.H.
 *
 *  ## Features
 *  - DFS path finding
 *  - BFS path finding (shortest in unweighted)
 *  - All paths enumeration
 *
 *  ## Complexity: O(V + E)
 *
 *  @see Dijkstra.H Weighted shortest paths
 *  @see Bellman_Ford.H Handles negative weights
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_FIND_PATH_H
# define TPL_FIND_PATH_H

# include <ah-graph-concepts.H>

# include <tpl_dynListStack.H>
# include <tpl_dynListQueue.H>
# include <tpl_graph_utils.H>
# include <ah-errors.H>

namespace Aleph
{
  template <class GT>
  struct Dft_Goal_Node
  {
    bool operator ()(typename GT::Node *) const noexcept { return false; }
  };

  /** Performs a depth-first search for a path between a pair of nodes.

      Find_Path_Depth_First searches depth-first for a path between
      start_node and end_node, building a path equivalent to the
      recursive depth of the search as it goes. If a path is found,
      the method returns true and the path parameter holds the path
      in question; otherwise, the function returns false and the
      path's value is indeterminate.

      The class takes two type parameters:
      -# GT: the graph type, which must be derived from List_Graph
      -# SA: class in charge of showing the arc. Internally, the
      function uses the filter iterator Node_Arc_Iterator (based
      on Filter_Iterator) to traverse the arcs of each node. SA is
      the class that determines whether or not the arc should be
      shown to the traversal
      .

      @see find_path_breadth_first()
      @see dijkstra_min_spanning_tree() dijkstra_min_path()
      @see bellman_ford_min_spanning_tree() q_bellman_ford_min_spanning_tree()
      @ingroup Graphs
  */
  template <AlephGraph GT,
            template <class, class> class Itor = Node_Arc_Iterator,
            ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  class Find_Path_Depth_First
  {
    SA & sa;
    GT *g_ptr = nullptr;

    template <class Op>
    bool find_path(typename GT::Node *curr, typename GT::Arc *arc,
                   Path<GT> & path, Op & op)
    {
      if (op(curr)) // goal node reached?
        {
          path.append(arc);
          return true;
        }

      if (IS_NODE_VISITED(curr, Find_Path)) // has it been visited?
        return false; // yes ==> there is no path from it

      path.append(arc); // add curr_arc to the path
      NODE_BITS(curr).set_bit(Find_Path, true);

      // recursively search through curr_node's arcs
      for (Itor<GT, SA> i(curr, sa); i.has_curr(); i.next())
        {
          auto next_arc = i.get_curr();
          if (IS_ARC_VISITED(next_arc, Find_Path))
            continue;

          ARC_BITS(next_arc).set_bit(Find_Path, true);
          auto next_node = g_ptr->get_connected_node(next_arc, curr);
          if (find_path(next_node, next_arc, path, op))
            return true; // a path was found
        }

      path.remove_last_node();

      return false;
    }

    template <class Op>
    bool find(const GT & g, typename GT::Node *start, Path<GT> & path, Op & op)
    {
      g_ptr = const_cast<GT *>(&g);
      path.set_graph(g, start);

      if (op(start))
        return true;

      g.reset_bit_nodes(Find_Path);
      g.reset_bit_arcs(Find_Path);

      NODE_BITS(start).set_bit(Find_Path, true);

      // recursively explore each of start's arcs
      for (Itor<GT, SA> i(start, sa); i.has_curr(); i.next())
        {
          auto arc = i.get_curr();
          ARC_BITS(arc).set_bit(Find_Path, true);

          auto next_node = g.get_connected_node(arc, start);
          if (IS_NODE_VISITED(next_node, Find_Path))
            continue;

          if (find_path(next_node, arc, path, op))
            return true;
        }

      path.remove_last_node();

      return false;
    }

    template <class Op>
    bool find(const GT & g, typename GT::Node *start, Path<GT> & path, Op && op)
    {
      return find(g, start, path, op);
    }

  public:
    Find_Path_Depth_First(SA && __sa = SA())
      : sa(__sa)
    { /* empty */
    }

    Find_Path_Depth_First(SA & __sa)
      : sa(__sa)
    { /* empty */
    }

    /** Invokes the depth-first path search.

        @param[in] g the graph on which the path is to be searched.
        @param[in] start pointer to the path's starting node.
        @param[in] end pointer to the path's destination node.
        @param[out] path the path seen during the depth-first
        search; only meaningful if the return value is true.
        @throw bad_alloc if there is not enough memory to keep
        building the path
    */
    bool operator ()(const GT & g,
                     typename GT::Node *start, typename GT::Node *end,
                     Path<GT> & path)
    {
      return find(g, start, path, [end](auto p) { return p == end; });
    }

    /** Invokes the depth-first path search.

        @param[in] g the graph on which the path is to be searched.
        @param[in] start pointer to the path's starting node.
        @param[in] end pointer to the path's destination node.
        @return path reaching `end` if this one was found;  `false` otherwise
        @throw bad_alloc if there is not enough memory to keep
        building the path
    */
    Path<GT> operator ()(const GT & g,
                         typename GT::Node *start,
                         typename GT::Node *end)
    {
      Path<GT> ret(g);
      find(g, start, ret, [end](auto p) { return p == end; });
      return ret;
    }

    /** Invokes the depth-first path search.

        @param[in] g the graph on which the path is to be searched.
        @param[in] start pointer to the path's starting node.
        @param[in] op functor that implements the search criterion. The
        functor receives the node as a parameter and must return `true`
        if the node satisfies the criterion, `false` otherwise.
        @return path the path seen during the depth-first
        search; if no such path exists then the path is empty
        @throw bad_alloc if there is not enough memory to keep
        building the path
    */
    template <class Op>
    Path<GT> operator ()(const GT & g, typename GT::Node *start, Op & op)
    {
      Path<GT> ret(g);
      find<Op>(g, start, ret, op);
      return ret;
    }

    template <class Op = Dft_Goal_Node<GT>>
    Path<GT> operator ()(const GT & g, typename GT::Node *start, Op && op)
    {
      Path<GT> ret(g);
      find<Op>(g, start, ret, op);
      return ret;
    }
  };


  /** Performs a breadth-first search for a path between a pair of nodes.

Find_Path_Breadth_First searches breadth-first for a path between
start_node and end_node, building a path toward the destination
node as it goes. If a path is found, the method returns true and
the path parameter holds the path in question; otherwise, the
function returns false and the path's value is indeterminate.

The class takes two type parameters:
-# GT: the graph type, which must be derived from List_Graph
-# SA: class in charge of showing the arc. Internally, the
function uses the filter iterator Node_Arc_Iterator (based on
Filter_Iterator) to traverse the arcs of each node. SA is
the class that determines whether or not the arc should be
shown to the traversal
.

@see find_path_depth_first()
@see dijkstra_min_spanning_tree() dijkstra_min_path()
@see bellman_ford_min_spanning_tree() q_bellman_ford_min_spanning_tree()
@ingroup Graphs
  */
  template <AlephGraph GT,
            template <typename, class> class Itor = Node_Arc_Iterator,
            ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  class Find_Path_Breadth_First
  {
    SA & sa;

    template <class Op>
    bool find_path(const GT & g, typename GT::Node *start,
                   Path<GT> & path, Op & op)
    {
      if (not path.inside_graph(g))
        ah_invalid_argument_if(true) << "Path does not belong to graph";

      path.empty(); // clear anything that may be in path
      g.reset_nodes();
      g.reset_arcs();

      DynListQueue<typename GT::Arc *> q;

      for (Itor<GT, SA> i(start, sa); i.has_curr(); i.next())
        q.put(i.get_current_arc());

      NODE_BITS(start).set_bit(Find_Path, true); // mark it visited

      typename GT::Node *end = nullptr;

      while (not q.is_empty()) // while arcs remain to be visited
        {
          auto arc = q.get();
          auto src = g.get_src_node(arc);
          auto tgt = g.get_tgt_node(arc);

          if (IS_NODE_VISITED(src, Find_Path) and IS_NODE_VISITED(tgt, Find_Path))
            continue;

          if (IS_NODE_VISITED(tgt, Find_Path))
            std::swap(src, tgt);

          ARC_BITS(arc).set_bit(Find_Path, true);
          NODE_BITS(tgt).set_bit(Find_Path, true);
          NODE_COOKIE(tgt) = src;

          if (op(tgt)) // was a path satisfying op found?
            {
              end = tgt;
              break;
            }

          for (Itor<GT, SA> i(tgt); i.has_curr(); i.next())
            {
              auto a = i.get_current_arc();
              if (IS_ARC_VISITED(a, Find_Path)) // arc visited?
                continue; // yes ==> move on to the next one

              // check the arc's nodes to see whether they were visited
              if (IS_NODE_VISITED(g.get_src_node(a), Find_Path) and
                  IS_NODE_VISITED(g.get_tgt_node(a), Find_Path))
                continue; // nodes already visited ==> do not enqueue the arc

              q.put(a);
            }
        }

      if (not end)
        return false;

      q.empty();
      path.insert(end);
      auto p = end;
      while (p != start)
        {
          p = (typename GT::Node *) NODE_COOKIE(p);
          path.insert(p);
        }

      return true;
    }

    template <class Op>
    bool find_path(const GT & g, typename GT::Node *start,
                   Path<GT> & path, Op && op)
    {
      return find_path(g, start, path, op);
    }

  public:
    Find_Path_Breadth_First(SA & _sa)
      : sa(_sa)
    { /* Empty */
    }

    Find_Path_Breadth_First(SA && _sa = SA())
      : sa(_sa)
    { /* Empty */
    }

    /** Invokes the breadth-first path search.

    @param[in] g the graph on which the path is to be searched.
    @param[in] start pointer to the path's starting node.
    @param[in] op the end-of-search criterion on the visited node
    @return the path seen during the breadth-first search
    @throw bad_alloc if there is not enough memory to keep building
    the path or for the breadth-first traversal's internal queue.
    */
    template <class Op>
    Path<GT> operator ()(const GT & g, typename GT::Node *start, Op & op)
    {
      Path<GT> ret(g);
      find_path(g, start, ret, op);
      return ret;
    }

    template <class Op>
    Path<GT> operator ()(const GT & g, typename GT::Node *start, Op && op)
    {
      Path<GT> ret(g);
      find_path(g, start, ret, op);
      return ret;
    }

    /** Invokes the breadth-first path search.

        @param[in] g the graph on which the path is to be searched.
        @param[in] start pointer to the path's starting node.
        @param[in] end pointer to the path's destination node.
        @param[out] path the path seen during the breadth-first
        search; only meaningful if the return value is true.
        @throw bad_alloc if there is not enough memory to keep building
        the path or for the breadth-first traversal's internal queue.
    */
    bool operator ()(const GT & g, typename GT::Node *start,
                     typename GT::Node *end, Path<GT> & path)
    {
      return find_path(g, start, path, [end](auto p) { return p == end; });
    }

    /** Invokes the breadth-first path search.

        @param[in] g the graph on which the path is to be searched.
        @param[in] start pointer to the path's starting node.
        @param[in] end pointer to the path's destination node.
        @return path the path seen during the breadth-first search
        @throw bad_alloc if there is not enough memory to keep building
        the path or for the breadth-first traversal's internal queue.
    */
    Path<GT> operator ()(const GT & g,
                         typename GT::Node *start, typename GT::Node *end)
    {
      Path<GT> ret(g);
      find_path(g, start, ret, [end](auto p) { return p == end; });
      return ret;
    }
  };

  /** Path search on directed graphs defined through a graph class
      (not a digraph).

     @ingroup Graphs
   */
  template <AlephGraph GT,
            template <typename, class> class Itor = Out_Iterator,
            ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  class Directed_Find_Path
  {
    const GT & g;
    SA & sa;

    template <template <typename T> class Q, class Op>
    Path<GT> find(typename GT::Node *start, Op & op)
    {
      g.reset_nodes();
      g.reset_arcs();

      start->set_state(Processed);

      Q<typename GT::Arc *> q;
      for (Itor<GT, SA> it(start, sa); it.has_curr(); it.next())
        {
          auto a = it.get_curr();
          g.get_tgt_node(a)->set_state(Processing);
          a->set_state(Processing);
          q.put(a);
        }

      typename GT::Node *end = nullptr, *curr = nullptr;
      while (not q.is_empty())
        {
          auto arc = q.get();
          assert(arc->state() == Processing);
          arc->set_state(Processed);

          curr = g.get_tgt_node(arc);
          if (curr->state() == Processed)
            continue;

          curr->set_state(Processed);
          NODE_COOKIE(curr) = g.get_src_node(arc);

          if (op(curr))
            {
              end = curr;
              break;
            }

          for (Itor<GT, SA> it(curr, sa); it.has_curr(); it.next())
            {
              auto a = it.get_curr();
              a->set_state(Processing);

              auto tgt = g.get_tgt_node(a);
              if (tgt->state() == Processed)
                continue;

              if (tgt->state() != Processed)
                {
                  q.put(a);
                  tgt->set_state(Processing);
                }
              else
                a->set_state(Processed);
            }
        } // end while

      Path<GT> ret(g);
      if (not end)
        return ret;

      assert(curr == end);

      while (curr != start)
        {
          ret.insert(curr);
          curr = (typename GT::Node *) NODE_COOKIE(curr);
        }
      ret.insert(start);

      return ret;
    }

  public:
    Directed_Find_Path(const GT & __g, SA & __sa) : g(__g), sa(__sa) {}

    Directed_Find_Path(const GT & __g, SA && __sa = SA()) : g(__g), sa(__sa) {}

    template <class Op>
    Path<GT> dfs(typename GT::Node *start, Op & op)
    {
      return find<DynListStack, Op>(start, op);
    }

    template <class Op>
    Path<GT> dfs(typename GT::Node *start, Op && op)
    {
      return find<DynListStack, Op>(start, op);
    }

    template <class Op>
    Path<GT> bfs(typename GT::Node *start, Op & op)
    {
      return find<DynListQueue, Op>(start, op);
    }

    template <class Op>
    Path<GT> bfs(typename GT::Node *start, Op && op)
    {
      return find<DynListQueue, Op>(start, op);
    }

    Path<GT> dfs(typename GT::Node *start, typename GT::Node *end)
    {
      return dfs(start, [end](auto p) { return p == end; });
    }

    Path<GT> bfs(typename GT::Node *start, typename GT::Node *end)
    {
      return bfs(start, [end](auto p) { return p == end; });
    }
  };
} // namespace Aleph

# endif // TPL_FIND_PATH_H
