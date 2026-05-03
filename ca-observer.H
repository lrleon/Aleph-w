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
  /** @brief Build the composite from child observers.
   *
   *  Reference types in `Observers...` are preserved and borrowed;
   *  value types are moved into the tuple and owned by the composite.
   *
   *  @param[in] observers child observers to store or reference.
   *  @throws Any exception thrown while constructing the internal tuple.
   *
   *  @par Complexity
   *  O(number of observers) time and space.
   *
   *  @par Thread-safety
   *  Not thread-safe for concurrent callback writes. Concurrent readers
   *  are safe only if the referenced/owned observers are safe.
   */
  explicit Composite_Observer(Observers... observers) : obs_(std::forward<Observers>(observers)...)
  {}

  /** @brief Return read-only access to a child observer.
   *  @tparam I child index.
   *  @return const reference to the selected observer.
   *  @throws This function does not throw.
   */
  template <std::size_t I>
  [[nodiscard]] const auto &get() const noexcept
  {
    return std::get<I>(obs_);
  }

  /** @brief Return mutable access to a child observer.
   *  @tparam I child index.
   *  @return mutable reference to the selected observer.
   *  @throws This function does not throw.
   */
  template <std::size_t I>
  [[nodiscard]] auto &get() noexcept
  {
    return std::get<I>(obs_);
  }

  /** @brief Return the number of child observers.
   *  @return `sizeof...(Observers)`.
   *  @throws This function does not throw.
   */
  [[nodiscard]] static constexpr std::size_t size() noexcept
  {
    return sizeof...(Observers);
  }

  /** @brief Forward a step-begin event to every child observer.
   *  @tparam Lattice lattice type.
   *  @param[in] step step index about to run.
   *  @param[in] frame current frame before the step.
   *  @throws Any exception thrown by a child observer.
   *
   *  @par Complexity
   *  O(number of observers).
   */
  template <typename Lattice>
  void on_step_begin(std::size_t step, const Lattice &frame)
  {
    std::apply([&](auto &...o)
    {
      (o.on_step_begin(step, frame), ...);
    }, obs_);
  }

  /** @brief Forward a step-end event to every child observer.
   *  @tparam Lattice lattice type.
   *  @param[in] step step index just completed.
   *  @param[in] frame current frame after the step.
   *  @throws Any exception thrown by a child observer.
   *
   *  @par Complexity
   *  O(number of observers).
   */
  template <typename Lattice>
  void on_step_end(std::size_t step, const Lattice &frame)
  {
    std::apply([&](auto &...o)
    {
      (o.on_step_end(step, frame), ...);
    }, obs_);
  }
};

/** @brief Build a `Composite_Observer` with deduced ownership semantics.
 *
 *  Lvalue arguments are stored as references, so later state changes
 *  remain visible through the original observer objects. Rvalue
 *  arguments are moved into and owned by the composite.
 *
 *  @tparam Observers deduced observer argument types.
 *  @param[in] observers observer objects or references.
 *  @return a composite observer preserving lvalue references.
 *  @throws Any exception thrown while constructing the composite.
 *
 *  @par Complexity
 *  O(number of observers) time and space.
 */
template <typename... Observers>
[[nodiscard]] inline auto make_composite_observer(Observers &&...observers)
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
 *  @throws Any exception thrown by engine hook registration.
 *
 *  @par Complexity
 *  O(1) time and space.
 *
 *  @par Thread-safety
 *  Not thread-safe if the engine is concurrently mutated. The observer
 *  is captured by reference and must outlive the engine callbacks.
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

/** @brief Attach an observer to engines without a `lattice_type` alias.
 *
 *  This overload is intended for `Graph_Synchronous_Engine`; it captures
 *  the observer by reference and deduces the frame type in the hook
 *  lambdas.
 *
 *  @param[in,out] engine engine instance.
 *  @param[in,out] observer observer instance; must outlive callbacks.
 *  @throws Any exception thrown by engine hook registration.
 *
 *  @par Complexity
 *  O(1) time and space.
 *
 *  @par Thread-safety
 *  Not thread-safe if the engine is concurrently mutated.
 */
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
 *
 *  @par Complexity
 *  Callbacks are O(N) per sampled frame and O(1) extra space per
 *  sample, where N is the number of cells.
 *
 *  @par Thread-safety
 *  Thread-safe for concurrent readers after simulation stops; not
 *  thread-safe for concurrent writers/callbacks.
 *
 *  @par Exception-safety
 *  Strong guarantee unless vector allocation throws during sampling.
 */
template <typename State>
class Density_Observer
{
  State tracked_state_{};
  std::vector<ca_size_t> counts_;
  std::vector<ca_size_t> totals_;
  bool seeded_ = false;

public:
  /** @brief Construct an observer that tracks `State{}`.
   *  @throws Any exception thrown by `State` default construction.
   */
  Density_Observer() = default;

  /** @brief Construct an observer tracking a specific state.
   *  @param[in] s state value counted at every sample.
   *  @throws Any exception thrown while moving/copying `s`.
   */
  explicit Density_Observer(State s) : tracked_state_(std::move(s)) {}

  /** @brief Return the count time series.
   *  @return counts, one per recorded step including the initial frame.
   *  @throws This function does not throw.
   */
  [[nodiscard]] const std::vector<ca_size_t> &counts() const noexcept
  {
    return counts_;
  }

  /** @brief Return the total-cell time series.
   *  @return total cell counts, aligned with `counts()`.
   *  @throws This function does not throw.
   */
  [[nodiscard]] const std::vector<ca_size_t> &totals() const noexcept
  {
    return totals_;
  }

  /** @brief Return density at a recorded sample.
   *  @param[in] i sample index.
   *  @return `counts()[i] / totals()[i]`, or 0 for empty lattices.
   *  @throws std::out_of_range if `i >= size()`.
   */
  [[nodiscard]] double density_at(std::size_t i) const
  {
    ah_out_of_range_error_if(i >= counts_.size())
      << "Density_Observer: index " << i << " out of range";
    return totals_[i] == 0 ? 0.0 : static_cast<double>(counts_[i]) / static_cast<double>(totals_[i]);
  }

  /** @brief Return the number of recorded samples.
   *  @return sample count.
   *  @throws This function does not throw.
   */
  [[nodiscard]] std::size_t size() const noexcept
  {
    return counts_.size();
  }

  /** @brief Clear all recorded samples.
   *  @throws This function does not throw.
   *
   *  @par Complexity
   *  O(size()).
   */
  void reset() noexcept
  {
    counts_.clear();
    totals_.clear();
    seeded_ = false;
  }

  /** @brief Record the initial frame before step zero.
   *  @tparam Lattice lattice type.
   *  @param[in] step step index about to run.
   *  @param[in] frame frame before the step.
   *  @throws std::bad_alloc if appending the sample allocates.
   */
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

  /** @brief Record the frame after a completed step.
   *  @tparam Lattice lattice type.
   *  @param[in] step completed step index, ignored.
   *  @param[in] frame frame after the step.
   *  @throws std::bad_alloc if appending the sample allocates.
   */
  template <typename Lattice>
  void on_step_end(std::size_t step, const Lattice &frame)
  {
    (void) step;
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
 *
 *  @par Complexity
 *  `on_step_begin` copies one frame in O(N); `on_step_end` compares
 *  frames in O(N).
 *
 *  @par Thread-safety
 *  Thread-safe for concurrent readers after simulation stops; not
 *  thread-safe for concurrent callbacks.
 *
 *  @par Exception-safety
 *  Strong guarantee unless copying the frame or appending the sample
 *  throws.
 */
template <typename Lattice>
class Activity_Observer
{
  std::optional<Lattice> prev_;
  std::vector<ca_size_t> activity_;

public:
  /** @brief Construct an empty activity observer.
   *  @throws This function does not throw.
   */
  Activity_Observer() = default;

  /** @brief Return the activity time series.
   *  @return changed-cell counts, one per completed step.
   *  @throws This function does not throw.
   */
  [[nodiscard]] const std::vector<ca_size_t> &activity() const noexcept
  {
    return activity_;
  }

  /** @brief Return the number of recorded steps.
   *  @return sample count.
   *  @throws This function does not throw.
   */
  [[nodiscard]] std::size_t size() const noexcept
  {
    return activity_.size();
  }

  /** @brief Clear the previous frame and activity samples.
   *  @throws This function does not throw.
   *
   *  @par Complexity
   *  O(size()) plus destruction cost of the stored frame.
   */
  void reset() noexcept
  {
    prev_.reset();
    activity_.clear();
  }

  /** @brief Capture the frame before a step.
   *  @param[in] step step index about to run, ignored.
   *  @param[in] frame frame before the step.
   *  @throws Any exception thrown while copying `frame`.
   */
  void on_step_begin(std::size_t step, const Lattice &frame)
  {
    (void) step;
    prev_.emplace(frame);
  }

  /** @brief Record changed cells after a step.
   *  @param[in] step completed step index, ignored.
   *  @param[in] frame frame after the step.
   *  @throws std::bad_alloc if appending the sample allocates.
   */
  void on_step_end(std::size_t step, const Lattice &frame)
  {
    (void) step;
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
 *
 *  @par Complexity
 *  Callbacks are O(N + MaxState) per sampled frame.
 *
 *  @par Thread-safety
 *  Thread-safe for concurrent readers after simulation stops; not
 *  thread-safe for concurrent callbacks.
 *
 *  @par Exception-safety
 *  Strong guarantee unless vector allocation throws.
 */
template <std::size_t MaxState>
class Entropy_Observer
{
  std::vector<double> entropy_;
  bool seeded_ = false;

public:
  /** @brief Construct an empty entropy observer.
   *  @throws This function does not throw.
   */
  Entropy_Observer() = default;

  /** @brief Return the entropy time series.
   *  @return entropies in nats, including the initial frame.
   *  @throws This function does not throw.
   */
  [[nodiscard]] const std::vector<double> &entropy() const noexcept
  {
    return entropy_;
  }

  /** @brief Return the number of entropy samples.
   *  @return sample count.
   *  @throws This function does not throw.
   */
  [[nodiscard]] std::size_t size() const noexcept
  {
    return entropy_.size();
  }

  /** @brief Clear all entropy samples.
   *  @throws This function does not throw.
   *
   *  @par Complexity
   *  O(size()).
   */
  void reset() noexcept
  {
    entropy_.clear();
    seeded_ = false;
  }

  /** @brief Record entropy of the initial frame.
   *  @tparam Lattice lattice type.
   *  @param[in] step step index about to run.
   *  @param[in] frame frame before the step.
   *  @throws std::bad_alloc if appending the sample allocates.
   */
  template <typename Lattice>
  void on_step_begin(std::size_t step, const Lattice &frame)
  {
    if (step == 0 and not seeded_)
      {
        entropy_.push_back(shannon_entropy(frame, MaxState));
        seeded_ = true;
      }
  }

  /** @brief Record entropy after a completed step.
   *  @tparam Lattice lattice type.
   *  @param[in] step completed step index, ignored.
   *  @param[in] frame frame after the step.
   *  @throws std::bad_alloc if appending the sample allocates.
   */
  template <typename Lattice>
  void on_step_end(std::size_t step, const Lattice &frame)
  {
    (void) step;
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
 *
 *  @par Complexity
 *  Each callback computes one frame hash in O(N) and scans at most
 *  `max_cycle_length + 1` hashes.
 *
 *  @par Thread-safety
 *  Thread-safe for concurrent readers after simulation stops; not
 *  thread-safe for concurrent callbacks.
 *
 *  @par Exception-safety
 *  Strong guarantee unless vector allocation throws.
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
  /** @brief Construct a detector with the default 16-frame window.
   *  @throws This function does not throw.
   */
  Stationary_Detector() = default;

  /** @brief Construct a detector with an explicit window size.
   *  @param[in] max_cycle_length maximum cycle length to retain.
   *  @throws This function does not throw.
   */
  explicit Stationary_Detector(std::size_t max_cycle_length) : max_cycle_length_(max_cycle_length)
  {}

  /** @brief Return whether a cycle has been detected.
   *  @return true after the first matching frame hash.
   *  @throws This function does not throw.
   */
  [[nodiscard]] bool cycle_detected() const noexcept
  {
    return cycle_length_.has_value();
  }

  /** @brief Return the detected cycle length.
   *  @return cycle length, or `std::nullopt` if none has been found.
   *  @throws This function does not throw.
   */
  [[nodiscard]] std::optional<std::size_t> cycle_length() const noexcept
  {
    return cycle_length_;
  }

  /** @brief Return the cycle start step.
   *  @return start step, or `std::nullopt` if no cycle has been found.
   *  @throws This function does not throw.
   */
  [[nodiscard]] std::optional<std::size_t> cycle_start() const noexcept
  {
    return cycle_start_;
  }

  /** @brief Return the number of buffered hashes.
   *  @return current ring-buffer occupancy.
   *  @throws This function does not throw.
   */
  [[nodiscard]] std::size_t buffered_hashes() const noexcept
  {
    return hashes_.size();
  }

  /** @brief Clear buffered hashes and detected-cycle state.
   *  @throws This function does not throw.
   *
   *  @par Complexity
   *  O(buffered_hashes()).
   */
  void reset() noexcept
  {
    hashes_.clear();
    steps_.clear();
    cycle_length_.reset();
    cycle_start_.reset();
  }

  /** @brief Seed the detector with the initial frame hash.
   *  @tparam Lattice lattice type.
   *  @param[in] step step index about to run.
   *  @param[in] frame frame before the step.
   *  @throws std::bad_alloc if buffering the hash allocates.
   */
  template <typename Lattice>
  void on_step_begin(std::size_t step, const Lattice &frame)
  {
    if (max_cycle_length_ == 0)
      max_cycle_length_ = 16;
    if (step == 0 and hashes_.empty())
      push(frame_hash(frame), step);
  }

  /** @brief Check the completed frame against the recent hash window.
   *  @tparam Lattice lattice type.
   *  @param[in] step completed step index.
   *  @param[in] frame frame after the step.
   *  @throws std::bad_alloc if buffering the hash allocates.
   */
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
 *
 *  @par Complexity
 *  Sampling a frame costs O(N) time and space. Non-sampling callbacks
 *  are O(1).
 *
 *  @par Thread-safety
 *  Thread-safe for concurrent readers after simulation stops; not
 *  thread-safe for concurrent callbacks.
 *
 *  @par Exception-safety
 *  Strong guarantee unless copying the lattice or appending to vectors
 *  throws.
 */
template <typename Lattice>
class Sampling_Observer
{
  std::size_t period_ = 1;
  std::vector<Lattice> snapshots_;
  std::vector<std::size_t> step_indices_;

public:
  /** @brief Construct an observer that snapshots every step.
   *  @throws This function does not throw.
   */
  Sampling_Observer() = default;

  /** @brief Construct an observer with an explicit sampling period.
   *  @param[in] period snapshot period; zero disables sampling.
   *  @throws This function does not throw.
   */
  explicit Sampling_Observer(std::size_t period) : period_(period) {}

  /** @brief Return collected snapshots.
   *  @return snapshots in chronological order.
   *  @throws This function does not throw.
   */
  [[nodiscard]] const std::vector<Lattice> &snapshots() const noexcept
  {
    return snapshots_;
  }

  /** @brief Return step indices for collected snapshots.
   *  @return step indices aligned with `snapshots()`.
   *  @throws This function does not throw.
   */
  [[nodiscard]] const std::vector<std::size_t> &step_indices() const noexcept
  {
    return step_indices_;
  }

  /** @brief Return the number of stored snapshots.
   *  @return snapshot count.
   *  @throws This function does not throw.
   */
  [[nodiscard]] std::size_t size() const noexcept
  {
    return snapshots_.size();
  }

  /** @brief Clear all snapshots and step indices.
   *  @throws This function does not throw.
   *
   *  @par Complexity
   *  O(size()) plus snapshot destruction cost.
   */
  void reset() noexcept
  {
    snapshots_.clear();
    step_indices_.clear();
  }

  /** @brief Capture the initial frame when sampling is enabled.
   *  @param[in] step step index about to run.
   *  @param[in] frame frame before the step.
   *  @throws Any exception thrown while copying `frame` or appending.
   */
  void on_step_begin(std::size_t step, const Lattice &frame)
  {
    if (period_ != 0 and step == 0 and snapshots_.empty())
      {
        snapshots_.push_back(frame);
        step_indices_.push_back(0);
      }
  }

  /** @brief Capture the completed frame at period boundaries.
   *  @param[in] step completed step index.
   *  @param[in] frame frame after the step.
   *  @throws Any exception thrown while copying `frame` or appending.
   */
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
