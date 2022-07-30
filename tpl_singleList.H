
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
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
# ifndef TPL_SINGLELIST_H
# define TPL_SINGLELIST_H

# include <ahDefs.H>

    template <class T>
class Single_List 
{
private:

  struct Node
  {
    T      data;
    Node * next; 

    Node * get_by_pos(int pos)
    {
      int i;
      Node * node = this;
      for (i = 0; i < pos; ++i)
        {
          if (node->next == nullptr)
            throw std::overflow_error("overflow in access by position");
          node = node->next;
        }
      
      return node;
    }
    Node() : next (nullptr) { /* empty */ }

    Node(const T& _data) : data(_data), next (nullptr) { /* empty */ }
  };
  int    num_items;
  Node   head_node;
  Node * head;

public:

  unsigned int get_num_items() const { return num_items; }
  T& operator [] (unsigned int pos) Exception_Prototypes(std::overflow_error) 
  { 
    if (pos > num_items) 
      throw std::overflow_error("List overflow");
    return head->get_by_pos(pos + 1)->data;
  }
  void insert(unsigned int pos, const T& t) 
    Exception_Prototypes(std::bad_alloc, std::overflow_error)  
  {
    if (pos > num_items) 
      throw std::overflow_error("List overflow");
    Node * node = new Node (t);
    Node * prev = head->get_by_pos(pos - 1); 
    node->next = prev->next;
    prev->next = node;
    ++num_items; 
  }
  void remove(unsigned int pos) Exception_Prototypes(std::overflow_error)
  { 
    if (pos > num_items) 
      throw std::overflow_error("List overflow");

    Node * prev = head->get_by_pos(pos - 1);
    Node * node_to_delete = prev->next;
    prev->next = node_to_delete->next;
    num_items--;
    delete node_to_delete;
  }
};

# endif
