
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


/** @file tpl_dyn_slist_nc.H
 *  @brief Dynamic singly linked list (no check).
 *  @ingroup Sequences
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_DYN_SLIST_NC_H
# define TPL_DYN_SLIST_NC_H

# include <tpl_snode_nc.H>
# include <ah-errors.H>

namespace Aleph
{
  /** @brief Dynamic list of elements of type @p T.

      Dyn_Slist_Nc<T> defines a dynamic list, which is a sequence of elements
      of some type @p T.

      This type can be used as a stack or a queue.

      @tparam T type of elements in the list. Semantic requirements for T:
        - T must be @b Destructible and @b CopyConstructible (or @b Copyable) 
          because operations like insert(), append(), copy construction,
          assignment of the Dyn_Slist_Nc<T> container, and element destruction
          rely on these properties.
        - Move-only types are currently not supported.
        - Destructors of T should ideally be @b noexcept.

      @see Slink, Snode_Nc, Dyn_Slist_Nc::Iterator

      @ingroup Sequences

      @author Leandro R. León
      @author Alejandro J. Mujica
   */
  template <typename T> class Dyn_Slist_Nc : public Snode_Nc <T>
  {
  public:
    typedef Snode_Nc <T> Node;

  private:
    Node * head;
    size_t num_items;

  public:

    /** @brief Empties the container. 
        
        @throws none
    */
    void empty()
    {
      while (not this->is_empty())
        delete Node::remove_next();
      this->reset();
      head = this;
      num_items = 0;
    }

    /** @brief Alias for empty(). 
        
        @throws none
        @see empty()
    */
    void clear() noexcept { empty(); }

    /** @brief Insert an element at the beginning of the list.

        Inserts a copy of @p data as the first element of @p this list. 
        After the operation, the first element of the list is @p data.

        @param[in] data the item to be inserted.
        @return reference to the newly inserted item.
        @throw bad_alloc if there is no memory for the new element.
     */
    T & insert(const T & data)
    {
      Node * p = new Node(data);
      Node::insert(p);

      if (num_items++ == 0)
        head = p;

      return p->get_data();
    }

    /** @brief Append an element at the end of the list.

        Inserts a copy of @p data as the last element of @p this list. 
        After the operation, the last element of the list is @p data.

        @param[in] data the item to be appended.
        @return reference to the newly inserted item.
        @throw bad_alloc if there is no memory for the new element.
     */
    T & append(const T & data)
    {
      assert(head not_eq nullptr);
      Node * p = new Node(data);
      head->insert(p);

      head = p;
      ++num_items;
      return p->get_data();
    }

    /** @brief Returns a reference to the first element of the list.
        @return reference to the first element.
        @throw underflow_error if the list is empty.
    */
    T & get_first()
    {
      ah_underflow_error_if(this->is_empty()) << "List is empty";

      return this->get_next()->get_data();
    }

    /** @brief Returns a reference to the first element of the list (const version).
        @return reference to the first element.
        @throw underflow_error if the list is empty.
    */
    const T & get_first() const
    {
      ah_underflow_error_if(this->is_empty()) << "List is empty";

      return this->get_next()->get_data();
    }

    /** @brief Returns a reference to the last element of the list.
        @return reference to the last element.
        @throw underflow_error if the list is empty.
    */
    T & get_last()
    {
      ah_underflow_error_if(this->is_empty()) << "List is empty";

      return head->get_data();
    }

    /** @brief Returns a reference to the last element of the list (const version).
        @return reference to the last element.
        @throw underflow_error if the list is empty.
    */
    const T & get_last() const
    {
      ah_underflow_error_if(this->is_empty()) << "List is empty";

      return head->get_data();
    }

    /** @brief Removes the first element of the list and returns a copy of it.

        @return a copy of the removed element.
        @throw underflow_error if the list is empty.
    */
    T remove_first()
    {
      ah_underflow_error_if(this->is_empty()) << "List is empty";

      Node * p = Node::remove_next();
      T ret_val = p->get_data();
      delete p;
      if (--num_items == 0)
        head = this;
      return ret_val;
    }

    /** @brief If this is a queue, inserts the item @p item. */
    T & put(const T & item) { return append(item); }

    /** @brief If this is a queue, extracts the oldest element. */
    T get() { return remove_first(); }

    /** @brief If this is a queue, returns the youngest element. */
    T & rear() { return get_last(); }

    /** @brief Returns a reference to the youngest element (const version). 
        @return constant reference to the youngest element.
        @throws underflow_error if the list is empty.
    */
    const T & rear() const { return get_last(); }

    /** @brief If this is a queue, returns the oldest element. */
    T & front() { return get_first(); }

    /** @brief Returns a reference to the oldest element (const version).
        @return constant reference to the oldest element.
        @throws underflow_error if the list is empty.
    */
    const T & front() const { return get_first(); }

    /** @brief If this is a stack, inserts the item @p item. */
    T & push(const T & item) { return insert(item); }

    /** @brief If this is a stack, removes the top element. */
    T pop() { return remove_first(); }

    /** @brief If this is a stack, returns the top element. */
    T & top() { return get_first(); }

    /** @brief Returns the top element (const version).
        @return constant reference to the top element.
        @throws underflow_error if the list is empty.
    */
    const T & top() const { return get_first(); }


    /** @brief Returns the number of elements in the list. */
    const size_t & size() const { return num_items; }

    /** @brief Iterator for Dyn_Slist_Nc. */
    class Iterator : public Node::Iterator
    {
    public:
      Iterator(Dyn_Slist_Nc<T> & list) : Node::Iterator(list)
      {
        // empty
      }

      T & get_curr_n() const noexcept
      {
        return Node::Iterator::get_curr()->get_data();
      }

      T & get_curr() const
      {
        return Node::Iterator::get_curr()->get_data();
      }

    };

    /** @brief Default constructor. Creates an empty list. */
    Dyn_Slist_Nc() : Node(), head(this), num_items(0)
    {
      // empty
    }

    /** @brief Copy constructor. */
    Dyn_Slist_Nc(const Dyn_Slist_Nc & l) : Node(l), head(this), num_items(0)
    {
      for (Iterator it(const_cast <Dyn_Slist_Nc &>(l));
           it.has_curr(); it.next_ne())
        append(it.get_curr());
    } 

    /** @brief Destructor. Empties the list. */
    ~Dyn_Slist_Nc()
    {
      empty();
    }

    /** @brief Assignment operator with lvalue semantics.

        Clears all elements from @p this and then copies all elements from @p list.

        @param[in] list the list to be assigned.
        @return reference to @p this.
        @throw bad_alloc if there is no memory.
    */
    Dyn_Slist_Nc & operator = (const Dyn_Slist_Nc & list)
    {
      if (this == &list) 
        return *this;
 
      empty();
    
      assert(this->is_empty());

      for (Iterator it(const_cast<Dyn_Slist_Nc &>(list)); 
           it.has_curr(); it.next_ne())
        append(it.get_curr()); 

      return *this;
    }

    /** @brief Access element at index @p n.
        @param[in] n index of the element.
        @return reference to the element.
    */
    T & operator [] (const size_t & n)
    {
      ah_range_error_if(n >= num_items) << "Index " << n << " out of range";

      Iterator it(*this);
      for (size_t i = 0; i < n; ++i, it.next_ne());

      return it.get_curr();
    }

    /** @brief Access element at index @p n (const version).
        @param[in] n index of the element.
        @return reference to the element.
    */
    const T & operator [] (const size_t & n) const
    {
      ah_range_error_if(n >= num_items) << "Index " << n << " out of range";

      Iterator it(const_cast<Dyn_Slist_Nc&>(*this));
      for (size_t i = 0; i < n; ++i, it.next_ne());

      return it.get_curr();
    }
  };
}

# endif
