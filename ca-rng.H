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

/** @file ca-rng.H
 *  @brief Reproducible random-number support for stochastic CA rules
 *         (Phase 8).
 *
 *  Provides:
 *
 *    - `splitmix64` / `mix_seed` — stateless 64-bit hashing primitives.
 *    - `cell_key_from_coord<Rank>(coord)` — deterministic 64-bit hash of
 *      a cell coordinate.
 *    - `cell_seed(master, step, coord)` — deterministic per-cell seed
 *      derived from `(master_seed, step, coord)`. Same `(master, step,
 *      coord)` triple → same seed regardless of thread scheduling,
 *      partition layout or visiting order.
 *    - `Per_Thread_RNG<Engine>` — thin wrapper around a master seed
 *      that builds fresh PRNG instances on demand from `(step, coord)`
 *      or `(thread_id, step)` triples.
 *    - `uniform_unit(eng)` / `uniform_int(eng, lo, hi)` — small,
 *      portable distribution helpers that bypass
 *      `std::uniform_real_distribution` (whose bit pattern is
 *      implementation-defined and therefore unsuitable for bit-exact
 *      reproducibility across compilers).
 *
 *  All helpers live in `Aleph::CA` and are header-only.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef CA_RNG_H
#define CA_RNG_H

#include <cstddef>
#include <cstdint>
#include <random>
#include <type_traits>

#include <ca-traits.H>

namespace Aleph {
namespace CA {

/** @brief 64-bit SplitMix hash.
 *
 *  Stateless mixing function used as the building block for every
 *  deterministic seed derivation in the CA module. Identical to the
 *  reference SplitMix64 step of Vigna's `xoroshiro` family.
 *
 *  @param[in] x input value.
 *  @return well-distributed 64-bit hash of `x`.
 *  @throws This function does not throw.
 */
[[nodiscard]] inline constexpr std::uint64_t splitmix64(std::uint64_t x) noexcept
{
  x += 0x9e3779b97f4a7c15ull;
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
  return x ^ (x >> 31);
}

/** @brief Combine two 64-bit values into a single deterministic hash.
 *
 *  @param[in] a first input.
 *  @param[in] b second input.
 *  @return a value that is sensitive to both inputs.
 *  @throws This function does not throw.
 */
[[nodiscard]] inline constexpr std::uint64_t mix_seed(const std::uint64_t a,
                                                      const std::uint64_t b) noexcept
{
  return splitmix64(splitmix64(a + 0x9e3779b97f4a7c15ull) ^ (b + 0xbf58476d1ce4e5b9ull));
}

/** @brief Hash a cell coordinate into a stable 64-bit key.
 *
 *  The hash depends only on the coordinate components; rank is folded
 *  into the avalanche through `splitmix64`. Two coordinates produce
 *  the same key iff every component matches.
 *
 *  @tparam Rank coordinate rank.
 *  @param[in] c cell coordinate.
 *  @return deterministic 64-bit key.
 *  @throws This function does not throw.
 */
template <std::size_t Rank>
[[nodiscard]] inline constexpr std::uint64_t cell_key_from_coord(const Coord_Vec<Rank> &c) noexcept
{
  std::uint64_t h = 0xcbf29ce484222325ull;  // FNV offset basis.
  for (std::size_t d = 0; d < Rank; ++d)
    {
      const std::uint64_t component = static_cast<std::uint64_t>(static_cast<std::int64_t>(c[d]));
      h = splitmix64(h ^ component);
    }
  return h;
}

/** @brief Build a deterministic per-cell seed.
 *
 *  Given a master seed, a step index and a cell coordinate, returns a
 *  64-bit value that uniquely identifies this `(master, step, coord)`
 *  triple. The mixing avalanche makes the output statistically
 *  independent across consecutive `(step, coord)` triples.
 *
 *  @tparam Rank coordinate rank.
 *  @param[in] master_seed user-controlled root seed.
 *  @param[in] step step index that is being evaluated.
 *  @param[in] coord cell coordinate.
 *  @return per-cell seed.
 *  @throws This function does not throw.
 */
template <std::size_t Rank>
[[nodiscard]] inline constexpr std::uint64_t cell_seed(const std::uint64_t master_seed,
                                                       const std::size_t step,
                                                       const Coord_Vec<Rank> &coord) noexcept
{
  const std::uint64_t step_hash
    = splitmix64(static_cast<std::uint64_t>(step) + 0xa5a5a5a5a5a5a5a5ull);
  const std::uint64_t coord_hash = cell_key_from_coord<Rank>(coord);
  return splitmix64(master_seed ^ step_hash ^ coord_hash);
}

/** @brief Build a deterministic per-cell seed from a `Cell_Context`.
 *
 *  @tparam Rank coordinate rank (deduced from `ctx`).
 *  @param[in] master_seed user-controlled root seed.
 *  @param[in] ctx per-cell context produced by the engine.
 *  @return per-cell seed.
 *  @throws This function does not throw.
 */
template <std::size_t Rank>
[[nodiscard]] inline constexpr std::uint64_t cell_seed(const std::uint64_t master_seed,
                                                       const Cell_Context<Rank> &ctx) noexcept
{
  return cell_seed<Rank>(master_seed, ctx.step, ctx.coord);
}

/** @brief Build a deterministic seed from `(master, thread_id, step)`.
 *
 *  Useful for the alternative "per-thread substream" strategy in which
 *  the rule reuses the same engine for every cell visited by a single
 *  worker. This trades determinism across thread counts for raw
 *  speed; the per-cell strategy provided by `cell_seed()` should be
 *  preferred whenever cross-scheduling reproducibility is required.
 *
 *  @param[in] master_seed user-controlled root seed.
 *  @param[in] thread_id stable identifier for the worker.
 *  @param[in] step step index being evaluated.
 *  @return per-thread sub-stream seed.
 *  @throws This function does not throw.
 */
[[nodiscard]] inline constexpr std::uint64_t thread_step_seed(const std::uint64_t master_seed,
                                                              const std::size_t thread_id,
                                                              const std::size_t step) noexcept
{
  const std::uint64_t tid_hash
    = splitmix64(static_cast<std::uint64_t>(thread_id) + 0xfeedfacecafebee0ull);
  const std::uint64_t step_hash
    = splitmix64((static_cast<std::uint64_t>(step) << 32) ^ 0x0123456789abcdefull);
  return splitmix64(master_seed ^ tid_hash ^ step_hash);
}

/** @brief Map a 64-bit RNG output to a uniform value in `[0, 1)`.
 *
 *  Implementation-defined components of `std::uniform_real_distribution`
 *  vary across standard library vendors, so we provide a small
 *  portable converter that uses the top 53 bits of `eng()`. The
 *  output is bit-exact across platforms when fed the same engine
 *  state.
 *
 *  @tparam Engine RNG engine type whose `result_type` is at least
 *          53 bits wide.
 *  @param[in,out] eng RNG engine; advanced by one draw.
 *  @return uniform value in `[0, 1)`.
 *  @throws Any exception thrown by `eng()`.
 */
template <typename Engine>
[[nodiscard]] inline double uniform_unit(Engine &eng)
{
  static_assert(std::is_unsigned_v<typename Engine::result_type>,
                "uniform_unit requires an unsigned result_type");
  const std::uint64_t v = static_cast<std::uint64_t>(eng());
  // 2^53 = 9007199254740992. The shift drops the noisy low 11 bits
  // and leaves a value in [0, 2^53) that maps exactly to [0, 1).
  return static_cast<double>(v >> 11) * (1.0 / 9007199254740992.0);
}

/** @brief Sample a uniform integer in `[lo, hi]` (inclusive).
 *
 *  Provided for portability of the stochastic rules; mirrors the
 *  semantics of `std::uniform_int_distribution<std::size_t>(lo, hi)`
 *  but uses our `uniform_unit` so the bit pattern is reproducible
 *  across vendors.
 *
 *  @tparam Engine RNG engine type.
 *  @param[in,out] eng RNG engine; advanced by one draw.
 *  @param[in] lo lower bound (inclusive).
 *  @param[in] hi upper bound (inclusive); must satisfy `lo <= hi`.
 *  @return uniform sample in `[lo, hi]`.
 *  @throws Any exception thrown by `eng()`.
 */
template <typename Engine>
[[nodiscard]] inline std::size_t uniform_int(Engine &eng, const std::size_t lo, const std::size_t hi)
{
  if (hi <= lo)
    return lo;
  const double u = uniform_unit(eng);
  const std::size_t span = hi - lo + 1;
  std::size_t pick = lo + static_cast<std::size_t>(u * static_cast<double>(span));
  if (pick > hi)
    pick = hi;
  return pick;
}

/** @brief Master seed dispenser for stochastic CA rules.
 *
 *  `Per_Thread_RNG` does *not* own any RNG state: it is a pure factory
 *  that builds a fresh deterministic engine on demand from a master
 *  seed and either a per-cell context or a per-thread descriptor.
 *  This makes it cheap to copy, share between threads (no
 *  synchronisation required) and to compose with the Phase 5 parallel
 *  engine without serialisation hazards.
 *
 *  Typical usage inside a contextual rule:
 *
 *  ```cpp
 *  struct My_Stochastic_Rule {
 *    double p; std::uint64_t master;
 *    template <typename State, std::size_t Rank>
 *    State operator()(const State& s, Neighbor_View<State> v,
 *                     const Cell_Context<Rank>& ctx) const {
 *      Per_Thread_RNG<std::mt19937_64> rng(master);
 *      auto eng = rng.for_cell(ctx);
 *      return uniform_unit(eng) < p ? next(s, v) : s;
 *    }
 *  };
 *  ```
 *
 *  @tparam Engine PRNG engine type. Defaults to `std::mt19937_64` for
 *          its 64-bit output and good statistical quality. Any engine
 *          whose `result_type` is unsigned and at least 53 bits wide
 *          works.
 */
template <typename Engine = std::mt19937_64>
class Per_Thread_RNG
{
  std::uint64_t master_;

public:
  /// Underlying engine type.
  using engine_type = Engine;
  /// Result type of `engine_type`.
  using result_type = typename Engine::result_type;

  /** @brief Build a dispenser around `seed`.
   *
   *  @param[in] seed master seed reused across every sub-stream.
   *  @throws This function does not throw.
   */
  explicit constexpr Per_Thread_RNG(const std::uint64_t seed = 0) noexcept : master_(seed) {}

  /** @brief Return the master seed.
   *  @return user-supplied master seed.
   *  @throws This function does not throw.
   */
  [[nodiscard]] constexpr std::uint64_t master_seed() const noexcept
  {
    return master_;
  }

  /** @brief Build a fresh deterministic engine for cell `(step, coord)`.
   *
   *  The returned engine state depends only on `(master_, step, coord)`,
   *  so two calls with the same arguments — possibly from different
   *  threads — produce identical streams.
   *
   *  @tparam Rank coordinate rank (deduced).
   *  @param[in] step step index being evaluated.
   *  @param[in] coord cell coordinate.
   *  @return engine seeded with `cell_seed(master_, step, coord)`.
   *  @throws Any exception thrown by `Engine`'s constructor.
   */
  template <std::size_t Rank>
  [[nodiscard]] Engine for_cell(const std::size_t step, const Coord_Vec<Rank> &coord) const
  {
    return Engine{static_cast<result_type>(cell_seed<Rank>(master_, step, coord))};
  }

  /** @brief Build a fresh deterministic engine from a `Cell_Context`.
   *
   *  @tparam Rank coordinate rank (deduced).
   *  @param[in] ctx per-cell context produced by the engine.
   *  @return engine seeded with `cell_seed(master_, ctx)`.
   *  @throws Any exception thrown by `Engine`'s constructor.
   */
  template <std::size_t Rank>
  [[nodiscard]] Engine for_cell(const Cell_Context<Rank> &ctx) const
  {
    return for_cell<Rank>(ctx.step, ctx.coord);
  }

  /** @brief Build an engine for a `(thread, step)` sub-stream.
   *
   *  The result depends on `(master_, thread_id, step)` and is
   *  therefore *not* invariant under different thread counts. Use
   *  this overload only when you explicitly want a per-thread
   *  sub-stream rather than per-cell determinism.
   *
   *  @param[in] thread_id stable identifier for the worker.
   *  @param[in] step step index being evaluated.
   *  @return engine seeded with `thread_step_seed`.
   *  @throws Any exception thrown by `Engine`'s constructor.
   */
  [[nodiscard]] Engine for_thread_step(const std::size_t thread_id, const std::size_t step) const
  {
    return Engine{static_cast<result_type>(thread_step_seed(master_, thread_id, step))};
  }
};

}  // namespace CA
}  // namespace Aleph

#endif  // CA_RNG_H
