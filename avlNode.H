
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
