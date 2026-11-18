
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
 * @file hash_function_allocation_test.cc
 * @brief set_hash_fct() of a hash container with keys when an allocation
 *        fails: the container keeps its function and its keys.
 *
 * The chained and the open tables place their keys again with the new
 * function, in lists or buckets that they allocate first; the maps of
 * tpl_dynSetHash.H also build the function of their table. When one of
 * those allocations fails, set_hash_fct() throws std::bad_alloc and the
 * container must go on finding its keys by its old function.
 *
 * This program replaces the global operator new, so that a test can make
 * an allocation fail.
 */

# include <gtest/gtest.h>

# include "allocation_failure.H"

# include <cstddef>
# include <new>
# include <string>
# include <utility>

# include <tpl_dynMapOhash.H>
# include <tpl_dynSetHash.H>
# include <tpl_lhash.H>
# include <tpl_odhash.H>
# include <tpl_olhash.H>

using namespace Aleph;

namespace
{
  constexpr int Keys = 300;

  size_t new_hash(const int & k) noexcept
  {
    return static_cast<size_t>(k) * 40503u + 17;
  }

  size_t new_pair_hash(const std::pair<int, int> & p) noexcept
  {
    return new_hash(p.first);
  }

  // Makes each allocation of set(c) fail in turn, on a container with Keys
  // keys, until set(c) completes: after each failure, which must be a
  // std::bad_alloc, holds(c, k) must still find every key. Returns how many
  // allocations failed.
  template <class Container, class Insert, class Set, class Holds>
  size_t fail_every_allocation(Insert insert, Set set, Holds holds)
  {
    Container c;
    for (int k = 0; k < Keys; ++k)
      insert(c, k);

    size_t failures = 0;
    for (size_t n = 1; n < 1000; ++n)
      {
        SCOPED_TRACE("the allocation " + std::to_string(n) + " fails");
        bool threw = false;
        {
          const Fail_At failing(n);
          try
            {
              set(c);
            }
          catch (const std::bad_alloc &)
            {
              threw = true;
            }
        }

        for (int k = 0; k < Keys; ++k)
          if (not holds(c, k))
            {
              ADD_FAILURE() << "key " << k << " is no longer found";
              return failures;
            }

        if (not threw)
          return failures;
        ++failures;
      }

    ADD_FAILURE() << "set_hash_fct() made more than 1000 allocations";
    return failures;
  }

  TEST(HashFunctionAllocation, AChainedTableKeepsItsFunction)
  {
    if (ALEPH_ALLOCATION_FAILURE_UNDER_TSAN)
      GTEST_SKIP() << "ThreadSanitizer replaces the global operator new";

    const size_t failures = fail_every_allocation<LhashTable<int>>(
      [] (auto & c, int k) { c.insert(new LhashBucket<int>(k)); },
      [] (auto & c) { c.set_hash_fct(new_hash); },
      [] (auto & c, int k) { return c.search(k) != nullptr; });
    EXPECT_GE(failures, 1u);
  }

  TEST(HashFunctionAllocation, AnOpenTableKeepsItsFunctions)
  {
    if (ALEPH_ALLOCATION_FAILURE_UNDER_TSAN)
      GTEST_SKIP() << "ThreadSanitizer replaces the global operator new";

    auto insert = [] (auto & c, int k) { (void) c.insert(k); };
    auto holds = [] (auto & c, int k) { return c.search(k) != nullptr; };
    EXPECT_GE(fail_every_allocation<OLhashTable<int>>(
                insert, [] (auto & c) { c.set_hash_fct(new_hash); }, holds), 1u);
    EXPECT_GE(fail_every_allocation<ODhashTable<int>>(
                insert, [] (auto & c) { c.set_hash_fct(new_hash); }, holds), 1u);
    EXPECT_GE(fail_every_allocation<ODhashTable<int>>(
                insert, [] (auto & c) { c.set_second_hash_fct(new_hash); }, holds), 1u);
  }

  TEST(HashFunctionAllocation, AMapKeepsItsFunction)
  {
    if (ALEPH_ALLOCATION_FAILURE_UNDER_TSAN)
      GTEST_SKIP() << "ThreadSanitizer replaces the global operator new";

    auto insert = [] (auto & c, int k) { (void) c.insert(k, 10 * k); };
    auto holds = [] (auto & c, int k)
    {
      const auto * p = c.search(k);
      return p != nullptr and p->second == 10 * k;
    };
    using Map = DynMapHash<int, int>;
    using Open_Map = MapOLhash<int, int>;
    EXPECT_GE(fail_every_allocation<Map>(
                insert, [] (auto & c) { c.set_hash_fct(new_hash); }, holds), 1u);
    EXPECT_GE(fail_every_allocation<Map>(
                insert, [] (auto & c) { c.set_hash_fct(new_pair_hash); }, holds), 1u);
    EXPECT_GE(fail_every_allocation<Open_Map>(
                insert, [] (auto & c) { c.set_hash_fct(new_pair_hash); }, holds), 1u);
  }
} // namespace
