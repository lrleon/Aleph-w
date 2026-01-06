/* Aleph-w

   / \  | | ___ _ __ | |__      __      __
  / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
 / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
/_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
              |_|

This file is part of Aleph-w library

Copyright (c) 2002-2018 Leandro Rabindranath Leon

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
 * @file ah_init_guard.cc
 * @brief Tests for Ah Init Guard
 */

/**
 * @file ah_init_guard.cc
 * @brief Comprehensive test suite for ah_init_guard.H
 *
 * Tests the RAII Init_Guard class for exception-safe cleanup.
 */

#include <gtest/gtest.h>
#include <stdexcept>
#include <functional>
#include <vector>

#include <ah_init_guard.H>

using namespace Aleph;

// ============================================================================
// Test Suite: Basic Functionality
// ============================================================================

TEST(InitGuard, CleanupCalledOnDestruction)
{
  bool cleanup_called = false;
  
  {
    Init_Guard guard([&cleanup_called]() { cleanup_called = true; });
    EXPECT_FALSE(cleanup_called);
  }
  
  EXPECT_TRUE(cleanup_called);
}

TEST(InitGuard, CleanupNotCalledIfReleased)
{
  bool cleanup_called = false;
  
  {
    Init_Guard guard([&cleanup_called]() { cleanup_called = true; });
    guard.release();
    EXPECT_FALSE(cleanup_called);
  }
  
  EXPECT_FALSE(cleanup_called);
}

TEST(InitGuard, IsReleasedGetter)
{
  bool dummy = false;
  Init_Guard guard([&dummy]() { dummy = true; });
  
  EXPECT_FALSE(guard.is_released());
  guard.release();
  EXPECT_TRUE(guard.is_released());
}

TEST(InitGuard, MultipleReleaseCalls)
{
  bool cleanup_called = false;
  
  {
    Init_Guard guard([&cleanup_called]() { cleanup_called = true; });
    guard.release();
    guard.release();  // Should be safe to call multiple times
    guard.release();
    EXPECT_TRUE(guard.is_released());
  }
  
  EXPECT_FALSE(cleanup_called);
}

// ============================================================================
// Test Suite: Exception Safety
// ============================================================================

TEST(InitGuard, CleanupCalledOnException)
{
  bool cleanup_called = false;
  
  try
    {
      Init_Guard guard([&cleanup_called]() { cleanup_called = true; });
      throw std::runtime_error("test exception");
    }
  catch (const std::runtime_error&)
    {
      // Expected
    }
  
  EXPECT_TRUE(cleanup_called);
}

TEST(InitGuard, CleanupNotCalledIfReleasedBeforeException)
{
  bool cleanup_called = false;
  
  try
    {
      Init_Guard guard([&cleanup_called]() { cleanup_called = true; });
      guard.release();
      throw std::runtime_error("test exception");
    }
  catch (const std::runtime_error&)
    {
      // Expected
    }
  
  EXPECT_FALSE(cleanup_called);
}

// ============================================================================
// Test Suite: Move Semantics
// ============================================================================

TEST(InitGuard, MoveConstructor)
{
  bool cleanup_called = false;
  
  {
    Init_Guard guard1([&cleanup_called]() { cleanup_called = true; });
    
    // Move to guard2
    Init_Guard guard2(std::move(guard1));
    
    // guard1 should be released after move
    EXPECT_TRUE(guard1.is_released());
    EXPECT_FALSE(guard2.is_released());
    EXPECT_FALSE(cleanup_called);
  }
  
  // Cleanup should be called when guard2 is destroyed
  EXPECT_TRUE(cleanup_called);
}

TEST(InitGuard, MoveConstructorFromReleased)
{
  bool cleanup_called = false;
  
  {
    Init_Guard guard1([&cleanup_called]() { cleanup_called = true; });
    guard1.release();
    
    Init_Guard guard2(std::move(guard1));
    
    // Both should be released
    EXPECT_TRUE(guard1.is_released());
    EXPECT_TRUE(guard2.is_released());
  }
  
  EXPECT_FALSE(cleanup_called);
}

TEST(InitGuard, MoveAssignment)
{
  int cleanup_count = 0;
  
  // Use std::function because lambdas have deleted assignment operator
  using CleanupType = std::function<void()>;
  
  {
    Init_Guard<CleanupType> guard1([&cleanup_count]() { cleanup_count++; });
    Init_Guard<CleanupType> guard2([&cleanup_count]() { cleanup_count++; });
    
    // Move assign guard1 to guard2
    // This should trigger cleanup of guard2's original function
    guard2 = std::move(guard1);
    
    EXPECT_EQ(cleanup_count, 1);  // guard2's original cleanup was called
    
    EXPECT_TRUE(guard1.is_released());
    EXPECT_FALSE(guard2.is_released());
  }
  
  // guard2's cleanup (originally from guard1) should be called
  EXPECT_EQ(cleanup_count, 2);
}

TEST(InitGuard, MoveAssignmentToReleased)
{
  int cleanup_count = 0;
  
  // Use std::function because lambdas have deleted assignment operator
  using CleanupType = std::function<void()>;
  
  {
    Init_Guard<CleanupType> guard1([&cleanup_count]() { cleanup_count++; });
    Init_Guard<CleanupType> guard2([&cleanup_count]() { cleanup_count++; });
    
    guard2.release();  // Release guard2 first
    
    // Move assign guard1 to guard2
    // Since guard2 was released, its cleanup should NOT be called
    guard2 = std::move(guard1);
    
    EXPECT_EQ(cleanup_count, 0);  // No cleanup yet
    
    EXPECT_TRUE(guard1.is_released());
    EXPECT_FALSE(guard2.is_released());
  }
  
  // Only one cleanup (from guard1 via guard2)
  EXPECT_EQ(cleanup_count, 1);
}

TEST(InitGuard, SelfMoveAssignment)
{
  bool cleanup_called = false;
  
  // Use std::function because lambdas have deleted assignment operator
  std::function<void()> cleanup_func = [&cleanup_called]() { cleanup_called = true; };
  
  {
    Init_Guard<std::function<void()>> guard(cleanup_func);
    
    // Self-assignment should be safe and do nothing
    // Use a reference to avoid self-move warning
    Init_Guard<std::function<void()>>& ref = guard;
    guard = std::move(ref);
    
    EXPECT_FALSE(cleanup_called);
    EXPECT_FALSE(guard.is_released());
  }
  
  EXPECT_TRUE(cleanup_called);
}

// ============================================================================
// Test Suite: Factory Function
// ============================================================================

TEST(InitGuard, MakeInitGuard)
{
  bool cleanup_called = false;
  
  {
    auto guard = make_init_guard([&cleanup_called]() { cleanup_called = true; });
    EXPECT_FALSE(cleanup_called);
    EXPECT_FALSE(guard.is_released());
  }
  
  EXPECT_TRUE(cleanup_called);
}

TEST(InitGuard, MakeInitGuardWithRelease)
{
  bool cleanup_called = false;
  
  {
    auto guard = make_init_guard([&cleanup_called]() { cleanup_called = true; });
    guard.release();
  }
  
  EXPECT_FALSE(cleanup_called);
}

// ============================================================================
// Test Suite: Complex Cleanup
// ============================================================================

TEST(InitGuard, ModifiesExternalState)
{
  int counter = 0;
  
  {
    Init_Guard guard([&counter]() { counter = 42; });
    EXPECT_EQ(counter, 0);
  }
  
  EXPECT_EQ(counter, 42);
}

TEST(InitGuard, MultipleResourceCleanup)
{
  bool resource1_cleaned = false;
  bool resource2_cleaned = false;
  bool resource3_cleaned = false;
  
  {
    Init_Guard guard1([&resource1_cleaned]() { resource1_cleaned = true; });
    Init_Guard guard2([&resource2_cleaned]() { resource2_cleaned = true; });
    Init_Guard guard3([&resource3_cleaned]() { resource3_cleaned = true; });
    
    // Release only guard2
    guard2.release();
    
    EXPECT_FALSE(resource1_cleaned);
    EXPECT_FALSE(resource2_cleaned);
    EXPECT_FALSE(resource3_cleaned);
  }
  
  EXPECT_TRUE(resource1_cleaned);
  EXPECT_FALSE(resource2_cleaned);  // Was released
  EXPECT_TRUE(resource3_cleaned);
}

TEST(InitGuard, CleanupOrder)
{
  std::vector<int> cleanup_order;
  
  {
    Init_Guard guard1([&cleanup_order]() { cleanup_order.push_back(1); });
    Init_Guard guard2([&cleanup_order]() { cleanup_order.push_back(2); });
    Init_Guard guard3([&cleanup_order]() { cleanup_order.push_back(3); });
  }
  
  // Destructors are called in reverse order of construction (LIFO)
  ASSERT_EQ(cleanup_order.size(), 3);
  EXPECT_EQ(cleanup_order[0], 3);
  EXPECT_EQ(cleanup_order[1], 2);
  EXPECT_EQ(cleanup_order[2], 1);
}

// ============================================================================
// Test Suite: Callable Types
// ============================================================================

void free_function_cleanup(bool& flag)
{
  flag = true;
}

TEST(InitGuard, WithLambda)
{
  bool called = false;
  {
    Init_Guard guard([&called]() { called = true; });
  }
  EXPECT_TRUE(called);
}

TEST(InitGuard, WithFunctionObject)
{
  struct Cleanup
  {
    bool& flag;
    explicit Cleanup(bool& f) : flag(f) {}
    void operator()() { flag = true; }
  };
  
  bool called = false;
  {
    auto guard = Init_Guard(Cleanup(called));
  }
  EXPECT_TRUE(called);
}

TEST(InitGuard, WithStdFunction)
{
  bool called = false;
  std::function<void()> cleanup = [&called]() { called = true; };
  
  {
    Init_Guard guard(cleanup);
  }
  EXPECT_TRUE(called);
}

// ============================================================================
// Test Suite: Edge Cases
// ============================================================================

TEST(InitGuard, EmptyLambda)
{
  // Should compile and work even with empty lambda
  {
    Init_Guard guard([]() {});
  }
  // Just verify it doesn't crash
  SUCCEED();
}

TEST(InitGuard, NestedGuards)
{
  std::vector<int> log;
  
  {
    Init_Guard outer([&log]() { log.push_back(1); });
    {
      Init_Guard inner([&log]() { log.push_back(2); });
    }
    log.push_back(3);  // After inner cleanup
  }
  log.push_back(4);  // After outer cleanup
  
  ASSERT_EQ(log.size(), 4);
  EXPECT_EQ(log[0], 2);  // Inner cleanup
  EXPECT_EQ(log[1], 3);  // Between cleanups
  EXPECT_EQ(log[2], 1);  // Outer cleanup
  EXPECT_EQ(log[3], 4);  // After all
}

