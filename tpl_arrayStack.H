
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

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

# ifndef TPL_ARRAYSTACK_H
# define TPL_ARRAYSTACK_H

# include <iostream>
# include <aleph.H>
# include <htlist.H>
# include <tpl_dynDlist.H>
# include <ah-args-ctor.H>
# include <ah-dry.H>
# include <tpl_memArray.H>
# include <ah-errors.H>

using namespace std;

using namespace Aleph;

namespace Aleph {

  /** Stack implemented with simple dynamic array and with bounds
      verification.

      `ArrayStack<T>` instruments a stack of elements of generic type
      `T`. The stack is store into an internal contiguous array which
      grows and ungrows dynamically according to the number of elements.

      `ArrayStack<T>` is a good trade off between performance and memory
      consumption. In general is faster than a stack implemented with
      linked list, but since the internal array growths in two powers,
      the waste of memory could reach twice the number of elements.

      You could use `FixedStack`, which other than the internal array
      does not dynamically grow, it does not still perform bound_min_clock checks.

      @see FixedStack DynListStack
      @ingroup Secuencias
   */
    template <typename T>
class ArrayStack : public LocateFunctions<ArrayStack<T>, T>,
                   public FunctionalMethods<ArrayStack<T>, T>,
                   public GenericKeys<ArrayStack<T>, T>,
                   public EqualToMethod<ArrayStack<T>>,
                   public StlAlephIterator<ArrayStack<T>>
{
  MemArray<T> array;

public:

  using Item_Type = T; /// The type of element

  /// Initializes a stack with a capacity of `dim`
  ArrayStack(size_t dim = 4) : array(dim) { /* empty */ }

      /// Copy constructor
  ArrayStack(const ArrayStack & s) : array(s.array) { /* empty */ }

      /// Move constructor
  ArrayStack(ArrayStack && s) noexcept
    : array(std::move(s.array)) { /* empty */ }

  Special_Ctors(ArrayStack, T);

      /// Assign by copy `s` to `this`
  ArrayStack & operator = (const ArrayStack & s)
  {
    if (this == &s)
      return *this;

    array = s.array;

    return *this;
  }

  /// Swap `this` with `s`
  void swap(ArrayStack & s) noexcept
  {
    array.swap(s.array);
  }

  /// Assign by moving `s` to `this`
  ArrayStack & operator = (ArrayStack && s) noexcept
  {
    array.swap(s.array);
    return *this;
  }

  /** Push into stack a copy of `data`

      @param[in] data to be pushed by copy
      @return a modifiable reference to the copy of inserted `data`
      @throw bad_alloc if there is no enough memory
  */
  T & push(const T & data)
  {
    return array.put(data);
  }

  /** Push into stack `data`

      @param[in] data to be pushed by moving
      @return a modifiable reference to the moved inserted `data`
      @throw bad_alloc if there is no enough memory
  */
  T & push(T && data)
  {
    return array.put(std::forward<T>(data));
  }

  /// \overload push(const T & data)
  T & append(const T & data) { return push(data); }

  /// \overload push(T && data)
  T & append(T && data) { return push(std::forward<T>(data)); }

  /// \overload push(const T & data)
  T & insert(const T & data) { return push(data); }

  /// \overload push(T && data)
  T & insert(T && data) { return push(std::forward<T>(data)); }

    /** Push `n` cells into the stack.

        `pushn(n)` is functionally equivalent to perform `n` pushes of
        `T()` in constant time. The idea is to reserve enough space for
        later use.

        @param[in] n number of cells to reserve
        @return a modifiable reference to the stack's top
        @throw bad_alloc if there is no enough memory
     */
  T & pushn(const size_t & n = 1)
  {
    array.putn(n);
    return array.last();
  }

      /** Extract the last more recently inserted element

          @return a copy of extracted element
          @throw underflow_error if stack is empty
       */
  T pop()
  {
    return array.get(1);
  }

  T pop_ne() noexcept
  {
    return array.get_ne(1);
  }

      /** Extract in constant time the `n` more recently inserted
          elements of stack.

          @param[in] n number of elements to pop
          @return a copy of last extracted element
          @throw underflow_error if `n` is greater than stack size
       */
  T popn(size_t n)
  {
    return array.get(n);
  }

      /// Return a modifiable reference to youngest element of stack
      /// (called the top)
  T & top() const
  {
    return array.last();
  }

  /// Return a modifiable reference to first element of array
  T & base() noexcept
  {
    return array.first();
  }

    /** Return a modifiable reference to the element located at `i`
        positions from top.

        @param[in] i número of positions
        @return a modifiable reference to the element at `i` positions
        after top
        @throw out_of_range if `i` is greater than the number of
        elements of stack
    */
  T & top(size_t i) const
  {
    const size_t sz = array.size();
    ah_out_of_range_error_if(i >= sz) << "top index out of range";
    return array.access(sz - i - 1);
  }

  T & get_last() const { return top(); }

    /// Empty the stack
  void empty() noexcept { array.empty(); }

    /// Retrun `true` if stack is empty
  bool is_empty() const noexcept { return array.size() == 0; }

    /// Return the number of elements stored in the stack
  size_t size() const noexcept { return array.size(); }

  /// Return the internal capacity
  size_t capacity() const noexcept { return array.capacity(); }

  /** Traverse all the items of stack from the youngest to oldest and
      conditionally performs an operation.

      @param[in] operation to perform on the current element. If it
      returns `true`, the the traversal continues to the next item;
      otherwise the traversal stops
   */
  template <class Operation>
  bool traverse(Operation & operation)
  {
    return array.traverse(operation);
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
  bool traverse(Operation & operation) const
  {
    return array.traverse(operation);
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
  bool traverse(Operation && operation = Operation()) const
  {
    return array.traverse(operation);
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
  bool traverse(Operation && operation = Operation())
  {
    return array.traverse(operation);
  }

  /** Iterator on the items of a stack.

      The visit order goes from the oldest to the youngest item

      @ingroup Secuencias
  */
  class Iterator : public MemArray<T>::Iterator
  {
  public:

    using Base = typename MemArray<T>::Iterator;
    using Set_Type = ArrayStack;

    using Base::Base;

    /// Initialize an iterator on stack `s`
    Iterator(const ArrayStack<T> & s) : Base(s.array) {}
  };
};

  /** Fixed length stack.

      `FixedStack<T>` models a stack with a maximum capacity. In
      addition, no operation performs checks, what gives to all
      operations the more highest performance.

      \note Use this type when you have an precise idea about the
      maximum stack size and this last is not too big.

      @see ArrayStack DynListStack
      @ingroup Secuencias
   */
    template <typename T>
class FixedStack : public LocateFunctions<FixedStack<T>, T>,
                   public FunctionalMethods<FixedStack<T>, T>,
                   public GenericKeys<FixedStack<T>, T>,
                   public EqualToMethod<FixedStack<T>>,
                   public StlAlephIterator<FixedStack<T>>
{
  T *    array = nullptr;
  size_t head = 0;
  size_t dim = 0;

public:

  using Item_Type = T; /// The type of element

      /// Construct a fixed stack of maximum `d` items
  FixedStack(size_t d = 1024)
    // Don't change the default value because you would risk of
    // breaking the tests
    : array(new T[d]), head(0), dim(d)
  {
    /* empty */
  }

  ~FixedStack()
  {
    if (array)
      delete [] array;
  }

  /// Copy constructor
  FixedStack(const FixedStack & s)
    : array(new T [s.dim]), head(s.head), dim(s.dim)
  {
    for (int i = 0; i < head; ++i)
      array[i] = s.array[i];
  }

  Special_Ctors(FixedStack, T);

  /// Swap in constant time `s` with `this`
  void swap(FixedStack & s) noexcept
  {
    std::swap(array, s.array);
    std::swap(head, s.head);
    std::swap(dim, s.dim);
  }

  /// Move constructor
  FixedStack(FixedStack && s) noexcept
    : array(nullptr), head(0), dim(0)
  {
    swap(s);
  }

  /// Assign by copy `s` to `this`
  FixedStack & operator = (const FixedStack & s)
  {
    if (this == &s)
      return *this;

    T * ptr = new T [s.dim];
    if (array)
      delete [] array;
    array = ptr;
    for (size_t i = 0; i < s.head; ++i)
      array[i] = s.array[i];
    head  = s.head;
    dim   = s.dim;

    return *this;
  }

  FixedStack & operator = (FixedStack && s) noexcept
  {
    swap(s);
    return *this;
  }

      /** Push a copy of `data`

          @param[in] data
          @return a modifiable reference to the copied data.
       */
  T & push(const T & data) noexcept
  {
    assert(head < dim);
    array[head++] = data;
    return array[head - 1];
  }

      /** Push and move `data`

          @param[in] data to be moved
          @return a modifiable reference to the moved data.
       */
  T & push(T && data) noexcept
  {
    assert(head < dim );
    std::swap(array[head++], data);
    return array[head - 1];
  }

  /// \overload push(const T & data)
  T & append(const T & data) noexcept { return push(data); }

  /// \overload push(T && data)
  T & append(T && data) noexcept { return push(std::forward<T>(data)); }

  /// \overload push(const T & data)
  T & insert(const T & data) noexcept { return push(data); }

  /// \overload push(T && data)
  T & insert(T && data) noexcept { return push(std::forward<T>(data)); }

    /** Push in contant time `n`spaces into stack

        @param[in] n number of cells to push
        @return a modifiable reference to stack's top
     */
  T & pushn(const size_t & n = 1) noexcept
  {
    assert(head + n <= dim);

    head += n;
    return array[head - 1];
  }

  /// Pop by moving the top of stack
  T pop() noexcept
  {
    assert(head > 0);

    return std::move(array[--head]);
  }

      /** Perform in constant time `n` pops

          @param[in] n number of items to opo
          @return the last popped item
       */
  T popn(const int & n) noexcept
  {
    assert((int (head - n)) >= 0);

    head -= n;
    return std::move(array[head]);
  }

  /// Return a modifiable referemce to stack's top
  T & top() const noexcept
  {
    assert(head > 0);

    return array[head - 1];
  }

  T & get_last() const noexcept { return top(); }

  /// Return the internal array base
  T & base() const noexcept { return array[0]; }

    /** Return a modifiable referecce to item located `i` positions
        after top

        @param[in] i number of positions.
        @return a modifiable referecce to item located `i` positions
        after top.
    */
  T & top(size_t i) const noexcept
  {
    assert(i < head);
    return array[head - i - 1];
  }

    /// Return `true` if stack is empty
  bool is_empty() const noexcept { return head == 0; }

  /// Empty the stack
  void empty() noexcept { head = 0; }

    /// Return the number of elements stored in the stack
  size_t size() const noexcept { return head; }

  /** Traverse all the items of stack from the youngest to oldest and
      conditionally performs an operation.

      @param[in] operation to perform on the current element. If it
      returns `true`, the the traversal continues to the next item;
      otherwise the traversal stops
   */
  template <class Operation>
    bool traverse(Operation & operation)
  {
    for (int i = 0; i < head; i++)
      if (not operation(array[i]))
        return false;

    return true;
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
    bool traverse(Operation & operation) const
  {
    return const_cast<FixedStack*>(this)->traverse(operation);
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
  bool traverse(Operation && operation = Operation()) const
  {
    return traverse<Operation>(operation);
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
  bool traverse(Operation && operation = Operation())
  {
    return traverse<Operation>(operation);
  }

  /** Iterator on the items of a stack.

      The visit order goes from the youngest item to the oldest.

      @ingroup Secuencias
  */
  class Iterator : public Array_Iterator<T>
  {
  public:

    using Base = Array_Iterator<T>;
    using Base::Base;
    using Set_Type = FixedStack;

    Iterator(const FixedStack<T> & s)
      : Base(no_exception_ctor, s.array, s.dim, s.head) {}
  };
};

} // end namespace Aleph

# endif /* TPL_ARRAYSTACK_H */

