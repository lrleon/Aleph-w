
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

/**
 * @file lazy_tree_traversal_example.cc
 * @brief Lazy binary tree traversal with coroutines (tpl_binNodeGenerators.H).
 *
 * ## Overview
 *
 * `for_each_in_order`/`for_each_preorder`/`for_each_postorder`
 * (`tpl_binNodeUtils.H`) visit a binary tree with a `void`-returning
 * callback, so those helpers always consume the whole traversal. Aleph also
 * has eager iterator-based helpers such as `infix_traverse()` and
 * `prefix_traverse()` whose `bool` callback can stop early.
 *
 * `lazy_in_order`/`lazy_pre_order`/`lazy_post_order`
 * (`tpl_binNodeGenerators.H`) traverse the exact same way, but as an
 * `Aleph::Generator<Node *>` driven by a range-`for`. A plain `break` stops
 * the traversal immediately, with no callback contract, and the result can be
 * composed with other `Generator`-based code.
 *
 * @author Leandro Rabindranath Leon
 * @ingroup Examples
 */

#include <iostream>
#include <vector>

#include <print_rule.H>
#include <tpl_binNode.H>
#include <tpl_binNodeGenerators.H>
#include <tpl_binNodeUtils.H>

using namespace Aleph;

namespace
{
// Owns every node allocated for the example tree and frees them on exit.
struct Node_Pool
{
  std::vector<BinNode<int> *> allocated;

  BinNode<int> *make(int k)
  {
    auto *p = new BinNode<int>(k);
    allocated.push_back(p);
    return p;
  }

  ~Node_Pool()
  {
    for (BinNode<int> *p : allocated)
      delete p;
  }
};

// Builds a complete, perfectly balanced BST over the ascending range
// [lo, hi] by recursively picking the midpoint as the root — the classic
// sorted-array-to-balanced-BST construction. Over 1..15 this produces:
//
//                    8
//            4               12
//        2       6       10      14
//      1   3   5   7   9  11  13   15
BinNode<int> *build_balanced_bst(Node_Pool &pool, int lo, int hi)
{
  if (lo > hi)
    return BinNode<int>::NullPtr;
  const int mid = lo + (hi - lo) / 2;
  BinNode<int> *node = pool.make(mid);
  node->getL() = build_balanced_bst(pool, lo, mid - 1);
  node->getR() = build_balanced_bst(pool, mid + 1, hi);
  return node;
}

void demo_lazy_traversal_orders(BinNode<int> *root)
{
  std::cout << "[1] in-order / pre-order / post-order, driven lazily\n";
  print_rule();

  std::cout << "in-order:   ";
  for (BinNode<int> *n : lazy_in_order(root))
    std::cout << KEY(n) << " ";
  std::cout << "\n";

  std::cout << "pre-order:  ";
  for (BinNode<int> *n : lazy_pre_order(root))
    std::cout << KEY(n) << " ";
  std::cout << "\n";

  std::cout << "post-order: ";
  for (BinNode<int> *n : lazy_post_order(root))
    std::cout << KEY(n) << " ";
  std::cout << "\n\n";
}

void demo_early_stop_ergonomics(BinNode<int> *root)
{
  std::cout << "[2] Early stop: callback contract vs plain break\n";
  print_rule();

  constexpr int target = 5;

  // --- Eager for_each: its callback returns void, so this helper cannot stop
  // before the end of the traversal.
  int for_each_visits = 0;
  bool for_each_found = false;
  for_each_in_order(root, [&](BinNode<int> *n)
  {
    ++for_each_visits;
    if (KEY(n) == target)
      for_each_found = true;
    // No return value here: this helper is intentionally unconditional.
  });
  std::cout << "Eager for_each_in_order searching for " << target << ":\n";
  std::cout << "  found=" << std::boolalpha << for_each_found
            << ", nodes visited=" << for_each_visits << " (always all 15)\n";

  // --- Eager traverse: iterator-based traversal can stop early too, but the
  // stop condition is encoded in the callback's boolean return value.
  int traverse_visits = 0;
  bool traverse_found = false;
  infix_traverse(root, [&](BinNode<int> *n)
  {
    ++traverse_visits;
    if (KEY(n) == target)
      {
        traverse_found = true;
        return false;
      }
    return true;
  });
  std::cout << "Eager infix_traverse searching for " << target << ":\n";
  std::cout << "  found=" << traverse_found
            << ", nodes visited=" << traverse_visits << " (stopped by false)\n";

  // --- Lazy: a plain `break` stops the coroutine at the point of interest.
  int lazy_visits = 0;
  bool lazy_found = false;
  for (BinNode<int> *n : lazy_in_order(root))
    {
      ++lazy_visits;
      if (KEY(n) == target)
        {
          lazy_found = true;
          break;
        }
    }
  std::cout << "Lazy lazy_in_order searching for " << target << ":\n";
  std::cout << "  found=" << lazy_found
            << ", nodes visited=" << lazy_visits << " (stopped as soon as found)\n\n";

  std::cout << "In-order visits 1,2,3,4,5 before reaching the target 5. Both\n"
               "infix_traverse and lazy_in_order avoid the remaining nodes;\n"
               "the lazy version reads as ordinary iteration-with-break and\n"
               "can be piped through other Generator stages.\n\n";
}
} // namespace

int main()
{
  std::cout << "\n=== Lazy binary tree traversal ===\n\n";

  Node_Pool pool;
  BinNode<int> *root = build_balanced_bst(pool, 1, 15);

  demo_lazy_traversal_orders(root);
  demo_early_stop_ergonomics(root);

  std::cout << "Done.\n";
  return 0;
}
