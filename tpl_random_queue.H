
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


/** @file tpl_random_queue.H
 *  @brief Random access queue (bag) with O(1) random pop.
 *
 *  Collection allowing random element extraction.
 *  Also defines Random_Set for random set operations.
 *
 *  @ingroup Sequences
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_RANDOM_QUEUE_H
# define TPL_RANDOM_QUEUE_H

# include <ah-random.H>
# include <htlist.H>
# include <ah-dry.H>
# include <tpl_dynArray.H>
# include <utility>
#include <ah-iterator.H>
 # include <ah-errors.H>

namespace Aleph {

    /** Random queue.

        Random_Set<T> implements a random set of items which are
        inserted and extracted in a random order.

	The class uses a Gsl_Compatible_Rng, the Mersenne twister with the
	numbers of the one of [GSL - GNU Scientific
	Library](\https://www.gnu.org/software/gsl/), which it used before:
	for a seed, the order is the same. By default, the seed is taken from
	system time.

	@ingroup Sequences 
     */
    template <class T> 
class Random_Set
      : public LocateFunctions<Random_Set<T>, T>,
	public FunctionalMethods<Random_Set<T>, T>,
	public GenericKeys<Random_Set<T>, T>,
	public StlAlephIterator<Random_Set<T>>
{
private:

  DynArray<T> array;
  Gsl_Compatible_Rng_Owner r;

public:

  using Set_Type = Random_Set; /// The type of set

  using Item_Type = T; /// The type of data contained in the queue

  /// The random number generator, which draws the positions
  [[nodiscard]] Gsl_Compatible_Rng & rng() const noexcept { return *r; }

# if ALEPH_WITH_GSL
  /** @brief A `gsl_rng` whose numbers come from rng().

      @deprecated The generator is no longer of GSL; use rng(). It goes in
      8.0.0.
      @return The `gsl_rng`, which the set frees.
      @throw std::bad_alloc If there is not enough memory, the first time.
  */
  [[deprecated("the generator is no longer of GSL: use rng(); it goes in 8.0.0")]]
  [[nodiscard]] gsl_rng * get_rng() const { return r.gsl(); }
# endif

  /// Set the seed of random number generator, as `gsl_rng_set()` did
  void set_seed(unsigned long seed) const noexcept { r->seed(seed); }

  /// Return the number of items in the queue
  [[nodiscard]] size_t size() const { return array.size(); }
  
  /// A random queue whose generator is seeded with the system time,
  /// modulo 2^32 - 1, as it was for GSL
  Random_Set()
    : array(0),
      r(time(nullptr) % static_cast<unsigned long>(Gsl_Compatible_Rng::max()))
  {
    // empty
  }

  /// Swap in constant time `this` with `s`
  void swap(Random_Set & s)
  {
    array.swap(s.array);
    r.swap(s.r);
  }

  /// Copy constructor
  Random_Set(const Random_Set & s) : Random_Set()
  {
    s.for_each([this] (auto item) { this->append(item); });
  }

  /// Move constructor
  Random_Set(Random_Set && s)  noexcept { swap(s); }

  Random_Set & operator = (const Random_Set & s)
  {
    if (this == &s)
      return *this;

    array.cut(0);  // Clear the array first
    s.for_each([this] (auto item) { this->append(item); });

    return *this;
  }

  Random_Set & operator = (Random_Set && s) noexcept
  {
    swap(s);
    return *this;
  }

  /// Initialize the random queue with the elements of `l`
  Random_Set(const DynList<T> & l) : Random_Set()
  {
    l.for_each([this] (const T & item) { append(item); });
  }

  /// Initialize the random queue with the items between `b` and `e`
  /// where these parameters are stl iterators
  template <class It> Random_Set(It b, It e) : Random_Set()
  {
    for (It it = b; it != e; ++it)
      append(*it);
  }
  
  /// Initialize the random queue with the items of `l`
  Random_Set(std::initializer_list<T> l) : Random_Set()
  {
    for (const auto & item : l)
      append(item);
  }

  /** Insert an item by copy.

      `put()` does not perform any random operation. It simply puts the
      item at the end of an internal sequence. It is when the item is
      extracted that a random choice is done.
      
      @param[in] item reference to the item to be inserted
      @throw bad_alloc if there is no enough memory
  */
  void put(const T & item)
  {
    static_assert(std::is_copy_constructible_v<T>,
                  "Random_Set::put requires copyable T");
    array.append(item);
  }

  
  /** Insert an item by moving
      
      `put()` does not perform any random operation. It simply puts the
      item at the end of an internal sequence. It is when the item is
      extracted that a random choice is done.
      
      @param[in] item rvalue reference to the item to be moved
      @throw bad_alloc if there is no enough memory
  */
  void put(T && item)
  {
    static_assert(std::is_move_constructible_v<T>,
                  "Random_Set::put requires moveable T");
    array.append(std::forward<T>(item));
  }

  /** Insert randomly an item by copy.

      `append(item)` performs a random choice. Use this method if you
      must obtain fastly a random array. It returns a reference to the
      stored element so callers can mutate it in place, same as other
      Aleph sequences.

      @param[in] item reference to the item to be inserted
      @return reference to the inserted item inside the queue
      @throw bad_alloc if there is no enough memory
  */
  T & append(const T & item)
  {
    put(item);
    const size_t last_index = array.size() - 1;
    const size_t pos = r->uniform_int(array.size()); // at random
    if (pos != last_index)
      std::swap(array(pos), array(last_index));
    return array(pos);
  }

   /** Insert randomly an item by moving.

      @param[in] item rvalue reference to the item to be inserted
      @return reference to the inserted item inside the queue
      @throw bad_alloc if there is no enough memory
  */
  T & append(T && item)
  {
    put(std::forward<T>(item));
    const size_t last_index = array.size() - 1;
    const size_t pos = r->uniform_int(array.size()); // at random
    if (pos != last_index)
      std::swap(array(pos), array(last_index));
    return array(pos);
  }

  /// Extract randomly an item 
  T get()
  {
    ah_underflow_error_if(array.size() == 0) << "Random set is empty";

    const size_t pos = r->uniform_int(array.size()); // at random
    T ret_val = array.access(pos);
    array.access(pos) = array.access(array.size() - 1);
    array.cut(array.size() - 1);
    return ret_val;
  }

  // pick() is like get() but without removing the item
  T pick() const
  {
    ah_underflow_error_if(array.size() == 0) << "Random set is empty";

    const size_t pos = r->uniform_int(array.size()); // at random
    return array.access(pos);
  }

  /// Return `true` if the queue is empty
  bool is_empty() const { return array.size() == 0; }

  /** Iterator of the items of the queue.

      The order will be random if the insertion were performed with
      `append()`: otherwise, the order will be the same that the put order.

      @ingroup Sequences 
   */
  struct Iterator : public DynArray<T>::Iterator
  {
    using Base = typename DynArray<T>::Iterator;

    /// Default constructor creates an "end" iterator
    Iterator() noexcept = default;

    /** @brief Iterator on a modifiable queue.
        @param[in] q queue to iterate.
    */
    Iterator(Random_Set & q) noexcept : Base(q.array) {}

    /// A constant set is iterated through Const_Iterator.
    Iterator(const Random_Set &) = delete;
  };

  /// Read-only iterator: the iterator of a constant container.
  using Const_Iterator = Aleph::Const_Iterator<Iterator>;

  /** Conditional traversing of the random queue

      @param[in] operation to be performed on each item. If it return
      `true` then the traversing continues; otherwise stops.
  */
  template <class Operation>
    requires Aleph::PredicateWith<Operation &, T &>
  bool traverse(Operation & operation)
    noexcept(noexcept(array.traverse(operation)))
  {
    return array.traverse(operation);
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
    requires Aleph::PredicateWith<Operation &, const T &>
  bool traverse(Operation & operation) const
    noexcept(noexcept(array.traverse(operation)))
  {
    return array.traverse(operation);
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
    requires Aleph::PredicateWith<Operation &, const T &>
  bool traverse(Operation && operation = Operation()) const
    noexcept(noexcept(array.traverse(operation)))
  {
    return traverse(operation);
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
    requires Aleph::PredicateWith<Operation &, T &>
  bool traverse(Operation && operation = Operation())
    noexcept(noexcept(array.traverse(operation)))
  {
    return traverse(operation);
  }
};


/** Randomly shuffle a sequence.

    `shuffle(c)` produces a random permutation of container `c`

    @param[in] c container to be shuffled
    @return a shuffled permutation of `c`
    @ingroup Sequences 
 */
  template <typename T, template <typename> class C>
auto shuffle(const C<T> & c)
{
  Random_Set<T*> q;
  c.for_each([&q] (const T & item) { q.put(&const_cast<T&>(item)); });
  
  C<T> ret;
  while (not q.is_empty())
    ret.append(*q.get());

  return ret;
}


/**  Return a random sequence of pointers to items of a sequence.

     @param[in] c container to be shuffled
     @return a shuffled permutation of pointer to items in the sequence `c`

    @ingroup Sequences 
 */
  template <typename T, template <typename> class C>
C<T*> shuffle_ptr(const C<T> & c)
{
  Random_Set<T*> q;
  c.for_each([&q] (const T & item) { q.put(&const_cast<T&>(item)); });
  
  C<T*> ret;
  while (not q.is_empty())
    ret.append(q.get());

  return ret;
}

} // end namespace Aleph

# endif // TPL_RANDOM_QUEUE_H

