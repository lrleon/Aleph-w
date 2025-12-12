#include <algorithm>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_arrayHeap.H>

using namespace Aleph;
using namespace testing;

TEST(ArrayHeap, InsertAndExtract)
{
  ArrayHeap<int> heap(8);
  heap.insert(3);
  heap.insert(1);
  heap.insert(2);

  EXPECT_EQ(heap.size(), 3u);
  EXPECT_FALSE(heap.is_empty());
  EXPECT_EQ(heap.top(), 1);

  EXPECT_EQ(heap.getMin(), 1);
  EXPECT_EQ(heap.top(), 2);
  EXPECT_EQ(heap.get(), 2);
  EXPECT_EQ(heap.getMax(), 3); // alias of getMin when comparator flips
  EXPECT_TRUE(heap.is_empty());
}

TEST(ArrayHeap, UpdateAndRemove)
{
  ArrayHeap<int> heap(5);
  auto & ref = heap.insert(5);
  const auto & ref2 = heap.insert(7);

  ref = 1;
  heap.update(ref);
  EXPECT_EQ(heap.top(), 1);
  EXPECT_EQ(ref2, 7);

  heap.remove(ref);
  EXPECT_EQ(heap.size(), 1u);
  EXPECT_EQ(heap.top(), 7);
}

TEST(ArrayHeap, CapacityOverflow)
{
  ArrayHeap<int> heap(2);
  heap.insert(10);
  heap.insert(20);
  EXPECT_THROW(heap.insert(30), std::overflow_error);
}

TEST(ArrayHeap, ExtractsSortedOrder)
{
  ArrayHeap<int> heap(10);
  const std::vector<int> input{5, 3, 8, 1, 4, 9, 2, 7};
  for (int value : input)
    heap.insert(value);

  std::vector<int> drained;
  while (!heap.is_empty())
    drained.push_back(heap.getMin());

  std::vector<int> sorted = input;
  ranges::sort(sorted);
  EXPECT_EQ(drained, sorted);
  EXPECT_TRUE(heap.is_empty());
}
