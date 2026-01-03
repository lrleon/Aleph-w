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
 *  2. The root is black
 *  3. All leaves (NIL) are black
 *  4. Red nodes have only black children
 *  5. All paths from a node to leaves have same black height
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
 *  @ingroup Arboles
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
 * @ingroup Arboles
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

  /// Reset node to red (for reinsertion)
  void reset() { color = RED; }

}; 

/// Declare RbNode type with 128-byte pool allocation
DECLARE_BINNODE_SENTINEL(RbNode, 128, RbNode_Data);

/**
 * @brief Test the black-height property of an RB tree.
 *
 * Verifies that all paths from node p to leaves have the same
 * number of black nodes (black-height).
 *
 * @tparam Node RB node type
 * @param p Root of subtree to test
 * @param max Reference to store/compare black heights (-1 initially)
 * @param bh Current black height along this path (default 0)
 * @return true if all leaf paths have equal black height
 */
    template <class Node>
bool test_black_condition(Node *p, int &max, int bh = 0)
{
  if (p == Node::NullPtr)
    return true;

  if (COLOR(p) == BLACK)
    bh++; // count this black node

  if (LLINK(p) == Node::NullPtr and RLINK(p) == Node::NullPtr)
    {
      if (max == -1) // first leaf reached
        max = bh;
      
      return bh == max;
    }
    
  return test_black_condition(LLINK(p), max, bh) and 
         test_black_condition(RLINK(p), max, bh);
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
 * Recursively validates RB properties for node and all descendants.
 *
 * @tparam Node RB node type
 * @param node Root of subtree to test
 * @return true if entire subtree is a valid RB tree
 */
    template <class Node>
bool is_red_black_tree(Node * node)
{
  if (not is_red_black(node))
    return false;

  if (not is_red_black(LLINK(node)) or not is_red_black(RLINK(node)))
    return false;

  return true;
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
