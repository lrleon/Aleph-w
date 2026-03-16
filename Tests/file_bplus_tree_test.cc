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
 * @file file_bplus_tree_test.cc
 * @brief Tests for page-managed File_BPlus_Tree
 */

#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <bit>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <tpl_file_b_tree.H>
#include <tpl_file_bplus_tree.H>

#if defined(__unix__) || defined(__APPLE__)
# include <sys/wait.h>
# include <unistd.h>
#endif

using namespace Aleph;

namespace
{
  namespace fs = std::filesystem;

  fs::path make_temp_path()
  {
    static std::atomic<unsigned long long> counter{0};
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto id = std::to_string(now) + "_" + std::to_string(counter++);
    const fs::path dir = fs::temp_directory_path() / "aleph_file_bplustree_tests";
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
      fs::remove(path.string() + ".old", ec);
      fs::remove(path.string() + ".new", ec);
    }
  };

  template <typename T>
  std::vector<T> to_vector(const Array<T> & arr)
  {
    std::vector<T> ret;
    ret.reserve(arr.size());
    for (size_t i = 0; i < arr.size(); ++i)
      ret.push_back(arr[i]);
    return ret;
  }

#if defined(__unix__) || defined(__APPLE__)
  constexpr bool running_under_tsan() noexcept
  {
# if defined(__SANITIZE_THREAD__)
    return true;
# elif defined(__has_feature)
#   if __has_feature(thread_sanitizer)
    return true;
#   else
    return false;
#   endif
# else
    return false;
# endif
  }

  template <typename F>
  int run_in_child(F && fn)
  {
    const pid_t pid = ::fork();
    if (pid < 0)
      return -1;

    if (pid == 0)
      {
        int code = 100;
        try
          {
            code = fn();
          }
        catch (...)
          {
            code = 101;
          }
        ::_exit(code);
      }

    int status = 0;
    if (::waitpid(pid, &status, 0) < 0)
      return -1;
    return status;
  }
#endif

  constexpr std::uint32_t file_bplus_tree_test_file_version = 5;
  constexpr std::uint32_t file_bplus_tree_test_wal_version = 4;
  constexpr std::uint32_t file_bplus_tree_test_key_size =
    Aleph::detail::Paged_Value_Codec<int>::encoded_size;
  constexpr std::uint64_t file_bplus_tree_test_min_degree = 3;
  constexpr std::uint8_t file_bplus_tree_test_encoding = 3;

  std::array<char, 7> file_bplus_tree_reserved_bytes()
  {
    std::array<char, 7> reserved = {};
    const auto codec_id = Aleph::detail::Paged_Value_Codec<int>::storage_id;
    reserved[0] = static_cast<char>(codec_id & 0xFFu);
    reserved[1] = static_cast<char>((codec_id >> 8) & 0xFFu);
    reserved[2] = static_cast<char>((codec_id >> 16) & 0xFFu);
    reserved[3] = static_cast<char>((codec_id >> 24) & 0xFFu);
    return reserved;
  }

  std::uint32_t bplus_wal_checksum(const std::uint32_t wal_version,
                                   const std::uint64_t size,
                                   const std::uint64_t root_page,
                                   const std::uint64_t first_leaf_page,
                                   const std::uint64_t page_count,
                                   const std::uint64_t free_page_head,
                                   const std::uint64_t checkpoint_sequence,
                                   const std::uint64_t dirty_count)
  {
    std::uint32_t crc = Aleph::detail::crc32_begin();
    const auto reserved = file_bplus_tree_reserved_bytes();

    crc = Aleph::detail::crc32_add(crc, wal_version);
    crc = Aleph::detail::crc32_add(crc, file_bplus_tree_test_key_size);
    crc = Aleph::detail::crc32_add(crc, file_bplus_tree_test_min_degree);
    crc = Aleph::detail::crc32_add(crc, file_bplus_tree_test_encoding);
    crc = Aleph::detail::crc32_add_bytes(crc, reserved.data(), reserved.size());
    crc = Aleph::detail::crc32_add(crc, size);
    crc = Aleph::detail::crc32_add(crc, root_page);
    crc = Aleph::detail::crc32_add(crc, first_leaf_page);
    crc = Aleph::detail::crc32_add(crc, page_count);
    crc = Aleph::detail::crc32_add(crc, free_page_head);
    crc = Aleph::detail::crc32_add(crc, checkpoint_sequence);
    crc = Aleph::detail::crc32_add(crc, dirty_count);
    return Aleph::detail::crc32_finish(crc);
  }

  std::uint32_t bplus_wal_payload_checksum(const std::uint64_t page_count,
                                           const size_t page_bytes,
                                           const std::vector<char> & page_blob)
  {
    std::uint32_t crc = Aleph::detail::crc32_begin();
    for (std::uint64_t page_id = 1; page_id <= page_count; ++page_id)
      {
        crc = Aleph::detail::crc32_add(crc, page_id);
        crc = Aleph::detail::crc32_add_bytes(
          crc, page_blob.data() + (page_id - 1) * page_bytes, page_bytes);
      }
    return Aleph::detail::crc32_finish(crc);
  }

  void write_bplus_wal_from_file(const fs::path & data_path,
                                 const fs::path & wal_path)
  {
    constexpr std::uint32_t file_version = file_bplus_tree_test_file_version;
    constexpr std::uint32_t wal_version = file_bplus_tree_test_wal_version;
    constexpr std::uint32_t key_size = file_bplus_tree_test_key_size;
    constexpr std::uint64_t min_degree = file_bplus_tree_test_min_degree;
    constexpr std::uint8_t encoding = file_bplus_tree_test_encoding;
    constexpr size_t max_keys = 5;
    constexpr size_t max_children = 6;
    constexpr size_t page_bytes =
      sizeof(std::uint8_t) + sizeof(std::uint16_t) * 2 + 3
      + sizeof(std::uint64_t) + sizeof(std::uint64_t) + sizeof(int) * max_keys
      + sizeof(std::uint64_t) * max_children + sizeof(std::uint32_t);

    std::ifstream in(data_path, std::ios::binary);
    ASSERT_TRUE(in.good());

    std::array<char, Aleph::detail::Ordered_Tree_Snapshot_Magic_Size> magic = {};
    in.read(magic.data(), magic.size());
    ASSERT_TRUE(in.good());

    auto read_u32 = [&](std::uint32_t & value)
      {
        in.read(reinterpret_cast<char *>(&value), sizeof(value));
        ASSERT_TRUE(in.good());
      };
    auto read_u64 = [&](std::uint64_t & value)
      {
        in.read(reinterpret_cast<char *>(&value), sizeof(value));
        ASSERT_TRUE(in.good());
      };
    auto read_u8 = [&](std::uint8_t & value)
      {
        in.read(reinterpret_cast<char *>(&value), sizeof(value));
        ASSERT_TRUE(in.good());
      };

    std::uint32_t version = 0;
    std::uint32_t file_key_size = 0;
    std::uint64_t file_min_degree = 0;
    std::uint8_t file_encoding = 0;
    read_u32(version);
    read_u32(file_key_size);
    read_u64(file_min_degree);
    read_u8(file_encoding);
    ASSERT_EQ(version, file_version);
    ASSERT_EQ(file_key_size, key_size);
    ASSERT_EQ(file_min_degree, min_degree);
    ASSERT_EQ(file_encoding, encoding);

    const auto reserved = file_bplus_tree_reserved_bytes();
    std::array<char, 7> stored_reserved = {};
    in.read(stored_reserved.data(), stored_reserved.size());
    ASSERT_TRUE(in.good());
    ASSERT_EQ(stored_reserved, reserved);

    std::uint64_t size = 0;
    std::uint64_t root_page = 0;
    std::uint64_t first_leaf_page = 0;
    std::uint64_t page_count = 0;
    std::uint64_t free_page_head = 0;
    std::uint64_t checkpoint_sequence = 0;
    std::uint32_t checksum = 0;
    read_u64(size);
    read_u64(root_page);
    read_u64(first_leaf_page);
    read_u64(page_count);
    read_u64(free_page_head);
    read_u64(checkpoint_sequence);
    read_u32(checksum);

    std::vector<char> page_blob(page_bytes * page_count);
    in.read(page_blob.data(), page_blob.size());
    ASSERT_TRUE(in.good());

    std::ofstream out(wal_path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(out.good());

    const auto wal_magic =
      Aleph::detail::ordered_tree_snapshot_magic("AlephBPTWalv1");
    const auto trailer_magic =
      Aleph::detail::ordered_tree_snapshot_magic("AlephBPTCom1");
    out.write(wal_magic.data(), wal_magic.size());
    out.write(reinterpret_cast<const char *>(&wal_version), sizeof(wal_version));
    out.write(reinterpret_cast<const char *>(&key_size), sizeof(key_size));
    out.write(reinterpret_cast<const char *>(&min_degree), sizeof(min_degree));
    out.write(reinterpret_cast<const char *>(&encoding), sizeof(encoding));
    out.write(reserved.data(), reserved.size());
    out.write(reinterpret_cast<const char *>(&size), sizeof(size));
    out.write(reinterpret_cast<const char *>(&root_page), sizeof(root_page));
    out.write(reinterpret_cast<const char *>(&first_leaf_page), sizeof(first_leaf_page));
    out.write(reinterpret_cast<const char *>(&page_count), sizeof(page_count));
    out.write(reinterpret_cast<const char *>(&free_page_head), sizeof(free_page_head));
    out.write(reinterpret_cast<const char *>(&checkpoint_sequence), sizeof(checkpoint_sequence));
    out.write(reinterpret_cast<const char *>(&page_count), sizeof(page_count));
    const auto wal_checksum = bplus_wal_checksum(
      wal_version,
      size, root_page, first_leaf_page, page_count, free_page_head,
      checkpoint_sequence, page_count);
    out.write(reinterpret_cast<const char *>(&wal_checksum), sizeof(wal_checksum));

    for (std::uint64_t page_id = 1; page_id <= page_count; ++page_id)
      {
        out.write(reinterpret_cast<const char *>(&page_id), sizeof(page_id));
        out.write(page_blob.data() + (page_id - 1) * page_bytes, page_bytes);
      }

    const auto payload_checksum =
      bplus_wal_payload_checksum(page_count, page_bytes, page_blob);
    out.write(trailer_magic.data(), trailer_magic.size());
    out.write(reinterpret_cast<const char *>(&checkpoint_sequence),
              sizeof(checkpoint_sequence));
    out.write(reinterpret_cast<const char *>(&page_count), sizeof(page_count));
    out.write(reinterpret_cast<const char *>(&payload_checksum),
              sizeof(payload_checksum));

    ASSERT_TRUE(out.good());
  }

  std::uint64_t read_bplus_page_count(const fs::path & data_path)
  {
    std::ifstream in(data_path, std::ios::binary);
    EXPECT_TRUE(in.good());
    if (not in.good())
      return 0;

    in.seekg(Aleph::detail::Ordered_Tree_Snapshot_Magic_Size
             + sizeof(std::uint32_t) * 2 + sizeof(std::uint64_t)
             + sizeof(std::uint8_t) + 7 + sizeof(std::uint64_t) * 3);
    EXPECT_TRUE(in.good());
    std::uint64_t page_count = 0;
    in.read(reinterpret_cast<char *>(&page_count), sizeof(page_count));
    EXPECT_TRUE(in.good());
    return page_count;
  }

  std::vector<std::uint64_t> read_bplus_wal_page_ids(const fs::path & wal_path)
  {
    constexpr size_t page_bytes =
      sizeof(std::uint8_t) + sizeof(std::uint16_t) * 2 + 3
      + sizeof(std::uint64_t) + sizeof(std::uint64_t) + sizeof(int) * 5
      + sizeof(std::uint64_t) * 6 + sizeof(std::uint32_t);

    std::ifstream in(wal_path, std::ios::binary);
    EXPECT_TRUE(in.good());
    std::vector<std::uint64_t> page_ids;
    if (not in.good())
      return page_ids;

    auto read_u32 = [&](std::uint32_t & value)
      {
        in.read(reinterpret_cast<char *>(&value), sizeof(value));
        EXPECT_TRUE(in.good());
      };
    auto read_u64 = [&](std::uint64_t & value)
      {
        in.read(reinterpret_cast<char *>(&value), sizeof(value));
        EXPECT_TRUE(in.good());
      };
    auto read_u8 = [&](std::uint8_t & value)
      {
        in.read(reinterpret_cast<char *>(&value), sizeof(value));
        EXPECT_TRUE(in.good());
      };

    std::array<char, Aleph::detail::Ordered_Tree_Snapshot_Magic_Size> magic = {};
    std::array<char, 7> reserved = {};
    std::array<char, Aleph::detail::Ordered_Tree_Snapshot_Magic_Size>
      trailer_magic = {};
    std::uint32_t wal_version = 0;
    std::uint32_t key_size = 0;
    std::uint64_t min_degree = 0;
    std::uint8_t encoding = 0;
    std::uint64_t ignored64 = 0;
    std::uint32_t ignored32 = 0;
    std::uint64_t dirty_count = 0;

    in.read(magic.data(), magic.size());
    EXPECT_TRUE(in.good());
    read_u32(wal_version);
    read_u32(key_size);
    read_u64(min_degree);
    read_u8(encoding);
    in.read(reserved.data(), reserved.size());
    EXPECT_TRUE(in.good());
    read_u64(ignored64); // size
    read_u64(ignored64); // root page
    read_u64(ignored64); // first leaf
    read_u64(ignored64); // page count
    read_u64(ignored64); // free page head
    read_u64(ignored64); // checkpoint sequence
    read_u64(dirty_count);
    read_u32(ignored32); // header checksum

    page_ids.reserve(dirty_count);
    for (std::uint64_t i = 0; i < dirty_count; ++i)
      {
        std::uint64_t page_id = 0;
        read_u64(page_id);
        page_ids.push_back(page_id);
        in.seekg(static_cast<std::streamoff>(page_bytes), std::ios::cur);
        EXPECT_TRUE(in.good());
      }

    in.read(trailer_magic.data(), trailer_magic.size());
    EXPECT_TRUE(in.good());
    return page_ids;
  }

  void copy_region(const fs::path & src, const fs::path & dst,
                   const std::streamoff offset, const size_t size)
  {
    std::ifstream in(src, std::ios::binary);
    ASSERT_TRUE(in.good());
    std::fstream out(dst, std::ios::binary | std::ios::in | std::ios::out);
    ASSERT_TRUE(out.good());

    std::vector<char> buffer(size);
    in.seekg(offset);
    ASSERT_TRUE(in.good());
    in.read(buffer.data(), buffer.size());
    ASSERT_TRUE(in.good());

    out.seekp(offset);
    ASSERT_TRUE(out.good());
    out.write(buffer.data(), buffer.size());
    ASSERT_TRUE(out.good());
  }
}

TEST(FileBPlusTree, ReopensAndPreservesRangeQueries)
{
  TempFile tmp(make_temp_path());

  {
    File_BPlus_Tree<int, Aleph::less<int>, 3> tree(tmp.path.string());
    for (int value : {105, 110, 115, 120, 125, 130, 135, 140, 145, 150})
      EXPECT_TRUE(tree.insert(value));

    EXPECT_TRUE(tree.verify());
    EXPECT_EQ(to_vector(tree.range(118, 142)),
              (std::vector<int>{120, 125, 130, 135, 140}));
  }

  File_BPlus_Tree<int, Aleph::less<int>, 3> reopened(tmp.path.string());
  EXPECT_TRUE(reopened.verify());
  EXPECT_EQ(to_vector(reopened.keys()),
            (std::vector<int>{105, 110, 115, 120, 125, 130, 135, 140, 145, 150}));
  EXPECT_EQ(to_vector(reopened.range(118, 142)),
            (std::vector<int>{120, 125, 130, 135, 140}));
  EXPECT_EQ(reopened.lower_bound(126), std::optional<int>(130));
  EXPECT_EQ(reopened.upper_bound(140), std::optional<int>(145));
}

TEST(FileBPlusTree, ReloadRestoresPersistedLeafSequence)
{
  TempFile tmp(make_temp_path());

  File_BPlus_Tree<int, Aleph::less<int>, 3> tree(tmp.path.string(), false);
  for (int value : {10, 20, 30, 40, 50})
    EXPECT_TRUE(tree.insert(value));
  tree.sync();

  EXPECT_TRUE(tree.insert(60));
  EXPECT_TRUE(tree.remove(20));
  EXPECT_EQ(to_vector(tree.range(10, 70)),
            (std::vector<int>{10, 30, 40, 50, 60}));

  tree.reload();
  EXPECT_EQ(to_vector(tree.range(10, 70)),
            (std::vector<int>{10, 20, 30, 40, 50}));
  EXPECT_TRUE(tree.verify());
}

TEST(FileBPlusTree, LazyIteratorsTraverseLeafChain)
{
  TempFile tmp(make_temp_path());

  File_BPlus_Tree<int, Aleph::less<int>, 3> tree(tmp.path.string(), false);
  for (int value : {105, 110, 115, 120, 125, 130, 135, 140, 145, 150})
    EXPECT_TRUE(tree.insert(value));

  std::vector<int> all_keys;
  for (auto it = tree.get_it(); it.has_curr(); it.next_ne())
    all_keys.push_back(it.get_curr());
  EXPECT_EQ(all_keys,
            (std::vector<int>{105, 110, 115, 120, 125, 130, 135, 140, 145, 150}));

  std::vector<int> band;
  auto band_it = tree.get_range_it(118, 141);
  ASSERT_TRUE(band_it.has_curr());
  EXPECT_EQ(*band_it, 120);
  EXPECT_EQ(band_it.get_curr(), 120);
  band.push_back(band_it.get_curr());
  band_it.next();
  for (; band_it.has_curr(); band_it.next_ne())
    band.push_back(band_it.get_curr());

  EXPECT_EQ(band, (std::vector<int>{120, 125, 130, 135, 140}));
}

TEST(FileBPlusTree, SupportsBoundedStringCodec)
{
  TempFile tmp(make_temp_path());
  using codec_type = Aleph::detail::Paged_Bounded_String_Codec<24>;
  const std::vector<std::string> expected = {
    "alfa", "beta", "delta", "epsilon", "gama"
  };

  {
    File_BPlus_Tree<std::string, Aleph::less<std::string>, 3, codec_type>
      tree(tmp.path.string(), false);
    EXPECT_TRUE(tree.insert("delta"));
    EXPECT_TRUE(tree.insert("alfa"));
    EXPECT_TRUE(tree.insert("gama"));
    EXPECT_TRUE(tree.insert("beta"));
    EXPECT_TRUE(tree.insert("epsilon"));
    EXPECT_TRUE(tree.verify());
    EXPECT_EQ(to_vector(tree.range(std::string("beta"), std::string("epsilon"))),
              (std::vector<std::string>{"beta", "delta", "epsilon"}));
    tree.sync();
  }

  File_BPlus_Tree<std::string, Aleph::less<std::string>, 3, codec_type>
    reopened(tmp.path.string(),
             File_BPlus_Tree<std::string, Aleph::less<std::string>, 3,
                             codec_type>::Read_Only);
  EXPECT_TRUE(reopened.verify());
  EXPECT_EQ(to_vector(reopened.keys()), expected);

  std::vector<std::string> band;
  for (auto it = reopened.get_range_it(std::string("beta"),
                                       std::string("epsilon"));
       it.has_curr(); it.next())
    band.push_back(it.get_curr());
  EXPECT_EQ(band, (std::vector<std::string>{"beta", "delta", "epsilon"}));
}

TEST(FileBPlusTree, BoundedStringCodecRejectsOversizedKeys)
{
  TempFile tmp(make_temp_path());
  using codec_type = Aleph::detail::Paged_Bounded_String_Codec<8>;

  File_BPlus_Tree<std::string, Aleph::less<std::string>, 3, codec_type>
    tree(tmp.path.string(), false);
  EXPECT_TRUE(tree.insert("demasiado-largo"));
  EXPECT_THROW(tree.sync(), std::runtime_error);
  EXPECT_TRUE(tree.verify());
}

TEST(FileBPlusTree, CheckpointSequenceAdvancesAndPersists)
{
  TempFile tmp(make_temp_path());

  std::uint64_t first = 0;
  std::uint64_t second = 0;

  {
    File_BPlus_Tree<int, Aleph::less<int>, 3> tree(tmp.path.string(), false);
    EXPECT_EQ(tree.checkpoint_sequence(), 1u);
    EXPECT_TRUE(tree.insert(10));
    tree.checkpoint();
    first = tree.checkpoint_sequence();
    EXPECT_GT(first, 1u);

    EXPECT_TRUE(tree.insert(20));
    tree.sync();
    second = tree.checkpoint_sequence();
    EXPECT_GT(second, first);
  }

  File_BPlus_Tree<int, Aleph::less<int>, 3> reopened(tmp.path.string());
  EXPECT_EQ(reopened.checkpoint_sequence(), second);
}

TEST(FileBPlusTree, SecondOpenSameProcessIsRejected)
{
  TempFile tmp(make_temp_path());

  File_BPlus_Tree<int, Aleph::less<int>, 3> tree(tmp.path.string(), false);
  EXPECT_THROW((File_BPlus_Tree<int, Aleph::less<int>, 3>(tmp.path.string(), false)),
               std::runtime_error);
}

TEST(FileBPlusTree, ReadOnlySharedOpensAreAllowedAndBlockWriter)
{
  TempFile tmp(make_temp_path());

  {
    File_BPlus_Tree<int, Aleph::less<int>, 3> tree(tmp.path.string(), false);
    for (int value : {10, 20, 30, 40})
      EXPECT_TRUE(tree.insert(value));
    tree.sync();
  }

  File_BPlus_Tree<int, Aleph::less<int>, 3> reader1(
    tmp.path.string(), File_BPlus_Tree<int, Aleph::less<int>, 3>::Read_Only);
  File_BPlus_Tree<int, Aleph::less<int>, 3> reader2(
    tmp.path.string(), File_BPlus_Tree<int, Aleph::less<int>, 3>::Read_Only);
  const auto read_only_mode =
    File_BPlus_Tree<int, Aleph::less<int>, 3>::Read_Only;

  EXPECT_TRUE(reader1.is_read_only());
  EXPECT_EQ(reader1.open_mode(), read_only_mode);
  EXPECT_EQ(to_vector(reader2.range(0, 50)), (std::vector<int>{10, 20, 30, 40}));
  EXPECT_THROW((File_BPlus_Tree<int, Aleph::less<int>, 3>(
                 tmp.path.string(),
                 File_BPlus_Tree<int, Aleph::less<int>, 3>::Read_Write)),
               std::runtime_error);
}

TEST(FileBPlusTree, ReadOnlyRejectsMutations)
{
  TempFile tmp(make_temp_path());

  {
    File_BPlus_Tree<int, Aleph::less<int>, 3> tree(tmp.path.string(), false);
    EXPECT_TRUE(tree.insert(10));
    tree.sync();
  }

  File_BPlus_Tree<int, Aleph::less<int>, 3> reader(
    tmp.path.string(), File_BPlus_Tree<int, Aleph::less<int>, 3>::Read_Only);

  EXPECT_THROW(reader.insert(20), std::runtime_error);
  EXPECT_THROW(reader.remove(10), std::runtime_error);
  EXPECT_THROW(reader.clear(), std::runtime_error);
  EXPECT_THROW(reader.sync(), std::runtime_error);
  EXPECT_EQ(to_vector(reader.range(0, 20)), (std::vector<int>{10}));
}

TEST(FileBPlusTree, ReadOnlyRejectsPendingRecoverySidecars)
{
  TempFile tmp(make_temp_path());

  {
    File_BPlus_Tree<int, Aleph::less<int>, 3> tree(tmp.path.string(), false);
    EXPECT_TRUE(tree.insert(10));
    tree.sync();
  }

  {
    std::ofstream out(tmp.path.string() + ".wal",
                      std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(out.good());
    out << "pending recovery";
  }

  EXPECT_THROW((File_BPlus_Tree<int, Aleph::less<int>, 3>(
                 tmp.path.string(),
                 File_BPlus_Tree<int, Aleph::less<int>, 3>::Read_Only)),
               std::runtime_error);
}

TEST(FileBPlusTree, ReadOnlySharedLocksWorkAcrossProcesses)
{
#if defined(__unix__) || defined(__APPLE__)
  if (running_under_tsan())
    GTEST_SKIP() << "TSAN does not support these fork-based lock checks";

  TempFile tmp(make_temp_path());

  {
    File_BPlus_Tree<int, Aleph::less<int>, 3> tree(tmp.path.string(), false);
    EXPECT_TRUE(tree.insert(10));
    EXPECT_TRUE(tree.insert(20));
    tree.sync();
  }

  File_BPlus_Tree<int, Aleph::less<int>, 3> reader(
    tmp.path.string(), File_BPlus_Tree<int, Aleph::less<int>, 3>::Read_Only);

  const int shared_status = run_in_child([&]() -> int
  {
    try
      {
        File_BPlus_Tree<int, Aleph::less<int>, 3> child(
          tmp.path.string(), File_BPlus_Tree<int, Aleph::less<int>, 3>::Read_Only);
        return child.range(0, 30).size() == 2 ? 0 : 2;
      }
    catch (...)
      {
        return 1;
      }
  });
  ASSERT_NE(shared_status, -1);
  ASSERT_TRUE(WIFEXITED(shared_status));
  EXPECT_EQ(WEXITSTATUS(shared_status), 0);

  const int writer_status = run_in_child([&]() -> int
  {
    try
      {
        File_BPlus_Tree<int, Aleph::less<int>, 3> child(
          tmp.path.string(), File_BPlus_Tree<int, Aleph::less<int>, 3>::Read_Write);
        return 1;
      }
    catch (const std::runtime_error &)
      {
        return 0;
      }
    catch (...)
      {
        return 2;
      }
  });
  ASSERT_NE(writer_status, -1);
  ASSERT_TRUE(WIFEXITED(writer_status));
  EXPECT_EQ(WEXITSTATUS(writer_status), 0);
#else
  GTEST_SKIP() << "fork-based lock validation is only available on Unix-like systems";
#endif
}

TEST(FileBPlusTree, ReusesStaleLockSidecar)
{
  TempFile tmp(make_temp_path());

  {
    std::ofstream out(tmp.path.string() + ".lock",
                      std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(out.good());
    out << "stale\n";
  }

  {
    File_BPlus_Tree<int, Aleph::less<int>, 3> tree(tmp.path.string(), false);
    EXPECT_TRUE(tree.insert(5));
    tree.sync();
  }

  File_BPlus_Tree<int, Aleph::less<int>, 3> reopened(tmp.path.string(), false);
  EXPECT_EQ(to_vector(reopened.range(0, 10)), (std::vector<int>{5}));
}

TEST(FileBPlusTree, RejectsBTreeSnapshotFormat)
{
  TempFile tmp(make_temp_path());

  {
    File_B_Tree<int, Aleph::less<int>, 3> tree(tmp.path.string());
    EXPECT_TRUE(tree.insert(1));
    EXPECT_TRUE(tree.insert(2));
  }

  EXPECT_THROW((File_BPlus_Tree<int, Aleph::less<int>, 3>(tmp.path.string())),
               std::runtime_error);
}

TEST(FileBPlusTree, RejectsChecksumCorruption)
{
  TempFile tmp(make_temp_path());

  {
    File_BPlus_Tree<int, Aleph::less<int>, 3> tree(tmp.path.string());
    for (int value : {5, 10, 15, 20, 25, 30})
      EXPECT_TRUE(tree.insert(value));
  }

  std::fstream io(tmp.path, std::ios::binary | std::ios::in | std::ios::out);
  ASSERT_TRUE(io.good());
  io.seekg(0, std::ios::end);
  const auto end = io.tellg();
  ASSERT_GT(end, 0);
  io.seekg(end - std::streamoff(1));

  char byte = 0;
  io.read(&byte, 1);
  ASSERT_TRUE(io.good());
  byte ^= static_cast<char>(0x33);
  io.seekp(end - std::streamoff(1));
  io.write(&byte, 1);
  ASSERT_TRUE(io.good());
  io.close();

  EXPECT_THROW((File_BPlus_Tree<int, Aleph::less<int>, 3>(tmp.path.string())),
               std::runtime_error);
}

TEST(FileBPlusTree, RecoversFromJournalImage)
{
  TempFile tmp(make_temp_path());
  const auto journal = tmp.path.string() + ".journal";

  {
    File_BPlus_Tree<int, Aleph::less<int>, 3> tree(tmp.path.string(), false);
    for (int value : {25, 5, 35, 15, 45, 30, 10, 20, 40})
      EXPECT_TRUE(tree.insert(value));
    tree.sync();
  }

  std::error_code ec;
  fs::copy_file(tmp.path, journal, fs::copy_options::overwrite_existing, ec);
  ASSERT_FALSE(ec) << ec.message();

  {
    std::ofstream out(tmp.path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(out.good());
    out << "corrupt";
  }

  File_BPlus_Tree<int, Aleph::less<int>, 3> reopened(tmp.path.string());
  EXPECT_TRUE(reopened.verify());
  EXPECT_EQ(to_vector(reopened.range(0, 50)),
            (std::vector<int>{5, 10, 15, 20, 25, 30, 35, 40, 45}));
  EXPECT_FALSE(fs::exists(journal));
}

TEST(FileBPlusTree, RecoversFromWalImage)
{
  TempFile tmp(make_temp_path());
  const auto wal = tmp.path.string() + ".wal";

  {
    File_BPlus_Tree<int, Aleph::less<int>, 3> tree(tmp.path.string());
    for (int value : {25, 5, 35, 15, 45, 30, 10, 20, 40})
      EXPECT_TRUE(tree.insert(value));
  }

  write_bplus_wal_from_file(tmp.path, wal);

  {
    std::ofstream out(tmp.path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(out.good());
    out << "corrupt";
  }

  File_BPlus_Tree<int, Aleph::less<int>, 3> reopened(tmp.path.string());
  EXPECT_TRUE(reopened.verify());
  EXPECT_EQ(to_vector(reopened.range(0, 50)),
            (std::vector<int>{5, 10, 15, 20, 25, 30, 35, 40, 45}));
  EXPECT_FALSE(fs::exists(wal));
}

TEST(FileBPlusTree, RejectsCorruptWalCommitTrailer)
{
  TempFile tmp(make_temp_path());
  const auto wal = tmp.path.string() + ".wal";

  {
    File_BPlus_Tree<int, Aleph::less<int>, 3> tree(tmp.path.string());
    for (int value : {25, 5, 35, 15, 45, 30, 10, 20, 40})
      EXPECT_TRUE(tree.insert(value));
  }

  write_bplus_wal_from_file(tmp.path, wal);

  std::fstream wal_io(wal, std::ios::binary | std::ios::in | std::ios::out);
  ASSERT_TRUE(wal_io.good());
  wal_io.seekg(0, std::ios::end);
  const auto wal_end = wal_io.tellg();
  ASSERT_GT(wal_end, 0);
  wal_io.seekg(wal_end - std::streamoff(1));

  char byte = 0;
  wal_io.read(&byte, 1);
  ASSERT_TRUE(wal_io.good());
  byte ^= static_cast<char>(0x11);
  wal_io.seekp(wal_end - std::streamoff(1));
  wal_io.write(&byte, 1);
  ASSERT_TRUE(wal_io.good());
  wal_io.close();

  {
    std::ofstream out(tmp.path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(out.good());
    out << "corrupt";
  }

  EXPECT_THROW((File_BPlus_Tree<int, Aleph::less<int>, 3>(tmp.path.string())),
               std::runtime_error);
}

TEST(FileBPlusTree, RecoversFromPartiallyCheckpointedWal)
{
  constexpr size_t header_bytes =
    Aleph::detail::Ordered_Tree_Snapshot_Magic_Size + sizeof(std::uint32_t) * 3
    + sizeof(std::uint64_t) * 6 + sizeof(std::uint8_t) + 7;
  constexpr size_t page_bytes =
    sizeof(std::uint8_t) + sizeof(std::uint16_t) * 2 + 3
    + sizeof(std::uint64_t) + sizeof(std::uint64_t) + sizeof(int) * 5
    + sizeof(std::uint64_t) * 6 + sizeof(std::uint32_t);

  TempFile tmp(make_temp_path());
  const auto wal = tmp.path.string() + ".wal";
  const auto old_snapshot = tmp.path.string() + ".old";
  const auto new_snapshot = tmp.path.string() + ".new";

  {
    File_BPlus_Tree<int, Aleph::less<int>, 3> tree(tmp.path.string(), false);
    for (int value : {105, 110, 115, 120, 125, 130, 135, 140, 145, 150})
      EXPECT_TRUE(tree.insert(value));
    tree.sync();
  }

  std::error_code ec;
  fs::copy_file(tmp.path, old_snapshot, fs::copy_options::overwrite_existing, ec);
  ASSERT_FALSE(ec) << ec.message();

  {
    File_BPlus_Tree<int, Aleph::less<int>, 3> tree(tmp.path.string(), false);
    EXPECT_TRUE(tree.remove(120));
    tree.sync();
  }

  fs::copy_file(tmp.path, new_snapshot, fs::copy_options::overwrite_existing, ec);
  ASSERT_FALSE(ec) << ec.message();
  ASSERT_EQ(read_bplus_page_count(old_snapshot), read_bplus_page_count(new_snapshot));

  write_bplus_wal_from_file(new_snapshot, wal);
  const auto dirty_pages = read_bplus_wal_page_ids(wal);
  ASSERT_GE(dirty_pages.size(), 2u);

  fs::copy_file(old_snapshot, tmp.path, fs::copy_options::overwrite_existing, ec);
  ASSERT_FALSE(ec) << ec.message();
  copy_region(new_snapshot, tmp.path, 0, header_bytes);
  copy_region(new_snapshot, tmp.path,
              static_cast<std::streamoff>(header_bytes
                + (dirty_pages.front() - 1) * page_bytes),
              page_bytes);

  File_BPlus_Tree<int, Aleph::less<int>, 3> reopened(tmp.path.string());
  File_BPlus_Tree<int, Aleph::less<int>, 3> expected(new_snapshot);
  EXPECT_TRUE(reopened.verify());
  EXPECT_EQ(to_vector(reopened.keys()), to_vector(expected.keys()));
  EXPECT_FALSE(fs::exists(wal));
}

TEST(FileBPlusTree, IgnoresStaleWalWhenMainFileIsNewer)
{
  TempFile tmp(make_temp_path());
  const auto wal = tmp.path.string() + ".wal";
  const auto old_snapshot = tmp.path.string() + ".old";

  {
    File_BPlus_Tree<int, Aleph::less<int>, 3> tree(tmp.path.string(), false);
    for (int value : {25, 5, 35, 15, 45, 30, 10, 20, 40})
      EXPECT_TRUE(tree.insert(value));
    tree.sync();
  }

  std::error_code ec;
  fs::copy_file(tmp.path, old_snapshot, fs::copy_options::overwrite_existing, ec);
  ASSERT_FALSE(ec) << ec.message();

  {
    File_BPlus_Tree<int, Aleph::less<int>, 3> tree(tmp.path.string(), false);
    EXPECT_TRUE(tree.insert(50));
    EXPECT_TRUE(tree.insert(55));
    tree.sync();
  }

  write_bplus_wal_from_file(old_snapshot, wal);
  fs::remove(old_snapshot, ec);

  File_BPlus_Tree<int, Aleph::less<int>, 3> reopened(tmp.path.string());
  EXPECT_TRUE(reopened.verify());
  EXPECT_EQ(to_vector(reopened.range(0, 60)),
            (std::vector<int>{5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55}));
  EXPECT_FALSE(fs::exists(wal));
}

TEST(FileBPlusTree, ReopenAfterSplitsAndMergesPreservesStructure)
{
  TempFile tmp(make_temp_path());

  {
    File_BPlus_Tree<int, Aleph::less<int>, 3> tree(tmp.path.string(), false);
    for (int value = 1; value <= 80; ++value)
      EXPECT_TRUE(tree.insert(value));

    for (int value : {1, 2, 3, 7, 8, 9, 15, 16, 17, 31, 32, 33, 63, 64, 65, 80})
      EXPECT_TRUE(tree.remove(value));

    EXPECT_TRUE(tree.verify());
    tree.sync();
  }

  File_BPlus_Tree<int, Aleph::less<int>, 3> reopened(tmp.path.string());
  EXPECT_TRUE(reopened.verify());
  EXPECT_EQ(reopened.min_key(), std::optional<int>(4));
  EXPECT_EQ(reopened.max_key(), std::optional<int>(79));
  EXPECT_EQ(to_vector(reopened.range(10, 20)),
            (std::vector<int>{10, 11, 12, 13, 14, 18, 19, 20}));
  EXPECT_FALSE(reopened.contains(1));
  EXPECT_FALSE(reopened.contains(64));
  EXPECT_TRUE(reopened.contains(66));
}
