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

/** @file treapNode.H
 *  @brief Treap node with key and priority.
 *
 *  Defines TreapNode with BST key and heap priority.
 *
 *  @see tpl_treap.H Treap implementation
 *  @ingroup Arboles
 *  @author Leandro Rabindranath León
 */

# ifndef TREAPNODE_H
# define TREAPNODE_H

# include <limits.h> 
# include <tpl_binNode.H>

using namespace Aleph;

namespace Aleph {

const long Max_Priority = ULONG_MAX; 
const long Min_Priority = 0; 

class TreapNode_Data
{
  unsigned long priority;  

public:

  TreapNode_Data() noexcept : priority (Max_Priority) {}

  TreapNode_Data(SentinelCtor) noexcept : priority (Max_Priority) {}

  unsigned long & getPriority() noexcept { return priority; }

  void reset() noexcept { /* empty */ }
};

DECLARE_BINNODE_SENTINEL(TreapNode, 80, TreapNode_Data);

/** Return the priority of node

    @ingroup Arboles
*/
template <class Node> 
unsigned long & PRIO(Node * p) noexcept { return p->getPriority(); }

/// Return `true` if root is a consistent treap
    template <class Node>
bool is_treap(Node * root) noexcept
{
  if (root == Node::NullPtr) 
    return true;  

  if (PRIO(root) > PRIO(LLINK(root)) or PRIO(root) > PRIO(RLINK(root)))
    return false; // violación de relación ancestral

  return is_treap(LLINK(root)) and is_treap(RLINK(root));
}

} // end namespace Aleph

# endif // TREAPNODE_H

