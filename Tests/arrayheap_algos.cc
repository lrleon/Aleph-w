#include <algorithm>
#include <ranges>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_arrayHeap.H>

using namespace Aleph;
using namespace testing;

static bool is_min_heap(const std::vector<int> &v)
{
  if (v.empty())
    return true;

  std::vector<int> tmp(v.size() + 1);
  for (size_t i = 0; i < v.size(); ++i)
    tmp[i + 1] = v[i];

  Aleph::less<int> cmp;
  return valid_heap(tmp.data(), 1, v.size(), cmp);
}

TEST(ArrayHeapAlgos, ValidHeapDetectsHeapAndNonHeap)
{
  {
    std::vector<int> heap_like{1, 3, 2, 7, 5, 4};
    EXPECT_TRUE(is_min_heap(heap_like));
  }

  {
    std::vector<int> not_heap{3, 1, 2};
    EXPECT_FALSE(is_min_heap(not_heap));
  }
}

TEST(ArrayHeapAlgos, SiftUpRestoresHeapAfterInsertAtEnd)
{
  // Start from a valid min-heap of size 3: [1, 3, 2]
  int arr[5] = {0, 1, 3, 2, 0};
  Aleph::less<int> cmp;

  // Insert a smaller item at the end and sift it up
  arr[4] = 0;
  int &ref = sift_up(arr, 1, 4, cmp);

  EXPECT_EQ(ref, 0);
  EXPECT_TRUE(valid_heap(arr, 1, 4, cmp));
  EXPECT_EQ(arr[1], 0);
}

TEST(ArrayHeapAlgos, SiftDownRestoresHeapAfterRootViolation)
{
  // A min-heap of 5 elements would have 1 at root, but we break it.
  int arr[6] = {0, 9, 1, 2, 3, 4};
  Aleph::less<int> cmp;

  EXPECT_FALSE(valid_heap(arr, 1, 5, cmp));
  sift_down(arr, 1, 5, cmp);
  EXPECT_TRUE(valid_heap(arr, 1, 5, cmp));
  EXPECT_EQ(arr[1], 1);
}

TEST(ArrayHeapAlgos, SiftDownUpRestoresHeapAfterInternalUpdate)
{
  // Start with a valid heap
  int arr[8] = {0, 1, 3, 2, 7, 5, 4, 8};
  Aleph::less<int> cmp;
  ASSERT_TRUE(valid_heap(arr, 1, 7, cmp));

  // Make an internal node very small; it should bubble up.
  arr[6] = 0;
  sift_down_up(arr, 1, 6, 7, cmp);
  EXPECT_TRUE(valid_heap(arr, 1, 7, cmp));
  EXPECT_EQ(arr[1], 0);

  // Make an internal node very large; it should sink.
  arr[2] = 100;
  sift_down_up(arr, 1, 2, 7, cmp);
  EXPECT_TRUE(valid_heap(arr, 1, 7, cmp));
}

TEST(ArrayHeapAlgos, HeapsortMatchesStdSort)
{
  std::vector<int> v{5, 1, 4, 2, 8, 0, 3, 7, 6, 9};
  std::vector<int> expected = v;
  std::sort(expected.begin(), expected.end());

  heapsort(v.data(), v.size());
  EXPECT_EQ(v, expected);
}

TEST(ArrayHeapAlgos, FasterHeapsortMatchesStdSort)
{
  std::vector<int> v{12, -1, 5, 5, 3, 99, 0, -10, 7, 2, 4};
  std::vector<int> expected = v;
  std::sort(expected.begin(), expected.end());

  faster_heapsort(v.data(), v.size());
  EXPECT_EQ(v, expected);
}
