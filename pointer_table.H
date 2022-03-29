

/*
                          Aleph_w

  Data structures & Algorithms
  version 1.9d
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2022 Leandro Rabindranath Leon

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
