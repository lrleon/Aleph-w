
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


/** @file tpl_balanceXt.H
 *  @brief Tree balancing with extended nodes.
 *
 *  Utilities for balancing trees with subtree counts.
 *
 *  @see tpl_binNodeXt.H Extended nodes
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_BALANCEXT_H
# define TPL_BALANCEXT_H

# include <tpl_binNodeXt.H>
 # include <ah-errors.H>

using namespace Aleph;

namespace Aleph {

    /** Select a node of a binary tree by its inorder position and make it
        the root.

        select_gotoup_root(r,i) selects the node at inorder position i and
        rotates it up until it becomes the root.

        This selection algorithm is recursive.

        @param[in] root root of the binary tree with subtree counts.
        @param[in] i inorder position to access.
        @return pointer to the node at inorder position i, which is the
        root of the binary tree after the call.
        @throw out_of_range if i is greater than or equal to the number of
        nodes of the binary tree.
        @see select() select_rec()

        @ingroup Trees
     */
    template <class Node> inline
Node * select_gotoup_root(Node * root, const size_t & i) 
{
  assert(root not_eq Node::NullPtr);
  assert(COUNT(Node::NullPtr) == 0);

  if (i >= COUNT(root))
    {
      ah_out_of_range_error_if(true) << "";
    }

  if (i == COUNT(LLINK(root))) 
    return root;

  if (i < COUNT(LLINK(root)))
    {
      LLINK(root) = select_gotoup_root(LLINK(root), i);
      root = rotate_to_right_xt(root);
    }
  else
    {
      RLINK(root) = select_gotoup_root(RLINK(root), 
                                       i - COUNT(LLINK(root)) - 1);
      root = rotate_to_left_xt(root);
    }
  return root;
}

    /** Rebalance a binary search tree.

        balance_tree(root) takes the binary search tree rooted at root and
        rebalances it "perfectly", in the sense that, for every node, the
        sizes of its two subtrees differ by at most one.

        The operation takes \f$O(n\; \lg n)\f$ time.

        @param[in] root the root of the binary search tree to rebalance.
        @return the root of the rebalanced binary search tree.
     */
template <class Node> inline Node * balance_tree(Node * root)
{
  if (COUNT(root) <= 1) 
    return root;

  root        = select_gotoup_root(root, COUNT(root) / 2);
  LLINK(root) = balance_tree(LLINK(root));
  RLINK(root) = balance_tree(RLINK(root));

  return root;
}

} // end namespace Aleph

# endif // TPL_BALANCEXT_H
