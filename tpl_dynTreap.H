
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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


/** @file tpl_dynTreap.H
 *  @brief Dynamic treap alias.
 *
 *  Compatibility wrapper for treap.
 *
 *  @see tpl_treap.H Main treap implementation
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_DYNTREAP_H
# define TPL_DYNTREAP_H

# include <tpl_dynMapTree.H>
# include <tpl_treap.H>

namespace Aleph {

/**
   Dynamic mapping implemented with treap trees.

 */
    template <typename Key, typename Type, class Compare = Aleph::less<Key> >
struct DynTreapTree : public DynMapTree<Key, Type, Treap, Compare>
{
  
};

} // end namespace Aleph

# endif /* TPL_DYNTREAP_H */



