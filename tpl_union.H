

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
 }
  };

  struct Cmp
  {
    bool operator () (const Pair & p1, const Pair & p2) const noexcept
    {
      return Compare() (p1.item, p2.item);
    }
  };

  DynSetAvlTree<Pair, Cmp> items_tree;

  // retorna el id del item; sea porque lo encuentra o porque lo inserta
  size_t test_and_insert_new_item(const T & item) 
  {
    size_t N = size();
    Pair p(item, N);
    Pair * ptr = items_tree.search_or_insert(p);
    return ptr->i;
  }

public:

      /// Retorna true is i y j están conectados
  bool are_connected(const T & p, const T & q)
  {
    size_t i = test_and_insert_new_item(p);
    size_t j = test_and_insert_new_item(q);

    return Relation::are_connected(i, j);
  }

      /// Une p con q
  void join(const T & p, const T & q)
  {
    size_t i = test_and_insert_new_item(p);
    size_t j = test_and_insert_new_item(q);
    Relation::join(i, j);
  }
};

# endif // TPL_UNION_H
