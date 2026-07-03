
/*
                          Aleph_w

  Data structures & Algorithms
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

/** @file tpl_binNodeGenerators.H
 *  @brief Lazy (coroutine-based) traversals of binary trees.
 *
 *  Lazy counterparts of the eager visitor-based traversals in
 *  `tpl_binNodeUtils.H` (`for_each_in_order`, `for_each_preorder`,
 *  `for_each_postorder`): instead of invoking a callback on every node,
 *  `lazy_in_order`/`lazy_pre_order`/`lazy_post_order` return an
 *  `Aleph::Generator<Node *>` that yields one node at a time, driven by the
 *  caller (range-`for`, early `break`, composing with other generators…).
 *
 *  The eager visitor-based traversals are **not replaced** — they remain
 *  the right tool when the full tree must be visited and no early exit is
 *  expected (a hand-written recursive call is cheaper than a coroutine
 *  resume). Reach for the lazy versions when you may stop early, when you
 *  want to interleave traversal with other lazy computation (`Aleph::
 *  Generator` composition), or when materializing the traversal into a
 *  container is wasteful.
 *
 *  @code
 *  for (Node *n : lazy_in_order(root))
 *    {
 *      if (KEY(n) == target)
 *        break;             // stops the traversal outright; no wasted work
 *      process(n);
 *    }
 *  @endcode
 *
 *  @note **Stack depth**: each level of tree depth is one nested coroutine
 *        frame (heap-allocated). For balanced trees (depth O(log n)) this
 *        is negligible; for deliberately degenerate trees (depth O(n),
 *        e.g. a tree built by inserting an already-sorted sequence into an
 *        unbalanced BST) this allocates O(n) coroutine frames instead of
 *        the O(1) auxiliary space a hand-written iterative traversal would
 *        use. Prefer the eager traversals (or a balanced tree) for
 *        adversarial-depth inputs.
 *
 *  @see tpl_binNodeUtils.H Eager visitor-based traversals.
 *  @see ah-generator.H `Aleph::Generator<T>`, the underlying lazy sequence
 *       type.
 *
 *  @ingroup Trees
 *  @author Leandro Rabindranath Leon
 */

#ifndef TPL_BINNODEGENERATORS_H
#define TPL_BINNODEGENERATORS_H

#include <ah-generator.H>
#include <tpl_binNode.H>

namespace Aleph
{

/** @brief Lazily traverse a binary tree in-order (left, node, right).
 *
 *  Yields nodes in ascending key order for a valid BST.
 *
 *  @tparam Node Binary tree node type providing `getL()`/`getR()` and a
 *          `NullPtr` sentinel (any Aleph binary tree node, e.g. `BinNode`,
 *          `Avl_Node`, `Rb_Tree_Node`…).
 *  @param root Root of the (sub)tree to traverse; `Node::NullPtr` yields an
 *         empty sequence.
 *  @return A lazy sequence of `Node *`, one per visited node, in in-order.
 *  @note Complexity: O(1) amortized per yielded node; O(depth) nested
 *        coroutine frames alive at any time (see the file-level note).
 *
 *  @ingroup Trees
 */
template <class Node>
Aleph::Generator<Node *> lazy_in_order(Node *root)
{
  if (root == Node::NullPtr)
    co_return;

  for (Node *n : lazy_in_order(LLINK(root)))
    co_yield n;
  co_yield root;
  for (Node *n : lazy_in_order(RLINK(root)))
    co_yield n;
}

/** @brief Lazily traverse a binary tree pre-order (node, left, right).
 *
 *  @tparam Node Binary tree node type (see `lazy_in_order`).
 *  @param root Root of the (sub)tree to traverse; `Node::NullPtr` yields an
 *         empty sequence.
 *  @return A lazy sequence of `Node *`, one per visited node, in pre-order.
 *  @note Complexity: O(1) amortized per yielded node; O(depth) nested
 *        coroutine frames alive at any time (see the file-level note).
 *
 *  @ingroup Trees
 */
template <class Node>
Aleph::Generator<Node *> lazy_pre_order(Node *root)
{
  if (root == Node::NullPtr)
    co_return;

  co_yield root;
  for (Node *n : lazy_pre_order(LLINK(root)))
    co_yield n;
  for (Node *n : lazy_pre_order(RLINK(root)))
    co_yield n;
}

/** @brief Lazily traverse a binary tree post-order (left, right, node).
 *
 *  @tparam Node Binary tree node type (see `lazy_in_order`).
 *  @param root Root of the (sub)tree to traverse; `Node::NullPtr` yields an
 *         empty sequence.
 *  @return A lazy sequence of `Node *`, one per visited node, in post-order.
 *  @note Complexity: O(1) amortized per yielded node; O(depth) nested
 *        coroutine frames alive at any time (see the file-level note).
 *
 *  @ingroup Trees
 */
template <class Node>
Aleph::Generator<Node *> lazy_post_order(Node *root)
{
  if (root == Node::NullPtr)
    co_return;

  for (Node *n : lazy_post_order(LLINK(root)))
    co_yield n;
  for (Node *n : lazy_post_order(RLINK(root)))
    co_yield n;
  co_yield root;
}

} // end namespace Aleph

#endif // TPL_BINNODEGENERATORS_H