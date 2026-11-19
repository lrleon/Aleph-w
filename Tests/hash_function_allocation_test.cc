
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
# include <gtest/gtest-spi.h>

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
  // std::bad_alloc that reaches the caller, holds(c, k) must still find
  // every key. Returns how many allocations failed.
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
          {
            // an allocation failed and set(c) completed: it caught it
            if (refused_allocations > 0)
              ADD_FAILURE() << "an allocation failed, but no std::bad_alloc reached the caller";
            return failures;
          }
        ++failures;
      }

    ADD_FAILURE() << "set_hash_fct() made more than 1000 allocations";
    return failures;
  }

  // Volatile, so that the compiler keeps the allocations of
  // allocate_and_free(), which it could otherwise leave out
  int * volatile allocated = nullptr;

  void allocate_and_free()
  {
    allocated = new int[4];
    delete [] allocated;
  }

  // The harness: an operation that lets the failure of its first
  // allocation through, and catches that of the second one and goes on,
  // completes when the second one fails; that is a failure that did not
  // reach the caller, not the end of the allocations to fail
  TEST(HashFunctionAllocation, AnAllocationThatTheOperationCatchesFailsTheTest)
  {
    if (ALEPH_ALLOCATION_FAILURE_UNDER_TSAN)
      GTEST_SKIP() << "ThreadSanitizer replaces the global operator new";

    EXPECT_NONFATAL_FAILURE(fail_every_allocation<LhashTable<int>>(
                              [] (auto & c, int k) { c.insert(new LhashBucket<int>(k)); },
                              [] (auto &)
                              {
                                allocate_and_free();
                                try
                                  {
                                    allocate_and_free();
                                  }
                                catch (const std::bad_alloc &)
                                  {
                                    // caught: the operation goes on
                                  }
                              },
                              [] (auto & c, int k) { return c.search(k) != nullptr; }),
                            "no std::bad_alloc reached the caller");
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
  // A chained table that resizes itself, with a load factor at its upper
  // threshold or above, as one filled while it did not resize itself:
  // placing its keys again, with set_hash_fct(), with resize() or with
  // the growth of an insertion, grew it again in the middle, and a failure
  // of that allocation left keys out of the table and lost its lists. The
  // capacity that it grows to is allocated at once now
  TEST(HashFunctionAllocation, AChainedTableAboveItsLoadKeepsItsKeys)
  {
    if (ALEPH_ALLOCATION_FAILURE_UNDER_TSAN)
      GTEST_SKIP() << "ThreadSanitizer replaces the global operator new";

    // Keys keys in the 101 lists of a table built by default: a load of 3
    auto filled = [] (auto & c, int k, auto add)
    {
      c.set_with_resize(false);
      add(c, k);
      c.set_with_resize(true);
    };
    auto insert = [&filled] (auto & c, int k)
    {
      filled(c, k, [] (auto & t, int key) { t.insert(new LhashBucket<int>(key)); });
    };
    auto holds = [] (auto & c, int k) { return c.search(k) != nullptr; };
    using Table = LhashTable<int>;
    EXPECT_GE(fail_every_allocation<Table>(
                insert, [] (auto & c) { c.set_hash_fct(new_hash); }, holds), 1u);
    EXPECT_GE(fail_every_allocation<Table>(
                insert, [] (auto & c) { (void) c.resize(53); }, holds), 1u);
    int next = Keys;  // a key that is not in the table, for each try
    EXPECT_GE(fail_every_allocation<Table>(
                insert, [&next] (auto & c)
                {
                  auto * bucket = new LhashBucket<int>(next);
                  ++next;
                  if (c.insert(bucket) == nullptr)
                    delete bucket;
                }, holds), 1u);

    auto set_insert = [&filled] (auto & c, int k)
    {
      filled(c, k, [] (auto & s, int key) { (void) s.insert(key); });
    };
    EXPECT_GE(fail_every_allocation<DynSetHash<int>>(
                set_insert, [] (auto & c) { c.set_hash_fct(new_hash); }, holds), 1u);

    auto map_insert = [&filled] (auto & c, int k)
    {
      filled(c, k, [] (auto & m, int key) { (void) m.insert(key, 10 * key); });
    };
    using Map = DynMapHash<int, int>;
    EXPECT_GE(fail_every_allocation<Map>(
                map_insert, [] (auto & c) { c.set_hash_fct(new_hash); }, holds), 1u);
  }

  // The case of the review: 190 keys in 211 lists, the last one left there
  // by the growth of its insertion, which failed, and set_hash_fct() with
  // each of its allocations failing in turn
  TEST(HashFunctionAllocation, SetHashFctAfterAFailedGrowthKeepsItsKeys)
  {
    if (ALEPH_ALLOCATION_FAILURE_UNDER_TSAN)
      GTEST_SKIP() << "ThreadSanitizer replaces the global operator new";

    LhashTable<int> table(211);
    const size_t len = table.capacity();
    int keys = 0;
    for (bool failed = false; not failed; ++keys)
      {
        auto * bucket = new LhashBucket<int>(keys);
        const bool grows = (keys + 1.0) / len >= table.get_upper_alpha();
        const Fail_At failing(grows ? 1 : 1000000);
        try
          {
            ASSERT_NE(table.insert(bucket), nullptr);
          }
        catch (const std::bad_alloc &)
          {
            failed = true;  // the bucket stays in the table
          }
        ASSERT_EQ(table.capacity(), len);
      }
    ASSERT_GE(table.current_alpha(), table.get_upper_alpha());

    size_t failures = 0;
    for (size_t n = 1; n < 100; ++n)
      {
        SCOPED_TRACE("the allocation " + std::to_string(n) + " fails");
        bool threw = false;
        {
          const Fail_At failing(n);
          try
            {
              table.set_hash_fct(new_hash);
            }
          catch (const std::bad_alloc &)
            {
              threw = true;
            }
        }
        EXPECT_EQ(table.size(), static_cast<size_t>(keys));
        for (int k = 0; k < keys; ++k)
          ASSERT_NE(table.search(k), nullptr) << "key " << k;
        if (not threw)
          break;
        ++failures;
      }
    EXPECT_EQ(failures, 1u) << "set_hash_fct() made more than the allocation of its lists";
  }

} // namespace
