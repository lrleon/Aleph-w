
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
 * @file ahSingleton_test.cc
 * @brief Unit tests for ahSingleton.H
 *
 * Tests cover:
 * - Single instance guarantee (same address on multiple calls)
 * - Macro-based singleton functionality
 * - Mutability of singleton state
 * - Copy/move operations are deleted
 * - Thread-safe initialization (concurrent access)
 * - noexcept guarantee
 */

#include <gtest/gtest.h>

#include <ahSingleton.H>

#include <atomic>
#include <thread>
#include <type_traits>
#include <vector>

namespace
{
class MacroSingletonMutable
{
  Make_Singleton(MacroSingletonMutable)

private:
  MacroSingletonMutable() = default;

public:
  int value = 0;
};

class MacroSingletonCtorCount
{
  Make_Singleton(MacroSingletonCtorCount)

private:
  MacroSingletonCtorCount() { ++ctor_count; }

public:
  static inline std::atomic<int> ctor_count{0};
};
} // namespace

TEST(AhSingleton, SingletonGetInstanceReturnsSameObject)
{
  Singleton & a = Singleton::get_instance();
  Singleton & b = Singleton::get_instance();
  EXPECT_EQ(&a, &b);
}

TEST(AhSingleton, MakeSingletonReturnsSameObject)
{
  auto & a = MacroSingletonMutable::get_instance();
  auto & b = MacroSingletonMutable::get_instance();
  EXPECT_EQ(&a, &b);
}

TEST(AhSingleton, MakeSingletonReturnsNonConstReferenceAndIsMutable)
{
  auto & a = MacroSingletonMutable::get_instance();
  a.value = 123;
  EXPECT_EQ(MacroSingletonMutable::get_instance().value, 123);
}

TEST(AhSingleton, CopyAndMoveAreDisabled)
{
  static_assert(!std::is_copy_constructible_v<Singleton>);
  static_assert(!std::is_copy_assignable_v<Singleton>);
  static_assert(!std::is_move_constructible_v<Singleton>);
  static_assert(!std::is_move_assignable_v<Singleton>);

  static_assert(!std::is_copy_constructible_v<MacroSingletonMutable>);
  static_assert(!std::is_copy_assignable_v<MacroSingletonMutable>);
  static_assert(!std::is_move_constructible_v<MacroSingletonMutable>);
  static_assert(!std::is_move_assignable_v<MacroSingletonMutable>);
}

TEST(AhSingleton, GetInstanceIsNoexcept)
{
  // Verify that get_instance() is noexcept for both Singleton class and macro
  static_assert(noexcept(Singleton::get_instance()));
  static_assert(noexcept(MacroSingletonMutable::get_instance()));
}

TEST(AhSingleton, ThreadSafeInitializationMeyersSingleton)
{
  MacroSingletonCtorCount::ctor_count.store(0);

  constexpr int kThreads = 16;
  constexpr int kIters = 5000;

  std::vector<std::thread> threads;
  threads.reserve(kThreads);

  std::atomic<void *> first_addr{nullptr};
  std::atomic<bool> mismatch{false};

  for (int t = 0; t < kThreads; ++t)
    {
      threads.emplace_back([&]() {
        for (int i = 0; i < kIters; ++i)
          {
            auto * addr = (void *) &MacroSingletonCtorCount::get_instance();
            void * expected = nullptr;
            if (first_addr.compare_exchange_strong(expected, addr))
              continue;

            if (first_addr.load() != addr)
              mismatch.store(true);
          }
      });
    }

  for (auto & th : threads)
    th.join();

  EXPECT_FALSE(mismatch.load());
  EXPECT_EQ(MacroSingletonCtorCount::ctor_count.load(), 1);
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
