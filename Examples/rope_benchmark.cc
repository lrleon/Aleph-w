/*
                          Aleph_w

  Data structures & Algorithms
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
 * @file rope_benchmark.cc
 * @brief Benchmarks `Aleph::Rope` against `std::string` for the two access
 *        patterns each type is respectively built for.
 *
 * ## What this measures
 *
 * - **Large concatenation**: building a big sequence by repeatedly
 *   appending small chunks. `std::string::operator+=` is the natural
 *   baseline here -- it is already amortized O(1) per append (geometric
 *   growth), so this benchmark is not "Rope wins trivially"; it exists to
 *   confirm `Rope`'s leaf-absorption fast path (see tpl_rope.H's "Leaf
 *   absorption" note) keeps it competitive rather than falling back to
 *   its pre-fix near-O(n^2/LeafSize) behavior for this exact pattern.
 * - **Structural editing**: repeatedly inserting/erasing in the middle of
 *   a large, already-built sequence. This is where `Rope` is expected to
 *   win decisively: `insert`/`erase` are typically `O(log n)` (share whole
 *   subtrees and only touch boundary leaves in the common case), while
 *   `std::string::insert`/`erase` are `O(n)` (shift every following
 *   byte).
 *
 * ## Usage
 *
 * ```bash
 * ./rope_benchmark
 * ./rope_benchmark --chunks 500000 --edits 20000
 * ```
 *
 * @see tpl_rope.H for the full complexity contract.
 * @author Leandro Rabindranath Leon
 * @ingroup Examples
 */

#include <tpl_rope.H>

#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <string_view>

using namespace Aleph;

namespace
{
using Clock = std::chrono::steady_clock;

double elapsed_ms(const Clock::time_point start)
{
  return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

void print_row(const std::string & label, const double ms, const size_t final_size)
{
  std::cout << "  " << std::left << std::setw(28) << label << std::right
            << std::setw(10) << std::fixed << std::setprecision(2) << ms << " ms"
            << "   (final size " << final_size << ")\n";
}

/// Build a sequence of `chunk_count` one-character chunks (cycling
/// through the alphabet) by repeated `std::string::operator+=`.
void bench_string_append(const int chunk_count)
{
  const auto start = Clock::now();
  std::string s;
  for (int i = 0; i < chunk_count; ++i)
    {
      const char c = static_cast<char>('a' + (i % 26));
      s += std::string_view(&c, 1);
    }
  print_row("std::string::operator+=", elapsed_ms(start), s.size());
}

/// Same construction, via repeated `Rope::concat`.
void bench_rope_concat(const int chunk_count)
{
  const auto start = Clock::now();
  Rope<char> r;
  for (int i = 0; i < chunk_count; ++i)
    {
      const char c = static_cast<char>('a' + (i % 26));
      r = r.concat(Rope<char>{std::string_view(&c, 1)});
    }
  print_row("Rope::concat (1 char/call)", elapsed_ms(start), r.size());
}

/// Same total size, but concatenating `chunk_size`-character pieces
/// instead of one character at a time -- the pattern `Rope` is actually
/// designed for (see tpl_rope.H's "Leaf absorption" @warning: one
/// character at a time is specifically *not* it, since every successful
/// absorption re-copies the whole receiving leaf). Included alongside the
/// 1-char case above for an honest picture: `Rope` is not a universal
/// `std::string::operator+=` replacement, it trades that narrow case away
/// for typical O(log n) structural edits (see bench_rope_edits) and O(1)
/// structural sharing that `std::string` cannot offer at all.
void bench_rope_concat_chunked(const int total_chars, const int chunk_size)
{
  const auto start = Clock::now();
  Rope<char> r;
  if (total_chars <= 0 or chunk_size <= 0)
    {
      print_row("Rope::concat (" + std::to_string(chunk_size) + " chars/call)",
                elapsed_ms(start), r.size());
      return;
    }
  std::string chunk(static_cast<size_t>(chunk_size), 'a');
  for (int built = 0; built < total_chars; )
    {
      const int remaining = total_chars - built;
      const int piece_size = remaining < chunk_size ? remaining : chunk_size;
      const std::string_view piece(chunk.data(), static_cast<size_t>(piece_size));
      r = r.concat(Rope<char>{piece});
      built += piece_size;
    }
  print_row("Rope::concat (" + std::to_string(chunk_size) + " chars/call)",
            elapsed_ms(start), r.size());
}

/// Repeatedly insert and erase a short marker in the middle of an
/// already-built large sequence -- `std::string`'s O(n) shift-heavy case,
/// vs. `Rope`'s typical O(log n) share-subtrees case.
void bench_string_edits(const std::string & base, const int edit_count)
{
  const auto start = Clock::now();
  std::string s = base;
  const std::string marker = "<<edit>>";
  std::mt19937 rng(0xEDEDu);
  for (int i = 0; i < edit_count; ++i)
    {
      std::uniform_int_distribution<size_t> pos_dist(0, s.size());
      const size_t pos = pos_dist(rng);
      s.insert(pos, marker);
      s.erase(pos, marker.size());
    }
  print_row("std::string insert+erase", elapsed_ms(start), s.size());
}

void bench_rope_edits(const Rope<char> & base, const int edit_count)
{
  const auto start = Clock::now();
  Rope<char> r = base;
  const Rope<char> marker{std::string_view("<<edit>>")};
  std::mt19937 rng(0xEDEDu);
  for (int i = 0; i < edit_count; ++i)
    {
      std::uniform_int_distribution<size_t> pos_dist(0, r.size());
      const size_t pos = pos_dist(rng);
      r = r.insert(pos, marker);
      r = r.erase(pos, marker.size());
    }
  print_row("Rope insert+erase", elapsed_ms(start), r.size());
}
}  // namespace

int main(int argc, char * argv[])
{
  // Defaults large enough to actually reach the crossover where Rope's
  // typical O(log n) structural edits beat std::string's O(n) ones (see [2]
  // below); smaller sizes finish faster but may not show it.
  int chunk_count = 2000000;
  int edit_count = 5000;

  for (int i = 1; i < argc; ++i)
    {
      const std::string_view arg = argv[i];
      if (arg == "--chunks" and i + 1 < argc)
        chunk_count = std::atoi(argv[++i]);
      else if (arg == "--edits" and i + 1 < argc)
        edit_count = std::atoi(argv[++i]);
      else if (arg == "--help")
        {
          std::cout << "Usage: rope_benchmark [--chunks N] [--edits N]\n";
          return 0;
        }
    }
  if (chunk_count < 0)
    chunk_count = 0;
  if (edit_count < 0)
    edit_count = 0;

  std::cout << "\n=== Aleph::Rope vs std::string benchmark ===\n\n";

  std::cout << "[1] Large concatenation: " << chunk_count
            << " single-character appends\n";
  bench_string_append(chunk_count);
  bench_rope_concat(chunk_count);
  std::cout << "  (Rope's documented weak case -- every successful "
               "absorption re-copies the whole receiving leaf; see below "
               "for the pattern it is actually built for.)\n";

  constexpr int chunk_size = 1000;
  const int chunked_calls =
    chunk_count == 0 ? 0 : (chunk_count + chunk_size - 1) / chunk_size;
  std::cout << "\n[1b] Same total size via " << chunked_calls
            << " chunked concatenations (" << chunk_size
            << " chars/call, Rope's intended "
               "usage pattern)\n";
  bench_rope_concat_chunked(chunk_count, chunk_size);

  std::cout << "\n[2] Structural editing: " << edit_count
            << " random middle insert+erase pairs on a "
            << chunk_count << "-character base sequence\n";
  const std::string base_string(static_cast<size_t>(chunk_count), 'x');
  const Rope<char> base_rope{std::string_view(base_string)};
  bench_string_edits(base_string, edit_count);
  bench_rope_edits(base_rope, edit_count);

  std::cout << "\nDone.\n";
  return 0;
}
