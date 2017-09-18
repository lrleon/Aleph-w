# ifndef ARRAY_IT_H
# define ARRAY_IT_H

# include <cassert>
# include <tuple>
# include <htlist.H>

namespace Aleph {

  /** Wrapper que provee la interfaz de un iterador para un arreglo C++.

      @ingroups Secuencias
  */
    template <class T>
class Array_Iterator
{
  T * ptr = NULL;
  size_t dim = 0;
  size_t num_items = 0;
  long idx = 0;
  long first = 0;
  long last = 0;
  long pos = 0;

public:

  using Item_Type = T;

  T * get_base() noexcept { return ptr; }

  Array_Iterator() noexcept {}

  Array_Iterator(T * p, size_t sz, size_t n) noexcept
    : ptr(p), dim(sz), num_items(n), last(sz - 1)
  {
    assert(ptr);
    assert(num_items <= dim);
  }

  Array_Iterator(T * p, size_t sz, size_t n, long f, long l) noexcept
    : ptr(p), dim(sz), num_items(n), idx(f), first(f), last(l)
  {
    assert(ptr);
    assert(first >= 0 and first < dim and last >= 0 and last <= dim and
           num_items < dim and idx == first);
  }

  bool has_curr() const noexcept { return pos >= 0 and pos < num_items; }

  bool has_current() const noexcept { return has_curr(); }

  bool is_last() const noexcept { return pos == num_items - 1; }

  long get_pos() const noexcept { return pos; }

  T & get_curr() const
  {
    if (pos < 0)
      throw std::underflow_error("");

    if (pos >= num_items)
      throw std::overflow_error("");

    return ptr[idx]; 
  }

  T & get_current() const { return get_curr(); }

  void next() noexcept
  {
    if (++idx == dim)
      idx = 0;
    ++pos; 
  }

  void prev() noexcept
  {
    if (--idx < 0)
      idx = dim - 1;
    ++pos; 
  }

  void reset() noexcept
  { 
    idx = first;
    pos = 0; 
  }

  void reset_first() noexcept { reset(); }

  void reset_last() noexcept
  { 
    idx = last; 
    pos = num_items - 1;
  }

  void end() noexcept
  {
    reset_last();
    next();
  }
};

  template <typename T> inline
Array_Iterator<T> get_array_it(const T * array, size_t n)
{
  return Array_Iterator<T>(array, n, n);
}

template <typename T>
class Array_Container 
  : public GenericTraverse<Array_Container<T>>,
    public LocateFunctions<Array_Container<T>, T>,
    public FunctionalMethods<Array_Container<T>, T>,
    public GenericKeys<Array_Container<T>, T>,
    public EqualToMethod<Array_Container<T>>,
    public StlAlephIterator<Array_Container<T>>
{
  T * base = nullptr;
  const size_t n = 0;

public:

  Array_Container(T * base_ptr, const size_t d) : base(base_ptr), n(d) 
  {
    // empty
  }

  Array_Container(const T * base_ptr, const size_t d) : base(base_ptr), n(d) 
  {
    // empty
  }

  T & get_first() const noexcept { return base[0]; }

  T& get_last() const noexcept { return base[n - 1]; }

  struct Iterator : public Array_Iterator<T>
  {
    Iterator(const Array_Container & c) : Array_Iterator<T>(c.base, c.n, c.n) {}
  };
};


  template <typename T> inline
Array_Container<T> get_array_container(const T * array, size_t n)
{
  return Array_Iterator<T>(array, n);
}


} // end namespace Aleph





# endif // ARRAY_IT_H
