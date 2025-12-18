
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
