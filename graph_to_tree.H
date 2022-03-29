

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

      Tree_Node<Key> * 
  operator () (GT & g, typename GT::Node * groot, Convert && conv = Convert())
  { 
    return graph_to_tree(g, groot, conv);
  }

      Tree_Node<Key> * 
  operator () (GT & g, typename GT::Node * groot, Convert & conv)
  { 
    return graph_to_tree(g, groot, conv);
  }
};


  template <class GT, typename Key, typename Convert, class SA> static inline
void __graph_to_tree_node(GT & g, typename GT::Node * groot,
                          Tree_Node<Key> * troot)
{           
  typedef typename GT::Node Node;
  typedef typename GT::Arc Arc;

      // recorrer arcos de groot y construir recursivamente
  for (Node_Arc_Iterator<GT, SA> it(groot); it.has_curr(); it.next_ne())
    {
      Arc * arc = it.get_current_arc_ne();
      if (IS_ARC_VISITED(arc, Convert_Tree)) 
        continue;

      ARC_BITS(arc).set_bit(Convert_Tree, true); // arc visitado 
      Node * gtgt = it.get_tgt_node();
      Tree_Node<Key> * ttgt = new Tree_Node<Key>; 

      Convert () (gtgt, ttgt); // asignarle la clave

      troot->insert_rightmost_child(ttgt); // insertarlo como hijo 

      __graph_to_tree_node <GT, Key, Convert, SA> (g, gtgt, ttgt);
    }
}

} // end namespace Aleph

# endif // GRAPH_TO_TREE_H
