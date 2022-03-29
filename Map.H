

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

    iterator erase (const iterator & beg, const iterator & end)
    {
      Aleph::verify_iterators (beg, end);
      Aleph::verify_container_and_iterator (*this, beg);

      iterator ret_val = end;

      const size_t pos_beg = beg.itor.get_current_position ();
      const size_t pos_end = end.itor.get_current_position ();

      Node * removed_tree = tree.remove (pos_beg, pos_end - 1);

      destroyRec (removed_tree);

      return ret_val;
    }

    /// Borra todos los elementos del mapeo.
    void clear ()
    {
      destroyRec (tree.getRoot());
    }

  private:

    struct Proxy
    {
      map  * map_ptr;
      Node * node;
      Key    key;

      Proxy (map * m_ptr, const Key & _key) : map_ptr (m_ptr), key (_key)
      {
        node = map_ptr->search_in_tree(key);
      }

      Proxy & operator = (const Elem & elem)
      {
        if (node == nullptr)
          {
            node = map_ptr->node_pool.allocate(std::make_pair(key, elem));
            map_ptr->tree.insert(node);
          }
        else
          KEY(node).second = elem;

        return *this;
      }

      Proxy & operator = (const Proxy & proxy)
      {
        if (this == &proxy)
          return *this;

        if (proxy.node == nullptr)
          throw std::domain_error("key not found");;

        if (map_ptr == proxy.map_ptr and key == proxy.key)
          return *this; // They are the same

        if (node == nullptr)
          {
            node = map_ptr->node_pool.allocate(KEY(proxy.node));
            map_ptr->tree.insert(node);
          }
        else
          KEY(node).second = KEY(proxy.node).second;

        return *this;
      }

      operator Elem & () const
      {
        if (node == nullptr)
          throw std::domain_error ("key not found");;

        return KEY (node).second;
      }
    };

  public:

    const Proxy operator [] (const Key & key) const
    {
      return Proxy (this, key);
    }

    Proxy operator [] (const Key & key)
    {
      return Proxy (this, key);
    }
  };



} // end namespace Aleph


# endif // ALEPH_MAP_H
