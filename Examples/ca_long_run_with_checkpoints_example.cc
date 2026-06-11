/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file ca_long_run_with_checkpoints_example.cc
 * @brief Phase 15 example: long-running CA with periodic checkpoints
 *        and crash-style resume.
 *
 *  The program runs Conway's Game of Life for a configurable number
 *  of steps, writing a binary checkpoint every `every` steps via
 *  `Periodic_Checkpoint_Observer`. It then simulates a crash by
 *  abandoning the engine, building a fresh one, and `load`-ing the
 *  most recent checkpoint to continue. The resumed run is compared
 *  cell-by-cell against an uninterrupted reference run to prove the
 *  bit-by-bit reproducibility of the resume protocol.
 *
 *  Run:
 *    ./ca_long_run_with_checkpoints_example [steps] [side] [every]
 *
 *  Defaults: steps=200, side=24, every=50.
 */

#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>

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

/// Strict CLI parser; rejects signed input, leading whitespace and
/// trailing garbage.
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

Grid seed_grid(const ca_size_t side)
{
  Grid g({side, side}, 0);
  std::mt19937 rng(0xC0FFEE);
  std::uniform_real_distribution<double> u(0.0, 1.0);
  for (ca_size_t i = 0; i < side; ++i)
    for (ca_size_t j = 0; j < side; ++j)
      g.set({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)},
            u(rng) < 0.35 ? 1 : 0);
  return g;
}

std::size_t alive_count(const Grid &g)
{
  std::size_t n = 0;
  for (ca_size_t i = 0; i < g.size(0); ++i)
    for (ca_size_t j = 0; j < g.size(1); ++j)
      if (g.at({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)}) != 0)
        ++n;
  return n;
}

bool grids_equal(const Grid &a, const Grid &b)
{
  for (ca_size_t i = 0; i < a.size(0); ++i)
    for (ca_size_t j = 0; j < a.size(1); ++j)
      if (a.at({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)})
          != b.at({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)}))
        return false;
  return true;
}

}  // namespace

int main(int argc, char **argv)
{
  std::size_t steps = 200;
  ca_size_t side = 24;
  std::size_t every = 50;

  // Helper: validate-and-assign with explicit overflow check against
  // the destination type's max. On 64-bit platforms `std::size_t` and
  // `std::uint64_t` coincide, but the check is essential for narrower
  // size_t targets (32-bit) and documents intent.
  auto assign_arg = [](const char *arg, const char *name, auto &dst) -> bool
  {
    using Target = std::remove_reference_t<decltype(dst)>;
    std::uint64_t v = 0;
    if (not parse_u64(arg, v))
      {
        std::cerr << "Invalid [" << name << "] (got '" << arg << "')\n";
        return false;
      }
    if (v > static_cast<std::uint64_t>(std::numeric_limits<Target>::max()))
      {
        std::cerr << "Invalid [" << name << "] out of range\n";
        return false;
      }
    dst = static_cast<Target>(v);
    return true;
  };

  if (argc >= 2 and not assign_arg(argv[1], "steps", steps))
    return 1;
  if (argc >= 3 and not assign_arg(argv[2], "side", side))
    return 1;
  if (argc >= 4 and not assign_arg(argv[3], "every", every))
    return 1;
  if (side < 4 or every == 0)
    {
      std::cerr << "Invalid arguments: side >= 4, every >= 1\n";
      return 1;
    }

  // Unique scratch directory for this run.
  const auto tick = std::chrono::steady_clock::now().time_since_epoch().count();
  const auto dir = std::filesystem::temp_directory_path()
                   / ("aleph_ca_longrun_" + std::to_string(static_cast<long long>(tick)));
  std::filesystem::create_directories(dir);

  std::cout << "Long-run example with periodic checkpoints\n";
  std::cout << "  steps=" << steps << "  side=" << side
            << "  every=" << every << " steps\n";
  std::cout << "  checkpoint dir: " << dir.string() << "\n\n";

  Grid initial = seed_grid(side);
  std::cout << "Initial alive cells: " << alive_count(initial) << "\n\n";

  // ---- Reference: uninterrupted run --------------------------------
  Engine reference(initial, make_game_of_life_rule(), Moore<2, 1>{});
  reference.run(steps);
  std::cout << "Reference run: " << reference.steps_run()
            << " steps completed, alive=" << alive_count(reference.frame()) << "\n";

  // ---- Working run with checkpoint observer ------------------------
  Engine working(initial, make_game_of_life_rule(), Moore<2, 1>{});
  Periodic_Checkpoint_Observer obs(working, every,
                                   (dir / "snap_{step}.bin").string(),
                                   /*zero_pad=*/6);
  working.on_post_step([&](std::size_t s, const Grid &f) { obs.on_step_end(s, f); });

  // Run only `steps - every` steps to leave a tail to be replayed by
  // the resume below. (If the user picks tiny values, fall back to
  // running everything.)
  const std::size_t partial = steps > every ? steps - every : steps;
  working.run(partial);
  std::cout << "Working run: " << working.steps_run() << " steps then 'crash'\n";

  // ---- Simulated crash + resume from latest snapshot --------------
  Engine resumed(Grid({side, side}, 0), make_game_of_life_rule(), Moore<2, 1>{});
  if (obs.last_path().empty())
    {
      // No periodic checkpoint was written (this happens when
      // `partial < every`, so the observer never triggered). Fall
      // back to a fresh start: the resumed engine simply begins at
      // step 0 with the same seed pattern as the reference.
      std::cout << "No checkpoint produced (partial < every); restarting fresh.\n";
      resumed = Engine(seed_grid(side), make_game_of_life_rule(), Moore<2, 1>{});
    }
  else
    {
      std::cout << "Most recent checkpoint: " << obs.last_path().string() << "\n\n";
      const Resume_Token token = load_checkpoint_into(resumed, obs.last_path());
      std::cout << "Resumed engine from step " << token.header.step_count
                << " (file=" << token.source_path.string() << ")\n";
    }
  // Continue until we reach the same total step count as the
  // reference run.
  const std::size_t remaining = steps - resumed.steps_run();
  resumed.run(remaining);
  std::cout << "Resumed run: " << resumed.steps_run()
            << " steps completed, alive=" << alive_count(resumed.frame()) << "\n";

  // ---- Verify reproducibility -------------------------------------
  if (not grids_equal(reference.frame(), resumed.frame()))
    {
      std::cerr << "\nFAIL: resumed frame differs from reference\n";
      return 1;
    }
  std::cout << "\nPASS — resumed frame is bit-by-bit identical to the "
               "uninterrupted reference run.\n";

  // Cleanup.
  std::filesystem::remove_all(dir);
  return 0;
}