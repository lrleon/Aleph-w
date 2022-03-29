

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

    template <class Node, class Key, class Compare> inline
Node * search_or_insert_root_rec_xt(Node * root, Node * p, Compare & cmp)
      noexcept
{
  if (root == Node::NullPtr)
    return p; // insertion in empty tree

  if (cmp(KEY(p), KEY(root)))
    { // insert in left subtree
      Node * left_branch = search_or_insert_root_rec_xt(LLINK(root), p, cmp);
      if (left_branch == p) // p was inserted?
        {
          ++COUNT(root);
          LLINK(root) = left_branch;
          root        = rotate_to_right_xt(root);
          return p;
        }

        return left_branch;
    }
  else if (cmp(KEY(root), KEY(p)))
    { // insert in right subtree
      Node * right_branch = search_or_insert_root_rec_xt(RLINK(root), p, cmp);
      if (right_branch == p) // p was inserted?
        {
          ++COUNT(root);
          RLINK(root) = right_branch;
          root        = rotate_to_left_xt(root);
          return p;
        }

      return right_branch;
    }

  return root;
}

/// \overload search_or_insert_root_rec_xt()
template <class Node, class Key,
              class Compare = Aleph::less<typename Node::key_type>> inline
Node * search_or_insert_root_rec_xt(Node * root, Node * p,
                                    Compare && cmp = Compare()) noexcept
{
  return search_or_insert_root_rec_xt(root, p, cmp);
}


} // end namespace Aleph

# endif // TPL_BINNODEXT_H

