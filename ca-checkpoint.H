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
 *  @brief Binary checkpoint format for `Aleph::CA` engines
 *         (Phase 15 + Phase 17 crash-safe / compress / async writer).
 *
 *  A *checkpoint* is a self-describing binary snapshot of an engine's
 *  state on disk. It captures everything required to resume a long-
 *  running simulation bit-by-bit:
 *
 *    - A magic + version + engine type-hash header that lets `load`
 *      reject incompatible files early.
 *    - The lattice extents and the current step counter.
 *    - The full frame data, optionally DEFLATE-compressed.
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
 *  Phase 17 additions:
 *
 *    - **Atomic-rename writes**: snapshots are first written to a
 *      `.tmp.<pid>.<rand>` sibling, `fsync`'d, then renamed onto the
 *      final path. A `kill -9` between flush and rename leaves any
 *      previous file intact.
 *    - **Optional compression**: `Checkpoint_Options{ .compress = true }`
 *      activates a DEFLATE pass on the frame payload (bumps the file
 *      to `format_version = 2`).
 *    - **`Async_Checkpoint_Writer`**: pumps checkpoint writes onto a
 *      dedicated worker thread so `step()` is never blocked by disk
 *      I/O.
 *    - **Delta snapshots** *(best-effort)*: when a baseline payload is
 *      available, `save_delta_checkpoint` persists only the cells that
 *      changed.
 *
 *  Backward compatibility: `load_checkpoint_into` and
 *  `inspect_checkpoint` accept both `format_version = 1` (Phase 15) and
 *  `format_version = 2` (Phase 17). New writes always emit v2.
 *
 *  Two top-level entry points:
 *
 *    - `save_checkpoint(engine, path[, options])` writes a complete snapshot.
 *    - `load_checkpoint_into(engine, path)` rebuilds the engine state
 *      in-place; a `Resume_Token` records the path and the validated
 *      header for later auditing.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef CA_CHECKPOINT_H
#define CA_CHECKPOINT_H

#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <concepts>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iosfwd>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

#if defined(_WIN32)
#  include <io.h>
#  include <windows.h>
#else
#  include <fcntl.h>
#  include <sys/stat.h>
#  include <sys/types.h>
#  include <unistd.h>
#endif

#include <ah-errors.H>

#include <ca-checkpoint-compress.H>
#include <ca-rng.H>
#include <ca-traits.H>
#include <tpl_ca_concepts.H>

namespace Aleph {
namespace CA {

namespace ca_checkpoint_detail {

/// Magic prefix identifying an Aleph CA checkpoint stream. The trailing
/// digit is kept fixed at `'1'` across format-version bumps; consumers
/// discriminate via `format_version` instead.
inline constexpr std::array<char, 8> magic_bytes
  = {{'A', 'L', 'E', 'P', 'H', 'C', 'A', '1'}};

/// Current format version. Bumped to 2 in Phase 17 to introduce the
/// `flags` / `compression_level` / `payload_size` / `delta_base_step`
/// trailer fields.
inline constexpr std::uint32_t format_version = 2;

/// Oldest format version this file knows how to read. Loaders accept
/// any version in `[format_version_min_read, format_version]`.
inline constexpr std::uint32_t format_version_min_read = 1;

/// Size in bytes of the v1 header (Phase 15 layout).
inline constexpr std::streamoff header_bytes_v1
  = 8 + 4 + 8 + 4 + 4 + 4 * 8 + 8 + 1 + 7 + 8 + 8;  // 92

/// Size in bytes of the v2 header (Phase 17 layout: v1 + 4 trailer
/// fields of 4 + 4 + 8 + 8 = 24 bytes).
inline constexpr std::streamoff header_bytes_v2 = header_bytes_v1 + 24;  // 116

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

// --- Atomic-write primitives (Phase 17) ------------------------------

/** @brief Generate a temporary sibling path for atomic writes.
 *
 *  The temporary name lives next to `target` so the final
 *  `std::filesystem::rename` is an atomic, same-filesystem operation.
 *
 *  @param[in] target final path that the caller wants to publish.
 *  @return path of the form `<target>.tmp.<pid>.<rand>`.
 */
[[nodiscard]] inline std::filesystem::path
make_tmp_path(const std::filesystem::path &target)
{
  std::mt19937_64 rng(static_cast<std::uint64_t>(
    std::chrono::steady_clock::now().time_since_epoch().count())
                      ^ static_cast<std::uint64_t>(
                        reinterpret_cast<std::uintptr_t>(&target)));
  const std::uint64_t r = rng();
#if defined(_WIN32)
  const long pid = static_cast<long>(::GetCurrentProcessId());
#else
  const long pid = static_cast<long>(::getpid());
#endif
  std::string suffix = ".tmp." + std::to_string(pid) + "." + std::to_string(r);
  std::filesystem::path tmp = target;
  tmp += suffix;
  return tmp;
}

/** @brief Force a file's contents to be durable on the underlying
 *         storage device.
 *
 *  Wraps `fsync` (POSIX) and `_commit` (Windows) on a freshly opened
 *  file descriptor of `path`. Callers must have already closed any
 *  buffered C++ stream attached to the same file.
 *
 *  @param[in] path file to fsync.
 *  @throws std::runtime_error on open/sync failure.
 */
inline void sync_file(const std::filesystem::path &path)
{
#if defined(_WIN32)
  HANDLE h = ::CreateFileW(path.wstring().c_str(),
                           GENERIC_WRITE,
                           FILE_SHARE_READ,
                           nullptr,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           nullptr);
  ah_runtime_error_if(h == INVALID_HANDLE_VALUE)
    << "sync_file: CreateFileW failed for '" << path.string() << "'";
  const BOOL ok = ::FlushFileBuffers(h);
  const DWORD err = ok ? 0 : ::GetLastError();
  ::CloseHandle(h);
  ah_runtime_error_if(not ok)
    << "sync_file: FlushFileBuffers failed for '" << path.string()
    << "' (error " << err << ")";
#else
  const int fd = ::open(path.c_str(), O_WRONLY);
  ah_runtime_error_if(fd < 0)
    << "sync_file: open() failed for '" << path.string()
    << "' (errno=" << errno << ")";
  const int rc = ::fsync(fd);
  const int saved = errno;
  ::close(fd);
  ah_runtime_error_if(rc != 0)
    << "sync_file: fsync() failed for '" << path.string()
    << "' (errno=" << saved << ")";
#endif
}

/** @brief Force a directory's metadata to be durable.
 *
 *  Required on POSIX to ensure a `rename(tmp, final)` survives a kernel
 *  crash. Windows reports the rename as durable via `FlushFileBuffers`
 *  on the renamed file, so this call is a no-op there.
 *
 *  @param[in] dir directory whose metadata to sync.
 */
inline void sync_directory(const std::filesystem::path &dir) noexcept
{
#if defined(_WIN32)
  (void) dir;
#else
  const int fd = ::open(dir.c_str(), O_RDONLY | O_DIRECTORY);
  if (fd < 0)
    return;
  (void) ::fsync(fd);
  ::close(fd);
#endif
}

/** @brief RAII guard that removes a temporary file unless committed.
 *
 *  Used during atomic writes so a thrown exception (or early return)
 *  always cleans up the partial `<path>.tmp.*` sibling.
 */
class Tmp_File_Guard
{
  std::filesystem::path path_;
  bool committed_ = false;

public:
  explicit Tmp_File_Guard(std::filesystem::path p) noexcept
    : path_(std::move(p))
  {}

  Tmp_File_Guard(const Tmp_File_Guard &) = delete;
  Tmp_File_Guard &operator=(const Tmp_File_Guard &) = delete;

  ~Tmp_File_Guard() noexcept
  {
    if (not committed_)
      {
        std::error_code ec;
        std::filesystem::remove(path_, ec);
      }
  }

  /// Mark the temporary file as successfully renamed; suppress cleanup.
  void commit() noexcept { committed_ = true; }

  /// Access the underlying path.
  [[nodiscard]] const std::filesystem::path &path() const noexcept { return path_; }
};

}  // namespace ca_checkpoint_detail

// =====================================================================
// Public types
// =====================================================================

/** @brief Bitset values used in `Checkpoint_Header::flags` (v2+).
 *
 *  Flags are namespaced to avoid clashing with `Aleph::CA` enums.
 */
namespace ca_checkpoint_flags {

/// Frame payload was written through DEFLATE (miniz).
inline constexpr std::uint32_t compressed = 0x1;

/// File contains only the cells that changed relative to a baseline.
/// `Checkpoint_Header::delta_base_step` identifies the baseline step.
inline constexpr std::uint32_t delta = 0x2;

}  // namespace ca_checkpoint_flags

/** @brief Public on-disk header surfaced by `inspect_checkpoint`.
 *
 *  Mirrors the layout serialised at the start of every checkpoint
 *  file. Useful for tooling: `inspect_checkpoint(path)` returns this
 *  struct without touching the frame payload.
 *
 *  v1 fields are populated from both v1 and v2 files; v2-only fields
 *  default to zero when the source file is v1.
 */
struct Checkpoint_Header
{
  /// Stream magic ("ALEPHCA1").
  std::array<char, 8> magic{};
  /// Format version (1 = Phase 15, 2 = Phase 17).
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
  /// Number of cells stored in the frame payload (uncompressed view).
  std::uint64_t cell_count = 0;

  // --- v2-only trailer fields (default to 0 when reading v1) --------

  /// Bitmask of `ca_checkpoint_flags::*` values.
  std::uint32_t flags = 0;
  /// DEFLATE compression level used when `flags & compressed`.
  std::uint32_t compression_level = 0;
  /// Actual payload byte count on disk (compressed size when
  /// `flags & compressed`, or `(cell_count * state_type_size)` for raw
  /// snapshots; delta layout when `flags & delta`).
  std::uint64_t payload_size = 0;
  /// Step number of the baseline snapshot this delta references
  /// (meaningful only when `flags & delta`).
  std::uint64_t delta_base_step = 0;
};

/** @brief User-tunable knobs for `save_checkpoint`.
 *
 *  Defaults preserve the Phase 15 behaviour (raw, uncompressed bytes,
 *  no `fsync` on the parent directory). Phase 17 callers that care
 *  about crash safety should keep `sync_dir = true`.
 */
struct Checkpoint_Options
{
  /// Enable DEFLATE compression of the frame payload.
  bool compress = false;
  /// DEFLATE level in `[1, 9]`. `0` falls back to `6`.
  int level = 6;
  /// Call `fsync` on the parent directory after the atomic rename.
  /// Costly but required for survival across kernel crashes.
  bool sync_dir = true;
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

// =====================================================================
// inspect_checkpoint
// =====================================================================

/** @brief Read just the header from a checkpoint file.
 *
 *  Accepts both `format_version = 1` (Phase 15) and
 *  `format_version = 2` (Phase 17). v1 readers see v2-only fields as
 *  zero; flags / payload metadata are then synthesised from the
 *  defaults (uncompressed, full snapshot).
 *
 *  @param[in] path checkpoint file to inspect.
 *  @return validated header.
 *  @throws std::runtime_error on missing file, bad magic, version
 *          mismatch or short read.
 */
[[nodiscard]] inline Checkpoint_Header
inspect_checkpoint(const std::filesystem::path &path)
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
  ah_runtime_error_if(h.format_version < format_version_min_read
                      or h.format_version > format_version)
    << "inspect_checkpoint: unsupported format version " << h.format_version
    << " (supported [" << format_version_min_read << ", " << format_version
    << "]) in '" << path.string() << "'";

  h.engine_type_hash = read_le<std::uint64_t>(in);
  h.state_type_size = read_le<std::uint32_t>(in);
  h.rank = read_le<std::uint32_t>(in);
  // save_checkpoint and load_checkpoint_into currently implement
  // rank-1, rank-2 and rank-3 serialisation branches; rank-4 is
  // intentionally rejected here so that a corrupted or future-version
  // header cannot bypass the save/load symmetry.
  ah_runtime_error_if(h.rank == 0 or h.rank > 3)
    << "inspect_checkpoint: rank " << h.rank << " out of range in '"
    << path.string() << "'";
  for (std::size_t d = 0; d < h.extents.size(); ++d)
    h.extents[d] = read_le<std::uint64_t>(in);
  h.step_count = read_le<std::uint64_t>(in);
  h.has_rng = read_le<std::uint8_t>(in);
  // 7 bytes of padding to keep the next field 8-aligned in the file.
  for (int i = 0; i < 7; ++i)
    (void) read_le<std::uint8_t>(in);
  h.master_seed = read_le<std::uint64_t>(in);
  h.cell_count = read_le<std::uint64_t>(in);

  if (h.format_version >= 2)
    {
      h.flags = read_le<std::uint32_t>(in);
      h.compression_level = read_le<std::uint32_t>(in);
      h.payload_size = read_le<std::uint64_t>(in);
      h.delta_base_step = read_le<std::uint64_t>(in);
    }
  else
    {
      h.flags = 0;
      h.compression_level = 0;
      h.payload_size = h.cell_count * h.state_type_size;
      h.delta_base_step = 0;
    }

  ah_runtime_error_if(not in)
    << "inspect_checkpoint: short read while parsing header of '"
    << path.string() << "'";
  return h;
}

// =====================================================================
// Frame serialisation helpers (shared by sync / async / delta paths)
// =====================================================================

namespace ca_checkpoint_detail {

/** @brief Flatten an engine frame into a row-major byte buffer.
 *
 *  Shared by `save_checkpoint`, `Async_Checkpoint_Writer::submit` and
 *  `save_delta_checkpoint`. The output is laid out with the host's
 *  little-endian representation of `state_t`, matching the on-disk
 *  layout.
 *
 *  @tparam Engine engine type satisfying `Checkpointable_Engine`.
 *  @param[in] engine engine to snapshot.
 *  @return owning byte buffer with `cell_count * sizeof(state_t)` bytes.
 */
template <typename Engine>
[[nodiscard]] std::vector<std::uint8_t> snapshot_frame_bytes(const Engine &engine)
  requires Checkpointable_Engine<Engine>
{
  using Lattice = typename Engine::lattice_type;
  using state_t = typename Lattice::state_type;
  using coord_t = typename Lattice::coord_type;
  static_assert(std::is_trivially_copyable_v<state_t>,
                "snapshot_frame_bytes requires a trivially copyable state_type");
  static_assert(Lattice::rank <= 3,
                "snapshot_frame_bytes currently supports rank <= 3");

  const Lattice &frame = engine.frame();
  std::uint64_t cell_count = 1;
  for (std::size_t d = 0; d < Lattice::rank; ++d)
    cell_count *= static_cast<std::uint64_t>(frame.size(d));

  std::vector<std::uint8_t> bytes(static_cast<std::size_t>(cell_count) * sizeof(state_t));
  std::size_t off = 0;
  auto append = [&](const state_t &v)
  {
    std::memcpy(bytes.data() + off, &v, sizeof(state_t));
    off += sizeof(state_t);
  };

  if constexpr (Lattice::rank == 1)
    {
      for (ca_size_t i = 0; i < frame.size(0); ++i)
        append(frame.at(coord_t{static_cast<ca_index_t>(i)}));
    }
  else if constexpr (Lattice::rank == 2)
    {
      for (ca_size_t i = 0; i < frame.size(0); ++i)
        for (ca_size_t j = 0; j < frame.size(1); ++j)
          append(frame.at(coord_t{static_cast<ca_index_t>(i),
                                  static_cast<ca_index_t>(j)}));
    }
  else if constexpr (Lattice::rank == 3)
    {
      for (ca_size_t i = 0; i < frame.size(0); ++i)
        for (ca_size_t j = 0; j < frame.size(1); ++j)
          for (ca_size_t k = 0; k < frame.size(2); ++k)
            append(frame.at(coord_t{static_cast<ca_index_t>(i),
                                    static_cast<ca_index_t>(j),
                                    static_cast<ca_index_t>(k)}));
    }
  return bytes;
}

/// Inverse of `snapshot_frame_bytes`: spray the buffer back into the
/// engine's write lattice in row-major order.
template <typename Engine>
void restore_frame_from_bytes(Engine &engine, const std::vector<std::uint8_t> &bytes)
  requires Checkpointable_Engine<Engine>
{
  using Lattice = typename Engine::lattice_type;
  using state_t = typename Lattice::state_type;
  using coord_t = typename Lattice::coord_type;
  static_assert(std::is_trivially_copyable_v<state_t>,
                "restore_frame_from_bytes requires a trivially copyable state_type");
  static_assert(Lattice::rank <= 3,
                "restore_frame_from_bytes currently supports rank <= 3");

  const Lattice &frame = engine.frame();
  std::uint64_t cell_count = 1;
  for (std::size_t d = 0; d < Lattice::rank; ++d)
    cell_count *= static_cast<std::uint64_t>(frame.size(d));
  ah_runtime_error_if(bytes.size() != static_cast<std::size_t>(cell_count) * sizeof(state_t))
    << "restore_frame_from_bytes: size mismatch (expected "
    << (cell_count * sizeof(state_t)) << ", got " << bytes.size() << ")";

  std::size_t off = 0;
  auto next = [&]() -> state_t
  {
    state_t v{};
    std::memcpy(&v, bytes.data() + off, sizeof(state_t));
    off += sizeof(state_t);
    return v;
  };

  if constexpr (Lattice::rank == 1)
    {
      for (ca_size_t i = 0; i < frame.size(0); ++i)
        engine.write_lattice().set(coord_t{static_cast<ca_index_t>(i)}, next());
    }
  else if constexpr (Lattice::rank == 2)
    {
      for (ca_size_t i = 0; i < frame.size(0); ++i)
        for (ca_size_t j = 0; j < frame.size(1); ++j)
          engine.write_lattice().set(
            coord_t{static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)},
            next());
    }
  else if constexpr (Lattice::rank == 3)
    {
      for (ca_size_t i = 0; i < frame.size(0); ++i)
        for (ca_size_t j = 0; j < frame.size(1); ++j)
          for (ca_size_t k = 0; k < frame.size(2); ++k)
            engine.write_lattice().set(coord_t{static_cast<ca_index_t>(i),
                                               static_cast<ca_index_t>(j),
                                               static_cast<ca_index_t>(k)},
                                       next());
    }
}

/** @brief Header bundle pre-computed from an engine.
 *
 *  Internal helper shared by the sync and async write paths. Holds the
 *  values that go into the file header before serialisation, so the
 *  async writer can capture them once at submit time without keeping a
 *  pointer to the engine alive.
 */
struct Header_Snapshot
{
  std::uint64_t engine_type_hash = 0;
  std::uint32_t state_type_size = 0;
  std::uint32_t rank = 0;
  std::array<std::uint64_t, 4> extents{};
  std::uint64_t step_count = 0;
  std::uint8_t has_rng = 0;
  std::uint64_t master_seed = 0;
  std::uint64_t cell_count = 0;
};

/** @brief Capture all header fields a checkpoint file needs.
 *
 *  Pulls the values from the engine in a single synchronous pass so
 *  the async writer can be fed an opaque task without holding a
 *  reference to the engine across worker activations.
 *
 *  @tparam Engine engine type satisfying `Checkpointable_Engine`.
 *  @param[in] engine engine to snapshot.
 *  @return populated `Header_Snapshot`.
 */
template <typename Engine>
[[nodiscard]] Header_Snapshot capture_header(const Engine &engine)
  requires Checkpointable_Engine<Engine>
{
  using Lattice = typename Engine::lattice_type;
  using state_t = typename Lattice::state_type;
  Header_Snapshot hs;
  hs.engine_type_hash = type_hash<Engine>();
  hs.state_type_size = static_cast<std::uint32_t>(sizeof(state_t));
  hs.rank = static_cast<std::uint32_t>(Lattice::rank);
  const auto &ext = engine.frame().extents();
  for (std::size_t d = 0; d < Lattice::rank; ++d)
    hs.extents[d] = static_cast<std::uint64_t>(ext[d]);
  hs.step_count = static_cast<std::uint64_t>(engine.steps_run());
  std::uint64_t n = 1;
  for (std::size_t d = 0; d < Lattice::rank; ++d)
    n *= hs.extents[d];
  hs.cell_count = n;
  if constexpr (Has_Master_Seed<typename Engine::rule_type>)
    {
      hs.has_rng = 1;
      hs.master_seed = static_cast<std::uint64_t>(engine.rule().master_seed());
    }
  return hs;
}

/// Serialise the v2 header bytes for a checkpoint file.
inline void write_header_bytes(std::ostream &out,
                               const Header_Snapshot &hs,
                               const std::uint32_t flags,
                               const std::uint32_t compression_level,
                               const std::uint64_t payload_size,
                               const std::uint64_t delta_base_step)
{
  out.write(magic_bytes.data(), magic_bytes.size());
  write_le<std::uint32_t>(out, format_version);
  write_le<std::uint64_t>(out, hs.engine_type_hash);
  write_le<std::uint32_t>(out, hs.state_type_size);
  write_le<std::uint32_t>(out, hs.rank);
  for (std::size_t d = 0; d < hs.extents.size(); ++d)
    write_le<std::uint64_t>(out, hs.extents[d]);
  write_le<std::uint64_t>(out, hs.step_count);
  write_le<std::uint8_t>(out, hs.has_rng);
  for (int i = 0; i < 7; ++i)
    write_le<std::uint8_t>(out, std::uint8_t{0});
  write_le<std::uint64_t>(out, hs.master_seed);
  write_le<std::uint64_t>(out, hs.cell_count);
  write_le<std::uint32_t>(out, flags);
  write_le<std::uint32_t>(out, compression_level);
  write_le<std::uint64_t>(out, payload_size);
  write_le<std::uint64_t>(out, delta_base_step);
}

/** @brief Write `header + payload` atomically to `path`.
 *
 *  Uses the temp-file + fsync + rename pattern. On any failure the
 *  temporary file is removed by `Tmp_File_Guard`; the final `path`
 *  retains its previous contents (or stays absent if it did not exist
 *  before the call).
 */
inline void atomic_write_file(const std::filesystem::path &path,
                              const Header_Snapshot &hs,
                              const std::uint32_t flags,
                              const std::uint32_t compression_level,
                              const std::vector<std::uint8_t> &payload,
                              const std::uint64_t delta_base_step,
                              const Checkpoint_Options &options)
{
  const std::filesystem::path tmp = make_tmp_path(path);
  Tmp_File_Guard guard(tmp);
  {
    std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
    ah_runtime_error_if(not out)
      << "atomic_write_file: cannot open '" << tmp.string() << "' for write";
    write_header_bytes(out,
                       hs,
                       flags,
                       compression_level,
                       static_cast<std::uint64_t>(payload.size()),
                       delta_base_step);
    if (not payload.empty())
      out.write(reinterpret_cast<const char *>(payload.data()),
                static_cast<std::streamsize>(payload.size()));
    out.flush();
    ah_runtime_error_if(not out)
      << "atomic_write_file: write failed for '" << tmp.string() << "'";
  }
  sync_file(tmp);

  std::error_code ec;
  std::filesystem::rename(tmp, path, ec);
  ah_runtime_error_if(ec)
    << "atomic_write_file: rename '" << tmp.string() << "' -> '"
    << path.string() << "' failed: " << ec.message();
  guard.commit();

  if (options.sync_dir)
    {
      std::filesystem::path parent = path.parent_path();
      if (parent.empty())
        parent = std::filesystem::path(".");
      sync_directory(parent);
    }
}

/// Build the (possibly compressed) frame payload from a flat byte
/// buffer, honouring `options.compress`.
inline std::vector<std::uint8_t>
build_payload(const std::vector<std::uint8_t> &raw,
              const Checkpoint_Options &options,
              std::uint32_t &flags_out,
              std::uint32_t &level_out)
{
  if (not options.compress or raw.empty())
    {
      flags_out = 0;
      level_out = 0;
      return raw;
    }
  const int level = (options.level <= 0) ? 6 : options.level;
  std::vector<std::uint8_t> compressed = deflate_bytes(raw.data(), raw.size(), level);
  flags_out = ca_checkpoint_flags::compressed;
  level_out = static_cast<std::uint32_t>(level);
  return compressed;
}

/// Read the raw, uncompressed payload from a checkpoint file given a
/// validated header.
inline std::vector<std::uint8_t>
read_raw_payload(const std::filesystem::path &path, const Checkpoint_Header &h)
{
  std::ifstream in(path, std::ios::binary);
  ah_runtime_error_if(not in)
    << "read_raw_payload: cannot reopen '" << path.string() << "'";
  const std::streamoff header_bytes
    = (h.format_version >= 2) ? header_bytes_v2 : header_bytes_v1;
  in.seekg(header_bytes, std::ios::beg);
  std::vector<std::uint8_t> payload(static_cast<std::size_t>(h.payload_size));
  if (not payload.empty())
    {
      in.read(reinterpret_cast<char *>(payload.data()),
              static_cast<std::streamsize>(payload.size()));
      ah_runtime_error_if(not in)
        << "read_raw_payload: short read of payload in '" << path.string() << "'";
    }
  if (h.flags & ca_checkpoint_flags::compressed)
    {
      const std::size_t expected
        = static_cast<std::size_t>(h.cell_count) * h.state_type_size;
      return inflate_bytes(payload.data(), payload.size(), expected);
    }
  return payload;
}

}  // namespace ca_checkpoint_detail

// =====================================================================
// save_checkpoint
// =====================================================================

/** @brief Write a complete engine snapshot to disk atomically.
 *
 *  The frame is serialised cell-by-cell in row-major order, then
 *  optionally DEFLATE-compressed (when `options.compress`). The file
 *  is first emitted to `<path>.tmp.<pid>.<rand>`, `fsync`'d and finally
 *  renamed onto `path`. A crash before the rename leaves any previous
 *  `path` intact.
 *
 *  Two checkpoints of the same engine state always produce identical
 *  bytes when the same `options` are used, so the format remains
 *  reproducible across runs.
 *
 *  @tparam Engine engine type satisfying `Checkpointable_Engine`.
 *  @param[in] engine engine whose state is captured.
 *  @param[in] path destination file path. Parent directories must
 *             already exist (the function does not `mkdir -p`).
 *  @param[in] options optional knobs (compression, dir-fsync). Defaults
 *             preserve Phase-15 raw output but keep `sync_dir = true`
 *             for crash safety.
 *  @throws std::runtime_error on file open, stream write, fsync or
 *          rename failure.
 *  @note Requires `Lattice::rank <= 3`. Higher ranks fail at compile
 *        time via `static_assert`.
 *
 *  @par Complexity
 *  O(N) where N is the cell count.
 */
template <typename Engine>
void save_checkpoint(Engine &engine,
                     const std::filesystem::path &path,
                     const Checkpoint_Options &options = {})
  requires Checkpointable_Engine<Engine>
{
  using namespace ca_checkpoint_detail;
  const Header_Snapshot hs = capture_header(engine);
  std::vector<std::uint8_t> raw = snapshot_frame_bytes(engine);
  std::uint32_t flags = 0;
  std::uint32_t level = 0;
  std::vector<std::uint8_t> payload = build_payload(raw, options, flags, level);
  atomic_write_file(path, hs, flags, level, payload, /*delta_base_step=*/0, options);
}

// =====================================================================
// load_checkpoint_into
// =====================================================================

/** @brief Restore an engine's state in-place from a checkpoint file.
 *
 *  Accepts both v1 (Phase 15) and v2 (Phase 17) files; compressed and
 *  uncompressed payloads are handled transparently. Delta files
 *  (`flags & delta`) are *not* accepted here — use
 *  `apply_delta_checkpoint` for those.
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
 *          hash mismatch, extents mismatch, short read or attempts to
 *          load a delta file with this entry point.
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
  ah_runtime_error_if(token.header.flags & ca_checkpoint_flags::delta)
    << "load_checkpoint_into: '" << path.string() << "' is a delta snapshot; "
    << "use apply_delta_checkpoint() after loading the baseline";

  const auto &ext = engine.frame().extents();
  for (std::size_t d = 0; d < Lattice::rank; ++d)
    ah_runtime_error_if(token.header.extents[d] != static_cast<std::uint64_t>(ext[d]))
      << "load_checkpoint_into: extent[" << d << "] mismatch in '" << path.string()
      << "' (file=" << token.header.extents[d] << " engine=" << ext[d] << ")";

  std::vector<std::uint8_t> raw = read_raw_payload(path, token.header);
  restore_frame_from_bytes(engine, raw);

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
// Delta snapshots (best-effort)
// =====================================================================

/** @brief Write a delta checkpoint relative to a previous payload.
 *
 *  Compares the current engine frame against the raw byte payload of
 *  the baseline snapshot and persists only the cells whose byte
 *  representation differs. The file references the baseline via its
 *  step number (`base_step`) so consumers know which file to load
 *  first.
 *
 *  Delta payload layout (uncompressed view, repeats `n` times):
 *
 *  ```
 *  [ uint64 linear_index ][ state_type_size bytes new_value ]
 *  ```
 *
 *  When `options.compress` is set, the entire delta block is
 *  DEFLATE-compressed in one go (same as a full snapshot).
 *
 *  @tparam Engine engine type satisfying `Checkpointable_Engine`.
 *  @param[in] engine engine whose state will be diffed.
 *  @param[in] baseline raw byte payload of the previous snapshot
 *             (typically captured at the time the baseline was
 *             written). The size must equal `cell_count * sizeof(state)`.
 *  @param[in] base_step step number of the baseline checkpoint.
 *  @param[in] path destination file path.
 *  @param[in] options optional knobs (compression, dir-fsync).
 *  @throws std::runtime_error on size mismatch or write failure.
 *
 *  @par Complexity
 *  O(N) cell scan; output is proportional to the number of changes.
 */
template <typename Engine>
void save_delta_checkpoint(Engine &engine,
                           const std::vector<std::uint8_t> &baseline,
                           const std::uint64_t base_step,
                           const std::filesystem::path &path,
                           const Checkpoint_Options &options = {})
  requires Checkpointable_Engine<Engine>
{
  using namespace ca_checkpoint_detail;
  using Lattice = typename Engine::lattice_type;
  using state_t = typename Lattice::state_type;
  static_assert(std::is_trivially_copyable_v<state_t>,
                "save_delta_checkpoint requires a trivially copyable state_type");

  const Header_Snapshot hs = capture_header(engine);
  const std::size_t cell_bytes = sizeof(state_t);
  const std::size_t total_cells = static_cast<std::size_t>(hs.cell_count);
  ah_runtime_error_if(baseline.size() != total_cells * cell_bytes)
    << "save_delta_checkpoint: baseline size mismatch (expected "
    << (total_cells * cell_bytes) << ", got " << baseline.size() << ")";

  std::vector<std::uint8_t> current = snapshot_frame_bytes(engine);
  std::vector<std::uint8_t> diff;
  diff.reserve(64);  // arbitrary small reserve to avoid tiny reallocs.
  for (std::size_t i = 0; i < total_cells; ++i)
    {
      const std::size_t off = i * cell_bytes;
      if (std::memcmp(current.data() + off,
                      baseline.data() + off,
                      cell_bytes)
          != 0)
        {
          const std::uint64_t idx = static_cast<std::uint64_t>(i);
          const std::size_t head = diff.size();
          diff.resize(head + sizeof(std::uint64_t) + cell_bytes);
          std::memcpy(diff.data() + head, &idx, sizeof(std::uint64_t));
          std::memcpy(diff.data() + head + sizeof(std::uint64_t),
                      current.data() + off,
                      cell_bytes);
        }
    }

  std::uint32_t flags = ca_checkpoint_flags::delta;
  std::uint32_t level = 0;
  std::vector<std::uint8_t> payload;
  if (options.compress and not diff.empty())
    {
      const int lvl = (options.level <= 0) ? 6 : options.level;
      payload = deflate_bytes(diff.data(), diff.size(), lvl);
      flags |= ca_checkpoint_flags::compressed;
      level = static_cast<std::uint32_t>(lvl);
    }
  else
    payload = std::move(diff);

  atomic_write_file(path, hs, flags, level, payload, base_step, options);
}

/** @brief Apply a delta checkpoint to the engine's current state.
 *
 *  The caller must have already loaded the baseline snapshot (the one
 *  referenced by `header.delta_base_step`) into the engine, typically
 *  via `load_checkpoint_into`. This function then overlays the
 *  per-cell mutations recorded in the delta file.
 *
 *  @tparam Engine engine type satisfying `Checkpointable_Engine`.
 *  @param[in,out] engine engine to mutate in place.
 *  @param[in] path delta checkpoint path.
 *  @return resume token with the delta header (note: `step_count` is
 *          the target step recorded in the delta file).
 *  @throws std::runtime_error when the file is not a delta, when
 *          metadata disagrees with the engine, or on short reads.
 */
template <typename Engine>
[[nodiscard]] Resume_Token
apply_delta_checkpoint(Engine &engine, const std::filesystem::path &path)
  requires Checkpointable_Engine<Engine>
{
  using namespace ca_checkpoint_detail;
  using Lattice = typename Engine::lattice_type;
  using state_t = typename Lattice::state_type;
  using coord_t = typename Lattice::coord_type;
  static_assert(std::is_trivially_copyable_v<state_t>,
                "apply_delta_checkpoint requires a trivially copyable state_type");
  static_assert(Lattice::rank <= 3,
                "apply_delta_checkpoint currently supports rank <= 3");

  Resume_Token token;
  token.source_path = path;
  token.header = inspect_checkpoint(path);

  ah_runtime_error_if(not (token.header.flags & ca_checkpoint_flags::delta))
    << "apply_delta_checkpoint: '" << path.string() << "' is not a delta snapshot";
  ah_runtime_error_if(token.header.engine_type_hash != type_hash<Engine>())
    << "apply_delta_checkpoint: type hash mismatch in '" << path.string() << "'";
  ah_runtime_error_if(token.header.state_type_size != sizeof(state_t))
    << "apply_delta_checkpoint: state size mismatch in '" << path.string() << "'";
  ah_runtime_error_if(token.header.rank != Lattice::rank)
    << "apply_delta_checkpoint: rank mismatch in '" << path.string() << "'";

  std::ifstream in(path, std::ios::binary);
  ah_runtime_error_if(not in)
    << "apply_delta_checkpoint: cannot reopen '" << path.string() << "'";
  in.seekg(header_bytes_v2, std::ios::beg);
  std::vector<std::uint8_t> payload(static_cast<std::size_t>(token.header.payload_size));
  if (not payload.empty())
    {
      in.read(reinterpret_cast<char *>(payload.data()),
              static_cast<std::streamsize>(payload.size()));
      ah_runtime_error_if(not in)
        << "apply_delta_checkpoint: short read in '" << path.string() << "'";
    }

  std::vector<std::uint8_t> diff;
  if (token.header.flags & ca_checkpoint_flags::compressed)
    {
      // Upper bound on the uncompressed delta size: at most every cell
      // is mutated, producing `cell_count * (8 + state_size)` bytes.
      const std::size_t bound
        = static_cast<std::size_t>(token.header.cell_count)
          * (sizeof(std::uint64_t) + sizeof(state_t));
      diff = inflate_bytes_up_to(payload.data(), payload.size(), bound);
    }
  else
    diff = std::move(payload);

  const std::size_t cell_bytes = sizeof(state_t);
  const std::size_t entry_bytes = sizeof(std::uint64_t) + cell_bytes;
  ah_runtime_error_if(diff.size() % entry_bytes != 0)
    << "apply_delta_checkpoint: delta payload size " << diff.size()
    << " is not a multiple of (" << entry_bytes << ")";

  auto extents_from_header = [&](std::size_t d) -> ca_size_t
  { return static_cast<ca_size_t>(token.header.extents[d]); };

  const std::size_t entries = diff.size() / entry_bytes;
  for (std::size_t e = 0; e < entries; ++e)
    {
      std::uint64_t idx = 0;
      state_t value{};
      std::memcpy(&idx, diff.data() + e * entry_bytes, sizeof(std::uint64_t));
      std::memcpy(&value,
                  diff.data() + e * entry_bytes + sizeof(std::uint64_t),
                  cell_bytes);
      ah_runtime_error_if(idx >= token.header.cell_count)
        << "apply_delta_checkpoint: linear index " << idx
        << " out of range (cell_count=" << token.header.cell_count << ")";

      if constexpr (Lattice::rank == 1)
        {
          engine.write_lattice().set(
            coord_t{static_cast<ca_index_t>(idx)},
            value);
        }
      else if constexpr (Lattice::rank == 2)
        {
          const auto j = static_cast<ca_index_t>(idx % extents_from_header(1));
          const auto i = static_cast<ca_index_t>(idx / extents_from_header(1));
          engine.write_lattice().set(coord_t{i, j}, value);
        }
      else if constexpr (Lattice::rank == 3)
        {
          const auto e2 = extents_from_header(2);
          const auto e1 = extents_from_header(1);
          const auto k = static_cast<ca_index_t>(idx % e2);
          const auto j = static_cast<ca_index_t>((idx / e2) % e1);
          const auto i = static_cast<ca_index_t>(idx / (e1 * e2));
          engine.write_lattice().set(coord_t{i, j, k}, value);
        }
    }

  engine.set_step_count(token.header.step_count);
  if constexpr (Has_Master_Seed<typename Engine::rule_type>)
    {
      if (token.header.has_rng != 0)
        engine.rule().set_master_seed(token.header.master_seed);
    }
  return token;
}

// =====================================================================
// Async_Checkpoint_Writer
// =====================================================================

/** @brief Background writer pumping snapshots onto a dedicated thread.
 *
 *  Decouples `engine.step()` from disk I/O: `submit()` synchronously
 *  copies the current frame into an internal task buffer (a fast
 *  `memcpy`) and returns immediately; a worker thread later runs
 *  optional DEFLATE compression and the atomic-rename write.
 *
 *  Queue policy:
 *
 *    - `Block`        — `submit()` blocks until queue depth `< capacity`.
 *    - `Drop_Oldest`  — when the queue is full, the oldest pending task
 *                       is discarded and the new one enqueued.
 *
 *  The destructor blocks on `flush()` so no snapshot is silently lost
 *  when the writer goes out of scope.
 *
 *  Thread safety: `submit()` and `flush()` are safe to call from a
 *  single producer thread (typically the engine thread). The writer is
 *  *not* designed for multi-producer use — wrap it in your own
 *  synchronisation if multiple engines share the same writer.
 *
 *  @tparam Engine engine type satisfying `Checkpointable_Engine`.
 */
template <typename Engine>
class Async_Checkpoint_Writer
{
public:
  /// Behaviour when the internal queue is full.
  enum class Queue_Policy
  {
    Block,
    Drop_Oldest
  };

  /** @brief Construct the writer and spawn its worker thread.
   *
   *  @param[in] capacity maximum number of pending tasks. Must be `>= 1`.
   *  @param[in] policy what to do when `capacity` is reached.
   *  @throws std::domain_error when `capacity == 0`.
   */
  explicit Async_Checkpoint_Writer(const std::size_t capacity = 8,
                                   const Queue_Policy policy = Queue_Policy::Block)
    : capacity_(capacity)
    , policy_(policy)
  {
    ah_domain_error_if(capacity == 0)
      << "Async_Checkpoint_Writer: capacity must be >= 1";
    worker_ = std::thread([this] { this->worker_loop(); });
  }

  Async_Checkpoint_Writer(const Async_Checkpoint_Writer &) = delete;
  Async_Checkpoint_Writer &operator=(const Async_Checkpoint_Writer &) = delete;

  /// Destructor: drains the queue and joins the worker.
  ~Async_Checkpoint_Writer() noexcept
  {
    try
      {
        flush();
      }
    catch (...)
      {
        // Swallow flush errors during destruction; the worker thread
        // is still joined below.
      }
    {
      std::unique_lock<std::mutex> lk(mu_);
      stop_ = true;
      not_empty_.notify_all();
    }
    if (worker_.joinable())
      worker_.join();
  }

  /** @brief Synchronously capture the engine state and enqueue a write.
   *
   *  @param[in] engine engine to snapshot.
   *  @param[in] path destination path.
   *  @param[in] options compression / dir-fsync knobs.
   */
  void submit(Engine &engine,
              std::filesystem::path path,
              Checkpoint_Options options = {})
  {
    Task task;
    task.header = ca_checkpoint_detail::capture_header(engine);
    task.raw = ca_checkpoint_detail::snapshot_frame_bytes(engine);
    task.path = std::move(path);
    task.options = options;

    std::unique_lock<std::mutex> lk(mu_);
    if (policy_ == Queue_Policy::Drop_Oldest)
      {
        while (queue_.size() >= capacity_)
          {
            queue_.pop_front();
            total_dropped_.fetch_add(1, std::memory_order_relaxed);
          }
      }
    else
      {
        not_full_.wait(lk, [this] { return queue_.size() < capacity_ or stop_; });
      }
    queue_.push_back(std::move(task));
    not_empty_.notify_one();
  }

  /** @brief Block until the queue is drained.
   *
   *  Returns once every previously-submitted task has been written (or
   *  dropped). Subsequent `submit()` calls are still accepted.
   *
   *  @throws std::runtime_error rethrown from the worker if it
   *          captured a write failure.
   */
  void flush()
  {
    std::unique_lock<std::mutex> lk(mu_);
    drained_.wait(lk, [this] { return queue_.empty() and not in_flight_; });
    if (worker_error_)
      {
        const std::string msg = std::move(*worker_error_);
        worker_error_.reset();
        ah_runtime_error_if(true) << msg;
      }
  }

  /// Snapshot of pending queue depth (informational).
  [[nodiscard]] std::size_t pending() const
  {
    std::lock_guard<std::mutex> lk(mu_);
    return queue_.size();
  }

  /// Total number of files successfully written by the worker.
  [[nodiscard]] std::size_t total_written() const noexcept
  {
    return total_written_.load(std::memory_order_relaxed);
  }

  /// Total number of pending tasks discarded due to `Drop_Oldest`.
  [[nodiscard]] std::size_t total_dropped() const noexcept
  {
    return total_dropped_.load(std::memory_order_relaxed);
  }

  /// Configured queue capacity.
  [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }

  /// Configured back-pressure policy.
  [[nodiscard]] Queue_Policy policy() const noexcept { return policy_; }

private:
  struct Task
  {
    ca_checkpoint_detail::Header_Snapshot header;
    std::vector<std::uint8_t> raw;
    std::filesystem::path path;
    Checkpoint_Options options;
  };

  void worker_loop() noexcept
  {
    for (;;)
      {
        Task task;
        {
          std::unique_lock<std::mutex> lk(mu_);
          not_empty_.wait(lk, [this] { return not queue_.empty() or stop_; });
          if (queue_.empty() and stop_)
            return;
          task = std::move(queue_.front());
          queue_.pop_front();
          in_flight_ = true;
          not_full_.notify_one();
        }

        bool ok = true;
        std::string err;
        try
          {
            std::uint32_t flags = 0;
            std::uint32_t level = 0;
            std::vector<std::uint8_t> payload
              = ca_checkpoint_detail::build_payload(task.raw, task.options, flags, level);
            ca_checkpoint_detail::atomic_write_file(task.path,
                                                    task.header,
                                                    flags,
                                                    level,
                                                    payload,
                                                    /*delta_base_step=*/0,
                                                    task.options);
          }
        catch (const std::exception &e)
          {
            ok = false;
            err = e.what();
          }
        catch (...)
          {
            ok = false;
            err = "Async_Checkpoint_Writer: unknown exception";
          }

        {
          std::unique_lock<std::mutex> lk(mu_);
          in_flight_ = false;
          if (ok)
            total_written_.fetch_add(1, std::memory_order_relaxed);
          else if (not worker_error_)
            worker_error_ = std::move(err);
          if (queue_.empty())
            drained_.notify_all();
        }
      }
  }

  mutable std::mutex mu_;
  std::condition_variable not_empty_;
  std::condition_variable not_full_;
  std::condition_variable drained_;
  std::deque<Task> queue_;
  std::size_t capacity_;
  Queue_Policy policy_;
  bool stop_ = false;
  bool in_flight_ = false;
  std::optional<std::string> worker_error_;
  std::atomic<std::size_t> total_written_{0};
  std::atomic<std::size_t> total_dropped_{0};
  std::thread worker_;
};

// =====================================================================
// Periodic_Checkpoint_Observer
// =====================================================================

/** @brief Observer that auto-saves the engine state every `every`
 *         steps to a templated path.
 *
 *  The path pattern accepts a single `{step}` token which is replaced
 *  with the zero-padded step index. Snapshots are written after
 *  `on_step_end`, so the file always reflects the state *after* the
 *  step has completed.
 *
 *  When constructed with an `Async_Checkpoint_Writer`, every snapshot
 *  is forwarded to the writer's worker thread, keeping the engine's
 *  step loop free of disk I/O. Otherwise `save_checkpoint` is invoked
 *  synchronously.
 *
 *  Typical usage:
 *
 *  ```cpp
 *  Periodic_Checkpoint_Observer<MyEngine> obs(engine, 1000,
 *                                             "checkpoints/run_{step}.bin");
 *  ```
 *
 *  Or, with the async writer:
 *
 *  ```cpp
 *  Async_Checkpoint_Writer<MyEngine> writer(16);
 *  Periodic_Checkpoint_Observer<MyEngine> obs(engine, 1000,
 *                                             "ckpt/run_{step}.bin",
 *                                             writer);
 *  ```
 *
 *  @pre The engine reference passed to the constructor must outlive
 *       the observer.
 *  @pre The optional `Async_Checkpoint_Writer` reference must outlive
 *       the observer.
 *  @pre `every >= 1` (enforced by the constructor).
 *
 *  @note This class is not thread-safe. The engine, the observer and
 *        the underlying filesystem are assumed to be driven by a
 *        single thread. Use external synchronisation if a parallel
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
  Async_Checkpoint_Writer<Engine> *async_writer_ = nullptr;
  Checkpoint_Options options_{};

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
  /** @brief Build a synchronous periodic checkpoint observer.
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

  /** @brief Build a periodic observer routing writes through an
   *         `Async_Checkpoint_Writer`.
   *
   *  @param[in,out] engine engine to checkpoint (must outlive the observer).
   *  @param[in] every steps between snapshots (must be ≥ 1).
   *  @param[in] path_pattern path with a single `{step}` placeholder.
   *  @param[in,out] writer async writer (must outlive the observer).
   *  @param[in] options compression / fsync knobs forwarded to writes.
   *  @param[in] zero_pad minimum width for the step number when
   *             substituting `{step}`.
   *  @throws std::domain_error if `every == 0`.
   */
  Periodic_Checkpoint_Observer(Engine &engine,
                               const std::size_t every,
                               std::string path_pattern,
                               Async_Checkpoint_Writer<Engine> &writer,
                               Checkpoint_Options options = {},
                               const std::size_t zero_pad = 6)
    : engine_(&engine)
    , every_(every)
    , pattern_(std::move(path_pattern))
    , zero_pad_(zero_pad)
    , async_writer_(&writer)
    , options_(options)
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
   *  Writes through `Async_Checkpoint_Writer` when one was supplied at
   *  construction; otherwise calls `save_checkpoint` synchronously.
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
    if (async_writer_ != nullptr)
      async_writer_->submit(*engine_, last_path_, options_);
    else
      save_checkpoint(*engine_, last_path_, options_);
  }

  /** @brief Path of the most recent checkpoint scheduled by this
   *         observer.
   *
   *  Empty when no snapshot has been taken yet (e.g. because the total
   *  number of completed steps is still below `every`, or because
   *  `on_step_end` was never invoked). When the observer routes writes
   *  through `Async_Checkpoint_Writer`, this is the path the worker
   *  thread *will* (or has) written to — not a guarantee the file
   *  exists on disk; callers may need to call `writer.flush()` first.
   *
   *  @return const reference to the cached path, or an empty path when
   *          none was produced.
   */
  [[nodiscard]] const std::filesystem::path &last_path() const noexcept
  {
    return last_path_;
  }
};

}  // namespace CA
}  // namespace Aleph

#endif  // CA_CHECKPOINT_H