
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/


/** @file ringfilecache.H
 *  @brief File-backed ring buffer cache.
 *
 *  Circular buffer cache stored in a file for persistent
 *  storage with fixed memory footprint.
 *
 *  ## Features
 *  - Persistent storage
 *  - Ring buffer semantics
 *  - Fixed memory usage
 *
 *  @ingroup Hash
 *  @author Leandro Rabindranath León
 */

# ifndef RINGFILECACHE_H
# define RINGFILECACHE_H

# include <sys/time.h>
# include <stdio.h>
# include <cassert>
# include <string.h>
# include <stdexcept>
# include <memory>
# include <fstream>
# include <iostream>
# include <string>
# include <sstream>
# include <type_traits>

# include <ah-errors.H>
# include <tpl_array.H>

// Maximum expected length of the cache filename stored in the pars file
inline constexpr size_t Ring_Max_Name_Size = 4096;

/** Ring cache stored in a file.

    The cache stores `T` objects by writing their binary representation
    to disk; `T` should therefore be trivially copyable and default
    constructible.

    @author Leandro Rabindranath Leon
*/
template <typename T>
class RingFileCache
{
public:
  static_assert(std::is_trivially_copyable<T>::value,
                "RingFileCache requires trivially copyable types");
  static_assert(std::is_default_constructible<T>::value,
                "T must be default constructible");

  // The cache parameters are stored in a binary file with this structure
  struct Pars
  {
    size_t dim; // capacity in entries of cache
    size_t n; // Number of entries stored in cache
    size_t head; // next integral position ready for reading
    size_t tail; // next integral position ready for writing
    size_t size_cache_file = 0; // num of chars of std::string (including '\0')
    char cache_file_name[0]; // be careful with this field. DON'T USE IT!

    std::string to_string() const
    {
      std::ostringstream s;
      s << dim << " " << n << " " << tail << " " << head << " "
          << size_cache_file;
      return s.str();
    }

    friend std::ostream &operator <<(std::ostream & out, const Pars & pars)
    {
      return out << pars.to_string();
    }
  };

private:
  bool initialized = false;
  std::string pars_file_name;
  std::string cache_file_name;

  std::fstream pars_stream; // the stream for the pars (read/write mode)
  std::fstream cache_stream; // the stream for the cache (read/write mode)

  size_t dim = 0; // the maximum number of entries
  size_t n = 0; // the current number of stored entries
  size_t head = 0; // next integral position ready for reading
  size_t tail = 0; // next integral position ready for writing

  /// Position the write pointer at the current tail
  void test_and_set_tail_pointer()
  {
    cache_stream.seekp(tail * sizeof(T));
  }

  /// True if `offset` is a valid logical offset from the head (0 <= offset < n)
  bool is_valid_offset(const size_t offset) const noexcept
  {
    return offset < n;
  }

  /// True if `pos` is a valid absolute position within the stored entries
  bool is_valid_position(const size_t pos) const noexcept
  {
    if (head <= tail)
      return pos >= head and pos < tail;
    // Wraparound case: valid if pos >= head OR pos < tail
    return pos >= head or pos < tail;
  }

  /// Position the read pointer at head + num_entries (with wraparound)
  void test_and_set_head_pointer(const size_t num_entries = 0)
  {
    const size_t pos = (head + num_entries) % dim;
    cache_stream.seekg(pos * sizeof(T));
  }

  /// Load an entry from current stream position. WARNING: stream pointer not set here
  T read_entry()
  {
    T ret;
    cache_stream.read(reinterpret_cast<char *>(&ret), sizeof ret);

    return ret;
  }

  /// Read the entry at logical position `pos` from head (with bounds check)
  T read_entry(const size_t pos)
  {
    ah_range_error_if(pos >= n)
      << "read_entry(" << pos << "): cache has " << n << " items";

    const size_t real_pos = (head + pos) % dim;

    T ret;
    cache_stream.seekg(real_pos * sizeof(ret));
    cache_stream.read(reinterpret_cast<char *>(&ret), sizeof ret);
    return ret;
  }

  /// Write an entry at current stream position
  void write_entry(const T & item)
  {
    cache_stream.write(reinterpret_cast<const char *>(&item), sizeof item);
  }

  /// Throw out_of_range if pos >= dim
  void validate_absolute_position(const size_t pos) const
  {
    if (pos < dim)
      return;

    ah_out_of_range_error() << "position " << pos << " is greater than dim " << dim;
  }

  /// Read entry at absolute file position (with bounds check)
  T read_absolute(const size_t pos)
  {
    validate_absolute_position(pos);
    T ret;
    cache_stream.seekg(pos * sizeof(T));
    cache_stream.read(reinterpret_cast<char *>(&ret), sizeof ret);
    return ret;
  }

  /// Write entry at absolute file position (with bounds check)
  void write_absolute(const size_t pos, const T & item)
  {
    validate_absolute_position(pos);
    cache_stream.seekp(pos * sizeof(T));
    cache_stream.write(reinterpret_cast<const char *>(&item), sizeof item);
  }

public:
  /// Human-readable dump of the cache internal state (debugging)
  std::string to_string() const
  {
    const size_t tg = const_cast<std::fstream &>(cache_stream).tellg() / sizeof(T);
    const size_t tp = const_cast<std::fstream &>(cache_stream).tellp() / sizeof(T);
    std::ostringstream s;
    s << "Cache pars" << std::endl
        << "capacity = " << capacity() << std::endl
        << "size     = " << size() << std::endl
        << "sizeof T = " << sizeof(T) << std::endl
        << "head pos = " << head_pos() << std::endl
        << "tail pos = " << tail_pos() << std::endl
        << "tellg/T  = " << tg << std::endl
        << "tellp/T  = " << tp << std::endl;
    return s.str();
  }

  friend std::ostream &operator <<(std::ostream & out, const RingFileCache & cache)
  {
    return out << cache.to_string();
  }

  /** Create a brand new on-disk ring cache.

      This must be called once to create and initialize the parameter and
      data files before any `RingFileCache` instance can be constructed.
      Two files are created:

      - `pars_file_name`: stores the cache metadata (dimension, head/tail,
        number of items) and the cache file name.
      - `cache_file_name`: stores the raw cached entries (preallocated to
        `num_entries` items).

      The internal state saved in `pars_file_name` includes the name of
      the cache file so that a subsequent constructor only needs the
      parameters file path.

      @param[in] pars_file_name Path of the parameters file to create.
      @param[in] cache_file_name Path of the data file to create.
      @param[in] num_entries Capacity (number of entries) of the cache.
      @throw std::domain_error if any file cannot be opened/written.
      @throw std::bad_alloc if memory allocation fails.
  */
  static void create(const std::string & pars_file_name,
                     const std::string & cache_file_name,
                     const size_t num_entries)
  {
    std::ostringstream s;

    std::fstream pars_stream(pars_file_name, std::ios::binary | std::ios::out);
    ah_domain_error_if(not pars_stream.is_open() or not pars_stream.good())
      << "cannot open " << pars_file_name;

    std::fstream cache_stream(cache_file_name, std::ios::binary | std::ios::out);
    ah_domain_error_if(not cache_stream.is_open() or not cache_stream.good())
      << "cannot open " << cache_file_name;

    const size_t pars_size = sizeof(Pars) + cache_file_name.size() + 1;
    std::shared_ptr<Pars> pars_ptr(static_cast<Pars *>(malloc(pars_size)), free);

    ah_bad_alloc_if(pars_ptr == nullptr);

    pars_ptr->dim = num_entries;
    pars_ptr->n = pars_ptr->head = pars_ptr->tail = 0;
    pars_ptr->size_cache_file = cache_file_name.size() + 1;
    strncpy(pars_ptr->cache_file_name, cache_file_name.c_str(), 
            pars_ptr->size_cache_file);
    pars_ptr->cache_file_name[pars_ptr->size_cache_file - 1] = '\0';
    pars_stream.write(reinterpret_cast<const char *>(pars_ptr.get()), pars_size);

    T init{};
    for (size_t i = 0; i < num_entries; ++i)
      cache_stream.write(reinterpret_cast<const char *>(&init), sizeof(init));
  }

private:
  static std::string state(std::fstream & ss)
  {
    std::ostringstream s;
    const auto state = ss.rdstate();
    if (state & std::ios::goodbit)
      s << "    goodbit = true " << std::endl;
    else
      s << "    goodbit = false " << std::endl;
    if (state & std::ios::badbit)
      s << "    badbit  = true " << std::endl;
    else
      s << "    badbit  = false " << std::endl;
    if (state & std::ios::failbit)
      s << "    failbit = true " << std::endl;
    else
      s << "    failbit = false " << std::endl;
    if (state & std::ios::eofbit)
      s << "    eofbit  = true " << std::endl;
    else
      s << "    eofbit  = false " << std::endl;
    return s.str();
  }

  // Helper method for initializing the cache from the pars file
  void read_pars(const std::string & pars_file_name)
  {
    pars_stream.open(pars_file_name, std::ios::binary | std::ios::out | std::ios::in);

    ah_domain_error_if((not pars_stream.is_open()) or (not pars_stream.good()))
      << "cannot open " << pars_file_name;

    // set the cache internal state from the saved state in pars file
    Pars pars;
    pars_stream.read(reinterpret_cast<char *>(&pars), sizeof pars);
    dim = pars.dim;
    n = pars.n;
    head = pars.head;
    tail = pars.tail;

    // read the cache file name, which should be saved at the end of
    // pars file
    const size_t name_size = std::min(Ring_Max_Name_Size, pars.size_cache_file);
    const std::shared_ptr<char> name(static_cast<char *>(malloc(name_size)), free);
    pars_stream.read(name.get(), name_size);
    cache_file_name = name.get();

    cache_stream.open(cache_file_name, std::ios::binary | std::ios::out | std::ios::in);
    ah_domain_error_if(not cache_stream.is_open() or not cache_stream.good())
      << "cannot open " << cache_file_name;

    pars_stream.seekp(0);

    cache_stream.seekg(head * sizeof(T)); // set read position
    cache_stream.seekp(tail * sizeof(T)); // set write position
    initialized = true;

    assert(pars_stream.good());
    assert(cache_stream.good());
  }

public:
  /** Defines a pointer to a specific location in the cache.

      Cache entries can be read or written through a pointer object that
      moves circularly over the storage.
  */
  class Pointer
  {
    friend class RingFileCache;

    RingFileCache *cache_ptr = nullptr;
    size_t pos = 0; // this an absolute position in the file

    void validate_position(const size_t pos) const
    {
      if (pos < cache_ptr->dim)
        return;

      std::ostringstream s;
      s << "position " << pos << " is outside of maximum " << cache_ptr->dim;
      ah_out_of_range_error_if(pos >= cache_ptr->dim) << s.str();
    }

    void increase_pos(const long delta = 1) noexcept
    {
      if (delta < 0)
        {
          decrease_pos(-delta);
          return;
        }

      const size_t dim = cache_ptr->dim;
      pos = (pos + static_cast<size_t>(delta)) % dim;
    }

    void decrease_pos(const long delta = 1) noexcept
    {
      if (delta < 0)
        {
          increase_pos(-delta);
          return;
        }

      const size_t dim = cache_ptr->dim;
      if (const size_t decr = static_cast<size_t>(delta) % dim; pos >= decr)
        pos -= decr;
      else
        pos = dim - (decr - pos);
    }

  public:
    Pointer() {}

    /** Construct a pointer to current head (oldest item in the cache)

        @param[in] cache the cache
        @param[in] __pos offset relative to the current head of cache
    */
    Pointer(const RingFileCache & cache, const size_t __pos = 0)
      : cache_ptr(&const_cast<RingFileCache &>(cache)),
        pos((cache.head_pos() + __pos) % cache.dim) {}

    /// Prefix increment: advance one position (with wraparound)
    Pointer operator ++() noexcept
    {
      increase_pos(1);
      return *this;
    }

    /// Postfix increment: advance one position (with wraparound)
    Pointer operator ++(int) noexcept
    {
      Pointer ret = *this;
      increase_pos(1);
      return ret;
    }

    /// Prefix decrement: move back one position (with wraparound)
    Pointer operator --() noexcept
    {
      decrease_pos(1);
      return *this;
    }

    /// Postfix decrement: move back one position (with wraparound)
    Pointer operator --(int) noexcept
    {
      Pointer ret = *this;
      decrease_pos(1);
      return ret;
    }

    /// Advance by `val` positions (with wraparound)
    Pointer &operator +=(const long val) noexcept
    {
      increase_pos(val);
      return *this;
    }

    /// Move back by `val` positions (with wraparound)
    Pointer &operator -=(const long val) noexcept
    {
      decrease_pos(val);
      return *this;
    }

    /// Return a new Pointer advanced by `val` positions
    Pointer operator +(const long val) const noexcept
    {
      Pointer ret = *this;
      ret += val;
      return ret;
    }

    /// Return a new Pointer moved back by `val` positions
    Pointer operator -(const long val) const noexcept
    {
      Pointer ret = *this;
      ret -= val;
      return ret;
    }

    /// Offset of this pointer relative to the current head of the cache
    size_t get_pos_respect_to_head() const noexcept
    {
      auto head = cache_ptr->head;
      if (pos >= head)
        return pos - head;
      return cache_ptr->dim - head + pos;
    }

    /// Alias of `get_pos_respect_to_head()`
    size_t get_pos() const noexcept
    {
      return get_pos_respect_to_head();
    }
  };

  /// Read the entry pointed to by `ptr`
  T read(const Pointer & ptr)
  {
    ah_domain_error_if(ptr.cache_ptr != this)
        << "RingFileCache::read(const Pointer&): invalid ptr";

    return read_absolute(ptr.pos);
  }

  /// Write `item` at an absolute position designated by `ptr`.
  void write(const Pointer & ptr, const T & item)
  {
    ah_domain_error_if(ptr.cache_ptr != this)
        << "RingFileCache::write(const Pointer&): invalid ptr";

    return write_absolute(ptr.pos, item);
  }

  /// True if the cache has been initialized with a valid parameters file.
  bool is_initialized() const { return initialized; }

  /// Returns the number of entries stored in the cache
  size_t size() const noexcept { return n; }

  /// Returns the maximum capacity
  size_t capacity() const noexcept { return dim; }

  /// Returns the number of available entries
  size_t avail() const noexcept { return dim - n; }

  /// Returns the current head position
  size_t head_pos() const noexcept { return head; }

  /// return the current tail position
  size_t tail_pos() const noexcept { return tail; }

  /// Returns true if the cache is empty
  bool is_empty() const noexcept { return n == 0; }

  /// Returns true if the cache is full (no more entries can be inserted)
  bool is_full() const noexcept { return n == dim; }

  /** Initialize a cache previously built.

      The internal state of cache is represented by a file specified
      with the parameters file called `pars_file_name` and passed during
      `create()` static method. The name of cache is also passed to
      `create()`, but this one is stored in the parameters file.

      So, in order to instantiate a cache, it is enough to pass the
      parameters file name.

      @param[in] pars_fname name of cache parameters file
      @throw domain_error if any file cannot be opened
      @throw std::ios_base::failure if there is a failure in any file.
  */
  RingFileCache(const std::string & pars_fname) : pars_file_name(pars_fname)
  {
    read_pars(pars_file_name);
  }

  /** Default constructor.

      Provided to allow deferred initialization when file creation must be
      handled elsewhere.

      @warning The constructed cache is in an invalid state

      @see init()
  */
  RingFileCache() {}

  /** Tests if pars and cache files have been created.

      @param[in] pars_fname name of parameters file
      @return true if pars_fname exists and its associated cache file
      exists too.
  */
  static bool test(const std::string & pars_fname)
  {
    std::ifstream pars_stream_test(pars_fname, std::ios::binary);
    if (not pars_stream_test.good())
      return false;

    Pars pars;
    pars_stream_test.read(reinterpret_cast<char *>(&pars), sizeof pars);

    // read the cache file name, which should be saved at the end of
    // pars file
    const size_t name_size = std::min(Ring_Max_Name_Size, pars.size_cache_file);
    std::shared_ptr<char> name(static_cast<char *>(malloc(name_size)), free);
    pars_stream_test.read(name.get(), name_size);
    const char *cache_file_name = name.get();

    if (std::ifstream cache_stream(cache_file_name, std::ios::binary); not cache_stream.good())
      return false;

    return true;
  }

  /** Initialize a cache constructed with the default constructor.

      @warning Behavior is undefined if a valid cache has already been
      constructed.

      @param[in] pars_fname name of cache parameters file
      @throw domain_error if a pars file is already opened
      @throw std::ios_base::failure if there is a failure in any file.
  */
  void init(const std::string & pars_fname)
  {
    ah_domain_error_if(pars_stream.is_open())
      << "this cache has already an opened pars file";

    pars_file_name = pars_fname;
    read_pars(pars_file_name);
  }

  /** Insert an item into the cache.

      The cache is FIFO: each new entry is appended at the tail.

      @param[in] item to be inserted into the queue
      @return `true` if the 'item' was correctly inserted; `false`
      otherwise

      @throw std::ios_base::failure if there is a failure in any file.
  */
  bool put(const T & item)
  {
    if (n == dim) // cache full?
      return false;

    //    assert(cache_stream.good());

    test_and_set_tail_pointer();

    assert(cache_stream.tellp() == tail*sizeof(T));

    write_entry(item);
    if (++tail == dim) // eof border reached?
      {
        tail = 0;
        cache_stream.seekp(0);
        assert(cache_stream.good());
      }
    ++n;

    assert(cache_stream.good());
    assert(cache_stream.tellp() == tail*sizeof(T));

    return true;
  }

  /** Read the `m` the oldest entries and load them in a contiguous array.

      `read()` reads the `m` the oldest entries located on the queue head
      side and loads them in the contiguous array `entries`. Of course,
      `entries` must be large enough to contain the `m` entries.

      This method does not alter the internal state of the cache, no
      element is removed.

      The typical usage pattern is to read with this method and,
      afterward (if the caller is satisfied), remove them with `get()`.

      @param[in] m the number of entries to be read from the queue
      `head` side
      @param[in] entries an array where the read entries will be put.
      @return true if the `m` entries were read; `false` otherwise, what
      only happens if `m` is greater than the current number of stored
      elements.
      @throw std::ios_base::failure if there is a failure in any file.
      @see get()
  */
  bool read(T entries[], const size_t m = 1)
  {
    if (m > n)
      return false;

    test_and_set_head_pointer();

    assert(cache_stream.good());
    // assert(cache_stream.tellg() == head*sizeof(T));

    const size_t n_avail_until_eof = dim - head;
    if (m <= n_avail_until_eof)
      cache_stream.read(reinterpret_cast<char *>(entries), m * sizeof(T));
    else
      {
        cache_stream.read(reinterpret_cast<char *>(entries),
                          n_avail_until_eof * sizeof(T));
        cache_stream.seekg(0);
        cache_stream.read(reinterpret_cast<char *>(&entries[n_avail_until_eof]),
                          (m - n_avail_until_eof) * sizeof(T));
        assert(cache_stream.gcount() == (m - n_avail_until_eof)*sizeof(T));
      }

    assert(cache_stream.good());

    return true;
  }

  /** Read the oldest entry in the cache.

      @return the oldest (first inserted) entry
      @throw underflow_error if the cache is empty
  */
  T read_first()
  {
    ah_underflow_error_if(n == 0) << "read_first(): cache is empty";

    test_and_set_head_pointer();
    T ret_val;
    read(&ret_val);
    return ret_val;
  }

  /** Read the youngest (most recently inserted) entry in the cache.

      @return the youngest (last inserted) entry
      @throw underflow_error if the cache is empty
  */
  T read_last()
  {
    ah_underflow_error_if(n == 0) << "read_last(): cache is empty";

    if (tail == 0)
      cache_stream.seekg((dim - 1) * sizeof(T));
    else
      cache_stream.seekg((tail - 1) * sizeof(T));

    return read_entry();
  }

  /** Alias for `read_last()`.

      @return the youngest (last inserted) entry
      @throw underflow_error if the cache is empty
  */
  T youngest() { return read_last(); }

  /** Alias for `read_first()`.

      @return the oldest (first inserted) entry
      @throw underflow_error if the cache is empty
  */
  T oldest() { return read_first(); }

  /** Read the `i`-th oldest element without removing it.

      @param[in] i zero-based index from the oldest entry
      @return the entry at position `i` from the head
      @throw overflow_error if `i >= size()`
  */
  T oldest(size_t i)
  {
    ah_overflow_error_if(i >= n)
      << "oldest(" << i << ") but the cache has " << n << " entries";

    return read_entry(i);
  }

  /** Read all entries in insertion order.

      @return a dynamic and contiguous array with all the entries of the cache.
              If the cache is empty the returned array is empty.
  */
  Array<T> read_all()
  {
    if (n == 0)
      return Array<T>();

    Array<T> ret(n);
    ret.putn(n);
    assert(ret.capacity() >= n);
    read(&ret.base(), n);
    return ret;
  }

  /** Read up to `m` entries starting from the `pos`-th oldest position.

      The returned array size indicates how many entries were effectively read.

  */
  Array<T> read_from(const size_t pos, const size_t m)
  {
    Array<T> ret;
    Iterator it(*this, pos);
    for (size_t i = 0; i < m and it.has_curr(); ++i, it.next_ne())
      ret.append(it.get_curr());

    return ret;
  }

  /** Read up to `m` entries starting from the position designated by `ptr`.

      The returned array size indicates how many entries were effectively read.

  */
  Array<T> read_from(const Pointer & ptr, const size_t m)
  {
    Array<T> ret;
    Iterator it(*this, ptr.get_pos_respect_to_head());
    for (size_t i = 0; i < m and it.has_curr(); ++i, it.next_ne())
      ret.append(it.get_curr_ne());

    return ret;
  }

  /** Extracts (deletes) from the cache the `m` oldest inserted items.

      @param[in] m number of items to be extracted
      @return true if the `m` items were extracted
  */
  bool get(const size_t m = 1) noexcept
  {
    if (m > n)
      return false;

    head = (head + m) % dim;
    n -= m;

    return true;
  }

  /** Empties the cache; all entries are logically deleted.

      The underlying file storage is not modified; only the head/tail
      pointers are reset.
  */
  void empty() noexcept { get(n); }

  /** Flushes the current cache state to disk.

      Use this method wisely: frequent calls can be expensive, but the
      persistence guarantees of this class rely on writing both the
      parameters and data streams.
  */
  void flush()
  {
    assert(pars_stream.tellp() == 0);
    Pars pars;
    pars.dim = dim;
    pars.n = n;
    pars.head = head;
    pars.tail = tail;
    pars.size_cache_file = cache_file_name.size() + 1;
    pars_stream.write(reinterpret_cast<const char *>(&pars), sizeof(pars));
    pars_stream.flush();
    pars_stream.seekp(0);

    cache_stream.flush();
  }

  /** Flushes state and closes the underlying streams.

      Safe to call multiple times; subsequent calls after the first are
      no-ops.
  */
  void close()
  {
    if (not initialized)
      return;
    flush();
    cache_stream.close();
    pars_stream.close();
    initialized = false;
  }

  ~RingFileCache() { close(); }

  /** Resize the maximum capacity of the cache.

      Currently only size increase is implemented. Newly added slots are
      value-initialized.

      @param[in] sz new capacity
      @throw domain_error if sz is lesser than the current capacity
      @throw std::ios_base::failure if there is a failure in any file
  */
  void resize(const size_t sz)
  {
    ah_domain_error_if(sz < dim)
      << "RingFileCache::resize(): file truncation is not implemented (yet?)";

    if (sz == dim)
      return;  // no-op

    const size_t old_dim = dim;

    // Handle wraparound case: data spans [head, old_dim) and [0, tail)
    // Move the wrapped portion [0, tail) to [old_dim, old_dim + tail)
    if (n > 0 && tail <= head && tail > 0)
      {
        const size_t wrapped_count = tail;
        auto wrapped_data = std::make_unique<T[]>(wrapped_count);

        cache_stream.seekg(0);
        cache_stream.read(reinterpret_cast<char *>(wrapped_data.get()),
                          wrapped_count * sizeof(T));

        cache_stream.seekp(old_dim * sizeof(T));
        cache_stream.write(reinterpret_cast<const char *>(wrapped_data.get()),
                           wrapped_count * sizeof(T));

        tail = old_dim + wrapped_count;
      }

    // Zero-initialize remaining slots from current tail (or old_dim) to sz
    T init{};
    const size_t fill_start = (tail > old_dim) ? tail : old_dim;
    for (size_t pos = fill_start; pos < sz; ++pos)
      {
        cache_stream.seekp(pos * sizeof(T));
        write_entry(init);
      }

    dim = sz;
    // tail is already correct after wraparound fix, or needs update for non-wrapped case
    if (n > 0 && tail <= old_dim)
      tail = (head + n) % dim;

    test_and_set_tail_pointer();
    flush();
  }

  // Iterator for read-only traversal (oldest to youngest).
  // Not reentrant: do not execute other operations on the cache object
  // while iterating.
  class Iterator
  {
    RingFileCache<T> *cache_ptr = nullptr;
    T curr;
    size_t pos;
    size_t curr_pos;

    void set_curr_pointer()
    {
      cache_ptr->cache_stream.seekg(curr_pos * sizeof(T));
    }

    void increase_pos()
    {
      ++pos;
      if (++curr_pos == cache_ptr->dim)
        curr_pos = 0;
    }

    void load_curr()
    {
      set_curr_pointer();
      curr = cache_ptr->read_entry();
    }

  public:
    /// Logical offset from the head (number of elements already visited)
    size_t get_pos() const noexcept { return pos; }

    /// True if the iterator currently refers to a valid element
    bool has_curr() const { return pos < cache_ptr->n; }

    /** Construct an iterator positioned `offset` entries forward from the
        oldest cache entry

        @param[in] cache the cache on which to iterate
        @param[in] offset how many positions from the oldest cache entry
    */
    Iterator(const RingFileCache<T> & cache, const size_t offset = 0)
      : cache_ptr(&const_cast<RingFileCache<T> &>(cache)),
        pos(offset), curr_pos((cache_ptr->head + offset) % cache.dim)
    {
      if (cache_ptr->is_empty())
        return;

      if (not cache_ptr->is_valid_offset(pos))
        return;

      load_curr();
    }

    /// Return the current element (no bounds check). Undefined if exhausted.
    T get_curr_ne() const noexcept { return curr; }

    /** Return the current element.

        @return the element at the current iterator position
        @throw overflow_error if the iterator is exhausted
    */
    T get_curr() const
    {
      ah_overflow_error_if(not has_curr())
        << "RingFileCache::Iterator::get_curr(): iterator exhausted";
      return get_curr_ne();
    }

    /// Advance to the next element without checking bounds. Safe to call when exhausted.
    void next_ne() noexcept
    {
      increase_pos();
      if (has_curr())
        load_curr();
    }

    /** Advance to the next element.

        @throw overflow_error if the iterator is already exhausted
    */
    void next()
    {
      ah_overflow_error_if(not has_curr())
        << "RingFileCache::Iterator::next(): iterator exhausted";
      next_ne();
    }
  };

  /** Returns a forward iterator starting at the oldest entry.

      @return an Iterator positioned at the head of the cache
  */
  Iterator get_it() { return Iterator(*this); }
};

# endif // RINGFILECACHE_H
