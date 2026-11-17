
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

/** @file moved_from_engines_test.cc
 *  @brief The moved-from tests (moved_from.H) of the cellular automata
 *         storages, lattices and engines, of the lock guards of
 *         concurrency_utils.H and of the CSV reader.
 */

#include "moved_from.H"

#include <ca-observer.H>
#include <ca-streaming-storage.H>
#include <concurrency_utils.H>
#include <parse-csv.H>
#include <thread_pool.H>
#include <tpl_ca_async_engine.H>
#include <tpl_ca_bit_storage.H>
#include <tpl_ca_engine.H>
#include <tpl_ca_ghost_lattice.H>
#include <tpl_ca_graph_automaton.H>
#include <tpl_ca_hashlife.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_multi_field_engine.H>
#include <tpl_ca_multi_field_lattice.H>
#include <tpl_ca_multi_field_rule.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_parallel_engine.H>
#include <tpl_ca_rule.H>
#include <tpl_ca_storage.H>

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>

#if defined(_WIN32)
#  include <process.h>
#else
#  include <unistd.h>
#endif

using namespace Aleph;
using namespace Aleph::CA;

namespace
{
  using Extents = std::array<ca_size_t, 2>;

  // About n cells: one row for the smallest objects, ten rows otherwise
  Extents extents_of(const size_t n)
  {
    const ca_size_t rows = n < 10 ? 1 : 10;
    return {rows, n / rows};
  }

  // Calls f(c) with every coordinate of a lattice of extents e
  template <typename F>
  void for_each_cell(const Extents & e, F && f)
  {
    for (ca_size_t i = 0; i < e[0]; ++i)
      for (ca_size_t j = 0; j < e[1]; ++j)
        f(Coord_Vec<2>{static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)});
  }

  using Grid = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;
  using Ghost_Grid = Ghost_Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;
  using Field_Grid = Multi_Field_Lattice<Layout_SoA, OpenBoundary, 2, int, int>;
  using Two_Fields = Multi_Field_Lattice<Layout_AoS, OpenBoundary, 2, int, int>;

  using Life = Synchronous_Engine<Grid, Game_Of_Life_Rule, Moore<2, 1>>;
  using Async_Life = Async_Engine<Grid, Game_Of_Life_Rule, Moore<2, 1>>;
  using Parallel_Life = Parallel_Synchronous_Engine<Grid, Game_Of_Life_Rule, Moore<2, 1>>;
  using Field_Life = Multi_Field_Engine<Two_Fields, Field_Slice_Rule<0, Game_Of_Life_Rule>,
                                        Moore<2, 1>>;
  using Graph_Life = Graph_Synchronous_Engine<Graph_Lattice<int>, Game_Of_Life_Rule>;

  using Random_Life
    = Async_Engine<Grid, Game_Of_Life_Rule, Moore<2, 1>, Random_Asynchronous_Update<>>;

  using Tiles = Tile_Cache<int, 4>;
  using Density = Density_Observer<int>;
  using Entropy = Entropy_Observer<2>;

  using Locked = Synchronized<long>::LockedPtr;
  using Const_Locked = Synchronized<long>::ConstLockedPtr;
  using Read_Locked = RwSynchronized<long>::ReadLockedPtr;
  using Write_Locked = RwSynchronized<long>::WriteLockedPtr;

  // A lattice of about n cells, with a blinker where it fits
  Grid life_grid(const size_t n)
  {
    Grid g(extents_of(n), 0);
    if (g.size(1) >= 3)
      for (ca_index_t j = 0; j < 3; ++j)
        g.set({0, j}, 1);
    return g;
  }

  // What a user does with a rectangular engine: step it, read its frame and
  // run it
  template <class Engine>
  void use_engine(Engine & e)
  {
    const size_t steps = e.steps_run();
    e.step();
    EXPECT_EQ(e.steps_run(), steps + 1);
    size_t alive = 0;
    for_each_cell(e.frame().extents(), [&e, &alive](const Coord_Vec<2> & c)
      {
        if (e.frame().at(c) != 0)
          ++alive;
      });
    EXPECT_LE(alive, e.frame().size());
    e.run(2);
    EXPECT_EQ(e.steps_run(), steps + 3);
  }

  // The cells, the live cells and the steps of a rectangular engine
  template <class Engine>
  std::tuple<size_t, size_t, size_t> engine_print(const Engine & e)
  {
    size_t alive = 0;
    for_each_cell(e.frame().extents(), [&e, &alive](const Coord_Vec<2> & c)
      {
        if (e.frame().at(c) != 0)
          ++alive;
      });
    return {e.frame().size(), alive, e.steps_run()};
  }

  // Runs an engine for some steps from step 0 with the observer attached,
  // as a user does
  template <class Observer>
  void observe_run(Observer & o, const size_t steps)
  {
    Life e(life_grid(100), make_game_of_life_rule(), Moore<2, 1>{});
    attach_observer(e, o);
    e.run(steps);
  }

  // Extents of about n cells, in whole tiles
  Extents tile_extents_of(const size_t n)
  {
    ca_size_t side = Tiles::tile_side;
    while (side * side < n)
      side += Tiles::tile_side;
    return {side, side};
  }

  // Calls f(r, c) with every cell of a cache of extents e, tile by tile
  template <typename F>
  void for_each_tiled_cell(const Extents & e, F && f)
  {
    constexpr ca_size_t side = Tiles::tile_side;
    for (ca_size_t r0 = 0; r0 < e[0]; r0 += side)
      for (ca_size_t c0 = 0; c0 < e[1]; c0 += side)
        for (ca_size_t r = r0; r < r0 + side; ++r)
          for (ca_size_t c = c0; c < c0 + side; ++c)
            f(r, c);
  }

  ThreadPool & small_pool()
  {
    static ThreadPool pool(2);
    return pool;
  }

  // A new value, with its own mutex, for each guard that a test makes: a
  // test can hold two guards, and since it takes them in the order of the
  // values, ThreadSanitizer sees no inversion of the order of the locks
  Synchronized<long> & sync_slot()
  {
    static Synchronized<long> slots[32];
    static size_t next = 0;
    return slots[next++ % 32];
  }

  RwSynchronized<long> & rw_slot()
  {
    static RwSynchronized<long> slots[32];
    static size_t next = 0;
    return slots[next++ % 32];
  }

  // Each test runs in its own process, maybe beside others: the names of
  // the files carry the process
  long long process_id() noexcept
  {
#if defined(_WIN32)
    return static_cast<long long>(_getpid());
#else
    return static_cast<long long>(getpid());
#endif
  }

  // The files and directories that the tests write, all of them in the
  // temporary directory; they are removed when the program ends, once
  // every object has closed them
  struct Temp_Paths
  {
    Array<std::filesystem::path> paths;

    ~Temp_Paths()
    {
      for (size_t i = 0; i < paths.size(); ++i)
        {
          std::error_code ec;
          std::filesystem::remove_all(paths(i), ec);
        }
    }
  };

  // A new path in the temporary directory, unique to the process and to
  // the call
  std::filesystem::path temp_path(const std::string & kind)
  {
    static Temp_Paths temps;
    const std::string name = "aleph_moved_from_" + kind + "_" + std::to_string(process_id())
                             + "_" + std::to_string(temps.paths.size());
    const std::filesystem::path path = std::filesystem::temp_directory_path() / name;
    temps.paths.append(path);
    return path;
  }

  // A file whose header gives its number of rows, followed by n rows of
  // three fields
  std::string write_csv(const size_t n)
  {
    const std::string path = temp_path("csv").string();
    std::ofstream out(path);
    out << "rows," << n << "\n";
    for (size_t i = 0; i < n; ++i)
      out << i << ",b" << i << ",c" << i << "\n";
    return path;
  }

  // A rule that, like a lambda, can be moved but not assigned
  struct Fixed_Rule
  {
    Fixed_Rule() = default;
    Fixed_Rule(const Fixed_Rule &) = default;
    Fixed_Rule(Fixed_Rule &&) = default;
    Fixed_Rule & operator = (const Fixed_Rule &) = delete;

    template <typename State>
    State operator()(const State & s, Neighbor_View<State>) const { return s; }
  };
}

// The moves of the storages, the lattices and the guards do not throw, and
// those of the engines do not throw when the moves of their parts do not
static_assert(std::is_nothrow_move_constructible_v<Dense_Cell_Storage<int, 2>>);
static_assert(std::is_nothrow_move_assignable_v<Dense_Cell_Storage<int, 2>>);
static_assert(std::is_nothrow_move_constructible_v<Bit_Cell_Storage<2>>);
static_assert(std::is_nothrow_move_assignable_v<Ghost_Grid>);
static_assert(std::is_nothrow_move_assignable_v<Field_Grid>);
static_assert(std::is_nothrow_move_constructible_v<Graph_Lattice<int>>);
static_assert(std::is_nothrow_move_constructible_v<Life>);
static_assert(std::is_nothrow_move_assignable_v<Life>);
static_assert(std::is_nothrow_move_assignable_v<Async_Life>);
static_assert(std::is_nothrow_move_assignable_v<Parallel_Life>);
static_assert(std::is_nothrow_move_assignable_v<Field_Life>);
static_assert(std::is_nothrow_move_assignable_v<Graph_Life>);
static_assert(std::is_nothrow_move_assignable_v<Random_Life>);
static_assert(std::is_nothrow_move_assignable_v<Hashlife_Engine>);
static_assert(std::is_nothrow_move_assignable_v<Tiles>);
static_assert(std::is_nothrow_move_constructible_v<Density>);
static_assert(std::is_nothrow_move_assignable_v<Entropy>);
static_assert(std::is_nothrow_move_assignable_v<Locked>);
static_assert(std::is_nothrow_move_constructible_v<Read_Locked>);

// The copies are kept, except those of Hashlife_Engine and Tile_Cache, which
// shared the nodes and the tiles of the original
static_assert(std::is_copy_constructible_v<Dense_Cell_Storage<int, 2>>);
static_assert(std::is_copy_assignable_v<Life>);
static_assert(std::is_copy_constructible_v<Density>);
static_assert(not std::is_copy_constructible_v<Hashlife_Engine>);
static_assert(not std::is_copy_constructible_v<Tiles>);

// An engine whose rule cannot be assigned is moved, but not assigned, as
// with the implicit moves
static_assert(std::is_move_constructible_v<Synchronous_Engine<Grid, Fixed_Rule, Moore<2, 1>>>);
static_assert(not std::is_move_assignable_v<Synchronous_Engine<Grid, Fixed_Rule, Moore<2, 1>>>);

template <> struct Moved_From_Case<Dense_Cell_Storage<int, 2>>
{
  static Dense_Cell_Storage<int, 2> make(const size_t n)
  {
    return Dense_Cell_Storage<int, 2>(extents_of(n), 1);
  }
  static bool is_empty(const Dense_Cell_Storage<int, 2> & s)
  {
    return s.size() == 0 and s.size(0) == 0 and s.data() == nullptr;
  }
  static std::tuple<ca_size_t, ca_size_t, long> fingerprint(const Dense_Cell_Storage<int, 2> & s)
  {
    long sum = 0;
    for_each_cell(s.extents(), [&s, &sum](const Coord_Vec<2> & c) { sum += s.at(c); });
    return {s.size(0), s.size(1), sum};
  }
  static void use(Dense_Cell_Storage<int, 2> & s)
  {
    for_each_cell(s.extents(), [&s](const Coord_Vec<2> & c) { s.set(c, s.at(c) + 1); });
    s.fill(7);
    for_each_cell(s.extents(), [&s](const Coord_Vec<2> & c) { EXPECT_EQ(s.at(c), 7); });
    EXPECT_EQ(s.view().extent(0), s.size(0));
  }
};

template <> struct Moved_From_Case<Bit_Cell_Storage<2>>
{
  static Bit_Cell_Storage<2> make(const size_t n)
  {
    return Bit_Cell_Storage<2>(extents_of(n), true);
  }
  static bool is_empty(const Bit_Cell_Storage<2> & s) { return s.size() == 0 and s.size(0) == 0; }
  static std::tuple<ca_size_t, ca_size_t, size_t> fingerprint(const Bit_Cell_Storage<2> & s)
  {
    size_t set = 0;
    for_each_cell(s.extents(), [&s, &set](const Coord_Vec<2> & c)
      {
        if (s.at(c))
          ++set;
      });
    return {s.size(0), s.size(1), set};
  }
  static void use(Bit_Cell_Storage<2> & s)
  {
    for_each_cell(s.extents(), [&s](const Coord_Vec<2> & c) { s.set(c, not s.at(c)); });
    s.fill(true);
    for_each_cell(s.extents(), [&s](const Coord_Vec<2> & c) { EXPECT_TRUE(s.at(c)); });
  }
};

template <> struct Moved_From_Case<Ghost_Grid>
{
  static Ghost_Grid make(const size_t n)
  {
    Ghost_Grid g(extents_of(n), 0);
    g.set({0, 0}, 1);
    g.refresh_halo();
    return g;
  }
  static bool is_empty(const Ghost_Grid & g)
  {
    return g.size() == 0 and g.size(0) == 0 and g.store_extents()[0] == 0;
  }
  static std::tuple<ca_size_t, ca_size_t, long> fingerprint(const Ghost_Grid & g)
  {
    long sum = 0;
    for_each_cell(g.extents(), [&g, &sum](const Coord_Vec<2> & c) { sum += g.at(c); });
    return {g.size(0), g.size(1), sum};
  }
  static void use(Ghost_Grid & g)
  {
    for_each_cell(g.extents(), [&g](const Coord_Vec<2> & c) { g.set(c, g.at(c) + 1); });
    g.refresh_halo();
    for_each_cell(g.extents(), [&g](const Coord_Vec<2> & c) { EXPECT_EQ(g.at_safe(c), g.at(c)); });
    (void) g.at_safe({-1, -1});
    g.fill(3);
  }
};

template <> struct Moved_From_Case<Field_Grid>
{
  static Field_Grid make(const size_t n) { return Field_Grid(extents_of(n), std::tuple{1, 2}); }
  static bool is_empty(const Field_Grid & m)
  {
    return m.size() == 0 and m.size(0) == 0 and m.field<0>().size() == 0;
  }
  static std::tuple<ca_size_t, ca_size_t, long, long> fingerprint(const Field_Grid & m)
  {
    long first = 0;
    long second = 0;
    for_each_cell(m.extents(), [&m, &first, &second](const Coord_Vec<2> & c)
      {
        first += m.at<0>(c);
        second += m.at<1>(c);
      });
    return {m.size(0), m.size(1), first, second};
  }
  static void use(Field_Grid & m)
  {
    for_each_cell(m.extents(), [&m](const Coord_Vec<2> & c)
      {
        m.set<0>(c, m.at<0>(c) + 1);
        m.set_cell(c, m.at_cell(c));
      });
    m.fill(std::tuple{3, 4});
    for_each_cell(m.extents(), [&m](const Coord_Vec<2> & c) { EXPECT_EQ(m.at<1>(c), 4); });
  }
};

template <> struct Moved_From_Case<Graph_Lattice<int>>
{
  static Graph_Lattice<int> make(const size_t n)
  {
    return Graph_Lattice<int>(make_path_graph_adjacency(n, true), 1);
  }
  static bool is_empty(const Graph_Lattice<int> & g)
  {
    return g.size() == 0 and g.max_degree() == 0;
  }
  static std::tuple<size_t, long, size_t> fingerprint(const Graph_Lattice<int> & g)
  {
    long sum = 0;
    size_t degrees = 0;
    for (size_t i = 0; i < g.size(); ++i)
      {
        sum += g.at_node(i);
        degrees += g.degree(i);
      }
    return {g.size(), sum, degrees};
  }
  static void use(Graph_Lattice<int> & g)
  {
    for (size_t i = 0; i < g.size(); ++i)
      {
        g.set_node(i, g.at_node(i) + 1);
        for (size_t k = 0; k < g.degree(i); ++k)
          (void) g.at_node(g.neighbours(i)[k]);
        const Coord_Vec<1> c{static_cast<ca_index_t>(i)};
        g.set(c, g.at(c));
      }
    g.fill(2);
    (void) g.max_degree();
  }
};

template <> struct Moved_From_Case<Life>
{
  static Life make(const size_t n)
  {
    Life e(life_grid(n), make_game_of_life_rule(), Moore<2, 1>{});
    e.step();
    return e;
  }
  static bool is_empty(const Life & e) { return e.frame().size() == 0 and e.steps_run() == 0; }
  static auto fingerprint(const Life & e) { return engine_print(e); }
  static void use(Life & e) { use_engine(e); }
};

template <> struct Moved_From_Case<Async_Life>
{
  static Async_Life make(const size_t n)
  {
    Async_Life e(life_grid(n), make_game_of_life_rule(), Moore<2, 1>{});
    e.step();
    return e;
  }
  static bool is_empty(const Async_Life & e)
  {
    return e.frame().size() == 0 and e.steps_run() == 0 and e.step_count() == 0;
  }
  static auto fingerprint(const Async_Life & e) { return engine_print(e); }
  static void use(Async_Life & e) { use_engine(e); }
};

template <> struct Moved_From_Case<Parallel_Life>
{
  static Parallel_Life make(const size_t n)
  {
    Parallel_Engine_Config cfg;
    cfg.pool = &small_pool();
    cfg.num_partitions = 2;
    cfg.min_parallel_cells = 0;
    Parallel_Life e(life_grid(n), make_game_of_life_rule(), Moore<2, 1>{}, cfg);
    e.step();
    return e;
  }
  static bool is_empty(const Parallel_Life & e)
  {
    return e.frame().size() == 0 and e.steps_run() == 0 and e.config().pool == &small_pool();
  }
  static auto fingerprint(const Parallel_Life & e) { return engine_print(e); }
  static void use(Parallel_Life & e) { use_engine(e); }
};

template <> struct Moved_From_Case<Field_Life>
{
  static Field_Life make(const size_t n)
  {
    Two_Fields lat(extents_of(n), std::tuple{0, 7});
    if (lat.size(1) >= 3)
      for (ca_index_t j = 0; j < 3; ++j)
        lat.set<0>({0, j}, 1);
    Field_Life e(std::move(lat), Field_Slice_Rule<0, Game_Of_Life_Rule>(make_game_of_life_rule()),
                 Moore<2, 1>{});
    e.step();
    return e;
  }
  static bool is_empty(const Field_Life & e)
  {
    return e.frame().size() == 0 and e.steps_run() == 0;
  }
  static std::tuple<size_t, size_t, size_t> fingerprint(const Field_Life & e)
  {
    size_t alive = 0;
    for_each_cell(e.frame().extents(), [&e, &alive](const Coord_Vec<2> & c)
      {
        if (e.frame().at<0>(c) != 0)
          ++alive;
      });
    return {e.frame().size(), alive, e.steps_run()};
  }
  static void use(Field_Life & e)
  {
    const size_t steps = e.steps_run();
    e.step();
    EXPECT_EQ(e.steps_run(), steps + 1);
    for_each_cell(e.frame().extents(), [&e](const Coord_Vec<2> & c)
      {
        EXPECT_EQ(e.frame().at<1>(c), 7);
      });
    e.run(2);
    EXPECT_EQ(e.steps_run(), steps + 3);
  }
};

template <> struct Moved_From_Case<Graph_Life>
{
  static Graph_Life make(const size_t n)
  {
    Graph_Lattice<int> g(make_path_graph_adjacency(n, true), 0);
    for (size_t i = 0; i < n; i += 3)
      g.set_node(i, 1);
    Graph_Life e(std::move(g), make_game_of_life_rule());
    e.step();
    return e;
  }
  static bool is_empty(const Graph_Life & e)
  {
    return e.frame().size() == 0 and e.steps_run() == 0;
  }
  static std::tuple<size_t, size_t, size_t> fingerprint(const Graph_Life & e)
  {
    size_t alive = 0;
    for (size_t i = 0; i < e.frame().size(); ++i)
      if (e.frame().at_node(i) != 0)
        ++alive;
    return {e.frame().size(), alive, e.steps_run()};
  }
  static void use(Graph_Life & e)
  {
    const size_t steps = e.steps_run();
    e.step();
    EXPECT_EQ(e.steps_run(), steps + 1);
    for (size_t i = 0; i < e.frame().size(); ++i)
      EXPECT_LE(e.frame().at_node(i), 1);
    e.run(2);
    EXPECT_EQ(e.steps_run(), steps + 3);
  }
};

template <> struct Moved_From_Case<Hashlife_Engine>
{
  static Hashlife_Engine make(const size_t n)
  {
    Hashlife_Engine h;
    for (size_t i = 0; i < n; ++i)
      h.set_alive(static_cast<std::int64_t>(i % 40), static_cast<std::int64_t>(i / 40));
    return h;
  }
  static bool is_empty(const Hashlife_Engine & h)
  {
    return h.population() == 0 and h.generation() == 0 and h.bbox().is_empty();
  }
  static std::pair<std::uint64_t, std::int64_t> fingerprint(const Hashlife_Engine & h)
  {
    return {h.population(), h.generation()};
  }
  static void use(Hashlife_Engine & h)
  {
    h.set_alive(-5, 7);
    EXPECT_TRUE(h.alive_at(-5, 7));
    std::uint64_t seen = 0;
    h.for_each_alive([&seen](std::int64_t, std::int64_t) { ++seen; });
    EXPECT_EQ(seen, h.population());
    EXPECT_FALSE(h.save_rle_string().empty());
    (void) h.run(1);
    h.clear();
    EXPECT_EQ(h.population(), 0u);
  }
};

template <> struct Moved_From_Case<Locked>
{
  static Locked make(const size_t n)
  {
    Locked g = sync_slot().lock();
    *g = static_cast<long>(n);
    return g;
  }
  static bool is_empty(Locked & g) { return g.operator->() == nullptr; }
  // The value that the guard gives; its operator-> is not const
  static const long * fingerprint(const Locked & g)
  {
    return const_cast<Locked &>(g).operator->();
  }
  static void use(Locked & g)
  {
    if (long * v = g.operator->())
      {
        ++*v;
        EXPECT_EQ(g.get(), *v);
      }
  }
};

template <> struct Moved_From_Case<Const_Locked>
{
  static Const_Locked make(const size_t n)
  {
    Synchronized<long> & s = sync_slot();
    s.with_lock([n](long & v) { v = static_cast<long>(n); });
    return std::as_const(s).lock();
  }
  static bool is_empty(const Const_Locked & g) { return g.operator->() == nullptr; }
  static const long * fingerprint(const Const_Locked & g) { return g.operator->(); }
  static void use(Const_Locked & g)
  {
    if (const long * v = g.operator->())
      {
        EXPECT_EQ(*g, *v);
      }
  }
};

template <> struct Moved_From_Case<Read_Locked>
{
  static Read_Locked make(const size_t n)
  {
    RwSynchronized<long> & s = rw_slot();
    s.with_write_lock([n](long & v) { v = static_cast<long>(n); });
    return s.read();
  }
  static bool is_empty(const Read_Locked & g) { return g.operator->() == nullptr; }
  static const long * fingerprint(const Read_Locked & g) { return g.operator->(); }
  static void use(Read_Locked & g)
  {
    if (const long * v = g.operator->())
      {
        EXPECT_EQ(g.get(), *v);
      }
  }
};

template <> struct Moved_From_Case<Write_Locked>
{
  static Write_Locked make(const size_t n)
  {
    Write_Locked g = rw_slot().write();
    *g = static_cast<long>(n);
    return g;
  }
  static bool is_empty(Write_Locked & g) { return g.operator->() == nullptr; }
  // The value that the guard gives; its operator-> is not const
  static const long * fingerprint(const Write_Locked & g)
  {
    return const_cast<Write_Locked &>(g).operator->();
  }
  static void use(Write_Locked & g)
  {
    if (long * v = g.operator->())
      {
        *v += 2;
        EXPECT_EQ(*g, *v);
      }
  }
};

template <> struct Moved_From_Case<CsvReader>
{
  static CsvReader make(const size_t n)
  {
    CsvReader r(write_csv(n));
    r.read_header();
    return r;
  }
  static bool is_empty(const CsvReader & r)
  {
    return not r.has_next() and r.rows_read() == 0 and not r.has_header()
           and r.header().size() == 0;
  }
  // The header of a reader of make() names its file; the rows read say
  // where the reader is
  static std::tuple<bool, std::string, size_t> fingerprint(const CsvReader & r)
  {
    return {r.has_header(), r.header().size() > 1 ? r.header()[1] : std::string(),
            r.rows_read()};
  }
  static void use(CsvReader & r)
  {
    // The header of a reader of make() gives its rows; an empty one has none
    const size_t expected = r.has_header() ? std::stoul(r.header()[1]) : 0;
    r.reset();
    size_t rows = 0;
    while (r.has_next())
      {
        EXPECT_EQ(r.next().size(), 3u);
        ++rows;
      }
    EXPECT_EQ(rows, expected);
    EXPECT_EQ(r.rows_read(), rows);
  }
};

template <> struct Moved_From_Case<Random_Life>
{
  static Random_Life make(const size_t n)
  {
    Random_Life e(life_grid(n), make_game_of_life_rule(), Moore<2, 1>{},
                  Random_Asynchronous_Update<>(7, 10));
    e.step();
    return e;
  }
  static bool is_empty(const Random_Life & e)
  {
    return e.frame().size() == 0 and e.steps_run() == 0;
  }
  static auto fingerprint(const Random_Life & e) { return engine_print(e); }
  static void use(Random_Life & e) { use_engine(e); }
};

template <> struct Moved_From_Case<Tiles>
{
  static Tiles make(const size_t n)
  {
    Tiles t(tile_extents_of(n), temp_path("tiles"), 4);
    for_each_tiled_cell(t.extents(), [&t](const ca_size_t r, const ca_size_t c)
      {
        t.set(r, c, static_cast<int>((r + c) % 3));
      });
    return t;
  }
  static bool is_empty(const Tiles & t)
  {
    return t.extents()[0] == 0 and t.extents()[1] == 0 and t.tiles_rows() == 0
           and t.resident() == 0 and t.stats().misses == 0;
  }
  // The content is out of reach of a const cache; its activity is not
  static std::tuple<ca_size_t, ca_size_t, size_t, std::uint64_t, std::uint64_t>
  fingerprint(const Tiles & t)
  {
    return {t.extents()[0], t.extents()[1], t.resident(), t.stats().misses, t.stats().writes};
  }
  static void use(Tiles & t)
  {
    for_each_tiled_cell(t.extents(), [&t](const ca_size_t r, const ca_size_t c)
      {
        const int v = t.at(r, c);
        t.set(r, c, v + 1);
        EXPECT_EQ(t.at(r, c), v + 1);
      });
    t.flush();
    EXPECT_LE(t.resident(), t.capacity());
  }
};

template <> struct Moved_From_Case<Density>
{
  static Density make(const size_t n)
  {
    Density o(1);
    observe_run(o, n);
    return o;
  }
  static bool is_empty(const Density & o) { return o.size() == 0 and o.totals().size() == 0; }
  static std::tuple<size_t, size_t, size_t> fingerprint(const Density & o)
  {
    size_t counts = 0;
    size_t totals = 0;
    for (size_t i = 0; i < o.size(); ++i)
      {
        counts += o.counts()[i];
        totals += o.totals()[i];
      }
    return {o.size(), counts, totals};
  }
  static void use(Density & o)
  {
    const size_t before = o.size();
    observe_run(o, 3);
    // An observer with no samples records the initial frame of the run too
    EXPECT_EQ(o.size(), before + 3 + (before == 0 ? 1 : 0));
    for (size_t i = 0; i < o.size(); ++i)
      EXPECT_LE(o.density_at(i), 1.0);
  }
};

template <> struct Moved_From_Case<Entropy>
{
  static Entropy make(const size_t n)
  {
    Entropy o;
    observe_run(o, n);
    return o;
  }
  static bool is_empty(const Entropy & o) { return o.size() == 0; }
  static std::pair<size_t, double> fingerprint(const Entropy & o)
  {
    double sum = 0;
    for (size_t i = 0; i < o.size(); ++i)
      sum += o.entropy()[i];
    return {o.size(), sum};
  }
  static void use(Entropy & o)
  {
    const size_t before = o.size();
    observe_run(o, 3);
    // An observer with no samples records the initial frame of the run too
    EXPECT_EQ(o.size(), before + 3 + (before == 0 ? 1 : 0));
    for (size_t i = 0; i < o.size(); ++i)
      EXPECT_GE(o.entropy()[i], 0.0);
  }
};

using Engines = ::testing::Types<Dense_Cell_Storage<int, 2>, Bit_Cell_Storage<2>, Ghost_Grid,
                                 Field_Grid, Graph_Lattice<int>, Life, Async_Life, Parallel_Life,
                                 Field_Life, Graph_Life, Hashlife_Engine, Locked, Const_Locked,
                                 Read_Locked, Write_Locked, CsvReader, Random_Life, Tiles,
                                 Density, Entropy>;
INSTANTIATE_TYPED_TEST_SUITE_P(Engines, MovedFrom, Engines);

// A move assignment exchanges the simulations of two engines, with their
// steps
template <class Engine>
void expect_exchanged_steps()
{
  Engine a = Moved_From_Case<Engine>::make(100);  // one step
  Engine b = Moved_From_Case<Engine>::make(1000);
  b.run(4);
  a = std::move(b);
  EXPECT_EQ(a.steps_run(), 5u);
  EXPECT_EQ(a.frame().size(), 1000u);
  EXPECT_EQ(b.steps_run(), 1u);
  EXPECT_EQ(b.frame().size(), 100u);
}

TEST(MovedFromEngines, AnAssignmentExchangesTheSteps)
{
  expect_exchanged_steps<Life>();
  expect_exchanged_steps<Async_Life>();
  expect_exchanged_steps<Random_Life>();
  expect_exchanged_steps<Parallel_Life>();
  expect_exchanged_steps<Field_Life>();
  expect_exchanged_steps<Graph_Life>();
}

// An engine on an empty lattice, with a scheme that picks a fixed number of
// cells at random, steps without picking any, as the moved-from one does
TEST(MovedFromEngines, AnEmptyRandomEngineSteps)
{
  Random_Life e(Grid(), make_game_of_life_rule(), Moore<2, 1>{},
                Random_Asynchronous_Update<>(7, 10));
  EXPECT_NO_THROW(e.run(3));
  EXPECT_EQ(e.steps_run(), 3u);
  EXPECT_EQ(e.frame().size(), 0u);
}

// A moved-from cache has no cells: it reads and writes no tile, in no
// directory
TEST(MovedFromTileCache, TheMovedFromCacheTouchesNoFile)
{
  Tiles src({8, 8}, temp_path("tiles"), 2);
  src.set(1, 1, 5);
  Tiles dst(std::move(src));
  EXPECT_THROW(src.set(5, 5, 7), std::out_of_range);
  EXPECT_THROW((void) src.at(0, 0), std::out_of_range);
  src.flush();
  EXPECT_EQ(src.stats().writes, 0u);
  EXPECT_EQ(dst.at(1, 1), 5);
}

// A moved-from engine keeps no hook of the simulation that it gave
TEST(MovedFromEngines, TheHooksGoWithTheSimulation)
{
  size_t fired = 0;
  Life src(life_grid(100), make_game_of_life_rule(), Moore<2, 1>{});
  src.on_post_step([&fired](size_t, const Grid &) { ++fired; });
  Life dst(std::move(src));
  src.step();
  EXPECT_EQ(fired, 0u);
  dst.step();
  EXPECT_EQ(fired, 1u);
}

// The lock goes with the moved guard, while another thread writes the value
// under the lock; the moved-from guard gives no access, and the guard that
// takes another one by assignment releases its own lock
TEST(MovedFromGuards, TheLockGoesWithTheGuard)
{
  Synchronized<long> s(0);
  Synchronized<long> t(0);
  std::thread other([&s]
    {
      for (int i = 0; i < 1000; ++i)
        ++*s.lock();
    });
  for (int i = 0; i < 1000; ++i)
    {
      Locked g = s.lock();
      Locked taken = std::move(g);
      EXPECT_EQ(g.operator->(), nullptr);
      ++*taken;
      Locked last = t.lock();
      last = std::move(taken);
      EXPECT_EQ(taken.operator->(), nullptr);
      ++*last;
    }
  other.join();
  EXPECT_EQ(*s.lock(), 3000);
  EXPECT_EQ(*t.lock(), 0);
}

// The reader that takes a file reads it also after the moved-from reader is
// destroyed
TEST(MovedFromCsvReader, TheFileGoesWithTheReader)
{
  auto src = std::make_unique<CsvReader>(write_csv(5));
  CsvReader dst(std::move(*src));
  src.reset();
  ASSERT_TRUE(dst.has_next());
  EXPECT_EQ(dst.read_header()[1], "5");
  size_t rows = 0;
  while (dst.has_next())
    {
      EXPECT_EQ(dst.next().size(), 3u);
      ++rows;
    }
  EXPECT_EQ(rows, 5u);
}

// A reader of a stream that it does not own gives the stream to the reader
// that takes it, and reads nothing afterwards
TEST(MovedFromCsvReader, AReaderOfAStreamGivesTheStream)
{
  std::istringstream in("a,b\n1,2\n3,4\n");
  CsvReader src(in);
  CsvReader dst(std::move(src));
  EXPECT_FALSE(src.has_next());
  EXPECT_EQ(dst.read_header().size(), 2u);
  size_t rows = 0;
  while (dst.has_next())
    {
      EXPECT_EQ(dst.next().size(), 2u);
      ++rows;
    }
  EXPECT_EQ(rows, 2u);
  src.reset();
  EXPECT_FALSE(src.has_next());
}
