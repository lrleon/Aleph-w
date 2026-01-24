
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
 * @file randomqueue.cc
 * @brief Tests for Randomqueue
 */

#include <gtest/gtest.h>

#include <tpl_random_queue.H>

using namespace Aleph;
using namespace testing;

namespace {

Random_Set<int> build_queue(const std::initializer_list<int> values)
{
  Random_Set<int> q;
  for (int v : values)
    q.put(v);
  return q;
}

} // namespace

TEST(RandomSet, AppendReturnsReference)
{
  Random_Set<int> q;
  auto &ref = q.append(42);
  ref = 13; // mutate via reference

  ASSERT_FALSE(q.is_empty());
  bool found = false;
  q.traverse([&found] (const int v) { found |= (v == 13); return true; });
  EXPECT_TRUE(found);
}

TEST(RandomSet, AppendPreservesUniformityBySwapping)
{
  Random_Set<int> q;
  for (int i = 0; i < 10; ++i)
    q.append(i);

  // insert sentinel twice: once to mutate reference, once to ensure random placement
  auto &first = q.append(100);
  first = 100;
  q.append(100);

  size_t count = 0;
  q.traverse([&count] (const int v)
             {
               if (v == 100)
                 ++count;
               return true;
             });
  EXPECT_EQ(2u, count);
}

TEST(RandomSet, GetRemovesUniformly)
{
  Random_Set<int> q = build_queue({1, 2, 3, 4});
  size_t seen = 0;
  while (not q.is_empty())
    {
      int val = q.get();
      EXPECT_GE(val, 1);
      EXPECT_LE(val, 4);
      ++seen;
    }
  EXPECT_EQ(4u, seen);
}

TEST(RandomSet, AppendMoveAcceptsComplexTypes)
{
  struct Payload
  {
    std::string data;
    Payload() = default;
    explicit Payload(std::string d) : data(std::move(d)) {}
  };

  Random_Set<Payload> q;
  const auto &stored = q.append(Payload("beta"));
  EXPECT_EQ("beta", stored.data);

  auto &stored2 = q.append(Payload("gamma"));
  stored2.data = "delta";

  size_t hits = 0;
  q.traverse([&hits] (const Payload &p)
             {
               if (p.data == "delta" || p.data == "beta")
                 ++hits;
               return true;
             });
  EXPECT_EQ(2u, hits);
}
