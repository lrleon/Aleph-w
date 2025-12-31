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
# ifndef POINTER_TABLE_H
# define POINTER_TABLE_H

# include <tpl_dynArray.H>
# include <tpl_dynStack.H>
# include <ah-errors.H>
# include <iostream>

class Pointer_Table
{
private:

  static const long Null_Index = -1;

  DynArray<void*> pointer_table;

  DynArray<long> free_table;

  int num_pointers;

  /* Apunta a proximo disponible. Hacia abajo están ocupados */
  int heap_index;

  long threshold_size;

  long allocate_above_heap()
  {
    if (free_table.size() == 0)
      return Null_Index;

    long ret_val = free_table[free_table.size() - 1];

    assert(pointer_table[ret_val] == nullptr);

    free_table.cut(free_table.size() - 1);

    return ret_val;
  }

  void insert_in_free_table(long i)
  {
    assert(i >= 0 and i < heap_index);

    pointer_table[i] = nullptr;
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

    return heap_index > 0 and pointer_table[heap_index - 1] not_eq nullptr;
  }

public: 

  Pointer_Table(size_t initial_size = 0) 
    : pointer_table(initial_size), free_table(0),
      num_pointers(0), heap_index(0), threshold_size(initial_size)
  {
    for (int i = 0; i < pointer_table.size(); i++)
      pointer_table[i] = nullptr;
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
      ah_range_error_if(true) << "index out of range";

    if (pointer_table[i] == nullptr) 
      ah_domain_error_if(true) << "index is not busy";

    if (i == heap_index - 1)
      { /* Bajar el heap todos los disponibles que se puedan */
	pointer_table[heap_index - 1] = nullptr;
	while (heap_index > 0 and pointer_table[heap_index - 1] == nullptr)
	  {
	    heap_index--;
	    pointer_table[heap_index] = nullptr;
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
      ah_range_error_if(true) << "index out of range";

    if (not pointer_matchs_with_index(i, ptr))
      ah_domain_error_if(true) << "pointer does not match";

    return ptr;
  }

# ifdef DEBUG
  void print_parameters()
  {
    std::cout << "Number of pointers = " << num_pointers << std::endl
	      << "Pointer table size = " << pointer_table.size() << std::endl
	      << "Free table size    = " << free_table.size() << std::endl
	      << "Threshold          = " << threshold_size << std::endl
	      << "Heap index         = " << heap_index << std::endl;
    if (heap_index > 0)
      std::cout << "pointer_table[" << heap_index - 1 << "]= " 
                << pointer_table[heap_index - 1] << std::endl;
  }
# endif
};


# endif // POINTER_TABLE_H
