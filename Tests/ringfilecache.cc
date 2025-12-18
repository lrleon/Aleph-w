#include <gtest/gtest.h>

#include <ringfilecache.H>

#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <random>
#include <string>
#include <vector>

using namespace Aleph;
using namespace std;
namespace fs = std::filesystem;

namespace {

struct TempPaths
{
  fs::path pars;
  fs::path cache;
};

TempPaths make_temp_paths()
{
  static std::atomic<unsigned long long> counter{0};
  const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
  const auto id = std::to_string(now) + "_" + std::to_string(counter++);

  const fs::path dir = fs::temp_directory_path() / "aleph_ringcache_tests";
  fs::create_directories(dir);
  return {dir / (id + ".pars"), dir / (id + ".cache")};
}

struct TempFiles
{
  TempPaths paths;
  explicit TempFiles(TempPaths p) : paths(std::move(p)) {}
  ~TempFiles()
  {
    std::error_code ec;
    fs::remove(paths.pars, ec);
    fs::remove(paths.cache, ec);
  }
};

template <typename T>
std::vector<T> to_vector(const Array<T> &arr)
{
  std::vector<T> out;
  for (size_t i = 0; i < arr.size(); ++i)
    out.push_back(arr[i]);
  return out;
}

TEST(RingFileCache, basic_put_read_get_and_overflow)
{
  TempFiles tmp(make_temp_paths());
  RingFileCache<int>::create(tmp.paths.pars.string(), tmp.paths.cache.string(), 4);
  ASSERT_TRUE(RingFileCache<int>::test(tmp.paths.pars.string()));

  RingFileCache<int> cache(tmp.paths.pars.string());
  EXPECT_TRUE(cache.is_initialized());
  EXPECT_EQ(cache.capacity(), 4u);
  EXPECT_EQ(cache.size(), 0u);
  EXPECT_TRUE(cache.read_all().is_empty());

  EXPECT_TRUE(cache.put(1));
  EXPECT_TRUE(cache.put(2));
  EXPECT_TRUE(cache.put(3));
  EXPECT_TRUE(cache.put(4));
  EXPECT_FALSE(cache.put(5)); // full
  EXPECT_EQ(cache.size(), 4u);

  int buf[4] = {0, 0, 0, 0};
  EXPECT_TRUE(cache.read(buf, 4));
  EXPECT_EQ(std::vector<int>(buf, buf + 4), std::vector<int>({1, 2, 3, 4}));
  EXPECT_EQ(cache.read_first(), 1);
  EXPECT_EQ(cache.read_last(), 4);
  EXPECT_EQ(cache.oldest(), 1);
  EXPECT_EQ(cache.oldest(2), 3);

  EXPECT_TRUE(cache.get(2));
  EXPECT_EQ(cache.size(), 2u);
  EXPECT_EQ(cache.read_first(), 3);
  EXPECT_EQ(cache.read_last(), 4);
  EXPECT_TRUE(cache.get(2));
  EXPECT_TRUE(cache.is_empty());

  EXPECT_THROW(cache.read_first(), std::underflow_error);
  EXPECT_THROW(cache.read_last(), std::underflow_error);
  EXPECT_FALSE(cache.get(1));          // cannot extract if empty
  EXPECT_FALSE(cache.read(buf, 1));    // cannot read if empty
}

TEST(RingFileCache, read_from_position_pointer_and_iterator)
{
  TempFiles tmp(make_temp_paths());
  RingFileCache<int>::create(tmp.paths.pars.string(), tmp.paths.cache.string(), 5);
  RingFileCache<int> cache(tmp.paths.pars.string());

  for (int v : {10, 20, 30, 40})
    ASSERT_TRUE(cache.put(v));

  auto sub = cache.read_from(1, 2);
  EXPECT_EQ(to_vector(sub), std::vector<int>({20, 30}));

  RingFileCache<int>::Pointer ptr(cache); // at head (10)
  ptr += 2;                               // points to 30
  auto sub_ptr = cache.read_from(ptr, 3); // should stop at available items
  EXPECT_EQ(to_vector(sub_ptr), std::vector<int>({30, 40}));

  std::vector<int> iterated;
  for (auto it = cache.get_it(); it.has_curr(); it.next_ne())
    iterated.push_back(it.get_curr_ne());
  EXPECT_EQ(iterated, std::vector<int>({10, 20, 30, 40}));
}

TEST(RingFileCache, persistence_and_flush)
{
  TempPaths paths = make_temp_paths();
  TempFiles tmp(paths);
  {
    RingFileCache<int>::create(tmp.paths.pars.string(), tmp.paths.cache.string(), 6);
    RingFileCache<int> cache(tmp.paths.pars.string());
    for (int v : {1, 2, 3})
      ASSERT_TRUE(cache.put(v));
    cache.flush();
  }

  RingFileCache<int> reopened(tmp.paths.pars.string());
  EXPECT_EQ(reopened.size(), 3u);
  EXPECT_EQ(reopened.read_first(), 1);
  EXPECT_EQ(reopened.read_last(), 3);
  auto all = reopened.read_all();
  EXPECT_EQ(to_vector(all), std::vector<int>({1, 2, 3}));
  reopened.get(2);
  EXPECT_EQ(reopened.size(), 1u);
  EXPECT_EQ(reopened.read_first(), 3);
}

TEST(RingFileCache, resize_and_append)
{
  TempFiles tmp(make_temp_paths());
  RingFileCache<int>::create(tmp.paths.pars.string(), tmp.paths.cache.string(), 2);
  RingFileCache<int> cache(tmp.paths.pars.string());

  cache.put(7);
  cache.put(8);
  EXPECT_EQ(cache.capacity(), 2u);
  cache.resize(5);
  EXPECT_EQ(cache.capacity(), 5u);
  EXPECT_TRUE(cache.put(9));
  EXPECT_TRUE(cache.put(10));
  EXPECT_TRUE(cache.put(11));

  auto all = cache.read_all();
  EXPECT_EQ(to_vector(all), std::vector<int>({7, 8, 9, 10, 11}));
}

TEST(RingFileCache, read_all_empty_is_safe)
{
  TempFiles tmp(make_temp_paths());
  RingFileCache<int>::create(tmp.paths.pars.string(), tmp.paths.cache.string(), 3);
  RingFileCache<int> cache(tmp.paths.pars.string());

  auto all = cache.read_all();
  EXPECT_TRUE(all.is_empty());
}

TEST(RingFileCache, pointer_offsets_and_wraparound_iteration)
{
  TempFiles tmp(make_temp_paths());
  RingFileCache<int>::create(tmp.paths.pars.string(), tmp.paths.cache.string(), 3);
  RingFileCache<int> cache(tmp.paths.pars.string());

  cache.put(1);
  cache.put(2);
  cache.put(3);

  RingFileCache<int>::Pointer ptr(cache); // at head
  EXPECT_EQ(ptr.get_pos_respect_to_head(), 0u);

  auto first_two = cache.read_from(ptr, 2);
  EXPECT_EQ(to_vector(first_two), std::vector<int>({1, 2}));

  ptr += 1;
  EXPECT_EQ(ptr.get_pos_respect_to_head(), 1u);

  ptr += 5; // wraps around dimension 3
  EXPECT_EQ(ptr.get_pos_respect_to_head(), 0u);

  cache.get(2); // remove 1 and 2
  cache.put(4);
  cache.put(5);

  RingFileCache<int>::Pointer head_ptr(cache);
  auto wrapped = cache.read_from(head_ptr, 3);
  EXPECT_EQ(to_vector(wrapped), std::vector<int>({3, 4, 5}));

  std::vector<int> iterated;
  for (auto it = cache.get_it(); it.has_curr(); it.next_ne())
    iterated.push_back(it.get_curr_ne());
  EXPECT_EQ(iterated, std::vector<int>({3, 4, 5}));
}

TEST(RingFileCache, init_and_close_are_idempotent_and_persistent)
{
  TempPaths paths = make_temp_paths();
  TempFiles tmp(paths);

  RingFileCache<int>::create(tmp.paths.pars.string(), tmp.paths.cache.string(), 4);
  RingFileCache<int> cache;
  cache.init(tmp.paths.pars.string());
  ASSERT_TRUE(cache.is_initialized());
  cache.put(42);
  cache.close();
  cache.close(); // should be a no-op

  RingFileCache<int> reopened(tmp.paths.pars.string());
  EXPECT_EQ(reopened.size(), 1u);
  EXPECT_EQ(reopened.read_first(), 42);
}

TEST(RingFileCache, is_full_method)
{
  TempFiles tmp(make_temp_paths());
  RingFileCache<int>::create(tmp.paths.pars.string(), tmp.paths.cache.string(), 3);
  RingFileCache<int> cache(tmp.paths.pars.string());

  EXPECT_FALSE(cache.is_full());
  EXPECT_TRUE(cache.is_empty());

  cache.put(1);
  EXPECT_FALSE(cache.is_full());
  EXPECT_FALSE(cache.is_empty());

  cache.put(2);
  cache.put(3);
  EXPECT_TRUE(cache.is_full());
  EXPECT_FALSE(cache.is_empty());

  cache.get(1);
  EXPECT_FALSE(cache.is_full());
}

TEST(RingFileCache, wraparound_stress_test)
{
  TempFiles tmp(make_temp_paths());
  RingFileCache<int>::create(tmp.paths.pars.string(), tmp.paths.cache.string(), 4);
  RingFileCache<int> cache(tmp.paths.pars.string());

  // Fill, empty, refill multiple cycles to stress wraparound
  for (int cycle = 0; cycle < 5; ++cycle)
    {
      // Fill completely
      for (int i = 0; i < 4; ++i)
        ASSERT_TRUE(cache.put(cycle * 10 + i));
      EXPECT_TRUE(cache.is_full());

      // Verify contents
      for (int i = 0; i < 4; ++i)
        EXPECT_EQ(cache.oldest(i), cycle * 10 + i);

      // Empty completely
      EXPECT_TRUE(cache.get(4));
      EXPECT_TRUE(cache.is_empty());
    }

  // Partial fill/empty cycles
  for (int cycle = 0; cycle < 10; ++cycle)
    {
      cache.put(cycle);
      cache.put(cycle + 100);
      EXPECT_EQ(cache.size(), 2u);
      EXPECT_EQ(cache.read_first(), cycle);
      EXPECT_EQ(cache.read_last(), cycle + 100);
      cache.get(2);
    }
}

TEST(RingFileCache, resize_with_wrapped_data)
{
  TempFiles tmp(make_temp_paths());
  RingFileCache<int>::create(tmp.paths.pars.string(), tmp.paths.cache.string(), 4);
  RingFileCache<int> cache(tmp.paths.pars.string());

  // Create wraparound: fill, remove some, add more
  cache.put(1);
  cache.put(2);
  cache.put(3);
  cache.put(4);
  cache.get(2);  // remove 1, 2 -> head moves forward
  cache.put(5);
  cache.put(6);  // now data wraps: [5, 6, 3, 4] with head at index 2

  EXPECT_EQ(cache.size(), 4u);
  auto before = cache.read_all();
  EXPECT_EQ(to_vector(before), std::vector<int>({3, 4, 5, 6}));

  // Resize while wrapped
  cache.resize(8);
  EXPECT_EQ(cache.capacity(), 8u);
  EXPECT_EQ(cache.size(), 4u);

  // Data should still be correct after resize
  auto after = cache.read_all();
  EXPECT_EQ(to_vector(after), std::vector<int>({3, 4, 5, 6}));

  // Can add more now
  EXPECT_TRUE(cache.put(7));
  EXPECT_TRUE(cache.put(8));
  EXPECT_TRUE(cache.put(9));
  EXPECT_TRUE(cache.put(10));
  EXPECT_EQ(cache.size(), 8u);
  EXPECT_TRUE(cache.is_full());
}

TEST(RingFileCache, resize_same_size_and_empty)
{
  TempFiles tmp(make_temp_paths());
  RingFileCache<int>::create(tmp.paths.pars.string(), tmp.paths.cache.string(), 4);
  RingFileCache<int> cache(tmp.paths.pars.string());

  // Resize empty cache
  cache.resize(6);
  EXPECT_EQ(cache.capacity(), 6u);
  EXPECT_EQ(cache.size(), 0u);

  // Resize to same size should be no-op (but allowed)
  cache.put(1);
  cache.resize(6);
  EXPECT_EQ(cache.capacity(), 6u);
  EXPECT_EQ(cache.size(), 1u);
  EXPECT_EQ(cache.read_first(), 1);
}

TEST(RingFileCache, iterator_exhaustion_throws)
{
  TempFiles tmp(make_temp_paths());
  RingFileCache<int>::create(tmp.paths.pars.string(), tmp.paths.cache.string(), 3);
  RingFileCache<int> cache(tmp.paths.pars.string());

  cache.put(1);
  cache.put(2);

  auto it = cache.get_it();
  EXPECT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_curr(), 1);
  it.next();
  EXPECT_EQ(it.get_curr(), 2);
  it.next();
  EXPECT_FALSE(it.has_curr());

  // get_curr() should throw when exhausted
  EXPECT_THROW(it.get_curr(), std::overflow_error);
  // next() should throw when exhausted
  EXPECT_THROW(it.next(), std::overflow_error);
}

TEST(RingFileCache, oldest_bounds_check)
{
  TempFiles tmp(make_temp_paths());
  RingFileCache<int>::create(tmp.paths.pars.string(), tmp.paths.cache.string(), 5);
  RingFileCache<int> cache(tmp.paths.pars.string());

  cache.put(10);
  cache.put(20);
  cache.put(30);

  EXPECT_EQ(cache.oldest(0), 10);
  EXPECT_EQ(cache.oldest(1), 20);
  EXPECT_EQ(cache.oldest(2), 30);

  // Out of bounds should throw
  EXPECT_THROW(cache.oldest(3), std::overflow_error);
  EXPECT_THROW(cache.oldest(100), std::overflow_error);
}

TEST(RingFileCache, double_init_throws)
{
  TempFiles tmp(make_temp_paths());
  RingFileCache<int>::create(tmp.paths.pars.string(), tmp.paths.cache.string(), 4);

  RingFileCache<int> cache;
  cache.init(tmp.paths.pars.string());
  EXPECT_TRUE(cache.is_initialized());

  // Second init should throw
  EXPECT_THROW(cache.init(tmp.paths.pars.string()), std::domain_error);
}

TEST(RingFileCache, pointer_negative_arithmetic)
{
  TempFiles tmp(make_temp_paths());
  RingFileCache<int>::create(tmp.paths.pars.string(), tmp.paths.cache.string(), 5);
  RingFileCache<int> cache(tmp.paths.pars.string());

  for (int i = 0; i < 5; ++i)
    cache.put(i * 10);

  RingFileCache<int>::Pointer ptr(cache, 2);  // points to entry with value 20
  EXPECT_EQ(cache.read(ptr), 20);

  ptr -= 1;
  EXPECT_EQ(cache.read(ptr), 10);

  ptr -= 1;
  EXPECT_EQ(cache.read(ptr), 0);

  // Wraparound backward
  ptr -= 1;
  EXPECT_EQ(cache.read(ptr), 40);  // wraps to last entry

  ptr -= 2;
  EXPECT_EQ(cache.read(ptr), 20);

  // Large negative offset
  ptr -= 7;  // 7 % 5 = 2 steps back from 20 -> 0
  EXPECT_EQ(cache.read(ptr), 0);
}

TEST(RingFileCache, pointer_with_negative_delta_via_plus_operator)
{
  TempFiles tmp(make_temp_paths());
  RingFileCache<int>::create(tmp.paths.pars.string(), tmp.paths.cache.string(), 4);
  RingFileCache<int> cache(tmp.paths.pars.string());

  for (int i = 0; i < 4; ++i)
    cache.put(i);

  RingFileCache<int>::Pointer ptr(cache, 2);  // at value 2
  EXPECT_EQ(cache.read(ptr), 2);

  auto ptr2 = ptr + (-1);  // should go back 1
  EXPECT_EQ(cache.read(ptr2), 1);

  ptr2 = ptr + (-3);  // should wrap to value 3
  EXPECT_EQ(cache.read(ptr2), 3);
}

// Test with a non-int trivially copyable type
struct TestRecord
{
  int id;
  double value;
  char tag[8];

  bool operator==(const TestRecord &other) const
  {
    return id == other.id && value == other.value &&
           std::strncmp(tag, other.tag, 8) == 0;
  }
};

TEST(RingFileCache, non_int_trivially_copyable_type)
{
  TempFiles tmp(make_temp_paths());
  RingFileCache<TestRecord>::create(tmp.paths.pars.string(),
                                     tmp.paths.cache.string(), 3);
  RingFileCache<TestRecord> cache(tmp.paths.pars.string());

  TestRecord r1{1, 3.14, "alpha"};
  TestRecord r2{2, 2.71, "beta"};
  TestRecord r3{3, 1.41, "gamma"};

  EXPECT_TRUE(cache.put(r1));
  EXPECT_TRUE(cache.put(r2));
  EXPECT_TRUE(cache.put(r3));
  EXPECT_TRUE(cache.is_full());

  EXPECT_EQ(cache.read_first(), r1);
  EXPECT_EQ(cache.read_last(), r3);
  EXPECT_EQ(cache.oldest(1), r2);

  cache.get(1);
  EXPECT_EQ(cache.read_first(), r2);
}

TEST(RingFileCache, empty_cache_operations)
{
  TempFiles tmp(make_temp_paths());
  RingFileCache<int>::create(tmp.paths.pars.string(), tmp.paths.cache.string(), 5);
  RingFileCache<int> cache(tmp.paths.pars.string());

  EXPECT_TRUE(cache.is_empty());
  EXPECT_FALSE(cache.is_full());
  EXPECT_EQ(cache.size(), 0u);
  EXPECT_EQ(cache.avail(), 5u);

  // These should return false, not throw
  EXPECT_FALSE(cache.get(1));
  int buf[1];
  EXPECT_FALSE(cache.read(buf, 1));

  // read_all on empty returns empty array
  auto all = cache.read_all();
  EXPECT_TRUE(all.is_empty());

  // Iterator on empty cache
  auto it = cache.get_it();
  EXPECT_FALSE(it.has_curr());
}

TEST(RingFileCache, read_from_beyond_available)
{
  TempFiles tmp(make_temp_paths());
  RingFileCache<int>::create(tmp.paths.pars.string(), tmp.paths.cache.string(), 10);
  RingFileCache<int> cache(tmp.paths.pars.string());

  cache.put(1);
  cache.put(2);
  cache.put(3);

  // read_from with pos beyond size should return partial or empty
  auto result = cache.read_from(5, 3);  // pos=5 but only 3 items exist
  EXPECT_TRUE(result.is_empty());

  // read_from with large m should stop at available items
  auto result2 = cache.read_from(1, 100);  // start at 1, request 100, only 2 available
  EXPECT_EQ(to_vector(result2), std::vector<int>({2, 3}));
}

TEST(RingFileCache, persistence_after_wraparound)
{
  TempPaths paths = make_temp_paths();
  TempFiles tmp(paths);

  {
    RingFileCache<int>::create(tmp.paths.pars.string(), tmp.paths.cache.string(), 4);
    RingFileCache<int> cache(tmp.paths.pars.string());

    // Create wraparound state
    cache.put(1);
    cache.put(2);
    cache.put(3);
    cache.put(4);
    cache.get(2);  // remove 1, 2
    cache.put(5);
    cache.put(6);
    // State: [5, 6, 3, 4] with head at 2

    cache.flush();
  }

  // Reopen and verify wraparound state persisted correctly
  RingFileCache<int> reopened(tmp.paths.pars.string());
  EXPECT_EQ(reopened.size(), 4u);
  auto all = reopened.read_all();
  EXPECT_EQ(to_vector(all), std::vector<int>({3, 4, 5, 6}));
}

} // namespace
