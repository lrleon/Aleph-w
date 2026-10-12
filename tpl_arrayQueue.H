
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

/** @file tpl_arrayQueue.H
    @brief Circular queue implementations backed by arrays.

    This file provides two queue implementations:
    - `ArrayQueue<T>`: Dynamic circular queue with bounds checking
    - `FixedQueue<T>`: Fixed-capacity circular queue (no bounds checking)

    Both use circular indexing for O(1) enqueue/dequeue operations.

    @ingroup Sequences
 *  @author Leandro Rabindranath León
*/

#ifndef TPL_ARRAYQUEUE_H
#define TPL_ARRAYQUEUE_H

#include <ahAssert.H>
#include <ahDefs.H>
#include <htlist.H>
#include <ah-dry.H>
#include <ah-args-ctor.H>
#include <tpl_dynDlist.H>
#include <tpl_memArray.H>
#include <ah-iterator.H>

namespace Aleph {

/** Queue implemented with a single dynamic array.

    `ArrayQueue<T>` represents a queue if elements of generic type `T`
    which is stored in single dynamic. In this queue, modifiying
    operations test arrays bounds.

    The array is allocated with `new` operator and dynamically grows
    according to the size.

    @note MemArray is an implementation detail, inherited protectedly.
          Public indexing and first/last access use FIFO order. Raw buffer
          access and conversion to MemArray are intentionally unavailable.

    @ingroup Sequences
    @note If you are sure of knowing the bounds, then use `FixedStack`,
    which is faster because it does not perform bound_statics checks,
 */
template <typename T>
class ArrayQueue : protected MemArray<T>,
                   public LocateFunctions<ArrayQueue<T>, T>,
                   public FunctionalMethods<ArrayQueue<T>, T>,
                   public GenericKeys<ArrayQueue<T>, T>,
                   public EqualSequenceMethod<ArrayQueue<T>>,
                   public StlAlephIterator<ArrayQueue<T>>
{
private:
  size_t front_index;  // items are extracted from this point
  size_t rear_index;   // new item is inserted by this point

  /// @brief Advance a physical index modulo the allocated capacity.
  void increase_index(size_t &i, const size_t inc = 1) const noexcept
  {
    i += inc;
    i %= this->dim;
  }

  /// @brief Access the i-th item counting backwards from the youngest item.
  T &rear_item(const size_t i = 0) noexcept
  {
    const size_t offset = (rear_index + this->dim - (i + 1) % this->dim) % this->dim;
    return this->access(offset);
  }

  /// \overload
  /// A reference to const.
  const T &rear_item(const size_t i = 0) const noexcept
  {
    const size_t offset = (rear_index + this->dim - (i + 1) % this->dim) % this->dim;
    return this->access(offset);
  }

  /// @brief Reset indices after relocating the logical sequence to slot zero.
  void recenter_indices() noexcept
  {
    front_index = 0;
    rear_index = this->dim == 0 ? 0 : this->n % this->dim;
  }

public:
  using Item_Type = T;  ///< Type stored in the queue.
  using MemArray<T>::Min_Dim;
  using MemArray<T>::capacity;
  using MemArray<T>::size;
  using MemArray<T>::is_empty;
  using MemArray<T>::is_valid;
  using MemArray<T>::contract_threshold;

  /// Swap `this` with `q`
  void swap(ArrayQueue &q) noexcept
  {
    this->MemArray<T>::swap(q);
    std::swap(front_index, q.front_index);
    std::swap(rear_index, q.rear_index);
  }

  /** Construct a queue with capacity `sz`

      @param[in] sz initial capacity
      @throw bad_alloc if there is no enough memory
   */
  ArrayQueue(const size_t sz = 8) : MemArray<T>(sz), front_index(0), rear_index(0)
  {
    this->disable_annotations();  // circular layout: items are not a prefix
  }

  /// Copy constructor. The copy stores the items of `q` in queue order
  /// from slot 0, whatever the front position of `q` is: copying the raw
  /// buffer prefix would lose the items of a queue that has wrapped around.
  ArrayQueue(const ArrayQueue &q)
    requires(std::is_copy_constructible_v<T> and std::is_copy_assignable_v<T>)
    : MemArray<T>(q.dim), front_index(0), rear_index(0)
  {
    this->disable_annotations();
    for (size_t i = 0; i < q.n; ++i)
      this->access(i) = q.access((q.front_index + i) % q.dim);
    this->n = q.n;
    rear_index = q.n % this->dim;
  }

  /// Move constructor
  ArrayQueue(ArrayQueue &&q) noexcept
    : MemArray<T>(std::move(q)), front_index(q.front_index), rear_index(q.rear_index)
  {
    q.front_index = 0;
    q.rear_index = 0;
    q.disable_annotations();  // the empty source is still a circular queue
  }

  Special_Ctors(ArrayQueue, T);

  /// Copy assign (copy and swap: same queue order as the copy constructor)
  ArrayQueue &operator=(const ArrayQueue &q)
    requires(std::is_copy_constructible_v<T> and std::is_copy_assignable_v<T>)
  {
    if (this == &q)
      return *this;

    ArrayQueue copy(q);
    swap(copy);

    return *this;
  }

  /// Empty the queue keeping its capacity; both ends restart at slot 0.
  void empty() noexcept
  {
    MemArray<T>::empty();
    front_index = 0;
    rear_index = 0;
  }

  /// \overload empty()
  void clear() noexcept
  {
    empty();
  }

  /** @brief Empty the queue and reduce storage to Min_Dim.
      @throws std::bad_alloc or an exception from default construction of T.
      @note Both ends restart at slot zero. Failure leaves the queue unchanged.
  */
  void empty_and_release()
  {
    MemArray<T>::empty_and_release();
    front_index = 0;
    rear_index = 0;
  }

  /** Reserve capacity for at least `cap` items, keeping the queue order.

      Unlike `MemArray::reserve()`, which copies the raw buffer prefix,
      the items are moved in queue order from slot 0.

      @param[in] cap requested capacity
      @throws std::bad_alloc or an exception from constructing/moving T.
      @note O(cap + size()) time. References are invalidated on growth.
            A throwing move may leave some elements moved from.
  */
  void reserve(const size_t cap)
  {
    if (cap <= this->dim)
      return;

    this->reallocate(cap, front_index);
    recenter_indices();
  }

  /// Move assign
  ArrayQueue &operator=(ArrayQueue &&q) noexcept
  {
    std::swap(front_index, q.front_index);
    std::swap(rear_index, q.rear_index);
    static_cast<MemArray<T> *>(this)->swap(q);
    return *this;
  }

private:
  T &complete_put() noexcept
  {
    T &ret_val = this->access(rear_index);
    increase_index(rear_index);
    ++this->n;
    return ret_val;
  }

  /// Shared body of both put() overloads.
  template <class U>
  T &put_item(U &&item)
  {
    if (this->n == this->dim)  // growing frees the buffer that item may live in
      {
        T &ret_val = this->grow_and_insert(this->n, std::forward<U>(item), front_index);
        recenter_indices();
        return ret_val;
      }

    this->access(rear_index) = std::forward<U>(item);
    return complete_put();
  }

public:
  /** Copy and put an item in the queue

      @param[in] item to put; it may be an item of this queue, as in
                 `q.put(q.front())`, even when the queue must grow
      @return a modifiable reference to the copied item
      @throw bad_alloc if there is no enough memory
      @note The queue is unchanged if the allocation or the copy throws.
   */
  T &put(const T &item)
  {
    return put_item(item);
  }

  /** Move an item to the rear of the queue

      @param[in] item to put; it may be an item of this queue, which is
                 then left moved from
      @return a modifiable reference to the moved item
      @throw bad_alloc if there is no enough memory
      @note `item` is untouched if the allocation throws.
   */
  T &put(T &&item)
  {
    return put_item(std::move(item));
  }

  /// \overload put(const T & item)
  T &append(const T &item)
  {
    return put(item);
  }

  /// \overload put(T && item)
  T &append(T &&item)
  {
    return put(std::forward<T>(item));
  }

  /// \overload put(const T & item)
  T &insert(const T &item)
  {
    return put(item);
  }

  /// \overload put(T && item)
  T &insert(T &&item)
  {
    return put(std::forward<T>(item));
  }

  /** @brief Add sz logical slots at the rear without initializing scalars.
      @param[in] sz Number of slots to add; zero requires a nonempty queue.
      @return Reference to the youngest item after growth.
      @throws std::underflow_error if both sz and size() are zero.
      @throws std::overflow_error if sz exceeds twice the effective capacity
              minus size(); the effective capacity of a moved-from queue is
              Min_Dim.
      @throws std::bad_alloc or an exception from constructing/moving T.
      @note O(1) without reallocation; O(capacity()) otherwise. Initialize
            new scalar slots through front(i) or operator[] before reading.
   */
  T &putn(const size_t sz)
  {
    ah_underflow_error_if(sz == 0 and this->n == 0) << "queue is empty";
    const size_t cap = this->dim == 0 ? this->Min_Dim : this->dim;
    const size_t max_sz = cap > std::numeric_limits<size_t>::max() / 2
      ? std::numeric_limits<size_t>::max() - this->n : 2 * cap - this->n;
    ah_overflow_error_if(sz > max_sz) << "Maximum putn size reached";

    size_t remaining = sz;
    while (remaining > 0)
      {
        const size_t available = this->dim - this->n;
        if (available == 0)
          {
            if (this->expand(front_index))
              recenter_indices();
            continue;
          }

        const size_t delta = remaining < available ? remaining : available;
        increase_index(rear_index, delta);
        this->n += delta;
        remaining -= delta;
      }

    return rear_item();
  }

  /** Remove the oldest item of the queue and return a copy.

      @return a copy of extracted item
      @throw underflow_error if the queue is empty
  */
  T get()
  {
    ah_underflow_error_if(this->n == 0) << "queue is empty";

    T ret_val = std::move(this->access(front_index));
    --this->n;
    increase_index(front_index);

    if (this->contract(front_index))
      recenter_indices();

    return ret_val;
  }

  /** Remove the i oldest items of the queue.

      @param[in] i number of items to remove
      @return a modifiable reference to the oldest remaining item
      @throw underflow_error if `i` is greater than the number of items
  */
  T &getn(const size_t i)
  {
    ah_underflow_error_if(i >= this->n) << "queue is empty";

    this->n -= i;
    increase_index(front_index, i);

    if (this->contract(front_index))
      recenter_indices();

    return this->access(front_index);
  }

  /** Return the i-th oldest item of the queue.

      @param[in] i position to access
      @return a modifiable reference to the oldest i-th item of the queue
      @throw range_error if `i` is greater than the number of items
  */
  T &front(const size_t i = 0)
  {
    ah_range_error_if(i >= this->n) << "index of front out of range";

    return this->access((front_index + i) % this->dim);
  }

  /// \overload
  /// A reference to const.
  const T &front(const size_t i = 0) const
  {
    ah_range_error_if(i >= this->n) << "index of front out of range";

    return this->access((front_index + i) % this->dim);
  }

  /** Return the i-th youngest item of the queue.

      @param[in] i position to access
      @return a modifiable reference to the i-th youngest item of the queue
      @throw range_error if `i` is greater than the number of items
  */
  T &rear(const size_t i = 0)
  {
    ah_range_error_if(i >= this->n) << "index of rear out of range";

    return rear_item(i);
  }

  /// \overload
  /// A reference to const.
  const T &rear(const size_t i = 0) const
  {
    ah_range_error_if(i >= this->n) << "index of rear out of range";

    return rear_item(i);
  }

  /** @brief Access an item by its FIFO position.
      @param[in] i Zero-based index from the oldest item.
      @return Modifiable reference to the item.
      @throws std::out_of_range if i >= size().
      @note O(1).
  */
  T &operator[](const size_t i)
  {
    ah_out_of_range_error_if(i >= this->n) << "queue index out of range";
    return front(i);
  }

  /// \overload
  /// A reference to const.
  const T &operator[](const size_t i) const
  {
    ah_out_of_range_error_if(i >= this->n) << "queue index out of range";
    return front(i);
  }

  /** @brief Access an item by FIFO position with debug assertions.
      @param[in] i Index satisfying i < size().
      @return Modifiable reference to the item.
      @note O(1); no range check in Release builds.
      @throws Nothing.
  */
  T &operator()(const size_t i) noexcept
  {
    assert(i < this->n);
    return this->access((front_index + i) % this->dim);
  }

  /// \overload
  /// A reference to const.
  const T &operator()(const size_t i) const noexcept
  {
    assert(i < this->n);
    return this->access((front_index + i) % this->dim);
  }

  /** @brief Return the oldest item, in O(1).
      @return Modifiable reference to the front item.
      @throws std::underflow_error if empty.
  */
  T &first()
  {
    ah_underflow_error_if(this->n == 0) << "queue is empty";
    return front();
  }

  /// \overload
  /// A reference to const.
  const T &first() const
  {
    ah_underflow_error_if(this->n == 0) << "queue is empty";
    return front();
  }

  /** @brief Return the youngest item, in O(1).
      @return Modifiable reference to the rear item.
      @throws std::underflow_error if empty.
  */
  T &last()
  {
    ah_underflow_error_if(this->n == 0) << "queue is empty";
    return rear();
  }

  /// \overload
  /// A reference to const.
  const T &last() const
  {
    ah_underflow_error_if(this->n == 0) << "queue is empty";
    return rear();
  }

  /// @brief Alias for first(); throws std::underflow_error if empty.
  T &get_first() { return first(); }
  /// \overload
  /// A reference to const.
  const T &get_first() const { return first(); }

  /// @brief Alias for last(); throws std::underflow_error if empty.
  T &get_last() { return last(); }
  /// \overload
  /// A reference to const.
  const T &get_last() const { return last(); }

  /// @brief Alias for first(); throws std::underflow_error if empty.
  T &top() { return first(); }
  /// \overload
  /// A reference to const.
  const T &top() const { return first(); }

  /// @brief Remove the oldest item; same complexity and exceptions as get().
  T remove_first() { return get(); }

  /// @brief Alias for get(); remove the oldest item in FIFO order.
  T pop() { return get(); }

  /// @brief Alias for put(); enqueue a copy at the rear.
  T &push(const T &item) { return put(item); }

  /// @brief Alias for put(); enqueue an item by moving it at the rear.
  T &push(T &&item) { return put(std::move(item)); }

  /** @brief Remove the youngest item.
      @return The removed item, moved from the rear.
      @throws std::underflow_error if empty, or an exception from allocation/T.
      @note O(1) amortized; O(capacity()) if storage contracts.
  */
  T remove_last()
  {
    ah_underflow_error_if(this->n == 0) << "queue is empty";
    T result = std::move(rear_item());
    rear_index = (rear_index + this->dim - 1) % this->dim;
    --this->n;
    if (this->contract(front_index))
      recenter_indices();
    return result;
  }

  /** @brief Reverse the logical queue in place.
      @return This queue with FIFO order reversed.
      @throws Any exception from swapping T; earlier swaps are not undone.
      @note O(size()) time, O(1) auxiliary space.
  */
  ArrayQueue &reverse()
  {
    for (size_t i = 0; i < this->n / 2; ++i)
      std::swap(front(i), rear(i));
    return *this;
  }

  /** Traverse all the elements from the oldest to the youngest and execute
      `operation` on each on them.

      @param[in] operation to be performed on each element
      @return `true` if `operation` was executed on all elements;
      `false` otherwise.
   */
  template <class Operation>
    requires Aleph::PredicateWith<Operation &, T &>
  bool traverse(Operation &operation)
    noexcept(Aleph::NothrowPredicateWith<Operation &, T &>)
  {
    if (this->n == 0)
      return true;

    for (size_t i = 0, idx = front_index; i < this->n; increase_index(idx), ++i)
      if (not operation(this->access(idx)))
        return false;
    return true;
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
    requires Aleph::PredicateWith<Operation &, const T &>
  bool traverse(Operation &operation) const
    noexcept(Aleph::NothrowPredicateWith<Operation &, const T &>)
  {
    if (this->n == 0)
      return true;

    for (size_t i = 0, idx = front_index; i < this->n; increase_index(idx), ++i)
      if (not operation(static_cast<const T &>(this->access(idx))))
        return false;
    return true;
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
    requires Aleph::PredicateWith<Operation &, const T &>
  bool traverse(Operation &&operation) const
    noexcept(Aleph::NothrowPredicateWith<Operation &, const T &>)
  {
    return traverse<Operation>(operation);
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
    requires Aleph::PredicateWith<Operation &, T &>
  bool traverse(Operation &&operation)
    noexcept(Aleph::NothrowPredicateWith<Operation &, T &>)
  {
    return traverse<Operation>(operation);
  }

  /** Simple iterator on elements of a queue.

      The visit order is from the oldest to the youngest item.

      @ingroup Sequences
  */
  struct Iterator : public MemArray<T>::Iterator
  {
    using Base = typename MemArray<T>::Iterator;
    using Base::Base;
    using Set_Type = ArrayQueue;

    /// @brief Iterator on the modifiable queue `q`, in FIFO order (from the
    /// oldest item), including an empty moved-from queue.
    Iterator(ArrayQueue &q)
      : Base(q.ptr, q.dim, q.n, q.front_index,
             q.n == 0 ? 0 : (q.rear_index + q.dim - 1) % q.dim)
    {}

    /// A constant queue is iterated through Const_Iterator.
    Iterator(const ArrayQueue &) = delete;
  };

  /// Read-only iterator: the iterator of a constant container.
  using Const_Iterator = Aleph::Const_Iterator<Iterator>;
};

/** Very simple queue implemented with a contiguous array

    The capacity is given in construction time, but this is adjusted
    to the next two power immediately greater.

    This queue is the fastest of `Aleph-w` (\f$\aleph_\omega\f$), but
    take in account that no bound_statics cheks are performed. You must be
    sure that you usage is correct.

    @see ArrayQueue DynListQueue
    @ingroup Sequences
 */
template <typename T>
class FixedQueue : public LocateFunctions<FixedQueue<T>, T>,
                   public FunctionalMethods<FixedQueue<T>, T>,
                   public GenericKeys<FixedQueue<T>, T>,
                   public EqualSequenceMethod<FixedQueue<T>>,
                   public StlAlephIterator<FixedQueue<T>>
{
  size_t dim;
  T *array;
  size_t front_index; /* index of oldest inserted item */
  size_t rear_index;
  size_t mask;
  size_t num_items;

  void increase_index(size_t &i, const size_t inc = 1) const noexcept
  {
    assert(((i + inc) % dim) == ((i + inc) & mask));

    i += inc;
    i &= mask;
  }

  T &rear_item(const size_t i = 0) noexcept
  {
    assert(static_cast<size_t>((rear_index - i - 1) & mask) == (rear_index - i - 1) % dim);

    return array[static_cast<size_t>((rear_index - i - 1) & mask)];
  }

  /// \overload
  /// A reference to const.
  const T &rear_item(const size_t i = 0) const noexcept
  {
    assert(static_cast<size_t>((rear_index - i - 1) & mask) == (rear_index - i - 1) % dim);

    return array[static_cast<size_t>((rear_index - i - 1) & mask)];
  }

public:
  using Item_Type = T;

  void swap(FixedQueue &q) noexcept
  {
    std::swap(dim, q.dim);
    std::swap(array, q.array);
    std::swap(front_index, q.front_index);
    std::swap(rear_index, q.rear_index);
    std::swap(mask, q.mask);
    std::swap(num_items, q.num_items);
  }

  /// empty the queue
  void empty() noexcept
  {
    front_index = rear_index = num_items = 0;
  }

  /** @brief Empties the container.

      @throws none
      @see empty()
  */
  void clear() noexcept
  {
    empty();
  }

  /** Construct a queue of capacity the immediately two power of `d`

      The resulting capacity will be the first number such \f$2^k
      \geq d\f$,

      @param[in] d minimum capacity
   */
  FixedQueue(size_t d = 1024)
    // Don't change the default value because you would risk of
    // breaking the tests
    : dim(0), array(nullptr), front_index(0), rear_index(0), mask(0), num_items(0)
  {
    size_t k = log2(d);
    dim = is_power_of_2(d) ? d : 1 << ++k;
    mask = dim - 1;
    array = new T[dim];
  }

  ~FixedQueue()
  {
    if (array != nullptr)
      delete[] array;
  }

  /** Copy constructor: the copy holds the items of `q`, in the same slots.

      @param[in] q the queue to copy.
      @throw bad_alloc if there is no enough memory, or anything that the
             copy assignment of `T` throws; nothing leaks then.
  */
  FixedQueue(const FixedQueue &q)
    : dim(q.dim), array(new T[dim]), front_index(q.front_index), rear_index(q.rear_index),
      mask(q.mask), num_items(q.num_items)
  {
    try
      {
        // num_items slots along the circular index. Comparing with
        // rear_index copied nothing from a full queue (front_index equals
        // rear_index then) and ran past the buffer when the queue wraps.
        for (size_t i = 0, idx = front_index; i < num_items; ++i, increase_index(idx))
          array[idx] = q.array[idx];
      }
    catch (...)
      {
        delete[] array;  // the destructor does not run when a constructor throws
        throw;
      }
  }

  /// Move constructor
  FixedQueue(FixedQueue &&q) noexcept : FixedQueue()
  {
    swap(q);
  }

  Special_Ctors(FixedQueue, T);

  /// Copy assign
  FixedQueue &operator=(const FixedQueue &q)
  {
    if (this == &q)
      return *this;

    FixedQueue tmp(q);
    swap(tmp);
    return *this;
  }

  /// Move assign
  FixedQueue &operator=(FixedQueue &&q) noexcept
  {
    swap(q);
    return *this;
  }

  /** Put an item into the queue by copy

      @param[in] item to put
      @return a modifiable reference to the copied item.
   */
  T &put(const T &item) noexcept
  {
    assert(num_items < dim);
    array[rear_index] = item;
    T &ret_val = array[rear_index];
    increase_index(rear_index);
    num_items++;
    return ret_val;
  }

  /// \overload T & put(const T& item)
  T &put(T &&item) noexcept
  {
    assert(num_items < dim);
    array[rear_index] = std::forward<T>(item);
    T &ret_val = array[rear_index];
    increase_index(rear_index);
    num_items++;
    return ret_val;
  }

  /// \overload T & put(const T& item)
  T &append(const T &item) noexcept
  {
    return put(item);
  }

  /// \overload T & put(const T& item)
  T &append(T &&item) noexcept
  {
    return put(std::forward<T>(item));
  }

  /// \overload T & put(const T& item)
  T &insert(const T &item) noexcept
  {
    return put(item);
  }

  /// \overload T & put(const T& item)
  T &insert(T &&item) noexcept
  {
    return put(std::forward<T>(item));
  }

  /** Put `n` cells to the queue in constant time.

      @param[in] n number of cells
      @return a modifiable reference to the last inserted item
   */
  T &putn(const size_t n) noexcept
  {
    assert(num_items + n < dim);
    increase_index(rear_index, n);
    num_items += n;
    return rear_item();
  }

  /** Remove the oldest item of the queue.

      @return a copy of removed item
   */
  T get() noexcept
  {
    assert(num_items > 0);
    num_items--;
    T ret_val = std::move(array[front_index]);
    increase_index(front_index);
    return ret_val;
  }

  /** Remove in constant time the n oldest items of the queue

      @param[in] n number of items to remove
      @return a modifiable reference to the current oldest item in
      the queue after the removal of `n` entries
   */
  T &getn(const size_t n) noexcept
  {
    assert(num_items >= n);
    num_items -= n;
    increase_index(front_index, n);
    return array[front_index];
  }

  /** Return the i-th oldest item

    @param[in] i access position respect to oldest item
    @return a modifiable reference to the i-th item
 */
  T &front(const size_t i = 0) noexcept
  {
    assert(i < num_items);
    return array[(front_index + i) & mask];
  }

  /// \overload
  /// A reference to const.
  const T &front(const size_t i = 0) const noexcept
  {
    assert(i < num_items);
    return array[(front_index + i) & mask];
  }

  /** Return the i-th youngest item

      @param[in] i access position respect to youngest item
      @return a modifiable reference to the i-th item
   */
  T &rear(const size_t i = 0) noexcept
  {
    assert(i < num_items);
    return rear_item(i);
  }

  /// \overload
  /// A reference to const.
  const T &rear(const size_t i = 0) const noexcept
  {
    assert(i < num_items);
    return rear_item(i);
  }

  /// Return the number of items
  [[nodiscard]] constexpr size_t size() const noexcept
  {
    return num_items;
  }

  /// Return `true` if the queue is empty
  [[nodiscard]] constexpr bool is_empty() const noexcept
  {
    return num_items == 0;
  }

  /// Return the queue capacity (maximum number of element to be stored)
  [[nodiscard]] constexpr size_t capacity() const noexcept
  {
    return dim;
  }

  /** Traverse all the elements from the youngest to the oldest and execute
      `operation` on each on them.

      @param[in] operation to be performed on each element
      @return `true` if `operation` was executed on all elements;
      `false` otherwise.
   */
  template <class Operation>
    requires Aleph::PredicateWith<Operation &, T &>
  bool traverse(Operation &operation)
    noexcept(Aleph::NothrowPredicateWith<Operation &, T &>)
  {
    if (num_items == 0)
      return true;
    for (size_t i = 0, idx = front_index; i < num_items; increase_index(idx), ++i)
      if (not operation(array[idx]))
        return false;
    return true;
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
    requires Aleph::PredicateWith<Operation &, const T &>
  bool traverse(Operation &operation) const
    noexcept(Aleph::NothrowPredicateWith<Operation &, const T &>)
  {
    if (num_items == 0)
      return true;

    for (size_t i = 0, idx = front_index; i < num_items; increase_index(idx), ++i)
      if (not operation(static_cast<const T &>(array[idx])))
        return false;
    return true;
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
    requires Aleph::PredicateWith<Operation &, const T &>
  bool traverse(Operation &&operation = Operation()) const
    noexcept(Aleph::NothrowPredicateWith<Operation &, const T &>)
  {
    return traverse<Operation>(operation);
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
    requires Aleph::PredicateWith<Operation &, T &>
  bool traverse(Operation &&operation = Operation())
    noexcept(Aleph::NothrowPredicateWith<Operation &, T &>)
  {
    return traverse<Operation>(operation);
  }

  /** Simple iterator on elements of a queue.

      The visit order is from the youngest to the oldest item

      @ingroup Sequences
  */
  struct Iterator : public MemArray<T>::Iterator
  {
    using Base = typename MemArray<T>::Iterator;
    using Base::Base;
    using Set_Type = FixedQueue;

    /// Iterator on the modifiable queue `q`, from the oldest item.
    Iterator(FixedQueue &q) noexcept
      : Base(q.array, q.dim, q.num_items, q.front_index, (q.rear_index - 1) % q.dim)
    {}

    /// A constant queue is iterated through Const_Iterator.
    Iterator(const FixedQueue &) = delete;
  };

  /// Read-only iterator: the iterator of a constant container.
  using Const_Iterator = Aleph::Const_Iterator<Iterator>;
};

}  // end namespace Aleph

#endif /* TPL_ARRAYQUEUE_H */
