

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

  void splice(iterator pos, list & src_list, iterator src_pos)
  {
    Aleph::verify_container_and_iterator(*this, pos);
    Aleph::verify_container_and_iterator(src_list, src_pos);

    pos.itor.get_curr()->insert(src_pos.itor.del());
    --src_list.num_elem;
    ++num_elem;
  }

      /// Mueve el rango de elementos [src_beg,src_end) de la lista src_list
      /// hacia la lista this en tiempo \f$O(1)\f$.
  void splice(iterator pos, list & src_list, 
	      iterator src_beg, const iterator & src_end)
  {
    Aleph::verify_container_and_iterator(*this, pos);
    Aleph::verify_container_and_iterator(src_list, src_beg);
    Aleph::verify_iterators(src_beg, src_end);    

    Dlink list_to_insert;
    src_list.dlist.cut_list(src_beg.itor.get_curr(), &list_to_insert);

    Dlink remaining_list;
    list_to_insert.cut_list(src_end.itor.get_curr(), &remaining_list);
    
    pos.itor.get_curr()->insert_list(&list_to_insert);
    num_elem_is_updated = false;

    src_list.dlist.concat_list(&remaining_list);
    src_list.num_elem_is_updated = false;
  }

      /// Ordena la lista según criterio de comparación Cmp.
      template <class Cmp>
  void sort(const Cmp &)
  {
    quicksort<T, Cmp>(dlist);	
  }

      /// Ordena la lista.
  void sort()
  {
    sort(Aleph::less<T>()); 
  }

      /// Mezcla dos listas ordenadas según criterio de comparación Cmp.
      template <class Cmp>
  void merge(list & l, const Cmp &)
  {
    Dnode<T> result;

    Aleph::merge_lists<T, Cmp>(dlist, l.dlist, result);

    dlist.swap(result);

    num_elem += l.num_elem;

    l.reset_num_elem();

    assert(l.dlist.is_empty());
  }

      /// Mezcla dos listas ordenadas.
  void merge(list & l)
  {
    merge(l, Aleph::less<T>());
  }

      /// Invierte la lista.
  void reverse()
  {
    reset_num_elem(dlist.reverse_list());
  }
};


    template <typename T>
typename list<T>::size_type distance(typename list<T>::iterator it1, 
				     typename list<T>::iterator it2)
{
  typename list<T>::size_type counter = 0;

  while (it1 != it2)
    {
      counter++;
      it1++;
    }

  return counter;
}

} // end namespace Aleph

# endif // ALEPH_LIST_H
