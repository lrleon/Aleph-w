/* 
  This file is part of Aleph-w library

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018

  Leandro Rabindranath Leon / Alejandro Mujica

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <https://www.gnu.org/licenses/>.
*/
# ifndef ALEPH_IT_H
# define ALEPH_IT_H


# define DEFINE_ITERATOR(Name, Base_Name, Set_Name, Item_Type)	\
class Name : public Base_Name
  {
  public:

    /// Tipo de elemento que retorna get_current()
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
    Node * get_current() { return get_current_node(); }

    /// \overload get_current_node()
    Node * get_curr() { return get_current_node(); }
  };



# endif // ALEPH_IT_H
