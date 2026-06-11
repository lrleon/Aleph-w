/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file ca_checkpoint_phase17_example.cc
 * @brief Phase 17 showcase: compression, async writer and delta
 *        snapshots for `ca-checkpoint.H`.
 *
 *  Three independent demos (selectable via the first CLI argument):
 *
 *    - `compress` — saves the same engine state both raw and
 *                   DEFLATE-compressed; reports the on-disk size and
 *                   the compression ratio.
 *    - `async`    — measures the per-step wall time of a long run
 *                   with and without an `Async_Checkpoint_Writer`
 *                   plumbed into a `Periodic_Checkpoint_Observer`,
 *                   and prints the relative overhead.
 *    - `delta`    — captures a baseline snapshot, advances the engine,
 *                   stores a `save_delta_checkpoint`, reloads the
 *                   baseline into a fresh engine and applies the
 *                   delta on top. Verifies cell-by-cell that the
 *                   reconstructed state matches the live engine.
 *    - `all`      — runs all three back-to-back (default).
 *
 *  Run:
 *    ./ca_checkpoint_phase17_example [mode] [side] [steps]
 *
 *  Defaults: mode=all, side=128, steps=200.
 */

#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <vector>

#include <ca-checkpoint.H>
#include <ca-traits.H>
#include <tpl_ca_engine.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_rule.H>
#include <tpl_ca_storage.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace
{

using Grid = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;
using Engine = Synchronous_Engine<Grid, Game_Of_Life_Rule, Moore<2, 1>>;

bool parse_u64(const char *text, std::uint64_t &out) noexcept
{
  if (text == nullptr)
    return false;
  std::string_view sv{text};
  if (sv.empty() or sv.front() == '+' or sv.front() == '-')
    return false;
  std::uint64_t v = 0;
  const auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), v, 10);
  if (ec != std::errc{} or ptr != sv.data() + sv.size())
    return false;
  out = v;
  return true;
}

Grid seed_grid(const ca_size_t side, const std::uint64_t seed, const double p_alive)
{
  Grid g({side, side}, 0);
  std::mt19937_64 rng(seed);
  std::uniform_real_distribution<double> u(0.0, 1.0);
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

std::filesystem::path make_scratch_dir(const std::string &tag)
{
  const auto tick = std::chrono::steady_clock::now().time_since_epoch().count();
  auto p = std::filesystem::temp_directory_path()
           / ("aleph_ca_ckpt_p17_" + tag + "_"
              + std::to_string(static_cast<long long>(tick)));
  std::filesystem::create_directories(p);
  return p;
}

void print_section(const std::string &title)
{
  std::cout << "\n=== " << title << " ===\n";
}

// ---------------------------------------------------------------------
// Demo 1 — DEFLATE compression ratio
// ---------------------------------------------------------------------

int demo_compress(const ca_size_t side)
{
  print_section("compress");
  const auto dir = make_scratch_dir("compress");

  // Sparse Game of Life initial state — DEFLATE catches the long runs
  // of zero bytes and shrinks the file substantially.
  Engine eng(seed_grid(side, /*seed=*/0xC0FFEE, /*p_alive=*/0.05),
             make_game_of_life_rule(),
             Moore<2, 1>{});

  const auto raw_path = dir / "snap.raw.bin";
  const auto cmp_path = dir / "snap.cmp.bin";

  save_checkpoint(eng, raw_path);

  Checkpoint_Options opts;
  opts.compress = true;
  opts.level = 6;
  save_checkpoint(eng, cmp_path, opts);

  const auto raw_size = byte_count(raw_path);
  const auto cmp_size = byte_count(cmp_path);
  const double ratio
    = (cmp_size > 0) ? static_cast<double>(raw_size) / static_cast<double>(cmp_size)
                     : 0.0;

  std::cout << std::fixed << std::setprecision(2);
  std::cout << "  side             : " << side << " x " << side
            << " (sparse, ~5% alive)\n";
  std::cout << "  raw file         : " << raw_size << " bytes  ("
            << raw_path.filename().string() << ")\n";
  std::cout << "  compressed file  : " << cmp_size << " bytes  ("
            << cmp_path.filename().string() << ")\n";
  std::cout << "  compression ratio: " << ratio << "x\n";

  // Verify round-trip equivalence: both files restore identically.
  Engine restored_raw(Grid({side, side}, 0), make_game_of_life_rule(), Moore<2, 1>{});
  Engine restored_cmp(Grid({side, side}, 0), make_game_of_life_rule(), Moore<2, 1>{});
  (void) load_checkpoint_into(restored_raw, raw_path);
  (void) load_checkpoint_into(restored_cmp, cmp_path);

  if (not grids_equal(restored_raw.frame(), restored_cmp.frame()))
    {
      std::cerr << "  FAIL: raw and compressed restores differ\n";
      std::filesystem::remove_all(dir);
      return 1;
    }
  std::cout << "  round-trip       : PASS (raw == compressed)\n";

  std::filesystem::remove_all(dir);
  return 0;
}

// ---------------------------------------------------------------------
// Demo 2 — Async writer overhead measurement
// ---------------------------------------------------------------------

double run_steps(Engine &eng, const std::size_t steps)
{
  const auto t0 = std::chrono::steady_clock::now();
  eng.run(steps);
  const auto t1 = std::chrono::steady_clock::now();
  return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

int demo_async(const ca_size_t side, const std::size_t steps)
{
  print_section("async");
  const auto dir = make_scratch_dir("async");

  // Configure a moderate snapshot cadence (every 16 steps) and a
  // queue capacity that comfortably absorbs the bursts.
  constexpr std::size_t every = 16;
  constexpr std::size_t queue_capacity = 8;

  // ----- Baseline run (no observer) -----
  Engine baseline(seed_grid(side, 0x9ABC, 0.35),
                  make_game_of_life_rule(),
                  Moore<2, 1>{});
  const double t_baseline = run_steps(baseline, steps);

  // ----- Sync observer (writes block the step loop) -----
  Engine sync_eng(seed_grid(side, 0x9ABC, 0.35),
                  make_game_of_life_rule(),
                  Moore<2, 1>{});
  Periodic_Checkpoint_Observer sync_obs(sync_eng,
                                        every,
                                        (dir / "sync_{step}.bin").string());
  sync_eng.on_post_step([&](std::size_t s, const Grid &f) { sync_obs.on_step_end(s, f); });
  const double t_sync = run_steps(sync_eng, steps);

  // ----- Async observer (writes go to a worker thread) -----
  Engine async_eng(seed_grid(side, 0x9ABC, 0.35),
                   make_game_of_life_rule(),
                   Moore<2, 1>{});
  Async_Checkpoint_Writer<Engine> writer(queue_capacity,
                                         Async_Checkpoint_Writer<Engine>::Queue_Policy::Block);
  Periodic_Checkpoint_Observer async_obs(async_eng,
                                         every,
                                         (dir / "async_{step}.bin").string(),
                                         writer);
  async_eng.on_post_step([&](std::size_t s, const Grid &f) { async_obs.on_step_end(s, f); });
  const double t_async = run_steps(async_eng, steps);
  writer.flush();

  // ----- Report -----
  auto pct = [&](double t)
  { return (t_baseline > 0.0) ? 100.0 * (t - t_baseline) / t_baseline : 0.0; };

  std::cout << std::fixed << std::setprecision(2);
  std::cout << "  side                : " << side << " x " << side << "\n";
  std::cout << "  steps               : " << steps << "\n";
  std::cout << "  snapshot every      : " << every << " steps\n";
  std::cout << "  queue capacity      : " << queue_capacity << " (Block policy)\n";
  std::cout << "  baseline (no obs.)  : " << t_baseline << " ms\n";
  std::cout << "  sync observer       : " << t_sync  << " ms  ("
            << std::showpos << pct(t_sync)  << std::noshowpos << "% vs baseline)\n";
  std::cout << "  async observer      : " << t_async << " ms  ("
            << std::showpos << pct(t_async) << std::noshowpos << "% vs baseline)\n";
  std::cout << "  async total written : " << writer.total_written() << "\n";
  std::cout << "  async total dropped : " << writer.total_dropped() << "\n";

  if (not grids_equal(baseline.frame(), sync_eng.frame())
      or not grids_equal(baseline.frame(), async_eng.frame()))
    {
      std::cerr << "  FAIL: observers altered the engine trajectory\n";
      std::filesystem::remove_all(dir);
      return 1;
    }
  std::cout << "  trajectory check    : PASS (all three engines agree)\n";

  std::filesystem::remove_all(dir);
  return 0;
}

// ---------------------------------------------------------------------
// Demo 3 — Delta snapshot round-trip
// ---------------------------------------------------------------------

int demo_delta(const ca_size_t side, const std::size_t steps)
{
  print_section("delta");
  const auto dir = make_scratch_dir("delta");
  const auto base_path  = dir / "baseline.bin";
  const auto delta_path = dir / "delta.bin";

  // Run the engine up to a baseline step, snapshot it (raw), capture
  // its byte payload, then advance further and store the diff.
  Engine eng(seed_grid(side, 0xDA17A, 0.4),
             make_game_of_life_rule(),
             Moore<2, 1>{});
  const std::size_t baseline_steps = steps / 2;
  const std::size_t delta_steps    = steps - baseline_steps;

  eng.run(baseline_steps);
  save_checkpoint(eng, base_path);
  const auto baseline_bytes = ca_checkpoint_detail::snapshot_frame_bytes(eng);
  const auto base_step      = static_cast<std::uint64_t>(eng.steps_run());

  eng.run(delta_steps);
  Checkpoint_Options opts;
  opts.compress = true;
  save_delta_checkpoint(eng, baseline_bytes, base_step, delta_path, opts);

  const auto base_size  = byte_count(base_path);
  const auto delta_size = byte_count(delta_path);
  const auto dh = inspect_checkpoint(delta_path);

  std::cout << std::fixed << std::setprecision(2);
  std::cout << "  side                : " << side << " x " << side << "\n";
  std::cout << "  baseline step       : " << base_step << "\n";
  std::cout << "  delta step          : " << dh.step_count
            << "  (base_step + " << delta_steps << ")\n";
  std::cout << "  baseline file       : " << base_size  << " bytes\n";
  std::cout << "  delta file          : " << delta_size << " bytes  ("
            << (base_size > 0
                  ? 100.0 * static_cast<double>(delta_size) / static_cast<double>(base_size)
                  : 0.0)
            << "% of baseline)\n";

  // Rebuild from baseline + delta and compare to the live engine.
  Engine reconstructed(Grid({side, side}, 0),
                       make_game_of_life_rule(),
                       Moore<2, 1>{});
  (void) load_checkpoint_into(reconstructed, base_path);
  (void) apply_delta_checkpoint(reconstructed, delta_path);

  if (reconstructed.steps_run() != eng.steps_run())
    {
      std::cerr << "  FAIL: reconstructed step count "
                << reconstructed.steps_run() << " != live " << eng.steps_run() << "\n";
      std::filesystem::remove_all(dir);
      return 1;
    }
  if (not grids_equal(reconstructed.frame(), eng.frame()))
    {
      std::cerr << "  FAIL: reconstructed frame differs from live engine\n";
      std::filesystem::remove_all(dir);
      return 1;
    }
  std::cout << "  reconstruction      : PASS (baseline + delta == live)\n";

  std::filesystem::remove_all(dir);
  return 0;
}

}  // namespace

int main(int argc, char **argv)
{
  std::string mode  = "all";
  ca_size_t   side  = 128;
  std::size_t steps = 200;

  if (argc >= 2)
    mode = argv[1];
  if (argc >= 3)
    {
      std::uint64_t v = 0;
      if (not parse_u64(argv[2], v)
          or v > static_cast<std::uint64_t>(std::numeric_limits<ca_size_t>::max()))
        {
          std::cerr << "Invalid [side] (got '" << argv[2] << "')\n";
          return 1;
        }
      side = static_cast<ca_size_t>(v);
    }
  if (argc >= 4)
    {
      std::uint64_t v = 0;
      if (not parse_u64(argv[3], v)
          or v > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
        {
          std::cerr << "Invalid [steps] (got '" << argv[3] << "')\n";
          return 1;
        }
      steps = static_cast<std::size_t>(v);
    }
  if (side < 8 or steps < 4)
    {
      std::cerr << "Invalid arguments: side >= 8, steps >= 4\n";
      return 1;
    }

  std::cout << "ca_checkpoint_phase17_example\n";
  std::cout << "  mode  = " << mode  << "\n";
  std::cout << "  side  = " << side  << "\n";
  std::cout << "  steps = " << steps << "\n";

  int rc = 0;
  if (mode == "compress" or mode == "all")
    rc |= demo_compress(side);
  if (mode == "async" or mode == "all")
    rc |= demo_async(side, steps);
  if (mode == "delta" or mode == "all")
    rc |= demo_delta(side, steps);

  if (mode != "compress" and mode != "async" and mode != "delta" and mode != "all")
    {
      std::cerr << "\nUnknown mode '" << mode
                << "' (expected: compress | async | delta | all)\n";
      return 1;
    }

  std::cout << "\n" << (rc == 0 ? "Overall: PASS" : "Overall: FAIL") << "\n";
  return rc;
}