# ifndef TPL_MEMARRAY_H
# define TPL_MEMARRAY_H

# include <utility>
# include <cstdlib>
# include <cmath>
# include <stdexcept>

# include <ahUtils.H>
# include <ahDry.H>
# include <ahIterator.H>
# include <array_it.H>
# include <array_utils.H>

namespace Aleph
{

  /** Simple, scalable and fast dynamic array.

      MemArray implement a totally sequential dynamic array. That is,
      conditioned to memory availability, all the array is stored in a
      contiguous chunk of memory. So, the access is direct and with
      exactly the same cost of accessing a normal array.

      The array allows to insert and remove elements. These operation are
      conceived by the ends. The number of stored elements is called `n`.
      
      The allocation technique obeys to "buddy system"; that is, the
      exact dimension of array always is an exact two power. 

      When the array is full, a new chunk twice as long is
      allocated and the current entries are moved (by move semantic if
      available).

      When the number of entries descend to a `contract_threshold` the
      array is half long contracted.

      \note This class is not really conceived for managing a
      set. Instead, it is conceived as implementation support of single
      sequentially based set as stacks and queues.

      @ingroup Secuencias
   */
    template <typename T>
class MemArray
{
public:

  static constexpr size_t Min_Dim = 4;

protected:

  T *    ptr = nullptr;
  size_t dim = Min_Dim;
  size_t n   = 0;

public:
  
  mutable size_t contract_threshold;

public:

  /// Return the current base of array.
  T * get_ptr() const noexcept { return ptr; }

  /// Return the current dimension of array
  const size_t & get_dim() const noexcept { return dim; }

protected:

  /// Allocate memory for the current dimension
  void allocate()
  {
    assert(is_power_of_2(dim));
    ptr = new T [dim];
    contract_threshold = dim / 4; 
  }

  /** Test is array is full and if affrimative, then expand the array
      twice as long and copy the content by swapping.

      This method first allocates a chunck of 2*dim and then copies from
      `first` index the `n` contained entries to the new chunck.

      @param[in] first index where is found the first item of array
      @return `true` if the array was full and then a new twice as long
      was allocated
      @throw bad_alloc if there is no enough memory
   */
  bool expand(const size_t first = 0)
  {
    assert(ptr);
    assert(is_power_of_2(dim));
    if (n < dim)
      return false;

    const size_t newsz = dim << 1; // 2*dim
    const size_t mask = dim - 1;
    T * new_ptr = new T [newsz];
    for (size_t i = 0; i < dim; ++i)
      {
	assert(((i + first) & mask) == ((i + first) % dim));
	std::swap(ptr[(i + first) & mask], new_ptr[i]);
      }
    delete [] ptr;
    ptr = new_ptr;
    dim = newsz;
    contract_threshold = dim/4;

    return true;
  }

  /** Test if `n` is lesser than `contract_threshold` and eventually
      contract the array half long and copies its content.

      `contract(first)` first testes `n` with `contract_threshold`. If
      it is lesser, then a new array half as long is allocated and the
      `n` elements from `first` are copied.

      @param[in] first index of first element
      @return `true` if the array is reallocated
  */
  bool contract(const size_t first = 0)
  {
    assert(ptr);
    if (n > contract_threshold)
      return false;

    const size_t newsz = dim >> 1; // dim/2

    if (newsz <= Min_Dim)
      return false;

    T * new_ptr = new T [newsz];

    const size_t mask = dim - 1;
    for (int i = 0; i < newsz; ++i)
      {
	assert(((first + i) & mask) == ((first + i) % dim));
	std::swap(ptr[(first + i) & mask], new_ptr[i]);
      }

    for (int i = newsz; i < dim; ++i) // call destructors 
      ptr[i].~T();

    delete [] ptr;
    ptr = new_ptr;
    dim = newsz;
    contract_threshold = dim/4;

    return true;
  }

  // if dim == 2^k returns dim; else next two power 2^k > dim 
  void init_dim(size_t d) noexcept
  {
    if (d == 0)
      d = Min_Dim;

    size_t k = log2(d);
    dim = is_power_of_2(d) ? d : 1 << ++k;

    assert(dim >= d);
    assert(dim == 1 << k);
  }

public:

  using Item_Type = T; /// The type of element of array

      /// Return the capacity of array (its dimension)
  size_t capacity() const noexcept { return dim; }

      /// Return the number of elements
  size_t size() const noexcept { return n; }

  /// Return `true` is the array is empty
  bool is_empty() const noexcept { return n == 0; }

  /** Construct an array con capacicity equal or greater than `__dim`.

      @param[in] __dim proposed dimension of arraym which could be
      adjusted to the next two power greater than `__dim`
      @throw bad_alloc si no hay suficiente memoria.
  */
  MemArray(size_t __dim = Min_Dim) : ptr(nullptr), n(0)
  {
    static_assert(std::is_copy_constructible<T>::value, 
		  "No copy constructor for T");
    static_assert(std::is_move_constructible<T>::value,
		  "No move constructor for T");
    static_assert(std::is_copy_assignable<T>::value,
		  "No copy assign for T");
    static_assert(std::is_move_assignable<T>::value,
		  "No move assign for T");
    init_dim(__dim);
    allocate();
  }

  ~MemArray()
  {
    if (ptr != nullptr)
      {
	delete [] ptr;
	ptr = nullptr;
      }
  }

  /// Swap in constant time `this` with `a`
  void swap(MemArray & a) noexcept
  {
    std::swap(ptr, a.ptr);
    std::swap(dim, a.dim);
    std::swap(n, a.n);
    std::swap(contract_threshold, a.contract_threshold);
  }

  /// Construct a copy of `a`
  MemArray(const MemArray & a) 
    : ptr(nullptr), dim(a.dim), n(a.n)
  {
    allocate();
    for (int i = 0; i < dim; ++i)
      ptr[i] = a.ptr[i];
  }
  
  /// Construct a array moved of rvalue `a`
  MemArray(MemArray && a) noexcept
    : ptr(nullptr), dim(0), n(0), contract_threshold(0)
  {
    assert(a.ptr);
    swap(a);
  }

  /// Assign by copy `a` to `this`
  MemArray & operator = (const MemArray & a)
  {
    if (this == &a)
      return *this;

    assert(a.ptr);

    T * newptr = new T [a.dim];    // allocate new array
    for (int i = 0; i < a.n; ++i)  // copy items to new array
      newptr[i] = a.ptr[i];

    if (ptr != nullptr)
      delete [] ptr;
    ptr = newptr;
    dim = a.dim;
    n = a.n;

    return *this;
  }

  /// Assign by moving `a` to `this`
  MemArray & operator = (MemArray && a) noexcept
  {
    swap(a);
    return *this;
  }

  /// Empty the container. The array is not contracted
  void empty() { n = 0; }

  /// Empty the array and release all memory
  void empty_and_release()
  {
    n = 0;
    if (dim <= Min_Dim)
      return;

    assert(ptr);
    delete [] ptr;
    ptr = nullptr;
    dim = Min_Dim;
    allocate();
  }

  /// Put a copy of `item` at the end of sequence. The array expands if
  /// this is already full
  T & put(const T & item)
  {
    assert(ptr);
    expand();

    ptr[n] = item;
    T & ret = ptr[n++];
    return ret;
  }

  /// Move `item` at the end of sequence. The array expands if
  /// this is already full
  T & put(T && item)
  {
    assert(ptr);
    expand();

    ptr[n] = std::forward<T>(item);
    T & ret = ptr[n++];
    return ret;
  }

private:

  void open_gap(size_t pos = 0, size_t num_entries = 1)
  {
    putn(num_entries);
    Aleph::open_gap(ptr, n, pos, num_entries);
  }

  void close_gap(size_t pos, size_t num_entries = 1)
  {
    Aleph::close_gap(ptr, n, pos, num_entries);
    get(num_entries);
  }

public:

  /// Push a copy of `item` at the begining of sequence. The array expands if
  /// this is already full
  T & push(const T & item)
  {
    assert(ptr);
    open_gap();

    ptr[0] = item;
    T & ret = ptr[0];
    return ret;
  }

  /// Push a copy of `item` at the begining of sequence. The array expands if
  /// this is already full
  T & push(T && item)
  {
    assert(ptr);
    open_gap();

    ptr[0] = std::forward<T>(item);
    T & ret = ptr[0];
    return ret;
  }

  T & top() const
  {
    if (n == 0)
      throw std::underflow_error("top(): MemArray is empty");

    return ptr[0];
  }

  /// Remove first item. Gap is closed
  T remove_first()
  {
    if (n == 0)
      throw std::underflow_error("remove_first(): MemArray is empty");
    assert(ptr);
    T ret = move(ptr[0]);
    close_gap(0);
    return ret;
  }

  /// pop() the most recently pushed item
  T pop() { return remove_first(); }
  
  /// \overload put(const T & item)
  T & append(T & item)
  {
    assert(ptr);
    return put(item);
  }

  /// \overload put(T && item)
  T & append(T && item)
  {
    assert(ptr);
    return put(std::forward<T>(item));
  }

    /// \overload push(const T & item)
  T & insert(T & item)
  {
    assert(ptr);
    return push(item);
  }

  /// \overload push(T && item)
  T & insert(T && item)
  {
    assert(ptr);
    return push(std::forward<T>(item));
  }

  /** Put `n` cells into the array.

      `putn(more)` is functionally equivalent to perform `more` puts of
      `T()` in constant time. The idea is to reserve enough space for
      later use.

      @param[in] n number of cells to reserve
      @throw bad_alloc if there is no enough memory
   */
  void putn(const size_t more)
  {
    assert(ptr);
    size_t new_n = n + more;
    if (new_n <= dim)
      {
	n = new_n;
	return;
      }

    size_t new_dim = dim;
    while (new_dim < new_n)
      new_dim <<= 1; // dim = 2*dim

    T * new_ptr = new T[new_dim];
    for (size_t i = 0; i < n; ++i)
      std::swap(ptr[i], new_ptr[i]);
    
    delete [] ptr;
    ptr = new_ptr;
    dim = new_dim;
    n = new_n;
    contract_threshold = dim/4;
  }

  /** Reserves `cap` cells into the array.

      @param[in] cap new dimension
      @throw bad_alloc if there is no enough memory
   */
  void reserve(const size_t cap)
  {
    assert(ptr);
    if (cap < dim)
      return;

    size_t k = log2(cap);
    const size_t new_dim = is_power_of_2(cap) ? cap : 1 << ++k;

    T * new_ptr = new T[new_dim];
    for (size_t i = 0; i < n; ++i)
      std::swap(ptr[i], new_ptr[i]);
    
    delete [] ptr;
    ptr = new_ptr;
    dim = new_dim;
    contract_threshold = dim/4;
  }
  
  /// Remove `i` elements from the end. Return the value of the last
  /// removed element
  T get(size_t i = 1) 
  {
    assert(ptr);
    long idx = n - i;
    if (idx < 0)
      throw std::underflow_error("MemArray::get(): deleted more entries"
				 " than capacity");

    n = idx;
    T ret = std::move(ptr[n]);

    contract();

    return ret;
  }

  /// \overload get()
  T remove_last() { return get(); }

  /// Return a modifiable reference to the last element
  T & last() const
  {
    if (n == 0)
      throw std::underflow_error("MemArray::last(): empty array");
    return ptr[n - 1];
  }

  /// Return a modifiable reference to the first element
  T & first() const
  {
    if (n == 0)
      throw std::underflow_error("MemArray::first(): empty array");
    return ptr[0];
  }

  /// \overload first
  T & get_first() const { return first(); }

    /// \overload last
  T & get_last() const { return last(); }

  /// Reverse the order of items in array
  MemArray & reverse()
  {
    for (size_t i = 0, j = n - 1; i < j; ++i, --j)
      std::swap(ptr[i], ptr[j]);

    return *this;
  }

  /// Return a modifiable reference to the ith element. No bound check
  /// is performed 
  T & access(const size_t i) const noexcept
  {
    assert(ptr);
    return ptr[i];
  }

  /// Return a reference to the ith element. Throws
  /// `out_of_range` if `i` is out of range
  T & operator [] (const size_t i) const
  {
    assert(ptr);
    if (i >= n)
      throw std::out_of_range("access out of range");

    return ptr[i];
  }
  
  /// \overload access(const size_t i)
  T & operator () (const size_t i) const noexcept
  {
    assert(ptr);
    assert(i < dim);
    return ptr[i];
  }

  /** Traverse all the elements from index 0 to `n - 1` and execute
      `operation` on each on them.

      @param[in] operation to be performed on each element
      @return `true` if `operation` was executed on all elements;
      `false` otherwise.
   */
  template <class Operation>
  bool traverse(Operation & operation) 
  {
    assert(ptr);
    for (int i = 0; i < n; i++)
      if (not operation(ptr[i]))
	return false;

    return true;
  }

  /// \overload traverse(Operation & operation) 
  template <class Operation>
  bool traverse(Operation & operation) const
  {
    return const_cast<MemArray*>(this)->traverse(operation);
  }

  /// \overload traverse(Operation & operation) 
  template <class Operation>
  bool traverse(Operation && operation) const
  {
    return traverse<Operation>(operation);
  }

  /// \overload traverse(Operation & operation) 
  template <class Operation>
  bool traverse(Operation && operation)
  {
    return traverse<Operation>(operation);
  }

  bool is_valid() const noexcept { return ptr; }

  /** Simple iterator on elements of array

      @ingroup Secuencias
  */
  struct Iterator : public Array_Iterator<T>
  {
    using Base = Array_Iterator<T>;
    using Base::Base;

    /// Construct an iterator on array `a`
    Iterator(const MemArray<T> & a)
      : Array_Iterator<T>(a.ptr, a.dim, a.n)
    {
      assert(a.ptr != nullptr);
    }
  };
};


} // end namespace Aleph

# endif // TPL_MEMARRAY_H
