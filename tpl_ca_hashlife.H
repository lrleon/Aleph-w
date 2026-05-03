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

/** @file tpl_ca_hashlife.H
 *  @brief Hashlife engine for outer-totalistic binary cellular automata.
 *
 *  `Hashlife_Engine` is the Phase 10 specialised engine of `Aleph::CA`. It
 *  represents the universe as a memoised quadtree of canonical (hash-consed)
 *  nodes and implements Bill Gosper's *Hashlife* algorithm:
 *
 *    - Every node is a power-of-two square (level `k` ⇒ side `2^k`).
 *    - Identical subtrees are interned in a global hash table, so the
 *      memory cost is proportional to the number of *distinct* sub-patterns,
 *      not to the area of the universe.
 *    - For every node of level `k ≥ 2`, the central `2^(k-1) × 2^(k-1)` area
 *      after `2^(k-2)` generations is computed once and cached, allowing
 *      *exponential* time advances: a single `advance(k)` call advances by
 *      `2^k` generations and reuses results across the entire universe.
 *
 *  The engine is restricted to:
 *
 *    - Two-state cells (alive / dead).
 *    - Outer-totalistic rules with the Moore radius-1 (8-neighbour) pattern.
 *      Pre-defined: `Conway_Life`, `HighLife`, `Day_And_Night`, `Seeds`.
 *
 *  These cover the vast majority of "Game of Life"-style CA used in the
 *  literature. Reaction-diffusion, Ising, multi-state and graph automata
 *  remain in the synchronous / parallel engines (`Synchronous_Engine`,
 *  `Parallel_Synchronous_Engine`).
 *
 *  @par Quick start
 *  @code
 *    Aleph::CA::Hashlife_Engine engine;            // Conway's GoL by default
 *    engine.set_alive(0, 0); engine.set_alive(1, 0); engine.set_alive(2, 0);
 *    engine.advance(10);                            // 1024 generations
 *    std::cout << engine.population() << '\n';
 *  @endcode
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef TPL_CA_HASHLIFE_H
#define TPL_CA_HASHLIFE_H

#include <algorithm>
#include <bit>
#include <cctype>
#include <cstdint>
#include <deque>
#include <istream>
#include <limits>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <ah-errors.H>

namespace Aleph {
namespace CA {

/** @brief Outer-totalistic binary rule encoded as two 9-bit bitmasks.
 *
 *  Bit `i` (`0 ≤ i ≤ 8`) of `birth_mask` is set iff a dead cell with `i`
 *  live Moore-radius-1 neighbours becomes alive in the next generation.
 *  Bit `i` of `survival_mask` is set iff a live cell with `i` live
 *  neighbours stays alive.
 *
 *  Pre-defined instances:
 *    - `Conway_Life`   — `B3/S23`.
 *    - `HighLife`      — `B36/S23`.
 *    - `Day_And_Night` — `B3678/S34678`.
 *    - `Seeds`         — `B2/S` (no survival).
 */
struct Outer_Totalistic_Binary_Rule
{
  std::uint16_t birth_mask;     ///< `bit i` ⇒ dead cell with `i` live neighbours becomes alive
  std::uint16_t survival_mask;  ///< `bit i` ⇒ live cell with `i` live neighbours stays alive

  /// True iff a cell with `current` state and `n` live neighbours is alive next.
  [[nodiscard]] constexpr bool apply(const bool current, const std::uint8_t n) const noexcept
  {
    return current ? (((survival_mask >> n) & 1u) != 0) : (((birth_mask >> n) & 1u) != 0);
  }

  [[nodiscard]] constexpr bool operator==(const Outer_Totalistic_Binary_Rule &) const noexcept
    = default;
};

/// Conway's Game of Life: `B3/S23`.
inline constexpr Outer_Totalistic_Binary_Rule Conway_Life{
  static_cast<std::uint16_t>(1u << 3), static_cast<std::uint16_t>((1u << 2) | (1u << 3))};

/// Nathan Thompson's HighLife: `B36/S23` (replicators).
inline constexpr Outer_Totalistic_Binary_Rule HighLife{
  static_cast<std::uint16_t>((1u << 3) | (1u << 6)),
  static_cast<std::uint16_t>((1u << 2) | (1u << 3))};

/// Bays' Day & Night: `B3678/S34678` (self-complementary).
inline constexpr Outer_Totalistic_Binary_Rule Day_And_Night{
  static_cast<std::uint16_t>((1u << 3) | (1u << 6) | (1u << 7) | (1u << 8)),
  static_cast<std::uint16_t>((1u << 3) | (1u << 4) | (1u << 6) | (1u << 7) | (1u << 8))};

/// Seeds: `B2/S` (every live cell dies, two neighbours produce a birth).
inline constexpr Outer_Totalistic_Binary_Rule Seeds{static_cast<std::uint16_t>(1u << 2),
                                                    std::uint16_t{0}};

/** @brief Format a rule as a Conway-style `Bxxx/Sxxx` string.
 *
 *  @param r the rule to format.
 *  @return human-readable rule string.
 */
[[nodiscard]] inline std::string format_rule(const Outer_Totalistic_Binary_Rule &r)
{
  std::string s;
  s.reserve(20);
  s.push_back('B');
  for (int i = 0; i <= 8; ++i)
    if (((r.birth_mask >> i) & 1u) != 0)
      s.push_back(static_cast<char>('0' + i));
  s.push_back('/');
  s.push_back('S');
  for (int i = 0; i <= 8; ++i)
    if (((r.survival_mask >> i) & 1u) != 0)
      s.push_back(static_cast<char>('0' + i));
  return s;
}

/** @brief Parse a `B.../S...` (or Wolfram `S/B`) rule string.
 *
 *  Accepts mixed case and tolerates surrounding whitespace.
 *
 *  @param s rule string (e.g. `"B3/S23"` or `"23/3"`).
 *  @return the parsed rule, or `std::nullopt` on syntactic error.
 */
[[nodiscard]] inline std::optional<Outer_Totalistic_Binary_Rule> parse_rule(const std::string &s)
{
  std::uint16_t birth = 0, survival = 0;
  std::size_t i = 0;
  while (i < s.size() and std::isspace(static_cast<unsigned char>(s[i])))
    ++i;
  if (i < s.size() and (s[i] == 'B' or s[i] == 'b'))
    {
      ++i;
      while (i < s.size() and std::isdigit(static_cast<unsigned char>(s[i])))
        {
          birth |= static_cast<std::uint16_t>(1u << (s[i] - '0'));
          ++i;
        }
      if (i < s.size() and s[i] == '/')
        ++i;
      if (i < s.size() and (s[i] == 'S' or s[i] == 's'))
        ++i;
      while (i < s.size() and std::isdigit(static_cast<unsigned char>(s[i])))
        {
          survival |= static_cast<std::uint16_t>(1u << (s[i] - '0'));
          ++i;
        }
    }
  else
    {
      while (i < s.size() and std::isdigit(static_cast<unsigned char>(s[i])))
        {
          survival |= static_cast<std::uint16_t>(1u << (s[i] - '0'));
          ++i;
        }
      if (i >= s.size() or s[i] != '/')
        return std::nullopt;
      ++i;
      while (i < s.size() and std::isdigit(static_cast<unsigned char>(s[i])))
        {
          birth |= static_cast<std::uint16_t>(1u << (s[i] - '0'));
          ++i;
        }
    }
  return Outer_Totalistic_Binary_Rule{birth, survival};
}

namespace ca_hashlife_detail {

/// Canonical quadtree node. Nodes are interned: two nodes with identical
/// children (or identical bits, for level-1 leaves) are the same `Node *`.
struct Node
{
  std::uint8_t level;        ///< `1` ⇒ 2x2 leaf; `k` ⇒ 2^k × 2^k node
  std::uint8_t bits;         ///< for level-1 leaves: NW NE SW SE bits
  std::uint64_t population;  ///< number of alive cells covered by this node
  Node *nw;
  Node *ne;
  Node *sw;
  Node *se;
  Node *result;  ///< memoised next-generation result (level `level - 1`)
};

/// Hash key used to canonicalise nodes via `std::unordered_map`.
struct Node_Key
{
  std::uint8_t level;
  std::uint8_t bits;
  Node *nw;
  Node *ne;
  Node *sw;
  Node *se;

  [[nodiscard]] bool operator==(const Node_Key &o) const noexcept
  {
    return level == o.level and bits == o.bits and nw == o.nw and ne == o.ne and sw == o.sw
       and se == o.se;
  }
};

struct Node_Key_Hash
{
  [[nodiscard]] std::size_t operator()(const Node_Key &k) const noexcept
  {
    auto mix = [](std::size_t a, std::size_t b) noexcept -> std::size_t
    {
      return a ^ (b + 0x9e3779b97f4a7c15ULL + (a << 6) + (a >> 2));
    };
    std::size_t h = (static_cast<std::size_t>(k.level) << 8) | k.bits;
    h = mix(h, reinterpret_cast<std::uintptr_t>(k.nw));
    h = mix(h, reinterpret_cast<std::uintptr_t>(k.ne));
    h = mix(h, reinterpret_cast<std::uintptr_t>(k.sw));
    h = mix(h, reinterpret_cast<std::uintptr_t>(k.se));
    return h;
  }
};

}  // namespace ca_hashlife_detail

/** @brief Hashlife engine for outer-totalistic binary cellular automata.
 *
 *  Implements Bill Gosper's *Hashlife* algorithm on top of a hash-consed
 *  quadtree. Suitable for very large patterns (10⁶+ cells) advanced by
 *  enormous numbers of generations (10⁹+) — uses cases that are out of
 *  reach for the dense `Synchronous_Engine`.
 *
 *  @par Algorithm
 *  Every node represents a `2^k × 2^k` square; level-1 leaves pack their
 *  4 cells into a single byte. `evolve(n)` returns the central
 *  `2^(k-1) × 2^(k-1)` area of `n` after `2^(k-2)` generations:
 *  it builds the 9 overlapping level `k-1` sub-squares of `n`, evolves
 *  each (giving 9 level `k-2` results), recombines them into 4 level
 *  `k-1` macrocells advanced by `2^(k-3)` generations, and evolves those
 *  for another `2^(k-3)` generations — totalling `2^(k-2)`.
 *
 *  @par Memoisation
 *  Each canonical node caches its `evolve` result. Because identical
 *  subtrees share the same `Node *`, the recursion sees an effective
 *  subset of nodes that grows with the pattern's *complexity*, not its
 *  *size*. Patterns with strong spatial / temporal self-similarity
 *  (gliders, repeating engines, regular gun outputs) are accelerated
 *  by orders of magnitude.
 *
 *  @par Memory bounds
 *  When the canonical-node hash table exceeds `cache_capacity()`, the
 *  per-node `result` pointers are invalidated (the canonical nodes
 *  themselves are kept alive). This bounds the memoisation footprint
 *  while preserving correctness; future `evolve` calls simply recompute.
 *
 *  @par Thread-safety
 *  Not thread-safe. Each engine instance must be driven by a single
 *  thread; the canonical table and `result` cache are written during
 *  evolution.
 *
 *  @ingroup CellularAutomata
 */
class Hashlife_Engine
{
public:
  /// Rule type used by the engine.
  using Rule = Outer_Totalistic_Binary_Rule;

  /// Inclusive bounding box (returned by `bbox`).
  struct BBox
  {
    std::int64_t x_min;
    std::int64_t y_min;
    std::int64_t x_max;
    std::int64_t y_max;

    /// True iff there are no alive cells.
    [[nodiscard]] bool is_empty() const noexcept
    {
      return x_min > x_max;
    }

    /// Width of the box (`0` if empty).
    [[nodiscard]] std::int64_t width() const noexcept
    {
      return is_empty() ? std::int64_t{0} : (x_max - x_min + 1);
    }

    /// Height of the box (`0` if empty).
    [[nodiscard]] std::int64_t height() const noexcept
    {
      return is_empty() ? std::int64_t{0} : (y_max - y_min + 1);
    }
  };

  /// Diagnostics returned by `stats`.
  struct Stats
  {
    std::size_t canonical_nodes;      ///< number of distinct canonical nodes
    std::size_t result_hits;          ///< hits on the per-node `result` cache
    std::size_t result_misses;        ///< misses on the per-node `result` cache
    std::size_t result_cache_clears;  ///< times the result cache was invalidated
    std::uint8_t root_level;          ///< current universe level (side `2^level`)
  };

  /** @brief Build an engine with the given rule and result-cache capacity.
   *
   *  @param rule           outer-totalistic rule (default: Conway's Game of Life).
   *  @param cache_capacity maximum number of canonical nodes before the
   *                       per-node `result` cache is dropped (default: 4 Mi).
   *  @throws std::domain_error if `cache_capacity < 1024`.
   */
  explicit Hashlife_Engine(Rule rule = Conway_Life, std::size_t cache_capacity = std::size_t{1} << 22)
    : rule_(rule), cache_capacity_(cache_capacity)
  {
    ah_domain_error_if(cache_capacity_ < 1024)
      << "Hashlife_Engine: cache_capacity (" << cache_capacity_ << ") below minimum 1024";
    root_ = empty_node(initial_level_);
  }

  /// Replace the rule. Invalidates the result cache.
  void set_rule(const Rule &r)
  {
    if (r == rule_)
      return;
    rule_ = r;
    clear_result_cache();
  }

  /// Current rule.
  [[nodiscard]] Rule rule() const noexcept
  {
    return rule_;
  }

  /** @brief Set or clear the cell at world coordinates `(x, y)`.
   *
   *  Coordinates are signed 64-bit. The universe automatically expands
   *  to contain the requested cell.
   *
   *  @param x      column (positive = east).
   *  @param y      row (positive = south, RLE convention).
   *  @param alive  new state (default `true`).
   */
  void set_alive(const std::int64_t x, const std::int64_t y, const bool alive = true)
  {
    while (true)
      {
        const std::int64_t half = std::int64_t{1} << (root_->level - 1);
        if (x >= -half and x < half and y >= -half and y < half)
          break;
        root_ = expand(root_);
      }
    const std::int64_t half = std::int64_t{1} << (root_->level - 1);
    root_ = set_in_node(root_, x + half, y + half, alive);
  }

  /// Read the cell at world coordinates `(x, y)`. Cells outside the root
  /// area are reported as dead.
  [[nodiscard]] bool alive_at(const std::int64_t x, const std::int64_t y) const
  {
    const std::int64_t half = std::int64_t{1} << (root_->level - 1);
    if (x < -half or x >= half or y < -half or y >= half)
      return false;
    return get_in_node(root_, x + half, y + half);
  }

  /** @brief Advance the universe by `2^k` generations.
   *
   *  The universe is automatically padded so that evolution is well-defined.
   *  If the alive pattern is too dense to fit at level `k + 2`, the engine
   *  expands further and the call advances by `2^L` generations for some
   *  `L > k`. Use the return value to discover the actual advance.
   *
   *  @param k logarithm of the desired step size.
   *  @return number of generations actually advanced (always a power of 2,
   *          always `≥ 2^k`).
   *  @throws std::domain_error if `k > 62`.
   */
  std::uint64_t advance(const unsigned k)
  {
    ah_domain_error_if(k > 62) << "Hashlife_Engine::advance: k=" << k << " exceeds 62";
    const std::uint8_t min_level =
      std::max<std::uint8_t>(static_cast<std::uint8_t>(k + 2), 4);
    while (root_->level < min_level)
      root_ = expand(root_);
    while (not is_centered_well(root_))
      root_ = expand(root_);

    const std::uint8_t L = root_->level;
    Node *const new_root = evolve(root_);
    const std::uint64_t steps = std::uint64_t{1} << (L - 2);
    generation_ += static_cast<std::int64_t>(steps);
    root_ = new_root;

    if (table_.size() > cache_capacity_)
      clear_result_cache();
    return steps;
  }

  /** @brief Advance by exactly `generations` steps.
   *
   *  Decomposes `generations` into a sum of powers of two and chains the
   *  corresponding `advance` calls. The actual advance may slightly exceed
   *  the request when the pattern outgrows the chosen level.
   *
   *  @param generations target step count (`0` is a no-op).
   *  @return number of generations actually advanced (`≥ generations`).
   */
  std::uint64_t run(std::uint64_t generations)
  {
    std::uint64_t advanced = 0;
    while (generations > 0)
      {
        const unsigned k = static_cast<unsigned>(63 - std::countl_zero(generations));
        const std::uint64_t step = advance(k);
        advanced += step;
        if (step >= generations)
          break;
        generations -= step;
      }
    return advanced;
  }

  /// Number of alive cells.
  [[nodiscard]] std::uint64_t population() const noexcept
  {
    return root_->population;
  }

  /// Number of generations elapsed since construction (or the last `clear`).
  [[nodiscard]] std::int64_t generation() const noexcept
  {
    return generation_;
  }

  /// Tight bounding box of the alive cells (empty if `population() == 0`).
  [[nodiscard]] BBox bbox() const
  {
    BBox bb{std::numeric_limits<std::int64_t>::max(), std::numeric_limits<std::int64_t>::max(),
            std::numeric_limits<std::int64_t>::min(), std::numeric_limits<std::int64_t>::min()};
    if (root_->population == 0)
      return bb;
    const std::int64_t half = std::int64_t{1} << (root_->level - 1);
    visit_alive(root_, -half, -half, [&bb](std::int64_t x, std::int64_t y) noexcept
    {
      if (x < bb.x_min)
        bb.x_min = x;
      if (x > bb.x_max)
        bb.x_max = x;
      if (y < bb.y_min)
        bb.y_min = y;
      if (y > bb.y_max)
        bb.y_max = y;
    });
    return bb;
  }

  /// Reset the universe to empty (preserves rule, capacity and node cache).
  void clear()
  {
    generation_ = 0;
    root_ = empty_node(initial_level_);
  }

  /// Diagnostic counters and current root level.
  [[nodiscard]] Stats stats() const noexcept
  {
    return Stats{table_.size(), result_hits_, result_misses_, result_cache_clears_, root_->level};
  }

  /// Maximum allowed canonical-node count before the result cache is dropped.
  [[nodiscard]] std::size_t cache_capacity() const noexcept
  {
    return cache_capacity_;
  }

  /** @brief Iterate over all alive cells, calling `f(x, y)` for each.
   *
   *  Visits each alive cell exactly once, in unspecified order.
   *
   *  @tparam F callable accepting `(std::int64_t, std::int64_t)`.
   *  @param  f visitor.
   */
  template <typename F>
  void for_each_alive(F &&f) const
  {
    if (root_->population == 0)
      return;
    const std::int64_t half = std::int64_t{1} << (root_->level - 1);
    visit_alive(root_, -half, -half, std::forward<F>(f));
  }

  // ------------------------------------------------------------------
  // RLE (Run-Length-Encoded) Conway-format I/O.
  // ------------------------------------------------------------------

  /** @brief Load an RLE pattern from `in` (centred at the origin).
   *
   *  Parses the standard RLE dialect (`#` comments, `x = W, y = H, rule = R`
   *  header, `b`/`o`/`$`/`!` body, run-length prefixes). The rule field is
   *  parsed and replaces the engine's current rule when valid.
   *
   *  @param in input stream positioned at the start of the pattern.
   */
  void load_rle(std::istream &in)
  {
    std::string line;
    bool header_seen = false;
    std::int64_t origin_x = 0;
    std::int64_t origin_y = 0;
    std::int64_t cur_x = 0;
    std::int64_t cur_y = 0;
    std::int64_t run = 0;
    bool ended = false;

    while (not ended and std::getline(in, line))
      {
        if (line.empty() or line[0] == '#')
          continue;
        if (not header_seen)
          {
            const std::int64_t x_size = parse_header_value(line, 'x');
            const std::int64_t y_size = parse_header_value(line, 'y');
            if (auto rule_opt = parse_rule_field(line); rule_opt)
              rule_ = *rule_opt;
            origin_x = -(x_size / 2);
            origin_y = -(y_size / 2);
            cur_x = origin_x;
            cur_y = origin_y;
            header_seen = true;
            continue;
          }
        for (const char c : line)
          {
            if (std::isdigit(static_cast<unsigned char>(c)))
                run = run * 10 + (c - '0');
            else if (c == 'b' or c == '.')
              {
                cur_x += (run == 0 ? 1 : run);
                run = 0;
              }
            else if (c == 'o' or c == 'A')
              {
                const std::int64_t r = (run == 0 ? 1 : run);
                for (std::int64_t i = 0; i < r; ++i)
                  set_alive(cur_x + i, cur_y, true);
                cur_x += r;
                run = 0;
              }
            else if (c == '$')
              {
                cur_y += (run == 0 ? 1 : run);
                cur_x = origin_x;
                run = 0;
              }
            else if (c == '!')
              {
                ended = true;
                break;
              }
          }
      }
  }

  /// Convenience overload that reads the pattern from a string.
  void load_rle_string(const std::string &s)
  {
    std::istringstream iss(s);
    load_rle(iss);
  }

  /** @brief Write the alive cells in RLE format.
   *
   *  Emits a single-line header (`x = W, y = H, rule = ...`) and a
   *  run-length-encoded body wrapped at ~70 characters per line.
   *
   *  @param out      output stream.
   *  @param comment  optional `#C` comment line (ignored if empty).
   */
  void save_rle(std::ostream &out, const std::string &comment = {}) const
  {
    if (not comment.empty())
      {
        std::istringstream cs(comment);
        std::string l;
        while (std::getline(cs, l))
          out << "#C " << l << '\n';
      }
    const BBox bb = bbox();
    if (bb.is_empty())
      {
        out << "x = 0, y = 0, rule = " << format_rule(rule_) << "\n!\n";
        return;
      }

    // Collect alive cells sorted by (y, x).
    std::vector<std::pair<std::int64_t, std::int64_t>> cells;
    cells.reserve(static_cast<std::size_t>(root_->population));
    for_each_alive([&cells](std::int64_t x, std::int64_t y)
    {
      cells.emplace_back(y, x);
    });
    std::sort(cells.begin(), cells.end());

    out << "x = " << bb.width() << ", y = " << bb.height() << ", rule = " << format_rule(rule_)
        << '\n';

    std::ostringstream body;
    auto emit_run = [&body](const std::int64_t n, const char glyph)
    {
      if (n <= 0)
        return;
      if (n != 1)
        body << n;
      body << glyph;
    };

    std::int64_t prev_y = bb.y_min;
    std::int64_t cur_x = bb.x_min;
    std::size_t i = 0;
    while (i < cells.size())
      {
        const std::int64_t y = cells[i].first;
        const std::int64_t x = cells[i].second;
        if (y != prev_y)
          {
            emit_run(y - prev_y, '$');
            prev_y = y;
            cur_x = bb.x_min;
          }
        if (x > cur_x)
          {
            emit_run(x - cur_x, 'b');
            cur_x = x;
          }
        std::int64_t alive_run = 0;
        std::size_t j = i;
        while (j < cells.size() and cells[j].first == y and cells[j].second == cur_x + alive_run)
          {
            ++alive_run;
            ++j;
          }
        emit_run(alive_run, 'o');
        cur_x += alive_run;
        i = j;
      }
    body << "!\n";

    // Wrap body at ~70 columns, never breaking inside a `<digits><glyph>` token.
    const std::string body_str = body.str();
    std::size_t pos = 0;
    while (pos < body_str.size())
      {
        std::size_t end = std::min<std::size_t>(pos + 70, body_str.size());
        while (end < body_str.size() and std::isdigit(static_cast<unsigned char>(body_str[end])))
          ++end;
        if (end < body_str.size() and (body_str[end] == 'b' or body_str[end] == 'o'))
          ++end;
        out << body_str.substr(pos, end - pos) << '\n';
        pos = end;
      }
  }

  /// Convenience overload returning the RLE serialisation as a string.
  [[nodiscard]] std::string save_rle_string(const std::string &comment = {}) const
  {
    std::ostringstream oss;
    save_rle(oss, comment);
    return oss.str();
  }

private:
  using Node = ca_hashlife_detail::Node;
  using Node_Key = ca_hashlife_detail::Node_Key;

  static constexpr std::uint8_t initial_level_ = 3;  ///< 8 × 8 starting universe

  Rule rule_;
  std::size_t cache_capacity_;

  std::deque<Node> pool_;
  std::unordered_map<Node_Key, Node *, ca_hashlife_detail::Node_Key_Hash> table_;
  std::vector<Node *> empty_by_level_;

  Node *root_ = nullptr;
  std::int64_t generation_ = 0;

  mutable std::size_t result_hits_ = 0;
  mutable std::size_t result_misses_ = 0;
  std::size_t result_cache_clears_ = 0;

  // -------------------- Canonical node creation --------------------

  Node *make_leaf(const std::uint8_t bits)
  {
    const Node_Key key{1, bits, nullptr, nullptr, nullptr, nullptr};
    if (auto it = table_.find(key); it != table_.end())
      return it->second;
    Node n{};
    n.level = 1;
    n.bits = bits;
    n.population = static_cast<std::uint64_t>(std::popcount(bits));
    pool_.push_back(n);
    Node *const p = &pool_.back();
    table_.emplace(key, p);
    return p;
  }

  Node *make_node(Node *nw, Node *ne, Node *sw, Node *se)
  {
    const Node_Key key{static_cast<std::uint8_t>(nw->level + 1), 0, nw, ne, sw, se};
    if (auto it = table_.find(key); it != table_.end())
      return it->second;
    Node n{};
    n.level = key.level;
    n.bits = 0;
    n.population = nw->population + ne->population + sw->population + se->population;
    n.nw = nw;
    n.ne = ne;
    n.sw = sw;
    n.se = se;
    pool_.push_back(n);
    Node *const p = &pool_.back();
    table_.emplace(key, p);
    return p;
  }

  Node *empty_node(const std::uint8_t level)
  {
    while (empty_by_level_.size() <= level)
      {
        const std::uint8_t L = static_cast<std::uint8_t>(empty_by_level_.size());
        if (L == 0)
          {
            empty_by_level_.push_back(nullptr);
            continue;
          }
        Node *n = (L == 1) ? make_leaf(0)
                           : make_node(empty_by_level_[L - 1], empty_by_level_[L - 1],
                                       empty_by_level_[L - 1], empty_by_level_[L - 1]);
        empty_by_level_.push_back(n);
      }
    return empty_by_level_[level];
  }

  void clear_result_cache()
  {
    for (auto &n : pool_)
      n.result = nullptr;
    result_hits_ = 0;
    result_misses_ = 0;
    ++result_cache_clears_;
  }

  // -------------------- Geometry helpers --------------------

  Node *expand(const Node *n)
  {
    const std::uint8_t L = n->level;
    if (L == 1)
      {
        Node *nw = make_leaf(static_cast<std::uint8_t>((n->bits & 1u) ? (1u << 3) : 0u));
        Node *ne = make_leaf(static_cast<std::uint8_t>((n->bits & 2u) ? (1u << 2) : 0u));
        Node *sw = make_leaf(static_cast<std::uint8_t>((n->bits & 4u) ? (1u << 1) : 0u));
        Node *se = make_leaf(static_cast<std::uint8_t>((n->bits & 8u) ? (1u << 0) : 0u));
        return make_node(nw, ne, sw, se);
      }
    Node *const e = empty_node(static_cast<std::uint8_t>(L - 1));
    Node *new_nw = make_node(e, e, e, n->nw);
    Node *new_ne = make_node(e, e, n->ne, e);
    Node *new_sw = make_node(e, n->sw, e, e);
    Node *new_se = make_node(n->se, e, e, e);
    return make_node(new_nw, new_ne, new_sw, new_se);
  }

  /// Standard Hashlife "centered well" predicate: every quadrant of `n`
  /// has its population concentrated in the sub-quadrant closest to the
  /// centre. This guarantees that, after `evolve` (which advances
  /// `2^(L-2)` generations and returns only the central area), no alive
  /// cell escapes the new root — for any rule whose maximum signal speed
  /// is at most `c/2` (which holds for every life-like outer-totalistic
  /// rule, including Conway's `B3/S23`).
  [[nodiscard]] static bool is_centered_well(const Node *n) noexcept
  {
    if (n->level < 4)
      return false;
    const std::uint64_t central =
      n->nw->se->se->population
    + n->ne->sw->sw->population
    + n->sw->ne->ne->population
    + n->se->nw->nw->population;
    return central == n->population;
  }

  Node *horizontal_centered(Node *w, Node *e)
  {
    return make_node(w->ne, e->nw, w->se, e->sw);
  }

  Node *vertical_centered(Node *n, Node *s)
  {
    return make_node(n->sw, n->se, s->nw, s->ne);
  }

  Node *center_of_4(Node *nw, Node *ne, Node *sw, Node *se)
  {
    return make_node(nw->se, ne->sw, sw->ne, se->nw);
  }

  // -------------------- Evolution --------------------

  Node *evolve_level_2(Node *n)
  {
    const std::uint8_t a = n->nw->bits;
    const std::uint8_t b = n->ne->bits;
    const std::uint8_t c = n->sw->bits;
    const std::uint8_t d = n->se->bits;

    // Pack 4x4 cells row-major into a 16-bit bitmap, bit `(r*4 + c)`.
    std::uint16_t g = 0;
    g |= static_cast<std::uint16_t>((a & 0x3u)) << 0;        // row 0 cols 0-1
    g |= static_cast<std::uint16_t>((b & 0x3u)) << 2;        // row 0 cols 2-3
    g |= static_cast<std::uint16_t>((a >> 2) & 0x3u) << 4;   // row 1 cols 0-1
    g |= static_cast<std::uint16_t>((b >> 2) & 0x3u) << 6;   // row 1 cols 2-3
    g |= static_cast<std::uint16_t>((c & 0x3u)) << 8;        // row 2 cols 0-1
    g |= static_cast<std::uint16_t>((d & 0x3u)) << 10;       // row 2 cols 2-3
    g |= static_cast<std::uint16_t>((c >> 2) & 0x3u) << 12;  // row 3 cols 0-1
    g |= static_cast<std::uint16_t>((d >> 2) & 0x3u) << 14;  // row 3 cols 2-3

    auto get = [g](const int r, const int c) noexcept -> bool
    {
      return ((g >> (r * 4 + c)) & 1u) != 0;
    };
    auto count = [&get](const int r, const int c) noexcept -> std::uint8_t
    {
      return static_cast<std::uint8_t>(get(r - 1, c - 1) + get(r - 1, c) + get(r - 1, c + 1)
                                       + get(r, c - 1) + get(r, c + 1) + get(r + 1, c - 1)
                                       + get(r + 1, c) + get(r + 1, c + 1));
    };

    std::uint8_t result = 0;
    if (rule_.apply(get(1, 1), count(1, 1)))
      result |= 1u << 0;
    if (rule_.apply(get(1, 2), count(1, 2)))
      result |= 1u << 1;
    if (rule_.apply(get(2, 1), count(2, 1)))
      result |= 1u << 2;
    if (rule_.apply(get(2, 2), count(2, 2)))
      result |= 1u << 3;
    return make_leaf(result);
  }

  Node *evolve(Node *n)
  {
    if (n->result != nullptr)
      {
        ++result_hits_;
        return n->result;
      }
    ++result_misses_;
    Node *result = nullptr;
    if (n->level == 2)
      {
        result = evolve_level_2(n);
      }
    else
      {
        Node *const nw = n->nw;
        Node *const ne = n->ne;
        Node *const sw = n->sw;
        Node *const se = n->se;

        Node *n00 = nw;
        Node *n01 = horizontal_centered(nw, ne);
        Node *n02 = ne;
        Node *n10 = vertical_centered(nw, sw);
        Node *n11 = center_of_4(nw, ne, sw, se);
        Node *n12 = vertical_centered(ne, se);
        Node *n20 = sw;
        Node *n21 = horizontal_centered(sw, se);
        Node *n22 = se;

        Node *r00 = evolve(n00);
        Node *r01 = evolve(n01);
        Node *r02 = evolve(n02);
        Node *r10 = evolve(n10);
        Node *r11 = evolve(n11);
        Node *r12 = evolve(n12);
        Node *r20 = evolve(n20);
        Node *r21 = evolve(n21);
        Node *r22 = evolve(n22);

        Node *m_nw = make_node(r00, r01, r10, r11);
        Node *m_ne = make_node(r01, r02, r11, r12);
        Node *m_sw = make_node(r10, r11, r20, r21);
        Node *m_se = make_node(r11, r12, r21, r22);

        Node *q_nw = evolve(m_nw);
        Node *q_ne = evolve(m_ne);
        Node *q_sw = evolve(m_sw);
        Node *q_se = evolve(m_se);

        result = make_node(q_nw, q_ne, q_sw, q_se);
      }
    n->result = result;
    return result;
  }

  // -------------------- Cell-level set / get / iterate --------------------

  Node *set_in_node(const Node *n, const std::int64_t x, const std::int64_t y, const bool alive)
  {
    if (n->level == 1)
      {
        const int bit_idx = static_cast<int>(y) * 2 + static_cast<int>(x);
        std::uint8_t new_bits = n->bits;
        if (alive)
          new_bits |= static_cast<std::uint8_t>(1u << bit_idx);
        else
          new_bits &= static_cast<std::uint8_t>(~(1u << bit_idx));
        return make_leaf(new_bits);
      }
    const std::int64_t half = std::int64_t{1} << (n->level - 1);
    Node *nw = n->nw;
    Node *ne = n->ne;
    Node *sw = n->sw;
    Node *se = n->se;
    if (y < half)
      {
        if (x < half)
          nw = set_in_node(nw, x, y, alive);
        else
          ne = set_in_node(ne, x - half, y, alive);
      }
    else
      {
        if (x < half)
          sw = set_in_node(sw, x, y - half, alive);
        else
          se = set_in_node(se, x - half, y - half, alive);
      }
    return make_node(nw, ne, sw, se);
  }

  [[nodiscard]] static bool get_in_node(const Node *n, const std::int64_t x, const std::int64_t y) noexcept
  {
    if (n->level == 1)
      {
        const int bit_idx = static_cast<int>(y) * 2 + static_cast<int>(x);
        return ((n->bits >> bit_idx) & 1u) != 0;
      }
    const std::int64_t half = std::int64_t{1} << (n->level - 1);
    if (y < half)
      return (x < half) ? get_in_node(n->nw, x, y) : get_in_node(n->ne, x - half, y);
    return (x < half) ? get_in_node(n->sw, x, y - half) : get_in_node(n->se, x - half, y - half);
  }

  template <typename F>
  void visit_alive(Node *n, std::int64_t x, std::int64_t y, F &&f) const
  {
    if (n->population == 0)
      return;
    if (n->level == 1)
      {
        if (n->bits & 0x1u)
          f(x, y);
        if (n->bits & 0x2u)
          f(x + 1, y);
        if (n->bits & 0x4u)
          f(x, y + 1);
        if (n->bits & 0x8u)
          f(x + 1, y + 1);
        return;
      }
    const std::int64_t half = std::int64_t{1} << (n->level - 1);
    visit_alive(n->nw, x, y, f);
    visit_alive(n->ne, x + half, y, f);
    visit_alive(n->sw, x, y + half, f);
    visit_alive(n->se, x + half, y + half, f);
  }

  // -------------------- RLE header parsing --------------------

  static std::int64_t parse_header_value(const std::string &line, const char key)
  {
    auto pos = line.find(key);
    if (pos == std::string::npos)
      return 0;
    pos = line.find('=', pos);
    if (pos == std::string::npos)
      return 0;
    ++pos;
    while (pos < line.size() and std::isspace(static_cast<unsigned char>(line[pos])))
      ++pos;
    bool neg = false;
    if (pos < line.size() and line[pos] == '-')
      {
        neg = true;
        ++pos;
      }
    std::int64_t v = 0;
    while (pos < line.size() and std::isdigit(static_cast<unsigned char>(line[pos])))
      {
        v = v * 10 + (line[pos] - '0');
        ++pos;
      }
    return neg ? -v : v;
  }

  static std::optional<Outer_Totalistic_Binary_Rule> parse_rule_field(const std::string &line)
  {
    auto pos = line.find("rule");
    if (pos == std::string::npos)
      pos = line.find("Rule");
    if (pos == std::string::npos)
      return std::nullopt;
    pos = line.find('=', pos);
    if (pos == std::string::npos)
      return std::nullopt;
    ++pos;
    while (pos < line.size() and std::isspace(static_cast<unsigned char>(line[pos])))
      ++pos;
    std::string token;
    while (pos < line.size() and not std::isspace(static_cast<unsigned char>(line[pos]))
           and line[pos] != ',')
      {
        token.push_back(line[pos]);
        ++pos;
      }
    if (token.empty())
      return std::nullopt;
    return parse_rule(token);
  }
};

}  // namespace CA
}  // namespace Aleph

#endif  // TPL_CA_HASHLIFE_H