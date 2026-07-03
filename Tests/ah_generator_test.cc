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
 * @file ah_generator_test.cc
 * @brief Tests for Aleph::Generator<T> (ah-generator.H).
 *
 * Covers basic yielding and range-for iteration, lvalue/rvalue/temporary
 * yield lifetime, partial consumption (early break / early destruction),
 * exception propagation, move-only element types, move semantics of the
 * generator itself, an empty (never-yields) generator, and a generator
 * that yields another generator's output (composition).
 */

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <ah-generator.H>

using Aleph::Generator;

namespace
{
Generator<int> countdown(int n)
{
  for (; n > 0; --n)
    co_yield n;
}

Generator<int> empty_generator()
{
  if (false)
    co_yield 0;  // never reached; keeps this a coroutine
}

Generator<std::string> mixed_value_categories()
{
  co_yield "a";                        // const char* -> std::string prvalue
  co_yield std::string("b") + "b";     // rvalue temporary
  std::string s = "c";
  co_yield s;                          // lvalue
}

Generator<int> throwing_after(int n)
{
  for (int i = 0; i < n; ++i)
    co_yield i;
  throw std::runtime_error("generator failure");
}

// Instrumented element type: counts live instances so tests can prove no
// leaks/double-destroys happen across suspension points.
struct Probe
{
  static int live;
  int value = 0;

  explicit Probe(int v) : value(v) { ++live; }
  Probe(const Probe &p) : value(p.value) { ++live; }
  Probe(Probe &&p) noexcept : value(p.value) { ++live; }
  Probe &operator=(const Probe &) = default;
  Probe &operator=(Probe &&) noexcept = default;
  ~Probe() { --live; }
};

int Probe::live = 0;

Generator<Probe> probes(int n)
{
  for (int i = 0; i < n; ++i)
    co_yield Probe(i);
}

Generator<std::unique_ptr<int>> unique_ptrs(int n)
{
  for (int i = 0; i < n; ++i)
    co_yield std::make_unique<int>(i);
}

// A generator whose body loops over another generator, forwarding values —
// exercises composition and nested coroutine-frame lifetime.
Generator<int> doubled(Generator<int> src)
{
  for (int x : src)
    co_yield x * 2;
}
}  // namespace

TEST(Generator, BasicYieldAndRangeFor)
{
  int sum = 0;
  int count = 0;
  for (int i : countdown(5))
    {
      sum += i;
      ++count;
    }
  EXPECT_EQ(sum, 15);
  EXPECT_EQ(count, 5);
}

TEST(Generator, IterationOrderIsDescending)
{
  std::vector<int> seen;
  for (int i : countdown(4))
    seen.push_back(i);
  EXPECT_EQ(seen, (std::vector<int>{4, 3, 2, 1}));
}

TEST(Generator, EmptyGeneratorYieldsNothing)
{
  int count = 0;
  for (int i : empty_generator())
    { (void) i; ++count; }
  EXPECT_EQ(count, 0);
}

TEST(Generator, MixedValueCategoriesPreserveContent)
{
  std::vector<std::string> seen;
  for (const std::string &s : mixed_value_categories())
    seen.push_back(s);
  EXPECT_EQ(seen, (std::vector<std::string>{"a", "bb", "c"}));
}

TEST(Generator, PartialConsumptionStopsEarly)
{
  int first = -1;
  int count = 0;
  for (int i : countdown(1000000))
    {
      if (count == 0)
        first = i;
      if (++count == 3)
        break;
    }
  EXPECT_EQ(first, 1000000);
  EXPECT_EQ(count, 3);
}

TEST(Generator, ManualIteratorProtocol)
{
  Generator<int> g = countdown(3);
  auto it = g.begin();
  ASSERT_NE(it, g.end());
  EXPECT_EQ(*it, 3);
  ++it;
  ASSERT_NE(it, g.end());
  EXPECT_EQ(*it, 2);
  ++it;
  ASSERT_NE(it, g.end());
  EXPECT_EQ(*it, 1);
  ++it;
  EXPECT_EQ(it, g.end());
}

TEST(Generator, ExceptionPropagatesFromResume)
{
  std::vector<int> seen;
  EXPECT_THROW(
    {
      for (int i : throwing_after(3))
        seen.push_back(i);
    },
    std::runtime_error);
  EXPECT_EQ(seen, (std::vector<int>{0, 1, 2}));
}

TEST(Generator, ExceptionMessageIsPreserved)
{
  try
    {
      for (int i : throwing_after(0))
        (void) i;
      FAIL() << "expected std::runtime_error";
    }
  catch (const std::runtime_error &e)
    {
      EXPECT_STREQ(e.what(), "generator failure");
    }
}

TEST(Generator, ElementLifetimesAreBalancedAcrossSuspension)
{
  ASSERT_EQ(Probe::live, 0);
  {
    int sum = 0;
    for (const Probe &p : probes(5))
      {
        // Exactly one live Probe at a time: the one currently yielded.
        EXPECT_EQ(Probe::live, 1);
        sum += p.value;
      }
    EXPECT_EQ(sum, 0 + 1 + 2 + 3 + 4);
  }
  EXPECT_EQ(Probe::live, 0);
}

TEST(Generator, ElementLifetimesAreBalancedOnEarlyDestruction)
{
  ASSERT_EQ(Probe::live, 0);
  {
    Generator<Probe> g = probes(1000);
    auto it = g.begin();
    EXPECT_EQ(Probe::live, 1);
    (void) *it;
    // g goes out of scope here, still holding one live Probe: the
    // coroutine frame (and the Probe within it) must be destroyed.
  }
  EXPECT_EQ(Probe::live, 0);
}

TEST(Generator, MoveOnlyElementsAreSupported)
{
  std::vector<int> seen;
  for (auto &p : unique_ptrs(4))
    seen.push_back(*p);
  EXPECT_EQ(seen, (std::vector<int>{0, 1, 2, 3}));
}

TEST(Generator, GeneratorItselfIsMoveOnly)
{
  static_assert(not std::is_copy_constructible_v<Generator<int>>);
  static_assert(std::is_move_constructible_v<Generator<int>>);

  Generator<int> g = countdown(2);
  Generator<int> moved = std::move(g);
  std::vector<int> seen;
  for (int i : moved)
    seen.push_back(i);
  EXPECT_EQ(seen, (std::vector<int>{2, 1}));
}

TEST(Generator, CompositionForwardsThroughNestedFrame)
{
  std::vector<int> seen;
  for (int i : doubled(countdown(3)))
    seen.push_back(i);
  EXPECT_EQ(seen, (std::vector<int>{6, 4, 2}));
}

TEST(Generator, MultipleIndependentInstancesDoNotInterfere)
{
  Generator<int> a = countdown(3);
  Generator<int> b = countdown(5);

  auto ita = a.begin();
  auto itb = b.begin();
  EXPECT_EQ(*ita, 3);
  EXPECT_EQ(*itb, 5);
  ++ita;
  EXPECT_EQ(*ita, 2);
  EXPECT_EQ(*itb, 5);  // b unaffected by advancing a
}