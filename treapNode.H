
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


/** @file treapNode.H
 *  @brief Treap node definition with BST key and heap priority.
 *
 *  This file defines the TreapNode structure for treaps - a randomized
 *  data structure combining binary search tree and heap properties.
 *
 *  ## Treap Properties
 *
 *  A treap maintains two invariants simultaneously:
 *  1. **BST property**: Left descendants < node < right descendants (keys)
 *  2. **Min-heap property**: Parent priority ≤ child priorities
 *
 *  The random priorities ensure expected O(log n) height regardless
 *  of insertion order.
 *
 *  ## Why Treaps?
 *
 *  | Feature | Treap | AVL/RB |
 *  |---------|-------|--------|
 *  | Balance | Probabilistic | Guaranteed |
 *  | Code complexity | Simple | Complex |
 *  | Expected height | 1.99 log n | 1.44 log n |
 *  | Worst case | O(n) unlikely | O(log n) |
 *
 *  @see tpl_treap.H Treap implementation
 *  @see tpl_rand_tree.H Randomized BST
 *
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

# ifndef TREAPNODE_H
# define TREAPNODE_H

# include <limits.h> 
# include <tpl_binNode.H>

using namespace Aleph;

namespace Aleph {

/// Maximum priority value (used for sentinel/leaves)
const long Max_Priority = ULONG_MAX; 

/// Minimum priority value
const long Min_Priority = 0; 

/**
 * @brief Data portion of a treap node.
 *
 * Stores the random priority used to maintain heap property.
 * Lower priority values are closer to the root.
 *
 * @ingroup Trees
 */
class TreapNode_Data
{
  unsigned long priority;  ///< Random priority for heap property

public:

  /// Default constructor sets maximum priority
  TreapNode_Data() noexcept : priority (Max_Priority) {}

  /// Sentinel constructor also uses maximum priority
  TreapNode_Data(SentinelCtor) noexcept : priority (Max_Priority) {}

  /// Get reference to priority value
  unsigned long & getPriority() noexcept { return priority; }

  /// Reset (no-op for treap nodes)
  void reset() noexcept { /* empty */ }
};

/// Declare TreapNode type with 80-byte pool allocation
DECLARE_BINNODE_SENTINEL(TreapNode, 80, TreapNode_Data);

/**
 * @brief Access the priority of a treap node.
 *
 * @tparam Node Treap node type
 * @param p Pointer to node
 * @return Reference to the node's priority
 *
 * @ingroup Trees
 */
template <class Node> 
unsigned long & PRIO(Node * p) noexcept { return p->getPriority(); }

/**
 * @brief Validate that a tree satisfies treap (heap) property.
 *
 * Checks that every parent has priority ≤ both children's priorities.
 * This is the min-heap property that treaps maintain along with
 * the BST property.
 *
 * @tparam Node Treap node type
 * @param root Root of subtree to validate
 * @return true if heap property holds throughout the subtree
 *
 * @note This only checks heap property, not BST ordering.
 *       Use with check_bst() for complete validation.
 *
 * @par Example
 * @code
 * Treap<int> treap;
 * // ... insertions ...
 * assert(is_treap(treap.getRoot()));
 * @endcode
 */
    template <class Node>
bool is_treap(Node * root) noexcept
{
  if (root == Node::NullPtr) 
    return true;  

  // Parent priority must be ≤ children's priorities
  if (PRIO(root) > PRIO(LLINK(root)) or PRIO(root) > PRIO(RLINK(root)))
    return false; // heap property violation

  return is_treap(LLINK(root)) and is_treap(RLINK(root));
}

} // end namespace Aleph

# endif // TREAPNODE_H
