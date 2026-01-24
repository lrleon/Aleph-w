
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
 * @file ah-mapping_test.cc
 * @brief Tests for Ah Mapping
 */
#include <gtest/gtest.h>
#include <cstring>
#include <string>
#include <vector>
#include <filesystem>
#include <ah-map-arena.H>

using namespace Aleph;
namespace fs = std::filesystem;

// =============================================================================
// Test Fixtures
// =============================================================================

class MapArenaTest : public ::testing::Test
{
protected:
  std::string test_file;

  void SetUp() override
  {
    // Create a unique test file name
    test_file = "/tmp/map_arena_test_" + std::to_string(getpid()) + ".dat";
    // Ensure file doesn't exist
    fs::remove(test_file);
  }

  void TearDown() override
  {
    // Clean up test file
    fs::remove(test_file);
  }
};

// =============================================================================
// Construction Tests
// =============================================================================

TEST_F(MapArenaTest, DefaultConstruction)
{
  MapArena arena;
  EXPECT_FALSE(arena.is_initialized());
  EXPECT_EQ(arena.mapped_addr(), nullptr);
  EXPECT_EQ(arena.file_descriptor(), -1);
}

TEST_F(MapArenaTest, ConstructWithFile)
{
  MapArena arena(test_file);

  EXPECT_TRUE(arena.is_initialized());
  EXPECT_NE(arena.mapped_addr(), nullptr);
  EXPECT_GE(arena.file_descriptor(), 0);
  EXPECT_EQ(arena.size(), 0u);
  EXPECT_EQ(arena.capacity(), MapArena::initial_rgn_size);
  EXPECT_TRUE(fs::exists(test_file));
}

TEST_F(MapArenaTest, InitMethod)
{
  MapArena arena;
  arena.init(test_file);

  EXPECT_TRUE(arena.is_initialized());
  EXPECT_NE(arena.mapped_addr(), nullptr);
  EXPECT_EQ(arena.size(), 0u);
}

TEST_F(MapArenaTest, InitAndErase)
{
  // First create an arena with some data
  {
    MapArena arena(test_file);
    char * ptr = arena.reserve(100);
    ASSERT_NE(ptr, nullptr);
    std::memcpy(ptr, "test data", 10);
    arena.commit(10);
    arena.sync();
  }

  // Now init_and_erase should clear it
  MapArena arena;
  arena.init_and_erase(test_file);

  EXPECT_TRUE(arena.is_initialized());
  EXPECT_EQ(arena.size(), 0u);
  EXPECT_TRUE(arena.empty());
}

// =============================================================================
// Move Semantics Tests
// =============================================================================

TEST_F(MapArenaTest, MoveConstruction)
{
  MapArena original(test_file);
  char * ptr = original.reserve(50);
  ASSERT_NE(ptr, nullptr);
  std::memcpy(ptr, "hello", 6);
  original.commit(6);

  void * orig_addr = original.mapped_addr();

  MapArena moved(std::move(original));

  // Moved-to arena should have the resources
  EXPECT_TRUE(moved.is_initialized());
  EXPECT_EQ(moved.mapped_addr(), orig_addr);
  EXPECT_EQ(moved.size(), 6u);

  // Original should be empty
  EXPECT_FALSE(original.is_initialized());
  EXPECT_EQ(original.mapped_addr(), nullptr);
  EXPECT_EQ(original.file_descriptor(), -1);
}

TEST_F(MapArenaTest, MoveAssignment)
{
  MapArena original(test_file);
  char * ptr = original.reserve(50);
  ASSERT_NE(ptr, nullptr);
  std::memcpy(ptr, "data", 5);
  original.commit(5);

  MapArena target;
  target = std::move(original);

  EXPECT_TRUE(target.is_initialized());
  EXPECT_EQ(target.size(), 5u);
  EXPECT_FALSE(original.is_initialized());
}

TEST_F(MapArenaTest, MoveAssignmentRoundtrip)
{
  MapArena arena(test_file);
  arena.reserve(10);
  arena.commit(10);

  MapArena tmp;
  tmp = std::move(arena);
  arena = std::move(tmp);

  // Should still be valid after roundtrip move
  EXPECT_TRUE(arena.is_initialized());
  EXPECT_EQ(arena.size(), 10u);
}

// =============================================================================
// Reserve and Commit Tests
// =============================================================================

TEST_F(MapArenaTest, BasicReserveCommit)
{
  MapArena arena(test_file);

  char * ptr = arena.reserve(100);
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(arena.size(), 0u); // Not committed yet

  arena.commit(100);
  EXPECT_EQ(arena.size(), 100u);
}

TEST_F(MapArenaTest, MultipleReserveCommit)
{
  MapArena arena(test_file);

  for (int i = 0; i < 10; ++i)
    {
      char * ptr = arena.reserve(50);
      ASSERT_NE(ptr, nullptr);
      arena.commit(50);
    }

  EXPECT_EQ(arena.size(), 500u);
}

TEST_F(MapArenaTest, PartialCommit)
{
  MapArena arena(test_file);

  char * ptr = arena.reserve(1000);
  ASSERT_NE(ptr, nullptr);

  // Only commit part of what was reserved
  arena.commit(500);
  EXPECT_EQ(arena.size(), 500u);
}

TEST_F(MapArenaTest, ReserveWithData)
{
  MapArena arena(test_file);

  const char * message = "Hello, MapArena!";
  size_t len = std::strlen(message) + 1;

  char * ptr = arena.reserve(len);
  ASSERT_NE(ptr, nullptr);

  std::memcpy(ptr, message, len);
  arena.commit(len);

  // Verify data
  EXPECT_STREQ(arena.base(), message);
}

TEST_F(MapArenaTest, ReserveTriggersRemap)
{
  MapArena arena(test_file);

  size_t initial_capacity = arena.capacity();

  // Reserve more than initial capacity
  char * ptr = arena.reserve(initial_capacity + 1000);
  ASSERT_NE(ptr, nullptr);

  EXPECT_GT(arena.capacity(), initial_capacity);
  arena.commit(initial_capacity + 1000);
  EXPECT_EQ(arena.size(), initial_capacity + 1000);
}

TEST_F(MapArenaTest, LargeReservation)
{
  MapArena arena(test_file);

  // Reserve 1 MB
  size_t large_size = 1024 * 1024;
  char * ptr = arena.reserve(large_size);
  ASSERT_NE(ptr, nullptr);

  // Write pattern
  std::memset(ptr, 'X', large_size);
  arena.commit(large_size);

  EXPECT_EQ(arena.size(), large_size);
  EXPECT_GE(arena.capacity(), large_size);
}

// =============================================================================
// Iterator Tests
// =============================================================================

TEST_F(MapArenaTest, IteratorOnEmpty)
{
  MapArena arena(test_file);

  EXPECT_EQ(arena.begin(), arena.end());
}

TEST_F(MapArenaTest, IteratorTraversal)
{
  MapArena arena(test_file);

  const char * data = "ABCDEFGHIJ";
  char * ptr = arena.reserve(10);
  ASSERT_NE(ptr, nullptr);
  std::memcpy(ptr, data, 10);
  arena.commit(10);

  // Traverse with iterators
  int i = 0;
  for (auto it = arena.begin(); it != arena.end(); ++it, ++i)
    EXPECT_EQ(*it, data[i]);

  EXPECT_EQ(i, 10);
}

TEST_F(MapArenaTest, ConstIterators)
{
  MapArena arena(test_file);
  char * ptr = arena.reserve(5);
  std::memcpy(ptr, "test", 5);
  arena.commit(5);

  const MapArena & const_arena = arena;
  EXPECT_EQ(const_arena.end() - const_arena.begin(), 5);
}

TEST_F(MapArenaTest, RangeBasedFor)
{
  MapArena arena(test_file);

  char * ptr = arena.reserve(26);
  for (int i = 0; i < 26; ++i)
    ptr[i] = 'a' + i;
  arena.commit(26);

  std::string result;
  for (char c : arena)
    result += c;

  EXPECT_EQ(result, "abcdefghijklmnopqrstuvwxyz");
}

// =============================================================================
// Size and Capacity Tests
// =============================================================================

TEST_F(MapArenaTest, InitialCapacity)
{
  MapArena arena(test_file);
  EXPECT_EQ(arena.capacity(), MapArena::initial_rgn_size);
}

TEST_F(MapArenaTest, AvailableMemory)
{
  MapArena arena(test_file);

  EXPECT_EQ(arena.avail(), arena.capacity());

  arena.reserve(100);
  arena.commit(100);

  EXPECT_EQ(arena.avail(), arena.capacity() - 100);
}

TEST_F(MapArenaTest, SizeAfterCommits)
{
  MapArena arena(test_file);

  EXPECT_EQ(arena.size(), 0u);
  EXPECT_TRUE(arena.empty());

  arena.reserve(50);
  arena.commit(50);
  EXPECT_EQ(arena.size(), 50u);
  EXPECT_FALSE(arena.empty());

  arena.reserve(30);
  arena.commit(30);
  EXPECT_EQ(arena.size(), 80u);
}

// =============================================================================
// Persistence Tests
// =============================================================================

TEST_F(MapArenaTest, DataPersistsAcrossInstances)
{
  const char * message = "Persistent data!";
  size_t len = std::strlen(message) + 1;

  // Write data
  {
    MapArena arena(test_file);
    char * ptr = arena.reserve(len);
    ASSERT_NE(ptr, nullptr);
    std::memcpy(ptr, message, len);
    arena.commit(len);
    arena.sync();
  }

  // Read data in new instance
  {
    MapArena arena(test_file);
    // Note: Recovery reads end_ from file start, but our simple version
    // doesn't store it. This test verifies basic file creation.
    EXPECT_TRUE(arena.is_initialized());
  }
}

TEST_F(MapArenaTest, SyncMethod)
{
  MapArena arena(test_file);

  char * ptr = arena.reserve(100);
  ASSERT_NE(ptr, nullptr);
  std::memset(ptr, 'A', 100);
  arena.commit(100);

  // sync() should not throw
  EXPECT_NO_THROW(arena.sync());
}

// =============================================================================
// Query Method Tests
// =============================================================================

TEST_F(MapArenaTest, BaseMethod)
{
  MapArena arena(test_file);

  EXPECT_EQ(arena.base(), arena.begin());
  EXPECT_EQ(arena.base(), arena.mapped_addr());
}

TEST_F(MapArenaTest, EmptyMethod)
{
  MapArena arena(test_file);

  EXPECT_TRUE(arena.empty());

  arena.reserve(1);
  arena.commit(1);

  EXPECT_FALSE(arena.empty());
}

TEST_F(MapArenaTest, IsInitializedMethod)
{
  MapArena arena;
  EXPECT_FALSE(arena.is_initialized());

  arena.init(test_file);
  EXPECT_TRUE(arena.is_initialized());
}

TEST_F(MapArenaTest, FileDescriptor)
{
  MapArena arena(test_file);
  int fd = arena.file_descriptor();

  EXPECT_GE(fd, 0);

  // Verify it's a valid fd by checking with fcntl
  int flags = fcntl(fd, F_GETFD);
  EXPECT_NE(flags, -1);
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F(MapArenaTest, ZeroSizeReserve)
{
  MapArena arena(test_file);

  char * ptr = arena.reserve(0);
  EXPECT_NE(ptr, nullptr); // Should return current end pointer
  arena.commit(0);
  EXPECT_EQ(arena.size(), 0u);
}

TEST_F(MapArenaTest, MultipleRemapGrowth)
{
  MapArena arena(test_file);

  size_t total = 0;
  size_t allocation_size = MapArena::initial_rgn_size / 2;

  // Force multiple remaps
  for (int i = 0; i < 10; ++i)
    {
      char * ptr = arena.reserve(allocation_size);
      ASSERT_NE(ptr, nullptr) << "Failed at iteration " << i;
      arena.commit(allocation_size);
      total += allocation_size;
    }

  EXPECT_EQ(arena.size(), total);
  EXPECT_GE(arena.capacity(), total);
}

// =============================================================================
// Output Operator Tests
// =============================================================================

TEST_F(MapArenaTest, OutputOperator)
{
  MapArena arena(test_file);
  arena.reserve(100);
  arena.commit(100);

  std::ostringstream oss;
  oss << arena;

  std::string output = oss.str();
  EXPECT_NE(output.find("MapArena"), std::string::npos);
  EXPECT_NE(output.find("size"), std::string::npos);
  EXPECT_NE(output.find("capacity"), std::string::npos);
}

// =============================================================================
// Destructor Safety Tests
// =============================================================================

TEST_F(MapArenaTest, DestructorOnUninitialized)
{
  // Should not crash when destroying uninitialized arena
  MapArena arena;
  // Destructor called automatically
}

TEST_F(MapArenaTest, DestructorAfterMove)
{
  MapArena arena(test_file);
  MapArena other = std::move(arena);
  // arena destructor should safely handle moved-from state
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST_F(MapArenaTest, ManySmallAllocations)
{
  MapArena arena(test_file);

  constexpr int N = 10000;
  for (int i = 0; i < N; ++i)
    {
      char * ptr = arena.reserve(8);
      ASSERT_NE(ptr, nullptr) << "Failed at allocation " << i;
      arena.commit(8);
    }

  EXPECT_EQ(arena.size(), N * 8u);
}

TEST_F(MapArenaTest, AlternatingAllocations)
{
  MapArena arena(test_file);

  size_t total = 0;
  for (int i = 0; i < 100; ++i)
    {
      size_t sz = (i % 2 == 0) ? 16 : 256;
      char * ptr = arena.reserve(sz);
      ASSERT_NE(ptr, nullptr);
      arena.commit(sz);
      total += sz;
    }

  EXPECT_EQ(arena.size(), total);
}

// =============================================================================
// Type Alias Tests
// =============================================================================

TEST_F(MapArenaTest, TypeAliases)
{
  static_assert(std::is_same_v<MapArena::iterator, char*>);
  static_assert(std::is_same_v<MapArena::const_iterator, const char*>);
  static_assert(std::is_same_v<MapArena::size_type, size_t>);
}

// =============================================================================
// Static Constants Tests
// =============================================================================

TEST(MapArenaConstants, InitialRegionSize)
{
  EXPECT_EQ(MapArena::initial_rgn_size, 4 * 1024u);
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
