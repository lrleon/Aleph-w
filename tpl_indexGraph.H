

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

  void remove_node(GT_Node * p)
  {
    // eliminar del índice los arcos emanantes del nodo (ellos serán
    // eliminados por la eliminación de nodo del grafo)
    for (typename GT::Node_Arc_Iterator it(p); it.has_curr(); it.next_ne())
      idx_arc.remove(it.get_curr_ne());

    return idx_node.remove_from_graph(p);
  }

  /// Elimina del grafo y del índice el arco a
  void remove_arc(GT_Arc * a)
  {
    return idx_arc.remove_from_graph(a);
  }

  /// Retorna el número de arcos que contiene el índice.
  size_t get_num_arcs() const { return idx_arc.size(); }

  /// Retorna el número de nodos que contiene el índice.
  size_t get_num_nodes() const { return idx_node.size(); }
};

  template <class GT> inline
  bool are_equal(const GT & g1, const GT & g2)
  {
    if (g1.vsize() != g2.vsize() or g1.esize() != g2.esize())
      return false;

    {
      IndexNode<GT> t2(const_cast<GT&>(g2));
      if (not g1.all_nodes([&t2] (auto p)
                           {
                             auto q = t2.search(p);
                             if (q == nullptr)
                               return false;
                             GT::map_nodes(p, q);
                             return true;
                           }))
        return false;
    }

    IndexArc<GT> t2(const_cast<GT&>(g2));
    return g1.all_arcs([idx = &t2, &g1] (auto a)
                       {
                         auto s1 = g1.get_src_node(a);
                         auto t1 = g1.get_tgt_node(a);
                         auto s2 = mapped_node<GT>(s1);
                         auto t2 = mapped_node<GT>(t1);
                         auto a2 = idx->search(s2, t2);
                         if (a2 == nullptr)
                           return false;
                         return a2->get_info() == a->get_info();
                       });
  }

}

# endif // TPL_INDEXGRAPH


