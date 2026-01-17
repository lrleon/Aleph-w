
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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


/** @file random_tree.H
 *  @brief Random tree generation using GSL random number generator.
 *
 *  This file provides the RandTree class for generating uniformly random
 *  trees with n nodes. Uses the recursive bijection between binary trees
 *  and general trees for generation.
 *
 *  ## Algorithm
 *
 *  The algorithm generates random binary trees by:
 *  1. Choosing a random inorder position i for the root (0 to n-1)
 *  2. Recursively building left subtree with i nodes
 *  3. Recursively building right subtree with n-i-1 nodes
 *  4. Converting binary tree to general tree
 *
 *  This produces uniformly distributed random trees.
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  #include <random_tree.H>
 *
 *  RandTree<int> gen(42);  // Seed = 42
 *  Tree_Node<int>* tree = gen(100);  // Generate tree with 100 nodes
 *
 *  // ... use tree ...
 *
 *  destroyRec(tree);  // Clean up
 *  ```
 *
 *  @see tpl_tree_node.H General tree node
 *  @see tpl_binNode.H Binary tree node
 *  @see random_graph.H Random graph generation
 *
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

#ifndef RANDOM_TREE_H
#define RANDOM_TREE_H

# include <gsl/gsl_rng.h>
# include <memory>
# include <tpl_tree_node.H>
# include <tpl_binNodeUtils.H>

# include <ah-errors.H>

using namespace Aleph;

/**
 * @brief Generator for uniformly random trees.
 *
 * Uses the GSL Mersenne Twister (MT19937) random number generator to
 * create uniformly distributed random trees with a specified number
 * of nodes.
 *
 * @tparam T Type of data stored in tree nodes (not used for generation)
 *
 * ## Complexity
 *
 * - Time: O(n) expected for n nodes
 * - Space: O(n) for the tree structure
 *
 * @par Example
 * @code
 * // Create generator with specific seed for reproducibility
 * RandTree<std::string> gen(12345);
 *
 * // Generate random tree with 50 nodes
 * Tree_Node<std::string>* root = gen(50);
 *
 * // Tree structure is random but deterministic for same seed
 * @endcode
 *
 * @ingroup Trees
 *  @author Leandro Rabindranath León
 */
template <typename T>
class RandTree
{
  gsl_rng * r = nullptr;  ///< GSL random number generator

  /**
   * @brief Recursively generate random binary tree.
   *
   * @param n Number of nodes in the subtree
   * @return Root of the generated subtree, or nullptr if n=0
   */
  BinNode<T> * random(unsigned long n)
  {
    if (n == 0)
      return nullptr;

    // inorder position of the root (random choice)
    unsigned long i = gsl_rng_uniform_int(r, n);
    BinNode<T> * root = new BinNode<T>();
    LLINK(root) = random(i);
    RLINK(root) = random(n - i - 1);
    return root;
  }

public:

  /**
   * @brief Construct generator with specified seed.
   *
   * @param seed Random seed for reproducibility
   * @throw std::bad_alloc if GSL allocation fails
   */
  RandTree(unsigned long seed)
  {
    r = gsl_rng_alloc (gsl_rng_mt19937);
    if (r == nullptr)
      ah_bad_alloc_if(true);
    gsl_rng_set(r, seed % gsl_rng_max(r));
  }

  /**
   * @brief Construct generator with time-based seed.
   *
   * Uses current time as seed for non-reproducible random trees.
   */
  RandTree() : RandTree(time(nullptr)) {}

  /**
   * @brief Destructor releases GSL resources.
   */
  ~RandTree()
  {
    if (r != nullptr)
      gsl_rng_free(r);
  }

  /**
   * @brief Generate a random tree with n nodes.
   *
   * @param n Number of nodes in the tree (must be > 0)
   * @return Pointer to root of newly allocated tree
   *
   * @pre n > 0
   * @note Caller is responsible for freeing the returned tree
   *
   * @par Example
   * @code
   * RandTree<int> gen(42);
   * Tree_Node<int>* tree = gen(100);
   * // ... use tree ...
   * destroyRec(tree);  // Don't forget to clean up!
   * @endcode
   */
  Tree_Node<T> * operator () (size_t n)
  {
    assert(n != 0);
    BinNode<T> * l = random(n - 1); // only left side
    BinNode<T> broot;
    broot.getL() = l;
    Tree_Node<T> * root = new Tree_Node<T>;
    bin_to_tree(&broot, root);
    destroyRec(l);
    return root;
  }
};




#endif
