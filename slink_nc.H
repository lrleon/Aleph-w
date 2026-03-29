
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


/** @file slink_nc.H
 *  @brief Singly linked node (no check variant).
 *  @ingroup Sequences
 *  @author Leandro Rabindranath León
 */

# ifndef SLINK_NC_H
# define SLINK_NC_H

# include <aleph.H>
# include <ah-errors.H>

namespace Aleph
{
  /** Singly linked list node (non-circular).

      Slink_Nc models a pointer to a node within a non-circular 
      singly linked list.

      @ingroup Sequences
      @see Snode Slist DynSlist Dlink 
   */
  class Slink_Nc
  {
  protected:
    Slink_Nc * next;
  
  public:
    /// Empty constructor.
    Slink_Nc() : next(nullptr) { /* empty */ }

    /// Copy constructor; sets the next link to nullptr.
    Slink_Nc(const Slink_Nc &) : next(nullptr) { /* empty */ }

    ~Slink_Nc() { /* empty */ }

    /// Assignment; sets the next link to nullptr.
    Slink_Nc & operator = (const Slink_Nc & link)
    {
      if (&link == this)
        return *this;

      ah_invalid_argument_if(not is_empty()) << "link is not empty";

      next = nullptr;

      return *this;
    }

    /// Resets the link to nullptr.
    void reset()
    {
      assert(this not_eq nullptr);
      next = nullptr;
    }

    /// Returns true if this is empty (points to nullptr).
    bool is_empty() const
    {
      assert(this not_eq nullptr);
      return next == nullptr;
    }

    /// Returns the next link.
    Slink_Nc *& get_next()
    {
      assert(this not_eq nullptr);
      return next;
    }

    /** @brief Returns the next link (constant version).
        @return a constant pointer to the next link.
        @throws none
    */
    const Slink_Nc * get_next() const
    {
      assert(this not_eq nullptr);
      return next;
    }

    /** Inserts link p after this.

          insert(p) inserts a simple node whose link is p after
          the node whose link is this.

          @param[in] p the simple link to insert.
       */
    void insert(Slink_Nc * p)
    {
      assert(this not_eq nullptr);
      assert(p not_eq nullptr);
      assert(p->is_empty());
      p->next = next;
      next = p;
    }

    /** Removes the link following this.

          remove_next() removes the simple node situated after
          the node whose link is this and returns the address of the
          removed link.

          @return the removed link.
       */
    Slink_Nc * remove_next()
    {
      assert(this not_eq nullptr);
      Slink_Nc * ret_val = next;
      next = ret_val->next;
      ret_val->reset();
      return ret_val;
    }

    class Iterator
    {
      Slink_Nc * head;
      Slink_Nc * curr;
    
    public:
      
      Iterator() : head(nullptr), curr(nullptr) { /* Empty */ }

      Iterator(Slink_Nc * head_ptr) : head(head_ptr), curr(head->get_next())
      { 
        // empty
      }

      Iterator(Slink_Nc & _head) : head(&_head), curr(head->get_next())
      { 
        // empty
      }

      Iterator(Slink_Nc * head_ptr, Slink_Nc * curr_ptr)
        : head(head_ptr), curr(curr_ptr)
      {
        // empty
      }

      void reset_first() /// Resets iterator to the first node of the list.
      {
        assert(curr != nullptr and head != nullptr);
        curr = head->get_next();
      }

      void set(Slink_Nc * new_curr) 
      {
        assert(curr != nullptr and head != nullptr);
        curr = new_curr;
      }

      void reset(Slink_Nc * new_head)
      {
        head = new_head;
        curr = head->get_next();;
      }

      bool has_curr() const 
      {
        assert(head != nullptr);
        return curr != nullptr;
      }

      /// Returns current node address.
      Slink_Nc * get_curr_ne() const noexcept
      {
        assert(curr != nullptr and head != nullptr);
        return curr;
      }

      Slink_Nc * get_curr() const
      {
        ah_overflow_error_if(not has_curr()) << "Not element in list";
        return get_curr_ne();
      }

      /// Returns true if iterator is on the first element.
      bool is_in_first() const { return curr == head->next; }

      /// Advances iterator by one position.
      void next_ne() noexcept
      {
        curr = curr->get_next();
      }

      void next()
      {
        ah_overflow_error_if(not has_curr()) << "Not next element in list";
        next_ne();
      }

      /// Returns true if iterators are on the same element.
      bool operator == (const Iterator & it) const { return curr == it.curr; }

      /// Returns true if iterators have different states.
      bool operator != (const Iterator & it) const { return curr != it.curr; }
    
      bool verify(Slink_Nc * l) const { return head == l; }

      bool verify(const Iterator & it) const { return head == it.head; }
    };
    
  };
}

# endif
