/*
  This file is part of Aleph-w library

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018

  Leandro Rabindranath Leon / Alejandro Mujica

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <https://www.gnu.org/licenses/>.
*/

# ifndef DLINK_H
# define DLINK_H

# include <aleph.H>

using namespace Aleph;

namespace Aleph {

template <typename T> class Dnode; // Forward declaration of derived class

    /**  Double link of a node belonging to a double circular linked
	 list with header node.

	 This class implements many operations on double circular linked
         list with header node.

         @ingroup Secuencias
         @see Macros_Dlink Dlink::Iterator Dnode Dlist DynDlist
    */
class Dlink
{
protected:

  mutable Dlink * prev;
  mutable Dlink * next;

public:

  template <typename T> inline Dnode<T> * to_dnode() noexcept;
  template <typename T> inline const Dnode<T> * to_dnode() const noexcept;
  template <typename T> inline T& to_data() noexcept;
  template <typename T> inline const T& to_data() const noexcept;

  /// Initialize a node or an empty list
  Dlink() noexcept : prev(this), next(this) { assert(is_empty()); }

    /// Copy constructor. Be very careful because if `l` contains
    /// items, then these will be lost
  Dlink(const Dlink & l) noexcept
    : prev(l.prev), next(l.next)
  {
    if (l.is_empty())
      reset();
  }

  /** Swap `this` with list whose header is `link`.

      `swap(link)` swaps the content og `this` with all the content of
      the list pointed by `link`. The operation is performed in constant
      time independently of sizes of both lists.

      \param[in] link pointer to the list to be swapped
   */
  void swap(Dlink * link) noexcept
  {
    if (is_empty() and link->is_empty())
      return;

    if (is_empty())
      {
        link->next->prev = this;
        link->prev->next = this;
        next = link->next;
        prev = link->prev;
        link->reset();
        return;
      }

    if (link->is_empty())
      {
        next->prev = link;
        prev->next = link;
        link->next = next;
        link->prev = prev;
        reset();
        return;
      }

    std::swap(prev->next, link->prev->next);
    std::swap(next->prev, link->next->prev);
    std::swap(prev, link->prev);
    std::swap(next, link->next);
  }

  /** Swap `this` with list whose header is `l`.

      `swap(l)` swaps the content og `this` with all the content of the
      list referenced by `l`. The operation is performed in constant
      time independently of sizes of both lists.

      \param[in] l list to be swapped with `this`
   */
  void swap(Dlink & l) noexcept
  {
    swap(&l);
  }

  /** Construct a new list with the items of l moved.

      This constructor takes a rvalue reference `l` to a list and moves it
      in constant time to `this`

      \param[in] l rvalue reference to the to be moved
   */
  Dlink(Dlink && l) noexcept : Dlink()
  {
    swap(l);
  }

  /** Copy assignation

	  \warning Be very careful with the possibility of `this` has
	  items, because these will be lost

          @param[in] l link to be copied
  */
  Dlink & operator = (const Dlink & l) noexcept
  {
    if (l.is_empty())
      return *this;

    assert(is_empty());
    prev = l.prev;
    next = l.next;
    return *this;
  }

  /** Move assignation

      Assign a rvalue list to `this` without copying.

      \note Since `l` is rvalue reference, take care of that if you are
      interested in to avoid the copy. So, if you have a lvalue
      reference to a list, use `std::move()`, upon your responsability,
      if and only if you are absolutely sure that the list will not be
      needed after.

      \param[in] rvalue reference to list to be copied
   */
  Dlink & operator = (Dlink && l) noexcept
  {
    swap(l);
    return *this;
  }

  /** Reset `this`

      `reset()` reinitialize the node to point to itself. So, all the
      context is lost. Use with care.

  */
  void reset() noexcept
  {
    next = prev = this;
  }

  /// \overload reset()
  void init() noexcept
  {
    reset();
  }

      /// Return `true` if `this` (as header node) is empty
  bool is_empty() const noexcept { return this == next and this == prev; }

   /// Return `true` if `this` (as header node) has exactly one element
  bool is_unitarian() const noexcept { return this != next and next == prev; }

   /// Return `true` if `this` (as header node) has zeor or one element
  bool is_unitarian_or_empty() const noexcept { return next == prev; }

    /** Insert `node` after `this`.

        @param[in] node pointer to an empty node (it must not be linked
        to nothing
    */
  void insert(Dlink * node) noexcept
  {
    assert(next != nullptr and prev != nullptr);
    assert(node != nullptr);
    assert(node->is_empty());

    node->prev = this;
    node->next = next;
    next->prev = node;
    next       = node;
  }

      /// \overload insert(Dlink * node)
  void push(Dlink * node) noexcept
  {
    insert(node);
  }

      /** Insert `node`before `this`.

          @param[in] node pointer to an empty node (it must no be
          inserted in another list)
      */
  void append(Dlink * node) noexcept
  {
    assert(next != nullptr and prev != nullptr);
    assert(node != nullptr);
    assert(node->is_empty());

    node->next = this;
    node->prev = prev;
    prev->next = node;
    prev       = node;
  }

    /// Return the link that is after `this`
  Dlink *& get_next() const noexcept
  {
    assert(next != nullptr and prev != nullptr);
    return next;
  }

    /// Return the link that is before `this`
  Dlink *& get_prev() const noexcept
  {
    assert(next != nullptr and prev != nullptr);
    return prev;
  }

    /// If `this` is a header node, it return the first node of `this`
  Dlink *& get_first() const noexcept { return next; }

    /// If `this` is a header node, it return the last node of `this`
  Dlink *& get_last() const noexcept { return prev; }

  /** Wrap a header to a list (without header).

      Sometimes, especially for low level applications, you coult manage
      linked list without header nodes. In this case, in order to profit
      some operations expeting a list with header, you could "wrap" a
      temporal header and use the list and the operations of this class.

      For example, suppose we have a list `l` without header node and we
      wish to insert it into a another list with a header node. In this
      case, we wrap a header to `l` as follows:

          Dlink h;
	  h.wrap_header(l);

      Now, if we have a node `p` of another list, we could insert `l`
      after `p` as follows:

          p->insert_list(&h);

      After this operation `h` becomes empty and the list `l` is
      inserted after the node `p`

      @param[in] l first node of a double and circular list **without
      header node**
   */
  void wrap_header(Dlink * l) noexcept
  {
    assert(is_empty());
    l->append(this);
  }

    /** Insert the list `head` before `this`

	This method assumes that `this` is a node part of list; it is
        not the header node. On the other hand, `head` is the header
        node of an entire list. So, all the list `head` is entirely
        inserted, in constant time, before the node `this`. After
        insertion, the list `head` becomes empty.

        @param[in] head header for the list to insert
     */
  void insert_list(Dlink * head) noexcept
  {
    if (head->is_empty())
      return;

    head->prev->next = next;
    head->next->prev = this;
    next->prev       = head->prev;
    next             = head->next;
    head->reset();
  }

    /** Insert the list `head` after `this`

	This method assumes that `this` is a node part of list; it is
        not the header node. On the other hand, `head` is the header
        node of an entire list. So, all the list `head` is entirely
        appended, in constant time, after the node `this`. After
        append, the list `head` becomes empty.

        @param[in] head header for the list to insert
     */
  void append_list(Dlink * head) noexcept
  {
    if (head->is_empty())
      return;

    head->next->prev = prev;
    head->prev->next = this;
    prev->next       = head->next;
    prev             = head->prev;
    head->reset();
  }

  /** Insert a list `l` without header node after the node `this`.

      @param[in] l list **without header node** to be inserted after `this`
   */
  void splice(Dlink * l) noexcept
  {
    Dlink head;
    head.wrap_header(l);
    insert_list(&head);
    assert(head.is_empty());
  }

    /** Concatenate list `head` to list `this`

        `this` and `head` are both header nodes of
        lists. `concat_list(head)` concatenates in constant time all the
        list `head` after `this`. After the concatenation `head` becomes
        empty.

        @param head header node of list to concatenate
     */
  void concat_list(Dlink * head) noexcept
  {
    assert(head != nullptr);

    if (head->is_empty())
      return;

    if (this->is_empty())
      {
        swap(head);
        return;
      }

    prev->next       = head->next;
    head->next->prev = prev;
    prev             = head->prev;
    head->prev->next = this;
    head->reset();
  }

  /// \overload concat_list(Dlink * head)
  void concat_list(Dlink & head) noexcept
  {
    concat_list(&head);
  }

  /// Remove `this` from the list. `this` must not be a header node
  Dlink * del() noexcept
  {
    assert(next != nullptr and prev != nullptr);

    prev->next = next;
    next->prev = prev;
    reset();
    return this;
  }

      /// \overload del()
  void erase() noexcept { del(); }

    /** Remove the item that is before `this`

	Remove the item predecessor of `this`.

	\note If `this` is a header node, then this method is equivalent
	to remove the last node.

	\warning The predecessor of `this` must not be a header node.

        @return a pointer to the removed node.
     */
  Dlink * remove_prev() noexcept
  {
    assert(not is_empty());

    Dlink* retValue = prev;
    retValue->del();

    return retValue;
  }

  /** Remove the item that is after `this`

      Remove the item successor of `this`.

      \note If `this` is a header node, then this method is equivalent
      to remove the first node.

      \warning The successor of `this` must not be a header node.

      @return a pointer to the removed node.
  */
  Dlink * remove_next() noexcept
  {
    assert(not is_empty());

    Dlink* retValue = next;
    retValue->del();

    return retValue;
  }

  Dlink * remove_last() noexcept /// \overload remove_prev()
  {
    return remove_prev();
  }

  Dlink * remove_first() noexcept /// \overload remove_next()
  {
    return remove_next();
  }

  /// \overload get_next()
  Dlink * top()
  {
    if (is_empty())
      throw std::underflow_error("Dlink as stack is not empty");
    return get_next();
  }

  Dlink * pop() /// \overload remove_next()
  {
    if (is_empty())
      throw std::underflow_error("Dlink as stack is not empty");
    return remove_next();
  }

      /** Reverse the list.

          @return the number of items that has the list
      */
  size_t reverse_list() noexcept
  {
    if (is_empty())
      return 0;

    Dlink tmp;
    size_t counter = 0;
    for (; not is_empty(); counter++)
      tmp.insert(remove_next());

    swap(&tmp);

    return counter;
  }

  /// \overload reverse_list
  size_t reverse() noexcept { return reverse_list(); }

  /**  Split `this` in the middle in two lists.

       `split_list(l, r)` searches the middle of `this` an on this point
       cuts the list in two lists `l` and `r` respectively. After the
       operation, `this` becomes empty.

       \note `l`

       @param[out] l first n/2 items of `this`
       @param[out] r last  n/2 items of `this`
       @return total number of nodes of both list (what is the same
       number of node that had `this` before the split
      */
  size_t split_list(Dlink & l, Dlink & r) noexcept
  {
    assert(l.is_empty() and r.is_empty()); // l y r deben estar vacías

    size_t count = 0;
    while (not is_empty())
      {
        l.append(remove_next()); ++count;

        if (is_empty())
          break;

        r.insert(remove_prev()); ++count;
      }

    return count;
  }

  /**  Cut `this` from `link`.

       `cut_list(link)` takes a valid link to an item of the list and on
       that `link` performs a cut; that is, all the items from `link`
       passes to a new list whose head is the return value.

       The operation takes constant time.

       \warning Takes in account that the return value is `Dlink`
       object, not a pointer.

       \warning if `link` belongs to a list, then this one will be in
       an inconsistent state

       @param[in] link pointer to the item from which the cut will be
       done
       @return a `Dlink` header of a new list containing the items from
       `link`
      */
  Dlink cut_list(Dlink * link) noexcept
  {
    assert(not is_empty() and not link->is_empty() and link != this);

    Dlink list;
    if (link == this->prev) // is link the last item?
      {
        link->del();
        list.append(link);
        return list;
      }

    if (link == this->next) // is link the first item?
      {
        list.swap(this);
        assert(this->is_empty());
        return list;
      }

    list.prev = this->prev;       // enlazar list a link (punto de corte)
    list.next = link;
    this->prev = link->prev;       // quitar de this todo a partir de link
    link->prev->next = this;
    link->prev = &list;       // colocar el corte en list
    list.prev->next = &list;

    return list;
  }

      /**  Iterator on `Dlink` lists

           @ingroup Secuencias
       */
  class Iterator
  {
  private:

    mutable Dlink * head = nullptr;
    mutable Dlink * curr = nullptr;

  public:

        /// The set type
    using Set_Type = Dlink;

        /// The type of elements of container
    using Item_Type = Dlink *;

      /** Initialize an iterator on the first item of the list pointed
          by `head_ptr`.

          @param[in] head_ptr pointer to the list (hedaer node)
        */
    Iterator(Dlink * head_ptr) noexcept
    : head(head_ptr), curr(head->get_next()) { /* */ }

        /** Initialize an iterator on the first item of `list`.

            @param[in] list reference to the list
        */
    Iterator(const Dlink & list) noexcept
      : head(&const_cast<Dlink&>(list)), curr(head->get_next())
    {
      // Empty
    }

    /** Set the current node .

        This method set the current node to `new_curr`, which of course
        must be a valid pointer to a item in the list.

        @param[in] new_curr pointer to the new current item
       */
    void set(Dlink * new_curr) noexcept
    {
      assert(curr != nullptr and head != nullptr);
      curr = new_curr;
    }

    Iterator() noexcept : head(nullptr), curr(nullptr) { /* Empty */ }

        /// Reset the iterator to the first item of list
    void reset_first() noexcept
    {
      assert(curr != nullptr and head != nullptr);
      curr = head->get_next();
    }

        /// Reset the iterator to the last item of list
    void reset_last() noexcept
    {
      assert(curr != nullptr and head != nullptr);
      curr = head->get_prev();
    }

        /// Put the iterator out of range
    void end()
    {
      reset_last();
      next();
    }

        /// Return `true` if the iterator has current item
    bool has_current() const noexcept
    {
      assert(curr != nullptr and head != nullptr);
      return curr != head;
    }

    /// \overload has_current()
    bool has_curr() const noexcept { return has_current(); }

    bool is_last() const noexcept
    {
      return head->is_empty() ? false : curr == head->prev;
    }

    /** Return the current node of iterator

        \return a pointer to the current node
        \throw overflow_error if there is not current node
    */
    Dlink * get_current() const
    {
      assert(curr != nullptr and head != nullptr);

      if (not has_current())
        throw std::overflow_error("Not element in list");

      return curr;
    }

    /// \overload get_current()
    Dlink * get_curr() const { return get_current(); }

        ///  Return `true` if the iterator is positiones on the first item
    bool is_in_first() const noexcept
    {
      return head->is_empty() ? false : curr == head->next;
    }

        ///  Return `true` if the iterator is positiones on the last item
    bool is_in_last() const noexcept { return is_last(); }

    /** Move the iterator one position backward

	\throw underflow_error if there is no current item
    */
    void prev() throw(std::exception, std::underflow_error)
    {
      if (not has_current())
        throw std::underflow_error("Not previous element in list");

      curr = curr->get_prev();
    }

    /** Move the iterator one position forward

        \throw overflow_error if there is no current item
    */
    void next() throw(std::exception, std::overflow_error)
    {
      if (not has_current())
        throw std::overflow_error("Not next element in list");

      curr = curr->get_next();
    }

    /// Return `true` if `this` and it are positioned on the same item
    bool operator == (const Iterator & it) const noexcept
    { return curr == it.curr; }

        /// Return `true` if `this` and `it` hace different states
    bool operator != (const Iterator & it) const noexcept
    { return curr != it.curr; }

    /** Remove from the list the current node and move the iterator one
        position forward.

        @return a pointer to removed item
    */
    Dlink * del()
    {
      assert(curr != nullptr and head != nullptr);
      assert(has_current());

      Dlink * current = get_current(); // obtener nodo actual
      next(); // avanzar al siguiente nodo
      current->del(); // eliminar de la lista antiguo nodo actual
      return current;
    }

    /// Return `true` if the iterator is on the list pointed by `l`
    bool verify(Dlink * l) const { return head == l; }

    /// Return `true` if `this` and `it` are on the same list
    bool verify(const Iterator & it) const { return head == it.head; }
  };

  /** Remove and free memory for all the items of list.

      `remove_all_and_delete()` remove each item of the list and call to
      `delete` operator on the removed item. At the end of call, all the
      items were removed, all the memory freed qand the list emptied.

      \warning This method only has sense if the items of list were
      dynamically allocated with `new`. Although That is very frequently
      the case, there are some exceptions. So, be sure that the items
      were allocated with `new` operator.
  */
  void remove_all_and_delete() noexcept
  {
    while (not is_empty())
      delete remove_next();
  }

  /** Rotate to left the list n positions.

      `rotate_left(n)` rotates the items to left `n` positions. For
      example, if the list is as follows:

          l0, l1, l2, l3, l4, l5, l6, l7, l8, l9

      Then `rotate_left(4)` produces the following state:

          l4, l5, l6, l7, l8, l9, l0, l1, l2, l3

      \param[in] n the number of position to be rotated
      \throw domain_error if list is empty
   */
  void rotate_left(size_t n)
  {
    if (is_empty())
      {
        if (n == 0)
          return;
        throw std::domain_error("List is empty");
      }

    for (size_t i = 0; i < n; ++i)
      append(remove_first());
  }

  /** Analogous to rotate_left() but to right */
  void rotate_right(size_t n)
  {
    if (is_empty())
      {
        if (n == 0)
          return;
        throw std::domain_error("List is empty");
      }

    for (size_t i = 0; i < n; ++i)
      insert(remove_last());
  }

  /// Return `true` if the list is consistent
  bool check()
  {
    Iterator itor(this);
    Dlink* node;

    for (/* nothing */; itor.has_current(); itor.next())
      {
        node = itor.get_current();

        if (not (node->get_next()->get_prev() == node))
          return false;

        if (not (node->get_prev()->get_next() == node))
          return false;
      }

    for (itor.reset_last(); itor.has_current(); itor.prev())
      {
        node = itor.get_current();

        if (not (node->get_next()->get_prev() == node))
          return false;

        if (not (node->get_prev()->get_next() == node))
          return false;
      }

    return true;
  }
};

    /** Generate a conversion function from a `Dlink` field to a class
	containing it.

	This macro is used when inside a class exists one o more `Dlink`
	fields and it is needed to obtain from a `Dlink` field a pointer
	to the class containing it.

	For example, suppose the following situation:

            struct Record
	    {
	      ...
	      Dlink l;
	      ...
	    };

        So, `DLINK_TO_TYPE(Record, l)` will generate the function:

            Record * dlink_to_Record(Dlink * link)

        which receives a pointer to the `Dlink` field `l` and returns a
        pointer to the class `Record` containing `l`.

        @param type_name the name of class or struct containing the
        `Dlink` field
        @param link_name the name of the `Dlink` field inside the class
        or struct.
        @ingroup Secuencias
    */
# define DLINK_TO_TYPE(type_name, link_name)			      \
  inline static type_name * dlink_to_##type_name(Dlink * link) noexcept \
  {									\
    type_name * ptr_zero = 0;						\
    size_t offset_link = reinterpret_cast<size_t>(&(ptr_zero->link_name)); \
    char * address_type = reinterpret_cast<char*>(link) - offset_link;	\
    return reinterpret_cast<type_name *>(address_type);			\
  }

    /** Generate a conversion function from a `Dlink` field pointer to
        a pointer to the class containing it.

        The name of the function is literally the name of second parameter

        This macro is used when one o more `Dlink` field are used inside
        a class and it is desired to obtain pointers to the class from
        the `Dlink` fields. Consider for example,

            struct Record
	          {
              ...
              Dlink l1;
              Dlink l2;
              ...
            };

        So, `LINKNAME_TO_TYPE(Record, l1)` and
        `LINKNAME_TO_TYPE(Record, l2)` will generate the following functions:

            Record * l1_to_type(Dlink * link)

	    Record * l2_to_type(Dlink * link)

	So for converting a `Dlink` pointer to `l1` to a pointer to
	`Record` object which contains it, you use
	`l1_to_type(link)`; analogously with `l2_to_type(link)`.

        The idea is having naming schemes allowing to distinguish
        several `Dlink` fields.

	\note You could have several `Dlink` fields. This situation
	arises if you want that the class simultaneously belongs to
	several lists. In this case you would declare a `Dlink` field by
	each different list.

        @param type_name the name of struct or class containing to the
        `Dlink` object.
        @param link_name the name of `Dlink` field
        @ingroup Secuencias
    */
# define LINKNAME_TO_TYPE(type_name, link_name)				\
  inline static type_name * link_name##_to_##type_name(Dlink * link) noexcept \
  {									\
    type_name * ptr_zero = 0;						\
    size_t offset_link = reinterpret_cast<size_t>(&(ptr_zero->link_name)); \
    char * address_type = reinterpret_cast<char*>(link) - offset_link;	\
    return reinterpret_cast<type_name *>(address_type);			\
  }


    /** Generate a function with name `link_to_base()` what converts a
        `Dlink` pointer to the class or struct that contains it.

        This macro could be used when a class have a `Dlink` field y
        and from it it is desired to get a pointer to the class. For
        example, suppose some such as:

	    struct Record
	    {
	      ...
	      Dlink l;
	      ...
	    };

        Then `DLINK_TO_BASE(Record, l)` will produce the function:

            Record * dlink_to_base(Dlink * link)

        which receives a pointer to the `Dlink` field `l` and return a
        pointer to the `Record` object storing `l`.

        @param type_name the name of class of struct containing the
        `Dlink` field.
        @param link_name the name of `Dlink` field inside the class or struct
        @ingroup Secuencias
    */
# define DLINK_TO_BASE(type_name, link_name)				\
  inline static type_name * dlink_to_base(Dlink * link)	noexcept	\
  {									\
    type_name * ptr_zero = 0;						\
    size_t offset_link = reinterpret_cast<size_t>(&(ptr_zero->link_name)); \
    char * address_type = reinterpret_cast<char*>(link) - offset_link;	\
    return reinterpret_cast<type_name *>(address_type);			\
  }
}

# endif /* DLINK_H */

