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


/** @file tpl_cartesian_tree.H
 *  @brief Cartesian Tree, LCA via Euler Tour, and RMQ via Cartesian Tree.
 *
 *  This file implements the classic chain of reductions:
 *
 *  @code
 *  RMQ  <-->  LCA  <-->  Cartesian Tree
 *  @endcode
 *
 *  Three class templates are provided:
 *
 *  - **Gen_Cartesian_Tree\<T, Comp\>** — Explicit Cartesian Tree built
 *    in O(n) with a monotonic stack.  The tree satisfies the heap
 *    property under @p Comp and the inorder traversal reproduces the
 *    original array.
 *  - **Gen_Euler_Tour_LCA\<T, Comp\>** — Lowest Common Ancestor in O(1)
 *    via Euler Tour + Sparse Table on the Cartesian Tree.
 *  - **Gen_Cartesian_Tree_RMQ\<T, Comp\>** — O(1) static range queries
 *    via the reduction RMQ -> LCA -> Cartesian Tree.
 *
 *  ## Complexity
 *  | Class                  | Build        | Query  | Space        |
 *  |------------------------|-------------|--------|-------------|
 *  | Gen_Cartesian_Tree     | O(n)        | —      | O(n)        |
 *  | Gen_Euler_Tour_LCA     | O(n log n)* | O(1)   | O(n log n)* |
 *  | Gen_Cartesian_Tree_RMQ | O(n log n)* | O(1)   | O(n log n)* |
 *
 *  *Dominated by the internal Sparse Table.
 *
 *  @see tpl_sparse_table.H   Sparse Table for static O(1) range queries.
 *  @see tpl_segment_tree.H   Dynamic range queries with updates.
 *  @see tpl_fenwick_tree.H   Prefix-based range queries.
 *  @ingroup Sequences
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_CARTESIAN_TREE_H
# define TPL_CARTESIAN_TREE_H

# include <cassert>
# include <cstddef>
# include <initializer_list>
# include <type_traits>
# include <utility>
# include <vector>
# include <tpl_array.H>
# include <tpl_dynList.H>
# include <tpl_sparse_table.H>
# include <ahFunction.H>
# include <ah-errors.H>
# include <ah-concepts.H>

namespace Aleph
{
  /** @brief Explicit Cartesian Tree built in O(n) with a monotonic stack.
   *
   *  A Cartesian Tree over an array `a[0..n-1]` is a binary tree where:
   *  - The **heap property** holds: no child is strictly better than its
   *    parent under @p Comp, i.e. `!Comp(a[child], a[parent])` for every
   *    parent–child pair (min-heap by default; equality is allowed).
   *  - The **inorder traversal** reproduces the original array order
   *    `0, 1, 2, ..., n-1`.
   *
   *  The tree is represented implicitly with three parallel arrays
   *  (`parent`, `left`, `right`) indexed by position in the original
   *  array.  A sentinel value `NONE = size_t(-1)` marks absent links.
   *
   *  @tparam T     element type.
   *  @tparam Comp  binary comparator defining the heap property.
   *
   *  @par Example
   *  @code
   *  Cartesian_Tree<int> ct = {3, 2, 6, 1, 9};
   *  // Root is at index 3 (value 1, the minimum)
   *  assert(ct.root() == 3);
   *  assert(ct.data_at(ct.root()) == 1);
   *  // Inorder traversal = {0, 1, 2, 3, 4}
   *  auto io = ct.inorder();
   *  for (size_t i = 0; i < io.size(); ++i)
   *    assert(io(i) == i);
   *  @endcode
   *
   *  @ingroup Sequences
   */
  template <typename T, class Comp>
    requires StrictWeakOrder<Comp, T>
  class Gen_Cartesian_Tree
  {
  public:
    /// Sentinel value for absent parent/child links.
    static constexpr size_t NONE = static_cast<size_t>(-1);

  private:
    Array<T> data_; // original values
    Array<size_t> parent_; // parent[i] = parent of node i
    Array<size_t> left_; // left[i]  = left child of node i
    Array<size_t> right_; // right[i] = right child of node i
    size_t root_; // index of the root node
    size_t n_; // number of elements

    Comp comp_;

    /// Build the Cartesian Tree using a monotonic stack in O(n).
    void build()
    {
      if (n_ == 0)
        {
          root_ = NONE;
          return;
        }

      // Stack stored in a plain array — at most n elements
      auto stk = Array<size_t>::create(n_);
      size_t top = 0; // stack size (top points to next free slot)

      for (size_t i = 0; i < n_; ++i)
        {
          size_t last_popped = NONE;

          // Pop while i is strictly better than stack top (stable on ties)
          while (top > 0 and comp_(data_(i), data_(stk(top - 1))))
            {
              last_popped = stk(top - 1);
              --top;
            }

          // The last popped node becomes the left child of i
          left_(i) = last_popped;
          if (last_popped != NONE)
            parent_(last_popped) = i;

          // i becomes right child of the current stack top
          if (top > 0)
            {
              right_(stk(top - 1)) = i;
              parent_(i) = stk(top - 1);
            }
          else
            parent_(i) = NONE;

          stk(top++) = i;
        }

      root_ = stk(0); // the bottom of the stack is the root
    }

    /// Initialize arrays of size n with NONE and build.
    void init_and_build()
    {
      parent_ = Array<size_t>::create(n_);
      left_ = Array<size_t>::create(n_);
      right_ = Array<size_t>::create(n_);
      for (size_t i = 0; i < n_; ++i)
        {
          parent_(i) = NONE;
          left_(i) = NONE;
          right_(i) = NONE;
        }
      build();
    }

  public:
    /// The type of the element stored in the tree.
    using Item_Type = T;

    /** @brief Construct from an Array\<T\>.
     *  @param values source array.
     *  @param c      comparator (default-constructed).
     */
    Gen_Cartesian_Tree(const Array<T> & values, Comp c = Comp())
      : data_(values), root_(NONE), n_(values.size()), comp_(c)
    {
      init_and_build();
    }

    /** @brief Construct from a std::vector\<T\>.
     *  @param values source vector.
     *  @param c      comparator.
     */
    Gen_Cartesian_Tree(const std::vector<T> & values, Comp c = Comp())
      : data_(Array<T>::create(values.size())), root_(NONE),
        n_(values.size()), comp_(c)
    {
      for (size_t i = 0; i < n_; ++i)
        data_(i) = values[i];
      init_and_build();
    }

    /** @brief Construct from an initializer_list\<T\>.
     *  @param il  initializer list.
     *  @param c   comparator.
     */
    Gen_Cartesian_Tree(std::initializer_list<T> il, Comp c = Comp())
      : data_(Array<T>::create(il.size())), root_(NONE),
        n_(il.size()), comp_(c)
    {
      size_t i = 0;
      for (auto it = il.begin(); it != il.end(); ++it)
        data_(i++) = *it;
      init_and_build();
    }

    /** @brief Construct from a DynList\<T\>.
     *  @param values source list.
     *  @param c      comparator.
     */
    Gen_Cartesian_Tree(const DynList<T> & values, Comp c = Comp())
      : data_(Array<T>::create(values.size())), root_(NONE),
        n_(values.size()), comp_(c)
    {
      size_t i = 0;
      for (auto it = values.get_it(); it.has_curr(); it.next_ne())
        data_(i++) = it.get_curr();
      init_and_build();
    }

    /** @brief Construct with @p num elements all equal to @p init.
     *  @param num   number of elements.
     *  @param init  initial value for every position.
     *  @param c     comparator.
     */
    Gen_Cartesian_Tree(const size_t num, const T & init, Comp c = Comp())
      : data_(Array<T>::create(num)), root_(NONE), n_(num), comp_(c)
    {
      for (size_t i = 0; i < n_; ++i)
        data_(i) = init;
      init_and_build();
    }

    Gen_Cartesian_Tree(const Gen_Cartesian_Tree &) = default;

    Gen_Cartesian_Tree &operator=(const Gen_Cartesian_Tree &) = default;

    Gen_Cartesian_Tree(Gen_Cartesian_Tree &&) noexcept(
      std::is_nothrow_move_constructible_v<Array<T>> and
      std::is_nothrow_move_constructible_v<Comp>) = default;

    Gen_Cartesian_Tree &operator=(Gen_Cartesian_Tree &&) noexcept(
      std::is_nothrow_move_assignable_v<Array<T>> and
      std::is_nothrow_move_assignable_v<Comp>) = default;

    /** @brief Index of the root node.
     *  @return root index, or NONE if empty.
     */
    [[nodiscard]] constexpr size_t root() const noexcept { return root_; }

    /** @brief Left child of node @p i.
     *  @throw std::out_of_range if `i >= size()`.
     */
    size_t left_child(const size_t i) const
    {
      ah_out_of_range_error_if(i >= n_)
        << "Gen_Cartesian_Tree::left_child: i=" << i << " >= n=" << n_;
      return left_(i);
    }

    /** @brief Right child of node @p i.
     *  @throw std::out_of_range if `i >= size()`.
     */
    size_t right_child(const size_t i) const
    {
      ah_out_of_range_error_if(i >= n_)
        << "Gen_Cartesian_Tree::right_child: i=" << i << " >= n=" << n_;
      return right_(i);
    }

    /** @brief Parent of node @p i.
     *  @throw std::out_of_range if `i >= size()`.
     */
    size_t parent_of(const size_t i) const
    {
      ah_out_of_range_error_if(i >= n_)
        << "Gen_Cartesian_Tree::parent_of: i=" << i << " >= n=" << n_;
      return parent_(i);
    }

    /** @brief Value at node @p i.
     *  @throw std::out_of_range if `i >= size()`.
     */
    const T &data_at(const size_t i) const
    {
      ah_out_of_range_error_if(i >= n_)
        << "Gen_Cartesian_Tree::data_at: i=" << i << " >= n=" << n_;
      return data_(i);
    }

    /** @brief Number of nodes. */
    [[nodiscard]] constexpr size_t size() const noexcept { return n_; }

    /** @brief True if the tree is empty. */
    [[nodiscard]] constexpr bool is_empty() const noexcept { return n_ == 0; }

    /** @brief True if node @p i is a leaf (no children).
     *  @throw std::out_of_range if `i >= size()`.
     */
    bool is_leaf(const size_t i) const
    {
      ah_out_of_range_error_if(i >= n_)
        << "Gen_Cartesian_Tree::is_leaf: i=" << i << " >= n=" << n_;
      return left_(i) == NONE and right_(i) == NONE;
    }

    /** @brief True if node @p i is the root. */
    bool is_root(const size_t i) const
    {
      ah_out_of_range_error_if(i >= n_)
        << "Gen_Cartesian_Tree::is_root: i=" << i << " >= n=" << n_;
      return i == root_;
    }

    /** @brief Height of the tree (longest root-to-leaf path).
     *
     *  @return Height in number of nodes on the longest root-to-leaf path
     *          (0 if empty, 1 for a single-node tree). Returned by value.
     *  @throw std::bad_alloc if temporary DFS stack allocation fails.
     *  @note Complexity: O(n) time and O(n) auxiliary space (explicit stack).
     */
    [[nodiscard]] size_t height() const
    {
      if (n_ == 0)
        return 0;

      size_t max_depth = 0;

      // DFS with explicit stack: (node, depth)
      struct Entry
      {
        size_t node;
        size_t depth;
      };
      auto stk = Array<Entry>::create(n_);
      size_t top = 0;
      stk(top++) = {root_, 1};

      while (top > 0)
        {
          auto [node, depth] = stk(--top);
          if (depth > max_depth)
            max_depth = depth;
          if (right_(node) != NONE)
            stk(top++) = {right_(node), depth + 1};
          if (left_(node) != NONE)
            stk(top++) = {left_(node), depth + 1};
        }

      return max_depth;
    }

    /** @brief Inorder traversal of the tree.
     *
     *  @return A new `Array<size_t>` with node indices in inorder.
     *          Ownership is transferred by value to the caller.
     *          For a correctly built Cartesian Tree, this is
     *          `{0, 1, ..., n-1}`. If the tree is empty, returns an empty
     *          array.
     *  @throw std::bad_alloc if result/stack allocation fails.
     *  @note Complexity: O(n) time and O(n) space
     *        (result array + traversal stack).
     */
    [[nodiscard]] Array<size_t> inorder() const
    {
      if (n_ == 0)
        return {};

      auto result = Array<size_t>::create(n_);
      size_t idx = 0;

      // Iterative inorder with explicit stack
      auto stk = Array<size_t>::create(n_);
      size_t top = 0;
      size_t curr = root_;

      while (curr != NONE or top > 0)
        {
          while (curr != NONE)
            {
              stk(top++) = curr;
              curr = left_(curr);
            }
          curr = stk(--top);
          result(idx++) = curr;
          curr = right_(curr);
        }

      return result;
    }

    /** @brief Copy of the original data array.
     *
     *  @return A new `Array<T>` with all stored values in original index
     *          order. Ownership is transferred by value to the caller.
     *          If the tree is empty, returns an empty array.
     *  @throw std::bad_alloc if array allocation fails.
     *  @throw Any exception thrown by `T` copy assignment during element copy.
     *  @note Complexity: O(n) time and O(n) space.
     */
    [[nodiscard]] Array<T> values() const
    {
      auto ret = Array<T>::create(n_);
      for (size_t i = 0; i < n_; ++i)
        ret(i) = data_(i);
      return ret;
    }

    /** @brief Swap with @p other in O(1). */
    void swap(Gen_Cartesian_Tree & other) noexcept
    {
      data_.swap(other.data_);
      parent_.swap(other.parent_);
      left_.swap(other.left_);
      right_.swap(other.right_);
      std::swap(root_, other.root_);
      std::swap(n_, other.n_);
      std::swap(comp_, other.comp_);
    }
  };

  /** @brief Cartesian Tree for min-heap (range minimum).
   *  @tparam T a totally ordered type.
   *  @ingroup Sequences
   */
  template <std::totally_ordered T>
  struct Cartesian_Tree
      : public Gen_Cartesian_Tree<T, Aleph::less<T>>
  {
    using Base = Gen_Cartesian_Tree<T, Aleph::less<T>>;
    using Base::Base;
  };

  /** @brief Cartesian Tree for max-heap (range maximum).
   *  @tparam T a totally ordered type.
   *  @ingroup Sequences
   */
  template <std::totally_ordered T>
  struct Max_Cartesian_Tree
      : public Gen_Cartesian_Tree<T, Aleph::greater<T>>
  {
    using Base = Gen_Cartesian_Tree<T, Aleph::greater<T>>;
    using Base::Base;
  };


  // ================================================================
  // Gen_Euler_Tour_LCA — LCA in O(1) via Euler Tour + Sparse Table
  // ================================================================

  /** @brief Lowest Common Ancestor queries in O(1) via Euler Tour
   *         reduction to RMQ on a Cartesian Tree.
   *
   *  Given an array, this class:
   *  1. Builds a Cartesian Tree in O(n).
   *  2. Performs an Euler Tour of the tree, producing `euler[]` (2n-1
   *     node visits) and `depth[]` arrays.
   *  3. Builds a Sparse Table over the depth array for O(1) RMQ.
   *
   *  The LCA of two nodes u, v corresponds to the minimum-depth node
   *  in the Euler Tour between the first occurrences of u and v.
   *
   *  @tparam T     element type.
   *  @tparam Comp  comparator for the Cartesian Tree heap property.
   *
   *  @par Example
   *  @code
   *  Euler_Tour_LCA<int> lca = {3, 2, 6, 1, 9};
   *  size_t ancestor = lca.lca(0, 2);  // LCA of positions 0 and 2
   *  @endcode
   *
   *  @ingroup Sequences
   */
  template <typename T, class Comp>
    requires StrictWeakOrder<Comp, T>
  class Gen_Euler_Tour_LCA
  {
    /// Entry for the depth-based Sparse Table: (depth, euler_node_index).
    struct DepthEntry
    {
      size_t depth;
      size_t node;
    };

    /// Min-op on DepthEntry: compares by depth.
    struct DepthMinOp
    {
      constexpr DepthEntry operator()(const DepthEntry & a,
                                      const DepthEntry & b) const noexcept
      {
        return a.depth <= b.depth ? a : b;
      }
    };

    Gen_Cartesian_Tree<T, Comp> tree_;
    Array<size_t> euler_; // euler tour: 2n-1 node indices
    Array<size_t> depth_arr_; // depth at each euler tour position
    Array<size_t> first_; // first_[node] = first position in euler
    Array<size_t> node_depth_; // depth of each node in the tree
    Gen_Sparse_Table<DepthEntry, DepthMinOp> sparse_;
    size_t euler_size_;

    /// Build Euler Tour iteratively using an explicit stack with phases.
    void build_euler_tour()
    {
      const size_t n = tree_.size();
      if (n == 0)
        return;

      constexpr size_t NONE = Gen_Cartesian_Tree<T, Comp>::NONE;

      euler_size_ = 2 * n - 1;
      euler_ = Array<size_t>::create(euler_size_);
      depth_arr_ = Array<size_t>::create(euler_size_);
      first_ = Array<size_t>::create(n);
      node_depth_ = Array<size_t>::create(n);

      for (size_t i = 0; i < n; ++i)
        first_(i) = NONE;

      // Stack entries: (node, phase, depth)
      // phase 0: first visit — record euler, push left child
      // phase 1: back from left — if left existed, record euler;
      //          push right child
      // phase 2: back from right — if right existed, record euler;
      //          pop
      struct Frame
      {
        size_t node;
        int phase;
        size_t depth;
      };
      auto stk = Array<Frame>::create(n);
      size_t top = 0;
      size_t eidx = 0;

      stk(top++) = {tree_.root(), 0, 0};

      while (top > 0)
        {
          if (auto & frame = stk(top - 1); frame.phase == 0)
            {
              // First visit — always record
              euler_(eidx) = frame.node;
              depth_arr_(eidx) = frame.depth;
              node_depth_(frame.node) = frame.depth;
              first_(frame.node) = eidx;
              ++eidx;
              frame.phase = 1;

              if (size_t lc = tree_.left_child(frame.node); lc != NONE)
                stk(top++) = {lc, 0, frame.depth + 1};
            }
          else if (frame.phase == 1)
            {
              // Back from left subtree
              if (const size_t lc = tree_.left_child(frame.node); lc != NONE)
                {
                  // We actually returned from left child — record again
                  euler_(eidx) = frame.node;
                  depth_arr_(eidx) = frame.depth;
                  ++eidx;
                }
              frame.phase = 2;

              if (size_t rc = tree_.right_child(frame.node); rc != NONE)
                stk(top++) = {rc, 0, frame.depth + 1};
            }
          else
            {
              // Back from right subtree
              if (const size_t rc = tree_.right_child(frame.node); rc != NONE)
                {
                  // We actually returned from right child — record again
                  euler_(eidx) = frame.node;
                  depth_arr_(eidx) = frame.depth;
                  ++eidx;
                }
              --top;
            }
        }

      assert(eidx == euler_size_);
    }

    /// Build the Sparse Table over the depth entries.
    void build_sparse_table()
    {
      if (euler_size_ == 0)
        return;

      auto entries = Array<DepthEntry>::create(euler_size_);
      for (size_t i = 0; i < euler_size_; ++i)
        entries(i) = {depth_arr_(i), euler_(i)};

      sparse_ = Gen_Sparse_Table<DepthEntry, DepthMinOp>(entries);
    }

  public:
    /** @brief Construct from an Array\<T\>.
     *  @param values source array.
     *  @param c      comparator.
     */
    Gen_Euler_Tour_LCA(const Array<T> & values, Comp c = Comp())
      : tree_(values, c), sparse_({DepthEntry{0, 0}}), euler_size_(0)
    {
      build_euler_tour();
      build_sparse_table();
    }

    /** @brief Construct from a std::vector\<T\>. */
    Gen_Euler_Tour_LCA(const std::vector<T> & values, Comp c = Comp())
      : tree_(values, c), sparse_({DepthEntry{0, 0}}), euler_size_(0)
    {
      build_euler_tour();
      build_sparse_table();
    }

    /** @brief Construct from an initializer_list\<T\>. */
    Gen_Euler_Tour_LCA(std::initializer_list<T> il, Comp c = Comp())
      : tree_(il, c), sparse_({DepthEntry{0, 0}}), euler_size_(0)
    {
      build_euler_tour();
      build_sparse_table();
    }

    /** @brief Construct from a DynList\<T\>. */
    Gen_Euler_Tour_LCA(const DynList<T> & values, Comp c = Comp())
      : tree_(values, c), sparse_({DepthEntry{0, 0}}), euler_size_(0)
    {
      build_euler_tour();
      build_sparse_table();
    }

    /** @brief Construct with @p num elements all equal to @p init. */
    Gen_Euler_Tour_LCA(const size_t num, const T & init, Comp c = Comp())
      : tree_(num, init, c), sparse_({DepthEntry{0, 0}}), euler_size_(0)
    {
      build_euler_tour();
      build_sparse_table();
    }

    /** @brief Lowest Common Ancestor of nodes @p u and @p v in O(1).
     *
     *  Returns the index of the LCA node in the original array.
     *
     *  @param u  0-based node index.
     *  @param v  0-based node index.
     *  @throw std::out_of_range if u or v are out of range.
     */
    size_t lca(const size_t u, const size_t v) const
    {
      const size_t n = tree_.size();
      ah_out_of_range_error_if(u >= n)
        << "Gen_Euler_Tour_LCA::lca: u=" << u << " >= n=" << n;
      ah_out_of_range_error_if(v >= n)
        << "Gen_Euler_Tour_LCA::lca: v=" << v << " >= n=" << n;

      size_t l = first_(u);
      size_t r = first_(v);
      if (l > r)
        std::swap(l, r);

      return sparse_.query(l, r).node;
    }

    /** @brief Depth of node @p u in the Cartesian Tree.
     *  @throw std::out_of_range if u >= size().
     */
    size_t depth_of(const size_t u) const
    {
      ah_out_of_range_error_if(u >= tree_.size())
        << "Gen_Euler_Tour_LCA::depth_of: u=" << u << " >= n=" << tree_.size();
      return node_depth_(u);
    }

    /** @brief Distance between nodes @p u and @p v in the tree.
     *
     *  Equals `depth(u) + depth(v) - 2 * depth(lca(u, v))`.
     */
    size_t distance(const size_t u, const size_t v) const
    {
      const size_t ancestor = lca(u, v);
      return node_depth_(u) + node_depth_(v) - 2 * node_depth_(ancestor);
    }

    /** @brief Number of nodes. */
    [[nodiscard]] constexpr size_t size() const noexcept
    {
      return tree_.size();
    }

    /** @brief True if empty. */
    [[nodiscard]] constexpr bool is_empty() const noexcept
    {
      return tree_.is_empty();
    }

    /** @brief Const reference to the internal Cartesian Tree. */
    const Gen_Cartesian_Tree<T, Comp> &tree() const noexcept
    {
      return tree_;
    }

    /** @brief Copy of the Euler Tour array (for inspection/debugging). */
    [[nodiscard]] Array<size_t> euler_tour() const
    {
      auto ret = Array<size_t>::create(euler_size_);
      for (size_t i = 0; i < euler_size_; ++i)
        ret(i) = euler_(i);
      return ret;
    }

    /** @brief Size of the Euler Tour (should be 2n-1 for n > 0). */
    [[nodiscard]] size_t euler_tour_size() const noexcept
    {
      return euler_size_;
    }
  };

  /** @brief LCA on a min-heap Cartesian Tree.
   *  @tparam T a totally ordered type.
   *  @ingroup Sequences
   */
  template <std::totally_ordered T>
  struct Euler_Tour_LCA
      : public Gen_Euler_Tour_LCA<T, Aleph::less<T>>
  {
    using Base = Gen_Euler_Tour_LCA<T, Aleph::less<T>>;
    using Base::Base;
  };

  /** @brief LCA on a max-heap Cartesian Tree.
   *  @tparam T a totally ordered type.
   *  @ingroup Sequences
   */
  template <std::totally_ordered T>
  struct Max_Euler_Tour_LCA
      : public Gen_Euler_Tour_LCA<T, Aleph::greater<T>>
  {
    using Base = Gen_Euler_Tour_LCA<T, Aleph::greater<T>>;
    using Base::Base;
  };


  // ================================================================
  // Gen_Cartesian_Tree_RMQ — O(1) RMQ via Cartesian Tree + LCA
  // ================================================================

  /** @brief O(1) static range queries via the Cartesian Tree reduction.
   *
   *  Wraps `Gen_Euler_Tour_LCA` and exposes a range query interface
   *  compatible with `Gen_Sparse_Table`:
   *
   *  `query(l, r)` returns the optimal value in `a[l..r]` under @p Comp.
   *  `query_idx(l, r)` returns the index of that optimal value.
   *
   *  The key insight: `RMQ(l, r) = data_at(LCA(l, r))` because the
   *  Cartesian Tree's root in any subrange is the position of the
   *  optimal element.
   *
   *  @tparam T     element type.
   *  @tparam Comp  comparator for the heap property.
   *
   *  @par Example
   *  @code
   *  Cartesian_Tree_RMQ<int> rmq = {5, 2, 4, 7, 1, 3, 6};
   *  assert(rmq.query(0, 3) == 2);  // min(5, 2, 4, 7)
   *  assert(rmq.query(2, 6) == 1);  // min(4, 7, 1, 3, 6)
   *  assert(rmq.query_idx(2, 6) == 4);  // index of min
   *  @endcode
   *
   *  @ingroup Sequences
   */
  template <typename T, class Comp>
    requires StrictWeakOrder<Comp, T>
  class Gen_Cartesian_Tree_RMQ
  {
    Gen_Euler_Tour_LCA<T, Comp> lca_;

  public:
    /// The type of the element.
    using Item_Type = T;

    /** @brief Construct from an Array\<T\>. */
    Gen_Cartesian_Tree_RMQ(const Array<T> & values, Comp c = Comp())
      : lca_(values, c) {}

    /** @brief Construct from a std::vector\<T\>. */
    Gen_Cartesian_Tree_RMQ(const std::vector<T> & values, Comp c = Comp())
      : lca_(values, c) {}

    /** @brief Construct from an initializer_list\<T\>. */
    Gen_Cartesian_Tree_RMQ(std::initializer_list<T> il, Comp c = Comp())
      : lca_(il, c) {}

    /** @brief Construct from a DynList\<T\>. */
    Gen_Cartesian_Tree_RMQ(const DynList<T> & values, Comp c = Comp())
      : lca_(values, c) {}

    /** @brief Construct with @p num elements all equal to @p init. */
    Gen_Cartesian_Tree_RMQ(const size_t num, const T & init,
                           Comp c = Comp())
      : lca_(num, init, c) {}

    /** @brief Range query over `[l, r]` in O(1).
     *
     *  Returns the optimal value (min or max depending on Comp)
     *  in positions `l..r` of the original array.
     *
     *  @param l 0-based left index (inclusive).
     *  @param r 0-based right index (inclusive).
     *  @throw std::out_of_range if `l > r` or `r >= size()`.
     */
    T query(const size_t l, const size_t r) const
    {
      ah_out_of_range_error_if(r >= lca_.size())
        << "Gen_Cartesian_Tree_RMQ::query: r=" << r << " >= n=" << lca_.size();
      ah_out_of_range_error_if(l > r)
        << "Gen_Cartesian_Tree_RMQ::query: l=" << l << " > r=" << r;

      return lca_.tree().data_at(lca_.lca(l, r));
    }

    /** @brief Return the index of the optimal element in `a[l..r]`.
     *
     *  @param l 0-based left index (inclusive).
     *  @param r 0-based right index (inclusive).
     *  @throw std::out_of_range if `l > r` or `r >= size()`.
     *
     *  When the optimal value occurs multiple times in the range, this
     *  returns the leftmost occurrence (stable tie-break).
     */
    size_t query_idx(const size_t l, const size_t r) const
    {
      ah_out_of_range_error_if(r >= lca_.size())
        << "Gen_Cartesian_Tree_RMQ::query_idx: r=" << r << " >= n=" << lca_.size();
      ah_out_of_range_error_if(l > r)
        << "Gen_Cartesian_Tree_RMQ::query_idx: l=" << l << " > r=" << r;

      return lca_.lca(l, r);
    }

    /** @brief Retrieve the value at position @p i in O(1). */
    T get(const size_t i) const
    {
      return lca_.tree().data_at(i);
    }

    /** @brief Number of elements. */
    [[nodiscard]] constexpr size_t size() const noexcept
    {
      return lca_.size();
    }

    /** @brief True if empty. */
    [[nodiscard]] constexpr bool is_empty() const noexcept
    {
      return lca_.is_empty();
    }

    /** @brief Copy of the original values. */
    [[nodiscard]] Array<T> values() const
    {
      return lca_.tree().values();
    }

    /** @brief Const reference to the internal LCA engine. */
    const Gen_Euler_Tour_LCA<T, Comp> &lca_engine() const noexcept
    {
      return lca_;
    }
  };

  /** @brief Range minimum queries via Cartesian Tree.
   *  @tparam T a totally ordered type.
   *  @ingroup Sequences
   */
  template <std::totally_ordered T>
  struct Cartesian_Tree_RMQ
      : public Gen_Cartesian_Tree_RMQ<T, Aleph::less<T>>
  {
    using Base = Gen_Cartesian_Tree_RMQ<T, Aleph::less<T>>;
    using Base::Base;
  };

  /** @brief Range maximum queries via Cartesian Tree.
   *  @tparam T a totally ordered type.
   *  @ingroup Sequences
   */
  template <std::totally_ordered T>
  struct Cartesian_Tree_RMaxQ
      : public Gen_Cartesian_Tree_RMQ<T, Aleph::greater<T>>
  {
    using Base = Gen_Cartesian_Tree_RMQ<T, Aleph::greater<T>>;
    using Base::Base;
  };
} // end namespace Aleph

# endif /* TPL_CARTESIAN_TREE_H */
