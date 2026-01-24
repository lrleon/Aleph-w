
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
 * @file ah_arena_test.cc
 * @brief Unit tests for AhArenaAllocator
 *
 * Tests cover:
 * - Basic allocation and deallocation
 * - External buffer vs internal allocation
 * - LIFO deallocation pattern
 * - Move semantics
 * - Typed object allocation
 * - Alignment
 * - Query methods (capacity, available, etc.)
 * - Edge cases (zero allocation, full arena, etc.)
 */

#include <gtest/gtest.h>

#include <ah-arena.H>

#include <string>
#include <type_traits>
#include <vector>
#include <cstdint>

using namespace Aleph;

// ============================================================================
// Basic Tests with External Buffer
// ============================================================================

class ExternalBufferArena : public ::testing::Test
{
protected:
  static constexpr size_t BUFFER_SIZE = 1024;
  alignas(std::max_align_t) char buffer[BUFFER_SIZE];
  AhArenaAllocator arena{buffer, BUFFER_SIZE};
};

TEST_F(ExternalBufferArena, InitialState)
{
  EXPECT_TRUE(arena.is_valid());
  EXPECT_FALSE(arena.owns_memory());
  EXPECT_EQ(arena.capacity(), BUFFER_SIZE);
  EXPECT_EQ(arena.allocated_size(), 0u);
  EXPECT_EQ(arena.available_size(), BUFFER_SIZE);
  EXPECT_TRUE(arena.empty());
  EXPECT_FALSE(arena.full());
}

TEST_F(ExternalBufferArena, SimpleAllocation)
{
  void* ptr = arena.alloc(100);

  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(ptr, buffer);
  EXPECT_EQ(arena.allocated_size(), 100u);
  EXPECT_EQ(arena.available_size(), BUFFER_SIZE - 100);
  EXPECT_FALSE(arena.empty());
}

TEST_F(ExternalBufferArena, MultipleAllocations)
{
  void* ptr1 = arena.alloc(100);
  void* ptr2 = arena.alloc(200);
  void* ptr3 = arena.alloc(50);

  ASSERT_NE(ptr1, nullptr);
  ASSERT_NE(ptr2, nullptr);
  ASSERT_NE(ptr3, nullptr);

  // Pointers should be sequential
  EXPECT_EQ(static_cast<char*>(ptr2), static_cast<char*>(ptr1) + 100);
  EXPECT_EQ(static_cast<char*>(ptr3), static_cast<char*>(ptr2) + 200);

  EXPECT_EQ(arena.allocated_size(), 350u);
}

TEST_F(ExternalBufferArena, AllocationFailsWhenFull)
{
  void* ptr1 = arena.alloc(BUFFER_SIZE - 10);
  ASSERT_NE(ptr1, nullptr);

  // This should fail (not enough space)
  void* ptr2 = arena.alloc(20);
  EXPECT_EQ(ptr2, nullptr);

  // Original allocation should still be valid
  EXPECT_EQ(arena.allocated_size(), BUFFER_SIZE - 10);
}

TEST_F(ExternalBufferArena, ExactFitAllocation)
{
  void* ptr = arena.alloc(BUFFER_SIZE);

  ASSERT_NE(ptr, nullptr);
  EXPECT_TRUE(arena.full());
  EXPECT_EQ(arena.available_size(), 0u);

  // No more allocations possible
  void* ptr2 = arena.alloc(1);
  EXPECT_EQ(ptr2, nullptr);
}

TEST_F(ExternalBufferArena, ZeroAllocationReturnsNull)
{
  void* ptr = arena.alloc(0);
  EXPECT_EQ(ptr, nullptr);
  EXPECT_TRUE(arena.empty());
}

TEST_F(ExternalBufferArena, LIFODeallocation)
{
  void* ptr1 = arena.alloc(100);
  void* ptr2 = arena.alloc(200);

  EXPECT_EQ(arena.allocated_size(), 300u);

  // Dealloc in LIFO order works
  arena.dealloc(ptr2, 200);
  EXPECT_EQ(arena.allocated_size(), 100u);

  arena.dealloc(ptr1, 100);
  EXPECT_EQ(arena.allocated_size(), 0u);
  EXPECT_TRUE(arena.empty());
}

TEST_F(ExternalBufferArena, NonLIFODeallocationIsNoop)
{
  void* ptr1 = arena.alloc(100);
  (void)arena.alloc(200);  // ptr2 not needed

  EXPECT_EQ(arena.allocated_size(), 300u);

  // Dealloc in non-LIFO order is a no-op
  arena.dealloc(ptr1, 100);
  EXPECT_EQ(arena.allocated_size(), 300u);  // No change
}

TEST_F(ExternalBufferArena, Reset)
{
  (void)arena.alloc(100);
  (void)arena.alloc(200);
  (void)arena.alloc(50);

  EXPECT_EQ(arena.allocated_size(), 350u);

  arena.reset();

  EXPECT_TRUE(arena.empty());
  EXPECT_EQ(arena.allocated_size(), 0u);
  EXPECT_EQ(arena.available_size(), BUFFER_SIZE);

  // Can allocate again after reset
  void* ptr = arena.alloc(500);
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(ptr, buffer);
}

TEST_F(ExternalBufferArena, Contains)
{
  void* ptr = arena.alloc(100);

  EXPECT_TRUE(arena.contains(ptr));
  EXPECT_TRUE(arena.contains(buffer));
  EXPECT_TRUE(arena.contains(buffer + BUFFER_SIZE - 1));
  EXPECT_FALSE(arena.contains(buffer + BUFFER_SIZE));  // One past end
  EXPECT_FALSE(arena.contains(nullptr));

  int external_var = 42;
  EXPECT_FALSE(arena.contains(&external_var));
}

// ============================================================================
// Internal Allocation Tests
// ============================================================================

TEST(InternalArena, DefaultSize)
{
  AhArenaAllocator arena;

  EXPECT_TRUE(arena.is_valid());
  EXPECT_TRUE(arena.owns_memory());
  EXPECT_EQ(arena.capacity(), AhArenaAllocator::DEFAULT_SIZE);
}

TEST(InternalArena, CustomSize)
{
  AhArenaAllocator arena(4096);

  EXPECT_TRUE(arena.is_valid());
  EXPECT_TRUE(arena.owns_memory());
  EXPECT_EQ(arena.capacity(), 4096u);
}

TEST(InternalArena, AllocationWorks)
{
  AhArenaAllocator arena(1024);

  void* ptr1 = arena.alloc(100);
  void* ptr2 = arena.alloc(200);

  ASSERT_NE(ptr1, nullptr);
  ASSERT_NE(ptr2, nullptr);
  EXPECT_EQ(arena.allocated_size(), 300u);
}

// ============================================================================
// Move Semantics Tests
// ============================================================================

TEST(ArenaMoveSemantics, MoveConstruction)
{
  AhArenaAllocator arena1(1024);
  void* ptr = arena1.alloc(100);
  ASSERT_NE(ptr, nullptr);

  AhArenaAllocator arena2(std::move(arena1));

  // arena2 should have the memory
  EXPECT_TRUE(arena2.is_valid());
  EXPECT_TRUE(arena2.owns_memory());
  EXPECT_EQ(arena2.capacity(), 1024u);
  EXPECT_EQ(arena2.allocated_size(), 100u);
  EXPECT_TRUE(arena2.contains(ptr));

  // arena1 should be invalid
  EXPECT_FALSE(arena1.is_valid());
  EXPECT_FALSE(arena1.owns_memory());
  EXPECT_EQ(arena1.capacity(), 0u);
}

TEST(ArenaMoveSemantics, MoveAssignment)
{
  AhArenaAllocator arena1(1024);
  (void)arena1.alloc(100);

  AhArenaAllocator arena2(512);
  (void)arena2.alloc(50);

  arena2 = std::move(arena1);

  // arena2 should have arena1's memory
  EXPECT_TRUE(arena2.is_valid());
  EXPECT_EQ(arena2.capacity(), 1024u);
  EXPECT_EQ(arena2.allocated_size(), 100u);

  // arena1 should be invalid
  EXPECT_FALSE(arena1.is_valid());
}

TEST(ArenaMoveSemantics, SelfMoveAssignment)
{
  AhArenaAllocator arena(1024);
  (void)arena.alloc(100);

  // Self-move assignment - silence warning as this is intentional test
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-move"
  arena = std::move(arena);
#pragma GCC diagnostic pop

  // Should still be valid (self-move is a no-op)
  EXPECT_TRUE(arena.is_valid());
  EXPECT_EQ(arena.allocated_size(), 100u);
}

// ============================================================================
// Typed Allocation Tests
// ============================================================================

namespace
{
struct TestObject
{
  int value;
  std::string name;
  static int construct_count;
  static int destruct_count;

  TestObject(int v, const std::string& n) : value(v), name(n)
  {
    ++construct_count;
  }

  ~TestObject()
  {
    ++destruct_count;
  }
};

int TestObject::construct_count = 0;
int TestObject::destruct_count = 0;

class TypedAllocationTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    TestObject::construct_count = 0;
    TestObject::destruct_count = 0;
  }
};
} // namespace

TEST_F(TypedAllocationTest, AllocateAndDeallocate)
{
  AhArenaAllocator arena(4096);

  TestObject* obj = allocate<TestObject>(arena, 42, "hello");

  ASSERT_NE(obj, nullptr);
  EXPECT_EQ(obj->value, 42);
  EXPECT_EQ(obj->name, "hello");
  EXPECT_EQ(TestObject::construct_count, 1);

  deallocate(arena, obj);

  EXPECT_EQ(TestObject::destruct_count, 1);
}

TEST_F(TypedAllocationTest, MultipleObjects)
{
  AhArenaAllocator arena(4096);

  TestObject* obj1 = allocate<TestObject>(arena, 1, "one");
  TestObject* obj2 = allocate<TestObject>(arena, 2, "two");
  TestObject* obj3 = allocate<TestObject>(arena, 3, "three");

  EXPECT_EQ(TestObject::construct_count, 3);

  // Deallocate in LIFO order
  deallocate(arena, obj3);
  deallocate(arena, obj2);
  deallocate(arena, obj1);

  EXPECT_EQ(TestObject::destruct_count, 3);
  EXPECT_TRUE(arena.empty());
}

TEST_F(TypedAllocationTest, ResetDoesNotCallDestructors)
{
  AhArenaAllocator arena(4096);

  (void)allocate<TestObject>(arena, 1, "one");
  (void)allocate<TestObject>(arena, 2, "two");

  EXPECT_EQ(TestObject::construct_count, 2);

  // Reset does NOT call destructors
  arena.reset();

  EXPECT_EQ(TestObject::destruct_count, 0);
  EXPECT_TRUE(arena.empty());
}

TEST_F(TypedAllocationTest, AllocationFailsReturnsNull)
{
  char buffer[sizeof(TestObject) - 1];  // Too small for TestObject
  AhArenaAllocator arena(buffer, sizeof(buffer));

  TestObject* obj = allocate<TestObject>(arena, 42, "test");

  EXPECT_EQ(obj, nullptr);
  EXPECT_EQ(TestObject::construct_count, 0);  // Constructor not called
}

// ============================================================================
// Alignment Tests
// ============================================================================

TEST(ArenaAlignment, AllocAligned)
{
  AhArenaAllocator arena(4096);

  // Allocate something small first to misalign
  (void)arena.alloc(3);

  // Now allocate with alignment
  void* ptr = arena.alloc_aligned(100, 16);

  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % 16, 0u);
}

TEST(ArenaAlignment, MultipleAlignedAllocations)
{
  AhArenaAllocator arena(4096);

  void* ptr1 = arena.alloc_aligned(10, 8);
  void* ptr2 = arena.alloc_aligned(10, 16);
  void* ptr3 = arena.alloc_aligned(10, 32);

  ASSERT_NE(ptr1, nullptr);
  ASSERT_NE(ptr2, nullptr);
  ASSERT_NE(ptr3, nullptr);

  EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr1) % 8, 0u);
  EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr2) % 16, 0u);
  EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr3) % 32, 0u);
}

TEST(ArenaAlignment, TypedAllocationRespectAlignment)
{
  struct alignas(32) AlignedStruct
  {
    char data[64];
  };

  AhArenaAllocator arena(4096);

  // Misalign first
  (void)arena.alloc(7);

  AlignedStruct* obj = allocate<AlignedStruct>(arena);

  ASSERT_NE(obj, nullptr);
  EXPECT_EQ(reinterpret_cast<uintptr_t>(obj) % 32, 0u);
}

// ============================================================================
// Type Traits Tests
// ============================================================================

TEST(ArenaTypeTraits, NonCopyable)
{
  static_assert(!std::is_copy_constructible_v<AhArenaAllocator>);
  static_assert(!std::is_copy_assignable_v<AhArenaAllocator>);
}

TEST(ArenaTypeTraits, Movable)
{
  static_assert(std::is_move_constructible_v<AhArenaAllocator>);
  static_assert(std::is_move_assignable_v<AhArenaAllocator>);
}

TEST(ArenaTypeTraits, NoexceptMoveOperations)
{
  static_assert(std::is_nothrow_move_constructible_v<AhArenaAllocator>);
  static_assert(std::is_nothrow_move_assignable_v<AhArenaAllocator>);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST(ArenaEdgeCases, VerySmallArena)
{
  char buffer[1];
  AhArenaAllocator arena(buffer, 1);

  void* ptr = arena.alloc(1);
  ASSERT_NE(ptr, nullptr);
  EXPECT_TRUE(arena.full());

  void* ptr2 = arena.alloc(1);
  EXPECT_EQ(ptr2, nullptr);
}

TEST(ArenaEdgeCases, DeallocWithZeroSize)
{
  AhArenaAllocator arena(1024);
  void* ptr = arena.alloc(100);

  arena.dealloc(ptr, 0);  // Should be no-op

  EXPECT_EQ(arena.allocated_size(), 100u);
}

TEST(ArenaEdgeCases, DeallocWithNullPtr)
{
  AhArenaAllocator arena(1024);
  (void)arena.alloc(100);

  arena.dealloc(nullptr, 100);  // Should be no-op

  EXPECT_EQ(arena.allocated_size(), 100u);
}

TEST(ArenaEdgeCases, AllocAlignedZeroSize)
{
  AhArenaAllocator arena(1024);

  void* ptr = arena.alloc_aligned(0, 16);
  EXPECT_EQ(ptr, nullptr);
}

TEST(ArenaEdgeCases, AllocAlignedZeroAlignment)
{
  AhArenaAllocator arena(1024);

  void* ptr = arena.alloc_aligned(100, 0);
  EXPECT_EQ(ptr, nullptr);
}

// ============================================================================
// Backward Compatibility
// ============================================================================

TEST(ArenaBackwardCompat, DeprecatedMethods)
{
  AhArenaAllocator arena(1024);

  // These are deprecated but should still work
  void* ptr = arena.allocate(100);
  EXPECT_NE(ptr, nullptr);

  arena.deallocate(ptr, 100);
  EXPECT_TRUE(arena.empty());
}

TEST(ArenaBackwardCompat, DealllocAlias)
{
  AhArenaAllocator arena(4096);

  TestObject* obj = allocate<TestObject>(arena, 1, "test");
  ASSERT_NE(obj, nullptr);

  // dealloc is an alias for deallocate
  dealloc(arena, obj);
  EXPECT_TRUE(arena.empty());
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
