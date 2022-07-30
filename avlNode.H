
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

# ifndef AVLNODE_H
# define AVLNODE_H

# include <tpl_binNodeUtils.H>

class AvlNode_Data
{

private:

  signed char diff = 0; // diferencia de altura

public:

  AvlNode_Data() noexcept : diff(0) { /* EMPTY */ }

  signed char & getDiff() noexcept { return diff; }

  void reset() noexcept { diff = 0; }

};

/// \class AvlNode
DECLARE_BINNODE(AvlNode, 40, AvlNode_Data);


# define DIFF(p) ((p)->getDiff())

    template <class Node>
bool is_avl(Node* p)
{
  if (p == Node::NullPtr) 
    return true;

  if (DIFF(p) < -1 or DIFF(p) > 1)
    return false;
      
  const int hL = computeHeightRec(LLINK(p));
  const int hR = computeHeightRec(RLINK(p));

  int diff = hR - hL;

  if (diff > 1 or diff < -1)
    return false;

  if (((int) DIFF(p)) != hR - hL)
    return false;

  if (not is_avl(LLINK(p)))
    return false;

  return is_avl(RLINK(p));
}

# endif // AVLNODE_H

