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

/** @file tpl_file_bplus_map.H
 *  @brief Persistent key/value map built on top of Aleph::File_BPlus_Tree.
 *
 *  This header provides \ref Aleph::File_BPlus_Map, a persistent ordered map
 *  optimized for range scans by reusing the page-managed
 *  \ref Aleph::File_BPlus_Tree storage engine.
 *
 *  @ingroup Trees
 */

# ifndef TPL_FILE_BPLUS_MAP_H
# define TPL_FILE_BPLUS_MAP_H

# include <array>
# include <bit>
# include <cstddef>
# include <cstring>
# include <optional>
# include <ostream>
# include <string>
# include <type_traits>
# include <utility>

# include <ah-concepts.H>
# include <ah-errors.H>
# include <tpl_array.H>
# include <tpl_file_bplus_tree.H>

namespace Aleph
{
  /** @brief Persistent ordered map backed by a paged File_BPlus_Tree.
   *
   *  `Gen_File_BPlus_Map` stores unique keys together with mapped values in a
   *  persistent B+ Tree file. It reuses the same format, locking, WAL,
   *  recovery, and read-only semantics as \ref Aleph::File_BPlus_Tree, while
   *  exposing range scans over key/value pairs.
   *
   *  @tparam Key Stored key type.
   *  @tparam Value Stored mapped-value type.
   *  @tparam Compare Strict weak ordering used to sort keys.
   *  @tparam MinDegree Minimum degree `t` of the underlying persistent B+ Tree.
   *  @tparam KeyCodec Fixed-size codec used to persist keys.
   *  @tparam ValueCodec Fixed-size codec used to persist mapped values.
   *
   *  @warning `Key` and `Value` must admit fixed-size portable codecs because
   *  map entries are serialized through the paged storage engine of
   *  \ref Aleph::File_BPlus_Tree. The default codecs cover fixed-size
   *  arithmetic types and `std::array`; bounded strings can be supported with
   *  \ref Aleph::detail::Paged_Bounded_String_Codec.
   */
  template <typename Key,
            typename Value,
            class Compare = Aleph::less<Key>,
            size_t MinDegree = 16,
            typename KeyCodec = detail::Paged_Value_Codec<Key>,
            typename ValueCodec = detail::Paged_Value_Codec<Value>>
    requires StrictWeakOrder<Compare, Key>
  class Gen_File_BPlus_Map
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
        detail::mix_codec_id(detail::mix_codec_id(0x52000000u,
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
      Gen_File_BPlus_Tree<Record, Record_Compare, MinDegree, Record_Codec>;

    static_assert(detail::Fixed_Paged_Value_Codec<key_codec, Key>,
                  "File_BPlus_Map requires a fixed-size key codec");
    static_assert(detail::Fixed_Paged_Value_Codec<value_codec, Value>,
                  "File_BPlus_Map requires a fixed-size mapped-value codec");
    static_assert(std::is_trivially_copyable_v<Record>,
                  "File_BPlus_Map requires trivially copyable records");

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
      out.reserve(records.size() == 0 ? 1 : records.size());
      for (size_t i = 0; i < records.size(); ++i)
        out.append(to_pair(records[i]));
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

  public:
    using key_type = Key;
    using mapped_type = Value;
    using value_type = std::pair<Key, Value>;
    using tree_type = tree_impl_type;
    using open_mode_type = typename tree_type::open_mode_type;

    static constexpr auto Read_Write = tree_type::Read_Write;
    static constexpr auto Read_Only = tree_type::Read_Only;

    /** @brief Lazy iterator over ordered key/value pairs.
     *
     *  The iterator wraps the backing tree iterator and converts records to
     *  pairs on demand. Any mutation of the owning map invalidates all
     *  iterators.
     */
    class Iterator
    {
      typename tree_type::Iterator it_;

    public:
      /** @brief Construct an exhausted iterator.
       *  @throws Nothing.
       */
      Iterator() noexcept = default;

      /** @brief Wrap an underlying tree iterator.
       *  @param it Tree iterator to adapt.
       *  @throws Nothing.
       */
      explicit Iterator(typename tree_type::Iterator it) noexcept
        : it_(std::move(it)) {}

      /** @brief Return whether the iterator still points to an item.
       *  @return `true` if there is a current key/value pair.
       *  @throws Nothing.
       */
      [[nodiscard]] bool has_curr() const noexcept
      {
        return it_.has_curr();
      }

      /** @brief Return the current key/value pair.
       *  @return Copy of the current item.
       *  @throws std::underflow_error If the iterator is exhausted.
       */
      [[nodiscard]] value_type get_curr() const
      {
        return to_pair(it_.get_curr());
      }

      /** @brief Return the current key.
       *  @return Copy of the current key.
       *  @throws std::underflow_error If the iterator is exhausted.
       */
      [[nodiscard]] Key get_key() const
      {
        return it_.get_curr().get_key();
      }

      /** @brief Return the current mapped value.
       *  @return Copy of the current mapped value.
       *  @throws std::underflow_error If the iterator is exhausted.
       */
      [[nodiscard]] Value get_value() const
      {
        return it_.get_curr().get_value();
      }

      /** @brief Advance to the next item.
       *  @throws std::underflow_error If the iterator is exhausted.
       */
      void next_ne()
      {
        it_.next_ne();
      }

      /** @brief Synonym for next_ne().
       *  @throws std::underflow_error If the iterator is exhausted.
       */
      void next()
      {
        next_ne();
      }
    };

    /** @brief Open or create a persistent B+ Tree map for read-write access.
     *  @param file_path Path to the backing file.
     *  @param auto_sync If `true`, successful mutations synchronize the file.
     *  @param cmp Comparison functor used to order keys.
     *  @throws std::runtime_error If the backing file cannot be used.
     *  @throws std::bad_alloc If the page cache cannot be allocated.
     */
    explicit Gen_File_BPlus_Map(std::string file_path,
                                const bool auto_sync = true,
                                const Compare & cmp = Compare())
      : tree_(std::move(file_path), auto_sync, Record_Compare{cmp}) {}

    /** @brief Open or create a persistent B+ Tree map with an explicit mode.
     *  @param file_path Path to the backing file.
     *  @param open_mode `Read_Write` or `Read_Only`.
     *  @param auto_sync If `true`, successful mutations synchronize the file.
     *  @param cmp Comparison functor used to order keys.
     *  @throws std::runtime_error If the backing file cannot be used.
     *  @throws std::bad_alloc If the page cache cannot be allocated.
     */
    explicit Gen_File_BPlus_Map(std::string file_path,
                                const open_mode_type open_mode,
                                const bool auto_sync = true,
                                const Compare & cmp = Compare())
      : tree_(std::move(file_path), open_mode, auto_sync, Record_Compare{cmp}) {}

    /** @brief Open or create a persistent B+ Tree map for read-write access.
     *  @param file_path Path to the backing file.
     *  @param auto_sync If `true`, successful mutations synchronize the file.
     *  @param cmp Comparison functor used to order keys.
     *  @throws std::runtime_error If the backing file cannot be used.
     *  @throws std::bad_alloc If the page cache cannot be allocated.
     */
    explicit Gen_File_BPlus_Map(const char * file_path,
                                const bool auto_sync = true,
                                const Compare & cmp = Compare())
      : tree_(file_path, auto_sync, Record_Compare{cmp}) {}

    /** @brief Open or create a persistent B+ Tree map with an explicit mode.
     *  @param file_path Path to the backing file.
     *  @param open_mode `Read_Write` or `Read_Only`.
     *  @param auto_sync If `true`, successful mutations synchronize the file.
     *  @param cmp Comparison functor used to order keys.
     *  @throws std::runtime_error If the backing file cannot be used.
     *  @throws std::bad_alloc If the page cache cannot be allocated.
     */
    explicit Gen_File_BPlus_Map(const char * file_path,
                                const open_mode_type open_mode,
                                const bool auto_sync = true,
                                const Compare & cmp = Compare())
      : tree_(file_path, open_mode, auto_sync, Record_Compare{cmp}) {}

    Gen_File_BPlus_Map(const Gen_File_BPlus_Map &) = delete;
    Gen_File_BPlus_Map & operator = (const Gen_File_BPlus_Map &) = delete;
    Gen_File_BPlus_Map(Gen_File_BPlus_Map &&) = delete;
    Gen_File_BPlus_Map & operator = (Gen_File_BPlus_Map &&) = delete;

    /** @brief Return the file path.
     *  @return Constant reference to the backing file path.
     *  @throws Nothing.
     */
    [[nodiscard]] const std::string & file_path() const noexcept
    {
      return tree_.file_path();
    }

    /** @brief Return the open mode.
     *  @return `Read_Write` or `Read_Only`.
     *  @throws Nothing.
     */
    [[nodiscard]] open_mode_type open_mode() const noexcept
    {
      return tree_.open_mode();
    }

    /** @brief Return whether the map handle is read-only.
     *  @return `true` if opened in `Read_Only` mode.
     *  @throws Nothing.
     */
    [[nodiscard]] bool is_read_only() const noexcept
    {
      return tree_.is_read_only();
    }

    /** @brief Return whether automatic synchronization is enabled.
     *  @return `true` if successful mutations call `sync()`.
     *  @throws Nothing.
     */
    [[nodiscard]] bool auto_sync_enabled() const noexcept
    {
      return tree_.auto_sync_enabled();
    }

    /** @brief Enable or disable automatic synchronization.
     *  @param enabled New auto-sync mode.
     *  @throws Nothing.
     */
    void set_auto_sync(const bool enabled) noexcept
    {
      tree_.set_auto_sync(enabled);
    }

    /** @brief Return the key comparator.
     *  @return Mutable reference to the key comparator.
     *  @throws Nothing.
     */
    Compare & key_comp() noexcept
    {
      return tree_.key_comp().cmp;
    }

    /** @brief Return the key comparator.
     *  @return Constant reference to the key comparator.
     *  @throws Nothing.
     */
    [[nodiscard]] const Compare & key_comp() const noexcept
    {
      return tree_.key_comp().cmp;
    }

    /** @brief Return whether the map is empty.
     *  @return `true` if empty, otherwise `false`.
     *  @throws Nothing.
     */
    [[nodiscard]] bool empty() const noexcept { return tree_.empty(); }

    /** @brief Return the number of stored key/value pairs.
     *  @return Number of unique keys stored in the map.
     *  @throws Nothing.
     */
    [[nodiscard]] size_t size() const noexcept { return tree_.size(); }

    /** @brief Return the current tree height.
     *  @return `0` for an empty file, otherwise the number of page levels.
     *  @throws Nothing.
     */
    [[nodiscard]] size_t height() const noexcept { return tree_.height(); }

    /** @brief Return the serialized page size of the backing tree.
     *  @return Number of bytes used by each page record.
     *  @throws Nothing.
     */
    [[nodiscard]] size_t page_size_bytes() const noexcept
    {
      return tree_.page_size_bytes();
    }

    /** @brief Return the number of allocated pages.
     *  @return Page count in the backing tree.
     *  @throws Nothing.
     */
    [[nodiscard]] size_t page_count() const noexcept
    {
      return tree_.page_count();
    }

    /** @brief Return the latest durable checkpoint sequence.
     *  @return Monotonic checkpoint sequence stored by the backing tree.
     *  @throws Nothing.
     */
    [[nodiscard]] std::uint64_t checkpoint_sequence() const noexcept
    {
      return tree_.checkpoint_sequence();
    }

    /** @brief Flush pending changes to the backing file.
     *  @throws std::runtime_error If the file cannot be synchronized.
     */
    void sync() const
    {
      tree_.sync();
    }

    /** @brief Synonym for sync().
     *  @throws std::runtime_error If the file cannot be synchronized.
     */
    void checkpoint() const
    {
      tree_.checkpoint();
    }

    /** @brief Discard unsynchronized changes and reload the file.
     *  @throws std::runtime_error If the file cannot be reopened or validated.
     */
    void reload()
    {
      tree_.reload();
    }

    /** @brief Remove every key/value pair from the map.
     *  @throws std::runtime_error If the backing tree rejects the mutation.
     */
    void clear()
    {
      tree_.clear();
    }

    /** @brief Return whether a key exists.
     *  @param key Key to search.
     *  @return `true` if the key is present.
     *  @throws Any exception propagated by `Compare` or `Key`.
     */
    [[nodiscard]] bool contains(const Key & key) const
    {
      return find_record(key).has_value();
    }

    /** @brief Synonym for contains().
     *  @param key Key to search.
     *  @return `true` if the key is present.
     *  @throws Any exception propagated by `Compare` or `Key`.
     */
    [[nodiscard]] bool search(const Key & key) const
    {
      return contains(key);
    }

    /** @brief Return the mapped value associated with a key, if any.
     *  @param key Key to search.
     *  @return Stored mapped value or `std::nullopt` if missing.
     *  @throws Any exception propagated by `Compare`, `Key`, or `Value`.
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
     */
    [[nodiscard]] Value at(const Key & key) const
    {
      const auto value = find(key);
      ah_domain_error_if(not value.has_value())
        << "File_BPlus_Map::at(): key not found";
      return *value;
    }

    /** @brief Insert a new key/value pair if the key is not present.
     *  @param key Key to insert.
     *  @param value Mapped value to associate with the key.
     *  @return `true` if insertion happened, otherwise `false`.
     *  @throws std::runtime_error If the backing tree rejects the mutation.
     *  @throws std::bad_alloc If the page cache cannot grow.
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
     *  @note **Crash-recovery limitation.**  When the key already exists the
     *  implementation issues two separate WAL-protected operations:
     *  `tree_.remove(probe)` followed by `tree_.insert(record)`.  Each
     *  individual operation is crash-safe (the WAL guarantees redo on the next
     *  open), but a process crash *between* the two leaves the key permanently
     *  deleted.  `File_BPlus_Tree` does not currently expose a single-WAL-
     *  transaction replace primitive, so full atomicity of the pair cannot be
     *  guaranteed without adding such a primitive.  Callers that require
     *  all-or-nothing replace semantics should wrap the two operations in their
     *  own application-level recovery logic.
     */
    bool insert_or_assign(const Key & key, const Value & value)
    {
      const auto probe = make_probe(key);
      const auto record = make_record(key, value);
      if (tree_.insert(record))
        return true;
      ah_runtime_error_unless(tree_.remove(probe))
        << "File_BPlus_Map::insert_or_assign(): internal replace failed";
      ah_runtime_error_unless(tree_.insert(record))
        << "File_BPlus_Map::insert_or_assign(): internal reinsert failed";
      return false;
    }

    /** @brief Remove a key and its mapped value, if present.
     *  @param key Key to erase.
     *  @return `true` if the key existed and was removed.
     *  @throws std::runtime_error If the backing tree rejects the mutation.
     */
    bool remove(const Key & key)
    {
      return tree_.remove(make_probe(key));
    }

    /** @brief Return the smallest stored key/value pair.
     *  @return Smallest pair or `std::nullopt` if empty.
     *  @throws Any exception propagated by `Key` or `Value`.
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
     */
    [[nodiscard]] std::optional<value_type> upper_bound(const Key & key) const
    {
      const auto record = tree_.upper_bound(make_probe(key));
      if (not record.has_value())
        return std::nullopt;
      return to_pair(*record);
    }

    /** @brief Collect all key/value pairs in the inclusive key range.
     *  @param first Lower key endpoint, inclusive.
     *  @param last Upper key endpoint, inclusive.
     *  @return Ordered array with every pair whose key lies in the range.
     *  @throws std::invalid_argument If the range is invalid.
     *  @throws std::bad_alloc If the result array cannot grow.
     */
    [[nodiscard]] Array<value_type> range(const Key & first, const Key & last) const
    {
      return to_pairs(tree_.range(make_probe(first), make_probe(last)));
    }

    /** @brief Materialize all keys in sorted order.
     *  @return Ordered array containing every stored key.
     *  @throws std::bad_alloc If the result array cannot grow.
     */
    [[nodiscard]] Array<Key> keys() const
    {
      Array<Key> out;
      const auto records = tree_.keys();
      out.reserve(records.size() == 0 ? 1 : records.size());
      for (size_t i = 0; i < records.size(); ++i)
        out.append(records[i].get_key());
      return out;
    }

    /** @brief Materialize all mapped values in key order.
     *  @return Ordered array containing every stored mapped value.
     *  @throws std::bad_alloc If the result array cannot grow.
     */
    [[nodiscard]] Array<Value> values() const
    {
      Array<Value> out;
      const auto records = tree_.keys();
      out.reserve(records.size() == 0 ? 1 : records.size());
      for (size_t i = 0; i < records.size(); ++i)
        out.append(records[i].get_value());
      return out;
    }

    /** @brief Materialize all key/value pairs in sorted order.
     *  @return Ordered array containing every stored key/value pair.
     *  @throws std::bad_alloc If the result array cannot grow.
     */
    [[nodiscard]] Array<value_type> items() const
    {
      return to_pairs(tree_.keys());
    }

    /** @brief Return a lazy iterator over all items in key order.
     *  @return Iterator positioned at the first item, or exhausted if empty.
     *  @throws Nothing.
     */
    [[nodiscard]] Iterator get_it() const noexcept
    {
      return Iterator(tree_.get_it());
    }

    /** @brief Return a lazy iterator over an inclusive key range.
     *  @param first Lower key endpoint, inclusive.
     *  @param last Upper key endpoint, inclusive.
     *  @return Iterator positioned at the first item in the range.
     *  @throws std::invalid_argument If the range is invalid.
     */
    [[nodiscard]] Iterator get_range_it(const Key & first, const Key & last) const
    {
      return Iterator(tree_.get_range_it(make_probe(first), make_probe(last)));
    }

    /** @brief Verify the structural invariants of the backing tree.
     *  @return `true` if the underlying persistent tree is valid.
     *  @throws Any exception propagated by the underlying comparator.
     */
    [[nodiscard]] bool verify() const
    {
      return tree_.verify();
    }
  };

  /** @brief Convenient alias for a persistent B+ Tree map. */
  template <typename Key,
            typename Value,
            class Compare = Aleph::less<Key>,
            size_t MinDegree = 16,
            typename KeyCodec = detail::Paged_Value_Codec<Key>,
            typename ValueCodec = detail::Paged_Value_Codec<Value>>
  using File_BPlus_Map = Gen_File_BPlus_Map<Key, Value, Compare, MinDegree,
                                            KeyCodec, ValueCodec>;
} // namespace Aleph

# endif // TPL_FILE_BPLUS_MAP_H
