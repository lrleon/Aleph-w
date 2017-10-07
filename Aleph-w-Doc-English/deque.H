
/*
  This file is part of Aleph-w system

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014
  Leandro Rabindranath León
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met: 

  1. Redistributions of source code must retain the above copyright 
     notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright 
     notice, this list of conditions and the following disclaimer in the 
     documentation and/or other materials provided with the distribution.

  3. All advertising materials mentioning features or use of this software
     must display the following acknowledgement:

     Copyright (c) 2002-2014 Leandro Rabindranath León. See details of 
     licence.     

     This product includes software developed by the Hewlett-Packard
     Company, Free Software Foundation and Silicon Graphics Computer
     Systems, Inc. 

  4. Neither the name of the ULA nor the names of its contributors may
     be used to endorse or promote products derived from this software
     without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY Leandro Rabindranath León ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Aleph-w is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.

  I request users of this software to return to 

  Leandro Rabindranath Leon
  CEMISID 
  Ed La Hechicera 
  3er piso, ala sur
  Facultad de Ingenieria 
  Universidad de Los Andes 
  Merida - REPÚBLICA BOLIVARIANA DE VENEZUELA    or

  leandro.r.leon@gmail.com

  any improvements or extensions that they make and grant me the rights
  to redistribute these changes.  
*/

/**
  @file deque.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# include <Vector.H>

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
      /* Empty */ 
    }

    void set_pos(int pos)
    {
      current_position = num_elem;
    }

    int get_position() const { return current_position; }

    void verify_array(DynArray<Key> * array_ptr) const
    {
      if (array_ptr not_eq dyn_array_ptr)
	throw std::domain_error("Iterator is not set to same array");
    }

    DynArray<Key> * get_dyn_array_ptr() { return dyn_array_ptr; }

  public:

    iterator() 
      : dyn_array_ptr(NULL), current_position(Invalid_Position) 
    { 
      /* Empty */ 
    }

    iterator(const iterator& itor)
      : dyn_array_ptr(itor.dyn_array_ptr), 
	current_position(itor.current_position) 
    { 
      /* Empty */ 
    }
  
  } ;

  deque() : array(0), num_elem(0) { /* Empty */}

  deque (const deque& c) : array(c.array), num_elem(c.num_elem)

    /* Empty */
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

  ~deque() { /* Empty */ }

  void push_front(const Key& value)
  {
    insert(begin(), value);
  }

  void pop_front()
  {
    erase(begin());
  }

};
