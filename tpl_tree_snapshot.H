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

/** @file tpl_tree_snapshot.H
 *  @brief Internal helpers for snapshot-backed persistent tree wrappers.
 *
 *  This header centralizes the low-level binary I/O used by
 *  \ref Aleph::File_B_Tree and \ref Aleph::File_BPlus_Tree.
 *
 *  @ingroup Trees
 */

# ifndef TPL_TREE_SNAPSHOT_H
# define TPL_TREE_SNAPSHOT_H

# include <array>
# include <bit>
# include <cstdint>
# include <cstring>
# include <filesystem>
# include <fstream>
# include <string>
# include <type_traits>

# include <ah-errors.H>
# include <tpl_array.H>

namespace Aleph
{
  namespace detail
  {
    inline constexpr size_t Ordered_Tree_Snapshot_Magic_Size = 16;
    inline constexpr std::uint32_t Ordered_Tree_Snapshot_Version = 1;

    struct Ordered_Tree_Snapshot_Header
    {
      char magic[Ordered_Tree_Snapshot_Magic_Size] = {};
      std::uint32_t version = 0;
      std::uint32_t key_size = 0;
      std::uint64_t min_degree = 0;
      std::uint64_t count = 0;
    };

    template <size_t N>
    [[nodiscard]] constexpr std::array<char, Ordered_Tree_Snapshot_Magic_Size>
    ordered_tree_snapshot_magic(const char (& text)[N]) noexcept
    {
      std::array<char, Ordered_Tree_Snapshot_Magic_Size> ret = {};
      constexpr size_t limit =
        N - 1 < Ordered_Tree_Snapshot_Magic_Size
          ? N - 1
          : Ordered_Tree_Snapshot_Magic_Size;

      for (size_t i = 0; i < limit; ++i)
        ret[i] = text[i];

      return ret;
    }

    [[nodiscard]] inline bool ordered_tree_snapshot_exists(const std::string & file_path)
    {
      return std::ifstream(file_path, std::ios::binary).good();
    }

    inline void ensure_ordered_tree_snapshot_parent_dir(const std::string & file_path,
                                                        const char * kind)
    {
      namespace fs = std::filesystem;

      const fs::path path(file_path);
      if (not path.has_parent_path())
        return;

      std::error_code ec;
      (void) fs::create_directories(path.parent_path(), ec);
      ah_runtime_error_unless(not ec)
        << kind << ": cannot create directory " << path.parent_path().string()
        << " for snapshot " << file_path << " (" << ec.message() << ")";
    }

    template <typename Key>
    [[nodiscard]] Key ordered_tree_read_key(std::istream & in,
                                            const std::string & file_path,
                                            const char * kind,
                                            const std::uint64_t index)
    {
      static_assert(std::is_trivially_copyable_v<Key>,
                    "Persistent tree snapshots require trivially copyable keys");

      std::array<char, sizeof(Key)> bytes = {};
      in.read(bytes.data(), bytes.size());
      ah_runtime_error_unless(in.good())
        << kind << ": cannot read key #" << index << " from snapshot "
        << file_path;

      return std::bit_cast<Key>(bytes);
    }

    template <typename Key>
    void ordered_tree_write_key(std::ostream & out,
                                const Key & key,
                                const std::string & file_path,
                                const char * kind,
                                const std::uint64_t index)
    {
      static_assert(std::is_trivially_copyable_v<Key>,
                    "Persistent tree snapshots require trivially copyable keys");

      const auto bytes = std::bit_cast<std::array<char, sizeof(Key)>>(key);
      out.write(bytes.data(), bytes.size());
      ah_runtime_error_unless(out.good())
        << kind << ": cannot write key #" << index << " to snapshot "
        << file_path;
    }

    inline void ordered_tree_validate_header(
      const Ordered_Tree_Snapshot_Header & header,
      const std::array<char, Ordered_Tree_Snapshot_Magic_Size> & expected_magic,
      const std::uint32_t expected_key_size,
      const std::uint64_t expected_min_degree,
      const std::string & file_path,
      const char * kind)
    {
      ah_runtime_error_unless(std::memcmp(header.magic, expected_magic.data(),
                                          Ordered_Tree_Snapshot_Magic_Size) == 0)
        << kind << ": snapshot " << file_path << " has an unexpected format";

      ah_runtime_error_unless(header.version == Ordered_Tree_Snapshot_Version)
        << kind << ": snapshot " << file_path << " uses unsupported version "
        << header.version;

      ah_runtime_error_unless(header.key_size == expected_key_size)
        << kind << ": snapshot " << file_path << " was created for key size "
        << header.key_size << ", but this instantiation expects "
        << expected_key_size;

      ah_runtime_error_unless(header.min_degree == expected_min_degree)
        << kind << ": snapshot " << file_path << " was created with minimum "
        << "degree " << header.min_degree << ", but this instantiation expects "
        << expected_min_degree;
    }

    template <typename Key>
    [[nodiscard]] Array<Key> load_ordered_tree_snapshot(
      const std::string & file_path,
      const std::array<char, Ordered_Tree_Snapshot_Magic_Size> & expected_magic,
      const std::uint64_t expected_min_degree,
      const char * kind)
    {
      static_assert(std::is_trivially_copyable_v<Key>,
                    "Persistent tree snapshots require trivially copyable keys");

      std::ifstream in(file_path, std::ios::binary);
      ah_runtime_error_unless(in.good())
        << kind << ": cannot open snapshot " << file_path << " for reading";

      Ordered_Tree_Snapshot_Header header;
      in.read(reinterpret_cast<char *>(&header), sizeof(header));
      ah_runtime_error_unless(in.good())
        << kind << ": cannot read snapshot header from " << file_path;

      ordered_tree_validate_header(header, expected_magic, sizeof(Key),
                                   expected_min_degree, file_path, kind);

      Array<Key> ret;
      ret.reserve(header.count);
      for (std::uint64_t i = 0; i < header.count; ++i)
        ret.append(ordered_tree_read_key<Key>(in, file_path, kind, i));

      return ret;
    }

    template <typename Key>
    void save_ordered_tree_snapshot(
      const std::string & file_path,
      const std::array<char, Ordered_Tree_Snapshot_Magic_Size> & expected_magic,
      const std::uint64_t expected_min_degree,
      const Array<Key> & keys,
      const char * kind)
    {
      static_assert(std::is_trivially_copyable_v<Key>,
                    "Persistent tree snapshots require trivially copyable keys");

      ensure_ordered_tree_snapshot_parent_dir(file_path, kind);

      std::ofstream out(file_path, std::ios::binary | std::ios::trunc);
      ah_runtime_error_unless(out.good())
        << kind << ": cannot open snapshot " << file_path << " for writing";

      Ordered_Tree_Snapshot_Header header;
      std::memcpy(header.magic, expected_magic.data(),
                  Ordered_Tree_Snapshot_Magic_Size);
      header.version = Ordered_Tree_Snapshot_Version;
      header.key_size = sizeof(Key);
      header.min_degree = expected_min_degree;
      header.count = keys.size();

      out.write(reinterpret_cast<const char *>(&header), sizeof(header));
      ah_runtime_error_unless(out.good())
        << kind << ": cannot write snapshot header to " << file_path;

      for (size_t i = 0; i < keys.size(); ++i)
        ordered_tree_write_key(out, keys[i], file_path, kind, i);

      out.flush();
      ah_runtime_error_unless(out.good())
        << kind << ": cannot flush snapshot " << file_path;
    }
  } // namespace detail
} // namespace Aleph

# endif // TPL_TREE_SNAPSHOT_H
