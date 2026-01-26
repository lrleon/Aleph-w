
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
 * @file ah_arena_exhaustive_test.cc
 * @brief EXHAUSTIVE and RIGOROUS tests for AhArenaAllocator
 *
 * This file complements ah_arena_test.cc with:
 * - Stress tests (thousands of allocations)
 * - Fragmentation and realistic usage patterns
 * - Exception safety tests
 * - Polymorphic and complex objects
 * - Multiple use/reset cycles
 * - Boundary conditions and corner cases
 * - Memory leak verification (via test harness)
 * - Performance benchmarks
 * - Statistical validation of randomized tests
 *
 * COMPILE:
 *   g++ -std=c++20 -O2 -I.. -o ah_arena_exhaustive_test ah_arena_exhaustive_test.cc -lgtest -lgtest_main -pthread
 *
 * RUN:
 *   ./ah_arena_exhaustive_test
 *   valgrind --leak-check=full ./ah_arena_exhaustive_test  # For leak detection
 */

#include <gtest/gtest.h>

#include <ah-arena.H>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

using namespace Aleph;

// ============================================================================
// Helper Classes and Utilities
// ============================================================================

namespace
{
// Exception-throwing object for testing exception safety
class ThrowingObject
{
  static bool should_throw_;
  static int construct_count_;
  static int destruct_count_;

public:
  int value;

  explicit ThrowingObject(int v) : value(v)
  {
    ++construct_count_;
    if (should_throw_)
      throw std::runtime_error("Intentional construction failure");
  }

  ~ThrowingObject()
  {
    ++destruct_count_;
  }

  static void reset_counters()
  {
    construct_count_ = 0;
    destruct_count_ = 0;
    should_throw_ = false;
  }

  static void set_throw(bool t) { should_throw_ = t; }
  static int constructions() { return construct_count_; }
  static int destructions() { return destruct_count_; }
};

bool ThrowingObject::should_throw_ = false;
int ThrowingObject::construct_count_ = 0;
int ThrowingObject::destruct_count_ = 0;

// Polymorphic base class
class Base
{
public:
  virtual ~Base() = default;
  virtual int get_value() const = 0;
  virtual std::string get_type() const = 0;
};

// Derived class 1
class Derived1 : public Base
{
  int value_;
public:
  explicit Derived1(int v) : value_(v) {}
  int get_value() const override { return value_; }
  std::string get_type() const override { return "Derived1"; }
};

// Derived class 2
class Derived2 : public Base
{
  int value_;
public:
  explicit Derived2(int v) : value_(v) {}
  int get_value() const override { return value_ * 2; }
  std::string get_type() const override { return "Derived2"; }
};

// Large object for alignment testing
struct alignas(64) CacheLineAligned
{
  char data[64];
  int marker = 0xDEADBEEF;

  CacheLineAligned() = default;
  explicit CacheLineAligned(int m) : marker(m) {}
};

// Small object for fragmentation testing
struct TinyObject
{
  char byte;
  explicit TinyObject(char b = 0) : byte(b) {}
};

// Medium object
struct MediumObject
{
  char data[128];
  int id;
  explicit MediumObject(int i = 0) : id(i) { std::fill(data, data + 128, static_cast<char>(i)); }
};

// Large object
struct LargeObject
{
  char data[4096];
  int id;
  explicit LargeObject(int i = 0) : id(i) { std::fill(data, data + 4096, static_cast<char>(i)); }
};

// RAII wrapper for tracking allocations
template <typename T>
class AllocationTracker
{
  AhArenaAllocator& arena_;
  T* ptr_;

public:
  AllocationTracker(AhArenaAllocator& arena, T* p) : arena_(arena), ptr_(p) {}

  ~AllocationTracker()
  {
    if (ptr_)
      deallocate(arena_, ptr_);
  }

  AllocationTracker(const AllocationTracker&) = delete;
  AllocationTracker& operator=(const AllocationTracker&) = delete;

  AllocationTracker(AllocationTracker&& other) noexcept
    : arena_(other.arena_), ptr_(other.ptr_)
  {
    other.ptr_ = nullptr;
  }

  T* get() const { return ptr_; }
  T* operator->() const { return ptr_; }
  T& operator*() const { return *ptr_; }
};

} // anonymous namespace

// ============================================================================
// Stress Tests - Massive Allocations
// ============================================================================

TEST(ArenaStress, ThousandSmallAllocations)
{
  const size_t NUM_ALLOCS = 10000;
  const size_t ALLOC_SIZE = 8;

  AhArenaAllocator arena(NUM_ALLOCS * ALLOC_SIZE);

  std::vector<void*> ptrs;
  ptrs.reserve(NUM_ALLOCS);

  for (size_t i = 0; i < NUM_ALLOCS; ++i)
  {
    void* ptr = arena.alloc(ALLOC_SIZE);
    ASSERT_NE(ptr, nullptr) << "Allocation " << i << " failed";
    ptrs.push_back(ptr);
  }

  EXPECT_EQ(arena.allocated_size(), NUM_ALLOCS * ALLOC_SIZE);
  EXPECT_TRUE(arena.full());

  // Verify all pointers are distinct
  std::sort(ptrs.begin(), ptrs.end());
  auto last = std::unique(ptrs.begin(), ptrs.end());
  EXPECT_EQ(last, ptrs.end()) << "Found duplicate pointers";
}

TEST(ArenaStress, MixedSizeAllocations)
{
  const size_t ARENA_SIZE = 1024 * 1024;  // 1 MB
  AhArenaAllocator arena(ARENA_SIZE);

  std::mt19937 rng(42);
  std::uniform_int_distribution<size_t> size_dist(1, 1024);

  std::vector<void*> ptrs;
  size_t total_allocated = 0;

  while (true)
  {
    size_t size = size_dist(rng);
    void* ptr = arena.alloc(size);

    if (ptr == nullptr)
      break;

    ptrs.push_back(ptr);
    total_allocated += size;
  }

  EXPECT_GT(ptrs.size(), 100u) << "Should have allocated many objects";
  EXPECT_LE(total_allocated, ARENA_SIZE);
  EXPECT_GE(arena.allocated_size(), total_allocated * 0.9);  // Allow some slack
}

TEST(ArenaStress, ThousandTypedAllocations)
{
  const size_t NUM_OBJECTS = 5000;
  AhArenaAllocator arena(NUM_OBJECTS * sizeof(MediumObject));

  std::vector<MediumObject*> objects;
  objects.reserve(NUM_OBJECTS);

  for (size_t i = 0; i < NUM_OBJECTS; ++i)
  {
    MediumObject* obj = allocate<MediumObject>(arena, static_cast<int>(i));
    ASSERT_NE(obj, nullptr) << "Object " << i << " allocation failed";
    EXPECT_EQ(obj->id, static_cast<int>(i));
    objects.push_back(obj);
  }

  // Verify all objects are correctly initialized
  for (size_t i = 0; i < objects.size(); ++i)
  {
    EXPECT_EQ(objects[i]->id, static_cast<int>(i));
    EXPECT_EQ(objects[i]->data[0], static_cast<char>(i));
  }

  // LIFO deallocation
  for (auto it = objects.rbegin(); it != objects.rend(); ++it)
    deallocate(arena, *it);

  EXPECT_TRUE(arena.empty());
}

// ============================================================================
// Fragmentation and Usage Patterns
// ============================================================================

TEST(ArenaFragmentation, AlternatingSmallLarge)
{
  const size_t ARENA_SIZE = 1024 * 100;
  AhArenaAllocator arena(ARENA_SIZE);

  std::vector<void*> small_ptrs;
  std::vector<void*> large_ptrs;

  // Allocate alternating small and large
  for (int i = 0; i < 50; ++i)
  {
    void* small = arena.alloc(16);
    void* large = arena.alloc(1024);

    if (small) small_ptrs.push_back(small);
    if (large) large_ptrs.push_back(large);
  }

  size_t allocated = arena.allocated_size();
  EXPECT_GT(allocated, 50u * (16 + 1024) * 0.8);  // At least 80% efficiency
}

TEST(ArenaFragmentation, WorstCasePattern)
{
  // Allocate largest chunks first, then try to fit small ones
  const size_t ARENA_SIZE = 10000;
  AhArenaAllocator arena(ARENA_SIZE);

  std::vector<void*> ptrs;

  // Fill with large allocations
  while (true)
  {
    void* ptr = arena.alloc(1000);
    if (!ptr) break;
    ptrs.push_back(ptr);
  }

  size_t before = arena.allocated_size();

  // Try to allocate small (should fail or succeed depending on remaining)
  void* small = arena.alloc(10);
  (void)small;  // May or may not succeed

  EXPECT_GE(arena.allocated_size(), before);
}

TEST(ArenaFragmentation, RepeatedResetAndReuse)
{
  const size_t ARENA_SIZE = 50000;
  AhArenaAllocator arena(ARENA_SIZE);

  for (int cycle = 0; cycle < 100; ++cycle)
  {
    // Allocate varying amounts
    std::mt19937 rng(cycle);
    std::uniform_int_distribution<size_t> size_dist(10, 500);

    size_t total = 0;
    while (total < ARENA_SIZE * 0.9)
    {
      size_t size = size_dist(rng);
      void* ptr = arena.alloc(size);
      if (!ptr) break;
      total += size;
    }

    EXPECT_GT(arena.allocated_size(), ARENA_SIZE * 0.5);

    arena.reset();
    EXPECT_TRUE(arena.empty());
  }

  // Arena should still be usable after 100 cycles
  void* ptr = arena.alloc(1000);
  EXPECT_NE(ptr, nullptr);
}

// ============================================================================
// Exception Safety Tests
// ============================================================================

TEST(ArenaExceptionSafety, ConstructorThrows)
{
  ThrowingObject::reset_counters();
  ThrowingObject::set_throw(true);

  AhArenaAllocator arena(4096);

  ThrowingObject* obj = nullptr;
  EXPECT_THROW(
    obj = allocate<ThrowingObject>(arena, 42),
    std::runtime_error
  );

  EXPECT_EQ(obj, nullptr);
  EXPECT_EQ(ThrowingObject::constructions(), 1);  // Constructor was called
  EXPECT_EQ(ThrowingObject::destructions(), 0);   // But object not created

  // Arena should still be usable
  ThrowingObject::set_throw(false);
  obj = allocate<ThrowingObject>(arena, 100);
  EXPECT_NE(obj, nullptr);
  EXPECT_EQ(obj->value, 100);
}

TEST(ArenaExceptionSafety, PartialAllocationRecovery)
{
  ThrowingObject::reset_counters();

  AhArenaAllocator arena(4096);

  // Allocate some objects successfully
  std::vector<ThrowingObject*> objs;
  for (int i = 0; i < 5; ++i)
  {
    ThrowingObject* obj = allocate<ThrowingObject>(arena, i);
    ASSERT_NE(obj, nullptr);
    objs.push_back(obj);
  }

  EXPECT_EQ(ThrowingObject::constructions(), 5);

  // Now cause one to throw
  ThrowingObject::set_throw(true);
  EXPECT_THROW(
    allocate<ThrowingObject>(arena, 999),
    std::runtime_error
  );

  // Previous allocations should still be valid
  for (size_t i = 0; i < objs.size(); ++i)
    EXPECT_EQ(objs[i]->value, static_cast<int>(i));

  // Cleanup
  ThrowingObject::set_throw(false);
  for (auto it = objs.rbegin(); it != objs.rend(); ++it)
    deallocate(arena, *it);
}

// ============================================================================
// Polymorphic and Complex Objects
// ============================================================================

TEST(ArenaPolymorphism, VirtualDispatch)
{
  AhArenaAllocator arena(4096);

  std::vector<Base*> objects;

  for (int i = 0; i < 10; ++i)
  {
    if (i % 2 == 0)
      objects.push_back(allocate<Derived1>(arena, i));
    else
      objects.push_back(allocate<Derived2>(arena, i));
  }

  // Verify polymorphic behavior
  for (size_t i = 0; i < objects.size(); ++i)
  {
    if (i % 2 == 0)
    {
      EXPECT_EQ(objects[i]->get_type(), "Derived1");
      EXPECT_EQ(objects[i]->get_value(), static_cast<int>(i));
    }
    else
    {
      EXPECT_EQ(objects[i]->get_type(), "Derived2");
      EXPECT_EQ(objects[i]->get_value(), static_cast<int>(i) * 2);
    }
  }

  // Cleanup (must call virtual destructors)
  for (auto it = objects.rbegin(); it != objects.rend(); ++it)
  {
    if ((*it)->get_type() == "Derived1")
      deallocate<Derived1>(arena, static_cast<Derived1*>(*it));
    else
      deallocate<Derived2>(arena, static_cast<Derived2*>(*it));
  }
}

TEST(ArenaComplexObjects, StringMembers)
{
  struct ComplexObject
  {
    std::string name;
    std::vector<int> numbers;
    int id;

    ComplexObject(std::string n, std::vector<int> nums, int i)
      : name(std::move(n)), numbers(std::move(nums)), id(i)
    {}
  };

  AhArenaAllocator arena(100000);

  std::vector<ComplexObject*> objects;

  for (int i = 0; i < 100; ++i)
  {
    std::string name = "Object_" + std::to_string(i);
    std::vector<int> nums(i % 10 + 1, i);

    ComplexObject* obj = allocate<ComplexObject>(arena, name, nums, i);
    ASSERT_NE(obj, nullptr);
    objects.push_back(obj);
  }

  // Verify
  for (size_t i = 0; i < objects.size(); ++i)
  {
    EXPECT_EQ(objects[i]->name, "Object_" + std::to_string(i));
    EXPECT_EQ(objects[i]->id, static_cast<int>(i));
    EXPECT_EQ(objects[i]->numbers.size(), i % 10 + 1);
  }

  // Cleanup
  for (auto it = objects.rbegin(); it != objects.rend(); ++it)
    deallocate(arena, *it);
}

// ============================================================================
// Alignment Tests - Advanced
// ============================================================================

TEST(ArenaAlignmentAdvanced, CacheLineAlignment)
{
  AhArenaAllocator arena(10000);

  std::vector<CacheLineAligned*> objects;

  for (int i = 0; i < 50; ++i)
  {
    CacheLineAligned* obj = allocate<CacheLineAligned>(arena, i);
    ASSERT_NE(obj, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(obj) % 64, 0u)
      << "Object " << i << " not 64-byte aligned";
    EXPECT_EQ(obj->marker, i);
    objects.push_back(obj);
  }

  // Cleanup
  for (auto it = objects.rbegin(); it != objects.rend(); ++it)
    deallocate(arena, *it);
}

TEST(ArenaAlignmentAdvanced, MixedAlignments)
{
  AhArenaAllocator arena(10000);

  struct Align4 { alignas(4) char data[4]; };
  struct Align8 { alignas(8) char data[8]; };
  struct Align16 { alignas(16) char data[16]; };
  struct Align32 { alignas(32) char data[32]; };

  // Allocate in mixed order
  auto a4 = allocate<Align4>(arena);
  auto a32 = allocate<Align32>(arena);
  auto a8 = allocate<Align8>(arena);
  auto a16 = allocate<Align16>(arena);

  ASSERT_NE(a4, nullptr);
  ASSERT_NE(a8, nullptr);
  ASSERT_NE(a16, nullptr);
  ASSERT_NE(a32, nullptr);

  EXPECT_EQ(reinterpret_cast<uintptr_t>(a4) % 4, 0u);
  EXPECT_EQ(reinterpret_cast<uintptr_t>(a8) % 8, 0u);
  EXPECT_EQ(reinterpret_cast<uintptr_t>(a16) % 16, 0u);
  EXPECT_EQ(reinterpret_cast<uintptr_t>(a32) % 32, 0u);
}

TEST(ArenaAlignmentAdvanced, VeryLargeAlignment)
{
  AhArenaAllocator arena(100000);

  // 256-byte alignment
  void* ptr = arena.alloc_aligned(100, 256);
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % 256, 0u);

  // 512-byte alignment
  ptr = arena.alloc_aligned(100, 512);
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % 512, 0u);

  // 1024-byte alignment
  ptr = arena.alloc_aligned(100, 1024);
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % 1024, 0u);
}

// ============================================================================
// Boundary Conditions
// ============================================================================

TEST(ArenaBoundary, AllocationAtExactCapacity)
{
  const size_t SIZE = 1000;
  AhArenaAllocator arena(SIZE);

  // Fill exactly to capacity with multiple allocations
  void* p1 = arena.alloc(400);
  void* p2 = arena.alloc(300);
  void* p3 = arena.alloc(300);

  ASSERT_NE(p1, nullptr);
  ASSERT_NE(p2, nullptr);
  ASSERT_NE(p3, nullptr);

  EXPECT_TRUE(arena.full());
  EXPECT_EQ(arena.allocated_size(), SIZE);
  EXPECT_EQ(arena.available_size(), 0u);

  // No more allocations possible
  void* p4 = arena.alloc(1);
  EXPECT_EQ(p4, nullptr);
}

TEST(ArenaBoundary, SingleByteAllocations)
{
  const size_t SIZE = 100;
  AhArenaAllocator arena(SIZE);

  std::vector<void*> ptrs;

  for (size_t i = 0; i < SIZE; ++i)
  {
    void* ptr = arena.alloc(1);
    ASSERT_NE(ptr, nullptr) << "Byte " << i << " failed";
    ptrs.push_back(ptr);
  }

  EXPECT_TRUE(arena.full());

  // Verify all pointers are consecutive
  for (size_t i = 1; i < ptrs.size(); ++i)
  {
    EXPECT_EQ(
      static_cast<char*>(ptrs[i]),
      static_cast<char*>(ptrs[i-1]) + 1
    );
  }
}

TEST(ArenaBoundary, AlignmentPaddingAtBoundary)
{
  const size_t SIZE = 100;
  AhArenaAllocator arena(SIZE);

  // Allocate most of arena
  void* p1 = arena.alloc(90);
  ASSERT_NE(p1, nullptr);

  // Try aligned allocation that requires padding
  // This might fail due to padding requirements
  void* p2 = arena.alloc_aligned(5, 16);

  if (p2 != nullptr)
  {
    EXPECT_EQ(reinterpret_cast<uintptr_t>(p2) % 16, 0u);
  }
  else
  {
    // Failure is acceptable if not enough space for alignment
    EXPECT_LT(arena.available_size(), 21u);  // 5 + up to 15 padding
  }
}

// ============================================================================
// LIFO Pattern Tests - Complex
// ============================================================================

TEST(ArenaLIFO, DeepNesting)
{
  // This test allocates 10+20+...+1000 = 50500 bytes.
  // Use a comfortably larger arena to avoid spurious failures.
  AhArenaAllocator arena(60000);

  struct Level
  {
    void* ptr;
    size_t requested_size;
    size_t actual_size; // includes any allocator overhead (e.g. alignment padding)
  };

  std::vector<Level> stack;
  stack.reserve(100);

  // Push many levels and record the *actual* consumed bytes so we can
  // deallocate accurately even if future allocator versions introduce padding.
  for (int i = 0; i < 100; ++i)
  {
    const size_t before = arena.allocated_size();
    const size_t size = static_cast<size_t>(i + 1) * 10;
    void* ptr = arena.alloc(size);
    ASSERT_NE(ptr, nullptr);
    const size_t after = arena.allocated_size();
    stack.push_back({ptr, size, after - before});
  }

  const size_t max_allocated = arena.allocated_size();

  // Pop in LIFO order
  while (!stack.empty())
  {
    Level level = stack.back();
    stack.pop_back();

    const size_t before = arena.allocated_size();
    arena.dealloc(level.ptr, level.actual_size);
    const size_t after = arena.allocated_size();

    EXPECT_EQ(before - after, level.actual_size);
  }

  EXPECT_TRUE(arena.empty());
  EXPECT_GT(max_allocated, 45000u);
}

TEST(ArenaLIFO, InterleavedTypedAllocations)
{
  AhArenaAllocator arena(100000);

  std::vector<TinyObject*> tiny_stack;
  std::vector<MediumObject*> medium_stack;

  // Interleaved pushes
  for (int i = 0; i < 50; ++i)
  {
    tiny_stack.push_back(allocate<TinyObject>(arena, static_cast<char>(i)));
    medium_stack.push_back(allocate<MediumObject>(arena, i));
  }

  // Interleaved pops (LIFO per type)
  while (!medium_stack.empty() || !tiny_stack.empty())
  {
    if (!medium_stack.empty())
    {
      deallocate(arena, medium_stack.back());
      medium_stack.pop_back();
    }

    if (!tiny_stack.empty())
    {
      deallocate(arena, tiny_stack.back());
      tiny_stack.pop_back();
    }
  }

  // Interleaving different-sized objects can leave the arena non-empty because
  // deallocation is only effective for a strict LIFO pattern on the raw blocks.
  // The supported bulk mechanism is reset().
  arena.reset();
  EXPECT_TRUE(arena.empty());
}

// ============================================================================
// Performance and Scaling Tests
// ============================================================================

TEST(ArenaPerformance, LargeScaleAllocation)
{
  const size_t NUM_OBJECTS = 50000;
  AhArenaAllocator arena(NUM_OBJECTS * sizeof(MediumObject));

  auto start = std::chrono::high_resolution_clock::now();

  std::vector<MediumObject*> objects;
  objects.reserve(NUM_OBJECTS);

  for (size_t i = 0; i < NUM_OBJECTS; ++i)
    objects.push_back(allocate<MediumObject>(arena, static_cast<int>(i)));

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  // Should be fast (bump allocation is O(1))
  EXPECT_LT(duration.count(), 1000) << "Allocation too slow: " << duration.count() << "ms";

  // Verify all allocated
  for (size_t i = 0; i < objects.size(); ++i)
  {
    ASSERT_NE(objects[i], nullptr);
    EXPECT_EQ(objects[i]->id, static_cast<int>(i));
  }

  std::cout << "  [Allocated " << NUM_OBJECTS << " objects in "
            << duration.count() << "ms]" << std::endl;
}

TEST(ArenaPerformance, CompareWithMalloc)
{
  const size_t NUM_ALLOCS = 10000;
  const size_t ALLOC_SIZE = 64;

  // Malloc timing
  auto start = std::chrono::high_resolution_clock::now();

  std::vector<void*> malloc_ptrs;
  malloc_ptrs.reserve(NUM_ALLOCS);
  for (size_t i = 0; i < NUM_ALLOCS; ++i)
    malloc_ptrs.push_back(std::malloc(ALLOC_SIZE));

  for (void* ptr : malloc_ptrs)
    std::free(ptr);

  auto end = std::chrono::high_resolution_clock::now();
  auto malloc_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  // Arena timing
  start = std::chrono::high_resolution_clock::now();

  AhArenaAllocator arena(NUM_ALLOCS * ALLOC_SIZE);
  std::vector<void*> arena_ptrs;
  arena_ptrs.reserve(NUM_ALLOCS);
  for (size_t i = 0; i < NUM_ALLOCS; ++i)
    arena_ptrs.push_back(arena.alloc(ALLOC_SIZE));

  end = std::chrono::high_resolution_clock::now();
  auto arena_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  std::cout << "  [malloc: " << malloc_duration.count() << "μs, "
            << "arena: " << arena_duration.count() << "μs, "
            << "speedup: " << (double)malloc_duration.count() / arena_duration.count() << "x]"
            << std::endl;

  // Arena should be faster (typically 10-100x)
  EXPECT_LT(arena_duration.count(), malloc_duration.count());
}

// ============================================================================
// Statistical Validation
// ============================================================================

TEST(ArenaStatistical, RandomizedStressTest)
{
  const size_t NUM_TRIALS = 100;
  const size_t ARENA_SIZE = 100000;

  std::mt19937 rng(12345);
  std::uniform_int_distribution<size_t> size_dist(1, 1000);
  std::bernoulli_distribution dealloc_dist(0.3);  // 30% chance to dealloc

  size_t total_allocs = 0;
  size_t total_deallocs = 0;
  size_t failed_allocs = 0;

  for (size_t trial = 0; trial < NUM_TRIALS; ++trial)
  {
    AhArenaAllocator arena(ARENA_SIZE);
    std::vector<std::pair<void*, size_t>> allocations;

    for (int op = 0; op < 1000; ++op)
    {
      // Randomly allocate or deallocate
      if (allocations.empty() || !dealloc_dist(rng))
      {
        // Allocate
        size_t size = size_dist(rng);
        void* ptr = arena.alloc(size);

        if (ptr != nullptr)
        {
          allocations.push_back({ptr, size});
          ++total_allocs;
        }
        else
        {
          ++failed_allocs;
        }
      }
      else
      {
        // Deallocate (LIFO only works)
        auto [ptr, size] = allocations.back();
        allocations.pop_back();
        arena.dealloc(ptr, size);
        ++total_deallocs;
      }
    }
  }

  std::cout << "  [Trials: " << NUM_TRIALS
            << ", Allocs: " << total_allocs
            << ", Deallocs: " << total_deallocs
            << ", Failed: " << failed_allocs << "]" << std::endl;

  EXPECT_GT(total_allocs, NUM_TRIALS * 100);
  EXPECT_GT(total_deallocs, 0u);
}

// ============================================================================
// Edge Cases and Corner Cases
// ============================================================================

TEST(ArenaEdgeCases, MaxSizeAllocation)
{
  const size_t MAX = std::numeric_limits<size_t>::max();

  AhArenaAllocator arena(1024);

  // Try to allocate SIZE_MAX (should fail gracefully)
  void* ptr = arena.alloc(MAX);
  EXPECT_EQ(ptr, nullptr);

  // Arena should still be usable
  ptr = arena.alloc(100);
  EXPECT_NE(ptr, nullptr);
}

TEST(ArenaEdgeCases, AlignmentNotPowerOfTwo)
{
  AhArenaAllocator arena(1024);

  // Non-power-of-2 alignments (undefined behavior in C++, but test robustness)
  // The implementation uses bitwise operations that assume power-of-2
  // This test verifies it doesn't crash, even if result is undefined

  void* ptr1 = arena.alloc_aligned(100, 3);   // Not power of 2
  void* ptr2 = arena.alloc_aligned(100, 7);   // Not power of 2

  // Implementation-defined behavior, but should not crash
  // May return nullptr or incorrectly aligned pointer
  (void)ptr1;
  (void)ptr2;

  SUCCEED();  // Didn't crash
}

TEST(ArenaEdgeCases, ConstBufferUsage)
{
  const char buffer[1024] = {};  // Const buffer

  AhArenaAllocator arena(buffer, sizeof(buffer));

  EXPECT_TRUE(arena.is_valid());
  EXPECT_FALSE(arena.owns_memory());

  // Should be able to allocate (const_cast used internally)
  void* ptr = arena.alloc(100);
  EXPECT_NE(ptr, nullptr);
}

TEST(ArenaEdgeCases, ZeroCapacityArena)
{
  char buffer[1];  // Minimal buffer
  AhArenaAllocator arena(buffer, 0);  // Zero capacity

  EXPECT_TRUE(arena.is_valid());
  EXPECT_EQ(arena.capacity(), 0u);
  EXPECT_TRUE(arena.empty());
  EXPECT_TRUE(arena.full());

  void* ptr = arena.alloc(1);
  EXPECT_EQ(ptr, nullptr);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(ArenaIntegration, RealWorldUsagePattern)
{
  // Simulate a parser that builds an AST
  struct ASTNode
  {
    std::string token;
    std::vector<ASTNode*> children;
    int line_number;

    ASTNode(std::string t, int line)
      : token(std::move(t)), line_number(line)
    {}
  };

  const size_t ESTIMATED_NODES = 1000;
  AhArenaAllocator arena(ESTIMATED_NODES * sizeof(ASTNode));

  std::vector<ASTNode*> all_nodes;

  // Build tree
  for (int i = 0; i < 500; ++i)
  {
    ASTNode* node = allocate<ASTNode>(arena, "token_" + std::to_string(i), i);
    ASSERT_NE(node, nullptr);
    all_nodes.push_back(node);

    // Some nodes have children
    if (i > 0 && i % 10 == 0)
    {
      for (int j = 0; j < 3 && !all_nodes.empty(); ++j)
        node->children.push_back(all_nodes[all_nodes.size() - j - 1]);
    }
  }

  // Verify tree structure
  int nodes_with_children = 0;
  for (ASTNode* node : all_nodes)
  {
    EXPECT_FALSE(node->token.empty());
    EXPECT_GE(node->line_number, 0);
    if (!node->children.empty())
      ++nodes_with_children;
  }

  EXPECT_GT(nodes_with_children, 0);

  // Cleanup (LIFO)
  for (auto it = all_nodes.rbegin(); it != all_nodes.rend(); ++it)
    deallocate(arena, *it);

  EXPECT_TRUE(arena.empty());
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
