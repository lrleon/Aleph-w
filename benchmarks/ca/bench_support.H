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

/**
 * @file bench_support.H
 * @brief Shared harness for the cellular-automata performance-gate anchors.
 *
 * This header is local to `benchmarks/`: it is perf-gate tooling, not part of
 * the installed Aleph API. Every anchor benchmark times its kernel with the
 * same warmup-plus-median protocol and emits a single line of structured JSON
 * so the `perf_gate.py` orchestrator can parse it deterministically:
 *
 * @code
 * {"name":"gol_1024","wall_ns":123456789,"cells_per_sec":1.234e8,
 *  "compiler":"gcc 13.2.0","cpu":"Intel(R) Core(TM) ..."}
 * @endcode
 *
 * The median of several timed repetitions (after one untimed warmup) absorbs
 * the run-to-run jitter of shared CI runners, which is why the perf gate can
 * key on a single number per benchmark.
 */

#ifndef CA_BENCH_SUPPORT_H
#define CA_BENCH_SUPPORT_H

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace Aleph {
namespace CA {
namespace Bench {

/** @brief Outcome of one measured anchor benchmark. */
struct Bench_Result
{
  std::string name;            ///< stable benchmark identifier.
  std::uint64_t wall_ns = 0;   ///< median wall-clock time of one run, nanoseconds.
  double cells_per_sec = 0.0;  ///< processed cells divided by the median run time.
};

/** @brief Describe the compiler that produced the running binary.
 * @return a short vendor-and-version string such as `"gcc 13.2.0"`.
 */
inline std::string compiler_string()
{
#if defined(__clang__)
  return std::string("clang ") + __clang_version__;
#elif defined(__GNUC__)
  return std::string("gcc ") + std::to_string(__GNUC__) + '.'
         + std::to_string(__GNUC_MINOR__) + '.' + std::to_string(__GNUC_PATCHLEVEL__);
#else
  return "unknown";
#endif
}

/** @brief Read the CPU model name advertised by the host.
 *
 * On Linux the first `model name` line of `/proc/cpuinfo` is returned; any
 * other platform (or a missing file) yields `"unknown"`.
 *
 * @return the CPU model string, sanitised of embedded commas and quotes so it
 *         is safe to splice into the JSON line.
 */
inline std::string cpu_string()
{
  std::string model = "unknown";
  std::ifstream cpuinfo("/proc/cpuinfo");
  std::string line;
  while (std::getline(cpuinfo, line))
    if (line.rfind("model name", 0) == 0)
      {
        const std::size_t colon = line.find(':');
        if (colon != std::string::npos and colon + 2 <= line.size())
          model = line.substr(colon + 2);
        break;
      }
  for (char &c : model)
    if (c == '"' or c == ',' or c == '\\')
      c = ' ';
  return model;
}

/** @brief Resolve the number of timed repetitions for one anchor.
 *
 * The environment variable `CA_BENCH_REPEATS`, when set to a positive integer,
 * overrides the per-benchmark default. This lets the orchestrator trade
 * stability for wall time on slow runners without recompiling.
 *
 * @param fallback default count used when the variable is unset or invalid.
 * @return the effective repetition count (always at least one).
 */
inline std::size_t resolve_repeats(const std::size_t fallback)
{
  if (const char *env = std::getenv("CA_BENCH_REPEATS"))
    {
      const long parsed = std::strtol(env, nullptr, 10);
      if (parsed > 0)
        return static_cast<std::size_t>(parsed);
    }
  return fallback == 0 ? 1 : fallback;
}

/** @brief Time a kernel with one warmup pass plus a median of timed passes.
 *
 * The callable is invoked `repeats + 1` times. The first invocation is an
 * untimed warmup that pages in memory and primes caches; the remaining
 * invocations are timed with a steady clock and their median is reported. A
 * fresh run on every invocation is the caller's responsibility, so engines
 * that mutate state should be constructed inside @p work.
 *
 * @tparam Work nullary callable performing exactly one run.
 * @param name stable benchmark identifier echoed into the JSON line.
 * @param processed_cells number of cell updates performed by one run; used to
 *        derive the throughput figure.
 * @param repeats number of timed passes (clamped to at least one).
 * @param work the kernel under test.
 * @return the populated benchmark result.
 */
template <typename Work>
[[nodiscard]] Bench_Result measure(std::string name,
                                    const double processed_cells,
                                    std::size_t repeats,
                                    Work &&work)
{
  using clock = std::chrono::steady_clock;
  if (repeats == 0)
    repeats = 1;

  work();  // untimed warmup.

  std::vector<std::uint64_t> samples;
  samples.reserve(repeats);
  for (std::size_t i = 0; i < repeats; ++i)
    {
      const auto start = clock::now();
      work();
      const auto elapsed = clock::now() - start;
      samples.push_back(static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count()));
    }

  std::sort(samples.begin(), samples.end());
  const std::uint64_t median = samples[samples.size() / 2];

  Bench_Result result;
  result.name = std::move(name);
  result.wall_ns = median;
  result.cells_per_sec
    = median == 0 ? 0.0 : processed_cells / (static_cast<double>(median) * 1e-9);
  return result;
}

/** @brief Print one benchmark result as a single structured JSON object.
 *
 * The schema is fixed at `{name, wall_ns, cells_per_sec, compiler, cpu}` so
 * that `perf_gate.py` can parse the line with a plain JSON reader. The number
 * fields use decimal (non-exponential) notation for maximum parser
 * compatibility.
 *
 * @param result the benchmark to serialise.
 * @param out destination stream (defaults to standard output).
 */
inline void emit_json(const Bench_Result &result, std::ostream &out = std::cout)
{
  char throughput[64];
  std::snprintf(throughput, sizeof throughput, "%.3f", result.cells_per_sec);
  out << "{\"name\":\"" << result.name << "\","
      << "\"wall_ns\":" << result.wall_ns << ','
      << "\"cells_per_sec\":" << throughput << ','
      << "\"compiler\":\"" << compiler_string() << "\","
      << "\"cpu\":\"" << cpu_string() << "\"}" << std::endl;
}

}  // namespace Bench
}  // namespace CA
}  // namespace Aleph

#endif  // CA_BENCH_SUPPORT_H