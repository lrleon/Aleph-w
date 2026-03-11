/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
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
      return Base_Name::Iterator::get_curr();
    }
  };


# endif
