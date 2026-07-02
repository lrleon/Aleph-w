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
 * @file small_vector_test.cc
 * @brief Tests for Aleph::SmallVector (tpl_small_vector.H).
 *
 * Covers the inline/heap storage transition (is_small), element lifetime
 * (constructor/destructor balance via an instrumented type), move-only
 * element support, copy/move semantics in both storage modes, positional
 * insert/erase, checked access, and Aleph conventions.
 */

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_small_vector.H>

using Aleph::SmallVector;

namespace
{
// Instrumented element type: counts live instances so tests can prove that
// placement-constructed elements are destroyed exactly once.
struct Probe
{
  static int live;
  int value = 0;

  Probe() { ++live; }
  explicit Probe(int v) : value(v) { ++live; }
  Probe(const Probe &p) : value(p.value) { ++live; }
  Probe(Probe &&p) noexcept : value(p.value) { ++live; }
  Probe &operator=(const Probe &) = default;
  Probe &operator=(Probe &&) noexcept = default;
  ~Probe() { --live; }
};

int Probe::live = 0;

struct Throwing_Copy
{
  static int live;
  static int copies;
  static int throw_after;
  int value = 0;

  explicit Throwing_Copy(int v) : value(v) { ++live; }

  Throwing_Copy(const Throwing_Copy &x) : value(x.value)
  {
    if (throw_after >= 0 and copies++ >= throw_after)
      throw std::runtime_error("copy failed");
    ++live;
  }

  Throwing_Copy(Throwing_Copy &&x) noexcept : value(x.value) { ++live; }

  Throwing_Copy &operator=(const Throwing_Copy &) = default;
  Throwing_Copy &operator=(Throwing_Copy &&) noexcept = default;

  ~Throwing_Copy() { --live; }

  static void reset(int limit = -1)
  {
    copies = 0;
    throw_after = limit;
  }
};

int Throwing_Copy::live = 0;
int Throwing_Copy::copies = 0;
int Throwing_Copy::throw_after = -1;
}  // namespace

TEST(SmallVector, StaysInlineUpToNThenSpills)
{
  SmallVector<int, 4> v;
  EXPECT_TRUE(v.is_small());
  EXPECT_EQ(v.capacity(), 4u);
  EXPECT_TRUE(v.is_empty());

  for (int i = 0; i < 4; ++i)
    v.append(i);
  EXPECT_TRUE(v.is_small());  // exactly N elements: still inline
  EXPECT_EQ(v.size(), 4u);

  v.append(4);  // spill
  EXPECT_FALSE(v.is_small());
  EXPECT_GE(v.capacity(), 5u);
  ASSERT_EQ(v.size(), 5u);
  for (int i = 0; i < 5; ++i)
    EXPECT_EQ(v[i], i);  // values preserved across the spill
}

TEST(SmallVector, InlineBufferIsInsideTheObject)
{
  SmallVector<int, 4> v;
  v.append(1);
  const char *obj = reinterpret_cast<const char *>(&v);
  const char *elem = reinterpret_cast<const char *>(v.data());
  EXPECT_GE(elem, obj);
  EXPECT_LT(elem, obj + sizeof(v));

  for (int i = 0; i < 10; ++i)
    v.append(i);
  const char *heap_elem = reinterpret_cast<const char *>(v.data());
  EXPECT_TRUE(heap_elem < obj or heap_elem >= obj + sizeof(v));
}

TEST(SmallVector, ElementLifetimesAreBalanced)
{
  ASSERT_EQ(Probe::live, 0);
  {
    SmallVector<Probe, 2> v;
    v.emplace_back(1);
    v.emplace_back(2);
    EXPECT_EQ(Probe::live, 2);
    v.emplace_back(3);  // spill: old elements destroyed, new ones created
    EXPECT_EQ(Probe::live, 3);
    v.pop_back();
    EXPECT_EQ(Probe::live, 2);
    v.clear();
    EXPECT_EQ(Probe::live, 0);
    v.emplace_back(9);
    EXPECT_EQ(Probe::live, 1);
  }
  EXPECT_EQ(Probe::live, 0);  // destructor cleaned everything
}

TEST(SmallVector, FailedConstructorsCleanPartialElements)
{
  ASSERT_EQ(Throwing_Copy::live, 0);

  {
    Throwing_Copy value(1);
    Throwing_Copy::reset(1);
    EXPECT_THROW((SmallVector<Throwing_Copy, 2>(3, value)),
                 std::runtime_error);
    EXPECT_EQ(Throwing_Copy::live, 1);
  }
  EXPECT_EQ(Throwing_Copy::live, 0);

  Throwing_Copy::reset(1);
  EXPECT_THROW((SmallVector<Throwing_Copy, 2>
                {Throwing_Copy(1), Throwing_Copy(2), Throwing_Copy(3)}),
               std::runtime_error);
  EXPECT_EQ(Throwing_Copy::live, 0);

  {
    std::vector<Throwing_Copy> src;
    src.emplace_back(1);
    src.emplace_back(2);
    src.emplace_back(3);
    ASSERT_EQ(Throwing_Copy::live, 3);
    Throwing_Copy::reset(1);
    EXPECT_THROW((SmallVector<Throwing_Copy, 2>(src.begin(), src.end())),
                 std::runtime_error);
    EXPECT_EQ(Throwing_Copy::live, 3);
  }
  EXPECT_EQ(Throwing_Copy::live, 0);

  {
    SmallVector<Throwing_Copy, 2> src;
    src.emplace_back(1);
    src.emplace_back(2);
    src.emplace_back(3);
    ASSERT_EQ(Throwing_Copy::live, 3);
    Throwing_Copy::reset(1);
    EXPECT_THROW((SmallVector<Throwing_Copy, 2>(src)), std::runtime_error);
    EXPECT_EQ(Throwing_Copy::live, 3);
  }
  EXPECT_EQ(Throwing_Copy::live, 0);
}

TEST(SmallVector, MoveOnlyElementsAreSupported)
{
  SmallVector<std::unique_ptr<int>, 2> v;
  v.append(std::make_unique<int>(1));
  v.append(std::make_unique<int>(2));
  v.append(std::make_unique<int>(3));  // spill with move-only type
  ASSERT_EQ(v.size(), 3u);
  EXPECT_EQ(*v[0], 1);
  EXPECT_EQ(*v[2], 3);

  std::unique_ptr<int> p = v.remove_last();
  EXPECT_EQ(*p, 3);
  EXPECT_EQ(v.size(), 2u);

  SmallVector<std::unique_ptr<int>, 2> moved = std::move(v);
  ASSERT_EQ(moved.size(), 2u);
  EXPECT_EQ(*moved[1], 2);
}

TEST(SmallVector, CopySemanticsInBothModes)
{
  SmallVector<std::string, 4> inline_v = {"a", "b"};
  SmallVector<std::string, 4> c1 = inline_v;
  EXPECT_EQ(c1, inline_v);
  c1[0] = "zzz";
  EXPECT_EQ(inline_v[0], "a");  // deep copy

  SmallVector<std::string, 2> heap_v = {"a", "b", "c", "d"};
  EXPECT_FALSE(heap_v.is_small());
  SmallVector<std::string, 2> c2 = heap_v;
  EXPECT_EQ(c2, heap_v);

  SmallVector<std::string, 2> small_src = {"s"};
  c2 = small_src;  // copy assignment: heap-mode target, inline-mode source
  ASSERT_EQ(c2.size(), 1u);
  EXPECT_EQ(c2[0], "s");
}

TEST(SmallVector, MoveSemanticsInBothModes)
{
  // Inline mode: elements are moved one by one; source is emptied.
  SmallVector<std::string, 4> a = {"x", "y"};
  SmallVector<std::string, 4> ma = std::move(a);
  ASSERT_EQ(ma.size(), 2u);
  EXPECT_EQ(ma[0], "x");
  EXPECT_TRUE(a.is_empty());

  // Heap mode: the buffer is stolen in O(1); source returns to inline.
  SmallVector<std::string, 2> b = {"1", "2", "3"};
  const std::string *heap_data = b.data();
  SmallVector<std::string, 2> mb = std::move(b);
  EXPECT_EQ(mb.data(), heap_data);  // pointer stolen, no element moves
  EXPECT_TRUE(b.is_empty());
  EXPECT_TRUE(b.is_small());
  b.append("reuse");  // source is reusable after move
  EXPECT_EQ(b[0], "reuse");
}

TEST(SmallVector, InsertAndEraseAtPosition)
{
  SmallVector<int, 4> v = {1, 2, 4};
  v.insert(2, 3);  // {1, 2, 3, 4}
  ASSERT_EQ(v.size(), 4u);
  for (int i = 0; i < 4; ++i)
    EXPECT_EQ(v[i], i + 1);

  v.insert(0, 0);  // {0, 1, 2, 3, 4} — forces spill
  EXPECT_FALSE(v.is_small());
  EXPECT_EQ(v[0], 0);
  EXPECT_EQ(v[4], 4);

  v.insert(v.size(), 5);  // append via insert at end
  EXPECT_EQ(v.get_last(), 5);

  v.erase(0);  // {1, 2, 3, 4, 5}
  EXPECT_EQ(v.get_first(), 1);
  v.erase(v.size() - 1);  // {1, 2, 3, 4}
  EXPECT_EQ(v.get_last(), 4);
  EXPECT_EQ(v.size(), 4u);

  EXPECT_THROW(v.insert(v.size() + 1, 9), std::out_of_range);
  EXPECT_THROW(v.erase(v.size()), std::out_of_range);
}

TEST(SmallVector, CheckedAndUncheckedAccess)
{
  SmallVector<int, 4> v = {10, 20};
  EXPECT_EQ(v[1], 20);
  EXPECT_EQ(v(0), 10);
  EXPECT_THROW((void) v[2], std::out_of_range);

  const SmallVector<int, 4> &cv = v;
  EXPECT_EQ(cv[0], 10);
  EXPECT_THROW((void) cv[5], std::out_of_range);

  SmallVector<int, 4> e;
  EXPECT_THROW((void) e.get_first(), std::underflow_error);
  EXPECT_THROW((void) e.get_last(), std::underflow_error);
  EXPECT_THROW((void) e.remove_last(), std::underflow_error);
  EXPECT_THROW(e.pop_back(), std::underflow_error);
}

TEST(SmallVector, IterationAndRangeFor)
{
  SmallVector<int, 3> v = {1, 2, 3, 4};  // heap mode
  int sum = 0;
  for (int x : v)
    sum += x;
  EXPECT_EQ(sum, 10);

  for (int &x : v)
    x *= 2;
  EXPECT_EQ(v[3], 8);

  EXPECT_EQ(static_cast<size_t>(v.end() - v.begin()), v.size());
}

TEST(SmallVector, AlephConventionsEmptyClearTraverse)
{
  SmallVector<int, 4> v = {1, 2, 3};

  int sum = 0;
  EXPECT_TRUE(v.traverse([&sum] (int &x) { sum += x; return true; }));
  EXPECT_EQ(sum, 6);

  int visited = 0;
  const SmallVector<int, 4> &cv = v;
  EXPECT_FALSE(cv.traverse([&visited] (const int &x)
  {
    ++visited;
    return x < 2;
  }));
  EXPECT_EQ(visited, 2);

  v.empty();  // Aleph convention: empty() clears
  EXPECT_TRUE(v.is_empty());
  EXPECT_EQ(v.size(), 0u);
}

TEST(SmallVector, ReserveGrowsAndKeepsElements)
{
  SmallVector<int, 2> v = {7, 8};
  EXPECT_TRUE(v.is_small());
  v.reserve(100);
  EXPECT_FALSE(v.is_small());
  EXPECT_GE(v.capacity(), 100u);
  EXPECT_EQ(v[0], 7);
  EXPECT_EQ(v[1], 8);

  const int *p = v.data();
  for (int i = 0; i < 90; ++i)
    v.append(i);
  EXPECT_EQ(v.data(), p);  // no reallocation within reserved capacity
}

TEST(SmallVector, RangeConstructorAndFillConstructor)
{
  const std::vector<int> src = {5, 6, 7, 8, 9};
  SmallVector<int, 2> v(src.begin(), src.end());
  ASSERT_EQ(v.size(), 5u);
  EXPECT_EQ(v[4], 9);

  SmallVector<std::string, 4> f(3, "ho");
  ASSERT_EQ(f.size(), 3u);
  EXPECT_EQ(f[2], "ho");
  EXPECT_TRUE(f.is_small());
}

TEST(SmallVector, EqualityAcrossDifferentInlineCapacities)
{
  SmallVector<int, 2> a = {1, 2, 3};
  SmallVector<int, 8> b = {1, 2, 3};
  EXPECT_TRUE(a == b);
  b.append(4);
  EXPECT_TRUE(a != b);
}

TEST(SmallVector, SwapInAllModeCombinations)
{
  // heap <-> heap: O(1) pointer swap.
  SmallVector<int, 2> h1 = {1, 2, 3};
  SmallVector<int, 2> h2 = {9, 8, 7, 6};
  const int *p1 = h1.data();
  const int *p2 = h2.data();
  h1.swap(h2);
  EXPECT_EQ(h1.data(), p2);
  EXPECT_EQ(h2.data(), p1);
  EXPECT_EQ(h1.size(), 4u);
  EXPECT_EQ(h2.size(), 3u);

  // inline <-> heap.
  SmallVector<int, 4> s1 = {1};
  SmallVector<int, 4> s2 = {9, 8, 7, 6, 5};
  s1.swap(s2);
  EXPECT_EQ(s1.size(), 5u);
  EXPECT_EQ(s1[0], 9);
  EXPECT_EQ(s2.size(), 1u);
  EXPECT_EQ(s2[0], 1);
}
