
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

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
# include <tpl_array.H>

using namespace std;

extern const size_t Ring_Max_Name_Size;

/** Ring cache stored in a file.

    @author Leandro Rabindranath Leon
 */
template <typename T>
class RingFileCache
{
public:

  // The cache parameters are stored in a binary file with this structure
  struct Pars 
  {
    size_t dim;  // capacity in entries of cache
    size_t n;    // Number of entries stored in cache
    size_t head; // next integral position ready for reading
    size_t tail; // next integral position ready for writing
    size_t size_cache_file = 0; // num of chars of string (including '\0')
    char cache_file_name[0]; // be careful with this field. DON'T USE IT!

    string to_string() const
    {
      ostringstream s;
      s <<dim << " " << n << " " << tail << " " << head << " "
       << size_cache_file;
      return s.str();
    }
    
    friend ostream & operator << (ostream & out, const Pars & pars)
    {
      return out << pars.to_string(); 
    }
  };

private:

  bool initialized = false;
  string pars_file_name; 
  string cache_file_name;

  fstream pars_stream;  // the stream for the pars (read/write mode)
  fstream cache_stream; // the stream for the cache (read/write mode)

  size_t dim  = 0; // the maximum number of entries
  size_t n    = 0; // the current number of stored entries 
  size_t head = 0; // next integral position ready for reading
  size_t tail = 0; // next integral position ready for writing

  void test_and_set_tail_pointer()
  {
    cache_stream.seekp(tail*sizeof(T));
  }

  bool is_valid_offset(const size_t offset) const noexcept
  {
    return offset < n;
  }

  bool is_valid_position(const size_t pos) const noexcept
  {
    return pos >= head and pos < tail;
  }

  void test_and_set_head_pointer(const size_t num_entries = 0)
  {
    const size_t pos = (head + num_entries) % dim;
    cache_stream.seekg(pos*sizeof(T));
  } 

  // load an entry from stream. WARNING: the stream pointer is not a priori set
  T read_entry()
  { 
    T ret;
    cache_stream.read((char*) &ret, sizeof ret);      

    return ret;
  }

  // read the pos entry respect to the head (oldest entry)
  T read_entry(const size_t pos)
  { 
    if (pos >= n)
      {
        ostringstream s;
        s << "read_entry(" << pos << "): cache has " << n << " items";
        throw range_error(s.str());
      }
    
    const size_t real_pos = (head + pos) % dim;

    T ret;
    cache_stream.seekg(real_pos*sizeof(ret));
    cache_stream.read((char*) &ret, sizeof ret);      
    return ret;
  }

  void write_entry(const T & item)
  {
    cache_stream.write((char*) &item, sizeof item);
  } 

  void validate_absolute_position(const size_t pos) const
  {
    if (pos < dim)
      return;

    ostringstream s;
    s << "position " << pos << " is greater than dim " << dim;
    throw out_of_range(s.str());
  }

  T read_absolute(const size_t pos) 
  {
    validate_absolute_position(pos);
    T ret;
    cache_stream.seekg(pos*sizeof(T));
    cache_stream.read((char*) &ret, sizeof ret);      
    return ret;
  }

  void write_absolute(const size_t pos, const T & item)
  {
    validate_absolute_position(pos);
    cache_stream.seekp(pos*sizeof(T));
    cache_stream.write((char*) &item, sizeof item);
  }

public:
  
  string to_string() const
  {
    const size_t tg = const_cast<fstream&>(cache_stream).tellg()/sizeof(T); 
    const size_t tp = const_cast<fstream&>(cache_stream).tellp()/sizeof(T); 
    ostringstream s;
    s << "Cache pars" << endl
      << "capacity = " << capacity() << endl
      << "size     = " << size() << endl
      << "sizeof T = " << sizeof(T) << endl
      << "head pos = " << head_pos() << endl
      << "tail pos = " << tail_pos() << endl
      << "tellg/T  = " << tg << endl
      << "tellp/T  = " << tp << endl;
    return s.str();
  }

  friend ostream & operator << (ostream & out, const RingFileCache & cache)
  {
    return out << cache.to_string();
  } 

  /** Create a cache with parameters file stored in `pars_file_name file`,
      cache file in `cache_file_name` and with capacity for `num_entries`
      entries.

      This is the very first function to be called for creating and
      initializing a new cache. Basically, the function create two new
      files: 
      
          - `pars_file_name` where the cache parameters (its internal
	    state) is saved
	  - `cache_file_name`: where the data entries to be cached will
	    be stored. This file is created for storing `num_entries`.

      After cache creation these files are created. The internal state
      of cache is stored in the file `pars_file_name`, included the
      cache file.

      @param[in] pars_file_name name of parameters file
      @param[in] pars_file_name name of cache file where the entries are
      stored. 
      @param[in] num_entries number of entries to be stored in the cache
      @throw domain_error if any file cannot be open, read o written
      @throw bad_alloc if there is no enough memory
  */
  static void create(const string & pars_file_name,
                     const string & cache_file_name,
                     const size_t num_entries)
  {
    static_assert(std::is_default_constructible<T>::value,
		  "T type has not default constructor");
    
    ostringstream s;

    fstream pars_stream(pars_file_name, ios::binary | ios::out);
    if ((not pars_stream.is_open()) or (not pars_stream.good()))
      {
        s << "cannot open " << pars_file_name;
        throw std::domain_error(s.str());
      }

    fstream cache_stream(cache_file_name, ios::binary | ios::out);
    if ((not cache_stream.is_open()) or (not cache_stream.good()))
      {
        s << "cannot open " << cache_file_name;
        throw domain_error(s.str());
      }
    
    const size_t pars_size = sizeof(Pars) + cache_file_name.size() + 1;
    shared_ptr<Pars> pars_ptr((Pars*) malloc(pars_size), free);
    if (pars_ptr == nullptr)
      throw bad_alloc();

    pars_ptr->dim = num_entries;
    pars_ptr->n = pars_ptr->head = pars_ptr->tail = 0;
    pars_ptr->size_cache_file = cache_file_name.size() + 1;
    strcpy(pars_ptr->cache_file_name, cache_file_name.c_str());
    pars_stream.write((char*) pars_ptr.get(), pars_size);

    T init;
    memset(&init, 0, sizeof(T));
    for (size_t i = 0; i < num_entries; ++i)
      cache_stream.write((char*) &init, sizeof(init));
  }

private:

  static string state(fstream & ss)
  {
    ostringstream s;
    auto state = ss.rdstate(); 
    if (state & ios::goodbit)
      s << "    goodbit = true " <<  endl;
    else
      s << "    goodbit = false " <<  endl;
    if (state & ios::badbit)
      s << "    badbit  = true " <<  endl;
    else
      s << "    badbit  = false " <<  endl;
    if (state & ios::failbit)
      s << "    failbit = true " <<  endl;
    else
      s << "    failbit = false " <<  endl;
    if (state & ios::eofbit)
      s << "    eofbit  = true " <<  endl;
    else
      s << "    eofbit  = false " <<  endl;
    return s.str();
  }

  // Helper method for initializing the cache from the pars file
  void read_pars(const string & pars_file_name)
  {
    ostringstream s;
    pars_stream.open(pars_file_name, ios::binary | ios::out | ios::in);
    if ((not pars_stream.is_open()) or (not pars_stream.good()))
      {
        s << "cannot open " << pars_file_name;
        throw domain_error(s.str());
      }
        // set the cache internal state from the saved state in pars file
    Pars pars;
    pars_stream.read((char*) &pars, sizeof pars);
    dim  = pars.dim;
    n    = pars.n;
    head = pars.head;
    tail = pars.tail;
    
        // read the cache file name, which shound be saved at the end of
        // pars file
    const size_t name_size = min(Ring_Max_Name_Size, pars.size_cache_file);
    shared_ptr<char> name((char*) malloc(name_size), free);
    pars_stream.read(name.get(), name_size);
    cache_file_name = name.get(); 

    cache_stream.open(cache_file_name, ios::binary | ios::out | ios::in);
    if ((not cache_stream.is_open()) or (not cache_stream.good()))
      {
        s << "cannot open " << cache_file_name;
        throw domain_error(s.str());
      }

    pars_stream.seekp(0); 

    cache_stream.seekg(head*sizeof(T)); // set read position
    cache_stream.seekp(tail*sizeof(T)); // set write position
    initialized = true;

    assert(pars_stream.good());
    assert(cache_stream.good());
  }

public:

  /** Defines a pointer to a specific location in the cache.

      Cache entries could be read or written through a pointer object
  */
  class Pointer
  {
    friend class RingFileCache;

    RingFileCache * cache_ptr = nullptr;
    size_t pos = 0; // this an absolute position in the file
    size_t end_pos; // bound according to dimension

    void validate_position(const size_t pos) const
    {
      if (pos < cache_ptr->dim)
	return;
	
      ostringstream s;
      s << "position " << pos << " is outside of maximum " << cache_ptr->dim;
      throw std::out_of_range(s.str());
    }

    void increase_pos(const long n = 1) noexcept
    {
      if (n < 0)
	{
	  decrease_pos(-n);
	  return;
	}

      pos += n;
      if (pos > end_pos)
	pos = pos % cache_ptr->dim;
    }

    void decrease_pos(const long n = 1) noexcept
    {
      if (n < 0)
	{
	  increase_pos(-n);
	  return;
	}

      long val = pos - n;
      if (val < 0)
	{
	  size_t m = n % cache_ptr->dim; // real positional offset
	  if (pos < m)
	    pos = cache_ptr->dim - (m - pos);
	  else
	    pos -= m;
	}
      else
	pos = val;
    }

  public:

    Pointer() {} 

    /** Construct a pointer to current head (oldest item in the cache)

	@param[in] cache the cache
	@param[in] __pos position respect the current head of cache
     */
    Pointer(const RingFileCache & cache, const size_t __pos = 0)
      : cache_ptr(&const_cast<RingFileCache&>(cache)), 
	pos(cache.head_pos() + __pos), end_pos(cache.dim) {}

    Pointer operator ++ () noexcept // prefix increment
    {
      increase_pos(1);
      return *this;
    }

    Pointer operator ++ (int) noexcept // postfix increment
    {
      Pointer ret = *this;
      increase_pos(1);
      return ret;
    }

    Pointer operator -- () noexcept // prefix decrement
    {
      decrease_pos(1);
      return *this;
    }

    Pointer operator -- (int) noexcept // postfix decrement
    {
      Pointer ret = *this;
      decrease_pos(1);
      return ret;
    }

    Pointer & operator += (const long val) noexcept
    {
      increase_pos(val);
      return *this;
    }

    Pointer & operator -= (const long val) noexcept
    {
      decrease_pos(val);
      return *this;
    }

    Pointer operator + (const long val) const noexcept
    {
      Pointer ret = *this;
      ret += val;
      return ret;
    }

    Pointer operator - (const long val) const noexcept
    {
      Pointer ret = *this;
      ret -= val;
      return ret;
    }

    size_t get_pos_respect_to_head() const noexcept
    {
      auto head = cache_ptr->head;
      if (head < pos)
	return pos - head;
      else
	return cache_ptr->dim - head + pos;
    }

    size_t get_pos() const noexcept 
    {
      return get_pos_respect_to_head();
    }
  };

  T read(const Pointer & ptr)
  {
    if (ptr.cache_ptr != this)
      throw domain_error("RingFileCache::read(const Pointer&): invalid ptr");

    return read_absolute(ptr.pos);
  }

  void write(const Pointer & ptr, const T & item)
  {
    if (ptr.cache_ptr != this)
      throw domain_error("RingFileCache::write(const Pointer&): invalid ptr");

    return write_absolute(ptr.pos, item);
  }

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
  RingFileCache(const string & pars_fname) : pars_file_name(pars_fname)
  {
    read_pars(pars_file_name);
  }

  /** Default constructor.

      This constructor is given for avoiding dependences of files creation.

      @warning The constructed cache is in an invalid state

      @see init()
   */
  RingFileCache() {}

  /** Tests if pars and cache files have been created.

      @param[in] pars_fname name of parameters file
      @return true if pars_fname exists and its associated cache file
      exists too.
   */
  static bool test(const string & pars_fname) 
  {
    ifstream pars_stream(pars_fname, ios::binary);
    if (not pars_stream.good())
      return false;
    
    Pars pars;
    pars_stream.read((char*) &pars, sizeof pars);
    
        // read the cache file name, which should be saved at the end of
        // pars file
    const size_t name_size = min(Ring_Max_Name_Size, pars.size_cache_file);
    shared_ptr<char> name((char*) malloc(name_size), free);
    pars_stream.read(name.get(), name_size);
    const char * cache_file_name = name.get(); 

    ifstream cache_stream(cache_file_name, ios::binary);
    if (not cache_stream.good())
      return false;

    return true;
  }

  /** Initialize a cache constructed with the default contructor.

      @warning: behaviour is unpredictable if a valid cache has already
      been constructed. 
      
      @param[in] pars_fname name of cache parameters file
      @throw domain_error if a pars file is already opened
      @throw std::ios_base::failure if there is a failure in any file.
   */
  void init(const string & pars_fname)
  {
    if (pars_stream.is_open())
      throw std::domain_error("this cache has already an opened pars file");

    new (this) RingFileCache(pars_fname);
  }

  /** Insert an item into the cache.

      recall that the cache obeys to a FIFO discipline. So, the entry is
      inserted at the tail.

      @param[in] item to be inserted into the queue
      @return `true` if the 'item' was correctly inserted; `false`
      otherwise 

      @throw std::ios_base::failure if there is a failure in any file.
   */
  bool put(const T & item)
  {
    if (n == dim)     // cache full?
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

  /** Read the m most old entries and load them in a contiguos array.

      `read()` reads the `m` most old entries, those located on the
      queue head side and loads them in the contiguous array
      `entries`. Of course, `entries` must be enough large for
      containing the `m` entries.

      This method does not alter the internal state of the cache, no
      element is removed.

      The expected protocol for using is to read with this method and
      afterward, if everything was ok, to remove them by invoking
      `get()` method.

      @param[in] m the number of entries to be read from the queue
      `head` side
      @param[in] entries a array where the read entries will be put. 
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
      cache_stream.read((char*) entries, m*sizeof(T));
    else
      {
        cache_stream.read((char*) entries, n_avail_until_eof*sizeof(T));
        cache_stream.seekg(0);
        cache_stream.read((char*) &entries[n_avail_until_eof],
                          (m - n_avail_until_eof)*sizeof(T));
        assert(cache_stream.gcount() == (m - n_avail_until_eof)*sizeof(T));
      }

    assert(cache_stream.good());

    return true; 
  }

  /// Read the oldest entry in the set
  T read_first()
  {
    if (n == 0)
      throw underflow_error("read_first(): cache is empty");
    
    test_and_set_head_pointer();
    T ret_val;
    read(&ret_val);
    return ret_val;
  }

  /// Read the youngest entry in the set
  T read_last()
  {
    if (n == 0)
      throw underflow_error("read_last(): cache is empty");

    if (tail == 0)
      cache_stream.seekg((dim - 1)*sizeof(T));
    else
      cache_stream.seekg((tail - 1)*sizeof(T));

    return read_entry();
  }

  T youngest() { return read_last(); }

  T oldest() { return read_first(); }

  T oldest(size_t i) 
  {
    if (i > n)
      {
	ostringstream s;
	s << "youngest(" << i << ") but the cache has " << n << " entries";
	throw overflow_error(s.str());
      }
    
    T ret_val;
    read_entry(i);
    return ret_val;
  }

  /** read all the entries 

      @return an dynamic and contiguous array with all the entries of the cache 
   */
  Array<T> read_all()
  {
    Array<T> ret(n);
    ret.putn(n);
    assert(ret.capacity() >= n);
    read(&ret.base(), n);
    return ret;
  }

  /** read `m` entries from the `pos`-th older position.

      The returned array size indicates how many entries were
      effectively read.

   */
  Array<T> read_from(const size_t pos, const size_t m)
  {
    Array<T> ret;
    Iterator it(*this, pos);
    for (size_t i = 0; i < m and it.has_curr(); ++i, it.next_ne())
      ret.append(it.get_curr_ne());
    
    return ret;
  }

  /** read `m` entries from the pointer older position.

      The returned array size indicates how many entries were
      effectively read.

  */
  Array<T> read_from(const Pointer & ptr, const size_t m)
  {
    Pointer p = ptr;    
    Array<T> ret;
    for (size_t i = 0; i < m; ++i)
      ret.append(read(p++));
    
    return ret;
  }

  /** Extracts (deletes) from the cache the n most older inserted items.

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

  /// empties the cache; all the entries are deleted
  void empty() { get(n); }

  /** Flushes to the disk all the internal cache state.

      Use this method wisely, Many calls could be very time expensive,
      but with very few calls the persistence reward of this class is
      lost. 
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
    pars_stream.write((char*) &pars, sizeof(pars));
    pars_stream.flush();
    pars_stream.seekp(0);
    
    cache_stream.flush();
  }

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

      Currently only the size increase is implemented.

      @param[in] sz new capacity
      @throw domain_error if sz is lesser than the current capacity
      @throw std::ios_base::failure if there is a failure in any file
   */
  void resize(const size_t sz)
  {
    if (sz < dim)
      throw domain_error("RingFileCache::resize(): file truncation is not "
			 "implemented (yet?)");

    T init;
    cache_stream.seekp(dim*sizeof(T));
    for (size_t i = 0, n = sz - dim; i < n; ++i)
      write_entry(init);
    
    dim = sz;
    flush(); 
  } 

  // Only for reading and *NOT REENTRANT* ==> while using you MUST not
  // to execute any others operations on cache object
  //
  // Iterate from oldest until youngest
  class Iterator
  {
    RingFileCache<T> * cache_ptr = nullptr;
    T curr;
    size_t pos;
    size_t curr_pos;

    void set_curr_pointer() 
    {
      cache_ptr->cache_stream.seekg(curr_pos*sizeof(T));
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

    size_t get_pos() const noexcept { return pos; }

    bool has_curr() const { return pos < cache_ptr->n; }

     /** Instance an iterator on the cache `n` entries forward from the oldest
	 cache entry 

	 @param[in] cache the cache on which to iterate
	 @param[in] offset how many positions from the oldest cache entry
     */
    Iterator(const RingFileCache<T> & cache, const size_t offset = 0)
      : cache_ptr(&const_cast<RingFileCache<T>&>(cache)),
	pos(offset), curr_pos((cache_ptr->head + offset) % cache.dim)
    {
      if (cache_ptr->is_empty())
	return;

      if (not cache_ptr->is_valid_offset(pos))
	return;

      load_curr();
    }

    T get_curr_ne() const noexcept { return curr; }

    T get_curr() const
    {
      if (not has_curr())
	throw overflow_error("RingFileCache::Iterator::get_curr()");
      return get_curr_ne();
    }

    void next_ne() noexcept
    {
      increase_pos();
      if (has_curr())
	load_curr();
    }

    void next()
    {
      if (not has_curr())
	throw overflow_error("RingFileCache::Iterator::next()");
      next_ne();
    }    
  };

  auto get_it() { return Iterator(*this); }
};

# endif // RINGFILECACHE_H
