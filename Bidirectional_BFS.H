
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

# include <limits>
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
  static constexpr size_t INF_DIST = std::numeric_limits<size_t>::max();

  struct BFS_Info
  {
    Node * fwd_parent = nullptr;
    Node * bck_parent = nullptr;
    Arc * fwd_parent_arc = nullptr;
    Arc * bck_parent_arc = nullptr;
    size_t fwd_dist = INF_DIST;
    size_t bck_dist = INF_DIST;
    int dir = UNSEEN;
  };

#define BIBFS_INFO(p)       (static_cast<BFS_Info *>(NODE_COOKIE(p)))
#define BIBFS_DIR(p)        (BIBFS_INFO(p)->dir)
#define BIBFS_FWD_PARENT(p) (BIBFS_INFO(p)->fwd_parent)
#define BIBFS_BCK_PARENT(p) (BIBFS_INFO(p)->bck_parent)
#define BIBFS_FWD_PARC(p)   (BIBFS_INFO(p)->fwd_parent_arc)
#define BIBFS_BCK_PARC(p)   (BIBFS_INFO(p)->bck_parent_arc)
#define BIBFS_FWD_DIST(p)   (BIBFS_INFO(p)->fwd_dist)
#define BIBFS_BCK_DIST(p)   (BIBFS_INFO(p)->bck_dist)

  class BiBFS_Init_Guard
  {
    Bidirectional_BFS * owner = nullptr;
    const GT * g = nullptr;

  public:
    BiBFS_Init_Guard(Bidirectional_BFS & __owner, const GT & __g) noexcept
      : owner(&__owner), g(&__g)
    {
      // empty
    }

    BiBFS_Init_Guard(const BiBFS_Init_Guard &) = delete;
    BiBFS_Init_Guard & operator = (const BiBFS_Init_Guard &) = delete;
    BiBFS_Init_Guard(BiBFS_Init_Guard &&) = delete;
    BiBFS_Init_Guard & operator = (BiBFS_Init_Guard &&) = delete;

    ~BiBFS_Init_Guard() noexcept
    {
      owner->cleanup(*g);
    }
  };

  void init(const GT & g)
  {
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next())
      NODE_COOKIE(it.get_curr()) = nullptr;

    try
      {
        for (typename GT::Node_Iterator it(g); it.has_curr(); it.next())
          {
            auto p = it.get_curr();
            NODE_COOKIE(p) = new BFS_Info;
          }
      }
    catch (...)
      {
        cleanup(g);
        throw;
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

  [[nodiscard]] bool is_seen(Node * p, const int dir) const noexcept
  {
    return (BIBFS_DIR(p) & dir) != 0;
  }

  void mark_seen(Node * p, const int dir) noexcept
  {
    BIBFS_DIR(p) |= dir;
  }

  [[nodiscard]] size_t get_dist(Node * p, const int dir) const noexcept
  {
    return dir == FORWARD ? BIBFS_FWD_DIST(p) : BIBFS_BCK_DIST(p);
  }

  void set_dist(Node * p, const int dir, const size_t d) noexcept
  {
    if (dir == FORWARD)
      BIBFS_FWD_DIST(p) = d;
    else
      BIBFS_BCK_DIST(p) = d;
  }

  Node *& parent_ref(Node * p, const int dir) noexcept
  {
    if (dir == FORWARD)
      return BIBFS_FWD_PARENT(p);
    return BIBFS_BCK_PARENT(p);
  }

  Arc *& parent_arc_ref(Node * p, const int dir) noexcept
  {
    if (dir == FORWARD)
      return BIBFS_FWD_PARC(p);
    return BIBFS_BCK_PARC(p);
  }

  template <template <typename, class> class Itor>
  void expand_frontier(const GT & g, DynListQueue<Node *> & frontier,
                       const int my_dir, const int other_dir,
                       size_t & best_len, Node *& best_meeting)
  {
    DynListQueue<Node *> next_frontier;

    while (not frontier.is_empty())
      {
        auto curr = frontier.get();
        const auto curr_dist = get_dist(curr, my_dir);

        for (Itor<GT, SA> it(curr, sa); it.has_curr(); it.next())
          {
            auto arc = it.get_current_arc();
            auto tgt = g.get_connected_node(arc, curr);

            const bool seen_my = is_seen(tgt, my_dir);
            if (not seen_my)
              {
                mark_seen(tgt, my_dir);
                parent_ref(tgt, my_dir) = curr;
                parent_arc_ref(tgt, my_dir) = arc;
                set_dist(tgt, my_dir, curr_dist + 1);
                next_frontier.put(tgt);
              }

            if (is_seen(tgt, other_dir))
              {
                const auto my_dist = get_dist(tgt, my_dir);
                const auto other_dist = get_dist(tgt, other_dir);

                if (my_dist != INF_DIST and other_dist != INF_DIST)
                  {
                    const auto candidate_len = my_dist + other_dist;
                    if (candidate_len < best_len)
                      {
                        best_len = candidate_len;
                        best_meeting = tgt;
                      }
                  }
              }
          }
      }

    frontier.swap(next_frontier);
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

    path.set_graph(g, start);

    // Reconstruct start -> meeting using stored forward parent arcs.
    DynList<Arc *> fwd_arcs;
    for (auto curr = meeting; curr != start; curr = BIBFS_FWD_PARENT(curr))
      {
        ah_runtime_error_if(BIBFS_FWD_PARENT(curr) == nullptr)
          << "Bidirectional_BFS: inconsistent forward parent chain";
        ah_runtime_error_if(BIBFS_FWD_PARC(curr) == nullptr)
          << "Bidirectional_BFS: missing forward parent arc";
        fwd_arcs.insert(BIBFS_FWD_PARC(curr)); // prepend for start->meeting order
      }

    for (auto it = fwd_arcs.get_it(); it.has_curr(); it.next())
      path.append(it.get_curr());

    // Reconstruct meeting -> end using stored backward parent arcs.
    for (auto curr = meeting; curr != end; curr = BIBFS_BCK_PARENT(curr))
      {
        ah_runtime_error_if(BIBFS_BCK_PARENT(curr) == nullptr)
          << "Bidirectional_BFS: inconsistent backward parent chain";
        ah_runtime_error_if(BIBFS_BCK_PARC(curr) == nullptr)
          << "Bidirectional_BFS: missing backward parent arc";
        path.append(BIBFS_BCK_PARC(curr));
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
   *  Performs bidirectional BFS by expanding one complete frontier level
   *  at a time. At each iteration the smaller frontier is expanded
   *  (`Fwd_Itor` from start or `Bck_Itor` from end). Meeting detection
   *  uses forward/backward distance maps and stops only when no shorter
   *  meeting point can still exist.
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
    BiBFS_Init_Guard guard(*this, g);

    bool found = false;
    Node * meeting = nullptr;

    mark_seen(start, FORWARD);
    mark_seen(end, BACKWARD);
    set_dist(start, FORWARD, 0);
    set_dist(end, BACKWARD, 0);

    DynListQueue<Node *> fwd_frontier;
    DynListQueue<Node *> bck_frontier;

    fwd_frontier.put(start);
    bck_frontier.put(end);

    size_t fwd_depth = 0;
    size_t bck_depth = 0;
    size_t best_len = INF_DIST;

    while (not fwd_frontier.is_empty() and not bck_frontier.is_empty())
      {
        const bool expand_forward = fwd_frontier.size() <= bck_frontier.size();

        if (expand_forward)
          {
            expand_frontier<Fwd_Itor>(g, fwd_frontier, FORWARD, BACKWARD,
                                      best_len, meeting);
            ++fwd_depth;
          }
        else
          {
            expand_frontier<Bck_Itor>(g, bck_frontier, BACKWARD, FORWARD,
                                      best_len, meeting);
            ++bck_depth;
          }

        if (meeting != nullptr)
          {
            const size_t min_depth = fwd_depth < bck_depth ? fwd_depth : bck_depth;
            const size_t lower_bound = min_depth + 1;
            if (lower_bound >= best_len)
              break;
          }
      }

    found = meeting != nullptr;
    if (found)
      build_path(g, start, meeting, end, path);

    return found;
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
#undef BIBFS_FWD_PARC
#undef BIBFS_BCK_PARC
#undef BIBFS_FWD_DIST
#undef BIBFS_BCK_DIST
};

} // end namespace Aleph

# endif // BIDIRECTIONAL_BFS_H
