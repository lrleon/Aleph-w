/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w
*/

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <optional>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include <tpl_file_bplus_map.H>

using namespace Aleph;

namespace
{
  namespace fs = std::filesystem;

  fs::path make_temp_path()
  {
    static std::atomic<unsigned long long> counter{0};
    // Use a per-process random generator to avoid filename collisions
    // across parallel processes.
    static std::mt19937_64 rng{[] {
        std::random_device rd;
        const auto now_seed =
          static_cast<unsigned long long>
          (std::chrono::steady_clock::now().time_since_epoch().count());
        return std::mt19937_64{now_seed ^
          (static_cast<unsigned long long>(rd()) << 1u)};
      }()};

    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    std::uniform_int_distribution<unsigned long long> dist;
    const auto random_value = dist(rng);

    const auto id = std::to_string(now) + "_" +
                    std::to_string(random_value) + "_" +
                    std::to_string(counter++);

    const fs::path dir = fs::temp_directory_path() / "aleph_file_bplusmap_tests";
    fs::create_directories(dir);
    return dir / (id + ".idx");
  }

  struct TempFile
  {
    fs::path path;

    explicit TempFile(fs::path p) : path(std::move(p)) {}

    ~TempFile()
    {
      std::error_code ec;
      fs::remove(path, ec);
      fs::remove(path.string() + ".wal", ec);
      fs::remove(path.string() + ".wal.tmp", ec);
      fs::remove(path.string() + ".journal", ec);
      fs::remove(path.string() + ".journal.tmp", ec);
      fs::remove(path.string() + ".lock", ec);
    }
  };

  template <typename T>
  std::vector<T> to_vector(const Array<T> & arr)
  {
    std::vector<T> out;
    out.reserve(arr.size());
    for (size_t i = 0; i < arr.size(); ++i)
      out.push_back(arr[i]);
    return out;
  }
} // namespace

TEST(FileBPlusMap, RangeAndReopen)
{
  TempFile tmp(make_temp_path());

  {
    File_BPlus_Map<int, int, Aleph::less<int>, 3> map(tmp.path.string(), false);
    EXPECT_TRUE(map.insert(10, 100));
    EXPECT_TRUE(map.insert(20, 200));
    EXPECT_TRUE(map.insert(30, 300));
    EXPECT_TRUE(map.insert(40, 400));
    EXPECT_TRUE(map.insert(50, 500));
    EXPECT_EQ(to_vector(map.range(15, 45)),
              (std::vector<std::pair<int, int>>{{20, 200}, {30, 300}, {40, 400}}));
    EXPECT_TRUE(map.verify());
    map.sync();
  }

  File_BPlus_Map<int, int, Aleph::less<int>, 3> reopened(
    tmp.path.string(),
    File_BPlus_Map<int, int, Aleph::less<int>, 3>::Read_Only);
  EXPECT_TRUE(reopened.is_read_only());
  EXPECT_EQ(to_vector(reopened.range(10, 30)),
            (std::vector<std::pair<int, int>>{{10, 100}, {20, 200}, {30, 300}}));
}

TEST(FileBPlusMap, AssignBoundsAndErase)
{
  TempFile tmp(make_temp_path());

  File_BPlus_Map<int, long, Aleph::less<int>, 3> map(tmp.path.string(), false);
  EXPECT_TRUE(map.insert(5, 50));
  EXPECT_TRUE(map.insert(15, 150));
  EXPECT_TRUE(map.insert(25, 250));
  EXPECT_TRUE(map.insert(35, 350));
  EXPECT_FALSE(map.insert_or_assign(25, 255));
  EXPECT_EQ(map.at(25), 255);

  EXPECT_EQ(map.lower_bound(16), (std::optional<std::pair<int, long>>{{25, 255}}));
  EXPECT_EQ(map.upper_bound(25), (std::optional<std::pair<int, long>>{{35, 350}}));
  EXPECT_EQ(to_vector(map.items()),
            (std::vector<std::pair<int, long>>{{5, 50}, {15, 150}, {25, 255}, {35, 350}}));

  EXPECT_TRUE(map.remove(15));
  EXPECT_FALSE(map.contains(15));
  EXPECT_EQ(to_vector(map.values()), (std::vector<long>{50, 255, 350}));
  EXPECT_TRUE(map.verify());
}

TEST(FileBPlusMap, LazyIteratorsTraverseOrderedPairs)
{
  TempFile tmp(make_temp_path());

  File_BPlus_Map<int, int, Aleph::less<int>, 3> map(tmp.path.string(), false);
  for (const auto & [key, value] : std::vector<std::pair<int, int>>{
         {101, 750}, {105, 920}, {110, 875}, {120, 990}, {130, 1010}})
    EXPECT_TRUE(map.insert(key, value));

  std::vector<std::pair<int, int>> all_items;
  for (auto it = map.get_it(); it.has_curr(); it.next_ne())
    all_items.push_back(it.get_curr());
  EXPECT_EQ(all_items,
            (std::vector<std::pair<int, int>>{
              {101, 750}, {105, 920}, {110, 875}, {120, 990}, {130, 1010}}));

  std::vector<std::pair<int, int>> band;
  auto it = map.get_range_it(104, 120);
  ASSERT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_key(), 105);
  EXPECT_EQ(it.get_value(), 920);
  for (; it.has_curr(); it.next())
    band.push_back(it.get_curr());

  EXPECT_EQ(band,
            (std::vector<std::pair<int, int>>{
              {105, 920}, {110, 875}, {120, 990}}));
}

TEST(FileBPlusMap, SupportsBoundedStringCodecs)
{
  TempFile tmp(make_temp_path());
  using key_codec = Aleph::detail::Paged_Bounded_String_Codec<16>;
  using value_codec = Aleph::detail::Paged_Bounded_String_Codec<32>;

  {
    File_BPlus_Map<std::string, std::string, Aleph::less<std::string>, 3,
                   key_codec, value_codec> map(tmp.path.string(), false);
    EXPECT_TRUE(map.insert("apple", "green"));
    EXPECT_TRUE(map.insert("banana", "yellow"));
    EXPECT_TRUE(map.insert("kiwi", "brown fuzz"));
    EXPECT_FALSE(map.insert_or_assign("banana", "ripe yellow"));
    EXPECT_EQ(map.at("banana"), "ripe yellow");
    EXPECT_EQ(to_vector(map.range(std::string("banana"), std::string("kiwi"))),
              (std::vector<std::pair<std::string, std::string>>{
                {"banana", "ripe yellow"}, {"kiwi", "brown fuzz"}}));
    EXPECT_TRUE(map.verify());
    map.sync();
  }

  File_BPlus_Map<std::string, std::string, Aleph::less<std::string>, 3,
                 key_codec, value_codec> reopened(
    tmp.path.string(),
    File_BPlus_Map<std::string, std::string, Aleph::less<std::string>, 3,
                   key_codec, value_codec>::Read_Only);
  EXPECT_TRUE(reopened.verify());
  EXPECT_EQ(to_vector(reopened.items()),
            (std::vector<std::pair<std::string, std::string>>{
              {"apple", "green"},
              {"banana", "ripe yellow"},
              {"kiwi", "brown fuzz"}}));

  std::vector<std::pair<std::string, std::string>> lazy_items;
  for (auto it = reopened.get_it(); it.has_curr(); it.next())
    lazy_items.push_back(it.get_curr());
  EXPECT_EQ(lazy_items, to_vector(reopened.items()));
}
