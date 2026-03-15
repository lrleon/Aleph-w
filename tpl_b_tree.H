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

/** @file tpl_b_tree.H
 *  @brief B-Tree implementation with configurable minimum degree.
 *
 *  This header provides \ref Aleph::B_Tree, a balanced multiway search tree
 *  suitable for ordered-set workloads and for experimenting with disk/page-
 *  oriented tree layouts. Keys are stored directly in every node and all leaves
 *  remain at the same depth.
 *
 *  The implementation follows the usual B-Tree invariants:
 *  - each node stores between `t - 1` and `2t - 1` keys (except the root)
 *  - each internal node with `k` keys owns `k + 1` children
 *  - keys are kept sorted in every node
 *  - all leaves appear at the same depth
 *
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

#ifndef TPL_B_TREE_H
#define TPL_B_TREE_H

#include <algorithm>
#include <concepts>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <optional>
#include <utility>

#include <ah-concepts.H>
#include <ah-errors.H>
#include <ahFunction.H>
#include <tpl_array.H>
#include <tpl_sort_utils.H>

namespace Aleph {
namespace detail {
template <typename Compare, typename Key> [[nodiscard]] bool equiv(const Compare &cmp, const Key &lhs, const Key &rhs)
{
  return not cmp(lhs, rhs) and not cmp(rhs, lhs);
}

template <typename Key> struct BTreeNode
{
  bool                              leaf = true;
  Array<Key>                        keys;
  Array<std::unique_ptr<BTreeNode>> children;

  explicit BTreeNode(const bool is_leaf) : leaf(is_leaf) {}
};
}  // namespace detail

/** @brief Generic B-Tree with configurable minimum degree.
 *
 *  `Gen_B_Tree` models an ordered set of unique keys. Unlike the raw
 *  binary-tree types in Aleph-w, this container owns its internal nodes and
 *  stores keys directly in them, which makes it a higher-level tree suited
 *  for page-like branching factors.
 *
 *  @tparam Key Key type stored in the tree.
 *  @tparam Compare Strict weak ordering used to sort keys.
 *  @tparam MinDegree Minimum degree `t` of the B-Tree. Valid values are `t >= 2`.
 *
 *  @par Complexity
 *  - Search: O(log n)
 *  - Insert: O(log n)
 *  - Remove: O(log n)
 *  - lower_bound(): O(log n)
 *  - upper_bound(): O(log n)
 *  - keys(): O(n)
 *
 *  @par Example
 *  @code
 *  B_Tree<int> tree;
 *  tree.insert(40);
 *  tree.insert(10);
 *  tree.insert(90);
 *
 *  if (tree.contains(10))
 *    std::cout << "found\n";
 *
 *  tree.remove(40);
 *  auto ordered = tree.keys();
 *  @endcode
 *
 *  @note This tree stores unique keys. Duplicate inserts return `false`.
 *  @note For leaf-linked range scans, see \ref Aleph::BPlus_Tree.
 *
 *  @par Thread Safety
 *  Instances are not synchronized. Concurrent access to the same tree
 *  requires external synchronization.
 *
 *  @par Exception Safety
 *  Read-only operations do not allocate. Mutating operations preserve the
 *  tree invariants if `Compare` and `Key` operations do not throw in the
 *  middle of internal node updates; allocation failures propagate as
 *  `std::bad_alloc`.
 */
template <typename Key,
          class Compare = Aleph::less<Key>,
          size_t MinDegree = 16>
  requires StrictWeakOrder<Compare, Key>
class Gen_B_Tree
{
  static_assert(MinDegree >= 2, "B_Tree requires MinDegree >= 2");
  static_assert(std::copy_constructible<Key>, "B_Tree requires copy-constructible keys");

  using Node = detail::BTreeNode<Key>;

  static constexpr size_t MaxKeys = 2 * MinDegree - 1;
  static constexpr size_t MinKeys = MinDegree - 1;

  std::unique_ptr<Node> root_;
  size_t                size_ = 0;
  Compare               cmp_;

  [[nodiscard]] size_t lower_bound_index(const Array<Key> &keys, const Key &key) const
  {
    size_t idx = 0;
    while (idx < keys.size() and cmp_(keys[idx], key))
      ++idx;
    return idx;
  }

  [[nodiscard]] size_t upper_bound_index(const Array<Key> &keys, const Key &key) const
  {
    size_t idx = 0;
    while (idx < keys.size() and (cmp_(keys[idx], key) or equals(keys[idx], key)))
      ++idx;
    return idx;
  }

  [[nodiscard]] bool equals(const Key &lhs, const Key &rhs) const
  {
    return detail::equiv(cmp_, lhs, rhs);
  }

  template <std::input_iterator It> void insert_range(It first, It last)
  {
    for (; first != last; ++first)
      insert(*first);
  }

  template <typename T, typename U> static void insert_at(Array<T> &array, const size_t idx, U &&value)
  {
    if (idx == array.size())
      {
        array.append(std::forward<U>(value));
        return;
      }

    array.putn(1);
    T *base = &array[0];
    Aleph::open_gap(base, array.size(), idx, 1);
    array(idx) = std::forward<U>(value);
  }

  template <typename T> static T erase_at(Array<T> &array, const size_t idx)
  {
    T ret = std::move(array(idx));
    if (idx + 1 < array.size())
      {
        T *base = &array[0];
        Aleph::close_gap(base, array.size(), idx, 1);
      }
    (void) array.remove_last();
    return ret;
  }

  template <typename T> static void truncate(Array<T> &array, const size_t new_size)
  {
    while (array.size() > new_size)
      (void) array.remove_last();
  }

  template <typename T> static void append_copy_range(Array<T> &dst, const Array<T> &src, const size_t from)
  {
    dst.reserve(dst.size() + src.size() - from);
    for (size_t i = from; i < src.size(); ++i)
      dst.append(src[i]);
  }

  template <typename T> static void append_move_range(Array<T> &dst, Array<T> &src, const size_t from)
  {
    dst.reserve(dst.size() + src.size() - from);
    for (size_t i = from; i < src.size(); ++i)
      dst.append(std::move(src(i)));
  }

  [[nodiscard]] std::unique_ptr<Node> clone_node(const Node *node) const
  {
    if (node == nullptr)
      return nullptr;

    auto copy  = std::make_unique<Node>(node->leaf);
    copy->keys = node->keys;
    if (not node->leaf)
      {
        copy->children.reserve(node->children.size());
        for (const auto &child : node->children)
          copy->children.append(clone_node(child.get()));
      }
    return copy;
  }

  [[nodiscard]] size_t height_of(const Node *node) const noexcept
  {
    size_t height = 0;
    for (const Node *curr = node; curr != nullptr;)
      {
        ++height;
        if (curr->leaf or curr->children.is_empty())
          break;
        curr = curr->children.get_first().get();
      }
    return height;
  }

  [[nodiscard]] bool contains_in(const Node *node, const Key &key) const
  {
    const Node *curr = node;
    while (curr != nullptr)
      {
        const size_t idx = lower_bound_index(curr->keys, key);
        if (idx < curr->keys.size() and equals(curr->keys[idx], key))
          return true;
        if (curr->leaf)
          return false;
        curr = curr->children[idx].get();
      }
    return false;
  }

  void split_child(Node *parent, const size_t idx)
  {
    Node *child = parent->children[idx].get();
    auto  right = std::make_unique<Node>(child->leaf);

    const Key median = child->keys[MinKeys];

    append_copy_range(right->keys, child->keys, MinDegree);
    truncate(child->keys, MinKeys);

    if (not child->leaf)
      {
        append_move_range(right->children, child->children, MinDegree);
        truncate(child->children, MinDegree);
      }

    insert_at(parent->keys, idx, median);
    insert_at(parent->children, idx + 1, std::move(right));
  }

  void insert_nonfull(Node *node, const Key &key)
  {
    if (node->leaf)
      {
        const size_t idx = lower_bound_index(node->keys, key);
        insert_at(node->keys, idx, key);
        return;
      }

    size_t idx = lower_bound_index(node->keys, key);
    if (node->children[idx]->keys.size() == MaxKeys)
      {
        split_child(node, idx);
        if (cmp_(node->keys[idx], key))
          ++idx;
      }
    insert_nonfull(node->children[idx].get(), key);
  }

  [[nodiscard]] std::optional<Key> min_in(const Node *node) const
  {
    if (node == nullptr)
      return std::nullopt;

    const Node *curr = node;
    while (curr != nullptr and not curr->leaf)
      {
        if (curr->children.is_empty())
          return std::nullopt;
        curr = curr->children.get_first().get();
      }

    if (curr == nullptr or curr->keys.is_empty())
      return std::nullopt;

    return curr->keys.get_first();
  }

  [[nodiscard]] std::optional<Key> max_in(const Node *node) const
  {
    if (node == nullptr)
      return std::nullopt;

    const Node *curr = node;
    while (curr != nullptr and not curr->leaf)
      {
        if (curr->children.is_empty())
          return std::nullopt;
        curr = curr->children.get_last().get();
      }

    if (curr == nullptr or curr->keys.is_empty())
      return std::nullopt;

    return curr->keys.get_last();
  }

  void borrow_from_prev(Node *parent, const size_t idx)
  {
    Node *child = parent->children[idx].get();
    Node *left  = parent->children[idx - 1].get();

    insert_at(child->keys, 0, parent->keys[idx - 1]);
    parent->keys[idx - 1] = left->keys.get_last();
    (void) left->keys.remove_last();

    if (not left->leaf)
      {
        insert_at(child->children, 0, std::move(left->children.get_last()));
        (void) left->children.remove_last();
      }
  }

  void borrow_from_next(Node *parent, const size_t idx)
  {
    Node *child = parent->children[idx].get();
    Node *right = parent->children[idx + 1].get();

    child->keys.append(parent->keys[idx]);
    parent->keys[idx] = right->keys.get_first();
    erase_at(right->keys, 0);

    if (not right->leaf)
      {
        child->children.append(std::move(right->children.get_first()));
        erase_at(right->children, 0);
      }
  }

  void merge_children(Node *parent, const size_t idx)
  {
    Node *left  = parent->children[idx].get();
    auto  right = std::move(parent->children[idx + 1]);

    left->keys.append(parent->keys[idx]);
    append_copy_range(left->keys, right->keys, 0);

    if (not left->leaf)
      append_move_range(left->children, right->children, 0);

    erase_at(parent->keys, idx);
    erase_at(parent->children, idx + 1);
  }

  [[nodiscard]] size_t ensure_child_has_extra(Node *parent, const size_t idx)
  {
    if (parent->children[idx]->keys.size() >= MinDegree)
      return idx;

    if (idx > 0 and parent->children[idx - 1]->keys.size() >= MinDegree)
      {
        borrow_from_prev(parent, idx);
        return idx;
      }

    if (idx + 1 < parent->children.size() and parent->children[idx + 1]->keys.size() >= MinDegree)
      {
        borrow_from_next(parent, idx);
        return idx;
      }

    if (idx + 1 < parent->children.size())
      {
        merge_children(parent, idx);
        return idx;
      }

    merge_children(parent, idx - 1);
    return idx - 1;
  }

  [[nodiscard]] bool remove_from(Node *node, const Key &key)
  {
    const size_t idx   = lower_bound_index(node->keys, key);
    const bool   found = idx < node->keys.size() and equals(node->keys[idx], key);

    if (found)
      {
        if (node->leaf)
          {
            erase_at(node->keys, idx);
            return true;
          }

        if (node->children[idx]->keys.size() >= MinDegree)
          {
            auto pred_opt = max_in(node->children[idx].get());
            ah_runtime_error_unless(pred_opt.has_value())
              << "max_in returned nullopt on non-empty child in remove_from";
            const Key pred  = *pred_opt;
            node->keys[idx] = pred;
            return remove_from(node->children[idx].get(), pred);
          }

        if (node->children[idx + 1]->keys.size() >= MinDegree)
          {
            auto succ_opt = min_in(node->children[idx + 1].get());
            ah_runtime_error_unless(succ_opt.has_value())
              << "min_in returned nullopt on non-empty child in remove_from";
            const Key succ  = *succ_opt;
            node->keys[idx] = succ;
            return remove_from(node->children[idx + 1].get(), succ);
          }

        merge_children(node, idx);
        return remove_from(node->children[idx].get(), key);
      }

    if (node->leaf)
      return false;

    const size_t child_idx = ensure_child_has_extra(node, idx);
    return remove_from(node->children[child_idx].get(), key);
  }

  void collect_keys(const Node *node, Array<Key> &out) const
  {
    if (node == nullptr)
      return;

    if (node->leaf)
      {
        for (const auto &key : node->keys)
          out.append(key);
        return;
      }

    for (size_t i = 0; i < node->keys.size(); ++i)
      {
        collect_keys(node->children[i].get(), out);
        out.append(node->keys[i]);
      }
    collect_keys(node->children.get_last().get(), out);
  }

  [[nodiscard]] bool verify_node(const Node *node, const std::optional<Key> &min_key, const std::optional<Key> &max_key,
                                 const size_t depth, size_t &leaf_depth, size_t &counted) const
  {
    if (node == nullptr)
      return false;

    if (node != root_.get())
      {
        if (node->keys.size() < MinKeys or node->keys.size() > MaxKeys)
          return false;
      }
    else if (node->keys.size() > MaxKeys)
      return false;

    // Keys must be strictly increasing — no equal adjacent entries are allowed.
    {
      const auto & ks = node->keys;
      for (size_t ki = 0; ki + 1 < ks.size(); ++ki)
        if (not cmp_(ks[ki], ks[ki + 1]))
          return false;
    }

    if (min_key.has_value() and not node->keys.is_empty() and not cmp_(*min_key, node->keys.get_first()))
      return false;

    if (max_key.has_value() and not node->keys.is_empty() and not cmp_(node->keys.get_last(), *max_key))
      return false;

    counted += node->keys.size();

    if (node->leaf)
      {
        if (leaf_depth == static_cast<size_t>(-1))
          leaf_depth = depth;
        return leaf_depth == depth;
      }

    if (node->children.size() != node->keys.size() + 1)
      return false;

    for (size_t i = 0; i < node->children.size(); ++i)
      {
        std::optional<Key> child_min = min_key;
        std::optional<Key> child_max = max_key;
        if (i > 0)
          child_min = node->keys[i - 1];
        if (i < node->keys.size())
          child_max = node->keys[i];
        if (not verify_node(node->children[i].get(), child_min, child_max, depth + 1, leaf_depth, counted))
          return false;
      }

    return true;
  }

public:
  using key_type = Key;  ///< Stored key type.

  /** @brief Construct an empty B-Tree.
   *  @param cmp Comparison functor used to sort keys.
   *  @throws Nothing.
   *  @note O(1).
   */
  explicit Gen_B_Tree(const Compare &cmp = Compare()) : cmp_(cmp) {}

  /** @brief Construct a B-Tree from an iterator range.
   *  @param first First element to insert.
   *  @param last One-past-the-end iterator.
   *  @param cmp Comparison functor used to sort keys.
   *  @throws std::bad_alloc if internal storage allocation fails.
   *  @note O(m log m) where m = distance(first, last).
   */
  template <std::input_iterator It> Gen_B_Tree(It first, It last, const Compare &cmp = Compare()) : cmp_(cmp)
  {
    insert_range(first, last);
  }

  /** @brief Construct a B-Tree from an initializer list.
   *  @param init Keys to insert.
   *  @param cmp Comparison functor used to sort keys.
   *  @throws std::bad_alloc if internal storage allocation fails.
   *  @note O(m log m) where m = init.size().
   */
  Gen_B_Tree(std::initializer_list<Key> init, const Compare &cmp = Compare())
    : Gen_B_Tree(init.begin(), init.end(), cmp)
  {}

  /** @brief Copy constructor performing a deep copy.
   *  @param other Tree to copy.
   *  @throws std::bad_alloc if cloning internal nodes fails.
   *  @note O(n) time and space.
   */
  Gen_B_Tree(const Gen_B_Tree &other) : root_(clone_node(other.root_.get())), size_(other.size_), cmp_(other.cmp_) {}

  /** @brief Move constructor.
   *  @param other Tree to move from.
   *  @note O(1).
   */
  Gen_B_Tree(Gen_B_Tree &&other) = default;

  /** @brief Copy assignment.
   *  @param other Tree to copy from.
   *  @return `*this`.
   *  @note O(n) time and space (copy-and-swap).
   */
  Gen_B_Tree &operator=(const Gen_B_Tree &other)
  {
    if (this == &other)
      return *this;

    // Copy-and-swap: build a full copy first so that if cmp_'s copy-assignment
    // (or clone_node) throws, *this is left unchanged.
    Gen_B_Tree tmp(other);
    std::swap(root_, tmp.root_);
    std::swap(size_, tmp.size_);
    std::swap(cmp_, tmp.cmp_);
    return *this;
  }

  /** @brief Move assignment.
   *  @param other Tree to move from.
   *  @return `*this`.
   *  @note O(1).
   */
  Gen_B_Tree &operator=(Gen_B_Tree &&other) = default;

  /** @brief Return the comparison object (read-only).
   *  @return Constant reference to the comparison functor.
   *  @note The comparator is immutable after construction to preserve tree order.
   *  @throws Nothing.
   */
  [[nodiscard]] const Compare &key_comp() const noexcept
  {
    return cmp_;
  }

  /** @brief Return the comparison object (read-only).
   *  @return Constant reference to the comparison functor.
   *  @note Alias for key_comp(). Immutable after construction.
   *  @throws Nothing.
   */
  [[nodiscard]] const Compare &get_compare() const noexcept
  {
    return key_comp();
  }

  /** @brief Return whether the tree has no keys.
   *  @return `true` if empty, otherwise `false`.
   *  @throws Nothing.
   *  @note O(1).
   */
  [[nodiscard]] bool empty() const noexcept
  {
    return size_ == 0;
  }

  /** @brief Synonym for empty() in traditional Aleph style.
   *  @return `true` if empty, otherwise `false`.
   *  @throws Nothing.
   *  @note O(1).
   */
  [[nodiscard]] bool is_empty() const noexcept
  {
    return empty();
  }

  /** @brief Return the number of stored keys.
   *  @return Number of unique keys stored in the tree.
   *  @throws Nothing.
   *  @note O(1).
   */
  [[nodiscard]] size_t size() const noexcept
  {
    return size_;
  }

  /** @brief Return the current tree height.
   *  @return `0` for an empty tree, otherwise the number of node levels.
   *  @throws Nothing.
   *  @note O(log n).
   */
  [[nodiscard]] size_t height() const noexcept
  {
    return height_of(root_.get());
  }

  /** @brief Remove every key from the tree.
   *  @throws Nothing.
   *  @note O(n).
   */
  void clear() noexcept
  {
    root_.reset();
    size_ = 0;
  }

  /** @brief Return whether a key is present.
   *  @param key Key to search.
   *  @return `true` if the key exists, otherwise `false`.
   *  @throws Any exception propagated by `Compare`.
   *  @note O(log n).
   */
  [[nodiscard]] bool contains(const Key &key) const
  {
    return contains_in(root_.get(), key);
  }

  /** @brief Synonym for contains().
   *  @param key Key to search.
   *  @return `true` if the key exists, otherwise `false`.
   *  @throws Any exception propagated by `Compare`.
   *  @note O(log n).
   */
  [[nodiscard]] bool search(const Key &key) const
  {
    return contains(key);
  }

  /** @brief Insert a key if it is not already present.
   *  @param key Key to insert.
   *  @return `true` if insertion happened, `false` if it was duplicate.
   *  @throws std::bad_alloc if node growth fails.
   *  @throws Any exception propagated by `Compare` or `Key` copy/move operations.
   *  @note O(log n).
   */
  bool insert(const Key &key)
  {
    if (contains(key))
      return false;

    if (root_ == nullptr)
      {
        root_ = std::make_unique<Node>(true);
        root_->keys.append(key);
        size_ = 1;
        return true;
      }

    if (root_->keys.size() == MaxKeys)
      {
        auto new_root = std::make_unique<Node>(false);
        new_root->children.append(std::move(root_));
        split_child(new_root.get(), 0);
        root_ = std::move(new_root);
      }

    insert_nonfull(root_.get(), key);
    ++size_;
    return true;
  }

  /** @brief Insert a key by move if it is not already present.
   *  @param key Key to insert.
   *  @return `true` if insertion happened, `false` if it was duplicate.
   *  @throws std::bad_alloc if node growth fails.
   *  @throws Any exception propagated by `Compare` or `Key` move/copy operations.
   *  @note O(log n).
   */
  bool insert(Key &&key)
  {
    Key copy = std::move(key);
    return insert(copy);
  }

  /** @brief Remove a key if present.
   *  @param key Key to erase.
   *  @return `true` if the key was removed, otherwise `false`.
   *  @throws Any exception propagated by `Compare` or `Key` assignments.
   *  @note O(log n).
   */
  bool remove(const Key &key)
  {
    if (root_ == nullptr)
      return false;

    const bool removed = remove_from(root_.get(), key);
    if (not removed)
      return false;

    --size_;

    while (root_ != nullptr and root_->keys.is_empty())
      {
        if (root_->leaf)
          {
            root_.reset();
            break;
          }

        if (root_->children.is_empty())
          {
            root_.reset();
            break;
          }

        root_ = std::move(root_->children.get_first());
      }

    return true;
  }

  /** @brief Return the minimum key, if any.
   *  @return Smallest key in the tree or `std::nullopt` if empty.
   *  @throws Any exception propagated by `Key` copy construction.
   *  @note O(log n).
   */
  [[nodiscard]] std::optional<Key> min_key() const
  {
    return min_in(root_.get());
  }

  /** @brief Return the maximum key, if any.
   *  @return Largest key in the tree or `std::nullopt` if empty.
   *  @throws Any exception propagated by `Key` copy construction.
   *  @note O(log n).
   */
  [[nodiscard]] std::optional<Key> max_key() const
  {
    return max_in(root_.get());
  }

  /** @brief Return the first key not less than `key`.
   *  @param key Probe key.
   *  @return Matching lower bound or `std::nullopt` if none exists.
   *  @throws Any exception propagated by `Compare` or `Key` copy construction.
   *  @note O(log n).
   */
  [[nodiscard]] std::optional<Key> lower_bound(const Key &key) const
  {
    const Node        *curr = root_.get();
    std::optional<Key> candidate;

    while (curr != nullptr)
      {
        const size_t idx = lower_bound_index(curr->keys, key);
        if (idx < curr->keys.size())
          candidate = curr->keys[idx];
        if (curr->leaf)
          return candidate;
        curr = curr->children[idx].get();
      }

    return candidate;
  }

  /** @brief Return the first key strictly greater than `key`.
   *  @param key Probe key.
   *  @return Matching upper bound or `std::nullopt` if none exists.
   *  @throws Any exception propagated by `Compare` or `Key` copy construction.
   *  @note O(log n).
   */
  [[nodiscard]] std::optional<Key> upper_bound(const Key &key) const
  {
    const Node        *curr = root_.get();
    std::optional<Key> candidate;

    while (curr != nullptr)
      {
        const size_t idx = upper_bound_index(curr->keys, key);
        if (idx < curr->keys.size())
          candidate = curr->keys[idx];
        if (curr->leaf)
          return candidate;
        curr = curr->children[idx].get();
      }

    return candidate;
  }

  /** @brief Materialize the tree contents in sorted order.
   *  @return Ordered array containing every stored key.
   *  @throws std::bad_alloc if the result array cannot grow.
   *  @throws Any exception propagated by `Key` copy construction.
   *  @note O(n) time and space.
   */
  [[nodiscard]] Array<Key> keys() const
  {
    Array<Key> out;
    out.reserve(size_ == 0 ? 1 : size_);
    collect_keys(root_.get(), out);
    return out;
  }

  /** @brief Verify structural B-Tree invariants.
   *  @return `true` if the tree is structurally valid, otherwise `false`.
   *  @throws Any exception propagated by `Compare`.
   *  @note O(n).
   */
  [[nodiscard]] bool verify() const
  {
    if (root_ == nullptr)
      return size_ == 0;

    auto   leaf_depth = static_cast<size_t>(-1);
    size_t counted    = 0;
    return verify_node(root_.get(), std::nullopt, std::nullopt, 1, leaf_depth, counted) and counted == size_;
  }
};

/** @brief Convenient B-Tree alias.
 *  @tparam Key Key type stored in the tree.
 *  @tparam Compare Ordering functor.
 *  @tparam MinDegree Minimum degree `t`.
 */
template <typename Key, class Compare = Aleph::less<Key>, size_t MinDegree = 16>
using B_Tree = Gen_B_Tree<Key, Compare, MinDegree>;
}  // namespace Aleph

#endif  // TPL_B_TREE_H
