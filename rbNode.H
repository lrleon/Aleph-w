
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


/** @file rbNode.H
 *  @brief Red-Black tree node definition and validation utilities.
 *
 *  This file defines the RbNode structure for Red-Black trees, including
 *  the color attribute (RED or BLACK) and functions to validate RB tree
 *  properties.
 *
 *  ## Red-Black Tree Properties
 *
 *  1. Every node is either red or black
 *  2. All leaves (NIL) are black
 *  3. Red nodes have only black children
 *  4. All paths from a node to leaves have same black height
 *
 *  The usual definition adds that the root is black. The red-black trees of
 *  the library do not force it: the root can be red, as the only node of a
 *  tree is. A red root has black children, so the height is still at most
 *  2 log2(n + 1), and the validation functions accept it.
 *
 *  ## Validation Functions
 *
 *  - `is_red_black()`: Tests all RB properties
 *  - `test_black_condition()`: Verifies black-height property
 *  - `is_red_black_bst()`: Verifies RB + BST ordering
 *
 *  @see tpl_rb_tree.H Bottom-up Red-Black tree
 *  @see tpl_tdRbTree.H Top-down Red-Black tree
 *
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_RBNODE_H
# define TPL_RBNODE_H

# include <tpl_binNode.H>

/// Color type for RB nodes
typedef unsigned char Color;

/// Access the color of node p
# define COLOR(p) ((p)->getColor())

/// Red color constant (newly inserted nodes are red)
# define RED      (0)

/// Black color constant
# define BLACK    (1)

/**
 * @brief Data portion of a Red-Black tree node.
 *
 * Stores the color attribute (red or black) used for balancing.
 * Newly created nodes start as RED; the sentinel is BLACK.
 *
 * @ingroup Trees
 */
class RbNode_Data
{
private:  

  Color  color;  ///< Node color: RED or BLACK

public:

  /// Default constructor creates a red node
  RbNode_Data() : color(RED) {}

  /// Sentinel constructor creates a black node
  RbNode_Data(SentinelCtor) : color(BLACK) {}

  /// Get reference to node color
  Color & getColor() { return color; }

  /// Get node color
  Color getColor() const { return color; }

  /// Reset node to red (for reinsertion)
  void reset() { color = RED; }

}; 

/// Declare RbNode type with 128-byte pool allocation
DECLARE_BINNODE_SENTINEL(RbNode, 128, RbNode_Data);

/**
 * @brief Test the black-height property of an RB tree.
 *
 * Verifies that all paths from node p down to `Node::NullPtr` have the
 * same number of black nodes (black-height).
 *
 * @tparam Node RB node type
 * @param p Root of subtree to test
 * @param max Reference to store/compare black heights (-1 initially)
 * @param bh Current black height along this path (default 0)
 * @return true if all the paths have equal black height
 */
    template <class Node>
bool test_black_condition(Node *p, int &max, int bh = 0)
{
  // A path ends at NullPtr, not at a leaf: a node with a single child
  // also ends one, through its missing child.
  if (p == Node::NullPtr)
    {
      if (max == -1) // first path end reached
        max = bh;

      return bh == max;
    }

  if (COLOR(p) == BLACK)
    bh++; // count this black node

  return test_black_condition(LLINK(p), max, bh) and 
         test_black_condition(RLINK(p), max, bh);
}

/**
 * @brief Black height of a red-black subtree, checking all its nodes.
 *
 * In one traversal, checks every node of the subtree rooted at `p`: its
 * color is RED or BLACK, a red node has no red child, and every path from
 * `p` down to `Node::NullPtr` crosses the same number of black nodes.
 *
 * @tparam Node RB node type
 * @param p Root of subtree to test
 * @return the number of black nodes of each path, or -1 if a property is
 *         violated
 * @note O(n).
 */
    template <class Node>
int red_black_height(Node *p) noexcept
{
  if (p == Node::NullPtr)
    return 0;

  if (not (COLOR(p) == RED or COLOR(p) == BLACK))
    return -1;

  if (COLOR(p) == RED)
    {
      if (LLINK(p) != Node::NullPtr and COLOR(LLINK(p)) == RED)
        return -1;
      if (RLINK(p) != Node::NullPtr and COLOR(RLINK(p)) == RED)
        return -1;
    }

  const int left_height = red_black_height(LLINK(p));
  if (left_height < 0)
    return -1;

  if (red_black_height(RLINK(p)) != left_height)
    return -1;

  return left_height + (COLOR(p) == BLACK ? 1 : 0);
}

/**
 * @brief Test all Red-Black tree properties for a node.
 *
 * Validates that:
 * - Node color is valid (RED or BLACK)
 * - Red nodes don't have red children
 * - Black-height is consistent across all paths
 *
 * @tparam Node RB node type
 * @param node Node to test
 * @return true if all RB properties are satisfied
 */
    template <class Node>
bool is_red_black(Node * node)
{
  if (node == Node::NullPtr)
    return true;

  if (not (COLOR(node) == RED or COLOR(node) == BLACK))
    return false;

  // Red nodes cannot have red children
  if (COLOR(node) == RED)
    if (COLOR(LLINK(node)) == RED or COLOR(RLINK(node)) == RED)
      return false; 

  int max = -1;

  return test_black_condition(node, max, 0);
}

/**
 * @brief Test RB properties for entire subtree.
 *
 * Validates the RB properties of node and of all its descendants: valid
 * colors, no red node with a red child, and the same number of black nodes
 * on every path down to `Node::NullPtr`. `node` itself can be red or black.
 *
 * @tparam Node RB node type
 * @param node Root of subtree to test
 * @return true if entire subtree is a valid RB tree
 * @note O(n).
 */
    template <class Node>
bool is_red_black_tree(Node * node)
{
  return red_black_height(node) >= 0;
}

/**
 * @brief Verify that tree is both a valid RB tree and valid BST.
 *
 * Combines RB property validation with BST ordering validation.
 *
 * @tparam Node RB node type
 * @tparam Compare Comparison functor for keys
 * @param node Root of tree to validate
 * @param cmp Comparison function object
 * @return true if tree is a valid RB-BST
 */
    template <class Node, class Compare>
bool is_red_black_bst(Node * node, Compare & cmp)
{
  if (not is_red_black_tree(node))
    return false;

  return check_bst(node, cmp);
}
# endif // TPL_RBNODE_H
