
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
  @file pointer_table.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef POINTER_TABLE_H
# define POINTER_TABLE_H

# include <tpl_dynArray.H>

using namespace std;

class Pointer_Table
{
private:

  static const long Null_Index = -1;

  DynArray<void*> pointer_table;

  DynArray<long> free_table;

  int num_pointers;

/** 
  @brief Points to the next available 
*/
  int heap_index;

  long threshold_size;

  long allocate_above_heap()
  {
    if (free_table.size() == 0)
      return Null_Index;

    long ret_val = free_table[free_table.size() - 1];

    assert(pointer_table[ret_val] == NULL);

    free_table.cut(free_table.size() - 1);

    return ret_val;
  }

  void insert_in_free_table(long i)
  {
    assert(i >= 0 and i < heap_index);

    pointer_table[i] = NULL;
    free_table[free_table.size()] = i;
  }

  bool is_valid_index(long i) const { return i < heap_index; }

  bool pointer_matchs_with_index(long i, void * ptr) const 
  {
    assert(is_valid_index(i));

    return pointer_table[i] == ptr;
  }

  bool invariant() const
  {
    if (num_pointers == 0)
      return (heap_index <= threshold_size and free_table.size() == 0 and
	      pointer_table.size() == 0);

    return heap_index > 0 and pointer_table[heap_index - 1] not_eq NULL;
  }

public: 

  Pointer_Table(size_t initial_size = 0) 
    : pointer_table(initial_size), free_table(0),
      num_pointers(0), heap_index(0), threshold_size(initial_size)
  {
    for (int i = 0; i < pointer_table.size(); i++)
      pointer_table[i] = NULL;
  }

  long size() const { return pointer_table.size(); }

  long busies() const { return num_pointers; }

  long frees() const { return free_table.size(); }

  long insert_pointer(void * ptr)
  {
    assert(invariant());
    
    long ret_val = allocate_above_heap();

    if (ret_val == Null_Index)
      ret_val = heap_index++;

    pointer_table[ret_val] = ptr;
    num_pointers++;

    return ret_val;
  }

  void remove_pointer(long i)
  {
    assert(invariant());
    
    if (not is_valid_index(i))
      throw range_error("index out of range");

    if (pointer_table[i] == NULL) 
      throw domain_error("index is not busy");

    if (i == heap_index - 1)
      { /* Bajar el heap todos los disponibles que se puedan */
	pointer_table[heap_index - 1] = NULL;
	while (heap_index > 0 and pointer_table[heap_index - 1] == NULL)
	  {
	    heap_index--;
	    pointer_table[heap_index] = NULL;
	  }
      }
    else
      insert_in_free_table(i);

    if (heap_index <= threshold_size)
      pointer_table.cut(threshold_size);

    num_pointers--;
  }

  void * verify_pointer(long i, void * ptr)
  {
    if (not is_valid_index(i))
      throw range_error("index out of range");

    if (not pointer_matchs_with_index(i, ptr))
      throw domain_error("pointer does not match");

    return ptr;
  }

# ifdef DEBUG
  void print_parameters()
  {
    cout << "Number of pointers = " << num_pointers << endl
	 << "Pointer table size = " << pointer_table.size() << endl
	 << "Free table size    = " << free_table.size() << endl
	 << "Threshold          = " << threshold_size << endl
	 << "Heap index         = " << heap_index << endl;
    if (heap_index > 0)
      cout << "pointer_table[" << heap_index - 1 << "]= " 
			    << pointer_table[heap_index - 1] << endl;
  }
# endif
};


# endif // POINTER_TABLE_H
