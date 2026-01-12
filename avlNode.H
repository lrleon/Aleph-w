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

/** @file avlNode.H
 *  @brief AVL tree node with balance factor.
 *
 *  This file defines the AvlNode structure for AVL trees, including
 *  the balance factor (height difference) attribute and validation
 *  functions.
 *
 *  ## AVL Tree Properties
 *
 *  An AVL tree maintains the invariant that for every node:
 *  |height(left) - height(right)| ≤ 1
 *
 *  The balance factor (diff) stores: height(right) - height(left)
 *  Valid values are -1, 0, or +1.
 *
 *  ## Balance Factor Meaning
 *
 *  | diff | Meaning |
 *  |------|---------|
 *  | -1 | Left subtree is 1 taller |
 *  | 0 | Both subtrees have equal height |
 *  | +1 | Right subtree is 1 taller |
 *
 *  @see tpl_avl.H AVL tree implementation
 *  @see rbNode.H Red-Black tree node (alternative balancing)
 *
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

# ifndef AVLNODE_H
# define AVLNODE_H

# include <tpl_binNodeUtils.H>

/**
 * @brief Data portion of an AVL tree node.
 *
 * Stores the balance factor (height difference between right and left
 * subtrees). Valid values are -1, 0, or +1.
 *
 * @ingroup Trees
 */
class AvlNode_Data
{

  signed char diff = 0;  ///< Balance factor: height(right) - height(left)

public:

  /// Default constructor initializes balance to 0 (equal heights)
  AvlNode_Data() noexcept : diff(0) { /* EMPTY */ }

  /// Get reference to balance factor
  signed char & getDiff() noexcept { return diff; }

  /// Reset balance factor to 0 (for reuse)
  void reset() noexcept { diff = 0; }

};

/// Access the balance factor of node p
# define DIFF(p) ((p)->getDiff())

/// Declare AvlNode type with 40-byte pool allocation
DECLARE_BINNODE(AvlNode, 40, AvlNode_Data);

/**
 * @brief Validate that a tree satisfies AVL properties.
 *
 * Checks that:
 * - The stored balance factor matches actual height difference
 * - The balance factor is in range [-1, +1]
 * - All subtrees also satisfy AVL properties
 *
 * @tparam Node AVL node type
 * @param p Root of subtree to validate
 * @return true if tree is a valid AVL tree
 *
 * @note This function computes heights recursively, so it's O(n²).
 *       Use only for testing/debugging.
 *
 * @par Example
 * @code
 * Avl_Tree<int> tree;
 * // ... insertions ...
 * assert(is_avl(tree.getRoot()));
 * @endcode
 */
    template <class Node>
bool is_avl(Node* p)
{
  if (p == Node::NullPtr) 
    return true;

  // Balance factor must be in valid range
  if (DIFF(p) < -1 or DIFF(p) > 1)
    return false;
      
  // Compute actual heights
  const int hL = computeHeightRec(LLINK(p));
  const int hR = computeHeightRec(RLINK(p));

  // Verify AVL property: heights differ by at most 1
  if (const int diff = hR - hL; diff > 1 or diff < -1)
    return false;

  // Verify stored balance factor matches reality
  if (static_cast<int>(DIFF(p)) != hR - hL)
    return false;

  // Recursively check subtrees
  if (not is_avl(LLINK(p)))
    return false;

  return is_avl(RLINK(p));
}

# endif // AVLNODE_H
