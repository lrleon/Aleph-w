
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


# ifndef OPBINTREE_H 
# define OPBINTREE_H

# include <limits>
# include <tpl_dynArray.H>

namespace Aleph {

/** @file opBinTree.H
 *  @brief Optimal Binary Search Tree construction using dynamic programming.
 *
 *  This module implements the dynamic programming algorithm for constructing
 *  an optimal binary search tree given keys and their access probabilities.
 *  The algorithm minimizes the expected search cost.
 *
 *  Uses two optimizations:
 *  - **Prefix sums**: O(1) range sum queries instead of O(n)
 *  - **Knuth's optimization**: Reduces search space using monotonicity of optimal roots
 *
 *  Time complexity: O(n²) (with Knuth's optimization)
 *  Space complexity: O(n²)
 *
 *  @see Knuth, D.E. "Optimum Binary Search Trees" Acta Informatica 1 (1971)
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

// Internal macros for 2D array access using 1D storage
# define COST(i,j) (cost[(i)*(n+1) + (j)])
# define TREE(i,j) (tree[(i)*(n+1) + (j)])

/** @brief Compute optimal costs and tree structure using dynamic programming.
 *
 *  Fills the cost and tree matrices using the recurrence:
 *  cost[i,j] = min over k in [i,j] of (cost[i,k-1] + cost[k+1,j]) + sum(p[i..j])
 *
 *  Uses two key optimizations:
 *  1. **Prefix sums** for O(1) probability range queries
 *  2. **Knuth's optimization**: root[i,j-1] ≤ root[i,j] ≤ root[i+1,j]
 *     This reduces the inner loop from O(n) to amortized O(1), giving O(n²) total.
 *
 *  @param[out] cost Matrix storing optimal costs for subproblems.
 *  @param[in] p Array of access probabilities (0-indexed, size n).
 *  @param[in] n Number of keys.
 *  @param[out] tree Matrix storing optimal root indices for subproblems.
 */
static inline void compute_optimal_costs(DynArray<double> & cost, double p[], 
                                         const size_t n, DynArray<int> & tree)
{
  // Precompute prefix sums for O(1) range sum queries
  // prefix[i] = p[0] + p[1] + ... + p[i-1], prefix[0] = 0
  DynArray<double> prefix(n + 2);
  prefix[0] = 0.0;
  for (size_t i = 0; i < n; ++i)
    prefix[i + 1] = prefix[i] + p[i];

  // sum_p(i, j) = prefix[j] - prefix[i-1] for 1-indexed i,j
  auto sum_p = [&prefix](size_t i, size_t j) noexcept -> double {
    return prefix[j] - prefix[i - 1];
  };

  // Base cases: empty intervals have zero cost
  for (size_t i = 1; i <= n + 1; ++i)
    COST(i, i - 1) = 0;

  // Base cases: single-element intervals - the only choice is the element itself
  for (size_t i = 1; i <= n; ++i)
    {
      TREE(i, i) = static_cast<int>(i);
      COST(i, i) = p[i - 1];  // Cost is just the probability (depth = 1)
    }

  // Fill DP table by increasing interval length
  // Knuth's optimization: search only in [TREE(i, j-1), TREE(i+1, j)]
  for (size_t len = 2; len <= n; ++len)  // len = j - i + 1
    {
      for (size_t i = 1; i + len - 1 <= n; ++i)
        {
          const size_t j = i + len - 1;

          // Knuth bounds: root[i,j-1] <= root[i,j] <= root[i+1,j]
          const size_t lo = static_cast<size_t>(TREE(i, j - 1));
          const size_t hi = static_cast<size_t>(TREE(i + 1, j));

          double min_cost = std::numeric_limits<double>::max();
          size_t best_root = lo;

          for (size_t r = lo; r <= hi; ++r)
            {
              const double c = COST(i, r - 1) + COST(r + 1, j);
              if (c < min_cost)
                {
                  min_cost = c;
                  best_root = r;
                }
            }

          TREE(i, j) = static_cast<int>(best_root);
          COST(i, j) = min_cost + sum_p(i, j);
        }
    }
}

/** @brief Recursively construct the optimal BST from the tree matrix.
 *  @tparam Node Binary tree node type (must have NullPtr, LLINK, RLINK).
 *  @tparam Key Key type stored in nodes.
 *  @param[in] keys Array of keys (0-indexed).
 *  @param[in] tree Matrix of optimal root indices.
 *  @param[in] n Total number of keys.
 *  @param[in] i Start index of current subproblem.
 *  @param[in] j End index of current subproblem.
 *  @return Root of the constructed subtree.
 */
template <class Node, typename Key>
[[nodiscard]] static inline Node * compute_tree(Key keys[], DynArray<int> & tree, 
                                                const size_t n, 
                                                const size_t i, const size_t j)
{
  if (i > j) 
    return Node::NullPtr;

  const int root_idx = TREE(i, j);
  Node * root = new Node(keys[root_idx - 1]);
  LLINK(root) = compute_tree<Node, Key>(keys, tree, n, i, root_idx - 1);
  RLINK(root) = compute_tree<Node, Key>(keys, tree, n, root_idx + 1, j);
  return root;
}

/** @brief Build an optimal binary search tree based on access probabilities.
 *
 *  Constructs a binary search tree that minimizes the expected search cost
 *  given the access probabilities for each key. Uses dynamic programming
 *  with Knuth's optimization for O(n²) time and O(n²) space complexity.
 *
 *  The algorithm computes the optimal root for each subproblem [i,j] using
 *  the monotonicity property: root[i,j-1] ≤ root[i,j] ≤ root[i+1,j]
 *
 *  @tparam Node Binary tree node type. Must provide:
 *         - `NullPtr` static member for null pointer
 *         - Constructor taking a Key
 *         - `LLINK(node)` and `RLINK(node)` macros for child access
 *  @tparam Key Key type stored in the tree nodes.
 *
 *  @param[in] keys Array of n keys in sorted order (0-indexed).
 *  @param[in] p Array of n access probabilities (0-indexed), parallel to keys.
 *             Should sum to 1.0 for proper interpretation as probabilities.
 *  @param[in] n Number of keys.
 *
 *  @return Root of the optimal binary search tree.
 *  @throw std::bad_alloc If memory allocation fails.
 *
 *  @note Keys must be in sorted order for the resulting tree to be a valid BST.
 *  @note The caller is responsible for deleting the returned tree.
 *
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */
template <class Node, typename Key>
[[nodiscard]] Node * build_optimal_tree(Key keys[], double p[], const size_t n)
{
  DynArray<int> tree((n + 1) * (n + 1));
  DynArray<double> cost((n + 1) * (n + 1));
  compute_optimal_costs(cost, p, n, tree);
  return compute_tree<Node, Key>(keys, tree, n, 1, n);
}


# undef COST
# undef TREE
} // end namespace Aleph
# endif // OPBINTREE_H

