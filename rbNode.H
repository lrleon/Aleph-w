
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

# ifndef TPL_RBNODE_H
# define TPL_RBNODE_H

# include <tpl_binNode.H>

typedef unsigned char Color;
# define COLOR(p) ((p)->getColor())
# define RED      (0)
# define BLACK    (1)
class RbNode_Data
{
private:  

  Color  color; // RED o BLACK

public:

  RbNode_Data() : color(RED) {}
  RbNode_Data(SentinelCtor) : color(BLACK) {}

  Color & getColor() { return color; }

  void reset() { color = RED; }

}; 

DECLARE_BINNODE_SENTINEL(RbNode, 128, RbNode_Data);

    template <class Node>
bool test_black_condition(Node *p, int &max, int bh = 0)
{
  if (p == Node::NullPtr)
    return true;

  if (COLOR(p) == BLACK)
    bh++; // se ha visto otro nodo negro

  if (LLINK(p) == Node::NullPtr and RLINK(p) == Node::NullPtr)
    {
      if (max == -1) // cierto cuando se alcanza la primera hoja
        max = bh;
      
      return bh == max;
    }
    
  return test_black_condition(LLINK(p), max, bh) and 
         test_black_condition(RLINK(p), max, bh);
}

    template <class Node>
bool is_red_black(Node * node)
{
  if (node == Node::NullPtr)
    return true;

  if (not (COLOR(node) == RED or COLOR(node) == BLACK))
    return false;

  if (COLOR(node) == RED)
    if (COLOR(LLINK(node)) == RED or COLOR(RLINK(node)) == RED)
      return false; 

  int max = -1;
  const int bh  = 0;
  
  return test_black_condition(node, max, bh);
}

    template <class Node>
bool is_red_black_tree(Node * node)
{
  if (is_red_black(node))
    return is_red_black(LLINK(node)) and is_red_black(RLINK(node));

  return false;
}
# endif // TPL_RBNODE_H

