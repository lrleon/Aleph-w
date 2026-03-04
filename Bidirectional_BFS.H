
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


/** @file Bidirectional_BFS.H
 *  @brief Bidirectional BFS for shortest unweighted path.
 *
 *  Implements bidirectional breadth-first search, which explores
 *  simultaneously from both the source and the target, meeting in the
 *  middle. This can be dramatically faster than standard BFS for large
 *  graphs, reducing the search space from O(b^d) to O(b^(d/2)).
 *
 *  ## How it works
 *
 *  Two BFS frontiers are maintained: one expanding from the start node
 *  (forward) and one expanding from the end node (backward). At each
 *  step, the smaller frontier is expanded. When a node is reached by
 *  both frontiers, the search terminates and the path is reconstructed
 *  by concatenating the forward path (start to meeting point) with the
 *  backward path (meeting point to end).
 *
 *  ## Complexity
 *
 *  | Operation | Time | Space |
 *  |-----------|------|-------|
 *  | Find path | O(b^(d/2)) | O(b^(d/2)) |
 *
 *  where b is the branching factor and d is the path depth.
 *
 *  ## Directed graph support
 *
 *  For directed graphs, use `Out_Iterator` for forward search and
 *  `In_Iterator` for backward search:
 *
 *  ```cpp
 *  Bidirectional_BFS<DGT, Out_Iterator, In_Iterator> bibfs;
 *  ```
 *
 *  @see tpl_find_path.H For standard BFS/DFS path finding
 *  @see Dijkstra.H For weighted shortest paths
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath Leon
 */

# ifndef BIDIRECTIONAL_BFS_H
# define BIDIRECTIONAL_BFS_H

# include <tpl_dynListQueue.H>
# include <tpl_graph_utils.H>
# include <ah-errors.H>

namespace Aleph
{

/** @brief Bidirectional BFS for finding shortest unweighted paths.
 *
 *  @details This class finds the shortest path (in terms of number of
 *  edges) between two nodes by simultaneously running BFS from both
 *  the source and the target. The search terminates when the two
 *  frontiers meet.
 *
 *  For undirected graphs, both iterators default to `Node_Arc_Iterator`.
 *  For directed graphs, use `Out_Iterator` for forward and `In_Iterator`
 *  for backward traversal.
 *
 *  The class receives the following template parameters:
 *  -# `GT`: graph type.
 *  -# `Fwd_Itor`: arc iterator for forward expansion (from start).
 *  -# `Bck_Itor`: arc iterator for backward expansion (from end).
 *  -# `SA`: arc filter for internal iterators.
 *
 *  @note This algorithm finds shortest paths in terms of edge count
 *        (unweighted). For weighted shortest paths, use Dijkstra or A*.
 *
 *  @warning This class is **not thread-safe**.
 *
 *  @see Find_Path_Breadth_First For standard (unidirectional) BFS.
 *  @see Dijkstra_Min_Paths For weighted shortest paths.
 *
 *  @ingroup Graphs
 */
template <class GT,
          template <typename, class> class Fwd_Itor = Node_Arc_Iterator,
          template <typename, class> class Bck_Itor = Node_Arc_Iterator,
          class SA = Dft_Show_Arc<GT>>
class Bidirectional_BFS
{
public:
  using Node = typename GT::Node;
  using Arc = typename GT::Arc;

private:
  SA sa;

  // Direction constants
  static constexpr int UNSEEN = 0;
  static constexpr int FORWARD = 1;
  static constexpr int BACKWARD = 2;

  struct BFS_Info
  {
    Node * fwd_parent = nullptr;
    Node * bck_parent = nullptr;
    int dir = UNSEEN;
  };

#define BIBFS_INFO(p)       (static_cast<BFS_Info *>(NODE_COOKIE(p)))
#define BIBFS_DIR(p)        (BIBFS_INFO(p)->dir)
#define BIBFS_FWD_PARENT(p) (BIBFS_INFO(p)->fwd_parent)
#define BIBFS_BCK_PARENT(p) (BIBFS_INFO(p)->bck_parent)

  void init(const GT & g)
  {
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next())
      {
        auto p = it.get_curr();
        NODE_COOKIE(p) = new BFS_Info;
      }
  }

  void cleanup(const GT & g)
  {
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next())
      {
        auto p = it.get_curr();
        delete BIBFS_INFO(p);
        NODE_COOKIE(p) = nullptr;
      }
  }

  void build_path(const GT & g, Node * start, Node * meeting, Node * end,
                  Path<GT> & path)
  {
    path.empty();

    if (start == end)
      {
        path.set_graph(g, start);
        return;
      }

    // Build forward half: start -> meeting
    DynList<Node *> fwd_nodes;
    for (auto curr = meeting; curr != nullptr; curr = BIBFS_FWD_PARENT(curr))
      fwd_nodes.insert(curr);  // prepend

    // Build backward half: meeting -> end
    DynList<Node *> bck_nodes;
    for (auto curr = BIBFS_BCK_PARENT(meeting); curr != nullptr;
         curr = BIBFS_BCK_PARENT(curr))
      bck_nodes.append(curr);

    // Construct the path from the node sequence
    // First node
    auto fwd_it = fwd_nodes.get_it();
    if (not fwd_it.has_curr())
      return;

    path.set_graph(g, fwd_it.get_curr());
    auto prev = fwd_it.get_curr();
    fwd_it.next();

    // Forward nodes
    while (fwd_it.has_curr())
      {
        auto curr = fwd_it.get_curr();
        // Find arc between prev and curr
        Arc * arc = search_arc<GT>(g, prev, curr);
        if (arc != nullptr)
          path.append(arc);
        prev = curr;
        fwd_it.next();
      }

    // Backward nodes
    for (auto bck_it = bck_nodes.get_it(); bck_it.has_curr(); bck_it.next())
      {
        auto curr = bck_it.get_curr();
        Arc * arc = search_arc<GT>(g, prev, curr);
        if (arc != nullptr)
          path.append(arc);
        prev = curr;
      }
  }

public:
  /** @brief Constructor.
   *
   *  @param[in] __sa Arc filter for iterators.
   */
  Bidirectional_BFS(SA __sa = SA()) : sa(__sa) {}

  /** @brief Constructor with lvalue arc filter.
   *
   *  @param[in] __sa Arc filter for iterators.
   */
  Bidirectional_BFS(SA & __sa) : sa(__sa) {}

  /** @brief Finds the shortest unweighted path between start and end.
   *
   *  Performs bidirectional BFS, expanding alternately from start
   *  (using Fwd_Itor) and from end (using Bck_Itor). Stops when
   *  the two frontiers meet.
   *
   *  @param[in] g The graph.
   *  @param[in] start The starting node.
   *  @param[in] end The destination node.
   *  @param[out] path The shortest path (empty if no path exists).
   *  @return true if a path was found, false otherwise.
   *  @throw domain_error If start or end is nullptr, or g is empty.
   *  @throw bad_alloc If there is not enough memory.
   *
   *  @note Time complexity: O(b^(d/2)) where b is branching factor,
   *        d is the shortest path length.
   */
  bool find_path(const GT & g, Node * start, Node * end, Path<GT> & path)
  {
    ah_domain_error_if(start == nullptr) << "start node cannot be null";
    ah_domain_error_if(end == nullptr) << "end node cannot be null";
    ah_domain_error_if(g.get_num_nodes() == 0) << "graph is empty";

    path.empty();

    if (start == end)
      {
        path.set_graph(g, start);
        return true;
      }

    init(g);

    // Mark start and end
    BIBFS_DIR(start) = FORWARD;
    BIBFS_DIR(end) = BACKWARD;

    DynListQueue<Node *> fwd_queue;
    DynListQueue<Node *> bck_queue;

    fwd_queue.put(start);
    bck_queue.put(end);

    Node * meeting = nullptr;

    while (not fwd_queue.is_empty() or not bck_queue.is_empty())
      {
        // Expand forward frontier
        if (not fwd_queue.is_empty())
          {
            auto curr = fwd_queue.get();

            for (Fwd_Itor<GT, SA> it(curr, sa); it.has_curr(); it.next())
              {
                auto arc = it.get_current_arc();
                auto tgt = g.get_connected_node(arc, curr);

                if (BIBFS_DIR(tgt) == BACKWARD)
                  {
                    // Meeting point found!
                    BIBFS_FWD_PARENT(tgt) = curr;
                    meeting = tgt;
                    goto found;
                  }

                if (BIBFS_DIR(tgt) == UNSEEN)
                  {
                    BIBFS_DIR(tgt) = FORWARD;
                    BIBFS_FWD_PARENT(tgt) = curr;
                    fwd_queue.put(tgt);
                  }
              }
          }

        // Expand backward frontier
        if (not bck_queue.is_empty())
          {
            auto curr = bck_queue.get();

            for (Bck_Itor<GT, SA> it(curr, sa); it.has_curr(); it.next())
              {
                auto arc = it.get_current_arc();
                auto tgt = g.get_connected_node(arc, curr);

                if (BIBFS_DIR(tgt) == FORWARD)
                  {
                    // Meeting point found!
                    BIBFS_BCK_PARENT(tgt) = curr;
                    meeting = tgt;
                    goto found;
                  }

                if (BIBFS_DIR(tgt) == UNSEEN)
                  {
                    BIBFS_DIR(tgt) = BACKWARD;
                    BIBFS_BCK_PARENT(tgt) = curr;
                    bck_queue.put(tgt);
                  }
              }
          }
      }

    // No path found
    cleanup(g);
    return false;

  found:
    build_path(g, start, meeting, end, path);
    cleanup(g);
    return true;
  }

  /** @brief Finds shortest path (operator interface with path output).
   *
   *  @param[in] g The graph.
   *  @param[in] start The starting node.
   *  @param[in] end The destination node.
   *  @param[out] path The shortest path.
   *  @return true if a path was found, false otherwise.
   */
  bool operator()(const GT & g, Node * start, Node * end, Path<GT> & path)
  {
    return find_path(g, start, end, path);
  }

  /** @brief Finds shortest path (operator interface returning path).
   *
   *  @param[in] g The graph.
   *  @param[in] start The starting node.
   *  @param[in] end The destination node.
   *  @return The shortest path (empty if no path exists).
   */
  Path<GT> operator()(const GT & g, Node * start, Node * end)
  {
    Path<GT> path(g);
    find_path(g, start, end, path);
    return path;
  }

#undef BIBFS_INFO
#undef BIBFS_DIR
#undef BIBFS_FWD_PARENT
#undef BIBFS_BCK_PARENT
};

} // end namespace Aleph

# endif // BIDIRECTIONAL_BFS_H
