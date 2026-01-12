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

/** @file Priority_Queue.H
 *  @brief STL-compatible priority queue adapter (legacy).
 *
 *  @deprecated This is a legacy Aleph implementation of std::priority_queue.
 *  It will be deprecated in future versions. For new code, prefer using
 *  `std::priority_queue` directly or `Aleph::DynBinHeap` for Aleph-native features.
 *
 *  Aleph implementation of std::priority_queue using binary heap
 *  for O(log n) push/pop with STL-compatible interface.
 *
 *  @ingroup Sequences
 *  @author Leandro Rabindranath León
 */

# ifndef AH_PRIORITY_QUEUE_H
# define AH_PRIORITY_QUEUE_H

# include <utility>
# include <vector>
# include <tpl_dynBinHeap.H>

namespace Aleph {

    /** Aleph implementation of a priority queue.

        This container is implemented on top of `DynBinHeap`, which stores
        heap nodes dynamically (no backing array). Consequently, other
        structures or algorithms can safely keep pointers/references to items
        stored in the queue (the underlying nodes are allocated individually).

        Important: this is a *min-heap* with the default comparator
        (`Compare = Aleph::less<T>`). Therefore, `top()` returns the smallest
        element and `pop()` removes the smallest element.

        This version does not take an external container type parameter like
        `std::priority_queue`; therefore the constructors differ slightly.

        @see DynBinHeap
        @ingroup STL
    */
    template <typename T, class Compare = Aleph::less<T> >
 class priority_queue : private Aleph::DynBinHeap<T, Compare>
{
public:

  using Base = Aleph::DynBinHeap<T, Compare>;

      /// Value type stored by the queue.
  typedef T value_type;

      /// Construct an empty priority queue.
  priority_queue() { /* empty */ }

      /** Construct a queue with all elements from a container.

          @tparam Container Iterable container.
          @param cont Container whose elements will be inserted.
      */
      template <class Container>
  priority_queue(const Container & cont)
  {
    for (auto & item : cont)
      this->insert(item);
  }

      /** Construct a queue with all elements in the iterator range [beg, end).

          @tparam Itor Input iterator type.
          @param beg Begin iterator.
          @param end End iterator.
      */
      template <class Itor>
  priority_queue(Itor beg, const Itor & end)
  {
    while (beg != end)
      this->insert(*beg++);
  }

      /** Insert a copy of @p value into the queue.

          @param value Value to insert.
          @return Modifiable reference to the stored element (non-standard).
      */
  T & push(const T & value) { return this->insert(value); }

      /** Insert a value by moving it into the queue.

          @param value Value to move into the queue.
          @return Modifiable reference to the stored element (non-standard).
      */
  T & push(T && value) { return this->insert(std::move(value)); }

      /** Construct a value in-place and insert it into the queue.

          @tparam Args Constructor argument types.
          @param args Arguments forwarded to T's constructor.
          @return Modifiable reference to the stored element (non-standard).

          @note This implementation constructs a temporary `T` and then moves it
          into the underlying heap.
      */
  template <class... Args>
  T & emplace(Args && ... args)
  {
    T tmp(std::forward<Args>(args)...);
    return this->insert(std::move(tmp));
  }

      /** Remove the smallest element from the queue.

          @throw underflow_error if the queue is empty.
      */
  void pop() { this->getMin(); }

      /** Return the smallest element in the queue.

          @return Reference to the smallest element.
          @throw underflow_error if the queue is empty.
      */
  const T & top() const { return Aleph::DynBinHeap<T, Compare>::top(); }

      /// Return true if the queue is empty.
  bool empty() const { return this->is_empty(); }

      /// Return the number of elements.
  size_t size() const { return Aleph::DynBinHeap<T, Compare>::size(); }

      /// Remove all elements.
  void clear() noexcept { Base::empty(); }

      /** Swap contents with another priority queue.

          @note This swap is implemented by draining both queues and
          reinserting the elements, because the underlying heap swap is not
          safe for all internal invariants.

          @note Complexity is O((n+m) log(n+m)) in the number of elements.
      */
  void swap(priority_queue & other) noexcept
  {
    if (this == &other)
      return;

    std::vector<T> a;
    std::vector<T> b;
    a.reserve(this->size());
    b.reserve(other.size());

    while (not this->empty())
      {
        a.push_back(this->top());
        this->pop();
      }

    while (not other.empty())
      {
        b.push_back(other.top());
        other.pop();
      }

    for (auto & item : b)
      this->push(std::move(item));

    for (auto & item : a)
      other.push(std::move(item));
  }
};

} // end namespace Aleph

# endif // AH_PRIORITY_QUEUE_H
