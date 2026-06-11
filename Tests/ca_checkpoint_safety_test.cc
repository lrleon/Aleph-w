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
 * @file ca_checkpoint_safety_test.cc
 * @brief Phase 17 safety/compression/async/delta tests for
 *        `ca-checkpoint.H`.
 *
 *  Coverage:
 *    - Compressed round-trip matches the raw round-trip.
 *    - Atomic-rename leaves any pre-existing target intact when the
 *      process dies between fsync and rename (POSIX only).
 *    - Background writer persists every submitted task by `flush()`.
 *    - `Queue_Policy::Drop_Oldest` actually drops entries.
 *    - DEFLATE compression ratio comfortably exceeds 4× on a sparse
 *      binary 1024² grid.
 *    - The reader still accepts hand-crafted v1 files (Phase 15
 *      backward compatibility).
 *    - Delta snapshots round-trip when applied on top of the baseline.
 */

#include <chrono>
#include <cstdint>
#include <thread>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <vector>

#if !defined(_WIN32)
#  include <sys/wait.h>
#  include <unistd.h>
#endif

#include <gtest/gtest.h>

#include <ca-checkpoint-compress.H>
#include <ca-checkpoint.H>
#include <tpl_ca_engine.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_rule.H>
#include <tpl_ca_storage.H>
#include <tpl_ca_stochastic_rules.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace
{

using Grid = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;
using Engine = Synchronous_Engine<Grid, Game_Of_Life_Rule, Moore<2, 1>>;

std::filesystem::path tmp_dir(const std::string &name)
{
  const auto tick = std::chrono::steady_clock::now().time_since_epoch().count();
  auto p = std::filesystem::temp_directory_path()
           / ("aleph_ca_ckpt_safety_" + name + "_"
              + std::to_string(static_cast<long long>(tick)));
  std::filesystem::create_directories(p);
  return p;
}

Grid random_grid(ca_size_t side, std::uint32_t seed, double p_alive = 0.4)
{
  std::mt19937 rng(seed);
  std::uniform_real_distribution<double> u(0.0, 1.0);
  Grid g({side, side}, 0);
  for (ca_size_t i = 0; i < side; ++i)
    for (ca_size_t j = 0; j < side; ++j)
      g.set({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)},
            u(rng) < p_alive ? 1 : 0);
  return g;
}

bool grids_equal(const Grid &a, const Grid &b)
{
  if (a.extents() != b.extents())
    return false;
  for (ca_size_t i = 0; i < a.size(0); ++i)
    for (ca_size_t j = 0; j < a.size(1); ++j)
      if (a.at({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)})
          != b.at({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)}))
        return false;
  return true;
}

std::size_t byte_count(const std::filesystem::path &p)
{
  std::error_code ec;
  const auto sz = std::filesystem::file_size(p, ec);
  return ec ? 0 : static_cast<std::size_t>(sz);
}

std::size_t count_tmp_siblings(const std::filesystem::path &target)
{
  std::size_t n = 0;
  const auto dir = target.parent_path();
  const auto stem = target.filename().string();
  for (const auto &entry : std::filesystem::directory_iterator(dir))
    {
      const auto name = entry.path().filename().string();
      if (name.rfind(stem + ".tmp.", 0) == 0)
        ++n;
    }
  return n;
}

}  // namespace

// =====================================================================
// Compressed round-trip matches the raw round-trip
// =====================================================================

TEST(CACheckpointSafety, CompressedRoundTripMatchesRaw)
{
  const auto dir = tmp_dir("compress_rt");
  const auto raw_path = dir / "raw.bin";
  const auto cmp_path = dir / "compressed.bin";

  Engine eng(random_grid(48, 0xC0FFEE), make_game_of_life_rule(), Moore<2, 1>{});
  eng.run(11);

  save_checkpoint(eng, raw_path);
  Checkpoint_Options opts;
  opts.compress = true;
  opts.level = 6;
  save_checkpoint(eng, cmp_path, opts);

  const auto raw_h = inspect_checkpoint(raw_path);
  const auto cmp_h = inspect_checkpoint(cmp_path);
  EXPECT_EQ(raw_h.flags & ca_checkpoint_flags::compressed, 0u);
  EXPECT_NE(cmp_h.flags & ca_checkpoint_flags::compressed, 0u);
  EXPECT_EQ(cmp_h.compression_level, 6u);
  EXPECT_EQ(raw_h.cell_count, cmp_h.cell_count);
  EXPECT_EQ(raw_h.step_count, cmp_h.step_count);

  // Restore both checkpoints into fresh engines and compare the frames.
  Engine restored_raw(Grid({48, 48}, 0), make_game_of_life_rule(), Moore<2, 1>{});
  Engine restored_cmp(Grid({48, 48}, 0), make_game_of_life_rule(), Moore<2, 1>{});
  load_checkpoint_into(restored_raw, raw_path);
  load_checkpoint_into(restored_cmp, cmp_path);

  EXPECT_TRUE(grids_equal(restored_raw.frame(), restored_cmp.frame()));
  EXPECT_EQ(restored_raw.steps_run(), restored_cmp.steps_run());

  // Compressed payload should be strictly smaller than raw for this
  // mid-density GoL pattern (cells are mostly 0/1 ints, so DEFLATE
  // catches the long runs of equal bytes).
  EXPECT_LT(byte_count(cmp_path), byte_count(raw_path));

  std::filesystem::remove_all(dir);
}

// =====================================================================
// Atomic-rename: a crash between fsync and rename leaves any
// pre-existing target intact and never publishes a partial file.
// =====================================================================

#if !defined(_WIN32)

TEST(CACheckpointSafety, CrashInjectionPreservesOriginalContents)
{
  const auto dir = tmp_dir("crash_preserve");
  const auto target = dir / "snap.bin";

  // Plant a baseline file that must survive an aborted write.
  {
    std::ofstream f(target, std::ios::binary);
    f << "PRE-EXISTING";
  }
  const std::string before = [&]
  {
    std::ifstream in(target);
    std::string s((std::istreambuf_iterator<char>(in)),
                  std::istreambuf_iterator<char>());
    return s;
  }();
  ASSERT_EQ(before, std::string("PRE-EXISTING"));

  const pid_t pid = ::fork();
  ASSERT_GE(pid, 0);
  if (pid == 0)
    {
      // Child: emulate save_checkpoint up to (but not past) the
      // rename point, then exit hard. `_Exit` skips destructors so
      // the temp file remains; the parent verifies the *target* file
      // is unchanged regardless.
      using namespace Aleph::CA::ca_checkpoint_detail;
      const auto tmp = make_tmp_path(target);
      {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        std::vector<char> dummy(4096, 'X');
        out.write(dummy.data(), dummy.size());
        out.flush();
      }
      sync_file(tmp);
      // ------- CRASH WINDOW: post-flush, pre-rename -------
      std::_Exit(0);
    }

  int status = 0;
  ::waitpid(pid, &status, 0);

  EXPECT_TRUE(std::filesystem::exists(target));
  std::ifstream in(target);
  const std::string after((std::istreambuf_iterator<char>(in)),
                          std::istreambuf_iterator<char>());
  EXPECT_EQ(after, std::string("PRE-EXISTING"))
    << "target must keep its previous contents when the writer crashes "
       "before rename";

  // The child died after writing the .tmp sibling — that file may
  // legitimately linger on disk. What must NOT happen is the partial
  // file being mistaken for `target`.
  std::filesystem::remove_all(dir);
}

TEST(CACheckpointSafety, CrashInjectionDoesNotPublishNewFile)
{
  const auto dir = tmp_dir("crash_new");
  const auto target = dir / "snap.bin";
  ASSERT_FALSE(std::filesystem::exists(target));

  const pid_t pid = ::fork();
  ASSERT_GE(pid, 0);
  if (pid == 0)
    {
      using namespace Aleph::CA::ca_checkpoint_detail;
      const auto tmp = make_tmp_path(target);
      {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        std::vector<char> dummy(2048, 'Y');
        out.write(dummy.data(), dummy.size());
        out.flush();
      }
      sync_file(tmp);
      std::_Exit(0);
    }
  int status = 0;
  ::waitpid(pid, &status, 0);

  EXPECT_FALSE(std::filesystem::exists(target))
    << "no target file must exist when the writer crashes before rename";
  std::filesystem::remove_all(dir);
}

#endif  // !_WIN32

// =====================================================================
// Async writer persists every submitted task by flush()
// =====================================================================

TEST(CACheckpointSafety, AsyncWriterPersistsAllSubmitted)
{
  const auto dir = tmp_dir("async_persist");

  Engine eng(random_grid(24, 7), make_game_of_life_rule(), Moore<2, 1>{});
  Async_Checkpoint_Writer<Engine> writer(/*capacity=*/4,
                                         Async_Checkpoint_Writer<Engine>::Queue_Policy::Block);

  constexpr std::size_t N = 6;
  std::vector<std::filesystem::path> paths;
  paths.reserve(N);
  for (std::size_t k = 0; k < N; ++k)
    {
      eng.run(1);
      const auto p = dir / ("async_" + std::to_string(k) + ".bin");
      paths.push_back(p);
      writer.submit(eng, p);
    }
  writer.flush();

  for (const auto &p : paths)
    EXPECT_TRUE(std::filesystem::exists(p)) << "missing async snapshot: " << p;
  EXPECT_EQ(writer.total_written(), N);
  EXPECT_EQ(writer.total_dropped(), 0u);

  // No leftover temp siblings.
  for (const auto &p : paths)
    EXPECT_EQ(count_tmp_siblings(p), 0u);

  std::filesystem::remove_all(dir);
}

// =====================================================================
// Drop_Oldest policy actually drops queued tasks under back-pressure
// =====================================================================

TEST(CACheckpointSafety, AsyncWriterDropOldestActuallyDrops)
{
  const auto dir = tmp_dir("async_drop");

  Engine eng(random_grid(32, 11), make_game_of_life_rule(), Moore<2, 1>{});
  Async_Checkpoint_Writer<Engine> writer(/*capacity=*/2,
                                         Async_Checkpoint_Writer<Engine>::Queue_Policy::Drop_Oldest);

  // Throttle the worker so the queue fills up reliably on any hardware.
  // Without this, fast SSDs can drain the 2-slot queue between submissions
  // and zero drops are observed (flaky in Debug builds).
  writer.set_before_write_hook([] {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  });

  constexpr std::size_t N = 32;
  for (std::size_t k = 0; k < N; ++k)
    {
      eng.run(1);
      writer.submit(eng, dir / ("snap_" + std::to_string(k) + ".bin"));
    }
  writer.flush();

  EXPECT_GT(writer.total_dropped(), 0u)
    << "Drop_Oldest must drop at least one task under sustained pressure";
  EXPECT_EQ(writer.total_written() + writer.total_dropped(), N);

  std::filesystem::remove_all(dir);
}

// =====================================================================
// Compression ratio exceeds 4× on a sparse binary 1024² grid
// =====================================================================

TEST(CACheckpointSafety, SparseCompressionRatioExceedsFourX)
{
  const auto dir = tmp_dir("compress_ratio");
  const auto raw_path = dir / "raw.bin";
  const auto cmp_path = dir / "compressed.bin";

  // 1024² sparse binary grid (~1% live cells, the rest are zeros).
  constexpr ca_size_t side = 1024;
  std::mt19937 rng(0x600D);
  std::bernoulli_distribution coin(0.01);
  Grid g({side, side}, 0);
  for (ca_size_t i = 0; i < side; ++i)
    for (ca_size_t j = 0; j < side; ++j)
      g.set({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)},
            coin(rng) ? 1 : 0);
  Engine eng(std::move(g), make_game_of_life_rule(), Moore<2, 1>{});

  save_checkpoint(eng, raw_path);
  Checkpoint_Options opts;
  opts.compress = true;
  opts.level = 6;
  save_checkpoint(eng, cmp_path, opts);

  const auto raw_size = byte_count(raw_path);
  const auto cmp_size = byte_count(cmp_path);
  ASSERT_GT(raw_size, 0u);
  ASSERT_GT(cmp_size, 0u);
  const double ratio = static_cast<double>(raw_size) / static_cast<double>(cmp_size);
  EXPECT_GT(ratio, 4.0)
    << "Sparse 1024^2 grid should compress > 4x (raw=" << raw_size
    << ", compressed=" << cmp_size << ", ratio=" << ratio << ")";

  std::filesystem::remove_all(dir);
}

// =====================================================================
// v1 file reader: hand-craft a Phase-15 file and verify inspect()
// =====================================================================

namespace
{

// Write a minimal valid v1 file with a freshly-trained type hash. The
// state payload encodes `cell_count` int32 values matching the engine.
template <typename Engine>
void write_v1_file(const std::filesystem::path &path,
                   const Engine &reference,
                   std::uint64_t step_count,
                   const std::vector<std::int32_t> &cells)
{
  using namespace Aleph::CA::ca_checkpoint_detail;
  using Lattice = typename Engine::lattice_type;
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  ASSERT_TRUE(out.good());
  out.write(magic_bytes.data(), magic_bytes.size());
  const std::uint32_t v1 = 1;
  out.write(reinterpret_cast<const char *>(&v1), sizeof(v1));
  const std::uint64_t th = type_hash<Engine>();
  out.write(reinterpret_cast<const char *>(&th), sizeof(th));
  const std::uint32_t st_size = static_cast<std::uint32_t>(sizeof(std::int32_t));
  out.write(reinterpret_cast<const char *>(&st_size), sizeof(st_size));
  const std::uint32_t rank = static_cast<std::uint32_t>(Lattice::rank);
  out.write(reinterpret_cast<const char *>(&rank), sizeof(rank));
  std::array<std::uint64_t, 4> ext{};
  const auto &e = reference.frame().extents();
  for (std::size_t d = 0; d < Lattice::rank; ++d)
    ext[d] = static_cast<std::uint64_t>(e[d]);
  for (std::size_t d = 0; d < 4; ++d)
    out.write(reinterpret_cast<const char *>(&ext[d]), sizeof(std::uint64_t));
  out.write(reinterpret_cast<const char *>(&step_count), sizeof(step_count));
  const std::uint8_t has_rng = 0;
  out.write(reinterpret_cast<const char *>(&has_rng), sizeof(has_rng));
  const char pad[7] = {0, 0, 0, 0, 0, 0, 0};
  out.write(pad, sizeof(pad));
  const std::uint64_t master_seed = 0;
  out.write(reinterpret_cast<const char *>(&master_seed), sizeof(master_seed));
  const std::uint64_t cell_count = cells.size();
  out.write(reinterpret_cast<const char *>(&cell_count), sizeof(cell_count));
  out.write(reinterpret_cast<const char *>(cells.data()),
            static_cast<std::streamsize>(cells.size() * sizeof(std::int32_t)));
  out.flush();
}

}  // namespace

TEST(CACheckpointSafety, V1ReaderAcceptsLegacyFile)
{
  const auto dir = tmp_dir("v1_legacy");
  const auto path = dir / "legacy.bin";

  Engine reference(Grid({8, 8}, 0), make_game_of_life_rule(), Moore<2, 1>{});
  std::vector<std::int32_t> cells(64);
  for (std::size_t i = 0; i < cells.size(); ++i)
    cells[i] = static_cast<std::int32_t>((i * 37) % 5);

  write_v1_file(path, reference, /*step=*/3, cells);

  const auto h = inspect_checkpoint(path);
  EXPECT_EQ(h.format_version, 1u);
  EXPECT_EQ(h.rank, 2u);
  EXPECT_EQ(h.cell_count, 64u);
  EXPECT_EQ(h.step_count, 3u);
  EXPECT_EQ(h.flags, 0u);
  EXPECT_EQ(h.payload_size, 64u * sizeof(std::int32_t));

  Engine engine(Grid({8, 8}, 0), make_game_of_life_rule(), Moore<2, 1>{});
  const Resume_Token tok = load_checkpoint_into(engine, path);
  EXPECT_EQ(tok.header.format_version, 1u);
  EXPECT_EQ(engine.steps_run(), 3u);

  // Spot-check a couple of cells were restored faithfully.
  EXPECT_EQ(engine.frame().at({0, 0}), cells[0]);
  EXPECT_EQ(engine.frame().at({1, 1}), cells[1 * 8 + 1]);

  std::filesystem::remove_all(dir);
}

// =====================================================================
// Delta round-trip: save baseline, advance, save delta, apply delta
// =====================================================================

TEST(CACheckpointSafety, DeltaRoundTripMatchesFullSnapshot)
{
  const auto dir = tmp_dir("delta");
  const auto base_path = dir / "base.bin";
  const auto delta_path = dir / "delta.bin";

  Engine eng(random_grid(20, 0xBABA), make_game_of_life_rule(), Moore<2, 1>{});
  eng.run(5);
  save_checkpoint(eng, base_path);

  // Capture the baseline payload before advancing — `save_delta`
  // diff's against this exact buffer.
  const auto baseline_bytes
    = ca_checkpoint_detail::snapshot_frame_bytes(eng);
  const auto base_step = static_cast<std::uint64_t>(eng.steps_run());

  // Advance, then store only the diff.
  eng.run(3);
  Checkpoint_Options opts;
  opts.compress = true;
  save_delta_checkpoint(eng, baseline_bytes, base_step, delta_path, opts);

  const auto dh = inspect_checkpoint(delta_path);
  EXPECT_NE(dh.flags & ca_checkpoint_flags::delta, 0u);
  EXPECT_EQ(dh.delta_base_step, base_step);
  EXPECT_EQ(dh.step_count, base_step + 3);

  // Rebuild from baseline + delta and compare to the live engine.
  Engine restored(Grid({20, 20}, 0), make_game_of_life_rule(), Moore<2, 1>{});
  load_checkpoint_into(restored, base_path);
  EXPECT_EQ(restored.steps_run(), base_step);
  apply_delta_checkpoint(restored, delta_path);
  EXPECT_EQ(restored.steps_run(), base_step + 3);
  EXPECT_TRUE(grids_equal(restored.frame(), eng.frame()));

  // load_checkpoint_into must refuse to consume a delta file.
  Engine bad(Grid({20, 20}, 0), make_game_of_life_rule(), Moore<2, 1>{});
  EXPECT_THROW(load_checkpoint_into(bad, delta_path), std::runtime_error);

  std::filesystem::remove_all(dir);
}