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

/** @file ca-checkpoint.H
 *  @brief Phase 15 binary checkpoint format for `Aleph::CA` engines.
 *
 *  A *checkpoint* is a self-describing binary snapshot of an engine's
 *  state on disk. It captures everything required to resume a long-
 *  running simulation bit-by-bit:
 *
 *    - A magic + version + engine type-hash header that lets `load`
 *      reject incompatible files early.
 *    - The lattice extents and the current step counter.
 *    - The full frame data, written cell-by-cell in the host's
 *      little-endian representation (the wire format is fixed and
 *      independent of compiler/architecture).
 *    - The optional master RNG seed, when the rule exposes one
 *      through the `Has_Master_Seed` concept (the `Per_Thread_RNG`
 *      pattern from `ca-rng.H`).
 *
 *  Because every contextual stochastic rule in the module derives its
 *  per-cell engine deterministically from `(master_seed, step, coord)`,
 *  restoring `(master_seed, step, frame)` is sufficient to reproduce
 *  the future trajectory bit-by-bit, regardless of thread scheduling
 *  or build mode.
 *
 *  Two top-level entry points:
 *
 *    - `save_checkpoint(engine, path)` writes a complete snapshot.
 *    - `load_checkpoint_into(engine, path)` rebuilds the engine state
 *      in-place; the caller controls the engine type. A
 *      `Resume_Token` records the path and the validated header for
 *      later auditing.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef CA_CHECKPOINT_H
#define CA_CHECKPOINT_H

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iosfwd>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>

#include <ah-errors.H>

#include <ca-rng.H>
#include <ca-traits.H>
#include <tpl_ca_concepts.H>

namespace Aleph {
namespace CA {

namespace ca_checkpoint_detail {

/// Magic prefix identifying an Aleph CA checkpoint stream.
inline constexpr std::array<char, 8> magic_bytes = {{'A', 'L', 'E', 'P', 'H', 'C', 'A', '1'}};

/// Format version. Bumped whenever the on-disk layout changes in a
/// non-backward-compatible way.
inline constexpr std::uint32_t format_version = 1;

/// FNV-1a 64-bit hash. Used to derive a stable hash from
/// `typeid(Engine).name()` so that `load_checkpoint_into` can reject
/// snapshots produced by a different engine type within the same
/// build.
[[nodiscard]] inline constexpr std::uint64_t fnv1a_64(std::string_view s) noexcept
{
  std::uint64_t h = 0xcbf29ce484222325ull;
  for (const char c : s)
    {
      h ^= static_cast<std::uint64_t>(static_cast<unsigned char>(c));
      h *= 0x100000001b3ull;
    }
  return h;
}

/// Stable type hash of `T`. Mixes the `typeid` name and `sizeof` so
/// pointer-equal `typeid` strings still differ across template
/// instantiations of the same name (rare but possible in mixed
/// dynamic-library setups).
template <typename T>
[[nodiscard]] inline std::uint64_t type_hash() noexcept
{
  const std::uint64_t name_hash = fnv1a_64(typeid(T).name());
  return splitmix64(name_hash ^ static_cast<std::uint64_t>(sizeof(T)));
}

// --- Little-endian byte serialisation --------------------------------

template <typename T>
inline void write_le(std::ostream &out, const T &value)
{
  static_assert(std::is_trivially_copyable_v<T>,
                "write_le requires a trivially copyable type");
  // We always emit the host's byte representation; for the integral
  // sizes we use it is identical to little-endian on every platform
  // Aleph-w supports.
  std::array<char, sizeof(T)> bytes{};
  std::memcpy(bytes.data(), &value, sizeof(T));
  out.write(bytes.data(), sizeof(T));
}

template <typename T>
[[nodiscard]] inline T read_le(std::istream &in)
{
  static_assert(std::is_trivially_copyable_v<T>,
                "read_le requires a trivially copyable type");
  std::array<char, sizeof(T)> bytes{};
  in.read(bytes.data(), sizeof(T));
  T value{};
  std::memcpy(&value, bytes.data(), sizeof(T));
  return value;
}

}  // namespace ca_checkpoint_detail

/** @brief Public on-disk header surfaced by `inspect_checkpoint`.
 *
 *  Mirrors the layout serialised at the start of every checkpoint
 *  file. Useful for tooling: `inspect_checkpoint(path)` returns this
 *  struct without touching the frame payload.
 */
struct Checkpoint_Header
{
  /// Stream magic ("ALEPHCA1").
  std::array<char, 8> magic{};
  /// Format version.
  std::uint32_t format_version = 0;
  /// Stable type hash of the engine type that produced this snapshot.
  std::uint64_t engine_type_hash = 0;
  /// `sizeof(state_type)` recorded at save time, for sanity checks.
  std::uint32_t state_type_size = 0;
  /// Lattice rank (1, 2 or 3 in the bundled lattices).
  std::uint32_t rank = 0;
  /// Per-axis extents (only the first `rank` entries are meaningful).
  std::array<std::uint64_t, 4> extents{};
  /// Number of completed steps at save time.
  std::uint64_t step_count = 0;
  /// 1 when the snapshot carries a master RNG seed.
  std::uint8_t has_rng = 0;
  /// Recorded master RNG seed (only valid when `has_rng == 1`).
  std::uint64_t master_seed = 0;
  /// Number of cells stored in the frame payload.
  std::uint64_t cell_count = 0;
};

/** @brief Concept: rule type that exposes a master RNG seed.
 *
 *  Stochastic rules built on `Per_Thread_RNG` follow this contract:
 *  the rule stores a master seed and offers `master_seed()` /
 *  `set_master_seed(s)` accessors. Checkpointing uses the concept
 *  to detect-and-restore the seed automatically.
 *
 *  @tparam R rule type.
 */
template <typename R>
concept Has_Master_Seed = requires(R &r, std::uint64_t s) {
  { r.master_seed() } -> std::convertible_to<std::uint64_t>;
  r.set_master_seed(s);
};

/** @brief Concept: lattice exposing the standard CA layout interface.
 *
 *  Mirrors the requirements of `LatticeLike` plus an `extents()`
 *  accessor so we can reshape the frame on load without external
 *  metadata.
 *
 *  @tparam L lattice type.
 */
template <typename L>
concept Checkpointable_Lattice
  = LatticeLike<L> and requires(const L &l) {
                         { l.extents() } -> std::convertible_to<typename L::extents_type>;
                       };

/** @brief Concept: engine that exposes the minimum surface needed by
 *         checkpoint save/load.
 *
 *  The contract covers both directions of the round-trip:
 *
 *    - `frame()`         — read access used by `save_checkpoint`.
 *    - `steps_run()`     — current step counter, serialised verbatim.
 *    - `rule()`          — needed to detect `Has_Master_Seed` and
 *                          restore the master RNG seed on load.
 *    - `write_lattice()` — mutable handle the loader fills cell by
 *                          cell.
 *    - `set_step_count(s)` — restore the recorded step counter on load.
 *
 *  Both `Synchronous_Engine` and `Async_Engine` satisfy this contract
 *  out of the box; engines lacking any of these accessors fail at the
 *  concept boundary (cleaner than a deep template error).
 *
 *  @tparam E engine type.
 */
template <typename E>
concept Checkpointable_Engine
  = Checkpointable_Lattice<typename E::lattice_type>
    and requires { typename E::rule_type; }
    and requires(E &e, std::size_t s) {
          { e.frame() } -> std::convertible_to<const typename E::lattice_type &>;
          { e.steps_run() } -> std::convertible_to<std::size_t>;
          { e.rule() } -> std::convertible_to<typename E::rule_type &>;
          { e.write_lattice() } -> std::convertible_to<typename E::lattice_type &>;
          e.set_step_count(s);
        };

/** @brief Resume handle returned by `load_checkpoint_into`.
 *
 *  Captures the verified header of the checkpoint file from which the
 *  engine was rebuilt. Logging or auditing tools can persist the
 *  token alongside the engine to record the exact provenance of a
 *  long-running simulation.
 */
struct Resume_Token
{
  /// Path of the checkpoint file consumed by `load`.
  std::filesystem::path source_path;
  /// Validated header read from `source_path`.
  Checkpoint_Header header{};
};

/** @brief Read just the header from a checkpoint file.
 *
 *  @param[in] path checkpoint file to inspect.
 *  @return validated header.
 *  @throws std::runtime_error on missing file, bad magic, version
 *          mismatch or short read.
 */
[[nodiscard]] inline Checkpoint_Header inspect_checkpoint(const std::filesystem::path &path)
{
  using namespace ca_checkpoint_detail;
  std::ifstream in(path, std::ios::binary);
  ah_runtime_error_if(not in)
    << "inspect_checkpoint: cannot open '" << path.string() << "'";

  Checkpoint_Header h;
  in.read(h.magic.data(), h.magic.size());
  ah_runtime_error_if(not in or h.magic != magic_bytes)
    << "inspect_checkpoint: bad magic in '" << path.string() << "'";

  h.format_version = read_le<std::uint32_t>(in);
  ah_runtime_error_if(h.format_version != format_version)
    << "inspect_checkpoint: unsupported format version " << h.format_version
    << " (expected " << format_version << ") in '" << path.string() << "'";

  h.engine_type_hash = read_le<std::uint64_t>(in);
  h.state_type_size = read_le<std::uint32_t>(in);
  h.rank = read_le<std::uint32_t>(in);
  // save_checkpoint and load_checkpoint_into currently implement
  // rank-1, rank-2 and rank-3 serialisation branches; rank-4 is
  // intentionally rejected here so that a corrupted or future-version
  // header cannot bypass the save/load symmetry. Lift the upper bound
  // when (and only when) a rank-4 branch is added to both
  // serialiser and deserialiser.
  ah_runtime_error_if(h.rank == 0 or h.rank > 3)
    << "inspect_checkpoint: rank " << h.rank << " out of range in '" << path.string() << "'";
  for (std::size_t d = 0; d < h.extents.size(); ++d)
    h.extents[d] = read_le<std::uint64_t>(in);
  h.step_count = read_le<std::uint64_t>(in);
  h.has_rng = read_le<std::uint8_t>(in);
  // 7 bytes of padding to keep the next field 8-aligned in the file.
  for (int i = 0; i < 7; ++i)
    (void) read_le<std::uint8_t>(in);
  h.master_seed = read_le<std::uint64_t>(in);
  h.cell_count = read_le<std::uint64_t>(in);

  ah_runtime_error_if(not in)
    << "inspect_checkpoint: short read while parsing header of '" << path.string() << "'";
  return h;
}

/** @brief Write a complete engine snapshot to disk.
 *
 *  The frame is serialised cell-by-cell in row-major order. The
 *  on-disk byte layout is fixed and reproducible: two checkpoints of
 *  the same engine state always produce identical files.
 *
 *  @tparam Engine engine type satisfying `Checkpointable_Engine`.
 *  @param[in] engine engine whose state is captured.
 *  @param[in] path destination file path. Parent directories must
 *             already exist (the function does not `mkdir -p`).
 *  @throws std::runtime_error on file open or stream write failure.
 *  @throws std::out_of_range if the lattice rank exceeds 4.
 *
 *  @par Complexity
 *  O(N) where N is the cell count.
 */
template <typename Engine>
void save_checkpoint(Engine &engine, const std::filesystem::path &path)
  requires Checkpointable_Engine<Engine>
{
  using namespace ca_checkpoint_detail;
  using Lattice = typename Engine::lattice_type;
  using state_t = typename Lattice::state_type;
  using coord_t = typename Lattice::coord_type;
  static_assert(std::is_trivially_copyable_v<state_t>,
                "save_checkpoint requires a trivially copyable state_type");
  static_assert(Lattice::rank <= 3,
                "save_checkpoint currently supports rank <= 3");

  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  ah_runtime_error_if(not out)
    << "save_checkpoint: cannot open '" << path.string() << "'";

  // ---- Header --------------------------------------------------------
  out.write(magic_bytes.data(), magic_bytes.size());
  write_le<std::uint32_t>(out, format_version);
  write_le<std::uint64_t>(out, type_hash<Engine>());
  write_le<std::uint32_t>(out, static_cast<std::uint32_t>(sizeof(state_t)));
  write_le<std::uint32_t>(out, static_cast<std::uint32_t>(Lattice::rank));

  const auto &ext = engine.frame().extents();
  std::array<std::uint64_t, 4> ext_buf{};
  for (std::size_t d = 0; d < Lattice::rank; ++d)
    ext_buf[d] = static_cast<std::uint64_t>(ext[d]);
  for (std::size_t d = 0; d < ext_buf.size(); ++d)
    write_le<std::uint64_t>(out, ext_buf[d]);
  write_le<std::uint64_t>(out, static_cast<std::uint64_t>(engine.steps_run()));

  if constexpr (Has_Master_Seed<typename Engine::rule_type>)
    {
      write_le<std::uint8_t>(out, std::uint8_t{1});
      for (int i = 0; i < 7; ++i)
        write_le<std::uint8_t>(out, std::uint8_t{0});
      write_le<std::uint64_t>(out, static_cast<std::uint64_t>(engine.rule().master_seed()));
    }
  else
    {
      write_le<std::uint8_t>(out, std::uint8_t{0});
      for (int i = 0; i < 7; ++i)
        write_le<std::uint8_t>(out, std::uint8_t{0});
      write_le<std::uint64_t>(out, std::uint64_t{0});
    }

  std::uint64_t cell_count = 1;
  for (std::size_t d = 0; d < Lattice::rank; ++d)
    cell_count *= ext_buf[d];
  write_le<std::uint64_t>(out, cell_count);

  // ---- Frame payload (row-major) -------------------------------------
  const Lattice &frame = engine.frame();
  if constexpr (Lattice::rank == 1)
    {
      for (ca_size_t i = 0; i < frame.size(0); ++i)
        {
          const state_t v = frame.at(coord_t{static_cast<ca_index_t>(i)});
          write_le<state_t>(out, v);
        }
    }
  else if constexpr (Lattice::rank == 2)
    {
      for (ca_size_t i = 0; i < frame.size(0); ++i)
        for (ca_size_t j = 0; j < frame.size(1); ++j)
          {
            const state_t v
              = frame.at(coord_t{static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)});
            write_le<state_t>(out, v);
          }
    }
  else if constexpr (Lattice::rank == 3)
    {
      for (ca_size_t i = 0; i < frame.size(0); ++i)
        for (ca_size_t j = 0; j < frame.size(1); ++j)
          for (ca_size_t k = 0; k < frame.size(2); ++k)
            {
              const state_t v = frame.at(coord_t{static_cast<ca_index_t>(i),
                                                 static_cast<ca_index_t>(j),
                                                 static_cast<ca_index_t>(k)});
              write_le<state_t>(out, v);
            }
    }

  out.flush();
  ah_runtime_error_if(not out)
    << "save_checkpoint: write failed for '" << path.string() << "'";
}

/** @brief Restore an engine's state in-place from a checkpoint file.
 *
 *  The engine must have been built with extents that match the
 *  snapshot. The function rebuilds the lattice contents cell-by-cell,
 *  restores the step counter and forwards the recorded master seed
 *  to the rule when both sides expose the `Has_Master_Seed`
 *  contract.
 *
 *  @tparam Engine engine type satisfying `Checkpointable_Engine`.
 *  @param[in,out] engine engine to populate.
 *  @param[in] path checkpoint file path.
 *  @return resume token recording the file path and validated header.
 *  @throws std::runtime_error on bad magic, version mismatch, type
 *          hash mismatch, extents mismatch or short read.
 *
 *  @par Complexity
 *  O(N) where N is the cell count.
 */
template <typename Engine>
[[nodiscard]] Resume_Token load_checkpoint_into(Engine &engine,
                                                const std::filesystem::path &path)
  requires Checkpointable_Engine<Engine>
{
  using namespace ca_checkpoint_detail;
  using Lattice = typename Engine::lattice_type;
  using state_t = typename Lattice::state_type;
  using coord_t = typename Lattice::coord_type;
  static_assert(std::is_trivially_copyable_v<state_t>,
                "load_checkpoint_into requires a trivially copyable state_type");

  Resume_Token token;
  token.source_path = path;
  token.header = inspect_checkpoint(path);

  ah_runtime_error_if(token.header.engine_type_hash != type_hash<Engine>())
    << "load_checkpoint_into: type hash mismatch in '" << path.string()
    << "' (file=" << token.header.engine_type_hash
    << " engine=" << type_hash<Engine>() << ")";
  ah_runtime_error_if(token.header.state_type_size != sizeof(state_t))
    << "load_checkpoint_into: state size mismatch in '" << path.string()
    << "' (file=" << token.header.state_type_size << " engine=" << sizeof(state_t) << ")";
  ah_runtime_error_if(token.header.rank != Lattice::rank)
    << "load_checkpoint_into: rank mismatch in '" << path.string()
    << "' (file=" << token.header.rank << " engine=" << Lattice::rank << ")";

  const auto &ext = engine.frame().extents();
  for (std::size_t d = 0; d < Lattice::rank; ++d)
    ah_runtime_error_if(token.header.extents[d] != static_cast<std::uint64_t>(ext[d]))
      << "load_checkpoint_into: extent[" << d << "] mismatch in '" << path.string()
      << "' (file=" << token.header.extents[d] << " engine=" << ext[d] << ")";

  // Re-open and skip past the header to read the frame payload.
  std::ifstream in(path, std::ios::binary);
  ah_runtime_error_if(not in)
    << "load_checkpoint_into: cannot reopen '" << path.string() << "'";
  // Header layout: 8 magic + 4 version + 8 type_hash + 4 state_size +
  //                4 rank + 4*8 extents + 8 step_count + 1 has_rng +
  //                7 padding + 8 master_seed + 8 cell_count.
  constexpr std::streamoff header_bytes
    = 8 + 4 + 8 + 4 + 4 + 4 * 8 + 8 + 1 + 7 + 8 + 8;
  in.seekg(header_bytes, std::ios::beg);

  // Read the frame in row-major order matching `save_checkpoint`.
  if constexpr (Lattice::rank == 1)
    {
      for (ca_size_t i = 0; i < engine.frame().size(0); ++i)
        engine.write_lattice().set(coord_t{static_cast<ca_index_t>(i)},
                                   read_le<state_t>(in));
    }
  else if constexpr (Lattice::rank == 2)
    {
      for (ca_size_t i = 0; i < engine.frame().size(0); ++i)
        for (ca_size_t j = 0; j < engine.frame().size(1); ++j)
          engine.write_lattice().set(
            coord_t{static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)},
            read_le<state_t>(in));
    }
  else if constexpr (Lattice::rank == 3)
    {
      for (ca_size_t i = 0; i < engine.frame().size(0); ++i)
        for (ca_size_t j = 0; j < engine.frame().size(1); ++j)
          for (ca_size_t k = 0; k < engine.frame().size(2); ++k)
            engine.write_lattice().set(coord_t{static_cast<ca_index_t>(i),
                                               static_cast<ca_index_t>(j),
                                               static_cast<ca_index_t>(k)},
                                       read_le<state_t>(in));
    }

  ah_runtime_error_if(not in)
    << "load_checkpoint_into: short read of frame payload in '" << path.string() << "'";

  // Restore step counter.
  engine.set_step_count(token.header.step_count);

  // Restore master RNG seed when both sides agree on the contract.
  if constexpr (Has_Master_Seed<typename Engine::rule_type>)
    {
      if (token.header.has_rng != 0)
        engine.rule().set_master_seed(token.header.master_seed);
    }
  return token;
}

// =====================================================================
// Periodic_Checkpoint_Observer
// =====================================================================

/** @brief Observer that auto-saves the engine state every `every`
 *         steps to a templated path.
 *
 *  The path pattern accepts a single `{step}` token which is
 *  replaced with the zero-padded step index. Snapshots are written
 *  after `on_step_end`, so the file always reflects the state *after*
 *  the step has completed.
 *
 *  Typical usage:
 *
 *  ```cpp
 *  Periodic_Checkpoint_Observer<MyEngine> obs(engine, 1000,
 *                                             "checkpoints/run_{step}.bin");
 *  ```
 *
 *  @pre The engine reference passed to the constructor must outlive
 *       the observer.
 *  @pre `every >= 1` (enforced by the constructor).
 *
 *  @note This class is not thread-safe. The engine, the observer
 *        and the underlying filesystem are assumed to be driven by
 *        a single thread. Use external synchronisation if a parallel
 *        engine fires `on_step_end` from multiple workers.
 *
 *  @tparam Engine engine type to checkpoint.
 */
template <typename Engine>
class Periodic_Checkpoint_Observer
{
  Engine *engine_;
  std::size_t every_;
  std::string pattern_;
  std::size_t zero_pad_;
  std::filesystem::path last_path_;

  /// Replace the first `{step}` placeholder with `step`.
  [[nodiscard]] std::filesystem::path format_path(const std::size_t step) const
  {
    constexpr std::string_view placeholder = "{step}";
    const std::size_t pos = pattern_.find(placeholder);
    std::string padded = std::to_string(step);
    if (padded.size() < zero_pad_)
      padded.insert(0, zero_pad_ - padded.size(), '0');
    if (pos == std::string::npos)
      return std::filesystem::path(pattern_ + "." + padded);
    return std::filesystem::path(pattern_.substr(0, pos) + padded
                                 + pattern_.substr(pos + placeholder.size()));
  }

public:
  /** @brief Build a periodic checkpoint observer.
   *
   *  @param[in,out] engine engine to checkpoint (must outlive the observer).
   *  @param[in] every steps between snapshots (must be ≥ 1).
   *  @param[in] path_pattern path with a single `{step}` placeholder.
   *  @param[in] zero_pad minimum width for the step number when
   *             substituting `{step}`.
   *  @throws std::domain_error if `every == 0`.
   */
  Periodic_Checkpoint_Observer(Engine &engine,
                               const std::size_t every,
                               std::string path_pattern,
                               const std::size_t zero_pad = 6)
    : engine_(&engine), every_(every), pattern_(std::move(path_pattern)), zero_pad_(zero_pad)
  {
    ah_domain_error_if(every == 0)
      << "Periodic_Checkpoint_Observer: every must be >= 1";
  }

  /** @brief Pre-step hook. No-op for this observer.
   *
   *  Snapshots are taken at end-of-step so the pre-step callback is
   *  intentionally empty. Provided to satisfy the `ObserverLike`
   *  concept and to keep the observer composable inside a
   *  `Composite_Observer`.
   *
   *  @tparam Lattice lattice type of the engine being observed.
   *  @param[in] step step that is about to start (unused).
   *  @param[in] frame current frame (unused).
   */
  template <typename Lattice>
  void on_step_begin(const std::size_t step, const Lattice &frame) noexcept
  {
    (void) step;
    (void) frame;
  }

  /** @brief Snapshot when `step % every == 0`.
   *
   *  @tparam Lattice lattice type.
   *  @param[in] step step that just completed.
   *  @throws Any exception propagated by `save_checkpoint`.
   */
  template <typename Lattice>
  void on_step_end(const std::size_t step, const Lattice &)
  {
    if (step == 0 or (step % every_) != 0)
      return;
    last_path_ = format_path(step);
    save_checkpoint(*engine_, last_path_);
  }

  /** @brief Path of the most recent checkpoint written by this observer.
   *
   *  Empty when no snapshot has been taken yet (e.g. because the
   *  total number of completed steps is still below `every`, or
   *  because `on_step_end` was never invoked). Callers that resume
   *  from this path should check `empty()` first.
   *
   *  @return const reference to the cached path of the most recent
   *          snapshot, or an empty path when none was produced.
   */
  [[nodiscard]] const std::filesystem::path &last_path() const noexcept
  {
    return last_path_;
  }
};

}  // namespace CA
}  // namespace Aleph

#endif  // CA_CHECKPOINT_H