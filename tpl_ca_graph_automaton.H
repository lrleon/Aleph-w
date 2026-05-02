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

/** @file tpl_ca_graph_automaton.H
 *  @brief CA whose underlying topology is an arbitrary undirected graph.
 *
 *  Phase 6 generalises the rectangular lattice to *network* CAs:
 *
 *    - the cells are the nodes of an undirected graph,
 *    - the neighbourhood of each cell is the set of nodes adjacent
 *      via an edge,
 *    - per-step adjacency is precomputed once at construction time,
 *      so the inner loop is a tight pointer-walk with no graph
 *      traversal overhead.
 *
 *  Two pieces:
 *
 *    - `Graph_Lattice<NodeState>` — owns one `state_type` per node
 *      and a precomputed adjacency list (`Array<Array<size_t>>`).
 *      Satisfies `LatticeLike` with rank `1` so it slots into the
 *      same `Synchronous_Engine` machinery for halo / hooks /
 *      iteration. Strict `at` / `set` use a single-component
 *      coordinate equal to the node id; `at_safe` returns
 *      `state_type{}` for out-of-range ids.
 *
 *    - `Graph_Synchronous_Engine<Lattice, Rule>` — analogue of
 *      `Synchronous_Engine` specialised to graph CAs. Iterates the
 *      nodes in id order and gathers each node's neighbour states
 *      into a small heap-allocated buffer that grows lazily up to
 *      the largest degree seen. Buffers are owned by the engine so
 *      `step()` is allocation-free after the first call.
 *
 *  The header does not depend on `tpl_graph.H`: the user supplies
 *  the adjacency directly, which keeps the test surface small and
 *  lets callers drive the engine from any graph backend (Aleph
 *  `List_Graph`, `random_graph.H`, `grid.H`, or hand-built).
 *
 *  Free helpers `make_grid_graph_adjacency` and
 *  `make_path_graph_adjacency` are provided so common test cases
 *  fit in two lines of user code.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef TPL_CA_GRAPH_AUTOMATON_H
#define TPL_CA_GRAPH_AUTOMATON_H

#include <array>
#include <cstddef>
#include <functional>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

#include <ah-errors.H>
#include <tpl_array.H>

#include <ca-traits.H>
#include <tpl_ca_concepts.H>

namespace Aleph {
namespace CA {

/** @brief Graph lattice: one cell per node + precomputed adjacency.
 *
 *  Satisfies `LatticeLike` as a rank-1 lattice whose `coord_type` is
 *  `Coord_Vec<1>` and whose `extents_type` is `std::array<ca_size_t, 1>`.
 *  Component 0 is interpreted as the node id.
 *
 *  Adjacency is stored as `Array<Array<size_t>>`: row `n` lists the
 *  ids of the neighbours of node `n`. The list is what the engine
 *  hands to the rule, in the order supplied by the constructor.
 *  Self-loops are accepted but have no special semantics: they show
 *  up as the node's own id in its own adjacency list.
 *
 *  @tparam T cell state type, must satisfy `CellState`.
 */
template <typename T>
class Graph_Lattice
{
  static_assert(CellState<T>, "Graph_Lattice requires a CellState type");

public:
  using state_type = T;
  using coord_type = Coord_Vec<1>;
  using extents_type = std::array<ca_size_t, 1>;

  /// Rank-1 lattice: node id is the only axis.
  static constexpr std::size_t rank = 1;

private:
  ca_size_t num_nodes_ = 0;
  Array<T> cells_{};
  Array<Array<std::size_t>> adjacency_{};

public:
  /// Construct an empty graph lattice (0 nodes).
  Graph_Lattice() = default;

  /** @brief Build a graph lattice from an adjacency list.
   *
   *  @param[in] adjacency  one row per node listing neighbour ids.
   *  @param[in] init       initial value for every cell (default `T{}`).
   *
   *  @throws std::out_of_range if a neighbour id is `>= adjacency.size()`.
   *  @throws std::bad_alloc on allocation failure.
   *
   *  @par Complexity
   *  O(N + E), where N is the node count and E the number of edges.
   */
  explicit Graph_Lattice(const std::vector<std::vector<std::size_t>> &adjacency,
                         const T &init = T{})
    : num_nodes_(static_cast<ca_size_t>(adjacency.size()))
    , cells_(num_nodes_, init)
    , adjacency_(num_nodes_, Array<std::size_t>{})
  {
    for (ca_size_t i = 0; i < num_nodes_; ++i)
      {
        const auto &row = adjacency[i];
        Array<std::size_t> r(row.size(), std::size_t{0});
        for (std::size_t k = 0; k < row.size(); ++k)
          {
            ah_out_of_range_error_if(row[k] >= num_nodes_)
              << "Graph_Lattice: neighbour id " << row[k] << " of node " << i
              << " is out of range [0, " << num_nodes_ << ")";
            r[k] = row[k];
          }
        adjacency_[i] = std::move(r);
      }
  }

  // -------------------------------------------------------------
  // LatticeLike surface.
  // -------------------------------------------------------------

  [[nodiscard]] static constexpr std::size_t dimension() noexcept { return rank; }

  [[nodiscard]] ca_size_t size() const noexcept { return num_nodes_; }

  [[nodiscard]] ca_size_t size(std::size_t d) const
  {
    ah_out_of_range_error_if(d >= rank)
      << "Graph_Lattice::size: axis " << d << " out of range";
    return num_nodes_;
  }

  [[nodiscard]] extents_type extents() const noexcept
  {
    return extents_type{num_nodes_};
  }

  [[nodiscard]] T at(const coord_type &c) const
  {
    ah_out_of_range_error_if(c[0] < 0 or static_cast<ca_size_t>(c[0]) >= num_nodes_)
      << "Graph_Lattice::at: id " << c[0] << " out of [0, " << num_nodes_ << ")";
    return cells_[static_cast<ca_size_t>(c[0])];
  }

  void set(const coord_type &c, const T &v)
  {
    ah_out_of_range_error_if(c[0] < 0 or static_cast<ca_size_t>(c[0]) >= num_nodes_)
      << "Graph_Lattice::set: id " << c[0] << " out of [0, " << num_nodes_ << ")";
    cells_[static_cast<ca_size_t>(c[0])] = v;
  }

  /// Boundary-aware read. For graphs the only meaningful interpretation
  /// is "out-of-range cells return the default value", which mirrors
  /// `OpenBoundary` on a rectangular lattice.
  [[nodiscard]] T at_safe(const coord_type &c) const
  {
    if (c[0] < 0 or static_cast<ca_size_t>(c[0]) >= num_nodes_)
      return T{};
    return cells_[static_cast<ca_size_t>(c[0])];
  }

  // -------------------------------------------------------------
  // Graph-specific accessors.
  // -------------------------------------------------------------

  /// Direct read at node id `n`.
  [[nodiscard]] T at_node(std::size_t n) const
  {
    ah_out_of_range_error_if(n >= num_nodes_)
      << "Graph_Lattice::at_node: id " << n << " out of [0, " << num_nodes_ << ")";
    return cells_[n];
  }

  /// Direct write at node id `n`.
  void set_node(std::size_t n, const T &v)
  {
    ah_out_of_range_error_if(n >= num_nodes_)
      << "Graph_Lattice::set_node: id " << n << " out of [0, " << num_nodes_ << ")";
    cells_[n] = v;
  }

  /// Degree of node `n`.
  [[nodiscard]] std::size_t degree(std::size_t n) const
  {
    ah_out_of_range_error_if(n >= num_nodes_)
      << "Graph_Lattice::degree: id " << n << " out of [0, " << num_nodes_ << ")";
    return adjacency_[n].size();
  }

  /// Neighbour ids of node `n` in the order supplied at construction.
  [[nodiscard]] const Array<std::size_t> &neighbours(std::size_t n) const
  {
    ah_out_of_range_error_if(n >= num_nodes_)
      << "Graph_Lattice::neighbours: id " << n << " out of [0, " << num_nodes_ << ")";
    return adjacency_[n];
  }

  /// Maximum degree across the graph (computed in O(N)).
  [[nodiscard]] std::size_t max_degree() const noexcept
  {
    std::size_t m = 0;
    for (ca_size_t i = 0; i < num_nodes_; ++i)
      if (adjacency_[i].size() > m)
        m = adjacency_[i].size();
    return m;
  }

  /// Set every cell to `value`.
  void fill(const T &value)
  {
    for (ca_size_t i = 0; i < num_nodes_; ++i)
      cells_[i] = value;
  }

  /// O(1) swap with another graph lattice of the same type.
  void swap(Graph_Lattice &other) noexcept
  {
    using std::swap;
    swap(num_nodes_, other.num_nodes_);
    cells_.swap(other.cells_);
    adjacency_.swap(other.adjacency_);
  }
};

/// Free-function swap so the lattice plays nicely with std::swap.
template <typename T>
inline void swap(Graph_Lattice<T> &a, Graph_Lattice<T> &b) noexcept
{
  a.swap(b);
}

// -----------------------------------------------------------------------
// Synchronous engine for graph CAs.
// -----------------------------------------------------------------------

/** @brief Concept satisfied by graph rules.
 *
 *  A graph rule consumes the current state of a node and a span over
 *  the states of its neighbours, and produces the next state. The
 *  canonical signature is `(state, Neighbor_View<state>) -> state`, so
 *  any rule that already satisfies `RuleLike` for a rectangular
 *  lattice is reusable for graph CAs (e.g. `Outer_Totalistic_Rule`).
 *
 *  Phase 8 introduces stochastic rules that need a per-cell context to
 *  derive deterministic pseudo-random numbers. Such rules expose the
 *  alternative `(state, Neighbor_View<state>, Cell_Context<1>) -> state`
 *  signature; the engine routes them through `apply_rule()`. The graph
 *  is treated as a rank-1 lattice whose only coordinate component is
 *  the node id.
 */
template <typename R, typename T>
concept GraphRuleLike = CellState<T>
and (requires(const R &r, const T &s, Neighbor_View<T> v) {
       { r(s, v) } -> std::convertible_to<T>;
     }
     or requires(const R &r, const T &s, Neighbor_View<T> v,
                 const Cell_Context<1> &ctx) {
          { r(s, v, ctx) } -> std::convertible_to<T>;
        });

/** @brief Synchronous double-buffered engine for graph CAs.
 *
 *  Mirrors the API of `Synchronous_Engine` (frame, hooks, step, run,
 *  steps_run). The differences are internal: there is no neighbour
 *  template parameter, the per-cell neighbour count is variable
 *  (taken from the precomputed adjacency), and the gather buffer is
 *  a single `std::vector` owned by the engine and reused across cells.
 *
 *  @tparam Lattice graph lattice (typically `Graph_Lattice<T>`).
 *  @tparam Rule    rule type satisfying `GraphRuleLike<Rule, state_type>`.
 *
 *  @par Complexity
 *  `step()` is `O(sum of degrees)` = `O(2|E|)` for an undirected graph.
 *
 *  @par Thread-safety
 *  Single driving thread.
 */
template <typename Lattice, typename Rule>
class Graph_Synchronous_Engine
{
public:
  using lattice_type = Lattice;
  using state_type = typename Lattice::state_type;

private:
  static_assert(GraphRuleLike<Rule, state_type>,
                "Graph_Synchronous_Engine requires a GraphRuleLike rule");

  Lattice cur_buf_;
  Lattice nxt_buf_;
  Rule rule_;
  std::vector<state_type> nbuf_;  ///< reused across cells
  std::function<void(std::size_t, const Lattice &)> pre_hook_;
  std::function<void(std::size_t, const Lattice &)> post_hook_;
  std::size_t step_count_ = 0;

public:
  /** @brief Build an engine on top of an existing graph lattice.
   *
   *  @param[in] initial graph lattice (consumed by move).
   *  @param[in] r       rule instance.
   *
   *  @throws std::bad_alloc on next-buffer allocation.
   *
   *  @par Complexity
   *  O(N + E) for cloning the adjacency into the next buffer.
   */
  Graph_Synchronous_Engine(Lattice initial, Rule r)
    : cur_buf_(std::move(initial)), nxt_buf_(cur_buf_), rule_(std::move(r))
  {
    nbuf_.reserve(cur_buf_.max_degree());
  }

  /// @return read-only view of the current frame.
  [[nodiscard]] const Lattice &frame() const noexcept { return cur_buf_; }

  /// @return number of completed steps.
  [[nodiscard]] std::size_t steps_run() const noexcept { return step_count_; }

  /// Register a hook fired *before* every `step()`.
  template <typename F>
  void on_pre_step(F &&f)
  {
    pre_hook_ = std::forward<F>(f);
  }

  /// Register a hook fired *after* every `step()`.
  template <typename F>
  void on_post_step(F &&f)
  {
    post_hook_ = std::forward<F>(f);
  }

  /** @brief Apply the rule to every node once and swap buffers.
   *
   *  @throws Any exception propagated by the rule or hooks.
   *
   *  @par Complexity
   *  O(N + E).
   */
  void step()
  {
    if (pre_hook_)
      pre_hook_(step_count_, cur_buf_);

    const ca_size_t n = cur_buf_.size();
    for (ca_size_t node = 0; node < n; ++node)
      {
        const auto &neigh_ids = cur_buf_.neighbours(node);
        const std::size_t deg = neigh_ids.size();
        if (nbuf_.size() < deg)
          nbuf_.resize(deg);

        for (std::size_t k = 0; k < deg; ++k)
          nbuf_[k] = cur_buf_.at_node(neigh_ids[k]);

        const Neighbor_View<state_type> view(nbuf_.data(), deg);
        const Cell_Context<1> ctx{step_count_, {static_cast<ca_index_t>(node)}};
        nxt_buf_.set_node(node,
                          apply_rule(rule_, cur_buf_.at_node(node), view, ctx));
      }

    cur_buf_.swap(nxt_buf_);
    ++step_count_;

    if (post_hook_)
      post_hook_(step_count_, cur_buf_);
  }

  /** @brief Run several synchronous steps.
   *  @param[in] steps number of consecutive `step()` calls.
   */
  void run(const std::size_t steps)
  {
    for (std::size_t i = 0; i < steps; ++i)
      step();
  }
};

// -----------------------------------------------------------------------
// Graph builders for common test patterns.
// -----------------------------------------------------------------------

/** @brief Build the adjacency of a 2D 4-neighbour grid graph.
 *
 *  Node id `n = i * cols + j` for cell `(i, j)`. Each interior node
 *  has four neighbours (up, down, left, right). When `periodic` is
 *  set the boundaries wrap (every node has exactly four neighbours).
 *
 *  @param[in] rows     grid rows.
 *  @param[in] cols     grid cols.
 *  @param[in] periodic toroidal boundaries when `true`.
 *  @return adjacency list with `rows * cols` entries.
 *
 *  @par Complexity
 *  O(rows * cols).
 */
[[nodiscard]] inline std::vector<std::vector<std::size_t>> make_grid_graph_adjacency(
  std::size_t rows, std::size_t cols, bool periodic = false)
{
  std::vector<std::vector<std::size_t>> adj(rows * cols);
  auto idx = [cols](std::size_t i, std::size_t j) { return i * cols + j; };
  for (std::size_t i = 0; i < rows; ++i)
    for (std::size_t j = 0; j < cols; ++j)
      {
        auto &row = adj[idx(i, j)];
        // North
        if (i > 0)
          row.push_back(idx(i - 1, j));
        else if (periodic)
          row.push_back(idx(rows - 1, j));
        // South
        if (i + 1 < rows)
          row.push_back(idx(i + 1, j));
        else if (periodic)
          row.push_back(idx(0, j));
        // West
        if (j > 0)
          row.push_back(idx(i, j - 1));
        else if (periodic)
          row.push_back(idx(i, cols - 1));
        // East
        if (j + 1 < cols)
          row.push_back(idx(i, j + 1));
        else if (periodic)
          row.push_back(idx(i, 0));
      }
  return adj;
}

/** @brief Build the adjacency of a path graph with `n` nodes.
 *
 *  Node `k` is adjacent to `k-1` and `k+1` (when those exist). When
 *  `cycle` is `true`, the path closes into a ring (every node has
 *  exactly two neighbours).
 *
 *  @param[in] n     number of nodes.
 *  @param[in] cycle close into a cycle when `true`.
 *  @return adjacency list with `n` entries.
 */
[[nodiscard]] inline std::vector<std::vector<std::size_t>> make_path_graph_adjacency(
  std::size_t n, bool cycle = false)
{
  std::vector<std::vector<std::size_t>> adj(n);
  for (std::size_t i = 0; i < n; ++i)
    {
      if (i > 0)
        adj[i].push_back(i - 1);
      else if (cycle and n > 1)
        adj[i].push_back(n - 1);
      if (i + 1 < n)
        adj[i].push_back(i + 1);
      else if (cycle and n > 1)
        adj[i].push_back(0);
    }
  return adj;
}

}  // namespace CA
}  // namespace Aleph

#endif  // TPL_CA_GRAPH_AUTOMATON_H