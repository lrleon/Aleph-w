#include <gtest/gtest.h>

#include <tpl_dynArrayHeap.H>

using namespace Aleph;
using namespace testing;

TEST(DynArrayHeap, InsertAndExtract)
{
  DynArrayHeap<int> heap;
  heap.insert(5);
  heap.insert(2);
  heap.insert(4);

  EXPECT_EQ(heap.size(), 3u);
  EXPECT_EQ(heap.top(), 2);

  EXPECT_EQ(heap.getMin(), 2);
  EXPECT_EQ(heap.get(), 4);
  EXPECT_EQ(heap.getMax(), 5);
  EXPECT_TRUE(heap.is_empty());
}

TEST(DynArrayHeap, ReserveAndDirectInsert)
{
  DynArrayHeap<int> heap;
  heap.reserve(4);
  heap.insert(10);
  heap.insert(1);

  auto & ref = heap.insert_direct(5);
  EXPECT_EQ(ref, 5);
  EXPECT_EQ(heap.size(), 3u);
  EXPECT_EQ(heap.top(), 1);
}

TEST(DynArrayHeap, ThrowsOnEmpty)
{
  DynArrayHeap<int> heap;
  EXPECT_TRUE(heap.is_empty());
  EXPECT_THROW(heap.top(), std::underflow_error);
  EXPECT_THROW(heap.getMin(), std::underflow_error);
}
