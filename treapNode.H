

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

