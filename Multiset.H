

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

    iterator erase (iterator beg, const iterator & end)
    {
      Aleph::verify_container_and_iterator (*this, beg);
      Aleph::verify_iterators (beg, end);

      return delete_range(beg, end);
    }

    /// Borra todos los elementos del multi-conjunto.
    void clear ()
    {
      destroyRec (static_cast<Node *> (tree.getRoot ()));
      num_elem = 0;
    }
  };


  template <typename T>
  typename iterator_traits<typename multiset<T>::iterator>::difference_type 
  distance (typename multiset<T>::iterator it1, 
            typename multiset<T>::iterator it2)
  {
    typename iterator_traits<typename multiset<T>::iterator>::difference_type
      counter = 0;

  
    while (it1 != it2)
      {
        ++counter;
        ++it1;
      }

    return counter;
  }

} // end namespace Aleph

# endif // AH_MULTISET_H
