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

/** @file ca-observer.H
 *  @brief Phase-7 observability layer for cellular automata engines.
 *
 *  Observers are passive listeners that receive a `(step, frame)`
 *  notification before and after every step, without modifying the
 *  rule, the lattice or the engine. They are the primary way to
 *  collect time series, compute metrics, snapshot frames or detect
 *  cycles without poking at the simulation.
 *
 *  Catalogue:
 *
 *    - `ObserverLike<O, Lattice>` — concept that any observer must
 *      satisfy: `on_step_begin(step, frame)` and
 *      `on_step_end(step, frame)`.
 *    - `Composite_Observer<...>`  — variadic adaptor that delegates
 *      both callbacks to a tuple of observers.
 *    - `Density_Observer<State>`  — records the count of cells in
 *      a chosen state per step.
 *    - `Activity_Observer`        — records the number of cells that
 *      changed value across each step (delta with the previous frame).
 *    - `Entropy_Observer<MaxState>` — Shannon entropy of the integer
 *      state distribution per step.
 *    - `Stationary_Detector`      — keeps a sliding window of the
 *      last K frame hashes and reports the cycle length the moment a
 *      new frame hashes equal to one already seen.
 *    - `Sampling_Observer<Lattice>` — copies the current frame every
 *      `period` steps into an in-memory snapshot vector.
 *
 *  Wiring helpers:
 *
 *    - `attach_observer(engine, observer)` plugs an observer into the
 *      pre/post-step hooks of any engine that exposes
 *      `on_pre_step` / `on_post_step` (Synchronous_Engine,
 *      Parallel_Synchronous_Engine, Graph_Synchronous_Engine).
 *
 *  All observers are header-only and stateful by design (they own
 *  their own time series). They do not allocate inside their
 *  callbacks except where the spec requires it (e.g.
 *  `Sampling_Observer` clones the frame).
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef CA_OBSERVER_H
#define CA_OBSERVER_H

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

#include <ah-errors.H>

#include <ca-metrics.H>
#include <ca-traits.H>
#include <tpl_ca_concepts.H>

namespace Aleph {
namespace CA {

/** @brief Concept satisfied by any cellular-automaton observer.
 *
 *  An observer responds to two engine events with the signatures
 *  `on_step_begin(std::size_t step, const Lattice& frame)` and
 *  `on_step_end(std::size_t step, const Lattice& frame)`. The first
 *  is fired with the index of the step about to run; the second
 *  with the index that just completed.
 *
 *  @tparam O       observer type.
 *  @tparam Lattice lattice type the observer is going to inspect.
 */
template <typename O, typename Lattice>
concept ObserverLike = requires(O &o, std::size_t s, const Lattice &f) {
  o.on_step_begin(s, f);
  o.on_step_end(s, f);
};

// ---------------------------------------------------------------------------
// Composite observer.
// ---------------------------------------------------------------------------

/** @brief Variadic adaptor that fan-outs notifications to many observers.
 *
 *  `Composite_Observer<O1, O2, ...>` owns its child observers by
 *  value (move-constructed at build time) and forwards
 *  `on_step_begin` / `on_step_end` to each one in declaration order.
 *  This lets the engine's single pre/post hook host an arbitrary
 *  number of independent observers.
 *
 *  @tparam Observers types of the child observers.
 */
template <typename... Observers>
class Composite_Observer
{
  std::tuple<Observers...> obs_;

public:
  /// Build the composite from the child observers. Reference types
  /// in `Observers...` are preserved (the composite borrows them);
  /// value types are moved into the tuple.
  explicit Composite_Observer(Observers... observers) : obs_(std::forward<Observers>(observers)...)
  {}

  /// Read-only access to a child observer (by index).
  template <std::size_t I>
  [[nodiscard]] const auto &get() const noexcept
  {
    return std::get<I>(obs_);
  }

  /// Mutable access to a child observer (for reading time series).
  template <std::size_t I>
  [[nodiscard]] auto &get() noexcept
  {
    return std::get<I>(obs_);
  }

  /// Number of child observers.
  [[nodiscard]] static constexpr std::size_t size() noexcept
  {
    return sizeof...(Observers);
  }

  template <typename Lattice>
  void on_step_begin(std::size_t step, const Lattice &frame)
  {
    std::apply([&](auto &...o)
    {
      (o.on_step_begin(step, frame), ...);
    }, obs_);
  }

  template <typename Lattice>
  void on_step_end(std::size_t step, const Lattice &frame)
  {
    std::apply([&](auto &...o)
    {
      (o.on_step_end(step, frame), ...);
    }, obs_);
  }
};

/// Helper that deduces template arguments for `Composite_Observer`.
template <typename... Observers>
[[nodiscard]] inline auto make_composite_observer(Observers... observers)
{
  return Composite_Observer<Observers...>(std::forward<Observers>(observers)...);
}

// ---------------------------------------------------------------------------
// Engine plumbing.
// ---------------------------------------------------------------------------

/** @brief Plug an observer into an engine's pre/post-step hooks.
 *
 *  Captures `observer` by reference: the caller owns the observer's
 *  lifetime, which must outlive the engine. The wiring uses the
 *  existing `on_pre_step` / `on_post_step` hooks, so the parallel
 *  engine and the graph engine are supported transparently.
 *
 *  @param[in,out] engine   engine instance.
 *  @param[in,out] observer observer instance to wire.
 */
template <typename Engine, typename Observer>
inline void attach_observer(Engine &engine, Observer &observer)
{
  using Lat = typename Engine::lattice_type;
  engine.on_pre_step([&observer](std::size_t step, const Lat &frame)
  {
    observer.on_step_begin(step, frame);
  });
  engine.on_post_step([&observer](std::size_t step, const Lat &frame)
  {
    observer.on_step_end(step, frame);
  });
}

/// Overload for `Graph_Synchronous_Engine`, which does not expose
/// the `lattice_type` typedef. Falls back to template deduction
/// through the hook signatures.
template <typename Engine, typename Observer>
inline void attach_observer_graph(Engine &engine, Observer &observer)
{
  engine.on_pre_step([&observer](std::size_t step, const auto &frame)
  {
    observer.on_step_begin(step, frame);
  });
  engine.on_post_step([&observer](std::size_t step, const auto &frame)
  {
    observer.on_step_end(step, frame);
  });
}

// ---------------------------------------------------------------------------
// Density observer.
// ---------------------------------------------------------------------------

/** @brief Records the count of cells in `tracked_state` per step.
 *
 *  At the end of every step the observer pushes one entry into its
 *  history. The very first sample is recorded inside `on_step_begin`
 *  for `step == 0`, so the time series includes the initial frame.
 *
 *  @tparam State cell state type to track.
 */
template <typename State>
class Density_Observer
{
  State tracked_state_{};
  std::vector<ca_size_t> counts_;
  std::vector<ca_size_t> totals_;
  bool seeded_ = false;

public:
  /// Default-constructed observer tracks `State{}`.
  Density_Observer() = default;

  /// Observer tracking a specific state value.
  explicit Density_Observer(State s) : tracked_state_(std::move(s)) {}

  /// @return the time series of counts (one per recorded step,
  /// including the initial frame).
  [[nodiscard]] const std::vector<ca_size_t> &counts() const noexcept
  {
    return counts_;
  }

  /// @return the time series of total cell counts (mirrors `counts`).
  [[nodiscard]] const std::vector<ca_size_t> &totals() const noexcept
  {
    return totals_;
  }

  /// @return density at sample `i` in `[0, 1]`.
  [[nodiscard]] double density_at(std::size_t i) const
  {
    ah_out_of_range_error_if(i >= counts_.size())
      << "Density_Observer: index " << i << " out of range";
    return totals_[i] == 0 ? 0.0 : static_cast<double>(counts_[i]) / static_cast<double>(totals_[i]);
  }

  /// @return number of recorded samples.
  [[nodiscard]] std::size_t size() const noexcept
  {
    return counts_.size();
  }

  /// Reset the time series.
  void reset() noexcept
  {
    counts_.clear();
    totals_.clear();
    seeded_ = false;
  }

  template <typename Lattice>
  void on_step_begin(std::size_t step, const Lattice &frame)
  {
    if (step == 0 and not seeded_)
      {
        counts_.push_back(count_state(frame, tracked_state_));
        totals_.push_back(total_cells(frame));
        seeded_ = true;
      }
  }

  template <typename Lattice>
  void on_step_end(std::size_t /*step*/, const Lattice &frame)
  {
    counts_.push_back(count_state(frame, tracked_state_));
    totals_.push_back(total_cells(frame));
  }
};

// ---------------------------------------------------------------------------
// Activity observer.
// ---------------------------------------------------------------------------

/** @brief Records the number of cells that changed across each step.
 *
 *  Implementation note: the observer maintains a copy of the previous
 *  frame between `on_step_begin` and `on_step_end`. This costs O(N)
 *  memory but avoids depending on the engine to expose the previous
 *  frame, and keeps the observer self-contained.
 *
 *  @tparam Lattice lattice type the observer is wired to. Stored as a
 *                  member so the previous frame can be retained.
 */
template <typename Lattice>
class Activity_Observer
{
  std::optional<Lattice> prev_;
  std::vector<ca_size_t> activity_;

public:
  Activity_Observer() = default;

  /// @return time series of cells changed per step.
  [[nodiscard]] const std::vector<ca_size_t> &activity() const noexcept
  {
    return activity_;
  }

  /// @return number of recorded steps.
  [[nodiscard]] std::size_t size() const noexcept
  {
    return activity_.size();
  }

  void reset() noexcept
  {
    prev_.reset();
    activity_.clear();
  }

  void on_step_begin(std::size_t /*step*/, const Lattice &frame)
  {
    prev_.emplace(frame);
  }

  void on_step_end(std::size_t /*step*/, const Lattice &frame)
  {
    if (prev_.has_value())
      activity_.push_back(cell_diff_count(*prev_, frame));
    else
      activity_.push_back(0);
  }
};

// ---------------------------------------------------------------------------
// Entropy observer.
// ---------------------------------------------------------------------------

/** @brief Records the Shannon entropy of the state distribution.
 *
 *  Computes the entropy in nats over the integer states in
 *  `[0, MaxState)`. Pushes one sample per step (including the
 *  initial frame).
 *
 *  @tparam MaxState exclusive upper bound on the state values.
 */
template <std::size_t MaxState>
class Entropy_Observer
{
  std::vector<double> entropy_;
  bool seeded_ = false;

public:
  Entropy_Observer() = default;

  /// @return time series of entropies in nats.
  [[nodiscard]] const std::vector<double> &entropy() const noexcept
  {
    return entropy_;
  }

  /// @return number of recorded samples.
  [[nodiscard]] std::size_t size() const noexcept
  {
    return entropy_.size();
  }

  void reset() noexcept
  {
    entropy_.clear();
    seeded_ = false;
  }

  template <typename Lattice>
  void on_step_begin(std::size_t step, const Lattice &frame)
  {
    if (step == 0 and not seeded_)
      {
        entropy_.push_back(shannon_entropy(frame, MaxState));
        seeded_ = true;
      }
  }

  template <typename Lattice>
  void on_step_end(std::size_t /*step*/, const Lattice &frame)
  {
    entropy_.push_back(shannon_entropy(frame, MaxState));
  }
};

// ---------------------------------------------------------------------------
// Stationary detector.
// ---------------------------------------------------------------------------

/** @brief Detects fixed points and short cycles via frame hashing.
 *
 *  Keeps a ring buffer of the last `max_cycle_length` frame hashes
 *  paired with the step at which they were seen. After every step
 *  the new hash is compared against the buffer; on a hit the
 *  observer records the cycle length (current step minus the step
 *  at which the matching frame was seen) and exposes it through
 *  `cycle_length()`.
 *
 *  Multiple periodic patterns active at once (e.g. a blinker plus a
 *  glider in different regions) result in the longest reachable
 *  cycle within the buffer being detected.
 */
class Stationary_Detector
{
  std::size_t max_cycle_length_ = 0;
  std::vector<std::uint64_t> hashes_;
  std::vector<std::size_t> steps_;
  std::optional<std::size_t> cycle_length_;
  std::optional<std::size_t> cycle_start_;

  void push(const std::uint64_t h, const std::size_t step) noexcept
  {
    if (hashes_.size() >= max_cycle_length_ + 1)
      {
        // Drop the oldest entry (front of the buffer).
        hashes_.erase(hashes_.begin());
        steps_.erase(steps_.begin());
      }
    hashes_.push_back(h);
    steps_.push_back(step);
  }

public:
  /// Default constructor with a window of 16 frames.
  Stationary_Detector() = default;

  /// Construct with an explicit window size.
  explicit Stationary_Detector(std::size_t max_cycle_length) : max_cycle_length_(max_cycle_length)
  {}

  /// @return true once a cycle has been detected.
  [[nodiscard]] bool cycle_detected() const noexcept
  {
    return cycle_length_.has_value();
  }

  /// @return detected cycle length, or `std::nullopt`.
  [[nodiscard]] std::optional<std::size_t> cycle_length() const noexcept
  {
    return cycle_length_;
  }

  /// @return step at which the cycle started, or `std::nullopt`.
  [[nodiscard]] std::optional<std::size_t> cycle_start() const noexcept
  {
    return cycle_start_;
  }

  /// @return current number of stored hashes.
  [[nodiscard]] std::size_t buffered_hashes() const noexcept
  {
    return hashes_.size();
  }

  void reset() noexcept
  {
    hashes_.clear();
    steps_.clear();
    cycle_length_.reset();
    cycle_start_.reset();
  }

  template <typename Lattice>
  void on_step_begin(std::size_t step, const Lattice &frame)
  {
    if (max_cycle_length_ == 0)
      max_cycle_length_ = 16;
    if (step == 0 and hashes_.empty())
      push(frame_hash(frame), step);
  }

  template <typename Lattice>
  void on_step_end(std::size_t step, const Lattice &frame)
  {
    if (max_cycle_length_ == 0)
      max_cycle_length_ = 16;
    const std::uint64_t h = frame_hash(frame);

    if (not cycle_length_.has_value())
      for (std::size_t i = 0; i < hashes_.size(); ++i)
        if (hashes_[i] == h)
          {
            cycle_start_ = steps_[i];
            cycle_length_ = step - steps_[i];
            break;
          }
    push(h, step);
  }
};

// ---------------------------------------------------------------------------
// Sampling observer.
// ---------------------------------------------------------------------------

/** @brief Snapshots the lattice every `period` steps.
 *
 *  Stores in-memory copies into a `std::vector<Lattice>` accessible
 *  via `snapshots()`. The initial frame is captured (under
 *  `on_step_begin` for step `0`) and every step whose index is a
 *  multiple of `period` thereafter. Pass `period == 0` to disable
 *  sampling entirely.
 *
 *  @tparam Lattice lattice type to copy.
 */
template <typename Lattice>
class Sampling_Observer
{
  std::size_t period_ = 1;
  std::vector<Lattice> snapshots_;
  std::vector<std::size_t> step_indices_;

public:
  /// Snapshot every step (`period == 1`) by default.
  Sampling_Observer() = default;

  /// Snapshot every `period` steps. `0` disables sampling entirely.
  explicit Sampling_Observer(std::size_t period) : period_(period) {}

  /// @return collected snapshots in chronological order.
  [[nodiscard]] const std::vector<Lattice> &snapshots() const noexcept
  {
    return snapshots_;
  }

  /// @return step index recorded for each snapshot.
  [[nodiscard]] const std::vector<std::size_t> &step_indices() const noexcept
  {
    return step_indices_;
  }

  /// @return number of stored snapshots.
  [[nodiscard]] std::size_t size() const noexcept
  {
    return snapshots_.size();
  }

  void reset() noexcept
  {
    snapshots_.clear();
    step_indices_.clear();
  }

  void on_step_begin(std::size_t step, const Lattice &frame)
  {
    if (period_ != 0 and step == 0 and snapshots_.empty())
      {
        snapshots_.push_back(frame);
        step_indices_.push_back(0);
      }
  }

  void on_step_end(std::size_t step, const Lattice &frame)
  {
    if (period_ == 0)
      return;
    if (step % period_ == 0)
      {
        snapshots_.push_back(frame);
        step_indices_.push_back(step);
      }
  }
};

}  // namespace CA
}  // namespace Aleph

#endif  // CA_OBSERVER_H