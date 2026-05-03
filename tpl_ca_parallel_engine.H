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

/** @file tpl_ca_parallel_engine.H
 *  @brief Parallel synchronous engine for cellular automata (Phase 5).
 *
 *  `Parallel_Synchronous_Engine<Lattice, Rule, Neighborhood, Order>` is
 *  the multithreaded counterpart of `Synchronous_Engine`. It preserves
 *  the deterministic, bit-for-bit semantics of the sequential engine
 *  while distributing the per-step work across an `Aleph::ThreadPool`.
 *
 *  Design contract:
 *
 *    - **Bit-exact equivalence**: for any deterministic rule the
 *      trajectory produced by this engine is identical to the one
 *      produced by `Synchronous_Engine` for the same initial lattice.
 *      The synchronous double-buffer pattern guarantees that workers
 *      only read from `current` and only write to `next`, so the
 *      iteration order is irrelevant. Each cell is visited exactly
 *      once per step by exactly one worker.
 *
 *    - **No allocations inside `step()`**: the only heap activity per
 *      step is the bookkeeping intrinsic to the `ThreadPool::enqueue`
 *      machinery (a `std::packaged_task` per partition). The
 *      neighbour-gather buffer used by the rule lives on the worker
 *      stack, exactly like in the sequential engine.
 *
 *    - **Halo-aware**: when the lattice exposes `refresh_halo()` (e.g.
 *      `Ghost_Lattice`), the engine refreshes it serially before each
 *      step. The halo is never written by the rule, so workers can read
 *      it concurrently without synchronisation.
 *
 *    - **Sequential fallback**: when the workload is below
 *      `min_parallel_cells`, or when `num_partitions` collapses to 1,
 *      the engine runs the same loop as the sequential engine without
 *      ever touching the thread pool. This keeps small CAs cheap.
 *
 *  Partitioning strategy: contiguous row strips along axis 0 via
 *  `ca-tiling.H`. For row-major storage (the default for both
 *  `Dense_Cell_Storage` and `Bit_Cell_Storage`) this yields strictly
 *  contiguous memory per worker, which minimises false sharing on
 *  64-byte cache lines.
 *
 *  The lower-dimensional ranks (`Rank == 1` and `Rank == 3`) reuse the
 *  same row partitioning along axis 0. For 1D this means the full
 *  lattice is split into intervals of cells; for 3D it splits the
 *  outermost slab.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef TPL_CA_PARALLEL_ENGINE_H
#define TPL_CA_PARALLEL_ENGINE_H

#include <array>
#include <concepts>
#include <cstddef>
#include <functional>
#include <future>
#include <span>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include <thread_pool.H>

#include <ca-tiling.H>
#include <ca-traits.H>
#include <tpl_ca_bit_storage.H>
#include <tpl_ca_concepts.H>
#include <tpl_ca_neighborhood.H>

namespace Aleph {
namespace CA {

namespace ca_parallel_detail {

/// Detect whether a lattice exposes a `refresh_halo()` method.
template <typename L>
concept HasRefreshHalo = requires(L &l) { l.refresh_halo(); };

/// Detect whether `O` is a `Tile<W, H>` instantiation.
template <typename O>
struct is_tile : std::false_type
{
};

template <std::size_t W, std::size_t H>
struct is_tile<Tile<W, H>> : std::true_type
{
};

template <typename O>
inline constexpr bool is_tile_v = is_tile<O>::value;

template <typename Storage>
struct is_bit_cell_storage : std::false_type
{
};

template <std::size_t N>
struct is_bit_cell_storage<Bit_Cell_Storage<N>> : std::true_type
{
};

template <typename Lattice>
inline constexpr bool uses_bit_cell_storage_v = requires {
  typename Lattice::storage_type;
} and is_bit_cell_storage<typename Lattice::storage_type>::value;

}  // namespace ca_parallel_detail

/** @brief Configuration for `Parallel_Synchronous_Engine`.
 *
 *  Every field has a sensible default, so the structure is mostly
 *  useful for tuning. Pass it to the engine constructor or update the
 *  configuration in place via `set_config()`.
 */
struct Parallel_Engine_Config
{
  /// Thread pool to schedule on. `nullptr` means `Aleph::default_pool()`.
  ThreadPool *pool = nullptr;

  /// Number of partitions per step. `0` means "use `pool->num_threads()`".
  /// Setting this larger than the pool size oversubscribes the pool;
  /// setting it smaller reduces task overhead at the cost of using
  /// fewer cores.
  std::size_t num_partitions = 0;

  /// Workloads with fewer cells than this are executed sequentially
  /// without touching the pool. The default is sized so that the
  /// per-step task overhead never dominates the cell update cost.
  std::size_t min_parallel_cells = 4096;
};

/** @brief Parallel synchronous double-buffered engine.
 *
 *  Same template parameters as `Synchronous_Engine`. The only
 *  user-visible additions are the configuration knobs and the fact
 *  that `step()` schedules work on a thread pool when the lattice is
 *  large enough.
 *
 *  @tparam Lattice      lattice type satisfying `LatticeLike`.
 *  @tparam Rule         rule type satisfying `RuleLike<Rule, Lattice>`.
 *  @tparam Neighborhood neighbourhood type satisfying `NeighborhoodLike`.
 *  @tparam Order        iteration order tag (`RowMajor` or `Tile<W, H>`).
 *
 *  @par Complexity
 *  `step()` is `O((N * S) / P)` wall-clock with `P` workers, where
 *  `N` is the cell count and `S` is the neighbour count.
 *
 *  @par Thread-safety
 *  The engine is not concurrent: only one driving thread should call
 *  `step()` / `run()` at a time. Worker threads inside the pool
 *  cooperate without synchronisation by reading the previous frame
 *  (read-only) and writing into disjoint slices of the next frame.
 *  Bit-packed `Bit_Cell_Storage` writes are safely parallelized
 *  because the underlying bits use atomic operations to avoid data races.
 */
template <typename Lattice, typename Rule, typename Neighborhood, typename Order = RowMajor>
class Parallel_Synchronous_Engine
{
  static_assert(LatticeLike<Lattice>, "Parallel_Synchronous_Engine requires a LatticeLike lattice");
  static_assert(NeighborhoodLike<Neighborhood>,
                "Parallel_Synchronous_Engine requires a NeighborhoodLike neighborhood");
  static_assert(RuleLike<Rule, Lattice>, "Parallel_Synchronous_Engine requires a RuleLike rule");
  static_assert(Neighborhood::rank_v == Lattice::rank, "Neighborhood rank must match lattice rank");

public:
  using lattice_type = Lattice;
  using rule_type = Rule;
  using neighborhood_type = Neighborhood;
  using order_type = Order;

  using state_type = typename Lattice::state_type;
  using coord_type = typename Lattice::coord_type;
  using extents_type = typename Lattice::extents_type;

  /// Lattice dimension.
  static constexpr std::size_t rank = Lattice::rank;
  /// Number of neighbours.
  static constexpr std::size_t neighbour_count = Neighborhood::size_v;

  /// Hook signature: `(step_index, frame)`.
  using hook_type = std::function<void(std::size_t, const Lattice &)>;

private:
  Lattice cur_buf_;
  Lattice nxt_buf_;
  Rule rule_;
  Neighborhood nh_;
  hook_type pre_hook_;
  hook_type post_hook_;
  std::size_t step_count_ = 0;
  Parallel_Engine_Config cfg_{};

  // -------------------------------------------------------------------
  // Per-cell update used by both the sequential fallback and the
  // worker tasks. Captured by value-of-reference; safe to invoke from
  // multiple workers as long as their `c` arguments are disjoint.
  // -------------------------------------------------------------------
  void compute_cell(const coord_type &c) const = delete;  // not used

  /// Update a single cell `c`: gather its neighbours, evaluate the rule
  /// and write the result into `nxt_buf_`. The neighbour buffer is on
  /// the caller's stack to avoid any heap activity per cell. The
  /// `Cell_Context` is derived from the engine's step counter and the
  /// cell coordinate, so contextual rules (e.g. Phase 8 stochastic
  /// rules) see the same `(step, coord)` pair regardless of which
  /// worker visits the cell.
  void update_cell(const coord_type &c, std::span<state_type> nbuf)
  {
    const std::size_t neigh_sz = std::min(nh_.size(), nbuf.size());
    gather_neighbors(nh_, cur_buf_, c, std::span<state_type>(nbuf.data(), neigh_sz));
    const Cell_Context<rank> ctx{step_count_, c};
    nxt_buf_.set(
      c, apply_rule(rule_, cur_buf_.at(c), Neighbor_View<state_type>(nbuf.data(), neigh_sz), ctx));
  }

  // -------------------------------------------------------------------
  // Iteration kernels for each (rank, order) combination. They take a
  // half-open row range `[r_begin, r_end)` so the parallel scheduler
  // can hand each worker a disjoint slab.
  // -------------------------------------------------------------------

  void update_slab_1d(const ca_size_t r_begin, const ca_size_t r_end)
  {
    std::array<state_type, neighbour_count> nbuf{};
    coord_type c{};
    for (ca_size_t i = r_begin; i < r_end; ++i)
      {
        c[0] = static_cast<ca_index_t>(i);
        update_cell(c, nbuf);
      }
  }

  void update_slab_2d_row_major(const ca_size_t r_begin, const ca_size_t r_end)
  {
    std::array<state_type, neighbour_count> nbuf{};
    const ca_size_t cols = cur_buf_.size(1);
    coord_type c{};
    for (ca_size_t i = r_begin; i < r_end; ++i)
      for (ca_size_t j = 0; j < cols; ++j)
        {
          c[0] = static_cast<ca_index_t>(i);
          c[1] = static_cast<ca_index_t>(j);
          update_cell(c, nbuf);
        }
  }

  template <std::size_t TileW, std::size_t TileH>
  void update_slab_2d_tile(const ca_size_t r_begin, const ca_size_t r_end)
  {
    std::array<state_type, neighbour_count> nbuf{};
    const ca_size_t cols = cur_buf_.size(1);
    coord_type c{};
    // Walk tiles strictly inside the assigned row slab; the schedule
    // therefore covers each cell exactly once across all workers.
    for (ca_size_t ti = r_begin; ti < r_end; ti += TileH)
      for (ca_size_t tj = 0; tj < cols; tj += TileW)
        {
          const ca_size_t i_end = ti + TileH < r_end ? ti + TileH : r_end;
          const ca_size_t j_end = tj + TileW < cols ? tj + TileW : cols;
          for (ca_size_t i = ti; i < i_end; ++i)
            for (ca_size_t j = tj; j < j_end; ++j)
              {
                c[0] = static_cast<ca_index_t>(i);
                c[1] = static_cast<ca_index_t>(j);
                update_cell(c, nbuf);
              }
        }
  }

  void update_slab_3d_row_major(const ca_size_t r_begin, const ca_size_t r_end)
  {
    std::array<state_type, neighbour_count> nbuf{};
    const ca_size_t n1 = cur_buf_.size(1);
    const ca_size_t n2 = cur_buf_.size(2);
    coord_type c{};
    for (ca_size_t i = r_begin; i < r_end; ++i)
      for (ca_size_t j = 0; j < n1; ++j)
        for (ca_size_t k = 0; k < n2; ++k)
          {
            c[0] = static_cast<ca_index_t>(i);
            c[1] = static_cast<ca_index_t>(j);
            c[2] = static_cast<ca_index_t>(k);
            update_cell(c, nbuf);
          }
  }

  /// Drive the iteration over a single row slab according to `Order`.
  /// Picked at compile time so workers run a tight, branch-free loop.
  void process_slab(const ca_size_t r_begin, const ca_size_t r_end)
  {
    if constexpr (rank == 1)
      {
        update_slab_1d(r_begin, r_end);
      }
    else if constexpr (rank == 2)
      {
        if constexpr (ca_parallel_detail::is_tile_v<Order>)
          update_slab_2d_tile<Order::width, Order::height>(r_begin, r_end);
        else
          update_slab_2d_row_major(r_begin, r_end);
      }
    else if constexpr (rank == 3)
      {
        update_slab_3d_row_major(r_begin, r_end);
      }
    else
      {
        static_assert(rank <= 3, "Parallel_Synchronous_Engine currently supports rank <= 3");
      }
  }

  /// Resolve the effective number of partitions for the next `step()`.
  /// Falls back to 1 when the pool is unavailable or the lattice is
  /// too small to amortise the scheduling overhead.
  [[nodiscard]] std::size_t effective_partitions(ThreadPool *pool) const noexcept
  {
    if (pool == nullptr)
      return 1;
    const std::size_t pool_workers = pool->num_threads();
    std::size_t parts = cfg_.num_partitions == 0 ? pool_workers : cfg_.num_partitions;
    if (parts == 0)
      parts = 1;

    // Cannot split into more pieces than there are rows along axis 0:
    // an empty range would still cost a task scheduling roundtrip.
    const ca_size_t outer = cur_buf_.size(0);
    if (parts > outer)
      parts = outer == 0 ? 1 : outer;

    if (should_run_sequential(cur_buf_.size(), parts, cfg_.min_parallel_cells))
      return 1;
    return parts;
  }

public:
  /** @brief Build an engine on top of an existing initial lattice.
   *
   *  @param[in] initial initial lattice (consumed by move).
   *  @param[in] r       rule instance.
   *  @param[in] n       neighbourhood instance.
   *  @param[in] cfg     parallel-engine configuration.
   *
   *  @throws std::bad_alloc on next-buffer allocation.
   *
   *  @par Complexity
   *  O(N) for allocating the next buffer.
   */
  Parallel_Synchronous_Engine(Lattice initial, Rule r, Neighborhood n, Parallel_Engine_Config cfg = {})
    : cur_buf_(std::move(initial)), nxt_buf_(cur_buf_.extents()), rule_(std::move(r)),
      nh_(std::move(n)), cfg_(cfg)
  {}

  /** @brief Build an engine with a default-constructed neighbourhood.
   *
   *  @param[in] initial initial lattice (consumed by move).
   *  @param[in] r       rule instance.
   *  @param[in] cfg     parallel-engine configuration.
   *
   *  @throws std::bad_alloc on next-buffer allocation.
   */
  Parallel_Synchronous_Engine(Lattice initial, Rule r, Parallel_Engine_Config cfg = {})
    requires std::default_initializable<Neighborhood>
    : Parallel_Synchronous_Engine(std::move(initial), std::move(r), Neighborhood{}, cfg)
  {}

  /** @brief Return the current frame.
   *  @return read-only view of the current lattice buffer.
   */
  [[nodiscard]] const Lattice &frame() const noexcept
  {
    return cur_buf_;
  }

  /** @brief Return the number of completed steps.
   *  @return the number of `step()` invocations performed so far.
   */
  [[nodiscard]] std::size_t steps_run() const noexcept
  {
    return step_count_;
  }

  /// @return the current frame extents.
  [[nodiscard]] const extents_type &extents() const noexcept
  {
    return cur_buf_.extents();
  }

  /// @return the active configuration.
  [[nodiscard]] const Parallel_Engine_Config &config() const noexcept
  {
    return cfg_;
  }

  /// Replace the configuration. Takes effect on the next `step()`.
  void set_config(Parallel_Engine_Config cfg) noexcept
  {
    cfg_ = cfg;
  }

  /// Register a hook fired *before* every `step()`. Hooks run on the
  /// driving thread, never inside a worker.
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

  /** @brief Apply the rule to every cell once and swap buffers.
   *
   *  Refreshes the halo (when applicable), fires the pre-step hook,
   *  schedules the per-partition work onto the configured thread pool
   *  (or runs sequentially when the workload is below the threshold),
   *  swaps the buffers in O(1), and fires the post-step hook.
   *
   *  @throws Any exception propagated by the rule, lattice or hooks.
   *  @throws std::runtime_error if a worker encountered an exception
   *          during the parallel section.
   *
   *  @par Complexity
   *  O((N * K) / P) wall-clock, where `N` is the cell count, `K` the
   *  neighbourhood size and `P` the effective worker count.
   */
  void step()
  {
    if constexpr (ca_parallel_detail::HasRefreshHalo<Lattice>)
      cur_buf_.refresh_halo();

    if (pre_hook_)
      pre_hook_(step_count_, cur_buf_);

    ThreadPool *pool = cfg_.pool != nullptr ? cfg_.pool : &Aleph::default_pool();
    const std::size_t parts = effective_partitions(pool);
    const ca_size_t outer = cur_buf_.size(0);

    if (parts <= 1)
      {
        // Sequential fast path. Identical iteration to the synchronous
        // engine and to the parallel branch with parts == 1.
        process_slab(0, outer);
      }
    else
      {
        // Parallel branch: enqueue one task per partition and wait for
        // every future. We collect futures up-front so the destructor
        // semantics of `std::future` cannot block us in unexpected
        // orders if the rule throws.
        std::vector<std::future<void>> futures;
        futures.reserve(parts);
        const std::array<ca_size_t, rank> ext = cur_buf_.extents();
        for (std::size_t p = 0; p < parts; ++p)
          {
            const Range1D rng = Row_Partition<rank>::slab(ext, parts, p);
            if (rng.empty())
              continue;
            futures.emplace_back(pool->enqueue([this, b = rng.begin, e = rng.end]
            {
              this->process_slab(b, e);
            }));
          }
        // Phase 1: wait for every worker to finish without re-throwing,
        // so no worker is still writing to nxt_buf_ when we propagate.
        for (auto &f : futures)
          f.wait();
        // Phase 2: collect exceptions; re-throw the first one if any.
        for (auto &f : futures)
          f.get();
      }

    cur_buf_.swap(nxt_buf_);
    ++step_count_;

    if (post_hook_)
      post_hook_(step_count_, cur_buf_);
  }

  /** @brief Run several synchronous steps.
   *
   *  @param[in] steps number of consecutive `step()` calls to perform.
   *
   *  @throws Any exception propagated by `step()`.
   *
   *  @par Complexity
   *  O(steps * N * K / P).
   */
  void run(const std::size_t steps)
  {
    for (std::size_t i = 0; i < steps; ++i)
      step();
  }
};

}  // namespace CA
}  // namespace Aleph

#endif  // TPL_CA_PARALLEL_ENGINE_H
