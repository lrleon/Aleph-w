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

/** @file tpl_file_b_tree.H
 *  @brief Page-managed persistent B-Tree with file-backed storage.
 *
 *  This header provides \ref Aleph::File_B_Tree, a persistent B-Tree whose
 *  nodes are stored as fixed-size pages inside a single binary file.
 *
 *  @ingroup Trees
 */

# ifndef TPL_FILE_B_TREE_H
# define TPL_FILE_B_TREE_H

# include <array>
# include <bit>
# include <cstdint>
# include <cstring>
# include <filesystem>
# include <fstream>
# include <optional>
# include <string>
# include <type_traits>
# include <utility>

# include <ah-concepts.H>
# include <ah-errors.H>
# include <ahFunction.H>
# include <tpl_array.H>
# include <tpl_b_tree.H>
# include <tpl_paged_tree_durability.H>
# include <tpl_paged_value_codec.H>
# include <tpl_tree_snapshot.H>

namespace Aleph
{
  /** @brief Persistent page-managed B-Tree stored in a single binary file.
   *
   *  `Gen_File_B_Tree` implements a file-backed B-Tree with:
   *  - fixed-size node pages identified by page ids
   *  - an on-disk header containing format/version metadata
   *  - a free-list for recycling pages released by merges or root collapse
   *  - in-page keys that mirror the classic B-Tree layout, including keys
   *    stored in internal pages
   *
   *  The file is cached in memory after opening, but all updates operate
   *  directly on the paged representation instead of rebuilding an in-memory
   *  `B_Tree` and snapshotting its sorted keys. This makes the persistent file
   *  structurally faithful to a real on-disk B-Tree while keeping the exposed
   *  API close to the existing in-memory container.
   *
   *  @tparam Key Stored key type. It must be default-constructible and
   *          copyable for the in-memory page layout, and admit a fixed-size
   *          codec so keys can be serialized into portable on-disk pages.
   *  @tparam Compare Strict weak ordering used to sort keys.
   *  @tparam MinDegree Minimum degree `t` of the persistent B-Tree. Valid
   *          values are `t >= 2`.
   *  @tparam Codec Fixed-size codec used to serialize keys in current files.
   *
   *  @par Persistence Model
   *  The tree file stores:
   *  - a header with root page id, size, page count, and free-list head
   *  - one fixed-size record per allocated page
   *  - a reusable `.lock` sidecar with an advisory OS lock and pid marker
   *  - a `.wal` sidecar used for redo recovery of page-level commits with an
   *    explicit commit trailer
   *  - a `.journal` sidecar kept as a legacy full-image fallback path
   *  - checksummed headers and page records in format version 5, using a
   *    fixed-size portable little-endian key codec
   *
   *  With `auto_sync` enabled, successful mutations write a redo WAL with the
   *  dirty page images and then checkpoint those changes into the backing file.
   *  Files can also be opened in `Paged_File_Open_Mode::Read_Only`, which
   *  acquires a shared lock and rejects any operation that would write or
   *  recover the file.
   *  Legacy stores being upgraded still use a full-image fallback path.
   *  With `auto_sync` disabled, mutations stay in the in-memory page cache
   *  until `sync()` is called. `reload()` discards any unsynchronized changes,
   *  replays a pending `.wal` or legacy `.journal` if present, and rereads the
   *  file. Readers accept the legacy native layouts (`v1`-`v4`) as well as the
   *  current portable `v5` layout with per-page checkpoint sequences.
   *
   *  @par Complexity
   *  - Search: O(log n)
   *  - Insert: O(log n)
   *  - Remove: O(log n)
   *  - lower_bound()/upper_bound(): O(log n)
   *  - sync(): O(p_dirty) page serialization for current portable stores, with
   *    a full-image fallback when upgrading legacy files
   *
   *  @par Thread Safety
   *  Instances are not synchronized. `Read_Write` handles take an exclusive
   *  advisory lock on the `.lock` sidecar, while `Read_Only` handles take a
   *  shared advisory lock on that same sidecar. The process also tracks open
   *  paths locally so invalid mixed opens fail even inside one process.
   *  Concurrent access through one handle still requires external
   *  synchronization.
   *
   *  @par Exception Safety
   *  Read-only operations do not mutate the page cache. Mutating operations
   *  preserve B-Tree invariants in the in-memory cache if comparisons and key
   *  copies do not throw in the middle of a structural update. If `auto_sync`
   *  is enabled and file I/O fails after a successful in-memory mutation, the
   *  object remains valid but the on-disk file may lag behind until a later
   *  successful `sync()`.
   *
   *  @warning Current files store keys through `Codec` in a fixed-size
   *  portable representation. Reopening a file still requires the same
   *  `Compare`, `MinDegree`, and `Codec::storage_id`. Legacy native formats
   *  (`v1`-`v4`) remain readable only when `Key` is trivially copyable.
   */
  /** @brief Generic paged persistent B-Tree container.
   *
   *  This class implements a file-backed B-Tree with a page cache. It is
   *  optimized for large datasets that cannot fit entirely in RAM,
   *  providing persistent sorted storage with O(log n) performance.
   *
   *  @tparam Key Type of stored elements.
   *  @tparam Compare Strict weak ordering used to sort keys.
   *  @tparam MinDegree Minimum degree `t` determining the branching factor.
   *  @tparam Codec Fixed-size codec used for serialization.
   */
  template <typename Key,
            class Compare = Aleph::less<Key>,
            size_t MinDegree = 16,
            typename Codec = detail::Paged_Value_Codec<Key>>
    requires StrictWeakOrder<Compare, Key>
  class Gen_File_B_Tree
  {
    static_assert(MinDegree >= 2, "File_B_Tree requires MinDegree >= 2");
    static_assert(std::default_initializable<Key>,
                  "File_B_Tree requires default-initializable keys");
    static_assert(std::copy_constructible<Key> and std::is_copy_assignable_v<Key>,
                  "File_B_Tree requires copyable keys");
    static_assert(detail::Fixed_Paged_Value_Codec<Codec, Key>,
                  "File_B_Tree requires a fixed-size key codec");
    static_assert(std::same_as<decltype(Codec::decode(
                                 static_cast<const unsigned char *>(nullptr))),
                               Key>,
                  "File_B_Tree codec must decode back to Key");

    static constexpr size_t MaxKeys = 2 * MinDegree - 1;
    static constexpr size_t MaxChildren = 2 * MinDegree;
    static constexpr size_t MinKeys = MinDegree - 1;
    static constexpr std::uint32_t LegacyFormatVersion = 1;
    static constexpr std::uint32_t ChecksummedFormatVersion = 2;
    static constexpr std::uint32_t HeaderCheckpointFormatVersion = 3;
    static constexpr std::uint32_t NativeCheckpointFormatVersion = 4;
    static constexpr std::uint32_t PortableFormatVersion = 5;
    static constexpr std::uint32_t FormatVersion = PortableFormatVersion;
    static constexpr std::uint32_t EncodedKeySize =
      static_cast<std::uint32_t>(Codec::encoded_size);
    static constexpr std::uint8_t PortableEncodingMarker = 3;
    static constexpr std::uint8_t EndianMarker =
      std::endian::native == std::endian::little ? 1 : 2;

    using page_id_t = std::uint64_t;

    enum class PageKind : std::uint8_t
    {
      Free = 0,
      Internal = 1,
      Leaf = 2
    };

    struct Page
    {
      std::uint8_t kind = static_cast<std::uint8_t>(PageKind::Free);
      std::uint16_t key_count = 0;
      std::uint16_t child_count = 0;
      page_id_t link = 0; // next free page for free pages
      std::uint64_t checkpoint_sequence = 0;
      Key keys[MaxKeys] = {};
      page_id_t children[MaxChildren] = {};
    };

    struct Loaded_Image
    {
      std::uint32_t version = FormatVersion;
      size_t size = 0;
      page_id_t root_page = 0;
      page_id_t free_page_head = 0;
      std::uint64_t checkpoint_sequence = 0;
      Array<Page> pages;
    };

    struct Wal_Record
    {
      page_id_t page_id = 0;
      Page page;
    };

    Compare cmp_;
    std::string file_path_;
    Paged_File_Open_Mode open_mode_ = Paged_File_Open_Mode::Read_Write;
    std::string journal_path_;
    std::string journal_tmp_path_;
    std::string wal_path_;
    std::string wal_tmp_path_;
    std::string lock_path_;
    mutable detail::Pid_File_Lock lock_;
    mutable std::fstream file_;
    Array<Page> pages_;
    mutable Array<unsigned char> dirty_pages_;
    mutable bool header_dirty_ = false;
    bool auto_sync_ = true;
    mutable std::uint32_t storage_format_version_ = FormatVersion;
    mutable std::uint64_t checkpoint_sequence_ = 0;

    size_t size_ = 0;
    page_id_t root_page_ = 0;
    page_id_t free_page_head_ = 0;

    [[nodiscard]] static constexpr const char * tree_kind() noexcept
    {
      return "File_B_Tree";
    }

    [[nodiscard]] detail::Pid_File_Lock_Mode lock_mode() const noexcept
    {
      return open_mode_ == Paged_File_Open_Mode::Read_Only
               ? detail::Pid_File_Lock_Mode::Shared
               : detail::Pid_File_Lock_Mode::Exclusive;
    }

    [[nodiscard]] bool read_only() const noexcept
    {
      return open_mode_ == Paged_File_Open_Mode::Read_Only;
    }

    void ensure_writable(const char * operation) const
    {
      ah_runtime_error_unless(not read_only())
        << "File_B_Tree::" << operation
        << "(): file was opened in read-only mode";
    }

    [[nodiscard]] static constexpr auto file_magic() noexcept
    {
      return detail::ordered_tree_snapshot_magic("AlephFBTPg-v1");
    }

    [[nodiscard]] static constexpr auto wal_magic() noexcept
    {
      return detail::ordered_tree_snapshot_magic("AlephFBTWalv1");
    }

    [[nodiscard]] static constexpr auto wal_trailer_magic() noexcept
    {
      return detail::ordered_tree_snapshot_magic("AlephFBTCom1");
    }

    static constexpr std::uint32_t LegacyWalVersion = 1;
    static constexpr std::uint32_t CommitTrailerWalVersion = 2;
    static constexpr std::uint32_t NativeWalVersion = 3;
    static constexpr std::uint32_t PortableWalVersion = 4;
    static constexpr std::uint32_t WalVersion = PortableWalVersion;

    [[nodiscard]] static constexpr size_t legacy_header_bytes() noexcept
    {
      return detail::Ordered_Tree_Snapshot_Magic_Size + sizeof(std::uint32_t) * 2
             + sizeof(std::uint64_t) * 5 + sizeof(std::uint8_t) + 7;
    }

    [[nodiscard]] static constexpr size_t checksummed_header_bytes() noexcept
    {
      return legacy_header_bytes() + sizeof(std::uint32_t);
    }

    [[nodiscard]] static constexpr size_t header_bytes() noexcept
    {
      return checksummed_header_bytes() + sizeof(std::uint64_t);
    }

    [[nodiscard]] static constexpr size_t legacy_page_bytes() noexcept
    {
      return sizeof(std::uint8_t) + sizeof(std::uint16_t) * 2 + 3
             + sizeof(page_id_t)
             + sizeof(Key) * MaxKeys
             + sizeof(page_id_t) * MaxChildren;
    }

    [[nodiscard]] static constexpr size_t checksummed_page_bytes() noexcept
    {
      return legacy_page_bytes() + sizeof(std::uint32_t);
    }

    [[nodiscard]] static constexpr size_t native_page_bytes() noexcept
    {
      return legacy_page_bytes() + sizeof(std::uint64_t) + sizeof(std::uint32_t);
    }

    [[nodiscard]] static constexpr size_t portable_page_bytes() noexcept
    {
      return sizeof(std::uint8_t) + sizeof(std::uint16_t) * 2 + 3
             + sizeof(page_id_t) + sizeof(std::uint64_t)
             + Codec::encoded_size * MaxKeys
             + sizeof(page_id_t) * MaxChildren + sizeof(std::uint32_t);
    }

    [[nodiscard]] static constexpr size_t page_bytes() noexcept
    {
      return portable_page_bytes();
    }

    [[nodiscard]] static constexpr size_t serialized_header_bytes(
      const std::uint32_t version) noexcept
    {
      return version >= HeaderCheckpointFormatVersion ? header_bytes()
             : version >= ChecksummedFormatVersion ? checksummed_header_bytes()
             : legacy_header_bytes();
    }

    [[nodiscard]] static constexpr size_t serialized_page_bytes(
      const std::uint32_t version) noexcept
    {
      return version >= PortableFormatVersion ? portable_page_bytes()
             : version >= NativeCheckpointFormatVersion ? native_page_bytes()
             : version >= HeaderCheckpointFormatVersion ? checksummed_page_bytes()
             : legacy_page_bytes();
    }

    [[nodiscard]] static constexpr bool version_uses_portable_codec(
      const std::uint32_t version) noexcept
    {
      return version >= PortableFormatVersion;
    }

    [[nodiscard]] static constexpr std::uint8_t encoding_marker_for_version(
      const std::uint32_t version) noexcept
    {
      return version_uses_portable_codec(version)
               ? PortableEncodingMarker
               : EndianMarker;
    }

    [[nodiscard]] static constexpr std::uint32_t key_size_for_version(
      const std::uint32_t version) noexcept
    {
      return version_uses_portable_codec(version)
               ? EncodedKeySize
               : static_cast<std::uint32_t>(sizeof(Key));
    }

    [[nodiscard]] static constexpr std::array<char, 7>
    reserved_bytes_for_version(const std::uint32_t version) noexcept
    {
      std::array<char, 7> reserved = {};
      if (version_uses_portable_codec(version))
        {
          const auto codec_id = Codec::storage_id;
          reserved[0] = static_cast<char>(codec_id & 0xFFu);
          reserved[1] = static_cast<char>((codec_id >> 8) & 0xFFu);
          reserved[2] = static_cast<char>((codec_id >> 16) & 0xFFu);
          reserved[3] = static_cast<char>((codec_id >> 24) & 0xFFu);
        }
      return reserved;
    }

    [[nodiscard]] static constexpr Page make_page(const PageKind kind) noexcept
    {
      Page page;
      page.kind = static_cast<std::uint8_t>(kind);
      return page;
    }

    static void ensure_legacy_native_format_supported(
      const std::string & source_path,
      const std::uint32_t version)
    {
      if constexpr (not std::is_trivially_copyable_v<Key>)
        ah_runtime_error()
          << "File_B_Tree: file " << source_path
          << " uses legacy native format version " << version
          << ", which requires trivially copyable keys; reopen or rewrite it "
          << "with portable format version " << PortableFormatVersion
          << " or newer";
    }

    static void ensure_legacy_native_wal_supported(
      const std::string & source_path,
      const std::uint32_t wal_version)
    {
      if constexpr (not std::is_trivially_copyable_v<Key>)
        ah_runtime_error()
          << "File_B_Tree: WAL " << source_path
          << " uses legacy native format version " << wal_version
          << ", which requires trivially copyable keys; recover it with a "
          << "trivially copyable instantiation or discard the stale WAL";
    }

    [[nodiscard]] static constexpr bool is_leaf(const Page & page) noexcept
    {
      return page.kind == static_cast<std::uint8_t>(PageKind::Leaf);
    }

    [[nodiscard]] static constexpr bool is_internal(const Page & page) noexcept
    {
      return page.kind == static_cast<std::uint8_t>(PageKind::Internal);
    }

    [[nodiscard]] static constexpr bool is_free(const Page & page) noexcept
    {
      return page.kind == static_cast<std::uint8_t>(PageKind::Free);
    }

    void init_empty_cache()
    {
      pages_.empty();
      dirty_pages_.empty();
      pages_.append(Page{});
      dirty_pages_.append(0);
      size_ = 0;
      root_page_ = 0;
      free_page_head_ = 0;
      checkpoint_sequence_ = 0;
      header_dirty_ = true;
      storage_format_version_ = FormatVersion;
    }

    void ensure_parent_dir() const
    {
      namespace fs = std::filesystem;

      const fs::path path(file_path_);
      if (not path.has_parent_path())
        return;

      std::error_code ec;
      (void) fs::create_directories(path.parent_path(), ec);
      ah_runtime_error_unless(not ec)
        << "File_B_Tree: cannot create directory "
        << path.parent_path().string() << " for " << file_path_
        << " (" << ec.message() << ")";
    }

    void open_storage(const bool truncate = false) const
    {
      if (truncate)
        ensure_writable("open_storage");

      if (not read_only())
        ensure_parent_dir();

      if (file_.is_open())
        file_.close();

      if (truncate)
        {
          std::ofstream create(file_path_, std::ios::binary | std::ios::trunc);
          ah_runtime_error_unless(create.good())
            << "File_B_Tree: cannot create " << file_path_;
        }

      const auto mode = read_only()
                          ? (std::ios::binary | std::ios::in)
                          : (std::ios::binary | std::ios::in | std::ios::out);
      file_.open(file_path_, mode);
      ah_runtime_error_unless(file_.good())
        << "File_B_Tree: cannot open " << file_path_;
    }

    void init_sidecar_paths()
    {
      journal_path_ = file_path_ + ".journal";
      journal_tmp_path_ = file_path_ + ".journal.tmp";
      wal_path_ = file_path_ + ".wal";
      wal_tmp_path_ = file_path_ + ".wal.tmp";
      lock_path_ = file_path_ + ".lock";
    }

    template <typename T>
    static void write_pod(std::ostream & out,
                          const T & value,
                          const std::string & file_path,
                          const char * what)
    {
      out.write(reinterpret_cast<const char *>(&value), sizeof(value));
      ah_runtime_error_unless(out.good())
        << "File_B_Tree: cannot write " << what << " to " << file_path;
    }

    template <typename T>
    static T read_pod(std::istream & in,
                      const std::string & file_path,
                      const char * what)
    {
      T value{};
      in.read(reinterpret_cast<char *>(&value), sizeof(value));
      ah_runtime_error_unless(in.good())
        << "File_B_Tree: cannot read " << what << " from " << file_path;
      return value;
    }

    static void write_native_key(std::ostream & out,
                                 const Key & key,
                                 const std::string & file_path,
                                 const char * what)
    {
      if constexpr (std::is_trivially_copyable_v<Key>)
        {
          const auto bytes = std::bit_cast<std::array<char, sizeof(Key)>>(key);
          out.write(bytes.data(), bytes.size());
          ah_runtime_error_unless(out.good())
            << "File_B_Tree: cannot write " << what << " to " << file_path;
        }
      else
        ah_runtime_error()
          << "File_B_Tree: cannot write legacy native " << what << " to "
          << file_path << " because legacy native formats require trivially "
          << "copyable keys";
    }

    static void write_portable_key(std::ostream & out,
                                   const Key & key,
                                   const std::string & file_path,
                                   const char * what)
    {
      std::array<unsigned char, Codec::encoded_size> bytes = {};
      Codec::encode(key, bytes.data());
      out.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
      ah_runtime_error_unless(out.good())
        << "File_B_Tree: cannot write " << what << " to " << file_path;
    }

    static void write_key(std::ostream & out,
                          const Key & key,
                          const std::string & file_path,
                          const char * what)
    {
      write_portable_key(out, key, file_path, what);
    }

    [[nodiscard]] static Key read_native_key(std::istream & in,
                                             const std::string & file_path,
                                             const char * what)
    {
      if constexpr (std::is_trivially_copyable_v<Key>)
        {
          std::array<char, sizeof(Key)> bytes = {};
          in.read(bytes.data(), bytes.size());
          ah_runtime_error_unless(in.good())
            << "File_B_Tree: cannot read " << what << " from " << file_path;
          return std::bit_cast<Key>(bytes);
        }
      else
        {
          ah_runtime_error()
            << "File_B_Tree: cannot read legacy native " << what << " from "
            << file_path << " because legacy native formats require trivially "
            << "copyable keys";
          return Key{};
        }
    }

    [[nodiscard]] static Key read_portable_key(std::istream & in,
                                               const std::string & file_path,
                                               const char * what)
    {
      std::array<unsigned char, Codec::encoded_size> bytes = {};
      in.read(reinterpret_cast<char *>(bytes.data()), bytes.size());
      ah_runtime_error_unless(in.good())
        << "File_B_Tree: cannot read " << what << " from " << file_path;
      return Codec::decode(bytes.data());
    }

    [[nodiscard]] static Key read_key(std::istream & in,
                                      const std::string & file_path,
                                      const char * what,
                                      const std::uint32_t version)
    {
      return version_uses_portable_codec(version)
               ? read_portable_key(in, file_path, what)
               : read_native_key(in, file_path, what);
    }

    [[nodiscard]] static std::uint32_t add_key_to_crc(
      const std::uint32_t crc,
      const Key & key)
    {
      return Codec::add_to_crc(crc, key);
    }

    [[nodiscard]] static std::uint32_t add_native_key_to_crc(
      const std::uint32_t crc,
      const Key & key)
    {
      if constexpr (std::is_trivially_copyable_v<Key>)
        return detail::crc32_add_bytes(crc, &key, sizeof(Key));
      else
        {
          ah_runtime_error()
            << "File_B_Tree: cannot checksum legacy native keys because "
            << "legacy native formats require trivially copyable keys";
          return crc;
        }
    }

    [[nodiscard]] static std::uint32_t header_checksum_v2(
      const std::uint32_t version,
      const std::uint64_t size,
      const page_id_t root_page,
      const std::uint64_t page_count,
      const page_id_t free_page_head) noexcept
    {
      std::uint32_t crc = detail::crc32_begin();
      crc = detail::crc32_add(crc, version);
      crc = detail::crc32_add(crc, key_size_for_version(version));
      crc = detail::crc32_add(crc, static_cast<std::uint64_t>(MinDegree));
      crc = detail::crc32_add(crc, encoding_marker_for_version(version));
      const auto reserved = reserved_bytes_for_version(version);
      crc = detail::crc32_add_bytes(crc, reserved.data(), reserved.size());
      crc = detail::crc32_add(crc, size);
      crc = detail::crc32_add(crc, root_page);
      crc = detail::crc32_add(crc, page_count);
      crc = detail::crc32_add(crc, free_page_head);
      return detail::crc32_finish(crc);
    }

    [[nodiscard]] static std::uint32_t header_checksum(
      const std::uint32_t version,
      const std::uint64_t size,
      const page_id_t root_page,
      const std::uint64_t page_count,
      const page_id_t free_page_head,
      const std::uint64_t checkpoint_sequence) noexcept
    {
      std::uint32_t crc = detail::crc32_begin();
      crc = detail::crc32_add(crc, version);
      crc = detail::crc32_add(crc, key_size_for_version(version));
      crc = detail::crc32_add(crc, static_cast<std::uint64_t>(MinDegree));
      crc = detail::crc32_add(crc, encoding_marker_for_version(version));
      const auto reserved = reserved_bytes_for_version(version);
      crc = detail::crc32_add_bytes(crc, reserved.data(), reserved.size());
      crc = detail::crc32_add(crc, size);
      crc = detail::crc32_add(crc, root_page);
      crc = detail::crc32_add(crc, page_count);
      crc = detail::crc32_add(crc, free_page_head);
      crc = detail::crc32_add(crc, checkpoint_sequence);
      return detail::crc32_finish(crc);
    }

    [[nodiscard]] static std::uint32_t page_checksum_v3(const Page & page)
    {
      std::uint32_t crc = detail::crc32_begin();
      const std::array<char, 3> padding = {};

      crc = detail::crc32_add(crc, page.kind);
      crc = detail::crc32_add(crc, page.key_count);
      crc = detail::crc32_add(crc, page.child_count);
      crc = detail::crc32_add_bytes(crc, padding.data(), padding.size());
      crc = detail::crc32_add(crc, page.link);
      for (size_t i = 0; i < MaxKeys; ++i)
        crc = add_native_key_to_crc(crc, page.keys[i]);
      crc = detail::crc32_add_bytes(crc, page.children, sizeof(page.children));
      return detail::crc32_finish(crc);
    }

    [[nodiscard]] static std::uint32_t page_checksum(const Page & page)
    {
      std::uint32_t crc = detail::crc32_begin();
      const std::array<char, 3> padding = {};

      crc = detail::crc32_add(crc, page.kind);
      crc = detail::crc32_add(crc, page.key_count);
      crc = detail::crc32_add(crc, page.child_count);
      crc = detail::crc32_add_bytes(crc, padding.data(), padding.size());
      crc = detail::crc32_add(crc, page.link);
      crc = detail::crc32_add(crc, page.checkpoint_sequence);
      for (size_t i = 0; i < MaxKeys; ++i)
        crc = add_key_to_crc(crc, page.keys[i]);
      crc = detail::crc32_add_bytes(crc, page.children, sizeof(page.children));
      return detail::crc32_finish(crc);
    }

    [[nodiscard]] static std::uint32_t page_checksum_v4(const Page & page)
    {
      std::uint32_t crc = detail::crc32_begin();
      const std::array<char, 3> padding = {};

      crc = detail::crc32_add(crc, page.kind);
      crc = detail::crc32_add(crc, page.key_count);
      crc = detail::crc32_add(crc, page.child_count);
      crc = detail::crc32_add_bytes(crc, padding.data(), padding.size());
      crc = detail::crc32_add(crc, page.link);
      crc = detail::crc32_add(crc, page.checkpoint_sequence);
      for (size_t i = 0; i < MaxKeys; ++i)
        crc = add_native_key_to_crc(crc, page.keys[i]);
      crc = detail::crc32_add_bytes(crc, page.children, sizeof(page.children));
      return detail::crc32_finish(crc);
    }

    [[nodiscard]] static std::uint32_t wal_record_checksum_add(
      std::uint32_t crc,
      const page_id_t page_id,
      const Page & page)
    {
      const std::array<char, 3> padding = {};

      crc = detail::crc32_add(crc, page_id);
      crc = detail::crc32_add(crc, page.kind);
      crc = detail::crc32_add(crc, page.key_count);
      crc = detail::crc32_add(crc, page.child_count);
      crc = detail::crc32_add_bytes(crc, padding.data(), padding.size());
      crc = detail::crc32_add(crc, page.link);
      crc = detail::crc32_add(crc, page.checkpoint_sequence);
      for (size_t i = 0; i < MaxKeys; ++i)
        crc = add_key_to_crc(crc, page.keys[i]);
      crc = detail::crc32_add_bytes(crc, page.children, sizeof(page.children));
      crc = detail::crc32_add(crc, page_checksum(page));
      return crc;
    }

    [[nodiscard]] static std::uint32_t wal_record_checksum_add_v3(
      std::uint32_t crc,
      const page_id_t page_id,
      const Page & page)
    {
      const std::array<char, 3> padding = {};

      crc = detail::crc32_add(crc, page_id);
      crc = detail::crc32_add(crc, page.kind);
      crc = detail::crc32_add(crc, page.key_count);
      crc = detail::crc32_add(crc, page.child_count);
      crc = detail::crc32_add_bytes(crc, padding.data(), padding.size());
      crc = detail::crc32_add(crc, page.link);
      crc = detail::crc32_add(crc, page.checkpoint_sequence);
      for (size_t i = 0; i < MaxKeys; ++i)
        crc = add_native_key_to_crc(crc, page.keys[i]);
      crc = detail::crc32_add_bytes(crc, page.children, sizeof(page.children));
      crc = detail::crc32_add(crc, page_checksum_v4(page));
      return crc;
    }

    [[nodiscard]] static std::uint32_t wal_record_checksum_add_v2(
      std::uint32_t crc,
      const page_id_t page_id,
      const Page & page)
    {
      const std::array<char, 3> padding = {};

      crc = detail::crc32_add(crc, page_id);
      crc = detail::crc32_add(crc, page.kind);
      crc = detail::crc32_add(crc, page.key_count);
      crc = detail::crc32_add(crc, page.child_count);
      crc = detail::crc32_add_bytes(crc, padding.data(), padding.size());
      crc = detail::crc32_add(crc, page.link);
      for (size_t i = 0; i < MaxKeys; ++i)
        crc = add_native_key_to_crc(crc, page.keys[i]);
      crc = detail::crc32_add_bytes(crc, page.children, sizeof(page.children));
      crc = detail::crc32_add(crc, page_checksum_v3(page));
      return crc;
    }

    static void write_page_record(std::ostream & out,
                                  const Page & page,
                                  const std::string & target_path)
    {
      write_pod(out, page.kind, target_path, "page kind");
      write_pod(out, page.key_count, target_path, "page key count");
      write_pod(out, page.child_count, target_path, "page child count");

      const std::array<char, 3> padding = {};
      out.write(padding.data(), padding.size());
      ah_runtime_error_unless(out.good())
        << "File_B_Tree: cannot write page padding to " << target_path;

      write_pod(out, page.link, target_path, "page link");
      write_pod(out, page.checkpoint_sequence, target_path,
                "page checkpoint sequence");

      for (size_t i = 0; i < MaxKeys; ++i)
        write_key(out, page.keys[i], target_path, "page key");

      for (size_t i = 0; i < MaxChildren; ++i)
        write_pod(out, page.children[i], target_path, "page child");

      write_pod(out, page_checksum(page), target_path, "page checksum");
    }

    void seekp_to(const std::streamoff offset) const
    {
      file_.clear();
      file_.seekp(offset);
      ah_runtime_error_unless(file_.good())
        << "File_B_Tree: cannot seek for writing in " << file_path_;
    }

    [[nodiscard]] std::streamoff current_page_offset(const page_id_t page_id) const noexcept
    {
      return static_cast<std::streamoff>(header_bytes()
             + (page_id - 1) * page_bytes());
    }

    void write_header_to_storage(const std::uint64_t size,
                                 const page_id_t root_page,
                                 const std::uint64_t page_count,
                                 const page_id_t free_page_head,
                                 const std::uint64_t checkpoint_sequence) const
    {
      seekp_to(0);
      const auto magic = file_magic();
      file_.write(magic.data(), magic.size());
      ah_runtime_error_unless(file_.good())
        << "File_B_Tree: cannot write magic to " << file_path_;

      const auto reserved = reserved_bytes_for_version(FormatVersion);

      write_pod(file_, FormatVersion, file_path_, "header version");
      write_pod(file_, key_size_for_version(FormatVersion), file_path_,
                "header key size");
      write_pod(file_, static_cast<std::uint64_t>(MinDegree), file_path_,
                "header minimum degree");
      write_pod(file_, encoding_marker_for_version(FormatVersion), file_path_,
                "header encoding");
      file_.write(reserved.data(), reserved.size());
      ah_runtime_error_unless(file_.good())
        << "File_B_Tree: cannot write reserved header bytes to " << file_path_;
      write_pod(file_, size, file_path_, "header size");
      write_pod(file_, root_page, file_path_, "header root page");
      write_pod(file_, page_count, file_path_, "header page count");
      write_pod(file_, free_page_head, file_path_, "header free page head");
      write_pod(file_, checkpoint_sequence, file_path_,
                "header checkpoint sequence");
      write_pod(file_,
                header_checksum(FormatVersion, size, root_page, page_count,
                                free_page_head, checkpoint_sequence),
                file_path_, "header checksum");
    }

    void write_current_header_to_storage(const std::uint64_t checkpoint_sequence) const
    {
      write_header_to_storage(static_cast<std::uint64_t>(size_), root_page_,
                              static_cast<std::uint64_t>(pages_.size() - 1),
                              free_page_head_, checkpoint_sequence);
    }

    void write_current_page_to_storage(const page_id_t page_id,
                                       const Page & page) const
    {
      seekp_to(current_page_offset(page_id));
      write_page_record(file_, page, file_path_);
    }

    [[nodiscard]] size_t dirty_page_count() const noexcept
    {
      size_t count = 0;
      for (page_id_t page_id = 1; page_id < dirty_pages_.size(); ++page_id)
        if (dirty_pages_[page_id] != 0)
          ++count;
      return count;
    }

    [[nodiscard]] static constexpr size_t wal_header_bytes() noexcept
    {
      return detail::Ordered_Tree_Snapshot_Magic_Size + sizeof(std::uint32_t) * 3
             + sizeof(std::uint64_t) * 7 + sizeof(std::uint8_t) + 7;
    }

    [[nodiscard]] static constexpr std::uint32_t
    wal_record_format_version(const std::uint32_t wal_version) noexcept
    {
      return wal_version >= PortableWalVersion ? PortableFormatVersion
             : wal_version >= NativeWalVersion ? NativeCheckpointFormatVersion
             : HeaderCheckpointFormatVersion;
    }

    [[nodiscard]] static constexpr size_t
    wal_record_bytes(const std::uint32_t wal_version) noexcept
    {
      return sizeof(page_id_t)
             + serialized_page_bytes(wal_record_format_version(wal_version));
    }

    [[nodiscard]] static std::uint32_t wal_header_checksum(
      const std::uint32_t wal_version,
      const std::uint64_t size,
      const page_id_t root_page,
      const std::uint64_t page_count,
      const page_id_t free_page_head,
      const std::uint64_t checkpoint_sequence,
      const std::uint64_t dirty_count) noexcept
    {
      std::uint32_t crc = detail::crc32_begin();
      crc = detail::crc32_add(crc, wal_version);
      crc = detail::crc32_add(crc, key_size_for_version(
                                     wal_record_format_version(wal_version)));
      crc = detail::crc32_add(crc, static_cast<std::uint64_t>(MinDegree));
      crc = detail::crc32_add(
        crc, encoding_marker_for_version(wal_record_format_version(wal_version)));
      const auto reserved =
        reserved_bytes_for_version(wal_record_format_version(wal_version));
      crc = detail::crc32_add_bytes(crc, reserved.data(), reserved.size());
      crc = detail::crc32_add(crc, size);
      crc = detail::crc32_add(crc, root_page);
      crc = detail::crc32_add(crc, page_count);
      crc = detail::crc32_add(crc, free_page_head);
      crc = detail::crc32_add(crc, checkpoint_sequence);
      crc = detail::crc32_add(crc, dirty_count);
      return detail::crc32_finish(crc);
    }

    [[nodiscard]] static constexpr size_t wal_trailer_bytes() noexcept
    {
      return detail::Ordered_Tree_Snapshot_Magic_Size
             + sizeof(std::uint64_t) * 2 + sizeof(std::uint32_t);
    }

    void write_wal_to_path(const std::string & target_path,
                           const std::uint64_t checkpoint_sequence) const
    {
      std::ofstream out(target_path, std::ios::binary | std::ios::trunc);
      ah_runtime_error_unless(out.good())
        << "File_B_Tree: cannot open " << target_path << " for writing";

      const auto magic = wal_magic();
      out.write(magic.data(), magic.size());
      ah_runtime_error_unless(out.good())
        << "File_B_Tree: cannot write WAL magic to " << target_path;

      const auto page_count = static_cast<std::uint64_t>(pages_.size() - 1);
      const auto size = static_cast<std::uint64_t>(size_);
      const auto dirty_count = static_cast<std::uint64_t>(dirty_page_count());
      const auto reserved = reserved_bytes_for_version(FormatVersion);
      std::uint32_t payload_crc = detail::crc32_begin();

      write_pod(out, WalVersion, target_path, "WAL version");
      write_pod(out, key_size_for_version(FormatVersion), target_path,
                "WAL key size");
      write_pod(out, static_cast<std::uint64_t>(MinDegree), target_path,
                "WAL minimum degree");
      write_pod(out, encoding_marker_for_version(FormatVersion), target_path,
                "WAL encoding");
      out.write(reserved.data(), reserved.size());
      ah_runtime_error_unless(out.good())
        << "File_B_Tree: cannot write WAL reserved bytes to " << target_path;
      write_pod(out, size, target_path, "WAL size");
      write_pod(out, root_page_, target_path, "WAL root page");
      write_pod(out, page_count, target_path, "WAL page count");
      write_pod(out, free_page_head_, target_path, "WAL free page head");
      write_pod(out, checkpoint_sequence, target_path,
                "WAL checkpoint sequence");
      write_pod(out, dirty_count, target_path, "WAL dirty count");
      write_pod(out,
                wal_header_checksum(WalVersion, size, root_page_, page_count,
                                    free_page_head_, checkpoint_sequence,
                                    dirty_count),
                target_path, "WAL checksum");

      for (page_id_t page_id = 1; page_id < dirty_pages_.size(); ++page_id)
        if (dirty_pages_[page_id] != 0)
          {
            payload_crc = wal_record_checksum_add(payload_crc, page_id,
                                                  page(page_id));
            write_pod(out, page_id, target_path, "WAL page id");
            write_page_record(out, page(page_id), target_path);
          }

      const auto trailer_magic = wal_trailer_magic();
      out.write(trailer_magic.data(), trailer_magic.size());
      ah_runtime_error_unless(out.good())
        << "File_B_Tree: cannot write WAL commit trailer to " << target_path;
      write_pod(out, checkpoint_sequence, target_path,
                "WAL trailer checkpoint sequence");
      write_pod(out, dirty_count, target_path, "WAL trailer dirty count");
      write_pod(out, detail::crc32_finish(payload_crc), target_path,
                "WAL payload checksum");

      out.flush();
      ah_runtime_error_unless(out.good())
        << "File_B_Tree: cannot flush " << target_path;
      out.close();
      ah_runtime_error_unless(out.good())
        << "File_B_Tree: cannot close " << target_path;

      detail::sync_file_by_path(target_path, tree_kind());
      detail::sync_parent_directory(target_path, tree_kind());
    }

    [[nodiscard]] static Page read_page_from_stream(std::istream & in,
                                                    const std::string & path,
                                                    const std::uint32_t version)
    {
      Page page;
      page.kind = read_pod<std::uint8_t>(in, path, "page kind");
      page.key_count = read_pod<std::uint16_t>(in, path, "page key count");
      page.child_count = read_pod<std::uint16_t>(in, path, "page child count");

      std::array<char, 3> padding = {};
      in.read(padding.data(), padding.size());
      ah_runtime_error_unless(in.good())
        << "File_B_Tree: cannot read page padding from " << path;

      page.link = read_pod<page_id_t>(in, path, "page link");

      if (version >= NativeCheckpointFormatVersion)
        page.checkpoint_sequence =
          read_pod<std::uint64_t>(in, path, "page checkpoint sequence");

      for (size_t i = 0; i < MaxKeys; ++i)
        page.keys[i] = read_key(in, path, "page key", version);

      for (size_t i = 0; i < MaxChildren; ++i)
        page.children[i] = read_pod<page_id_t>(in, path, "page child");

      if (version >= HeaderCheckpointFormatVersion)
        {
          const auto stored = read_pod<std::uint32_t>(in, path, "page checksum");
          ah_runtime_error_unless(stored
                                  == (version >= PortableFormatVersion
                                        ? page_checksum(page)
                                        : version >= NativeCheckpointFormatVersion
                                            ? page_checksum_v4(page)
                                            : page_checksum_v3(page)))
            << "File_B_Tree: page checksum mismatch in " << path;
        }

      return page;
    }

    void write_full_image(std::ostream & out,
                          const std::string & target_path,
                          const std::uint64_t checkpoint_sequence) const
    {
      const auto magic = file_magic();
      out.write(magic.data(), magic.size());
      ah_runtime_error_unless(out.good())
        << "File_B_Tree: cannot write magic to " << target_path;

      const auto page_count = static_cast<std::uint64_t>(pages_.size() - 1);
      const auto size = static_cast<std::uint64_t>(size_);
      const auto reserved = reserved_bytes_for_version(FormatVersion);

      write_pod(out, FormatVersion, target_path, "header version");
      write_pod(out, key_size_for_version(FormatVersion), target_path,
                "header key size");
      write_pod(out, static_cast<std::uint64_t>(MinDegree), target_path,
                "header minimum degree");
      write_pod(out, encoding_marker_for_version(FormatVersion), target_path,
                "header encoding");
      out.write(reserved.data(), reserved.size());
      ah_runtime_error_unless(out.good())
        << "File_B_Tree: cannot write reserved header bytes to " << target_path;
      write_pod(out, size, target_path, "header size");
      write_pod(out, root_page_, target_path, "header root page");
      write_pod(out, page_count, target_path, "header page count");
      write_pod(out, free_page_head_, target_path, "header free page head");
      write_pod(out, checkpoint_sequence, target_path,
                "header checkpoint sequence");
      write_pod(out,
                header_checksum(FormatVersion, size, root_page_, page_count,
                                free_page_head_, checkpoint_sequence),
                target_path, "header checksum");

      for (page_id_t page_id = 1; page_id < pages_.size(); ++page_id)
        write_page_record(out, page(page_id), target_path);
    }

    void write_full_image_to_path(const std::string & target_path,
                                  const std::uint64_t checkpoint_sequence) const
    {
      namespace fs = std::filesystem;
      const fs::path path(target_path);
      if (path.has_parent_path())
        {
          std::error_code ec;
          (void) fs::create_directories(path.parent_path(), ec);
          ah_runtime_error_unless(not ec)
            << "File_B_Tree: cannot create directory "
            << path.parent_path().string() << " for " << target_path
            << " (" << ec.message() << ")";
        }

      std::ofstream out(target_path, std::ios::binary | std::ios::trunc);
      ah_runtime_error_unless(out.good())
        << "File_B_Tree: cannot open " << target_path << " for writing";

      write_full_image(out, target_path, checkpoint_sequence);
      out.flush();
      ah_runtime_error_unless(out.good())
        << "File_B_Tree: cannot flush " << target_path;
      out.close();
      ah_runtime_error_unless(out.good())
        << "File_B_Tree: cannot close " << target_path;

      detail::sync_file_by_path(target_path, tree_kind());
      detail::sync_parent_directory(target_path, tree_kind());
    }

    void apply_current_cache_to_storage(const std::uint64_t checkpoint_sequence) const
    {
      if (not file_exists() or storage_format_version_ != FormatVersion)
        {
          if (file_.is_open())
            file_.close();
          write_full_image_to_path(file_path_, checkpoint_sequence);
          if (not file_.is_open())
            open_storage(false);
          storage_format_version_ = FormatVersion;
          checkpoint_sequence_ = checkpoint_sequence;
          return;
        }

      if (not file_.is_open())
        open_storage(false);

      write_current_header_to_storage(checkpoint_sequence);
      for (page_id_t page_id = 1; page_id < dirty_pages_.size(); ++page_id)
        if (dirty_pages_[page_id] != 0)
          write_current_page_to_storage(page_id, page(page_id));

      file_.flush();
      ah_runtime_error_unless(file_.good())
        << "File_B_Tree: cannot flush " << file_path_;
      detail::sync_file_by_path(file_path_, tree_kind());
      storage_format_version_ = FormatVersion;
      checkpoint_sequence_ = checkpoint_sequence;
    }

    void recover_from_wal()
    {
      namespace fs = std::filesystem;
      if (read_only())
        {
          ah_runtime_error_unless(not fs::exists(wal_tmp_path_)
                                  and not fs::exists(wal_path_))
            << "File_B_Tree: cannot open read-only while WAL recovery sidecars "
            << "are pending for " << file_path_;
          return;
        }

      detail::remove_file_if_exists(wal_tmp_path_, tree_kind());
      if (not fs::exists(wal_path_))
        return;

      std::ifstream in(wal_path_, std::ios::binary);
      ah_runtime_error_unless(in.good())
        << "File_B_Tree: cannot open WAL " << wal_path_ << " for reading";

      const auto expected_magic = wal_magic();
      std::array<char, detail::Ordered_Tree_Snapshot_Magic_Size> magic = {};
      in.read(magic.data(), magic.size());
      ah_runtime_error_unless(in.good())
        << "File_B_Tree: cannot read WAL magic from " << wal_path_;
      ah_runtime_error_unless(magic == expected_magic)
        << "File_B_Tree: incompatible WAL file " << wal_path_;

      const auto wal_version = read_pod<std::uint32_t>(in, wal_path_, "WAL version");
      ah_runtime_error_unless(wal_version == LegacyWalVersion
                              or wal_version == CommitTrailerWalVersion
                              or wal_version == NativeWalVersion
                              or wal_version == WalVersion)
        << "File_B_Tree: unsupported WAL version " << wal_version
        << " in " << wal_path_;
      if (wal_version < PortableWalVersion)
        ensure_legacy_native_wal_supported(wal_path_, wal_version);

      const auto key_size = read_pod<std::uint32_t>(in, wal_path_, "WAL key size");
      ah_runtime_error_unless(key_size
                              == key_size_for_version(
                                wal_record_format_version(wal_version)))
        << "File_B_Tree: WAL " << wal_path_ << " was created for key size "
        << key_size << ", but this instantiation expects "
        << key_size_for_version(wal_record_format_version(wal_version));

      const auto min_degree =
        read_pod<std::uint64_t>(in, wal_path_, "WAL minimum degree");
      ah_runtime_error_unless(min_degree == MinDegree)
        << "File_B_Tree: WAL " << wal_path_
        << " was created with minimum degree " << min_degree
        << ", but this instantiation expects " << MinDegree;

      const auto encoding = read_pod<std::uint8_t>(in, wal_path_, "WAL encoding");
      ah_runtime_error_unless(encoding
                              == encoding_marker_for_version(
                                wal_record_format_version(wal_version)))
        << "File_B_Tree: WAL " << wal_path_
        << " was created with an incompatible encoding";

      std::array<char, 7> reserved = {};
      in.read(reserved.data(), reserved.size());
      ah_runtime_error_unless(in.good())
        << "File_B_Tree: cannot read WAL reserved bytes from " << wal_path_;
      ah_runtime_error_unless(
        reserved == reserved_bytes_for_version(
                      wal_record_format_version(wal_version)))
        << "File_B_Tree: WAL " << wal_path_
        << " was created with an incompatible key codec";

      const auto wal_size = read_pod<std::uint64_t>(in, wal_path_, "WAL size");
      const auto wal_root_page =
        read_pod<page_id_t>(in, wal_path_, "WAL root page");
      const auto wal_page_count =
        read_pod<std::uint64_t>(in, wal_path_, "WAL page count");
      const auto wal_free_head =
        read_pod<page_id_t>(in, wal_path_, "WAL free page head");
      const auto wal_checkpoint_sequence =
        read_pod<std::uint64_t>(in, wal_path_, "WAL checkpoint sequence");
      const auto wal_dirty_count =
        read_pod<std::uint64_t>(in, wal_path_, "WAL dirty count");
      const auto wal_checksum =
        read_pod<std::uint32_t>(in, wal_path_, "WAL checksum");

      ah_runtime_error_unless(wal_checksum == wal_header_checksum(
                                wal_version, wal_size, wal_root_page,
                                wal_page_count,
                                wal_free_head, wal_checkpoint_sequence,
                                wal_dirty_count))
        << "File_B_Tree: WAL checksum mismatch in " << wal_path_;

      std::error_code ec;
      const auto actual_size = fs::file_size(wal_path_, ec);
      ah_runtime_error_unless(not ec)
        << "File_B_Tree: cannot determine the size of WAL " << wal_path_
        << " (" << ec.message() << ")";
      const bool has_commit_trailer = wal_version >= CommitTrailerWalVersion;
      const auto expected_size = wal_header_bytes()
                                 + wal_dirty_count * wal_record_bytes(wal_version)
                                 + (has_commit_trailer ? wal_trailer_bytes() : 0);
      ah_runtime_error_unless(actual_size == expected_size)
        << "File_B_Tree: WAL size mismatch in " << wal_path_
        << " (expected " << expected_size << ", got " << actual_size << ")";

      if (file_.is_open())
        file_.close();

      if (not file_exists())
        open_storage(true);
      else
        open_storage(false);

      Array<Wal_Record> recovered_records;
      std::uint32_t payload_crc = detail::crc32_begin();
      for (std::uint64_t i = 0; i < wal_dirty_count; ++i)
        {
          const auto page_id = read_pod<page_id_t>(in, wal_path_, "WAL page id");
          ah_runtime_error_unless(page_id > 0 and page_id <= wal_page_count)
            << "File_B_Tree: invalid WAL page id " << page_id
            << " in " << wal_path_;
          const Page recovered = read_page_from_stream(
            in, wal_path_, wal_record_format_version(wal_version));
          if (wal_version >= PortableWalVersion)
            payload_crc = wal_record_checksum_add(payload_crc, page_id, recovered);
          else if (wal_version >= NativeWalVersion)
            payload_crc = wal_record_checksum_add_v3(payload_crc, page_id,
                                                     recovered);
          else if (has_commit_trailer)
            payload_crc = wal_record_checksum_add_v2(payload_crc, page_id,
                                                     recovered);
          recovered_records.append(Wal_Record{page_id, recovered});
        }

      if (has_commit_trailer)
        {
          const auto expected_trailer_magic = wal_trailer_magic();
          std::array<char, detail::Ordered_Tree_Snapshot_Magic_Size>
            trailer_magic = {};
          in.read(trailer_magic.data(), trailer_magic.size());
          ah_runtime_error_unless(in.good())
            << "File_B_Tree: cannot read WAL commit trailer from "
            << wal_path_;
          ah_runtime_error_unless(trailer_magic == expected_trailer_magic)
            << "File_B_Tree: missing WAL commit trailer in " << wal_path_;

          const auto trailer_checkpoint_sequence =
            read_pod<std::uint64_t>(in, wal_path_,
                                    "WAL trailer checkpoint sequence");
          const auto trailer_dirty_count =
            read_pod<std::uint64_t>(in, wal_path_, "WAL trailer dirty count");
          const auto trailer_payload_checksum =
            read_pod<std::uint32_t>(in, wal_path_, "WAL payload checksum");

          ah_runtime_error_unless(trailer_checkpoint_sequence
                                  == wal_checkpoint_sequence)
            << "File_B_Tree: WAL trailer checkpoint mismatch in "
            << wal_path_;
          ah_runtime_error_unless(trailer_dirty_count == wal_dirty_count)
            << "File_B_Tree: WAL trailer dirty-count mismatch in "
            << wal_path_;
          ah_runtime_error_unless(trailer_payload_checksum
                                  == detail::crc32_finish(payload_crc))
            << "File_B_Tree: WAL payload checksum mismatch in " << wal_path_;
        }

      bool stale_wal = false;
      if (storage_format_version_on_disk() == std::optional<std::uint32_t>(FormatVersion))
        {
          // Verify the on-disk header already reflects wal_checkpoint_sequence
          // before treating the WAL as stale.  When recovered_records is empty
          // (dirty_count == 0) the per-page loop below does not execute, which
          // would wrongly leave stale_wal=true even when the header write had
          // not yet been flushed to disk at crash time.
          bool header_current = false;
          try
            {
              const Loaded_Image disk = read_image(file_path_);
              header_current =
                disk.checkpoint_sequence >= wal_checkpoint_sequence;
            }
          catch (...) {}

          if (header_current)
            {
              stale_wal = true;
              for (size_t i = 0; i < recovered_records.size(); ++i)
                {
                  const auto persisted =
                    read_storage_page_if_current(recovered_records[i].page_id);
                  if (not persisted.has_value()
                      or persisted->checkpoint_sequence < wal_checkpoint_sequence)
                    {
                      stale_wal = false;
                      break;
                    }
                }
            }
        }

      if (stale_wal)
        {
          detail::remove_file_if_exists(wal_path_, tree_kind());
          return;
        }

      write_header_to_storage(wal_size, wal_root_page, wal_page_count,
                              wal_free_head, wal_checkpoint_sequence);
      for (size_t i = 0; i < recovered_records.size(); ++i)
        {
          const auto persisted =
            read_storage_page_if_current(recovered_records[i].page_id);
          if (persisted.has_value()
              and persisted->checkpoint_sequence >= wal_checkpoint_sequence)
            continue;
          write_current_page_to_storage(recovered_records[i].page_id,
                                        recovered_records[i].page);
        }

      file_.flush();
      ah_runtime_error_unless(file_.good())
        << "File_B_Tree: cannot flush recovered data to " << file_path_;
      detail::sync_file_by_path(file_path_, tree_kind());
      checkpoint_sequence_ = wal_checkpoint_sequence;
      storage_format_version_ = FormatVersion;
      detail::remove_file_if_exists(wal_path_, tree_kind());
    }

    void recover_if_needed()
    {
      namespace fs = std::filesystem;
      recover_from_wal();
      if (read_only())
        {
          ah_runtime_error_unless(not fs::exists(journal_tmp_path_)
                                  and not fs::exists(journal_path_))
            << "File_B_Tree: cannot open read-only while journal recovery "
            << "sidecars are pending for " << file_path_;
          return;
        }

      detail::remove_file_if_exists(journal_tmp_path_, tree_kind());
      if (not fs::exists(journal_path_))
        return;

      (void) read_image(journal_path_);

      if (file_.is_open())
        file_.close();

      detail::copy_file_contents(journal_path_, file_path_, tree_kind());
      detail::remove_file_if_exists(journal_path_, tree_kind());
    }

    [[nodiscard]] Loaded_Image read_image(const std::string & source_path) const
    {
      namespace fs = std::filesystem;

      std::ifstream in(source_path, std::ios::binary);
      ah_runtime_error_unless(in.good())
        << "File_B_Tree: cannot open " << source_path << " for reading";

      const auto expected_magic = file_magic();
      std::array<char, detail::Ordered_Tree_Snapshot_Magic_Size> magic = {};
      in.read(magic.data(), magic.size());
      ah_runtime_error_unless(in.good())
        << "File_B_Tree: cannot read header magic from " << source_path;

      ah_runtime_error_unless(magic == expected_magic)
        << "File_B_Tree: file " << source_path
        << " does not contain a compatible paged B-Tree";

      const auto version =
        read_pod<std::uint32_t>(in, source_path, "header version");
      ah_runtime_error_unless(version == LegacyFormatVersion
                              or version == ChecksummedFormatVersion
                              or version == HeaderCheckpointFormatVersion
                              or version == NativeCheckpointFormatVersion
                              or version == FormatVersion)
        << "File_B_Tree: unsupported format version " << version
        << " in " << source_path;
      if (version < PortableFormatVersion)
        ensure_legacy_native_format_supported(source_path, version);

      const auto key_size =
        read_pod<std::uint32_t>(in, source_path, "header key size");
      ah_runtime_error_unless(key_size == key_size_for_version(version))
        << "File_B_Tree: file " << source_path << " was created for key size "
        << key_size << ", but this instantiation expects "
        << key_size_for_version(version);

      const auto min_degree =
        read_pod<std::uint64_t>(in, source_path, "header minimum degree");
      ah_runtime_error_unless(min_degree == MinDegree)
        << "File_B_Tree: file " << source_path
        << " was created with minimum degree " << min_degree
        << ", but this instantiation expects " << MinDegree;

      const auto encoding =
        read_pod<std::uint8_t>(in, source_path, "header encoding");
      ah_runtime_error_unless(encoding == encoding_marker_for_version(version))
        << "File_B_Tree: file " << source_path
        << " was created with an incompatible encoding";

      std::array<char, 7> reserved = {};
      in.read(reserved.data(), reserved.size());
      ah_runtime_error_unless(in.good())
        << "File_B_Tree: cannot read reserved header bytes from " << source_path;
      ah_runtime_error_unless(reserved == reserved_bytes_for_version(version))
        << "File_B_Tree: file " << source_path
        << " was created with an incompatible key codec";

      Loaded_Image image;
      image.version = version;
      image.size = read_pod<std::uint64_t>(in, source_path, "header size");
      image.root_page = read_pod<page_id_t>(in, source_path, "header root page");
      const auto page_count =
        read_pod<std::uint64_t>(in, source_path, "header page count");
      image.free_page_head = read_pod<page_id_t>(in, source_path,
                                                 "header free page head");

      if (version >= HeaderCheckpointFormatVersion)
        {
          image.checkpoint_sequence =
            read_pod<std::uint64_t>(in, source_path,
                                    "header checkpoint sequence");
          const auto stored = read_pod<std::uint32_t>(in, source_path,
                                                      "header checksum");
          ah_runtime_error_unless(stored == header_checksum(
                                    version,
                                    static_cast<std::uint64_t>(image.size),
                                    image.root_page, page_count,
                                    image.free_page_head,
                                    image.checkpoint_sequence))
            << "File_B_Tree: header checksum mismatch in " << source_path;
        }
      else if (version >= ChecksummedFormatVersion)
        {
          const auto stored = read_pod<std::uint32_t>(in, source_path,
                                                      "header checksum");
          ah_runtime_error_unless(stored == header_checksum_v2(
                                    version, static_cast<std::uint64_t>(image.size),
                                    image.root_page, page_count,
                                    image.free_page_head))
            << "File_B_Tree: header checksum mismatch in " << source_path;
        }

      std::error_code ec;
      const auto actual_size = fs::file_size(source_path, ec);
      ah_runtime_error_unless(not ec)
        << "File_B_Tree: cannot determine the size of " << source_path
        << " (" << ec.message() << ")";
      const auto expected_size =
        serialized_header_bytes(version) + page_count * serialized_page_bytes(version);
      ah_runtime_error_unless(actual_size == expected_size)
        << "File_B_Tree: file size mismatch in " << source_path
        << " (expected " << expected_size << ", got " << actual_size << ")";

      image.pages.empty();
      image.pages.reserve(page_count + 1);
      image.pages.append(Page{});
      for (page_id_t page_id = 1; page_id <= page_count; ++page_id)
        image.pages.append(read_page_from_stream(in, source_path, version));

      ah_runtime_error_unless(image.root_page <= page_count)
        << "File_B_Tree: invalid root page id in " << source_path;
      ah_runtime_error_unless(image.free_page_head <= page_count)
        << "File_B_Tree: invalid free-list head in " << source_path;

      return image;
    }

    void load_from_disk()
    {
      Loaded_Image image = read_image(file_path_);

      size_ = image.size;
      root_page_ = image.root_page;
      free_page_head_ = image.free_page_head;
      checkpoint_sequence_ = image.checkpoint_sequence;
      storage_format_version_ = image.version;
      pages_ = std::move(image.pages);

      dirty_pages_.empty();
      dirty_pages_.reserve(pages_.size());
      for (size_t i = 0; i < pages_.size(); ++i)
        dirty_pages_.append(0);

      header_dirty_ = false;
      ah_runtime_error_unless(verify())
        << "File_B_Tree: structural verification failed for " << file_path_;
      open_storage(false);
    }

    void create_empty_file()
    {
      ensure_writable("create_empty_file");
      init_empty_cache();
      open_storage(true);
      sync();
    }

    [[nodiscard]] bool file_exists() const
    {
      return std::ifstream(file_path_, std::ios::binary).good();
    }

    [[nodiscard]] std::optional<std::uint32_t> storage_format_version_on_disk() const
    {
      try
        {
          if (not file_exists())
            return std::nullopt;

          std::ifstream in(file_path_, std::ios::binary);
          if (not in.good())
            return std::nullopt;

          std::array<char, detail::Ordered_Tree_Snapshot_Magic_Size> magic = {};
          in.read(magic.data(), magic.size());
          if (not in.good() or magic != file_magic())
            return std::nullopt;

          return read_pod<std::uint32_t>(in, file_path_, "header version");
        }
      catch (...)
        {
          return std::nullopt;
        }
    }

    [[nodiscard]] std::optional<Page>
    read_storage_page_if_current(const page_id_t page_id) const
    {
      if (storage_format_version_on_disk() != std::optional<std::uint32_t>(FormatVersion))
        return std::nullopt;

      if (not file_.is_open())
        open_storage(false);

      file_.clear();
      file_.seekg(0, std::ios::end);
      if (not file_.good())
        return std::nullopt;

      const auto file_size = file_.tellg();
      const auto offset = current_page_offset(page_id);
      if (file_size < 0
          or offset < 0
          or file_size - offset < static_cast<std::streamoff>(page_bytes()))
        return std::nullopt;

      file_.clear();
      file_.seekg(offset);
      if (not file_.good())
        return std::nullopt;

      try
        {
          return read_page_from_stream(file_, file_path_, FormatVersion);
        }
      catch (...)
        {
          file_.clear();
          return std::nullopt;
        }
    }

    [[nodiscard]] bool has_pending_changes() const noexcept
    {
      if (header_dirty_)
        return true;

      for (page_id_t page_id = 1; page_id < dirty_pages_.size(); ++page_id)
        if (dirty_pages_[page_id] != 0)
          return true;
      return false;
    }

    void clear_dirty_state() const noexcept
    {
      header_dirty_ = false;
      for (page_id_t page_id = 1; page_id < dirty_pages_.size(); ++page_id)
        dirty_pages_(page_id) = 0;
    }

    void mark_page_dirty(const page_id_t page_id)
    {
      page(page_id).checkpoint_sequence = checkpoint_sequence_ + 1;
      dirty_pages_(page_id) = 1;
    }

    void stamp_all_pages_for_checkpoint(
      const std::uint64_t checkpoint_sequence) noexcept
    {
      for (page_id_t page_id = 1; page_id < pages_.size(); ++page_id)
        page(page_id).checkpoint_sequence = checkpoint_sequence;
    }

    void mark_header_dirty() const noexcept
    {
      header_dirty_ = true;
    }

    [[nodiscard]] Page & page(const page_id_t page_id)
    {
      ah_runtime_error_unless(page_id > 0 and page_id < pages_.size())
        << "File_B_Tree: invalid page id " << page_id;
      return pages_(page_id);
    }

    [[nodiscard]] const Page & page(const page_id_t page_id) const
    {
      ah_runtime_error_unless(page_id > 0 and page_id < pages_.size())
        << "File_B_Tree: invalid page id " << page_id;
      return pages_(page_id);
    }

    [[nodiscard]] bool equals(const Key & lhs, const Key & rhs) const
    {
      return not cmp_(lhs, rhs) and not cmp_(rhs, lhs);
    }

    [[nodiscard]] bool strictly_sorted(const Page & page) const
    {
      if (page.key_count == 0)
        return true;

      for (size_t i = 1; i < page.key_count; ++i)
        if (not cmp_(page.keys[i - 1], page.keys[i]))
          return false;
      return true;
    }

    [[nodiscard]] size_t lower_bound_index(const Page & page,
                                           const Key & key) const
    {
      size_t idx = 0;
      while (idx < page.key_count and cmp_(page.keys[idx], key))
        ++idx;
      return idx;
    }

    [[nodiscard]] size_t upper_bound_index(const Page & page,
                                           const Key & key) const
    {
      size_t idx = 0;
      while (idx < page.key_count
             and (cmp_(page.keys[idx], key) or equals(page.keys[idx], key)))
        ++idx;
      return idx;
    }

    static void insert_key_at(Page & page, const size_t idx, const Key & key)
    {
      for (size_t i = page.key_count; i > idx; --i)
        page.keys[i] = page.keys[i - 1];
      page.keys[idx] = key;
      ++page.key_count;
    }

    static Key erase_key_at(Page & page, const size_t idx)
    {
      const Key ret = page.keys[idx];
      for (size_t i = idx + 1; i < page.key_count; ++i)
        page.keys[i - 1] = page.keys[i];
      --page.key_count;
      return ret;
    }

    static void insert_child_at(Page & page, const size_t idx,
                                const page_id_t child_id)
    {
      for (size_t i = page.child_count; i > idx; --i)
        page.children[i] = page.children[i - 1];
      page.children[idx] = child_id;
      ++page.child_count;
    }

    static page_id_t erase_child_at(Page & page, const size_t idx)
    {
      const page_id_t ret = page.children[idx];
      for (size_t i = idx + 1; i < page.child_count; ++i)
        page.children[i - 1] = page.children[i];
      --page.child_count;
      return ret;
    }

    page_id_t allocate_page(const PageKind kind)
    {
      if (free_page_head_ != 0)
        {
          const page_id_t page_id = free_page_head_;
          free_page_head_ = page(page_id).link;
          page(page_id) = make_page(kind);
          mark_page_dirty(page_id);
          mark_header_dirty();
          return page_id;
        }

      pages_.append(make_page(kind));
      dirty_pages_.append(0);
      mark_page_dirty(pages_.size() - 1);
      mark_header_dirty();
      return pages_.size() - 1;
    }

    void release_page(const page_id_t page_id)
    {
      Page free_page = make_page(PageKind::Free);
      free_page.link = free_page_head_;
      page(page_id) = free_page;
      free_page_head_ = page_id;
      mark_page_dirty(page_id);
      mark_header_dirty();
    }

    [[nodiscard]] std::optional<Key> min_in(page_id_t page_id) const
    {
      if (page_id == 0)
        return std::nullopt;

      while (is_internal(page(page_id)))
        {
          const Page & curr = page(page_id);
          if (curr.child_count == 0)
            return std::nullopt;
          page_id = curr.children[0];
        }

      const Page & leaf = page(page_id);
      if (leaf.key_count == 0)
        return std::nullopt;
      return leaf.keys[0];
    }

    [[nodiscard]] std::optional<Key> max_in(page_id_t page_id) const
    {
      if (page_id == 0)
        return std::nullopt;

      while (is_internal(page(page_id)))
        {
          const Page & curr = page(page_id);
          if (curr.child_count == 0)
            return std::nullopt;
          page_id = curr.children[curr.child_count - 1];
        }

      const Page & leaf = page(page_id);
      if (leaf.key_count == 0)
        return std::nullopt;
      return leaf.keys[leaf.key_count - 1];
    }

    void split_child(const page_id_t parent_id, const size_t idx)
    {
      const page_id_t child_id = page(parent_id).children[idx];
      const bool child_is_leaf = is_leaf(page(child_id));
      const page_id_t right_id =
        allocate_page(child_is_leaf ? PageKind::Leaf : PageKind::Internal);

      const Key median = page(child_id).keys[MinKeys];

      {
        Page & child = page(child_id);
        Page & right = page(right_id);

        for (size_t i = MinDegree; i < child.key_count; ++i)
          right.keys[right.key_count++] = child.keys[i];
        child.key_count = MinKeys;

        if (not child_is_leaf)
          {
            for (size_t i = MinDegree; i < child.child_count; ++i)
              right.children[right.child_count++] = child.children[i];
            child.child_count = MinDegree;
          }
      }

      insert_key_at(page(parent_id), idx, median);
      insert_child_at(page(parent_id), idx + 1, right_id);
      mark_page_dirty(child_id);
      mark_page_dirty(right_id);
      mark_page_dirty(parent_id);
    }

    void insert_nonfull(const page_id_t page_id, const Key & key)
    {
      if (is_leaf(page(page_id)))
        {
          const size_t idx = lower_bound_index(page(page_id), key);
          insert_key_at(page(page_id), idx, key);
          mark_page_dirty(page_id);
          return;
        }

      size_t idx = lower_bound_index(page(page_id), key);
      const page_id_t child_id = page(page_id).children[idx];
      if (page(child_id).key_count == MaxKeys)
        {
          split_child(page_id, idx);
          if (cmp_(page(page_id).keys[idx], key))
            ++idx;
        }
      insert_nonfull(page(page_id).children[idx], key);
    }

    void borrow_from_prev(const page_id_t parent_id, const size_t idx)
    {
      const page_id_t child_id = page(parent_id).children[idx];
      const page_id_t left_id = page(parent_id).children[idx - 1];

      insert_key_at(page(child_id), 0, page(parent_id).keys[idx - 1]);
      page(parent_id).keys[idx - 1] =
        page(left_id).keys[page(left_id).key_count - 1];
      erase_key_at(page(left_id), page(left_id).key_count - 1);

      if (not is_leaf(page(left_id)))
        {
          insert_child_at(page(child_id), 0,
                          page(left_id).children[page(left_id).child_count - 1]);
          erase_child_at(page(left_id), page(left_id).child_count - 1);
        }

      mark_page_dirty(parent_id);
      mark_page_dirty(child_id);
      mark_page_dirty(left_id);
    }

    void borrow_from_next(const page_id_t parent_id, const size_t idx)
    {
      const page_id_t child_id = page(parent_id).children[idx];
      const page_id_t right_id = page(parent_id).children[idx + 1];

      page(child_id).keys[page(child_id).key_count++] = page(parent_id).keys[idx];
      page(parent_id).keys[idx] = page(right_id).keys[0];
      erase_key_at(page(right_id), 0);

      if (not is_leaf(page(right_id)))
        {
          page(child_id).children[page(child_id).child_count++] = page(right_id).children[0];
          erase_child_at(page(right_id), 0);
        }

      mark_page_dirty(parent_id);
      mark_page_dirty(child_id);
      mark_page_dirty(right_id);
    }

    void merge_children(const page_id_t parent_id, const size_t idx)
    {
      const page_id_t left_id = page(parent_id).children[idx];
      const page_id_t right_id = page(parent_id).children[idx + 1];

      Page & left = page(left_id);
      const Page right = page(right_id);

      left.keys[left.key_count++] = page(parent_id).keys[idx];
      for (size_t i = 0; i < right.key_count; ++i)
        left.keys[left.key_count++] = right.keys[i];

      if (not is_leaf(left))
        for (size_t i = 0; i < right.child_count; ++i)
          left.children[left.child_count++] = right.children[i];

      erase_key_at(page(parent_id), idx);
      erase_child_at(page(parent_id), idx + 1);
      mark_page_dirty(left_id);
      mark_page_dirty(parent_id);
      release_page(right_id);
    }

    [[nodiscard]] size_t ensure_child_has_extra(const page_id_t parent_id,
                                                const size_t idx)
    {
      if (page(page(parent_id).children[idx]).key_count >= MinDegree)
        return idx;

      if (idx > 0
          and page(page(parent_id).children[idx - 1]).key_count >= MinDegree)
        {
          borrow_from_prev(parent_id, idx);
          return idx;
        }

      if (idx + 1 < page(parent_id).child_count
          and page(page(parent_id).children[idx + 1]).key_count >= MinDegree)
        {
          borrow_from_next(parent_id, idx);
          return idx;
        }

      if (idx + 1 < page(parent_id).child_count)
        {
          merge_children(parent_id, idx);
          return idx;
        }

      merge_children(parent_id, idx - 1);
      return idx - 1;
    }

    [[nodiscard]] bool remove_from(const page_id_t page_id, const Key & key)
    {
      const size_t idx = lower_bound_index(page(page_id), key);
      const bool found =
        idx < page(page_id).key_count and equals(page(page_id).keys[idx], key);

      if (found)
        {
          if (is_leaf(page(page_id)))
            {
              erase_key_at(page(page_id), idx);
              mark_page_dirty(page_id);
              return true;
            }

          const page_id_t left_child = page(page_id).children[idx];
          const page_id_t right_child = page(page_id).children[idx + 1];

          if (page(left_child).key_count >= MinDegree)
            {
              const Key pred = *max_in(left_child);
              page(page_id).keys[idx] = pred;
              mark_page_dirty(page_id);
              return remove_from(left_child, pred);
            }

          if (page(right_child).key_count >= MinDegree)
            {
              const Key succ = *min_in(right_child);
              page(page_id).keys[idx] = succ;
              mark_page_dirty(page_id);
              return remove_from(right_child, succ);
            }

          merge_children(page_id, idx);
          return remove_from(page(page_id).children[idx], key);
        }

      if (is_leaf(page(page_id)))
        return false;

      const size_t child_idx = ensure_child_has_extra(page_id, idx);
      return remove_from(page(page_id).children[child_idx], key);
    }

    void collect_keys(const page_id_t page_id, Array<Key> & out) const
    {
      if (page_id == 0)
        return;

      const Page & p = page(page_id);
      if (is_leaf(p))
        {
          for (size_t i = 0; i < p.key_count; ++i)
            out.append(p.keys[i]);
          return;
        }

      for (size_t i = 0; i < p.key_count; ++i)
        {
          collect_keys(p.children[i], out);
          out.append(p.keys[i]);
        }
      collect_keys(p.children[p.child_count - 1], out);
    }

    [[nodiscard]] bool verify_node(const page_id_t page_id,
                                   const std::optional<Key> & min_key,
                                   const std::optional<Key> & max_key,
                                   const size_t depth,
                                   size_t & leaf_depth,
                                   size_t & counted) const
    {
      if (page_id == 0 or page_id >= pages_.size())
        return false;

      const Page & p = page(page_id);
      if (is_free(p))
        return false;

      if (page_id != root_page_)
        {
          if (p.key_count < MinKeys or p.key_count > MaxKeys)
            return false;
        }
      else if (p.key_count > MaxKeys)
        return false;

      if (not strictly_sorted(p))
        return false;

      if (min_key.has_value() and p.key_count > 0
          and not cmp_(*min_key, p.keys[0]))
        return false;

      if (max_key.has_value() and p.key_count > 0
          and not cmp_(p.keys[p.key_count - 1], *max_key))
        return false;

      counted += p.key_count;

      if (is_leaf(p))
        {
          if (p.child_count != 0)
            return false;
          if (leaf_depth == static_cast<size_t>(-1))
            leaf_depth = depth;
          return leaf_depth == depth;
        }

      if (p.child_count != p.key_count + 1)
        return false;

      for (size_t i = 0; i < p.child_count; ++i)
        {
          if (p.children[i] == 0 or p.children[i] >= pages_.size())
            return false;

          std::optional<Key> child_min = min_key;
          std::optional<Key> child_max = max_key;
          if (i > 0)
            child_min = p.keys[i - 1];
          if (i < p.key_count)
            child_max = p.keys[i];

          if (not verify_node(p.children[i], child_min, child_max,
                              depth + 1, leaf_depth, counted))
            return false;
        }

      return true;
    }

    void sync_if_enabled()
    {
      if (auto_sync_)
        sync();
    }

  public:
    using key_type = Key; ///< Stored key type.
    using tree_type = Gen_B_Tree<Key, Compare, MinDegree>; ///< Logical in-memory counterpart.
    using page_id_type = page_id_t; ///< Type of on-disk page identifiers.
    using open_mode_type = Paged_File_Open_Mode; ///< File opening mode type.

    static constexpr auto Read_Write = Paged_File_Open_Mode::Read_Write;
    static constexpr auto Read_Only = Paged_File_Open_Mode::Read_Only;

    /** @brief Open or create a paged persistent B-Tree file for read-write access.
     *  @param file_path Path to the backing file.
     *  @param auto_sync If `true`, successful mutations flush dirty pages and
     *         the header to disk.
     *  @param cmp Comparison functor used to order keys.
     *  @throws std::runtime_error If the file cannot be created, opened, read,
     *          or validated.
     *  @throws std::bad_alloc If the in-memory page cache cannot be allocated.
     */
    explicit Gen_File_B_Tree(std::string file_path,
                             const bool auto_sync = true,
                             const Compare & cmp = Compare())
      : Gen_File_B_Tree(std::move(file_path), Paged_File_Open_Mode::Read_Write,
                        auto_sync, cmp) {}

    /** @brief Open a paged persistent B-Tree file with an explicit access mode.
     *  @param file_path Path to the backing file.
     *  @param open_mode `Read_Write` to allow mutations and recovery, or
     *         `Read_Only` for shared-lock read access to an already clean file.
     *  @param auto_sync If `true`, successful mutations flush dirty pages and
     *         the header to disk. Read-only handles always behave as if this
     *         were `false`.
     *  @param cmp Comparison functor used to order keys.
     *  @throws std::runtime_error If the file cannot be created, opened, read,
     *          or validated, or if a read-only open would require recovery.
     *  @throws std::bad_alloc If the in-memory page cache cannot be allocated.
     */
    explicit Gen_File_B_Tree(std::string file_path,
                             const Paged_File_Open_Mode open_mode,
                             const bool auto_sync = true,
                             const Compare & cmp = Compare())
      : cmp_(cmp),
        file_path_(std::move(file_path)),
        open_mode_(open_mode),
        auto_sync_(open_mode == Paged_File_Open_Mode::Read_Only ? false
                                                                : auto_sync)
    {
      ah_runtime_error_unless(not file_path_.empty())
        << "File_B_Tree requires a non-empty file path";

      init_sidecar_paths();
      lock_.acquire(lock_path_, tree_kind(), lock_mode());
      if (read_only())
        ah_runtime_error_unless(file_exists())
          << "File_B_Tree: cannot open missing file " << file_path_
          << " in read-only mode";
      recover_if_needed();

      if (file_exists())
        {
          open_storage(false);
          file_.seekg(0, std::ios::end);
          const std::streamoff file_size = file_.tellg();
          ah_runtime_error_unless(file_size >= 0)
            << "File_B_Tree: cannot determine the size of " << file_path_;

          if (file_size == 0)
            {
              ensure_writable("constructor");
              create_empty_file();
            }
          else
            load_from_disk();
        }
      else
        create_empty_file();
    }

    /** @brief Open or create a paged persistent B-Tree file for read-write access.
     *  @param file_path Path to the backing file.
     *  @param auto_sync If `true`, successful mutations flush dirty pages and
     *         the header to disk.
     *  @param cmp Comparison functor used to order keys.
     *  @throws std::runtime_error If the file cannot be created, opened, read,
     *          or validated.
     *  @throws std::bad_alloc If the in-memory page cache cannot be allocated.
     */
    explicit Gen_File_B_Tree(const char * file_path,
                             const bool auto_sync = true,
                             const Compare & cmp = Compare())
      : Gen_File_B_Tree(file_path == nullptr ? std::string()
                                             : std::string(file_path),
                        auto_sync, cmp) {}

    /** @brief Open a paged persistent B-Tree file with an explicit access mode.
     *  @param file_path Path to the backing file.
     *  @param open_mode `Read_Write` to allow mutations and recovery, or
     *         `Read_Only` for shared-lock read access to an already clean file.
     *  @param auto_sync If `true`, successful mutations flush dirty pages and
     *         the header to disk. Read-only handles always behave as if this
     *         were `false`.
     *  @param cmp Comparison functor used to order keys.
     *  @throws std::runtime_error If the file cannot be created, opened, read,
     *          or validated, or if a read-only open would require recovery.
     *  @throws std::bad_alloc If the in-memory page cache cannot be allocated.
     */
    explicit Gen_File_B_Tree(const char * file_path,
                             const Paged_File_Open_Mode open_mode,
                             const bool auto_sync = true,
                             const Compare & cmp = Compare())
      : Gen_File_B_Tree(file_path == nullptr ? std::string()
                                             : std::string(file_path),
                        open_mode, auto_sync, cmp) {}

    /** @brief Copying is disabled. */
    Gen_File_B_Tree(const Gen_File_B_Tree &) = delete;

    /** @brief Assignment is disabled. */
    Gen_File_B_Tree & operator = (const Gen_File_B_Tree &) = delete;

    /** @brief Moving is disabled. */
    Gen_File_B_Tree(Gen_File_B_Tree &&) = delete;

    /** @brief Move assignment is disabled. */
    Gen_File_B_Tree & operator = (Gen_File_B_Tree &&) = delete;

    /** @brief Close the backing file.
     *  @throws Nothing.
     */
    ~Gen_File_B_Tree() noexcept
    {
      if (file_.is_open())
        file_.close();
    }

    /** @brief Return the comparison object.
     *  @return Constant reference to the comparison functor.
     *  @note The comparator is immutable once the tree is persisted; no
     *        mutable accessor is provided to prevent callers from replacing
     *        the comparator after page splits/merges have been written to
     *        disk.
     *  @throws Nothing.
     */
    [[nodiscard]] const Compare & key_comp() const noexcept { return cmp_; }

    /** @brief Return the comparison object.
     *  @return Constant reference to the comparison functor.
     *  @throws Nothing.
     */
    [[nodiscard]] const Compare & get_compare() const noexcept
    {
      return key_comp();
    }

    /** @brief Return the backing file path.
     *  @return Constant reference to the file path passed at construction.
     *  @throws Nothing.
     */
    [[nodiscard]] const std::string & file_path() const noexcept
    {
      return file_path_;
    }

    /** @brief Synonym for file_path().
     *  @return Constant reference to the file path passed at construction.
     *  @throws Nothing.
     */
    [[nodiscard]] const std::string & get_file_path() const noexcept
    {
      return file_path();
    }

    /** @brief Return the access mode used to open the file.
     *  @return `Read_Write` or `Read_Only`.
     *  @throws Nothing.
     */
    [[nodiscard]] Paged_File_Open_Mode open_mode() const noexcept
    {
      return open_mode_;
    }

    /** @brief Return whether this handle is read-only.
     *  @return `true` if opened in `Read_Only` mode, otherwise `false`.
     *  @throws Nothing.
     */
    [[nodiscard]] bool is_read_only() const noexcept
    {
      return read_only();
    }

    /** @brief Return whether automatic synchronization is enabled.
     *  @return `true` if successful mutations call `sync()`.
     *  @throws Nothing.
     */
    [[nodiscard]] bool auto_sync_enabled() const noexcept
    {
      return auto_sync_;
    }

    /** @brief Enable or disable automatic flushing of dirty pages.
     *  @param enabled New auto-sync mode.
     *  @throws Nothing.
     */
    void set_auto_sync(const bool enabled) noexcept
    {
      auto_sync_ = enabled;
    }

    /** @brief Return the fixed serialized page size.
     *  @return Number of bytes used by each page record in the file.
     *  @throws Nothing.
     */
    [[nodiscard]] size_t page_size_bytes() const noexcept
    {
      return page_bytes();
    }

    /** @brief Return the number of allocated pages currently tracked.
     *  @return Number of pages in the page cache, excluding page id 0.
     *  @throws Nothing.
     */
    [[nodiscard]] size_t page_count() const noexcept
    {
      return pages_.size() - 1;
    }

    /** @brief Return the durable checkpoint sequence stored in the backing file.
     *  @return Monotone sequence advanced after each successful checkpoint.
     *  @throws Nothing.
     */
    [[nodiscard]] std::uint64_t checkpoint_sequence() const noexcept
    {
      return checkpoint_sequence_;
    }

    /** @brief Synonym for sync().
     *  @throws std::runtime_error If the file cannot be written or flushed.
     */
    void checkpoint() const
    {
      sync();
    }

    /** @brief Flush the current tree image using redo WAL or full-image fallback.
     *  @throws std::runtime_error If the file cannot be written or flushed.
     */
    void sync() const
    {
      ensure_writable("sync");
      if (not has_pending_changes() and file_exists())
        return;

      const auto target_checkpoint = checkpoint_sequence_ + 1;

      if (file_exists() and storage_format_version_ == FormatVersion)
        {
          write_wal_to_path(wal_tmp_path_, target_checkpoint);
          detail::rename_file_atomic(wal_tmp_path_, wal_path_, tree_kind());
          apply_current_cache_to_storage(target_checkpoint);
          detail::remove_file_if_exists(wal_path_, tree_kind());
          clear_dirty_state();
          return;
        }

      const_cast<Gen_File_B_Tree *>(this)->stamp_all_pages_for_checkpoint(
        target_checkpoint);
      write_full_image_to_path(journal_tmp_path_, target_checkpoint);
      detail::rename_file_atomic(journal_tmp_path_, journal_path_, tree_kind());
      apply_current_cache_to_storage(target_checkpoint);
      detail::remove_file_if_exists(journal_path_, tree_kind());
      clear_dirty_state();
    }

    /** @brief Discard unsynchronized changes and reload pages from disk.
     *  @throws std::runtime_error If the file cannot be reopened or validated.
     *  @throws std::bad_alloc If reloading the page cache exhausts memory.
     */
    void reload()
    {
      recover_if_needed();
      ah_runtime_error_unless(file_exists())
        << "File_B_Tree: backing file " << file_path_ << " no longer exists";
      load_from_disk();
    }

    /** @brief Return whether the tree has no keys.
     *  @return `true` if empty, otherwise `false`.
     *  @throws Nothing.
     */
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }

    /** @brief Synonym for empty().
     *  @return `true` if empty, otherwise `false`.
     *  @throws Nothing.
     */
    [[nodiscard]] bool is_empty() const noexcept { return empty(); }

    /** @brief Return the number of stored keys.
     *  @return Number of unique keys stored in the file-backed tree.
     *  @throws Nothing.
     */
    [[nodiscard]] size_t size() const noexcept { return size_; }

    /** @brief Return the current height of the paged tree.
     *  @return `0` for an empty tree, otherwise the number of node levels.
     *  @throws Nothing.
     */
    [[nodiscard]] size_t height() const noexcept
    {
      size_t h = 0;
      for (page_id_t curr = root_page_; curr != 0; )
        {
          ++h;
          if (is_leaf(page(curr)))
            break;
          curr = page(curr).children[0];
        }
      return h;
    }

    /** @brief Remove every key from the tree.
     *  @throws std::runtime_error If `auto_sync` is enabled and flushing fails.
     */
    void clear()
    {
      ensure_writable("clear");
      init_empty_cache();
      sync_if_enabled();
    }

    /** @brief Return whether a key is present.
     *  @param key Key to search.
     *  @return `true` if the key exists, otherwise `false`.
     *  @throws Any exception propagated by `Compare`.
     */
    [[nodiscard]] bool contains(const Key & key) const
    {
      page_id_t curr = root_page_;
      while (curr != 0)
        {
          const size_t idx = lower_bound_index(page(curr), key);
          if (idx < page(curr).key_count and equals(page(curr).keys[idx], key))
            return true;
          if (is_leaf(page(curr)))
            return false;
          curr = page(curr).children[idx];
        }
      return false;
    }

    /** @brief Synonym for contains().
     *  @param key Key to search.
     *  @return `true` if the key exists, otherwise `false`.
     *  @throws Any exception propagated by `Compare`.
     */
    [[nodiscard]] bool search(const Key & key) const
    {
      return contains(key);
    }

    /** @brief Insert a key if it is not already present.
     *  @param key Key to insert.
     *  @return `true` if insertion happened, `false` if it was duplicate.
     *  @throws std::bad_alloc If the page cache grows and allocation fails.
     *  @throws std::runtime_error If `auto_sync` is enabled and flushing fails.
     *  @throws Any exception propagated by `Compare` or `Key` copies.
     */
    bool insert(const Key & key)
    {
      ensure_writable("insert");
      if (contains(key))
        return false;

      if (root_page_ == 0)
        {
          const page_id_t leaf_id = allocate_page(PageKind::Leaf);
          page(leaf_id).keys[0] = key;
          page(leaf_id).key_count = 1;
          root_page_ = leaf_id;
          ++size_;
          mark_page_dirty(leaf_id);
          mark_header_dirty();
          sync_if_enabled();
          return true;
        }

      if (page(root_page_).key_count == MaxKeys)
        {
          const page_id_t new_root_id = allocate_page(PageKind::Internal);
          page(new_root_id).children[0] = root_page_;
          page(new_root_id).child_count = 1;
          split_child(new_root_id, 0);
          root_page_ = new_root_id;
          mark_header_dirty();
        }

      insert_nonfull(root_page_, key);
      ++size_;
      mark_header_dirty();
      sync_if_enabled();
      return true;
    }

    /** @brief Insert a key by move if it is not already present.
     *  @param key Key to insert.
     *  @return `true` if insertion happened, `false` if it was duplicate.
     *  @throws std::bad_alloc If the page cache grows and allocation fails.
     *  @throws std::runtime_error If `auto_sync` is enabled and flushing fails.
     *  @throws Any exception propagated by `Compare` or `Key` copies.
     */
    bool insert(Key && key)
    {
      Key copy = std::move(key);
      return insert(copy);
    }

    /** @brief Remove a key if present.
     *  @param key Key to erase.
     *  @return `true` if the key was removed, otherwise `false`.
     *  @throws std::runtime_error If `auto_sync` is enabled and flushing fails.
     *  @throws Any exception propagated by `Compare` or `Key` assignments.
     */
    bool remove(const Key & key)
    {
      ensure_writable("remove");
      if (root_page_ == 0)
        return false;

      if (not remove_from(root_page_, key))
        return false;

      --size_;
      mark_header_dirty();

      while (root_page_ != 0 and page(root_page_).key_count == 0)
        {
          if (is_leaf(page(root_page_)))
            {
              release_page(root_page_);
              root_page_ = 0;
              mark_header_dirty();
              break;
            }

          if (page(root_page_).child_count == 0)
            {
              release_page(root_page_);
              root_page_ = 0;
              mark_header_dirty();
              break;
            }

          const page_id_t old_root = root_page_;
          root_page_ = page(old_root).children[0];
          release_page(old_root);
          mark_header_dirty();
        }

      sync_if_enabled();
      return true;
    }

    /** @brief Return the minimum key, if any.
     *  @return Smallest key in the tree or `std::nullopt` if empty.
     *  @throws Any exception propagated by `Key` copy construction.
     */
    [[nodiscard]] std::optional<Key> min_key() const
    {
      return min_in(root_page_);
    }

    /** @brief Return the maximum key, if any.
     *  @return Largest key in the tree or `std::nullopt` if empty.
     *  @throws Any exception propagated by `Key` copy construction.
     */
    [[nodiscard]] std::optional<Key> max_key() const
    {
      return max_in(root_page_);
    }

    /** @brief Return the first key not less than `key`.
     *  @param key Probe key.
     *  @return Matching lower bound or `std::nullopt` if none exists.
     *  @throws Any exception propagated by `Compare` or `Key`.
     */
    [[nodiscard]] std::optional<Key> lower_bound(const Key & key) const
    {
      page_id_t curr = root_page_;
      std::optional<Key> candidate;

      while (curr != 0)
        {
          const size_t idx = lower_bound_index(page(curr), key);
          if (idx < page(curr).key_count)
            candidate = page(curr).keys[idx];
          if (is_leaf(page(curr)))
            return candidate;
          curr = page(curr).children[idx];
        }

      return candidate;
    }

    /** @brief Return the first key strictly greater than `key`.
     *  @param key Probe key.
     *  @return Matching upper bound or `std::nullopt` if none exists.
     *  @throws Any exception propagated by `Compare` or `Key`.
     */
    [[nodiscard]] std::optional<Key> upper_bound(const Key & key) const
    {
      page_id_t curr = root_page_;
      std::optional<Key> candidate;

      while (curr != 0)
        {
          const size_t idx = upper_bound_index(page(curr), key);
          if (idx < page(curr).key_count)
            candidate = page(curr).keys[idx];
          if (is_leaf(page(curr)))
            return candidate;
          curr = page(curr).children[idx];
        }

      return candidate;
    }

    /** @brief Materialize the tree contents in sorted order.
     *  @return Ordered array containing every stored key.
     *  @throws std::bad_alloc If the result array cannot grow.
     *  @throws Any exception propagated by `Key` copy construction.
     */
    [[nodiscard]] Array<Key> keys() const
    {
      Array<Key> out;
      out.reserve(size_ == 0 ? 1 : size_);
      collect_keys(root_page_, out);
      return out;
    }

    /** @brief Verify structural B-Tree invariants across cached pages.
     *  @return `true` if the tree is structurally valid, otherwise `false`.
     *  @throws Any exception propagated by `Compare` or `Key`.
     */
    [[nodiscard]] bool verify() const
    {
      if (root_page_ == 0)
        return size_ == 0;

      if (root_page_ >= pages_.size())
        return false;

      size_t leaf_depth = static_cast<size_t>(-1);
      size_t counted = 0;
      return verify_node(root_page_, std::nullopt, std::nullopt, 1,
                         leaf_depth, counted)
             and counted == size_;
    }
  };

  /** @brief Convenient alias for a page-managed persistent B-Tree. */
  template <typename Key,
            class Compare = Aleph::less<Key>,
            size_t MinDegree = 16,
            typename Codec = detail::Paged_Value_Codec<Key>>
  using File_B_Tree = Gen_File_B_Tree<Key, Compare, MinDegree, Codec>;
} // namespace Aleph

# endif // TPL_FILE_B_TREE_H
