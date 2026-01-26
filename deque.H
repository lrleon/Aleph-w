
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


/** @file deque.H
 *  @brief Double-ended queue implementation.
 *
 *  Provides deque class supporting efficient insertion and removal
 *  at both ends, implemented using Aleph's Vector.
 *
 *  @ingroup Sequences
 *  @author Leandro Rabindranath León
 */

# include <Vector.H>
 # include <ah-errors.H>

template <class Key>
class deque : public vector<Key>
{
private:
  DynArray<Key> array;

  typedef Key value_type;
  typedef deque::value_type& reference;
  typedef const deque::value_type& const_reference;
  typedef unsigned int size_type;

  size_type num_elem;

public:
  friend class iterator : public vector<Key>::iterator
  {
  private:

    friend class deque<Key>;
    
    static const int Invalid_Position = -1;
    
    DynArray<Key> * dyn_array_ptr;
    
    size_type current_position;
    
    iterator(deque<Key> & vec, int pos = 0)
      :  dyn_array_ptr(&vec.array), current_position(pos) 
    { 
      /* empty */ 
    }

    void set_pos(int pos)
    {
      current_position = num_elem;
    }

    int get_position() const { return current_position; }

    void verify_array(DynArray<Key> * array_ptr) const
    {
      ah_domain_error_if(array_ptr not_eq dyn_array_ptr)
        << "Iterator is not set to same array";
    }

    DynArray<Key> * get_dyn_array_ptr() { return dyn_array_ptr; }

  public:

    iterator() 
      : dyn_array_ptr(nullptr), current_position(Invalid_Position) 
    { 
      /* empty */ 
    }

    iterator(const iterator& itor)
      : dyn_array_ptr(itor.dyn_array_ptr), 
	current_position(itor.current_position) 
    { 
      /* empty */ 
    }
  
  } ;

  deque() : array(0), num_elem(0) { /* empty */}

  deque (const deque& c) : array(c.array), num_elem(c.num_elem)
  {
    // empty
  }

  deque (size_type num) : array(num), num_elem(num)
  {
    array.reserve(0, num_elem);
  }

  deque (size_type num, const Key& value) : array(num), num_elem(num)
  {
    array.reserve(0, num_elem);
        
    for(size_type i = 0; i < num; i++)
      array.access(i) = value;
  }

  deque (iterator beg, iterator end) : array(0), num_elem(end - beg)
  {
    array.reserve(0, num_elem);
    
    for (int i = 0; beg < end; i++, beg++, num_elem++)
      array.access(i) = *beg;
  }

  ~deque() { /* empty */ }

  void push_front(const Key& value)
  {
    insert(begin(), value);
  }

  void pop_front()
  {
    erase(begin());
  }

};
