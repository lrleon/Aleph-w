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

#include <tpl_file_b_map.H>

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

    const fs::path dir = fs::temp_directory_path() / "aleph_file_bmap_tests";
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

TEST(FileBMap, InsertFindAssignAndReopen)
{
  TempFile tmp(make_temp_path());

  {
    File_B_Map<int, long, Aleph::less<int>, 3> map(tmp.path.string(), false);
    EXPECT_TRUE(map.insert(10, 100));
    EXPECT_TRUE(map.insert(20, 200));
    EXPECT_FALSE(map.insert(10, 999));
    ASSERT_TRUE(map.find(10).has_value());
    EXPECT_EQ(*map.find(10), 100);
    EXPECT_FALSE(map.insert_or_assign(10, 150));
    EXPECT_EQ(map.at(10), 150);
    EXPECT_TRUE(map.verify());
    map.sync();
  }

  File_B_Map<int, long, Aleph::less<int>, 3> reopened(
    tmp.path.string(),
    File_B_Map<int, long, Aleph::less<int>, 3>::Read_Only);
  EXPECT_TRUE(reopened.is_read_only());
  ASSERT_TRUE(reopened.find(20).has_value());
  EXPECT_EQ(*reopened.find(20), 200);
  EXPECT_EQ(to_vector(reopened.items()),
            (std::vector<std::pair<int, long>>{{10, 150}, {20, 200}}));
}

TEST(FileBMap, BoundsItemsAndRemove)
{
  TempFile tmp(make_temp_path());

  File_B_Map<int, int, Aleph::less<int>, 3> map(tmp.path.string(), false);
  EXPECT_TRUE(map.insert(30, 300));
  EXPECT_TRUE(map.insert(10, 100));
  EXPECT_TRUE(map.insert(50, 500));
  EXPECT_TRUE(map.insert(40, 400));

  EXPECT_EQ(map.min_item(), (std::optional<std::pair<int, int>>{{10, 100}}));
  EXPECT_EQ(map.max_item(), (std::optional<std::pair<int, int>>{{50, 500}}));
  EXPECT_EQ(map.lower_bound(35), (std::optional<std::pair<int, int>>{{40, 400}}));
  EXPECT_EQ(map.upper_bound(40), (std::optional<std::pair<int, int>>{{50, 500}}));
  EXPECT_EQ(to_vector(map.keys()), (std::vector<int>{10, 30, 40, 50}));
  EXPECT_EQ(to_vector(map.values()), (std::vector<int>{100, 300, 400, 500}));

  EXPECT_TRUE(map.remove(30));
  EXPECT_FALSE(map.contains(30));
  EXPECT_EQ(to_vector(map.items()),
            (std::vector<std::pair<int, int>>{{10, 100}, {40, 400}, {50, 500}}));
  EXPECT_TRUE(map.verify());
}

TEST(FileBMap, SupportsBoundedStringCodecs)
{
  TempFile tmp(make_temp_path());
  using key_codec = Aleph::detail::Paged_Bounded_String_Codec<16>;
  using value_codec = Aleph::detail::Paged_Bounded_String_Codec<32>;

  {
    File_B_Map<std::string, std::string, Aleph::less<std::string>, 3,
               key_codec, value_codec> map(tmp.path.string(), false);
    EXPECT_TRUE(map.insert("apple", "green"));
    EXPECT_TRUE(map.insert("banana", "yellow"));
    EXPECT_TRUE(map.insert("kiwi", "brown fuzz"));
    EXPECT_FALSE(map.insert_or_assign("banana", "ripe yellow"));
    EXPECT_EQ(map.at("banana"), "ripe yellow");
    EXPECT_EQ(map.lower_bound(std::string("banana")),
              (std::optional<std::pair<std::string, std::string>>{
                {"banana", "ripe yellow"}}));
    EXPECT_TRUE(map.verify());
    map.sync();
  }

  File_B_Map<std::string, std::string, Aleph::less<std::string>, 3,
             key_codec, value_codec> reopened(
    tmp.path.string(),
    File_B_Map<std::string, std::string, Aleph::less<std::string>, 3,
               key_codec, value_codec>::Read_Only);
  EXPECT_TRUE(reopened.verify());
  EXPECT_EQ(to_vector(reopened.items()),
            (std::vector<std::pair<std::string, std::string>>{
              {"apple", "green"},
              {"banana", "ripe yellow"},
              {"kiwi", "brown fuzz"}}));
}
