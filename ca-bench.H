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

/** @file ca-bench.H
 *  @brief Tiny chrono-based timer used by the CA module Examples.
 *
 *  This header is intentionally minimal: it lives in the public headers
 *  layout so the Examples can include it the same way as the rest of
 *  the library, but it is not part of the engine API. It is only used
 *  by the Phase-5 microbenchmarks that need a deterministic wall-clock
 *  reading (e.g. `Examples/ca_bench_example.cc`).
 *
 *  Public surface:
 *
 *    - `Bench_Timer`         — start/stop wall-clock timer.
 *    - `bench_seconds(F)`    — measure the wall time of one invocation.
 *    - `format_throughput`   — pretty-print "M cells / s" numbers.
 *
 *  Everything is `inline` and depends only on `<chrono>` and
 *  `<cstdio>`, so it does not pull anything else from Aleph-w into the
 *  Examples build graph.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef CA_BENCH_H
#define CA_BENCH_H

#include <chrono>
#include <cstddef>
#include <cstdio>
#include <utility>

namespace Aleph {
namespace CA {

/** @brief Tiny start/stop wall-clock timer.
 *
 *  Wraps `std::chrono::steady_clock` so the timer cannot be tricked by
 *  wall-clock adjustments. Construction starts the timer; `restart()`
 *  resets it to `now()`; `elapsed_seconds()` and `elapsed_milliseconds()`
 *  read the duration without stopping the clock.
 */
class Bench_Timer
{
  using clock_type = std::chrono::steady_clock;
  using time_point = clock_type::time_point;
  time_point start_ = clock_type::now();

public:
  /// Restart the timer to `now()`.
  void restart() noexcept
  {
    start_ = clock_type::now();
  }

  /// @return seconds elapsed since the last `restart()` (or construction).
  [[nodiscard]] double elapsed_seconds() const noexcept
  {
    const auto now = clock_type::now();
    const std::chrono::duration<double> d = now - start_;
    return d.count();
  }

  /// @return milliseconds elapsed since the last `restart()`.
  [[nodiscard]] double elapsed_milliseconds() const noexcept
  {
    return elapsed_seconds() * 1e3;
  }
};

/** @brief Run `f()` once and return the wall-clock time it took, in seconds.
 *
 *  The functor's return value is discarded. Use this when the work is
 *  large enough that a single sample is meaningful. For repeatable
 *  microbenches at the µs scale, average several invocations manually.
 *
 *  @tparam F invocable type, callable as `f()`.
 *  @param[in] f functor to time.
 *  @return seconds spent inside `f()`.
 */
template <typename F>
[[nodiscard]] inline double bench_seconds(F &&f)
{
  Bench_Timer t;
  std::forward<F>(f)();
  return t.elapsed_seconds();
}

/** @brief Format a "cells per second" rate as "X.XX M cells/s".
 *
 *  Helper used by the bench example to pretty-print throughput numbers
 *  in a small table. Rates above one giga-cell are reported as `G`.
 *
 *  @param[in] cells_per_second raw rate in cells / s.
 *  @return a static-storage C string with the formatted value.
 */
[[nodiscard]] inline const char *format_throughput(double cells_per_second)
{
  thread_local char buf[64];
  if (const double abs_rate = cells_per_second < 0 ? -cells_per_second : cells_per_second;
      abs_rate >= 1e9)
    std::snprintf(buf, sizeof(buf), "%6.2f G cells/s", cells_per_second / 1e9);
  else if (abs_rate >= 1e6)
    std::snprintf(buf, sizeof(buf), "%6.2f M cells/s", cells_per_second / 1e6);
  else if (abs_rate >= 1e3)
    std::snprintf(buf, sizeof(buf), "%6.2f K cells/s", cells_per_second / 1e3);
  else
    std::snprintf(buf, sizeof(buf), "%6.2f   cells/s", cells_per_second);
  return buf;
}

}  // namespace CA
}  // namespace Aleph

#endif  // CA_BENCH_H