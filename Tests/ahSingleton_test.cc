
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
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
