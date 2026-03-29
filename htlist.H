
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


/** @file htlist.H
 *  @brief Singly linked list implementations with head-tail access.
 *
 *  This file provides efficient singly linked list data structures including
 *  `Slinknc` (non-circular link), `Snodenc` (node with data), `HTList`
 *  (head-tail list), and `DynList` (dynamic list with functional operations).
 *  These are fundamental building blocks used throughout Aleph-w.
 *
 *  @ingroup Sequences
 *  @author Leandro Rabindranath León
 */

# ifndef HTLIST_H
# define HTLIST_H

# include <type_traits>
# include <cassert>
# include <stdexcept>
# include <utility>
# include <ahFunction.H>
# include <ahFunctional.H>
# include <ah-args-ctor.H>
# include <ah-iterator.H>
# include <ah-errors.H>
# include <ah-dry.H>

# define NEXT(p) (p->get_next())

namespace Aleph {

template <typename T> class Snodenc; // forward declaration

  /**

      Link of a single linked list non-circular and without header node.

     @ingroup Sequences
     
     @note STRICT ALIASING DESIGN NOTE:
     
     This class stores `next` as `Slinknc*`. Derived classes like `Snodenc<T>`
     need to return `Snodenc<T>*&` from `get_next()`, which requires a
     `reinterpret_cast`. This violates C++ strict aliasing rules and requires
     the compiler flag `-fno-strict-aliasing` for correct behavior.
     
     A cleaner design would use CRTP (Curiously Recurring Template Pattern):
     @code
     template <typename Derived>
     class Slinknc_Base {
       Derived* next = nullptr;  // Type is already correct
     public:
       Derived*& get_next() { return next; }  // No cast needed
     };
     
     template <typename T>
     class Snodenc : public Slinknc_Base<Snodenc<T>> { ... };
     @endcode
     
     This refactoring is left for future work to avoid breaking changes.

  */
class Slinknc
{
  Slinknc * next = nullptr;

public:

  virtual ~Slinknc() = default;

  /// Return `true` if `this` is empty
  [[nodiscard]] constexpr bool is_empty() const noexcept { return next == nullptr; }

  /**  Init to nullptr
       */
  Slinknc() noexcept : next(nullptr) { /* empty */ }

  /** Dummy copy constructor. Link is set to nullptr
      */
  Slinknc(const Slinknc &) noexcept : next(nullptr) { /* empty */ }

  /** Reset the link to nullptr
      */
  void reset() noexcept
  {
    next = nullptr;
  }

   /** Dummy asignation; link is set to nullptr
       */
  Slinknc & operator = (const Slinknc & ) noexcept
  {
    next = nullptr;
    return *this;
  }

  
  Slinknc *& get_next() noexcept
  {
    return next;
  }

  

    /**
    
    `insert(p)` inserts the node pointed by `p` after `this`.

    @param[in] p pointer to link (or node)

    */
  void insert(Slinknc * p) noexcept
  {
    assert(p != nullptr);
    p->next = next;
    next = p;
  }

    /**
              Remove for linked list the node pointed by `this`

       @return a pointer to the removed node

       */
  Slinknc * remove_next() noexcept
  {
    Slinknc * ret_val = next;
    next = ret_val->next;
    ret_val->reset();
    return ret_val;
  }

  /** Convert `this` to a `Snodenc<T>`.

      @note Invalid and probably buggy behavior if `this` is not a
      valid pointer
  */
  template <typename T> inline
  Snodenc<T> * to_snodenc() noexcept;

  template <typename T> inline T & to_data() noexcept;

  template <typename T> inline
  const Snodenc<T> * to_snodenc() const noexcept;

  template <typename T> inline const T & to_data() const noexcept;

  /**
          Iterator on single links

     \ingroup Sequences

     */
  class Iterator
  {
    Slinknc * head;
    Slinknc * curr;

    void init() noexcept
    {
      curr = head->is_empty() ? nullptr : head->next;
    }

  public:

    Iterator() noexcept : head(nullptr), curr(nullptr)
    {
      // Empty
    }

    /** Initialize an iterator on the first node of `list`
        */
    Iterator(Slinknc & list) noexcept : head(&list)
    {
      init();
    }

    /**
       
       Initialize an iterator on the list pointed by `_head`, but on a
       node pointed by `curr`

       */
    Iterator(Slinknc * _head, Slinknc * _curr) noexcept
      : head(_head), curr(_curr)
    {
      // Empty
    }

    /**
              Return `true` if the iterator is positioned on a valid link

       */
    bool has_curr() const noexcept
    {
      return curr != nullptr;
    }

    /**
              Return the current link on which the iterator is positioned.
       Throw `overflow_error` if there is no current link.

       */
    Slinknc * get_curr() const
    {
      ah_overflow_error_if(not has_curr())
        << "Iterator is at the end of the list";
      return curr;
    }

    /// Return the current link guaranteeing no exception. Be careful.
    Slinknc * get_curr_ne() const noexcept { return curr; }

    /// Move the iterator one position forward guaranteeing no
    /// exception. Be careful.
    void next_ne() noexcept { curr = curr->next; }

    /**
              Move the iterator one position forward. Throw `overflow_error`
       if there is no current link.

       */
    void next()
    {
      ah_overflow_error_if(not has_curr())
        << "Iterator is at the end of the list";
      curr = curr->next;
    }

    /** Reset the iterator to the first link on the list
        */
    void reset_first() noexcept
    {
      init();
    }
  };
};


  /**
          Node belonging to a single non-circular linked list without header
     node.

     \ingroup Sequences

     */
    template <typename T>
class Snodenc : public Slinknc
{
  T data;

public:

  /** Return a modifiable reference to the data
      */
  T & get_data() noexcept { return data; }

  /** Return a constant reference to the data
      */
  const T & get_data() const noexcept { return data; }

  Snodenc()
  {
    static_assert(std::is_default_constructible<T>::value,
                  "No default constructor for T");
  }

  /** Construct with copy of `item`
      */
  Snodenc(const T & item) : data(item)
  {
    static_assert(std::is_copy_constructible<T>::value,
                  "No copy constructor for T");
  }

  /** Construct by moving `item`
      */
  Snodenc(T && item)
    : data(std::forward<T>(item))
  {
    static_assert(std::is_move_constructible<T>::value,
                  "No move constructor for T");
  }

      /**
                  Remove the node following to `this`.

         `remove_next()`removes the node linked after `this` and
         returns its address.

         @return a pointer to the removed node.

         */
  Snodenc * remove_next() noexcept
  {
    return static_cast<Snodenc *>(Slinknc::remove_next());
  }

  /** Return the node following to `this`
      */
  Snodenc *& get_next() noexcept
  {
    // STRICT ALIASING WARNING: This reinterpret_cast violates C++ strict
    // aliasing rules. Requires -fno-strict-aliasing compiler flag.
    return reinterpret_cast<Snodenc *&>(Slinknc::get_next());
  }

  /// \overload remove_next()
  Snodenc * remove_first() noexcept{ return Snodenc::remove_next(); }

  /**
          Return the node following to `this`.

     In the case where `this` is a pointer to the first node, returns
     the first node.

     \return a pointer to the first node.

     */
  Snodenc *& get_first() const noexcept { return Snodenc::get_next(); }
};

  template <typename T> inline
Snodenc<T> * Slinknc::to_snodenc() noexcept
{
  return static_cast<Snodenc<T>*>(this);
}

    template <typename T> inline
const Snodenc<T> * Slinknc::to_snodenc() const noexcept
{
  return static_cast<const Snodenc<T>*>(this);
}

template <typename T> T & Slinknc::to_data() noexcept
{
  return this->to_snodenc<T>()->get_data();
}

template <typename T> const T & Slinknc::to_data() const noexcept
{
  return this->to_snodenc<T>()->get_data();
}

    /**

       
       Single linked list of nodes.

       HTList models a list of nodes of type `Slinknc`. A `Slinknc`
       object is a link that could be contained inside any data
       structure. The possibility for belonging to a `HTList is defined
       for the owner of one o more objects of type `Slinknc`.

       A `HTlist` object maintains two references. The first one is
       called `head`" and corresponds to the first node. The second one
       is called `tail` and corresponds to last node.

       Take in account that this class does not manage memory neither
       access to the data that could be stored in the nodes.
       Slinknc y Snodenc. However, since this class is inherited by
       other more sophisticated classes, we will use the term "item"
       instead of node for referring an element of this list of nodes.

       @see Slinknc Snodenc DynList
       @author Alejandro Mujica
       @author Leandro Rabindranath León
       @ingroup Sequences

       */
class HTList
{
  Slinknc * head;
  Slinknc * tail;

public:

      
  HTList() noexcept : head(nullptr), tail(nullptr) { /* empty */ }

  void reset() noexcept
  {
    head = tail = nullptr;
  }

      
      
  [[nodiscard]] constexpr bool is_empty() const noexcept { return head == nullptr; }

       //!Return true if the list contains exactly just one
       
       
  bool is_unitarian() const noexcept
  {
    return head != nullptr and head == tail;
  }

  //!Return true if list contains one element or is empty
  
  bool is_unitarian_or_empty() const noexcept { return head == tail; }

  
  
  Slinknc * get_head() const noexcept { return head; }

 
 
  Slinknc * get_tail() const noexcept { return tail; }

 
 
  Slinknc * get_first() const noexcept { return get_head(); }

      /// Return the last item of the list (`nullptr` if the list is empty)
  Slinknc * get_last() const noexcept { return get_tail(); }

   
   

  /**
          \brief  Swap in constant time (very fast) 'this' elements with 'l'
     list elements <br>
     Referenced by append(), insert(), reverse() and split_list().

     \param [in] l New list which elements will be exchanged

     */
  HTList & swap(HTList & l) noexcept
  {
    std::swap(head, l.head);
    std::swap(tail, l.tail);
    return *this;
  }

  
  

  /**
          \param [in] link New element that will be inserted

     */
  void insert(Slinknc * link) noexcept
  {
    assert(NEXT(link) == nullptr);

    if (head == nullptr)
      {
        assert(tail == nullptr);
        head  = tail = link;
        return;
      }

    NEXT(link) = head;
    head = link;
  }

  
  

  /**
          \param [in] link New element that will be inserted

     */
  void append(Slinknc * link) noexcept
  {
    assert(link != nullptr and NEXT(link) == nullptr);

    if (head == nullptr)
      {
        assert(tail == nullptr);
        head = tail = link;
        return;
      }

    NEXT(tail) = link;
    tail = link;
  }

  
  

  /**
          \param [in] l List that will be inserted through list end

     */
  void append(HTList & l) noexcept
  {
    if (l.is_empty())
      return;

    if (this->is_empty())
      {
        this->swap(l);
        return;
      }

    NEXT(tail) = l.head;
    tail = l.tail;
    l.head = l.tail = nullptr;
  }

  
  
  void put(Slinknc * link) noexcept { append(link); }

  //!Concat to 'this' all 'l' list; 'l' becomes empty
  
  void concat(HTList & l) noexcept { append(l); }

      /// \overload concat(HTList & l)
  void concat_list(HTList & l) noexcept { append(l); }

  //!Insert starting in link (contained in 'this' list) the
  
  

  /**
          Insert list `l` starting from `this`. After insertion, `l` becomes empty.

     \param [in,out] l List that will be inserted starting from `this`.
                        After the call, `l` is empty.

     */
  void insert(HTList & l) noexcept
  {
    l.append(*this);
    this->swap(l);
  }

  /**
      Insert a `list` into `this` after one of its items.

      `insert(link, list)` assumes that `link` points to a item of
      `this` and inserts in constant time `list` just after the item
      pointed by `link`. The order of `list` is not altered. but `list`
      becomes empty after insertion.

      Suppose the following situation

          this-->t1-->t2-->t3-->t4-->t5-->t6-->t7-->t8
                          ^
        |
        link

    l-->l1-->l2-->l3-->l4

      Then `insert(link, list)` produces:

          this-->t1-->t2-->t3-->t4-->l1-->l2-->l3-->l4-->t5-->t6-->t7-->t8

    l becomes empty

      \param[in] link to the item after one wants to insert `list`
      \param[in] list to be inserted after item pointed by `link`

  */
  void insert(Slinknc * link, HTList & list) noexcept
  {
    if (list.is_empty())
      return;

    Slinknc * link_next = NEXT(link); // save what was after link
    NEXT(link) = list.head;           // link now points to list's head
    NEXT(list.tail) = link_next;      // list's tail points to what was after link

    if (link == tail)                 // if link was the tail, update tail
      tail = list.tail;

    list.head = list.tail = nullptr;
  }

  //!It deletes head element (first one). Return deleted
  
  
  Slinknc * remove_head_ne() noexcept
  {
    Slinknc * ret_val = head;
    if (head == tail)
      head = tail = nullptr;
    else
      {
        head = NEXT(head);
        if (head == nullptr)
          tail = nullptr;
      }

    ret_val->reset();

    return ret_val;
  }

  Slinknc * remove_head()
  {
    ah_underflow_error_if(is_empty())
      << "HTList is empty";
    return remove_head_ne();
  }

  /**
          Remove the header item from the list.

      \overload remove_head()

      \return a pointer to the removed item
      \throw underflow_error if the list is empty

    */
  Slinknc * remove_first_ne() noexcept { return remove_head_ne(); }

  Slinknc * remove_first() { return remove_head(); }

  /**
           Remove from the list the item pointed by `link`

      `remove(link)` perform a sequential traversal of `this` until find
      `link`. Then `link` is removed.

      This method has a complexity of \f$O(n)\f$ for worst and average
      case.

      \param[in] link pointer to the item to be removed.
      \return `true` if `link` was found and removed: `false` otherwise
      \throw underflow_error if the list is empty

    */
  bool remove(Slinknc * link)
  {
    ah_underflow_error_if(is_empty())
      << "Removing from a empty list";

    if (link == head)
      {
        head = NEXT(head);
        if (head == nullptr)
          tail = nullptr;
        link->reset();
        return true;
      }

    for (Slinknc * prev = head, * p = NEXT(head); p != nullptr;
         prev = p, p = NEXT(p))
      if (p == link)
        {
          NEXT(prev) = NEXT(p);
          if (link == tail)
            tail = prev;
          link->reset();
          return true;
        }

    return false;
  }

  
  
  void push(Slinknc * link) noexcept { insert(link); }

  /**
          Remove the first item of `this`.

     \overload remove_head()

     This synonym is adequate when `this` is dealed as a stack.

     \return a pointer to the removed item.
     \throw underflow_error if the list is empty

     */
  Slinknc * pop()
  {
    ah_underflow_error_if(is_empty())
      << "HTList as stack is empty";
    return remove_head();
  }

  Slinknc * top() const
  {
    ah_underflow_error_if(is_empty())
      << "HTList as stack is empty";
    return get_first();
  }

 //!It divides 'this' into two equal lists without modifying
 
 

  /**
    Split the list in two.

      This method takes the first n/2 items of `this` and puts them, in
      the same order, in list `l`. The remainder items are put in list
      `r`. After operation `this` becomes empty. The order of items is
      preserved through `l` and `r`.

      \param[out] l list containg the first n/2 first items
      \param[out] r list containg the last n/2 first items
      \return the total of items that has `this`

    */
  size_t split_list(HTList & l, HTList & r) noexcept
  {
    assert(l.is_empty() and r.is_empty()); // l y r deben estar vacías

    if (is_empty())
      return 0;

    if (is_unitarian())
      {
        swap(l);
        return 1;
      }

    size_t count = 0;
    Slinknc * p = head;
    Slinknc * q = head;
    while (q != tail and q != nullptr)
      {
        q = NEXT(q); ++count;
        if (q == tail or q == nullptr)
          break;

        p = NEXT(p);
        q = NEXT(q); ++count;
      }

    Slinknc * r_head = NEXT(p);

    l.head = head;
    l.tail = p;

    r.head = r_head;
    r.tail = r_head == nullptr ? nullptr : tail;

    NEXT(l.tail) = nullptr;

    head = tail = nullptr;

    return count;
  }

  size_t split_list_ne(HTList & l, HTList & r) noexcept
  {
    return split_list(l, r);
  }

      /// \overload split_list(HTList & l, HTList & r)
  size_t split(HTList & l, HTList & r) noexcept
  {
    return split_list(l, r);
  }

  //!It inverts all list elements. It returns list size
  
  
  size_t reverse() noexcept
  {
    HTList tmp;

    size_t count = 0;
    while (not is_empty())
      {
        tmp.insert(remove_first_ne());
        ++count;
      }

    swap(tmp);

    return count;
  }

      /// \overload reverse()
  size_t reverse_list() noexcept
  {
    return reverse();
  }

  //!It cuts 'this' over 'link' element and it puts all
  
  
  //!list

  /**
          \brief  It makes reference to is_empty(). <br>
     Referenced by cut_list().
     \param [in] link Element where 'list' will be cut.
     \param [in] list List that will be cut in link

     */
  void cut(Slinknc * link, HTList & list) noexcept
  {
    assert(list.is_empty());

    list.head = NEXT(link);
    list.tail = list.head == nullptr ? nullptr : tail;

    tail = link;
    NEXT(link) = nullptr;
  }

  /// \overload cut(Slinknc * link, HTList & list)
  void cut_list(Slinknc * link, HTList & list) noexcept { cut(link, list); }

  /**
          Remove and free memory for all the items of list.

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
      delete remove_head_ne();
  }

  /**
          Iterator on HTList.

      \ingroup Sequences

    */
  class Iterator
  {
    HTList * lptr = nullptr;
    Slinknc * curr = nullptr;
    Slinknc * prev = nullptr;

    long pos = 0;

  public:

    Iterator() noexcept = default;

    //!Initialize iterator on at the first item of `list`
    
    //!`list`
    Iterator(const HTList & list) noexcept
      : lptr(& const_cast<HTList &>(list)), curr(lptr->head), prev(curr)
    { /* empty */ }

    //!Reset the iterator at the first item
    
    void reset() noexcept
    {
      prev = curr = lptr->head;
      pos = 0;
    }

    
    
    long get_pos() const noexcept { return pos; }

    /// \overload reset()
    void reset_first() noexcept { reset(); }

    /// It has O(n) of performance
    void reset_last()
    {
      reset();
      if (lptr->is_empty())
        return;

      while (NEXT(curr))
        {
          prev = curr;
          curr = NEXT(curr);
          ++pos;
        }
    }

    //!Set the iterator to its end position, which has not
    
    void end() noexcept
    {
      curr = nullptr;
    }

     
    Iterator(const Iterator &) = default;

     
    Iterator & operator = (const Iterator & it) noexcept
      {
        lptr = it.lptr;
        curr = it.curr;
        prev = it.prev;
        pos  = it.pos;
        return *this;
      }

    
    
    bool has_curr() const noexcept { return curr != nullptr; }

    bool is_last() const noexcept
    {
      return lptr->is_empty() ? false : curr == lptr->tail;
    }

        ///  Return `true` if the iterator is positioned on the first item
    bool is_in_first() const noexcept
    {
      return lptr->is_empty() ? false : curr == lptr->head;
    }

        ///  Return `true` if the iterator is positioned on the last item
    bool is_in_last() const noexcept { return is_last(); }

    /**
        Return the current node on which the iterator is positioned

    @throw overflow_error if iterator has not current node

    */
    Slinknc * get_curr() const
    {
      ah_overflow_error_if(curr == nullptr)
        << "Iterator overflow";

      return curr;
    }

    /// Return the current link guaranteeing no exception. Be careful.
    Slinknc * get_curr_ne() const noexcept { return curr; }

    /// Move the iterator one position forward guaranteeing no
    /// exception. Be careful.
    void next_ne() noexcept
    {
      if (curr == lptr->head) // on the first item?
        {
          assert(prev == lptr->head);
          curr = NEXT(curr);
        }
      else if (curr == lptr->tail) // on the last item?
        {
          assert(NEXT(prev) == curr);
          curr = nullptr;
        }
      else
        {
          assert(NEXT(prev) == curr);
          prev = curr;
          curr = NEXT(curr);
          assert(NEXT(prev) == curr);
        }
      pos++;
    }

    /**
          Move the iterator one item forward. Throw overflow_error if
      iterator has not current item

    */

    void next()
    {
      ah_overflow_error_if(not has_curr())
        << "Iterator overflow";
      next_ne();
    }

    Slinknc * del_ne() noexcept
    {
      Slinknc * ret_val = nullptr;
      if (curr == lptr->head) // first item removal
        {
          ret_val = lptr->remove_first();
          prev = curr = lptr->head;
        }
      else if (curr == lptr->tail) // last item removal
        {
          assert(NEXT(prev) == curr);
          ret_val = curr;
          NEXT(prev) = NEXT(curr);
          lptr->tail = prev;
          curr = nullptr; // put in overflow
        }
      else
        {
          assert(NEXT(prev) == curr);
          ret_val = curr;
          NEXT(prev) = NEXT(curr);
          curr = NEXT(curr);
        }

      ret_val->reset();
      return ret_val;
    }

    /**
              Remove the current item.

       `del()` removes the current item of iterator and advaces it a
       position forward.

       \return a valid pointer to the removed item
       \throw overflow_error if iterator has not current item

       */
    Slinknc * del()
    {
      ah_overflow_error_if(not has_curr())
        << "Iterator overflow";

      return del_ne();
    }
  };

  /**
      
      Count the number of elements of the list.

      This method counts, it does not retrieve, the number of elements
      stored in the list.

      So it is complexity is \f$O(n)\f$. This is some polemic because one
      could maintain an internal counter and retrieve it in constant
      time. It possible that this feauture is put in next versions. We
      do not maintain this counter because it is possible to add or
      remove items from a given node. So these operations would
      require access to the full list's context, what it often not the
      case.

      \return the number of items of list

      */
  size_t size() const noexcept
  {
    size_t count = 0;
    for (Iterator it(*this); it.has_curr(); it.next_ne())
      ++count;

    return count;
  }

  /**
      Rotate to left the list n positions.

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
        ah_domain_error() << "List is empty";
        return; // unreachable, silences -Wreturn-type
      }

    for (size_t i = 0; i < n; ++i)
      append(remove_first());
  }
};

    /** @brief Dynamic singly linked list with functional programming support.

       DynList<T> is a versatile singly linked list that combines efficient
       list operations with a rich set of functional programming methods
       inherited from the FunctionalMethods mixin.

       This is one of the most commonly used containers in Aleph-w, providing:
       - O(1) insertion at head and tail
       - O(1) removal at head
       - O(n) traversal with iterators
       - Comprehensive functional operations (map, filter, fold, etc.)

       @tparam T The type of elements stored in the list (default: int).

       @par Complexity:
       - insert() / append(): O(1)
       - remove_first(): O(1)
       - size(): O(n) - computed by traversal
       - Access by index: O(n)

       @par Example:
       @code
       DynList<int> list = {1, 2, 3, 4, 5};

       // Functional operations
       auto doubled = list.maps<int>([](int x) { return x * 2; });
       auto evens = list.filter([](int x) { return x % 2 == 0; });
       int sum = list.foldl(0, [](int acc, int x) { return acc + x; });

       // Iteration
       for (const auto& item : list)
         std::cout << item << " ";
       @endcode

       @see HTList Base class providing head-tail structure.
       @see DynDlist Doubly linked list alternative.
       @see FunctionalMethods Inherited functional operations.
       @author Alejandro Mujica
       @author Leandro Rabindranath León
       @ingroup Sequences
    */
    template <typename T = int>
class DynList : public HTList,
                public GenericTraverse<DynList<T>>,
                public LocateFunctions<DynList<T>, T>,
                public SpecialCtors<DynList<T>, T>,
                public FunctionalMethods<DynList<T>, T>,
                public GenericItems<DynList<T>, T>,
                public EqualSequenceMethod<DynList<T>>,
                public StlAlephIterator<DynList<T>>
{
  using CtorBase = SpecialCtors<DynList<T>, T>;

  using CtorBase::CtorBase;

 public:

  
  using Item_Type = T;

  
  using Key_Type = T;

  Slinknc * get_head() const noexcept = delete;
  Slinknc * get_tail() const noexcept = delete;

  /**
          Swap this with l. All items of each list are swapped in constant
     time

     */
  DynList & swap(DynList & l) noexcept
  {
    return static_cast<DynList &>(HTList::swap(l));
  }

  
  /// Initialize an empty list
  DynList() { /* empty */ }

  /// Initialize a list with a copy of all the items of list `l`
  DynList(const DynList & l) : HTList(), CtorBase(l) {}

private:

  T & __insert(Snodenc<T> * p) noexcept
  {
    static_assert(std::is_copy_constructible_v<T> or
                  std::is_move_constructible_v<T>,
                  "No copy assign for T");
    HTList::insert(p);
    return p->get_data();
  }

  T & __append(Snodenc<T> * p) noexcept
  {
    static_assert(std::is_copy_constructible_v<T> or
                  std::is_move_constructible_v<T>,
                  "No copy assign for T");
    HTList::append(p);
    return p->get_data();
  }

public:

  /**
            Insert a new item by copy.

      Allocate memory for a new item, copy and insert into the list as
      the first item.

      \return a modifiable reference to the item in the list
      \throw  `bad_alloc` if there is no enough memory

      */
  T & insert(const T & item)
  {
    return __insert(new Snodenc<T> (item));
  }

  /**
      Insert a new item by movement.

      Allocate memory for a new item, move the item and insert into the
      list as the first item. This operation can be faster that the
      equivalent with copy

      \return a modifiable reference to the item in the list
      \throw  `bad_alloc` if there is no enough memory
   */
  T & insert(T && item)
  {
    return __insert(new Snodenc<T> (std::forward<T>(item)));
  }

  /// \overload insert(const T & item)
  T & push(const T & item)
  {
    return insert(item);
  }

  /// \overload insert(T && item)
  T & push(T && item) { return insert(std::forward<T>(item)); }

  /// \overload insert(const T & item)
  T & put(const T & item)
  {
    return push(item);
  }

  /// \overload insert(T && item)
  T & put(T && item)
  {
    return push(std::forward<T>(item));
  }

  /**
          Append a new item by copy.

      Allocate memory for a new item, copy and append it at the end of
      the list.

      \return a modifiable reference to the item in the list
      \throw  `bad_alloc` if there is no enough memory

    */
  T & append(const T & item)
  {
    return __append(new Snodenc<T> (item));
  }

  /**
          Append a new item by movement.

      Allocate memory for a new item, move to its allocated place and
      append it at the end of the list.

      \return a modifiable reference to the item in the list
      \throw  `bad_alloc` if there is no enough memory

    */
  T & append(T && item)
  {
    return __append(new Snodenc<T> (std::forward<T>(item)));
  }

  /** Remove the first item of the list without exception.

      \return a copy of removed item
      \throw underflow_error if the list is empty
  */
  T remove_ne() noexcept
  {
    Slinknc * l = this->HTList::remove_head_ne();
    auto * p = static_cast<Snodenc<T>*> (l);
    T ret_val = std::move(p->get_data());
    delete p;

    return ret_val;
  }

  /** Remove the first item of the list.

      \return a copy of removed item
      \throw underflow_error if the list is empty
  */
  T remove()
  {
    Slinknc * l = this->HTList::remove_head();
    auto * p = static_cast<Snodenc<T>*> (l);
    T ret_val = std::move(p->get_data());
    delete p;

    return ret_val;
  }

  /// \overload remove()
  T remove_first_ne() noexcept
  {
    return remove_ne();
  }

  /// \overload remove()
  T remove_first()
  {
    return remove();
  }

  /// \overload remove()
  T pop() { return remove_first(); }

  /// \overload remove()
  T get() { return remove(); }

  /** Return the last item of the list without exception.

      \return a modifiable reference to the last item
  */
  T & get_last_ne() const noexcept
  {
    auto * p = static_cast<Snodenc<T>*> (this->HTList::get_last());
    return p->get_data();
  }

  /** Return the first item of the list without exception

      \return a modifiable reference to the first item
  */
  T & get_first_ne() const noexcept
  {
    return static_cast<Snodenc<T>*>(this->HTList::get_first())->get_data();
  }

  /** Return the last item of the list

      \return a modifiable reference to the last item
      \throw underflow_error if the list is empty
  */
  T & get_last() const
  {
    ah_underflow_error_if(is_empty())
      << "List is empty";
    return get_last_ne();
  }

  /** Return the first item of the list

      \return a modifiable reference to the first item
      \throw underflow_error if the list is empty
  */
  T & get_first() const
  {
    ah_underflow_error_if(is_empty())
      << "List is empty";
    return get_first_ne();
  }

  /// \overload get_first()
  T & top() const
  {
    return get_first();
  }

      /// empty the list
  void empty() noexcept
  {
    if (is_empty())
      return;

        // a very fast deletion
    auto * last = static_cast<Snodenc<T>*> (this->HTList::get_last());
    auto * p = static_cast<Snodenc<T>*> (this->HTList::get_first());
    while (p != last)
      {
        Snodenc<T> * q = p;
        p = p->get_next();
        delete q;
      }
    delete p;
    reset();
  }

  /** @brief Empties the container. 
      
      @throws none
      @see empty()
  */
  void clear() noexcept { empty(); }

  ~DynList() { empty(); }

  /** Iterator on the items of list

      \ingroup Sequences
   */
  class Iterator : public HTList::Iterator
  {
  public:

    using Item_Type = T; /// The type of elements of list

    using Iterator_Type = Iterator;

    using Set_Type = DynList; /// The type of container

    Iterator() noexcept = default;

    /// Initiliaze an iterator on the first item of `list`
    Iterator(const DynList & list) noexcept
      : HTList::Iterator(list) { /* empty */ }

        /// Return the current link guaranteeing no exception. Be careful.
    T & get_curr_ne() const noexcept
    {
      return static_cast<Snodenc<T> *>(HTList::Iterator::get_curr_ne())->get_data();
    }

    /** Return the current item

        \return a modifiable reference to the current item
        \throw overflow_error if the iterator has not current item
    */
    T & get_curr() const
    {
      return static_cast<Snodenc<T> *>(HTList::Iterator::get_curr())->get_data();
    }

    /** Remove the current item of the iterator.

        `del()` removes the current item and advance forward the
        iterator to the next item.

        \return a moved copy of removed item (moved because the original
        will be destroyed)
        \throw overflow_error if the iterator has not current item
    */
    T del()
    {
      auto * p = static_cast<Snodenc<T> *>(this->HTList::Iterator::del());
      T ret_val = std::move(p->get_data());
      delete p;
      return ret_val;
    }
  };

  DynList & reverse() noexcept
  {
    reverse_list();
    return *this;
  }

  DynList & rev() noexcept { return reverse(); }

  /// Return a reversed copy of `this`. Not confuse with reverse
  /// without `const` which mutates `this`
  DynList reverse() const
  {
    DynList ret;
    for (auto it = this->get_it(); it.has_curr(); it.next())
      ret.insert(it.get_curr());

    return ret;
  }

  DynList rev() const { return reverse(); }

  template <class Equal = std::equal_to<T>>
  T remove_ne(Equal eq) noexcept
  {
    for (Iterator it(*this); it.has_curr(); it.next_ne())
      {
        const T & item = it.get_curr_ne();
        if (not eq(item))
          continue;
        T ret = item; // performs a copy
        auto * removed = static_cast<Snodenc<T> *>(it.del_ne());
        delete removed;
        return ret;
      }
    return T(); // not found, return default constructed T
  }

  /** Remove the first element matching an equality criteria.

      `remove(eq)` sequentially traverses the list and on each current
      element `curr` it calls to `eq(curr)`. If the test is `true`,
      then `curr` is removed from the list and a copy of `curr` is
      returned. Otherwise, if no element matches with `eq()` test,
      then exception `domain_error` is thrown.

      The `eq()` must match the following signature:

          bool eq(const T& op)

      This test would return `true` when `op` satisfies the finding
      criteria`.

      Since that this method is overloaded, you could define several
      flavors: a more sophisticated functor, function pointers or
      lambdas.

      \param[in] eq equality test
      \return a copy of removed element if the item was found and
      removed; otherwise the exception `domain_error` is thrown.
   */
  template <class Equal = std::equal_to<T>> T remove(Equal eq)
    {
      for (Iterator it(*this); it.has_curr(); it.next_ne())
          if (const T & item = it.get_curr(); eq(item))
            {
              T ret = item; // performs a copy
              auto * removed = static_cast<Snodenc<T> *>(it.HTList::Iterator::del());
              delete removed;
              return ret;
            }

      ah_domain_error() << "DynList::remove(Equal &): item not found";
      return T(); // unreachable, silences -Wreturn-type
    }

  /** Initialize the list with all the items of `l` moved to `this`.

      This constructor copies (by moving) all the items of list
      `l`. Consequently, no copy is done and the construction takes
      \f$O(1)\f$.

      \note Since `l` is a rvalue reference, take care of that if you are
      interested in to avoid the copy. So, if you have a lvalue
      reference to a list, use `std::move()`, upon your responsibility,
      if and only if you are absolutely sure that the list will not be
      needed after.

      \param[in] l a rvalue containing a entire list.
  */
  DynList(DynList && l) noexcept
  {
    swap(l);
  }

  /** Assign to this a copy of `l`

      First, the list is emptied (all its elements are deleted). Then
      sequential copy of each item of `l` is inserted into `this`.

      This assignation takes \f$O(|l|)\f$ in complexity.

      \param[in] l list whose items want to be inserted into `this`
      \throw  `bad_alloc` if there is no enough memory
   */
  DynList & operator = (const DynList & l)
  {
    if (&l == this)
      return *this;

    empty();

    for (typename DynList::Iterator it(l); it.has_curr(); it.next_ne())
      append(it.get_curr());

    return *this;
  }

  /** Assign to `this` by movement the list `l`

      This assignation swaps the content of `this` with the content of
      `l`. This swapping is done in \f$O(1)\f$.

      \note Since `l` is a rvalue reference, take care of that if you
      are interested in to avoid the copy. So, if you have a lvalue
      referenced to a list, use `std::move()`, upon your responsibility,
      if and only if you are absolutely sure that the list will not be
      needed after.

      \param[in] l a rvalue reference to the list to be assigned by
      movement.
   */
  DynList & operator = (DynList && l) noexcept
  {
    return static_cast<DynList &>(this->swap(l));
  }

  /** Append `list `at the end of `this` by movement.

      `append(l)` (in this rvalue version), puts in constant time the
      items of `l` at the end of `this`. After calling `l` becomes
      empty.

      \note Since `l` is a rvalue reference, take care of that if you
      are interested in to avoid the copy. So, if you have a lvalue
      reference to a list, use `std::move()`, upon your responsibility,
      if and only if you are absolutely sure that the list will not be
      needed after.

      \param[in] list a rvalue reference to the list to be appended
  */
  DynList & append(DynList && list) noexcept
  {
    HTList::append(list);
    return *this;
  }

  /** Insert `list `at the beginning of `this` by movement.

      `insert(l)` (in this rvalue version), puts in constant time the
      items of `l` at the beginning of `this`. After calling `l` becomes
      empty.

      \note Since `l` is a rvalue reference, take care of that if you
      are interested in to avoid the copy. So, if you have a lvalue
      reference to a list, use `std::move()`, upon your responsibility,
      if and only if you are absolutely sure that the list will not be
      needed after.

      \param[in] list a rvalue reference to the list to be inserted
  */
  DynList & insert(DynList && list) noexcept
  {
    HTList::insert(list);
    return *this;
  }

  /** Append to `this` a copy of `list`.

      This version of `append(l)` traverses the items of `l` and appends
      them (copies) into `this`.

      \param[in] list list to be copied at the end
      \throw bad_alloc if there is no enough memory
   */
  DynList & append(const DynList & list)
  {
    DynList copy = list;
    HTList::append(copy);
    return *this;
  }

  /** Insert to `this` a copy of `list`.

      This version of `insert(l)` traverses the items of `l` and insert
       a copy into `this`.

      \param[in] list list to be copied at the end
      \throw bad_alloc if there is no enough memory
   */
  DynList & insert(const DynList & list)
  {
    DynList tmp = list;
    HTList::insert(tmp);
    return *this;
  }

  /// Obtains a modifiable reference to the i-th item of `this`. Throws
  /// `overflow_error` if `i` is greater or equal to number of elements
  T & get(const size_t i)
  {
    Iterator it(*this);

    for (size_t __i = 0 ; it.has_curr() and __i < i; it.next_ne(), ++__i);

    return it.get_curr();
  }

  /// Array-style access to the i-th item. Equivalent to `get(i)`.
  T & operator [] (const size_t & i)
  {
    return get(i);
  }
};

template <class Container>
inline DynList<typename Container::Item_Type> to_dynlist(const Container & c)
{
  return c.template maps<typename Container::Item_Type>([] (const auto & d)
							{
							  return d;
							});
}

  template <typename T> inline
DynList<T> get_unitarian_DynList(const T & item)
{
  DynList<T> ret;
  ret.append(item);
  return ret;
}

    template <typename T> inline
DynList<T> unitarian_DynList(const T & item)
{
  return DynList<T>({item});
}

# undef NEXT

} // end namespace Aleph


# endif // HTLIST_H
