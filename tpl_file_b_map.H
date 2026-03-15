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

/** @file tpl_file_b_map.H
 *  @brief Persistent key/value map built on top of Aleph::File_B_Tree.
 *
 *  This header provides \ref Aleph::File_B_Map, a persistent ordered map whose
 *  storage engine is the page-managed \ref Aleph::File_B_Tree.
 *
 *  @ingroup Trees
 */

# ifndef TPL_FILE_B_MAP_H
# define TPL_FILE_B_MAP_H

# include <array>
# include <bit>
# include <cstddef>
# include <cstring>
# include <exception>
# include <optional>
# include <ostream>
# include <string>
# include <type_traits>
# include <utility>

# include <ah-concepts.H>
# include <ah-errors.H>
# include <tpl_array.H>
# include <tpl_file_b_tree.H>

namespace Aleph
{
  /** @brief Persistent ordered map backed by a paged File_B_Tree.
   *
   *  `Gen_File_B_Map` stores unique keys together with mapped values in a
   *  persistent B-Tree file. It reuses the same format, locking, WAL,
   *  recovery, and read-only semantics as \ref Aleph::File_B_Tree.
   *
   *  @tparam Key Stored key type.
   *  @tparam Value Stored mapped-value type.
   *  @tparam Compare Strict weak ordering used to sort keys.
   *  @tparam MinDegree Minimum degree `t` of the underlying persistent B-Tree.
   *  @tparam KeyCodec Fixed-size codec used to persist keys.
   *  @tparam ValueCodec Fixed-size codec used to persist mapped values.
   *
   *  @warning `Key` and `Value` must admit fixed-size portable codecs because
   *  map entries are serialized through the paged storage engine of
   *  \ref Aleph::File_B_Tree. The default codecs cover fixed-size arithmetic
   *  types and `std::array`; bounded strings can be supported with
   *  \ref Aleph::detail::Paged_Bounded_String_Codec.
   *
   *  @note **Thread-safety:** const observers (accessors, `find`, `contains`,
   *  `at`, `min_item`, `max_item`, bounds, and materialization methods) are
   *  safe for concurrent access from multiple readers when the map is opened
   *  in `Read_Only` mode.  Mutating methods (`insert`, `insert_or_assign`,
   *  `remove`, `clear`, `sync`, `checkpoint`, `reload`, `set_auto_sync`)
   *  require external synchronization.  No method is individually atomic with
   *  respect to the backing file unless `auto_sync` is enabled or `sync()` is
   *  called explicitly after mutation.
   */
  template <typename Key,
            typename Value,
            class Compare = Aleph::less<Key>,
            size_t MinDegree = 16,
            typename KeyCodec = detail::Paged_Value_Codec<Key>,
            typename ValueCodec = detail::Paged_Value_Codec<Value>>
    requires StrictWeakOrder<Compare, Key>
  class Gen_File_B_Map
  {
    using key_codec = KeyCodec;
    using value_codec = ValueCodec;

    struct Record
    {
      std::array<unsigned char,
                 key_codec::encoded_size + value_codec::encoded_size> bytes = {};

      friend std::ostream & operator << (std::ostream & out, const Record &)
      {
        return out << "<record>";
      }

      [[nodiscard]] Key get_key() const
      {
        return key_codec::decode(bytes.data());
      }

      [[nodiscard]] Value get_value() const
      {
        return value_codec::decode(bytes.data() + key_codec::encoded_size);
      }
    };

    struct Record_Codec
    {
      static constexpr std::uint32_t storage_id =
        detail::mix_codec_id(detail::mix_codec_id(0x51000000u,
                                                  key_codec::storage_id),
                             value_codec::storage_id);
      static constexpr size_t encoded_size =
        key_codec::encoded_size + value_codec::encoded_size;

      static void encode(const Record & record, unsigned char * out) noexcept
      {
        std::memcpy(out, record.bytes.data(), record.bytes.size());
      }

      [[nodiscard]] static Record decode(const unsigned char * in) noexcept
      {
        Record record;
        std::memcpy(record.bytes.data(), in, record.bytes.size());
        return record;
      }

      [[nodiscard]] static std::uint32_t add_to_crc(
        const std::uint32_t crc,
        const Record & record) noexcept
      {
        return detail::crc32_add_bytes(crc, record.bytes.data(),
                                       record.bytes.size());
      }
    };

    struct Record_Compare
    {
      Compare cmp;

      bool operator () (const Record & lhs, const Record & rhs) const
      {
        return cmp(lhs.get_key(), rhs.get_key());
      }
    };

    using tree_impl_type =
      Gen_File_B_Tree<Record, Record_Compare, MinDegree, Record_Codec>;

    static_assert(detail::Fixed_Paged_Value_Codec<key_codec, Key>,
                  "File_B_Map requires a fixed-size key codec");
    static_assert(detail::Fixed_Paged_Value_Codec<value_codec, Value>,
                  "File_B_Map requires a fixed-size mapped-value codec");
    static_assert(std::is_trivially_copyable_v<Record>,
                  "File_B_Map requires trivially copyable records");

    tree_impl_type tree_;

    [[nodiscard]] static Record make_probe(const Key & key)
    {
      Record record;
      key_codec::encode(key, record.bytes.data());
      return record;
    }

    [[nodiscard]] static Record make_record(const Key & key,
                                            const Value & value)
    {
      Record record;
      key_codec::encode(key, record.bytes.data());
      value_codec::encode(value, record.bytes.data() + key_codec::encoded_size);
      return record;
    }

    [[nodiscard]] static std::pair<Key, Value>
    to_pair(const Record & record)
    {
      return std::pair<Key, Value>(record.get_key(), record.get_value());
    }

    [[nodiscard]] static Array<std::pair<Key, Value>>
    to_pairs(const Array<Record> & records)
    {
      Array<std::pair<Key, Value>> out;
      out.reserve(records.size());
      for (const auto & rec : records)
        out.append(to_pair(rec));
      return out;
    }

    [[nodiscard]] bool equals_key(const Key & lhs, const Key & rhs) const
    {
      const auto & cmp = tree_.key_comp().cmp;
      return not cmp(lhs, rhs) and not cmp(rhs, lhs);
    }

    [[nodiscard]] std::optional<Record> find_record(const Key & key) const
    {
      const auto probe = make_probe(key);
      auto candidate = tree_.lower_bound(probe);
      if (candidate.has_value() and equals_key(candidate->get_key(), key))
        return candidate;
      return std::nullopt;
    }

    [[nodiscard]] static const char * validated_path(const char * p)
    {
      ah_invalid_argument_if(p == nullptr) << "Gen_File_B_Map: file_path must not be null";
      return p;
    }

  public:
    using key_type = Key;
    using mapped_type = Value;
    using value_type = std::pair<Key, Value>;
    using tree_type = tree_impl_type;
    using open_mode_type = typename tree_type::open_mode_type;

    static constexpr auto Read_Write = tree_type::Read_Write;
    static constexpr auto Read_Only = tree_type::Read_Only;


    /** @brief Open or create a persistent B-Tree map for read-write access.
     *  @param file_path Path to the backing file.
     *  @param auto_sync If `true`, successful mutations synchronize the file.
     *  @param cmp Comparison functor used to order keys.
     *  @throws std::runtime_error If the backing file cannot be used.
     *  @throws std::bad_alloc If the page cache cannot be allocated.
     *  @note O(page_count) I/O to read the file header and root page.
     */
    explicit Gen_File_B_Map(std::string file_path,
                            const bool auto_sync = true,
                            const Compare & cmp = Compare())
      : tree_(std::move(file_path), auto_sync, Record_Compare{cmp}) {}

    /** @brief Open or create a persistent B-Tree map with an explicit mode.
     *  @param file_path Path to the backing file.
     *  @param open_mode `Read_Write` or `Read_Only`.
     *  @param auto_sync If `true`, successful mutations synchronize the file.
     *  @param cmp Comparison functor used to order keys.
     *  @throws std::runtime_error If the backing file cannot be used.
     *  @throws std::bad_alloc If the page cache cannot be allocated.
     *  @note O(page_count) I/O to read the file header and root page.
     */
    explicit Gen_File_B_Map(std::string file_path,
                            const open_mode_type open_mode,
                            const bool auto_sync = true,
                            const Compare & cmp = Compare())
      : tree_(std::move(file_path), open_mode, auto_sync, Record_Compare{cmp}) {}

    /** @brief Open or create a persistent B-Tree map for read-write access.
     *  @param file_path Path to the backing file.
     *  @param auto_sync If `true`, successful mutations synchronize the file.
     *  @param cmp Comparison functor used to order keys.
     *  @throws std::runtime_error If the backing file cannot be used.
     *  @throws std::bad_alloc If the page cache cannot be allocated.
     *  @note O(page_count) I/O to read the file header and root page.
     */
    explicit Gen_File_B_Map(const char * file_path,
                            const bool auto_sync = true,
                            const Compare & cmp = Compare())
      : tree_(validated_path(file_path), auto_sync, Record_Compare{cmp}) {}

    /** @brief Open or create a persistent B-Tree map with an explicit mode.
     *  @param file_path Path to the backing file.
     *  @param open_mode `Read_Write` or `Read_Only`.
     *  @param auto_sync If `true`, successful mutations synchronize the file.
     *  @param cmp Comparison functor used to order keys.
     *  @throws std::runtime_error If the backing file cannot be used.
     *  @throws std::bad_alloc If the page cache cannot be allocated.
     *  @note O(page_count) I/O to read the file header and root page.
     */
    explicit Gen_File_B_Map(const char * file_path,
                            const open_mode_type open_mode,
                            const bool auto_sync = true,
                            const Compare & cmp = Compare())
      : tree_(validated_path(file_path), open_mode, auto_sync, Record_Compare{cmp}) {}

    /** @brief Copying is disabled. */
    Gen_File_B_Map(const Gen_File_B_Map &) = delete;

    /** @brief Assignment is disabled. */
    Gen_File_B_Map & operator = (const Gen_File_B_Map &) = delete;

    /** @brief Moving is disabled. */
    Gen_File_B_Map(Gen_File_B_Map &&) = delete;

    /** @brief Move assignment is disabled. */
    Gen_File_B_Map & operator = (Gen_File_B_Map &&) = delete;

    /** @brief Return the file path.
     *  @return Constant reference to the backing file path.
     *  @throws Nothing.
     *  @note O(1).
     */
    [[nodiscard]] const std::string & file_path() const noexcept
    {
      return tree_.file_path();
    }

    /** @brief Return the open mode.
     *  @return `Read_Write` or `Read_Only`.
     *  @throws Nothing.
     *  @note O(1).
     */
    [[nodiscard]] open_mode_type open_mode() const noexcept
    {
      return tree_.open_mode();
    }

    /** @brief Return whether the map handle is read-only.
     *  @return `true` if opened in `Read_Only` mode.
     *  @throws Nothing.
     *  @note O(1).
     */
    [[nodiscard]] bool is_read_only() const noexcept
    {
      return tree_.is_read_only();
    }

    /** @brief Return whether automatic synchronization is enabled.
     *  @return `true` if successful mutations call `sync()`.
     *  @throws Nothing.
     *  @note O(1).
     */
    [[nodiscard]] bool auto_sync_enabled() const noexcept
    {
      return tree_.auto_sync_enabled();
    }

    /** @brief Enable or disable automatic synchronization.
     *  @param enabled New auto-sync mode.
     *  @throws Nothing.
     *  @note O(1).  Requires external synchronization.
     */
    void set_auto_sync(const bool enabled) noexcept
    {
      tree_.set_auto_sync(enabled);
    }

    /** @brief Return the key comparator.
     *  @return Constant reference to the key comparator.
     *  @throws Nothing.
     *  @note O(1).
     */
    [[nodiscard]] const Compare & key_comp() const noexcept
    {
      return tree_.key_comp().cmp;
    }

    /** @brief Return whether the map is empty.
     *  @return `true` if empty, otherwise `false`.
     *  @throws Nothing.
     *  @note O(1).
     */
    [[nodiscard]] bool empty() const noexcept { return tree_.empty(); }

    /** @brief Return the number of stored key/value pairs.
     *  @return Number of unique keys stored in the map.
     *  @throws Nothing.
     *  @note O(1).
     */
    [[nodiscard]] size_t size() const noexcept { return tree_.size(); }

    /** @brief Return the current tree height.
     *  @return `0` for an empty file, otherwise the number of page levels.
     *  @throws Nothing.
     *  @note O(1).
     */
    [[nodiscard]] size_t height() const noexcept { return tree_.height(); }

    /** @brief Return the serialized page size of the backing tree.
     *  @return Number of bytes used by each page record.
     *  @throws Nothing.
     *  @note O(1).
     */
    [[nodiscard]] size_t page_size_bytes() const noexcept
    {
      return tree_.page_size_bytes();
    }

    /** @brief Return the number of allocated pages.
     *  @return Page count in the backing tree.
     *  @throws Nothing.
     *  @note O(1).
     */
    [[nodiscard]] size_t page_count() const noexcept
    {
      return tree_.page_count();
    }

    /** @brief Return the latest durable checkpoint sequence.
     *  @return Monotonic checkpoint sequence stored by the backing tree.
     *  @throws Nothing.
     *  @note O(1).
     */
    [[nodiscard]] std::uint64_t checkpoint_sequence() const noexcept
    {
      return tree_.checkpoint_sequence();
    }

    /** @brief Flush pending changes to the backing file.
     *  @throws std::runtime_error If the file cannot be synchronized.
     *  @note O(dirty pages) I/O.  Not atomic; requires external
     *  synchronization.  Durability is guaranteed only after this call returns.
     */
    void sync() const
    {
      tree_.sync();
    }

    /** @brief Synonym for sync().
     *  @throws std::runtime_error If the file cannot be synchronized.
     *  @note O(dirty pages) I/O.  See sync().
     */
    void checkpoint() const
    {
      tree_.checkpoint();
    }

    /** @brief Discard unsynchronized changes and reload the file.
     *  @throws std::runtime_error If the file cannot be reopened or validated.
     *  @note O(n) I/O where n is the number of stored records.
     */
    void reload()
    {
      tree_.reload();
    }

    /** @brief Remove every key/value pair from the map.
     *  @throws std::runtime_error If the backing tree rejects the mutation.
     *  @note O(n) I/O.  Requires external synchronization; call sync()
     *  afterwards for durability when auto_sync is disabled.
     */
    void clear()
    {
      tree_.clear();
    }

    /** @brief Return whether a key exists.
     *  @param key Key to search.
     *  @return `true` if the key is present.
     *  @throws Any exception propagated by `Compare` or `Key` copies.
     *  @note O(log n) with I/O per page traversed.
     */
    [[nodiscard]] bool contains(const Key & key) const
    {
      return find_record(key).has_value();
    }

    /** @brief Synonym for contains().
     *  @param key Key to search.
     *  @return `true` if the key is present.
     *  @throws Any exception propagated by `Compare` or `Key` copies.
     *  @note O(log n) with I/O per page traversed.
     */
    [[nodiscard]] bool search(const Key & key) const
    {
      return contains(key);
    }

    /** @brief Return the mapped value associated with a key, if any.
     *  @param key Key to search.
     *  @return Stored mapped value or `std::nullopt` if missing.
     *  @throws Any exception propagated by `Compare`, `Key`, or `Value`.
     *  @note O(log n) with I/O per page traversed.
     */
    [[nodiscard]] std::optional<Value> find(const Key & key) const
    {
      const auto record = find_record(key);
      if (not record.has_value())
        return std::nullopt;
      return record->get_value();
    }

    /** @brief Return the mapped value associated with a key.
     *  @param key Key to search.
     *  @return Copy of the stored mapped value.
     *  @throws std::domain_error If the key is absent.
     *  @throws Any exception propagated by `Compare`, `Key`, or `Value`.
     *  @note O(log n) with I/O per page traversed.
     */
    [[nodiscard]] Value at(const Key & key) const
    {
      const auto value = find(key);
      ah_domain_error_if(not value.has_value())
        << "File_B_Map::at(): key not found";
      return *value;
    }

    /** @brief Insert a new key/value pair if the key is not present.
     *  @param key Key to insert.
     *  @param value Mapped value to associate with the key.
     *  @return `true` if insertion happened, otherwise `false`.
     *  @throws std::runtime_error If the backing tree rejects the mutation.
     *  @throws std::bad_alloc If the page cache cannot grow.
     *  @note O(log n) I/O.  Durable after return when auto_sync is enabled;
     *  otherwise call sync() explicitly.
     */
    bool insert(const Key & key, const Value & value)
    {
      return tree_.insert(make_record(key, value));
    }

    /** @brief Insert or overwrite a key/value pair.
     *  @param key Key to insert or overwrite.
     *  @param value Mapped value to store.
     *  @return `true` if the key was newly inserted, `false` if it replaced an
     *          existing value.
     *  @throws std::runtime_error If the backing tree rejects the mutation.
     *  @throws std::bad_alloc If the page cache cannot grow.
     *
     *  @note **Crash-recovery note.**  When the key already exists the
     *  implementation suppresses `auto_sync` for the `tree_.remove(probe)` +
     *  `tree_.insert(record)` pair so both operations are batched into a single
     *  WAL write; the pair is either both present or both absent after recovery.
     *  A crash before the final `sync()` call leaves the key with its old value
     *  (WAL replay re-applies the last complete transaction).  Callers that
     *  require strict all-or-nothing durability should call `sync()` explicitly
     *  after `insert_or_assign` returns.
     */
    bool insert_or_assign(const Key & key, const Value & value)
    {
      const auto probe = make_probe(key);
      const auto record = make_record(key, value);
      if (tree_.insert(record))
        return true;
      // Suppress intermediate syncs so the remove+insert pair is written as a
      // single WAL-flushed unit; restore and sync once at the end.
      const bool was_sync = auto_sync_enabled();
      set_auto_sync(false);
      try
        {
          // Retrieve the full record (key+value) before removing so we can
          // rollback to it if the subsequent insert throws.
          auto old_record = tree_.lower_bound(probe);
          ah_runtime_error_unless(old_record.has_value())
            << "File_B_Map::insert_or_assign(): existing record not found";
          ah_runtime_error_unless(tree_.remove(probe))
            << "File_B_Map::insert_or_assign(): internal replace failed";
          try
            {
              ah_runtime_error_unless(tree_.insert(record))
                << "File_B_Map::insert_or_assign(): internal reinsert failed";
            }
          catch (...)
            {
              auto ep = std::current_exception();
              try { tree_.insert(*old_record); } catch (...) { /* rollback failed; propagate original */ }
              std::rethrow_exception(ep);
            }
        }
      catch (...)
        {
          set_auto_sync(was_sync);
          throw;
        }
      set_auto_sync(was_sync);
      if (was_sync)
        sync();
      return false;
    }

    /** @brief Remove a key and its mapped value, if present.
     *  @param key Key to erase.
     *  @return `true` if the key existed and was removed.
     *  @throws std::runtime_error If the backing tree rejects the mutation.
     *  @note O(log n) I/O.  Durable after return when auto_sync is enabled;
     *  otherwise call sync() explicitly.
     */
    bool remove(const Key & key)
    {
      return tree_.remove(make_probe(key));
    }

    /** @brief Return the smallest stored key/value pair.
     *  @return Smallest pair or `std::nullopt` if empty.
     *  @throws Any exception propagated by `Key` or `Value`.
     *  @note O(log n) with I/O per page traversed.
     */
    [[nodiscard]] std::optional<value_type> min_item() const
    {
      const auto record = tree_.min_key();
      if (not record.has_value())
        return std::nullopt;
      return to_pair(*record);
    }

    /** @brief Return the largest stored key/value pair.
     *  @return Largest pair or `std::nullopt` if empty.
     *  @throws Any exception propagated by `Key` or `Value`.
     *  @note O(log n) with I/O per page traversed.
     */
    [[nodiscard]] std::optional<value_type> max_item() const
    {
      const auto record = tree_.max_key();
      if (not record.has_value())
        return std::nullopt;
      return to_pair(*record);
    }

    /** @brief Return the first key/value pair whose key is not less than `key`.
     *  @param key Probe key.
     *  @return Matching lower bound or `std::nullopt` if none exists.
     *  @throws Any exception propagated by `Compare`, `Key`, or `Value`.
     *  @note O(log n) with I/O per page traversed.
     */
    [[nodiscard]] std::optional<value_type> lower_bound(const Key & key) const
    {
      const auto record = tree_.lower_bound(make_probe(key));
      if (not record.has_value())
        return std::nullopt;
      return to_pair(*record);
    }

    /** @brief Return the first key/value pair whose key is greater than `key`.
     *  @param key Probe key.
     *  @return Matching upper bound or `std::nullopt` if none exists.
     *  @throws Any exception propagated by `Compare`, `Key`, or `Value`.
     *  @note O(log n) with I/O per page traversed.
     */
    [[nodiscard]] std::optional<value_type> upper_bound(const Key & key) const
    {
      const auto record = tree_.upper_bound(make_probe(key));
      if (not record.has_value())
        return std::nullopt;
      return to_pair(*record);
    }

    /** @brief Materialize all keys in sorted order.
     *  @return Ordered array containing every stored key.
     *  @throws std::bad_alloc If the result array cannot grow.
     *  @note O(n) time and space.
     */
    [[nodiscard]] Array<Key> keys() const
    {
      Array<Key> out;
      const auto records = tree_.keys();
      out.reserve(records.size());
      for (const auto & rec : records)
        out.append(rec.get_key());
      return out;
    }

    /** @brief Materialize all mapped values in key order.
     *  @return Ordered array containing every stored mapped value.
     *  @throws std::bad_alloc If the result array cannot grow.
     *  @note O(n) time and space.
     */
    [[nodiscard]] Array<Value> values() const
    {
      Array<Value> out;
      const auto records = tree_.keys();
      out.reserve(records.size());
      for (const auto & rec : records)
        out.append(rec.get_value());
      return out;
    }

    /** @brief Materialize all key/value pairs in sorted order.
     *  @return Ordered array containing every stored key/value pair.
     *  @throws std::bad_alloc If the result array cannot grow.
     *  @note O(n) time and space.
     */
    [[nodiscard]] Array<value_type> items() const
    {
      return to_pairs(tree_.keys());
    }


    /** @brief Verify the structural invariants of the backing tree.
     *  @return `true` if the underlying persistent tree is valid.
     *  @throws Any exception propagated by the underlying comparator.
     *  @note O(n) I/O — reads every page.
     */
    [[nodiscard]] bool verify() const
    {
      return tree_.verify();
    }
  };

  /** @brief Convenient alias for a persistent B-Tree map. */
  template <typename Key,
            typename Value,
            class Compare = Aleph::less<Key>,
            size_t MinDegree = 16,
            typename KeyCodec = detail::Paged_Value_Codec<Key>,
            typename ValueCodec = detail::Paged_Value_Codec<Value>>
  using File_B_Map = Gen_File_B_Map<Key, Value, Compare, MinDegree,
                                    KeyCodec, ValueCodec>;
} // namespace Aleph

# endif // TPL_FILE_B_MAP_H
