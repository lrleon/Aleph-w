
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

/** @file moved_from_structures_test.cc
 *  @brief The moved-from tests (moved_from.H) of the data structures and
 *         the sketches.
 */

#include "moved_from.H"

#include <count-min-sketch.H>
#include <hyperloglog.H>
#include <minhash.H>
#include <reservoir-sampling.H>
#include <tpl_fenwick_tree.H>

#include <cstdint>
#include <limits>

using namespace Aleph;

template <> struct Moved_From_Case<HyperLogLog<int>>
{
  static HyperLogLog<int> make(const size_t n)
  {
    HyperLogLog<int> h(10);
    for (size_t i = 0; i < n; ++i)
      h.update(static_cast<int>(i));
    return h;
  }
  static bool is_empty(const HyperLogLog<int> & h) { return h.estimate() == 0.0; }
  static void use(HyperLogLog<int> & h)
  {
    (void) h.estimate();
    h.update(7);
    h.clear();
    h.update(3);
    EXPECT_GT(h.estimate(), 0.0);
  }
};

template <> struct Moved_From_Case<Count_Min_Sketch<int>>
{
  static Count_Min_Sketch<int> make(const size_t n)
  {
    Count_Min_Sketch<int> s(n < 10 ? 8 : 64, 4);
    for (size_t i = 0; i < n; ++i)
      s.update(static_cast<int>(i));
    return s;
  }
  static bool is_empty(const Count_Min_Sketch<int> & s) { return s.total_count() == 0; }
  static void use(Count_Min_Sketch<int> & s)
  {
    s.update(7, 2);
    EXPECT_GE(s.estimate(7), 2u);
    s.clear();
    EXPECT_EQ(s.estimate(7), 0u);
  }
};

template <> struct Moved_From_Case<MinHash<int>>
{
  static MinHash<int> make(const size_t n)
  {
    MinHash<int> m(n < 10 ? 4 : 64);
    for (size_t i = 0; i < n; ++i)
      m.update(static_cast<int>(i));
    return m;
  }
  static bool is_empty(const MinHash<int> & m)
  {
    for (size_t i = 0; i < m.size(); ++i)
      if (m.get_signature()[i] != std::numeric_limits<uint64_t>::max())
        return false;
    return true;
  }
  static void use(MinHash<int> & m)
  {
    m.update(7);
    MinHash<int> other = m;
    EXPECT_DOUBLE_EQ(m.similarity(other), 1.0);
    m.clear();
  }
};

template <> struct Moved_From_Case<Reservoir_Sampler<int>>
{
  static Reservoir_Sampler<int> make(const size_t n)
  {
    Reservoir_Sampler<int> r(n < 10 ? 2 : 8, 1);
    for (size_t i = 0; i < n; ++i)
      r.update(static_cast<int>(i));
    return r;
  }
  static bool is_empty(const Reservoir_Sampler<int> & r) { return r.total_seen() == 0; }
  static void use(Reservoir_Sampler<int> & r)
  {
    for (int i = 0; i < 20; ++i)
      r.update(i);
    EXPECT_EQ(r.get_sample().size(), r.sample_size());
  }
};

template <> struct Moved_From_Case<Fenwick_Tree<int>>
{
  static Fenwick_Tree<int> make(const size_t n)
  {
    Fenwick_Tree<int> f(n);
    for (size_t i = 0; i < n; ++i)
      f.update(i, 1);
    return f;
  }
  static bool is_empty(const Fenwick_Tree<int> & f) { return f.is_empty(); }
  static void use(Fenwick_Tree<int> & f)
  {
    for (size_t i = 0; i < f.size(); ++i)
      f.update(i, 1);
    if (not f.is_empty())
      (void) f.prefix(f.size() - 1);
  }
};

template <> struct Moved_From_Case<Range_Fenwick_Tree<int>>
{
  static Range_Fenwick_Tree<int> make(const size_t n)
  {
    Range_Fenwick_Tree<int> f(n);
    if (n > 0)
      f.update(0, n - 1, 1);
    return f;
  }
  static bool is_empty(const Range_Fenwick_Tree<int> & f) { return f.is_empty(); }
  static void use(Range_Fenwick_Tree<int> & f)
  {
    if (f.is_empty())
      return;
    f.update(0, f.size() - 1, 2);
    (void) f.query(0, f.size() - 1);
  }
};

using Structures = ::testing::Types<HyperLogLog<int>, Count_Min_Sketch<int>, MinHash<int>,
                                    Reservoir_Sampler<int>, Fenwick_Tree<int>,
                                    Range_Fenwick_Tree<int>>;
INSTANTIATE_TYPED_TEST_SUITE_P(Structures, MovedFrom, Structures);
