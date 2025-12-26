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

# ifndef TPL_RANDOM_QUEUE_H
# define TPL_RANDOM_QUEUE_H

# include <gsl/gsl_rng.h>
# include <htlist.H>
# include <ah-dry.H>
# include <tpl_dynArray.H>
# include <utility>
 # include <ah-errors.H>

namespace Aleph {

    /** Random queue.

        Random_Set<T> implements a random set of items which are
        inserted and extracted in a random order.

	The class internally uses the gsl random number generator of 
	[GSL - GNU Scientific Library](\https://www.gnu.org/software/gsl/). 
	By default, the Mersene twister is used and the seed is taken from
	system time. 

	@ingroup Secuencias 
     */
    template <class T> 
class Random_Set
      : public LocateFunctions<Random_Set<T>, T>,
	public FunctionalMethods<Random_Set<T>, T>,
	public GenericKeys<Random_Set<T>, T>,
	public EqualToMethod<Random_Set<T>>,
	public StlAlephIterator<Random_Set<T>>
{
private:

  DynArray<T> array;
  gsl_rng * r = nullptr;

public:

  using Set_Type = Random_Set; /// The type of set

  using Item_Type = T; /// The type of data contained in the queue

  /// Return a pointer to random number generator
  [[nodiscard]] gsl_rng * get_rng() const { return r; }

  /// Set the seed of random number generator
  void set_seed(unsigned long seed) const noexcept { gsl_rng_set(r, seed); }

  /// Return the number of items in the queue
  [[nodiscard]] size_t size() const { return array.size(); }
  
  Random_Set() : array(0), r(nullptr)
  {
    r = gsl_rng_alloc (gsl_rng_mt19937);
    ah_bad_alloc_if(r == nullptr);

    gsl_rng_set(r, time(nullptr) % gsl_rng_max(r));
  }

  /// Swap in constant time `this` with `s`
  void swap(Random_Set & s)
  {
    array.swap(s.array);
    std::swap(r, s.r);
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

    if (s.size() > size())
      array.reserve(s.size());
    else
      array.cut(s.size);
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

  ~Random_Set()
  {
    if (r != nullptr)
      gsl_rng_free(r);
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
    const size_t pos = gsl_rng_uniform_int(r, array.size()); // al azar
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
    const size_t pos = gsl_rng_uniform_int(r, array.size()); // al azar
    if (pos != last_index)
      std::swap(array(pos), array(last_index));
    return array(pos);
  }

  /// Extract randomly an item 
  T get()
  {
    ah_underflow_error_if(array.size() == 0) << "Random set is empty";

    const size_t pos = gsl_rng_uniform_int(r, array.size()); // al azar
    T ret_val = array.access(pos);
    array.access(pos) = array.access(array.size() - 1);
    array.cut(array.size() - 1);
    return ret_val;
  }

  // pick() is like get() but without removing the item
  T pick() const
  {
    ah_underflow_error_if(array.size() == 0) << "Random set is empty";

    const size_t pos = gsl_rng_uniform_int(r, array.size()); // al azar
    T ret_val = array.access(pos);
    std::swap(array.access(pos), array.access(0));
    return ret_val;
  }

  /// Return `true` if the queue is empty
  bool is_empty() const { return array.size() == 0; }

  /** Iterator of the items of the queue.

      The order will be random if the insertion were performed with
      `append()`: otherwise, the order will be the same that the put order.

      @ingroup Secuencias 
   */
  struct Iterator : public DynArray<T>
  {
    using Base = DynArray<T>;
    using Base::Base;
  };

  /** Conditional traversing of the random queue

      @param[in] operation to be performed on each item. If it return
      `true` then the traversing continues; otherwise stops.
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
    return const_cast<Random_Set*>(this)->traverse(operation);
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
  bool traverse(Operation && operation = Operation()) const
  {
    return traverse(operation);
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
  bool traverse(Operation && operation = Operation())
  {
    return traverse(operation);
  }
};


/** Randomly shuffle a sequence.

    `shuffle(c)` produces a random permutation of container `c`

    @param[in] c container to be shuffled
    @return a shuffled permutation of `c`
    @ingroup Secuencias 
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

    @ingroup Secuencias 
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

