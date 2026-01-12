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

/** @file cookie_guard.H
 *  @brief RAII guards for graph node/arc cookies.
 *
 *  This file provides RAII-style guards for managing cookies in graph
 *  algorithms. Cookies are temporary void* fields in nodes and arcs
 *  used by algorithms to store intermediate data.
 *
 *  Two main utilities are provided:
 *
 *  - **Cookie_Guard**: Automatically clears all cookies when the guard
 *    goes out of scope. Useful for exception safety - ensures cookies
 *    are cleaned even if an exception is thrown.
 *
 *  - **Cookie_Saver**: Saves current cookie values and restores them
 *    when the guard goes out of scope. Useful when an algorithm needs
 *    to temporarily use cookies without destroying data from a previous
 *    algorithm.
 *
 *  @par Example - Cookie_Guard (cleanup on exit)
 *  @code
 *  template <class GT>
 *  void my_algorithm(GT & g)
 *  {
 *    Cookie_Guard<GT> guard(g);  // will clear cookies on exit
 *    
 *    // Use cookies for algorithm...
 *    for_each_node(g, [](auto * p) {
 *      NODE_COOKIE(p) = new MyData();
 *    });
 *    
 *    // ... algorithm logic that might throw ...
 *    
 *  } // guard destructor clears all cookies (and optionally deletes)
 *  @endcode
 *
 *  @par Example - Cookie_Saver (preserve and restore)
 *  @code
 *  template <class GT>
 *  void nested_algorithm(GT & g)
 *  {
 *    // Another algorithm already stored data in cookies
 *    Cookie_Saver<GT> saver(g);  // saves current cookies
 *    
 *    // Now we can use cookies freely
 *    for_each_node(g, [](auto * p) {
 *      NODE_COOKIE(p) = new TempData();
 *    });
 *    
 *    // ... do work ...
 *    
 *    // Clean up our temporary data
 *    for_each_node(g, [](auto * p) {
 *      delete static_cast<TempData*>(NODE_COOKIE(p));
 *    });
 *    
 *  } // saver destructor restores original cookies
 *  @endcode
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

#ifndef COOKIE_GUARD_H
#define COOKIE_GUARD_H

#include <vector>
#include <utility>
#include <tpl_graph.H>

namespace Aleph {

/** @brief RAII guard that clears graph cookies on destruction.
 *
 *  This class provides automatic cleanup of node and arc cookies
 *  when the guard goes out of scope. This is essential for exception
 *  safety in graph algorithms.
 *
 *  @tparam GT Graph type (must have Node and Arc types with cookies).
 *
 *  @par Thread Safety
 *  Not thread-safe. Each thread should use its own guard.
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */
template <class GT>
class Cookie_Guard
{
public:
  using Node = typename GT::Node;
  using Arc = typename GT::Arc;

  /// Signature for custom node cookie deleter
  using Node_Deleter = std::function<void(Node*)>;
  
  /// Signature for custom arc cookie deleter
  using Arc_Deleter = std::function<void(Arc*)>;

private:
  GT & graph;
  bool clear_nodes;
  bool clear_arcs;
  Node_Deleter node_deleter;
  Arc_Deleter arc_deleter;
  bool active;

public:
  /** @brief Construct a cookie guard for the given graph.
   *
   *  @param g Graph to guard (const_cast applied - cookies are mutable).
   *  @param _clear_nodes Whether to clear node cookies (default: true).
   *  @param _clear_arcs Whether to clear arc cookies (default: true).
   */
  Cookie_Guard(const GT & g, bool _clear_nodes = true, bool _clear_arcs = true) noexcept
    : graph(const_cast<GT &>(g)), clear_nodes(_clear_nodes), clear_arcs(_clear_arcs),
      node_deleter(nullptr), arc_deleter(nullptr), active(true)
  {}

  /** @brief Construct with custom deleters for cookie data.
   *
   *  Use this constructor when cookies point to dynamically allocated
   *  data that needs to be freed.
   *
   *  @param g Graph to guard (const_cast applied - cookies are mutable).
   *  @param nd Node deleter function (called for each node before clearing).
   *  @param ad Arc deleter function (called for each arc before clearing).
   *
   *  @par Example
   *  @code
   *  Cookie_Guard<GT> guard(g,
   *    [](Node* p) { delete static_cast<MyNodeData*>(NODE_COOKIE(p)); },
   *    [](Arc* a) { delete static_cast<MyArcData*>(ARC_COOKIE(a)); }
   *  );
   *  @endcode
   */
  Cookie_Guard(const GT & g, Node_Deleter nd, Arc_Deleter ad = nullptr) noexcept
    : graph(const_cast<GT &>(g)), clear_nodes(true), clear_arcs(ad != nullptr),
      node_deleter(std::move(nd)), arc_deleter(std::move(ad)), active(true)
  {}

  /// Copying disabled
  Cookie_Guard(const Cookie_Guard &) = delete;
  Cookie_Guard & operator=(const Cookie_Guard &) = delete;

  /// Move construction transfers ownership
  Cookie_Guard(Cookie_Guard && other) noexcept
    : graph(other.graph), clear_nodes(other.clear_nodes),
      clear_arcs(other.clear_arcs), node_deleter(std::move(other.node_deleter)),
      arc_deleter(std::move(other.arc_deleter)), active(other.active)
  {
    other.active = false;
  }

  /// Move assignment transfers ownership
  Cookie_Guard & operator=(Cookie_Guard && other) noexcept
  {
    if (this != &other)
      {
        release();
        clear_nodes = other.clear_nodes;
        clear_arcs = other.clear_arcs;
        node_deleter = std::move(other.node_deleter);
        arc_deleter = std::move(other.arc_deleter);
        active = other.active;
        other.active = false;
      }
    return *this;
  }

  /** @brief Release the guard without clearing cookies.
   *
   *  Call this if you want to keep the cookies after the guard
   *  goes out of scope (e.g., algorithm completed successfully
   *  and results are stored in cookies).
   */
  void release() noexcept
  {
    active = false;
  }

  /** @brief Manually trigger cleanup now.
   *
   *  Clears cookies immediately and deactivates the guard.
   */
  void clear_now()
  {
    if (not active)
      return;

    if (clear_nodes)
      {
        for (auto it = graph.get_node_it(); it.has_curr(); it.next_ne())
          {
            Node * p = it.get_curr();
            if (node_deleter)
              node_deleter(p);
            NODE_COOKIE(p) = nullptr;
          }
      }

    if (clear_arcs)
      {
        for (auto it = graph.get_arc_it(); it.has_curr(); it.next_ne())
          {
            Arc * a = it.get_curr();
            if (arc_deleter)
              arc_deleter(a);
            ARC_COOKIE(a) = nullptr;
          }
      }

    active = false;
  }

  /** @brief Destructor - clears cookies if guard is still active. */
  ~Cookie_Guard()
  {
    clear_now();
  }
};


/** @brief RAII guard that saves and restores graph cookies.
 *
 *  This class saves the current cookie values when constructed and
 *  restores them when destroyed. Useful when an algorithm needs to
 *  temporarily use cookies without losing data from a previous algorithm.
 *
 *  @tparam GT Graph type (must have Node and Arc types with cookies).
 *
 *  @warning This class stores pointers to the original cookie values.
 *           If the graph structure changes (nodes/arcs added/removed)
 *           between save and restore, behavior is undefined.
 *
 *  @par Thread Safety
 *  Not thread-safe. Each thread should use its own saver.
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */
template <class GT>
class Cookie_Saver
{
public:
  using Node = typename GT::Node;
  using Arc = typename GT::Arc;

  /// Signature for custom cleanup of temporary node cookies
  using Node_Cleanup = std::function<void(Node*)>;
  
  /// Signature for custom cleanup of temporary arc cookies
  using Arc_Cleanup = std::function<void(Arc*)>;

private:
  GT & graph;
  std::vector<std::pair<Node*, void*>> saved_node_cookies;
  std::vector<std::pair<Arc*, void*>> saved_arc_cookies;
  bool save_nodes;
  bool save_arcs;
  Node_Cleanup node_cleanup;
  Arc_Cleanup arc_cleanup;
  bool active;

public:
  /** @brief Construct a cookie saver for the given graph.
   *
   *  Immediately saves all current cookie values.
   *
   *  @param g Graph to save cookies from (const_cast applied - cookies are mutable).
   *  @param _save_nodes Whether to save node cookies (default: true).
   *  @param _save_arcs Whether to save arc cookies (default: true).
   */
  Cookie_Saver(const GT & g, bool _save_nodes = true, bool _save_arcs = true)
    : graph(const_cast<GT &>(g)), save_nodes(_save_nodes), save_arcs(_save_arcs),
      node_cleanup(nullptr), arc_cleanup(nullptr), active(true)
  {
    save_cookies();
  }

  /** @brief Construct with custom cleanup for temporary cookies.
   *
   *  Use this when your algorithm allocates data in cookies that
   *  needs to be freed before restoring the original cookies.
   *
   *  @param g Graph to save cookies from (const_cast applied - cookies are mutable).
   *  @param nc Node cleanup function (called before restoring).
   *  @param ac Arc cleanup function (called before restoring).
   *
   *  @par Example
   *  @code
   *  Cookie_Saver<GT> saver(g,
   *    [](Node* p) { delete static_cast<TempData*>(NODE_COOKIE(p)); },
   *    nullptr  // no arc cleanup needed
   *  );
   *  @endcode
   */
  Cookie_Saver(const GT & g, Node_Cleanup nc, Arc_Cleanup ac = nullptr)
    : graph(const_cast<GT &>(g)), save_nodes(true), save_arcs(ac != nullptr),
      node_cleanup(std::move(nc)), arc_cleanup(std::move(ac)), active(true)
  {
    save_cookies();
  }

  /// Copying disabled
  Cookie_Saver(const Cookie_Saver &) = delete;
  Cookie_Saver & operator=(const Cookie_Saver &) = delete;

  /// Move construction transfers ownership
  Cookie_Saver(Cookie_Saver && other) noexcept
    : graph(other.graph),
      saved_node_cookies(std::move(other.saved_node_cookies)),
      saved_arc_cookies(std::move(other.saved_arc_cookies)),
      save_nodes(other.save_nodes), save_arcs(other.save_arcs),
      node_cleanup(std::move(other.node_cleanup)),
      arc_cleanup(std::move(other.arc_cleanup)),
      active(other.active)
  {
    other.active = false;
  }

  /// Move assignment transfers ownership
  Cookie_Saver & operator=(Cookie_Saver && other) noexcept
  {
    if (this != &other)
      {
        restore_now();
        saved_node_cookies = std::move(other.saved_node_cookies);
        saved_arc_cookies = std::move(other.saved_arc_cookies);
        save_nodes = other.save_nodes;
        save_arcs = other.save_arcs;
        node_cleanup = std::move(other.node_cleanup);
        arc_cleanup = std::move(other.arc_cleanup);
        active = other.active;
        other.active = false;
      }
    return *this;
  }

  /** @brief Discard saved cookies without restoring.
   *
   *  Call this if you want to keep the new cookie values instead
   *  of restoring the original ones.
   */
  void discard() noexcept
  {
    active = false;
    saved_node_cookies.clear();
    saved_arc_cookies.clear();
  }

  /** @brief Manually restore cookies now.
   *
   *  Restores original cookies immediately and deactivates the saver.
   */
  void restore_now()
  {
    if (not active)
      return;

    // First, cleanup current (temporary) cookies if cleanup function provided
    if (node_cleanup)
      {
        for (auto it = graph.get_node_it(); it.has_curr(); it.next_ne())
          node_cleanup(it.get_curr());
      }

    if (arc_cleanup)
      {
        for (auto it = graph.get_arc_it(); it.has_curr(); it.next_ne())
          arc_cleanup(it.get_curr());
      }

    // Restore saved cookies
    for (auto & [node, cookie] : saved_node_cookies)
      NODE_COOKIE(node) = cookie;

    for (auto & [arc, cookie] : saved_arc_cookies)
      ARC_COOKIE(arc) = cookie;

    active = false;
    saved_node_cookies.clear();
    saved_arc_cookies.clear();
  }

  /** @brief Get access to saved node cookies.
   *
   *  @return Const reference to vector of (node, saved_cookie) pairs.
   */
  [[nodiscard]] const std::vector<std::pair<Node*, void*>> & 
  get_saved_node_cookies() const noexcept
  {
    return saved_node_cookies;
  }

  /** @brief Get access to saved arc cookies.
   *
   *  @return Const reference to vector of (arc, saved_cookie) pairs.
   */
  [[nodiscard]] const std::vector<std::pair<Arc*, void*>> & 
  get_saved_arc_cookies() const noexcept
  {
    return saved_arc_cookies;
  }

  /** @brief Destructor - restores cookies if saver is still active. */
  ~Cookie_Saver()
  {
    restore_now();
  }

private:
  void save_cookies()
  {
    if (save_nodes)
      {
        saved_node_cookies.reserve(graph.get_num_nodes());
        for (auto it = graph.get_node_it(); it.has_curr(); it.next_ne())
          {
            Node * p = it.get_curr();
            saved_node_cookies.emplace_back(p, NODE_COOKIE(p));
          }
      }

    if (save_arcs)
      {
        saved_arc_cookies.reserve(graph.get_num_arcs());
        for (auto it = graph.get_arc_it(); it.has_curr(); it.next_ne())
          {
            Arc * a = it.get_curr();
            saved_arc_cookies.emplace_back(a, ARC_COOKIE(a));
          }
      }
  }
};


/** @brief Convenience function to run an algorithm with automatic cookie cleanup.
 *
 *  Executes the given callable with automatic cookie cleanup on exit
 *  (whether normal return or exception).
 *
 *  @tparam GT Graph type.
 *  @tparam Func Callable type.
 *  @param g Graph to operate on.
 *  @param func Function/lambda to execute.
 *  @return Result of func().
 *
 *  @par Example
 *  @code
 *  auto result = with_clean_cookies(g, [&]() {
 *    // Algorithm that uses cookies...
 *    return compute_something(g);
 *  });
 *  @endcode
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */
template <class GT, class Func>
auto with_clean_cookies(GT & g, Func && func) -> decltype(func())
{
  Cookie_Guard<GT> guard(g);
  return std::forward<Func>(func)();
}


/** @brief Convenience function to run an algorithm preserving existing cookies.
 *
 *  Executes the given callable, automatically saving cookies before
 *  and restoring them after (whether normal return or exception).
 *
 *  @tparam GT Graph type.
 *  @tparam Func Callable type.
 *  @param g Graph to operate on.
 *  @param func Function/lambda to execute.
 *  @return Result of func().
 *
 *  @par Example
 *  @code
 *  // Cookies already contain important data from previous algorithm
 *  auto result = with_saved_cookies(g, [&]() {
 *    // Temporary algorithm that uses cookies...
 *    return compute_temporary(g);
 *  });
 *  // Original cookies are restored
 *  @endcode
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */
template <class GT, class Func>
auto with_saved_cookies(GT & g, Func && func) -> decltype(func())
{
  Cookie_Saver<GT> saver(g);
  return std::forward<Func>(func)();
}


/** @brief Generic RAII scope guard for cleanup operations on graphs.
 *
 *  Executes a cleanup function when the guard goes out of scope,
 *  unless explicitly released. Useful for ensuring cleanup happens
 *  even if exceptions occur.
 *
 *  @tparam GT Graph type.
 *  @tparam Cleanup Callable type for cleanup (e.g., lambda, functor).
 *
 *  @par Example
 *  @code
 *  // Initialize graph cookies
 *  init_algorithm(g);
 *
 *  // Guard ensures cleanup even if exception occurs
 *  Scope_Guard guard(g, [](const GT & graph) {
 *    uninit_algorithm(graph);
 *  });
 *
 *  // ... algorithm code that might throw ...
 *
 *  // Destructor calls uninit_algorithm(g) automatically
 *  @endcode
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */
template <class GT, class Cleanup>
class Scope_Guard
{
  const GT & graph;
  Cleanup cleanup_fn;
  bool active;

public:
  /** @brief Construct a scope guard.
   *
   *  @param g Graph to pass to cleanup function.
   *  @param cleanup Function to call on destruction.
   */
  Scope_Guard(const GT & g, Cleanup cleanup)
    : graph(g), cleanup_fn(std::move(cleanup)), active(true)
  {}

  /// Copying disabled
  Scope_Guard(const Scope_Guard &) = delete;
  Scope_Guard & operator=(const Scope_Guard &) = delete;

  /// Move construction transfers ownership
  Scope_Guard(Scope_Guard && other) noexcept
    : graph(other.graph), cleanup_fn(std::move(other.cleanup_fn)),
      active(other.active)
  {
    other.active = false;
  }

  /** @brief Destructor - calls cleanup function if still active. */
  ~Scope_Guard()
  {
    if (active)
      cleanup_fn(graph);
  }

  /** @brief Release the guard without calling cleanup.
   *
   *  Use this if cleanup was performed manually or is no longer needed.
   */
  void release() noexcept { active = false; }

  /** @brief Check if guard is still active. */
  [[nodiscard]] bool is_active() const noexcept { return active; }

  /** @brief Execute cleanup immediately and deactivate.
   *
   *  Safe to call multiple times - only first call executes cleanup.
   */
  void cleanup_now()
  {
    if (active)
      {
        cleanup_fn(graph);
        active = false;
      }
  }
};

/// Deduction guide for Scope_Guard
template <class GT, class Cleanup>
Scope_Guard(const GT &, Cleanup) -> Scope_Guard<GT, Cleanup>;


} // end namespace Aleph

#endif // COOKIE_GUARD_H

