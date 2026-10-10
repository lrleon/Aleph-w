
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


/** @file rbNodeRk.H
 *  @brief Red-Black tree node with rank (subtree count).
 *
 *  Extends RbNode with subtree size for order statistics.
 *
 *  @see tpl_rbRk.H RB tree with rank
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

# ifndef RBNODERK_H
# define RBNODERK_H

# include <tpl_binNode.H>

namespace Aleph {

typedef unsigned char Color;
# define COLOR(p) ((p)->getColor())
# define RED      (0)
# define BLACK    (1)

/** Red-Black node data with color and subtree count.

    This class stores the color (RED or BLACK) for Red-Black balancing
    and a count field for the number of nodes in the subtree rooted
    at this node. The count enables O(log n) select and position operations.
*/
class RbNodeRk_Data
{
  Color color;      // RED or BLACK
  size_t count;     // number of nodes in subtree (including this node)

public:

  RbNodeRk_Data() noexcept : color(RED), count(1) { /* empty */ }

  RbNodeRk_Data(SentinelCtor) noexcept : color(BLACK), count(0) { /* empty */ }

  Color & getColor() noexcept { return color; }

  size_t & getCount() noexcept { return count; }

  void reset() noexcept
  {
    color = RED;
    count = 1;
  }
};

/// Extended Red-Black node with subtree counter
DECLARE_BINNODE_SENTINEL(RbNodeRk, 128, RbNodeRk_Data);


/** Test the black height condition for a red-black tree.

    @param p root of tree
    @param max reference to store the black height
    @param bh current black height
    @return true if condition is satisfied
*/
template <class Node>
bool test_black_condition_rk(Node *p, int &max, int bh = 0)
{
  // A path ends at NullPtr, not at a leaf: a node with a single child
  // also ends one, through its missing child.
  if (p == Node::NullPtr)
    {
      if (max == -1)
        max = bh;

      return bh == max;
    }

  if (COLOR(p) == BLACK)
    bh++;

  return test_black_condition_rk(LLINK(p), max, bh) and
         test_black_condition_rk(RLINK(p), max, bh);
}

/** Black height of a ranked red-black subtree, checking all its nodes.

    In one traversal, checks every node of the subtree rooted at `p`: its
    counter is the size of its subtree, its color is RED or BLACK, a red
    node has no red child, and every path from `p` down to `Node::NullPtr`
    crosses the same number of black nodes.

    @param p root of subtree
    @return the number of black nodes of each path, or -1 if a property is
            violated
    @note O(n).
*/
template <class Node>
int red_black_height_rk(Node *p) noexcept
{
  if (p == Node::NullPtr)
    return 0;

  if (p->getCount() != LLINK(p)->getCount() + RLINK(p)->getCount() + 1)
    return -1;

  if (not (COLOR(p) == RED or COLOR(p) == BLACK))
    return -1;

  if (COLOR(p) == RED)
    {
      if (LLINK(p) != Node::NullPtr and COLOR(LLINK(p)) == RED)
        return -1;
      if (RLINK(p) != Node::NullPtr and COLOR(RLINK(p)) == RED)
        return -1;
    }

  const int left_height = red_black_height_rk(LLINK(p));
  if (left_height < 0)
    return -1;

  if (red_black_height_rk(RLINK(p)) != left_height)
    return -1;

  return left_height + (COLOR(p) == BLACK ? 1 : 0);
}

/** Verify if tree rooted at node is a valid red-black tree.

    @param node root of tree
    @return true if tree satisfies red-black properties
*/
template <class Node>
bool is_red_black_rk(Node * node)
{
  if (node == Node::NullPtr)
    return true;

  if (not (COLOR(node) == RED or COLOR(node) == BLACK))
    return false;

  if (COLOR(node) == RED)
    if (COLOR(LLINK(node)) == RED or COLOR(RLINK(node)) == RED)
      return false;

  int max = -1;

  return test_black_condition_rk(node, max, 0);
}

/** Verify if tree is a valid red-black tree with correct counters.

    Checks every node: valid colors, no red node with a red child, the same
    number of black nodes on every path down to `Node::NullPtr`, and the
    counter of each subtree. `node` itself can be red or black.

    @param node root of tree
    @return true if tree is valid
*/
template <class Node>
bool is_red_black_tree_rk(Node * node)
{
  return red_black_height_rk(node) >= 0;
}

/** Verify that tree rooted at node is a valid red-black BST with counters.

    @param node root of tree
    @param cmp comparison criteria
    @return true if valid
*/
template <class Node, class Compare>
bool is_red_black_bst_rk(Node * node, Compare & cmp)
{
  if (not is_red_black_tree_rk(node))
    return false;

  return check_bst(node, cmp);
}

} // end namespace Aleph

# endif // RBNODERK_H