
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


/** @file avlNodeRk.H
 *  @brief AVL tree node with rank (subtree count).
 *
 *  Provides AvlNodeRk_Data with balance factor and subtree count
 *  for order-statistics AVL trees supporting select() and position().
 *
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

# ifndef AVLNODERK_H
# define AVLNODERK_H

# include <tpl_binNodeUtils.H>

namespace Aleph {

/** AVL node data with balance factor and subtree count.

    This class stores the balance factor (diff) for AVL balancing
    and a count field for the number of nodes in the subtree rooted
    at this node. The count enables O(log n) select and position operations.
*/
class AvlNodeRk_Data
{
  signed char diff = 0;   // balance factor: height(right) - height(left)
  size_t count = 1;       // number of nodes in subtree (including this node)

public:

  AvlNodeRk_Data() noexcept : diff(0), count(1) { /* empty */ }

  AvlNodeRk_Data(SentinelCtor) noexcept : diff(0), count(0) { /* empty */ }

  signed char & getDiff() noexcept { return diff; }

  size_t & getCount() noexcept { return count; }

  void reset() noexcept
  {
    diff = 0;
    count = 1;
  }
};

/// Extended AVL node with subtree counter
DECLARE_BINNODE_SENTINEL(AvlNodeRk, 40, AvlNodeRk_Data);

# define DIFF(p) ((p)->getDiff())

/** Height of a ranked AVL subtree, checking all its nodes.

    In one traversal, checks at every node of the subtree rooted at `p`
    that the heights of its two subtrees differ by at most one, that its
    stored balance factor is that difference, and that its counter is the
    size of its subtree.

    @param p root of subtree to validate
    @return the height of the subtree, or -1 if a node violates a property
    @note O(n).
*/
template <class Node>
int checked_avl_height_rk(Node* p) noexcept
{
  if (p == Node::NullPtr)
    return 0;

  if (p->getCount() != LLINK(p)->getCount() + RLINK(p)->getCount() + 1)
    return -1;

  const int hL = checked_avl_height_rk(LLINK(p));
  if (hL < 0)
    return -1;

  const int hR = checked_avl_height_rk(RLINK(p));
  if (hR < 0)
    return -1;

  const int diff = hR - hL;
  if (diff > 1 or diff < -1 or static_cast<int>(DIFF(p)) != diff)
    return -1;

  return 1 + (hL > hR ? hL : hR);
}

/** Verify if tree rooted at p is a valid AVL tree with correct counters.

    @param p root of tree to verify
    @return true if tree is valid AVL with correct counts
*/
template <class Node>
bool is_avl_rk(Node* p)
{
  return checked_avl_height_rk(p) >= 0;
}

} // end namespace Aleph

# endif // AVLNODERK_H