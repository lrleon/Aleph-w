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

/** @file tpl_bplus_tree.H
 *  @brief B+ Tree implementation with linked leaves and configurable degree.
 *
 *  This header provides \ref Aleph::BPlus_Tree, a multiway search tree where
 *  all user keys live in the leaves and internal nodes only store separators.
 *  Leaves are linked left-to-right, which makes ordered scans and range queries
 *  efficient and easy to express.
 *
 *  The implementation follows the usual B+ Tree invariants:
 *  - each non-root node stores between `t - 1` and `2t - 1` keys
 *  - each internal node with `k` separator keys owns `k + 1` children
 *  - leaves are linked in sorted order through a singly linked list
 *  - all leaves appear at the same depth
 *
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

#ifndef TPL_BPLUS_TREE_H
#define TPL_BPLUS_TREE_H

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
template <typename Compare, typename Key>
[[nodiscard]] bool bplus_equiv(const Compare &cmp, const Key &lhs, const Key &rhs)
{
  return not cmp(lhs, rhs) and not cmp(rhs, lhs);
}

template <typename Key> struct BPlusTreeNode
{
  bool                                  leaf = true;
  Array<Key>                            keys;
  Array<std::unique_ptr<BPlusTreeNode>> children;
  BPlusTreeNode                        *next = nullptr;

  explicit BPlusTreeNode(const bool is_leaf) : leaf(is_leaf) {}
};
}  // namespace detail

/** @brief Generic B+ Tree with configurable minimum degree.
 *
 *  `Gen_BPlus_Tree` models an ordered set of unique keys. All payload keys
 *  are stored in leaves, while internal nodes keep only separator keys. This
 *  organization is specially convenient for page-oriented indexes, database
 *  style range scans, and any workload where ordered iteration matters as
 *  much as point lookups.
 *
 *  @tparam Key Key type stored in the tree.
 *  @tparam Compare Strict weak ordering used to sort keys.
 *  @tparam MinDegree Minimum degree `t` of the B+ Tree. Valid values are
 *          `t >= 2`.
 *
 *  @par Complexity
 *  - Search: O(log n)
 *  - Insert: O(log n)
 *  - Remove: O(log n)
 *  - lower_bound()/upper_bound(): O(log n)
 *  - range(): O(log n + k), where `k` is the number of reported keys
 *  - keys(): O(n)
 *  - get_range_it(): O(log n) setup, then O(1) amortized per increment
 *
 *  @par Example
 *  @code
 *  BPlus_Tree<int> tree = {40, 10, 90, 30, 50, 70};
 *
 *  auto hit = tree.lower_bound(45);   // 50
 *  auto band = tree.range(25, 70);    // 30, 40, 50, 70
 *
 *  tree.remove(40);
 *  auto ordered = tree.keys();
 *  @endcode
 *
 *  @note This tree stores unique keys. Duplicate inserts return `false`.
 *  @note For a classic B-Tree where internal nodes also store user keys, see
 *        \ref Aleph::B_Tree.
 *
 *  @par Thread Safety
 *  Instances are not synchronized. Concurrent access to the same tree
 *  requires external synchronization.
 *
 *  @par Exception Safety
 *  Read-only operations do not allocate, except when materializing returned
 *  `Array<Key>` results. Mutating operations preserve leaf links and search
 *  invariants if `Compare` and `Key` operations do not throw in the middle of
 *  internal node updates; allocation failures propagate as `std::bad_alloc`.
 */
template <typename Key, class Compare = Aleph::less<Key>, size_t MinDegree = 16>
  requires StrictWeakOrder<Compare, Key>
class Gen_BPlus_Tree
{
  static_assert(MinDegree >= 2, "BPlus_Tree requires MinDegree >= 2");
  static_assert(std::copy_constructible<Key>, "BPlus_Tree requires copy-constructible keys");

  using Node = detail::BPlusTreeNode<Key>;

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

  [[nodiscard]] size_t child_index(const Node *node, const Key &key) const
  {
    return upper_bound_index(node->keys, key);
  }

  [[nodiscard]] bool equals(const Key &lhs, const Key &rhs) const
  {
    return detail::bplus_equiv(cmp_, lhs, rhs);
  }

  [[nodiscard]] bool strictly_sorted(const Array<Key> &keys) const
  {
    if (not Aleph::is_sorted(keys, cmp_))
      return false;

    for (size_t i = 1; i < keys.size(); ++i)
      if (not cmp_(keys[i - 1], keys[i]))
        return false;
    return true;
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

  [[nodiscard]] const Node *leftmost_leaf(const Node *node) const noexcept
  {
    const Node *curr = node;
    while (curr != nullptr and not curr->leaf)
      curr = curr->children.get_first().get();
    return curr;
  }

  [[nodiscard]] const Node *rightmost_leaf(const Node *node) const noexcept
  {
    const Node *curr = node;
    while (curr != nullptr and not curr->leaf)
      curr = curr->children.get_last().get();
    return curr;
  }

  [[nodiscard]] const Key *subtree_min_ptr(const Node *node) const noexcept
  {
    const Node *leaf = leftmost_leaf(node);
    if (leaf == nullptr or leaf->keys.is_empty())
      return nullptr;
    return &leaf->keys.get_first();
  }

  [[nodiscard]] const Key *subtree_max_ptr(const Node *node) const noexcept
  {
    const Node *leaf = rightmost_leaf(node);
    if (leaf == nullptr or leaf->keys.is_empty())
      return nullptr;
    return &leaf->keys.get_last();
  }

  [[nodiscard]] const Key &subtree_min(const Node *node) const
  {
    const Key *ptr = subtree_min_ptr(node);
    ah_runtime_error_unless(ptr != nullptr) << "BPlus_Tree invariant violated: empty subtree has no minimum";
    return *ptr;
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

  void rebuild_separators(Node *node) const
  {
    if (node == nullptr or node->leaf)
      return;

    node->keys.empty();
    node->keys.reserve(node->children.size() > 0 ? node->children.size() - 1 : 0);
    for (size_t i = 1; i < node->children.size(); ++i)
      node->keys.append(subtree_min(node->children[i].get()));
  }

  [[nodiscard]] std::unique_ptr<Node> clone_node(const Node *node, Node *&prev_leaf) const
  {
    if (node == nullptr)
      return nullptr;

    auto copy  = std::make_unique<Node>(node->leaf);
    copy->keys = node->keys;

    if (node->leaf)
      {
        if (prev_leaf != nullptr)
          prev_leaf->next = copy.get();
        prev_leaf = copy.get();
        return copy;
      }

    copy->children.reserve(node->children.size());
    for (const auto &child : node->children)
      copy->children.append(clone_node(child.get(), prev_leaf));
    rebuild_separators(copy.get());
    return copy;
  }

  [[nodiscard]] const Node *find_leaf(const Key &key) const noexcept
  {
    const Node *curr = root_.get();
    while (curr != nullptr and not curr->leaf)
      curr = curr->children[child_index(curr, key)].get();
    return curr;
  }

  [[nodiscard]] bool contains_in(const Node *node, const Key &key) const
  {
    if (node == nullptr)
      return false;

    const Node *leaf = find_leaf(key);
    if (leaf == nullptr)
      return false;

    const size_t idx = lower_bound_index(leaf->keys, key);
    return idx < leaf->keys.size() and equals(leaf->keys[idx], key);
  }

  void split_child(Node *parent, const size_t idx)
  {
    Node *child = parent->children[idx].get();
    auto  right = std::make_unique<Node>(child->leaf);

    if (child->leaf)
      {
        append_copy_range(right->keys, child->keys, MinKeys);
        truncate(child->keys, MinKeys);

        right->next = child->next;
        child->next = right.get();
      }
    else
      {
        append_move_range(right->children, child->children, MinDegree);
        truncate(child->children, MinDegree);
        rebuild_separators(child);
        rebuild_separators(right.get());
      }

    insert_at(parent->children, idx + 1, std::move(right));
    rebuild_separators(parent);
  }

  void insert_nonfull(Node *node, const Key &key)
  {
    if (node->leaf)
      {
        const size_t idx = lower_bound_index(node->keys, key);
        insert_at(node->keys, idx, key);
        return;
      }

    size_t idx = child_index(node, key);
    if (node->children[idx]->keys.size() == MaxKeys)
      {
        split_child(node, idx);
        idx = child_index(node, key);
      }

    insert_nonfull(node->children[idx].get(), key);
    rebuild_separators(node);
  }

  void borrow_from_prev(Node *parent, const size_t idx)
  {
    Node *child = parent->children[idx].get();
    Node *left  = parent->children[idx - 1].get();

    if (child->leaf)
      {
        insert_at(child->keys, 0, left->keys.get_last());
        (void) left->keys.remove_last();
      }
    else
      {
        insert_at(child->children, 0, std::move(left->children.get_last()));
        (void) left->children.remove_last();
        rebuild_separators(left);
        rebuild_separators(child);
      }

    rebuild_separators(parent);
  }

  void borrow_from_next(Node *parent, const size_t idx)
  {
    Node *child = parent->children[idx].get();
    Node *right = parent->children[idx + 1].get();

    if (child->leaf)
      {
        child->keys.append(right->keys.get_first());
        erase_at(right->keys, 0);
      }
    else
      {
        child->children.append(std::move(right->children.get_first()));
        erase_at(right->children, 0);
        rebuild_separators(child);
        rebuild_separators(right);
      }

    rebuild_separators(parent);
  }

  void merge_children(Node *parent, const size_t left_idx)
  {
    Node *left  = parent->children[left_idx].get();
    auto  right = std::move(parent->children[left_idx + 1]);

    if (left->leaf)
      {
        append_copy_range(left->keys, right->keys, 0);
        left->next = right->next;
      }
    else
      {
        append_move_range(left->children, right->children, 0);
        rebuild_separators(left);
      }

    erase_at(parent->children, left_idx + 1);
    rebuild_separators(parent);
  }

  void fix_underflow(Node *parent, const size_t idx)
  {
    if (parent->children[idx]->keys.size() >= MinKeys)
      {
        rebuild_separators(parent);
        return;
      }

    if (idx > 0 and parent->children[idx - 1]->keys.size() > MinKeys)
      {
        borrow_from_prev(parent, idx);
        return;
      }

    if (idx + 1 < parent->children.size() and parent->children[idx + 1]->keys.size() > MinKeys)
      {
        borrow_from_next(parent, idx);
        return;
      }

    if (idx + 1 < parent->children.size())
      merge_children(parent, idx);
    else
      merge_children(parent, idx - 1);
  }

  [[nodiscard]] bool remove_from(Node *node, const Key &key)
  {
    if (node->leaf)
      {
        const size_t idx = lower_bound_index(node->keys, key);
        if (idx >= node->keys.size() or not equals(node->keys[idx], key))
          return false;

        erase_at(node->keys, idx);
        return true;
      }

    const size_t idx = child_index(node, key);
    if (const bool removed = remove_from(node->children[idx].get(), key); not removed)
      return false;

    fix_underflow(node, idx);
    rebuild_separators(node);
    return true;
  }

  [[nodiscard]] bool verify_leaf_chain() const
  {
    const Node        *leaf    = leftmost_leaf(root_.get());
    size_t             counted = 0;
    std::optional<Key> prev;

    while (leaf != nullptr)
      {
        if (not leaf->leaf or leaf->keys.is_empty())
          return false;

        for (const auto &key : leaf->keys)
          {
            if (prev.has_value() and not cmp_(*prev, key))
              return false;
            prev = key;
            ++counted;
          }

        if (leaf->next != nullptr)
          {
            if (not leaf->next->leaf or leaf->next->keys.is_empty())
              return false;
            if (not cmp_(leaf->keys.get_last(), leaf->next->keys.get_first()))
              return false;
          }

        leaf = leaf->next;
      }

    return counted == size_;
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

    if (not strictly_sorted(node->keys))
      return false;

    if (node->leaf)
      {
        if (min_key.has_value() and not node->keys.is_empty() and cmp_(node->keys.get_first(), *min_key))
          return false;

        if (max_key.has_value() and not node->keys.is_empty() and not cmp_(node->keys.get_last(), *max_key))
          return false;

        counted += node->keys.size();

        if (leaf_depth == static_cast<size_t>(-1))
          leaf_depth = depth;
        return leaf_depth == depth;
      }

    if (node->children.size() != node->keys.size() + 1 or node->children.is_empty())
      return false;

    for (size_t i = 1; i < node->children.size(); ++i)
      {
        const Key *expected = subtree_min_ptr(node->children[i].get());
        if (expected == nullptr or not equals(node->keys[i - 1], *expected))
          return false;
      }

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

    const Key *first = subtree_min_ptr(node->children.get_first().get());
    const Key *last  = subtree_max_ptr(node->children.get_last().get());
    if (first == nullptr or last == nullptr)
      return false;

    if (min_key.has_value() and cmp_(*first, *min_key))
      return false;

    if (max_key.has_value() and not cmp_(*last, *max_key))
      return false;

    return true;
  }

public:
  using key_type = Key;  ///< Stored key type.

  /** @brief Lazy iterator over ordered leaf keys.
   *
   *  The iterator walks the leaf chain directly and therefore avoids
   *  materializing an intermediate `Array<Key>`. Any mutation of the owning
   *  tree invalidates all iterators.
   */
  class Iterator
  {
    const Gen_BPlus_Tree *tree_ = nullptr;
    const Node           *leaf_ = nullptr;
    size_t                idx_  = 0;
    std::optional<Key>    last_;

    void skip_past_end()
    {
      while (tree_ != nullptr and leaf_ != nullptr)
        {
          if (idx_ >= leaf_->keys.size())
            {
              leaf_ = leaf_->next;
              idx_  = 0;
              continue;
            }

          if (last_.has_value() and tree_->cmp_(*last_, leaf_->keys[idx_]))
            {
              leaf_ = nullptr;
              idx_  = 0;
            }
          return;
        }
    }

  public:
    /** @brief Construct an exhausted iterator.
     *  @throws Nothing.
     */
    Iterator() noexcept = default;

    /** @brief Construct an iterator over the full key order.
     *  @param tree Tree whose leaf chain will be traversed.
     *  @throws Nothing.
     */
    explicit Iterator(const Gen_BPlus_Tree &tree) noexcept
      : tree_(&tree), leaf_(tree.leftmost_leaf(tree.root_.get())), idx_(0)
    {
      skip_past_end();
    }

    /** @brief Construct an iterator over the inclusive range [`first`, `last`].
     *  @param tree Tree whose leaf chain will be traversed.
     *  @param first Lower bound, inclusive.
     *  @param last Upper bound, inclusive.
     *  @throws std::invalid_argument If `last < first`.
     *  @throws Any exception propagated by `Compare`.
     */
    Iterator(const Gen_BPlus_Tree &tree, const Key &first, const Key &last)
      : tree_(&tree), leaf_(nullptr), idx_(0), last_(last)
    {
      ah_invalid_argument_if(tree.cmp_(last, first))
        << "BPlus_Tree::Iterator(): invalid range [" << first << ", " << last << "]";

      leaf_ = tree.find_leaf(first);
      if (leaf_ != nullptr)
        idx_ = tree.lower_bound_index(leaf_->keys, first);
      skip_past_end();
    }

    /** @brief Return whether the iterator still points to a key.
     *  @return `true` if there is a current key, otherwise `false`.
     *  @throws Nothing.
     */
    [[nodiscard]] bool has_curr() const noexcept
    {
      return tree_ != nullptr and leaf_ != nullptr;
    }

    /** @brief Return the current key.
     *  @return Reference to the current key in the owning tree.
     *  @throws std::underflow_error If the iterator is exhausted.
     */
    [[nodiscard]] const Key &get_curr() const
    {
      ah_underflow_error_if(not has_curr()) << "BPlus_Tree::Iterator::get_curr(): no current key";
      return leaf_->keys[idx_];
    }

    /** @brief Advance to the next key.
     *  @return Nothing.
     *  @throws std::underflow_error If the iterator is exhausted.
     *  @throws Any exception propagated by `Compare`.
     */
    void next_ne()
    {
      ah_underflow_error_if(not has_curr()) << "BPlus_Tree::Iterator::next_ne(): no current key";
      ++idx_;
      skip_past_end();
    }

    /** @brief Synonym for next_ne().
     *  @return Nothing.
     *  @throws std::underflow_error If the iterator is exhausted.
     *  @throws Any exception propagated by `Compare`.
     */
    void next()
    {
      next_ne();
    }

    /** @brief Dereference the iterator.
     *  @return Reference to the current key.
     *  @throws std::underflow_error If the iterator is exhausted.
     */
    [[nodiscard]] const Key &operator*() const
    {
      return get_curr();
    }

    /** @brief Member-access shorthand for the current key.
     *  @return Pointer to the current key.
     *  @throws std::underflow_error If the iterator is exhausted.
     */
    [[nodiscard]] const Key *operator->() const
    {
      return &get_curr();
    }
  };

  /** @brief Construct an empty B+ Tree.
   *  @param cmp Comparison functor used to sort keys.
   *  @throws Nothing.
   */
  explicit Gen_BPlus_Tree(const Compare &cmp = Compare()) : cmp_(cmp) {}

  /** @brief Construct a B+ Tree from an iterator range.
   *  @param first First element to insert.
   *  @param last One-past-the-end iterator.
   *  @param cmp Comparison functor used to sort keys.
   *  @throws std::bad_alloc if internal storage allocation fails.
   */
  template <std::input_iterator It> Gen_BPlus_Tree(It first, It last, const Compare &cmp = Compare()) : cmp_(cmp)
  {
    insert_range(first, last);
  }

  /** @brief Construct a B+ Tree from an initializer list.
   *  @param init Keys to insert.
   *  @param cmp Comparison functor used to sort keys.
   *  @throws std::bad_alloc if internal storage allocation fails.
   */
  Gen_BPlus_Tree(std::initializer_list<Key> init, const Compare &cmp = Compare())
    : Gen_BPlus_Tree(init.begin(), init.end(), cmp)
  {}

  /** @brief Copy constructor performing a deep copy.
   *  @param other Tree to copy.
   *  @throws std::bad_alloc if cloning internal nodes fails.
   */
  Gen_BPlus_Tree(const Gen_BPlus_Tree &other) : size_(other.size_), cmp_(other.cmp_)
  {
    Node *prev_leaf = nullptr;
    root_           = clone_node(other.root_.get(), prev_leaf);
  }

  /** @brief Move constructor.
   *  @param other Tree to move from.
   */
  Gen_BPlus_Tree(Gen_BPlus_Tree &&) noexcept = default;

  /** @brief Copy assignment.
   *  @param other Tree to copy from.
   *  @return `*this`.
   */
  Gen_BPlus_Tree &operator=(const Gen_BPlus_Tree &other)
  {
    if (this == &other)
      return *this;

    Node *prev_leaf = nullptr;
    root_           = clone_node(other.root_.get(), prev_leaf);
    size_           = other.size_;
    cmp_            = other.cmp_;
    return *this;
  }

  /** @brief Move assignment.
   *  @param other Tree to move from.
   *  @return `*this`.
   */
  Gen_BPlus_Tree &operator=(Gen_BPlus_Tree &&) noexcept = default;

  /** @brief Return the comparison object.
   *  @return Mutable reference to the comparison functor.
   *  @throws Nothing.
   */
  Compare &key_comp() noexcept
  {
    return cmp_;
  }

  /** @brief Return the comparison object.
   *  @return Constant reference to the comparison functor.
   *  @throws Nothing.
   */
  [[nodiscard]] const Compare &key_comp() const noexcept
  {
    return cmp_;
  }

  /** @brief Return the comparison object.
   *  @return Mutable reference to the comparison functor.
   *  @throws Nothing.
   */
  Compare &get_compare() noexcept
  {
    return key_comp();
  }

  /** @brief Return the comparison object.
   *  @return Constant reference to the comparison functor.
   *  @throws Nothing.
   */
  [[nodiscard]] const Compare &get_compare() const noexcept
  {
    return key_comp();
  }

  /** @brief Return whether the tree has no keys.
   *  @return `true` if empty, otherwise `false`.
   *  @throws Nothing.
   */
  [[nodiscard]] bool empty() const noexcept
  {
    return size_ == 0;
  }

  /** @brief Synonym for empty() in traditional Aleph style.
   *  @return `true` if empty, otherwise `false`.
   *  @throws Nothing.
   */
  [[nodiscard]] bool is_empty() const noexcept
  {
    return empty();
  }

  /** @brief Return the number of stored keys.
   *  @return Number of unique keys stored in the tree.
   *  @throws Nothing.
   */
  [[nodiscard]] size_t size() const noexcept
  {
    return size_;
  }

  /** @brief Return the current tree height.
   *  @return `0` for an empty tree, otherwise the number of node levels.
   *  @throws Nothing.
   */
  [[nodiscard]] size_t height() const noexcept
  {
    return height_of(root_.get());
  }

  /** @brief Remove every key from the tree.
   *  @throws Nothing.
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
   */
  [[nodiscard]] bool contains(const Key &key) const
  {
    return contains_in(root_.get(), key);
  }

  /** @brief Synonym for contains().
   *  @param key Key to search.
   *  @return `true` if the key exists, otherwise `false`.
   *  @throws Any exception propagated by `Compare`.
   */
  [[nodiscard]] bool search(const Key &key) const
  {
    return contains(key);
  }

  /** @brief Insert a key if it is not already present.
   *  @param key Key to insert.
   *  @return `true` if insertion happened, `false` if it was duplicate.
   *  @throws std::bad_alloc if node growth fails.
   *  @throws Any exception propagated by `Compare` or `Key` copy/move
   * operations.
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
   *  @throws Any exception propagated by `Compare` or `Key` move/copy
   * operations.
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
   */
  bool remove(const Key &key)
  {
    if (root_ == nullptr)
      return false;

    const bool removed = remove_from(root_.get(), key);
    if (not removed)
      return false;

    --size_;

    if (root_->leaf and root_->keys.is_empty())
      root_.reset();
    else if (not root_->leaf and root_->children.size() == 1)
      root_ = std::move(root_->children.get_first());

    return true;
  }

  /** @brief Return the minimum key, if any.
   *  @return Smallest key in the tree or `std::nullopt` if empty.
   *  @throws Any exception propagated by `Key` copy construction.
   */
  [[nodiscard]] std::optional<Key> min_key() const
  {
    const Key *ptr = subtree_min_ptr(root_.get());
    if (ptr == nullptr)
      return std::nullopt;
    return *ptr;
  }

  /** @brief Return the maximum key, if any.
   *  @return Largest key in the tree or `std::nullopt` if empty.
   *  @throws Any exception propagated by `Key` copy construction.
   */
  [[nodiscard]] std::optional<Key> max_key() const
  {
    const Key *ptr = subtree_max_ptr(root_.get());
    if (ptr == nullptr)
      return std::nullopt;
    return *ptr;
  }

  /** @brief Return the first key not less than `key`.
   *  @param key Probe key.
   *  @return Matching lower bound or `std::nullopt` if none exists.
   *  @throws Any exception propagated by `Compare` or `Key` copy construction.
   */
  [[nodiscard]] std::optional<Key> lower_bound(const Key &key) const
  {
    const Node *leaf = find_leaf(key);
    if (leaf == nullptr)
      return std::nullopt;

    size_t idx = lower_bound_index(leaf->keys, key);
    while (leaf != nullptr)
      {
        if (idx < leaf->keys.size())
          return leaf->keys[idx];
        leaf = leaf->next;
        idx  = 0;
      }

    return std::nullopt;
  }

  /** @brief Return the first key strictly greater than `key`.
   *  @param key Probe key.
   *  @return Matching upper bound or `std::nullopt` if none exists.
   *  @throws Any exception propagated by `Compare` or `Key` copy construction.
   */
  [[nodiscard]] std::optional<Key> upper_bound(const Key &key) const
  {
    const Node *leaf = find_leaf(key);
    if (leaf == nullptr)
      return std::nullopt;

    size_t idx = upper_bound_index(leaf->keys, key);
    while (leaf != nullptr)
      {
        if (idx < leaf->keys.size())
          return leaf->keys[idx];
        leaf = leaf->next;
        idx  = 0;
      }

    return std::nullopt;
  }

  /** @brief Return a lazy iterator over the full key order.
   *  @return Iterator positioned at the first key, or exhausted if empty.
   *  @throws Nothing.
   */
  [[nodiscard]] Iterator get_it() const noexcept
  {
    return Iterator(*this);
  }

  /** @brief Return a lazy iterator over an inclusive key range.
   *  @param first Lower endpoint, inclusive.
   *  @param last Upper endpoint, inclusive.
   *  @return Iterator positioned at the first key in the range.
   *  @throws std::invalid_argument if `last < first` according to `Compare`.
   *  @throws Any exception propagated by `Compare`.
   */
  [[nodiscard]] Iterator get_range_it(const Key &first, const Key &last) const
  {
    return Iterator(*this, first, last);
  }

  /** @brief Collect all keys in the inclusive range [`first`, `last`].
   *  @param first Lower endpoint, inclusive.
   *  @param last Upper endpoint, inclusive.
   *  @return Ordered array with every key in the requested range.
   *  @throws std::invalid_argument if `last < first` according to `Compare`.
   *  @throws std::bad_alloc if the result array cannot grow.
   *  @throws Any exception propagated by `Compare` or `Key` copy construction.
   */
  [[nodiscard]] Array<Key> range(const Key &first, const Key &last) const
  {
    Array<Key> out;
    out.reserve(8);
    for (auto it = get_range_it(first, last); it.has_curr(); it.next_ne())
      out.append(it.get_curr());

    return out;
  }

  /** @brief Materialize the tree contents in sorted order.
   *  @return Ordered array containing every stored key.
   *  @throws std::bad_alloc if the result array cannot grow.
   *  @throws Any exception propagated by `Key` copy construction.
   */
  [[nodiscard]] Array<Key> keys() const
  {
    Array<Key> out;
    out.reserve(size_ == 0 ? 1 : size_);
    for (auto it = get_it(); it.has_curr(); it.next_ne())
      out.append(it.get_curr());

    return out;
  }

  /** @brief Verify structural B+ Tree invariants.
   *  @return `true` if the tree is structurally valid, otherwise `false`.
   *  @throws Any exception propagated by `Compare`.
   */
  [[nodiscard]] bool verify() const
  {
    if (root_ == nullptr)
      return size_ == 0;

    if (root_->keys.is_empty())
      return false;

    auto   leaf_depth = static_cast<size_t>(-1);
    size_t counted    = 0;
    return verify_node(root_.get(), std::nullopt, std::nullopt, 1, leaf_depth, counted) and counted == size_
       and verify_leaf_chain();
  }
};

/** @brief Convenient B+ Tree alias.
 *  @tparam Key Key type stored in the tree.
 *  @tparam Compare Ordering functor.
 *  @tparam MinDegree Minimum degree `t`.
 */
template <typename Key, class Compare = Aleph::less<Key>, size_t MinDegree = 16>
using BPlus_Tree = Gen_BPlus_Tree<Key, Compare, MinDegree>;
}  // namespace Aleph

#endif  // TPL_BPLUS_TREE_H
