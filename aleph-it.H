
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
# ifndef ALEPH_IT_H
# define ALEPH_IT_H


# define DEFINE_ITERATOR(Name, Base_Name, Set_Name, Item_Type)	\
class Name : public Base_Name
  {
  public:

    /// Tipo de elemento que retorna get_curr()
    typedef Node * Item_Type;

    /// El tipo de conjunto sobre el cual se itera.
    typedef List_AGraph Set_Type;

    Node_Iterator() { /* empty */ }

    Node_Iterator(List_AGraph & g) : Base_Name::Iterator(g.node_list)
    {
      // empty
    }

    /// retorna el nodo actual.
    Node * get_current_node()
    {
      return DynArray<Node*>::Iterator::get_curr();
    }

    /// \overload get_current_node()
    Node * get_curr() { return get_current_node(); }
  };



# endif // ALEPH_IT_H
