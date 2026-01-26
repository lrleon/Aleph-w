
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


/** @file tpl_dynDlist.H
 *  @brief Dynamic doubly linked list implementation.
 *
 *  This file provides `DynDlist`, a dynamic doubly linked list that supports
 *  efficient insertion and removal at both ends, bidirectional iteration,
 *  and a rich set of functional operations (map, filter, fold, etc.).
 *
 *  @ingroup Sequences
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_DYNDLIST_H
# define TPL_DYNDLIST_H

# include <ahFunctional.H>
# include <ahDry.H>
# include <ah-dry.H>
# include <ah-args-ctor.H>
# include <ahIterator.H>
# include <ah-iterator.H>
# include <tpl_dnode.H>
# include <stdexcept>
# include <ah-errors.H>

namespace Aleph {

  /** @brief Dynamic doubly linked list with O(1) size and bidirectional access.

      DynDlist<T> is a doubly linked circular list that maintains a count
      of elements, enabling O(1) size queries. Unlike DynList (singly linked),
      DynDlist supports efficient operations at both ends and bidirectional
      iteration.

      This container combines the flexibility of linked lists with a rich set
      of functional programming operations inherited from FunctionalMethods.

      @tparam T The type of elements stored in the list (default: int).

      @par Complexity:
      - insert() / append(): O(1)
      - remove_first() / remove_last(): O(1)
      - size(): O(1) - maintained by counter
      - Access by index: O(n)
      - Search: O(n)

      @par Key Features:
      - O(1) insertion/removal at both ends
      - O(1) size query (unlike DynList)
      - Bidirectional iteration
      - Comprehensive functional operations (map, filter, fold, etc.)
      - Support for initializer lists and range construction

      @par Example:
      @code
      DynDlist<int> list = {1, 2, 3, 4, 5};

      // O(1) operations
      list.insert(0);           // Insert at head
      list.append(6);           // Insert at tail
      auto first = list.remove_first();
      auto last = list.remove_last();

      // Functional operations
      auto doubled = list.maps<int>([](int x) { return x * 2; });
      int sum = list.foldl(0, [](int acc, int x) { return acc + x; });

      // Bidirectional iteration
      for (auto it = list.get_it(); it.has_curr(); it.next())
        std::cout << it.get_curr() << " ";

      for (auto it = list.get_last_it(); it.has_curr(); it.prev())
        std::cout << it.get_curr() << " ";
      @endcode

      @note The size is maintained automatically, so size() is O(1).
      @note This is a circular doubly linked list with sentinel header.

      @see DynList Singly linked list alternative (no size counter).
      @see Dlink Low-level doubly linked list node.
      @see FunctionalMethods Inherited functional operations.
      @ingroup Sequences
   */
    template <typename T = int>
class DynDlist
      : public Dnode<T>,
        public GenericTraverse<DynDlist<T>>,
        public SpecialCtors<DynDlist<T>, T>,
        public LocateFunctions<DynDlist<T>, T>,
        public FunctionalMethods<DynDlist<T>, T>,
        public GenericItems<DynDlist<T>, T>,
        public StlAlephIterator<DynDlist<T>>
{
  size_t num_elem = 0;

public:

      /// The type of container
  using Set_Type = DynDlist;

      /// The type of element that stores the container
  using Item_Type = T;

      /// The type of element that stores the container
  using Key_Type = T;

  /// Return the number of elements (constant time)
  const size_t & size() const noexcept { return num_elem; }

  DynDlist() = default;

  // using CtorBase = SpecialCtors<DynDlist<T>, T>;
  // using CtorBase::CtorBase;

  // /** Construct a new list with copies of elements of list `l`

  //     \param[in] l list to be copied
  //     \throw bad_alloc if there is no enough memory
  // */
  // DynDlist(const DynList<T> & l) : Dnode<T>(), CtorBase(l) {}

  Special_Ctors(DynDlist, T);

  void empty() noexcept /// Empty the list
  {
    while (not this->is_empty())
      delete this->Dnode<T>::remove_next();

    num_elem = 0;
  }

  ~DynDlist() /// Destructor
  {
    empty();
  }

private:

  T & __insert(Dnode<T> * p) noexcept
  {
    Dnode<T>::insert(p);
    ++num_elem;
    return p->get_data();
  }

  T & __append(Dnode<T> * p) noexcept
  {
    Dnode<T>::append(p);
    ++num_elem;
    return p->get_data();
  }

public:

  /** Insert a copy of item at the beginning of the list

      @param[in] item to be copied and inserted at the beginning
      @return a modifiable reference to the inserted item in the list
      \throw bad_alloc if there is no enough memory
  */
  T & insert(const T & item)
  {
    return __insert(new Dnode<T> (item));
  }

  /** Insert a moved item at the beginning of the list

      @param[in] item to be moved and inserted at the beginning of the list
      @return a modifiable reference to the inserted item in the list
      \throw bad_alloc if there is no enough memory
  */
  T & insert(T && item)
  {
    return __insert(new Dnode<T> (std::forward<T>(item)));
  }

    /** Append a copied item at the end of the list.

        @param[in] item to be copied and appended at the end of the list
        @return a modifiable reference to the appended item in the list
        @throw bad_alloc if there is no memory for the new item.
     */
  T & append(const T & item)
  {
    return __append(new Dnode<T> (item));
  }

  /** Append a moved item at the end of the list.

      @param[in] item to be moved and appended at the end of the list
      @return a modifiable reference to the appended item in the list
      @throw bad_alloc si no hay memoria para el nuevo elemento.
  */
  T & append(T && item)
  {
    return __append(new Dnode<T> (std::forward<T>(item)));
  }

 private:

  void __insert(DynDlist<T> & list) noexcept
  {
    Dlink::insert_list(&list);
    num_elem += list.num_elem;
    list.num_elem = 0;

    assert(list.is_empty());
  }

  void __append(DynDlist<T> & list) noexcept
  {
    Dlink::append_list(&list);
    num_elem += list.num_elem;
    list.num_elem = 0;

    assert(list.is_empty());
  }

 public:

      /** Insert all the elements of `list` before `this`.

          @param[in] list insert before `this`.
       */
  void insert(const DynDlist & list)
  {
    auto l = list; // perform a copy of list
    __insert(l);
  }

  /// \overload insert(const DynDlist & list)
  void insert(DynDlist && list) noexcept
  {
    __insert(list);
  }

  /** Append all the elements of `list` after `this`.

      @param[in] list to insert after `this`
  */
  void append(const DynDlist & list)
  {
    auto l = list; // perform a copy of list
    __append(l);
  }

  /// \overload append(const DynDlist & list)
  void append(DynDlist && list) noexcept
  {
    __append(list);
  }

    /// Return a modifiable reference to first item in the list
  T & get_first_ne() const noexcept
  {
    return this->get_next()->get_data();
  }

    /// Return a modifiable reference to last item in the list
  T & get_last_ne() const noexcept
  {
    return this->get_prev()->get_data();
  }

      /// Return a modifiable reference to first item in the list
  T & get_first() const
  {
    ah_underflow_error_if(this->is_empty())
      << "DynDlist is empty";
    return get_first_ne();
  }

    /// Return a modifiable reference to last item in the list
  T & get_last() const
  {
    ah_underflow_error_if(this->is_empty())
      << "DynDlist is empty";
    return get_last_ne();
  }

  /** Remove the first item of the list; return a copy of removed item.

        @return a copy of removed item
    */
  T remove_first_ne() noexcept
  {
    Dnode<T> * ptr = this->remove_next();
    T retVal = ptr->get_data();
    delete ptr;
    --num_elem;

    return retVal;
  }

  /** Remove the last item of the list; return a copy of removed item.

      @return a copy of removed item
  */
  T remove_last_ne() noexcept
  {
    Dnode<T> * ptr = this->remove_prev();
    T retVal = ptr->get_data();
    delete ptr;
    --num_elem;

    return retVal;
  }

  /** Remove the first item of the list; return a copy of removed item.

        @return a copy of removed item
        @throw underflow_error if `this` is empty
    */
  T remove_first()
  {
    ah_underflow_error_if(this->is_empty())
      << "DynDlist is empty";
    return remove_first_ne();
  }

  /** Remove the last item of the list; return a copy of removed item.

      @return a copy of removed item
      @throw underflow_error if `this` is empty
  */
  T remove_last()
  {
    ah_underflow_error_if(this->is_empty())
      << "DynDlist is empty";
    return remove_last_ne();
  }

  /// \overload append(const T & item)
  T & put(const T & item) { return append(item); }

  /// \overload append(T && item)
  T & put(T && item) { return append(std::forward<T>(item)); }

  /// \overload remove_first()
  T get() { return remove_first(); }

      /// If `this` was treated as a queue, the it returns the most
      /// recently inserted item
  T & rear() { return get_last(); }

      /// If `this` was treated as a queue, the it returns the most
      /// oldlest inserted item
  T & front() { return get_first(); }

      /// \overload append(const T & item)
  T & push(const T & item) { return insert(item); }

      /// \overload  append(T && item)
  T & push(T && item) { return insert(std::forward<T>(item)); }

      /// \overload remove_first()
  T pop() { return remove_first(); }

   /// \overload get_first()
  T & top() const { return get_first(); }

    /** Assuming that data is a reference to the item in the list, it
        removes the item.

        This method can be more powerful given that allows to remove
        any item in constant time given a valid reference to it.

        \warning Unpredictable results if the reference is not valid. So
        be sure that you do not pass a reference to a copy of the
        item. It must be a reference to the item returned by some
        accessor methods of this class.

        \param[in] data valid reference to the item in the list.
     */
  void remove(T & data) noexcept
  {
    Dnode<T> * p = Dnode<T>::data_to_node(data);
    p->del();
    delete p;
    --num_elem;
  }

    /// \overload remove(T & data)
  void erase(T & data) noexcept
  {
    remove(data);
  }

  /// Swap in constant time all the items of `this` with all the
  /// items of `l` (very fast!)
  void swap(DynDlist & l) noexcept
  {
    std::swap(num_elem, l.num_elem);
    this->Dlink::swap(&l);
  }

    /** Split the list in two.

      This method takes the first n/2 items of `this` and puts them, in
      the same order, in list `l`. The remainder items are put in list
      `r`. After operation `this` becomes empty. The order of items is
      preserved through `l` and `r`.

      \param[out] l list containing the first n/2 items
      \param[out] r list containing the last n/2 items
      @throw domain_error any of the lists is not empty
     */
  void split_list_ne(DynDlist & l, DynDlist & r) noexcept
  {
    Dlink::split_list(l, r);

    const auto count_nodes = [] (const DynDlist & list) noexcept -> size_t
    {
      size_t count = 0;
      for (auto it = list.get_it(); it.has_curr(); it.next_ne())
        ++count;
      return count;
    };

    l.num_elem = count_nodes(l);
    r.num_elem = count_nodes(r);
    num_elem = 0;
  }

      /** Split the list in two.

      This method takes the first n/2 items of `this` and puts them, in
      the same order, in list `l`. The remainder items are put in list
      `r`. After operation `this` becomes empty. The order of items is
      preserved through `l` and `r`.

      \param[out] l list containing the first n/2 items (must be empty)
      \param[out] r list containing the last n/2 items (must be empty)
      @throw domain_error if any of the lists is not empty
     */
  void split_list(DynDlist & l, DynDlist & r)
  {
    ah_domain_error_if((not l.is_empty()) or (not r.is_empty()))
      << "lists are not empty";
    split_list_ne(l, r);
  }

  /// \overload
  /// Alias for split_list. See split_list for parameter documentation.
  void split(DynDlist & l, DynDlist & r)
  {
    split_list(l, r);
  }

       /** Iterator dynamic list.

           @ingroup Sequences
       */
  class Iterator : public Dnode<T>::Iterator
  {
    DynDlist * list_ptr; // puntero a la lista
    long       pos;      // posición del elemento actual en la secuencia

    using Base = typename Dnode<T>::Iterator;

  public:

    /// The type of container
    using Set_Type = DynDlist;

        /// The type of element stored in the container
    using Item_Type = T;

    /// Return the ordinal position of current item
    long get_pos() const noexcept { return pos; }

    /// Move the iterator one position forward guaranteeing no
    /// exception. Be careful.
    void next_ne() noexcept
    {
      Dnode<T>::Iterator::next_ne();
      pos++;
    }

    /// Move the iterator one item forward. Throw overflow_error if
    /// iterator has not current item
    void next()
    {
      Dnode<T>::Iterator::next();
      pos++;
    }

    /// Move the iterator one item backward. Throw underflow_error if
    /// iterator has not current item
    void prev()
    {
      Dnode<T>::Iterator::prev();
      pos--;
    }

    /// Reset the iterator to the first item
    void reset_first() noexcept
    {
      Dnode<T>::Iterator::reset_first();
      pos = 0;
    }

    /// Reset the iterator to the last item
    void reset_last() noexcept
    {
      Dnode<T>::Iterator::reset_last();
      pos = list_ptr->num_elem - 1;
    }

    /// Put the iterator at the end state (where there is no current item)
    void end() noexcept
    {
      put_itor_at_the_end(*this);
    }

    /// Initialize the iterator to the first item of `list`
    Iterator(const DynDlist<T> & list) noexcept
    : Base(list), list_ptr(&const_cast<DynDlist&>(list)), pos(0)
    {
      // empty
    }

    Iterator() noexcept : list_ptr(nullptr) { /* empty */ }

    Iterator & operator = (const Iterator & it) noexcept
    {
      Dnode<T>::Iterator::operator = (it);
      list_ptr = it.list_ptr;
      pos      = it.pos;

      return *this;
    }

      /// Return the current item; throw `overflow_error` if there is no
      /// current item
    T & get_curr() const
    {
      return Dnode<T>::Iterator::get_curr()->get_data();
    }

    T & get_curr_ne() const noexcept
    {
      return Dnode<T>::Iterator::get_curr_ne()->get_data();
    }

      /** Insert by copy an item after the current item.

          This method inserts a copy of `item` after the current item of
          iterator. The current item is not moved.

          @param[in] item the item to insert
          \throw bad_alloc if there is no enough memory
          \throw overflow_error if there is no current item
       */
    void insert(const T & item)
    {
      ah_overflow_error_if(not this->has_curr())
        << "DynDlist Iterator has not current";

      Dnode<T>::Iterator::get_curr()->insert(new Dnode<T>(item));
      ++list_ptr->num_elem;
    }

    /** Insert by movement an item after the current item.

        This method inserts a new element in the list after the
        current and then `item` is moved. The current item is not
        moved.

        @param[in] item the item to insert
        \throw bad_alloc if there is no enough memory
        \throw overflow_error if there is no current item
    */
    void insert(T && item)
    {
      ah_overflow_error_if(not this->has_curr())
        << "DynDlist Iterator has not current";

      Dnode<T>::Iterator::get_curr()->
        insert(new Dnode<T>(std::forward<T>(item)));
      ++list_ptr->num_elem;
    }

    /** Append by copy an item before the current item.

        This method appends a copy of `item` before the current item
        of iterator. The current item is not moved.

        @param[in] item the item to append
        \throw bad_alloc if there is no enough memory
        \throw overflow_error if there is no current item
    */
    void append(const T & item)
    {
      ah_overflow_error_if(not this->has_curr())
        << "DynDlist Iterator has not current";

      Dnode<T>::Iterator::get_curr()->append(new Dnode<T>(item));
      ++list_ptr->num_elem;
    }

    /** Append by movement an item after the current item.

        This method appends a new element in the list before the
        current and then `item` is moved. The current item is not
        moved.

        @param[in] item the item to append
        \throw bad_alloc if there is no enough memory
        \throw overflow_error if there is no current item
    */
    void append(T && item)
    {
      ah_overflow_error_if(not this->has_curr())
        << "DynDlist Iterator has not current";

      Dnode<T>::Iterator::get_curr()->
        append(new Dnode<T>(std::forward<T>(item)));
      ++list_ptr->num_elem;
    }

    /** Move and insert all elements of `list` after the current item of
        iterator.

        This operation takes constant time and puts the element of
        `list` just after the current item. The current item of
        iterator is not moved. A `next()` operation will move forward
        the iterator to the first item of `list`.

        After operation `list` becomes empty, since all its items were
        moved to the list on which one is iterating.

        @param[in, out] list to insert after current item. It becomes
        empty.
        @throw overflow_error if there is no current item
    */
    void insert_list(DynDlist & list)
    {
      ah_overflow_error_if(not this->has_curr())
        << "DynDlist Iterator has not current";

      Dnode<T>::Iterator::get_curr()->insert_list(&list);
      list_ptr->num_elem += list.num_elem;
      list.num_elem = 0;

      assert(list.is_empty());
    }

    /** Move and append all elements of `list` before the current item of
        iterator.

        This operation takes constant time and puts the element of
        `list` just before the current item. The current item of
        iterator is not moved. A `prev()` operation will move forward
        the iterator to the last item of `list`.

        After operation `list` becomes empty, since all its items were
        moved to the list on which one is iterating.

        @param[in, out] list to append after current item. It becomes
        empty.
        @throw overflow_error if there is no current item
    */
    void append_list(DynDlist & list)
    {
      ah_overflow_error_if(not this->has_curr())
        << "DynDlist Iterator has not current";

      Dnode<T>::Iterator::get_curr()->append_list(&list);
      list_ptr->num_elem += list.num_elem;
      list.num_elem = 0;

      assert(list.is_empty());
    }

    /** Remove from the list the current node and move the iterator one
        position forward.

        @return a copy to removed item
        \throw overflow_error if there is no current item
    */
    T del()
    {
      ah_overflow_error_if(not this->has_curr())
        << "DynDlist Iterator has not current";

      Dnode<T> * ptr = Dnode<T>::Iterator::get_curr();
      T ret_val      = ptr->get_data();
      Dnode<T>::Iterator::next();
      ptr->del();
      delete ptr;
      --list_ptr->num_elem;
      return ret_val;
    }
  };

    /** Assignment by copy.

        In this assignment `this` is emptied and the the items of
        `list` are copied to `this`. It takes since time proportional
        to the size of `list` more the old size of `this`

        @param[in] list to be copied
        \throw bad_alloc if there is no enough memory
    */
  DynDlist<T> & operator = (const DynDlist & list)
  {
    if (this == &list)
      return *this;

    empty();

    for (Iterator itor(const_cast<DynDlist&>(list));
         itor.has_curr(); itor.next_ne())
      this->append(itor.get_curr());

    return *this;
  }

  /**  Copy constructor; all items of `list` are copied.

       The construction time is proportional to the number of items of `list`

       \param[in] list to be copied
       \throw bad_alloc if there is no enough memory
  */
  DynDlist(const DynDlist & list) : DynDlist()
  {
    assert(this->is_empty());

    for (Iterator itor(const_cast<DynDlist&>(list));
         itor.has_curr();itor.next_ne())
      this->append(itor.get_curr());
  }

  /**  Move constructor; all items of `list` are moved.

       The construction time is constant, independently of number of
       items of `list`

       \param[in] list to be moved
       \throw bad_alloc if there is no enough memory
  */
  DynDlist(DynDlist<T> && list) noexcept
    : DynDlist()
  {
    swap(list);
  }

  /** Assignment by moving.

      In this assignment `this` is swapped with `list`. So, this takes
      constant time independently of list sizes.

      @param[in] list to be assigned by moving
      \throw bad_alloc if there is no enough memory
  */
  DynDlist<T> & operator = (DynDlist && list) noexcept
  {
    swap(list);
    return *this;
  }

  /// Return a modifiable reference to the i-th item of the list.
  /// @throw out_of_range if `n` is greater than or equal to the list size.
  T & operator [] (const size_t n) const
  {
    auto it = this->get_it();
    for (size_t i = 0; i < n && it.has_curr(); ++i)
      it.next_ne();

    ah_out_of_range_error_if(not it.has_curr())
      << "DynDlist index out of range";

    return it.get_curr();
  }

  DynDlist & reverse() noexcept
  {
    Dlink::reverse();
    return *this;
  }

  DynDlist & rev() noexcept { return reverse(); }

  /** Return a reversed copy of `this`

      @note Not confuse with reverse without const, which is mutable
   */
  DynDlist<T> reverse() const
  {
    DynDlist<T> ret;
    for (auto it = this->get_it(); it.has_curr(); it.next_ne())
      ret.insert(it.get_curr());
    return ret;
  }

  DynDlist<T> rev() const { return reverse(); }
};

} // end namespace Aleph

# endif /* TPL_DYNDLIST_H */

