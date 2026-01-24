
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

/** Verify if tree rooted at p is a valid AVL tree with correct counters.

    @param p root of tree to verify
    @return true if tree is valid AVL with correct counts
*/
template <class Node>
bool is_avl_rk(Node* p)
{
  if (p == Node::NullPtr)
    return true;

  if (DIFF(p) < -1 or DIFF(p) > 1)
    return false;

  const int hL = computeHeightRec(LLINK(p));
  const int hR = computeHeightRec(RLINK(p));

  if (const int diff = hR - hL; diff > 1 or diff < -1)
    return false;

  if (static_cast<int>(DIFF(p)) != hR - hL)
    return false;

  // Verify counter
  if (p->getCount() != LLINK(p)->getCount() + RLINK(p)->getCount() + 1)
    return false;

  if (not is_avl_rk(LLINK(p)))
    return false;

  return is_avl_rk(RLINK(p));
}

} // end namespace Aleph

# endif // AVLNODERK_H