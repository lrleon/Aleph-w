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

/** @file tpl_persistent_treap.H
 *  @brief Immutable path-copying treap set and map.
 *
 *  `PersistentTreapSet` and `PersistentTreapMap` are in-memory persistent
 *  collections: operations that look like updates return a new collection
 *  version and never mutate the old one. Unchanged subtrees are shared through
 *  `std::shared_ptr<const Node>`, so copying a collection is O(1), snapshots
 *  are cheap, and an update copies only the nodes on the affected search path
 *  plus the nodes needed by treap rotations.
 *
 *  This is unrelated to Aleph's file-backed B/B+ tree persistence: these
 *  containers are volatile in-memory values, not durable storage.
 *
 *  @par Complexity
 *  Search, insert, erase, split and join are expected O(log n) with O(log n)
 *  new nodes allocated per update. Copying a collection value is O(1).
 *  `keys()` and `items()` flatten a version in sorted order and cost O(n).
 *
 *  @par Thread safety
 *  Published versions are immutable. Different threads may read distinct
 *  collection objects that share the same tree without synchronization. The
 *  publication of a new version into a shared variable is still the user's
 *  responsibility and must be synchronized externally.
 *
 *  @ingroup Trees
 */

#ifndef TPL_PERSISTENT_TREAP_H
#define TPL_PERSISTENT_TREAP_H

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

#include <ah-errors.H>
#include <ahFunction.H>
#include <tpl_array.H>

namespace Aleph
{

namespace detail
{
  [[nodiscard]] inline std::uint64_t persistent_treap_priority(const std::uint64_t n) noexcept
  {
    std::uint64_t x = n + 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
  }
}

/** @brief Immutable ordered set backed by a path-copying treap.
 *
 *  @tparam Key Key type stored by the set.
 *  @tparam Compare Strict weak ordering used for keys. Defaults to
 *          `Aleph::less<Key>`.
 */
template <typename Key, class Compare = Aleph::less<Key>>
class PersistentTreapSet
{
  struct Node
  {
    std::shared_ptr<const Key> key;
    std::uint64_t priority = 0;
    size_t count = 1;
    std::shared_ptr<const Node> left;
    std::shared_ptr<const Node> right;
  };

  using NodePtr = std::shared_ptr<const Node>;

  NodePtr root_;
  Compare cmp_;
  std::uint64_t next_priority_ = 0;

  [[nodiscard]] static size_t node_size(const NodePtr &p) noexcept
  {
    return p == nullptr ? 0 : p->count;
  }

  [[nodiscard]] static NodePtr make_node(std::shared_ptr<const Key> key,
                                         const std::uint64_t priority,
                                         NodePtr left,
                                         NodePtr right)
  {
    const size_t left_size = node_size(left);
    const size_t right_size = node_size(right);
    ah_overflow_error_if(left_size > std::numeric_limits<size_t>::max() - right_size)
      << "PersistentTreapSet: node size would overflow";
    const size_t children_size = left_size + right_size;
    ah_overflow_error_if(children_size == std::numeric_limits<size_t>::max())
      << "PersistentTreapSet: node size would overflow";

    auto node = std::make_shared<Node>();
    node->key = std::move(key);
    node->priority = priority;
    node->left = std::move(left);
    node->right = std::move(right);
    node->count = children_size + 1;
    return node;
  }

  [[nodiscard]] static NodePtr rotate_right(const NodePtr &node)
  {
    const NodePtr child = node->left;
    NodePtr new_right = make_node(node->key, node->priority, child->right, node->right);
    return make_node(child->key, child->priority, child->left, std::move(new_right));
  }

  [[nodiscard]] static NodePtr rotate_left(const NodePtr &node)
  {
    const NodePtr child = node->right;
    NodePtr new_left = make_node(node->key, node->priority, node->left, child->left);
    return make_node(child->key, child->priority, std::move(new_left), child->right);
  }

  [[nodiscard]] static bool equal_keys(const Key &a, const Key &b, const Compare &cmp)
  {
    return not cmp(a, b) and not cmp(b, a);
  }

  [[nodiscard]] static NodePtr insert_rec(const NodePtr &node,
                                          std::shared_ptr<const Key> key,
                                          const std::uint64_t priority,
                                          const Compare &cmp,
                                          bool &inserted)
  {
    if (node == nullptr)
      {
        inserted = true;
        return make_node(std::move(key), priority, nullptr, nullptr);
      }

    if (cmp(*key, *node->key))
      {
        NodePtr new_left = insert_rec(node->left, std::move(key), priority, cmp, inserted);
        if (not inserted)
          return node;
        NodePtr rebuilt = make_node(node->key, node->priority, std::move(new_left), node->right);
        return rebuilt->left->priority < rebuilt->priority ? rotate_right(rebuilt) : rebuilt;
      }

    if (cmp(*node->key, *key))
      {
        NodePtr new_right = insert_rec(node->right, std::move(key), priority, cmp, inserted);
        if (not inserted)
          return node;
        NodePtr rebuilt = make_node(node->key, node->priority, node->left, std::move(new_right));
        return rebuilt->right->priority < rebuilt->priority ? rotate_left(rebuilt) : rebuilt;
      }

    inserted = false;
    return node;
  }

  [[nodiscard]] static NodePtr join_nodes(const NodePtr &left, const NodePtr &right)
  {
    if (left == nullptr)
      return right;
    if (right == nullptr)
      return left;

    if (left->priority <= right->priority)
      {
        NodePtr new_right = join_nodes(left->right, right);
        return make_node(left->key, left->priority, left->left, std::move(new_right));
      }

    NodePtr new_left = join_nodes(left, right->left);
    return make_node(right->key, right->priority, std::move(new_left), right->right);
  }

  [[nodiscard]] static NodePtr erase_rec(const NodePtr &node,
                                         const Key &key,
                                         const Compare &cmp,
                                         bool &erased)
  {
    if (node == nullptr)
      {
        erased = false;
        return nullptr;
      }

    if (cmp(key, *node->key))
      {
        NodePtr new_left = erase_rec(node->left, key, cmp, erased);
        if (not erased)
          return node;
        return make_node(node->key, node->priority, std::move(new_left), node->right);
      }

    if (cmp(*node->key, key))
      {
        NodePtr new_right = erase_rec(node->right, key, cmp, erased);
        if (not erased)
          return node;
        return make_node(node->key, node->priority, node->left, std::move(new_right));
      }

    erased = true;
    return join_nodes(node->left, node->right);
  }

  [[nodiscard]] static std::pair<NodePtr, NodePtr> split_rec(const NodePtr &node,
                                                             const Key &pivot,
                                                             const Compare &cmp)
  {
    if (node == nullptr)
      return {nullptr, nullptr};

    if (cmp(*node->key, pivot))
      {
        auto [right_left, right_right] = split_rec(node->right, pivot, cmp);
        NodePtr left = make_node(node->key, node->priority, node->left,
                                 std::move(right_left));
        return {std::move(left), std::move(right_right)};
      }

    auto [left_left, left_right] = split_rec(node->left, pivot, cmp);
    NodePtr right = make_node(node->key, node->priority, std::move(left_right),
                              node->right);
    return {std::move(left_left), std::move(right)};
  }

  [[nodiscard]] static const Key *min_key(const NodePtr &node) noexcept
  {
    const Node *curr = node.get();
    while (curr->left != nullptr)
      curr = curr->left.get();
    return curr->key.get();
  }

  [[nodiscard]] static const Key *max_key(const NodePtr &node) noexcept
  {
    const Node *curr = node.get();
    while (curr->right != nullptr)
      curr = curr->right.get();
    return curr->key.get();
  }

  [[nodiscard]] static bool can_join(const NodePtr &left,
                                     const NodePtr &right,
                                     const Compare &cmp)
  {
    return left == nullptr or right == nullptr or cmp(*max_key(left), *min_key(right));
  }

  static void collect_keys(const NodePtr &node, Array<Key> &out)
    requires std::copy_constructible<Key>
  {
    if (node == nullptr)
      return;
    collect_keys(node->left, out);
    out.append(Key(*node->key));
    collect_keys(node->right, out);
  }

  [[nodiscard]] static bool verify_rec(const NodePtr &node,
                                       const Key *lo,
                                       const Key *hi,
                                       const Compare &cmp,
                                       size_t &count)
  {
    if (node == nullptr)
      return true;
    if (node->key == nullptr)
      return false;
    if (lo != nullptr and not cmp(*lo, *node->key))
      return false;
    if (hi != nullptr and not cmp(*node->key, *hi))
      return false;
    if (node->left != nullptr and node->left->priority < node->priority)
      return false;
    if (node->right != nullptr and node->right->priority < node->priority)
      return false;

    size_t left_count = 0;
    size_t right_count = 0;
    if (not verify_rec(node->left, lo, node->key.get(), cmp, left_count))
      return false;
    if (not verify_rec(node->right, node->key.get(), hi, cmp, right_count))
      return false;
    if (node->count != left_count + right_count + 1)
      return false;
    count = node->count;
    return true;
  }

  PersistentTreapSet(NodePtr root, Compare cmp, const std::uint64_t next_priority)
    : root_(std::move(root)), cmp_(std::move(cmp)), next_priority_(next_priority)
  {
    // Empty.
  }

public:
  /** @brief Construct an empty persistent set.
   *  @param cmp Comparator used for key ordering.
   *  @throws Nothing unless copying `cmp` throws.
   */
  explicit PersistentTreapSet(Compare cmp = Compare())
    : cmp_(std::move(cmp))
  {
    // Empty.
  }

  /** @brief Return true when the set has no keys.
   *  @return `true` iff `size() == 0`.
   *  @throws Nothing.
   */
  [[nodiscard]] bool is_empty() const noexcept { return root_ == nullptr; }

  /** @brief Return the number of keys stored in this version.
   *  @return Number of keys.
   *  @throws Nothing.
   */
  [[nodiscard]] size_t size() const noexcept { return node_size(root_); }

  /** @brief Find a key equivalent to `key`.
   *  @param key Key to search.
   *  @return Pointer to the stored key, or `nullptr` when absent. The pointer
   *          remains valid while this set version is alive.
   *  @throws Whatever `Compare` throws.
   */
  [[nodiscard]] const Key *find(const Key &key) const
  {
    const Node *curr = root_.get();
    while (curr != nullptr)
      if (cmp_(key, *curr->key))
        curr = curr->left.get();
      else if (cmp_(*curr->key, key))
        curr = curr->right.get();
      else
        return curr->key.get();
    return nullptr;
  }

  /** @brief Test whether `key` is present.
   *  @param key Key to search.
   *  @return `true` if an equivalent key is stored.
   *  @throws Whatever `Compare` throws.
   */
  [[nodiscard]] bool contains(const Key &key) const
  {
    return find(key) != nullptr;
  }

  /** @brief Return a new version with `key` inserted by copy.
   *  @param key Key to insert.
   *  @return New set version. If an equivalent key already exists, returns an
   *          unchanged version sharing the same root.
   *  @throws std::bad_alloc or whatever `Compare`/`Key` copy construction throws.
   */
  [[nodiscard]] PersistentTreapSet insert(const Key &key) const
  {
    auto stored_key = std::make_shared<const Key>(key);
    bool inserted = false;
    NodePtr root = insert_rec(root_, std::move(stored_key),
                              detail::persistent_treap_priority(next_priority_),
                              cmp_, inserted);
    return inserted ? PersistentTreapSet(std::move(root), cmp_, next_priority_ + 1) : *this;
  }

  /** @brief Return a new version with `key` inserted by move.
   *  @param key Key to move into the new version.
   *  @return New set version. If an equivalent key already exists, returns an
   *          unchanged version sharing the same root.
   *  @throws std::bad_alloc or whatever `Compare`/`Key` move construction throws.
   */
  [[nodiscard]] PersistentTreapSet insert(Key &&key) const
  {
    auto stored_key = std::make_shared<const Key>(std::move(key));
    bool inserted = false;
    NodePtr root = insert_rec(root_, std::move(stored_key),
                              detail::persistent_treap_priority(next_priority_),
                              cmp_, inserted);
    return inserted ? PersistentTreapSet(std::move(root), cmp_, next_priority_ + 1) : *this;
  }

  /** @brief Return a new version without `key`.
   *  @param key Key to erase.
   *  @return New set version. If the key is absent, returns an unchanged
   *          version sharing the same root.
   *  @throws std::bad_alloc or whatever `Compare` throws.
   */
  [[nodiscard]] PersistentTreapSet erase(const Key &key) const
  {
    bool erased = false;
    NodePtr root = erase_rec(root_, key, cmp_, erased);
    return erased ? PersistentTreapSet(std::move(root), cmp_, next_priority_) : *this;
  }

  /** @brief Split this version around `pivot`.
   *  @param pivot Split key.
   *  @return Pair `{less, greater_equal}` where the first set contains keys
   *          strictly less than `pivot` and the second contains the rest.
   *  @throws std::bad_alloc or whatever `Compare` throws.
   */
  [[nodiscard]] std::pair<PersistentTreapSet, PersistentTreapSet>
  split(const Key &pivot) const
  {
    auto [left, right] = split_rec(root_, pivot, cmp_);
    return {PersistentTreapSet(std::move(left), cmp_, next_priority_),
            PersistentTreapSet(std::move(right), cmp_, next_priority_)};
  }

  /** @brief Join two ordered, non-overlapping set versions.
   *  @param left Set whose keys must all be less than every key in `right`.
   *  @param right Set whose keys must all be greater than every key in `left`.
   *  @return Joined set version.
   *  @throws std::domain_error if the key ranges overlap or touch.
   *  @throws std::bad_alloc if node allocation fails.
   */
  [[nodiscard]] static PersistentTreapSet join(const PersistentTreapSet &left,
                                               const PersistentTreapSet &right)
  {
    ah_domain_error_unless(can_join(left.root_, right.root_, left.cmp_))
      << "PersistentTreapSet::join(): left keys must be strictly less than right keys";
    return PersistentTreapSet(join_nodes(left.root_, right.root_), left.cmp_,
                              std::max(left.next_priority_, right.next_priority_));
  }

  /** @brief Join this version with `right`.
   *  @param right Set whose keys must all be greater than every key in this set.
   *  @return Joined set version.
   *  @throws std::domain_error if the key ranges overlap or touch.
   *  @throws std::bad_alloc if node allocation fails.
   */
  [[nodiscard]] PersistentTreapSet join(const PersistentTreapSet &right) const
  {
    return join(*this, right);
  }

  /** @brief Return all keys in sorted order.
   *  @return Aleph array with a copy of every key.
   *  @throws std::bad_alloc or whatever copying `Key` throws.
   */
  [[nodiscard]] Array<Key> keys() const
    requires std::copy_constructible<Key>
  {
    Array<Key> out;
    out.reserve(size());
    collect_keys(root_, out);
    return out;
  }

  /** @brief Verify treap, BST and cached-size invariants.
   *  @return `true` if the internal structure is consistent.
   *  @throws Nothing unless `Compare` throws.
   *  @note Intended for tests and diagnostics; it is O(n).
   */
  [[nodiscard]] bool verify() const
  {
    size_t count = 0;
    return verify_rec(root_, nullptr, nullptr, cmp_, count) and count == size();
  }
};

/** @brief Immutable ordered map backed by a path-copying treap.
 *
 *  @tparam Key Key type.
 *  @tparam T Mapped value type.
 *  @tparam Compare Strict weak ordering used for keys. Defaults to
 *          `Aleph::less<Key>`.
 */
template <typename Key, typename T, class Compare = Aleph::less<Key>>
class PersistentTreapMap
{
  struct Node
  {
    std::shared_ptr<const Key> key;
    std::shared_ptr<const T> value;
    std::uint64_t priority = 0;
    size_t count = 1;
    std::shared_ptr<const Node> left;
    std::shared_ptr<const Node> right;
  };

  using NodePtr = std::shared_ptr<const Node>;

  NodePtr root_;
  Compare cmp_;
  std::uint64_t next_priority_ = 0;

  [[nodiscard]] static size_t node_size(const NodePtr &p) noexcept
  {
    return p == nullptr ? 0 : p->count;
  }

  [[nodiscard]] static NodePtr make_node(std::shared_ptr<const Key> key,
                                         std::shared_ptr<const T> value,
                                         const std::uint64_t priority,
                                         NodePtr left,
                                         NodePtr right)
  {
    const size_t left_size = node_size(left);
    const size_t right_size = node_size(right);
    ah_overflow_error_if(left_size > std::numeric_limits<size_t>::max() - right_size)
      << "PersistentTreapMap: node size would overflow";
    const size_t children_size = left_size + right_size;
    ah_overflow_error_if(children_size == std::numeric_limits<size_t>::max())
      << "PersistentTreapMap: node size would overflow";

    auto node = std::make_shared<Node>();
    node->key = std::move(key);
    node->value = std::move(value);
    node->priority = priority;
    node->left = std::move(left);
    node->right = std::move(right);
    node->count = children_size + 1;
    return node;
  }

  [[nodiscard]] static NodePtr rotate_right(const NodePtr &node)
  {
    const NodePtr child = node->left;
    NodePtr new_right = make_node(node->key, node->value, node->priority,
                                  child->right, node->right);
    return make_node(child->key, child->value, child->priority, child->left,
                     std::move(new_right));
  }

  [[nodiscard]] static NodePtr rotate_left(const NodePtr &node)
  {
    const NodePtr child = node->right;
    NodePtr new_left = make_node(node->key, node->value, node->priority,
                                 node->left, child->left);
    return make_node(child->key, child->value, child->priority,
                     std::move(new_left), child->right);
  }

  [[nodiscard]] static NodePtr insert_rec(const NodePtr &node,
                                          std::shared_ptr<const Key> key,
                                          std::shared_ptr<const T> value,
                                          const std::uint64_t priority,
                                          const Compare &cmp,
                                          bool &inserted)
  {
    if (node == nullptr)
      {
        inserted = true;
        return make_node(std::move(key), std::move(value), priority, nullptr, nullptr);
      }

    if (cmp(*key, *node->key))
      {
        NodePtr new_left = insert_rec(node->left, std::move(key), std::move(value),
                                      priority, cmp, inserted);
        if (not inserted)
          return node;
        NodePtr rebuilt = make_node(node->key, node->value, node->priority,
                                    std::move(new_left), node->right);
        return rebuilt->left->priority < rebuilt->priority ? rotate_right(rebuilt) : rebuilt;
      }

    if (cmp(*node->key, *key))
      {
        NodePtr new_right = insert_rec(node->right, std::move(key), std::move(value),
                                       priority, cmp, inserted);
        if (not inserted)
          return node;
        NodePtr rebuilt = make_node(node->key, node->value, node->priority,
                                    node->left, std::move(new_right));
        return rebuilt->right->priority < rebuilt->priority ? rotate_left(rebuilt) : rebuilt;
      }

    inserted = false;
    return node;
  }

  [[nodiscard]] static NodePtr insert_or_assign_rec(const NodePtr &node,
                                                    std::shared_ptr<const Key> key,
                                                    std::shared_ptr<const T> value,
                                                    const std::uint64_t priority,
                                                    const Compare &cmp,
                                                    bool &inserted)
  {
    if (node == nullptr)
      {
        inserted = true;
        return make_node(std::move(key), std::move(value), priority, nullptr, nullptr);
      }

    if (cmp(*key, *node->key))
      {
        NodePtr new_left = insert_or_assign_rec(node->left, std::move(key),
                                                std::move(value), priority, cmp,
                                                inserted);
        NodePtr rebuilt = make_node(node->key, node->value, node->priority,
                                    std::move(new_left), node->right);
        return inserted and rebuilt->left->priority < rebuilt->priority
          ? rotate_right(rebuilt) : rebuilt;
      }

    if (cmp(*node->key, *key))
      {
        NodePtr new_right = insert_or_assign_rec(node->right, std::move(key),
                                                 std::move(value), priority, cmp,
                                                 inserted);
        NodePtr rebuilt = make_node(node->key, node->value, node->priority,
                                    node->left, std::move(new_right));
        return inserted and rebuilt->right->priority < rebuilt->priority
          ? rotate_left(rebuilt) : rebuilt;
      }

    inserted = false;
    return make_node(node->key, std::move(value), node->priority, node->left, node->right);
  }

  [[nodiscard]] static NodePtr join_nodes(const NodePtr &left, const NodePtr &right)
  {
    if (left == nullptr)
      return right;
    if (right == nullptr)
      return left;

    if (left->priority <= right->priority)
      {
        NodePtr new_right = join_nodes(left->right, right);
        return make_node(left->key, left->value, left->priority, left->left,
                         std::move(new_right));
      }

    NodePtr new_left = join_nodes(left, right->left);
    return make_node(right->key, right->value, right->priority, std::move(new_left),
                     right->right);
  }

  [[nodiscard]] static NodePtr erase_rec(const NodePtr &node,
                                         const Key &key,
                                         const Compare &cmp,
                                         bool &erased)
  {
    if (node == nullptr)
      {
        erased = false;
        return nullptr;
      }

    if (cmp(key, *node->key))
      {
        NodePtr new_left = erase_rec(node->left, key, cmp, erased);
        if (not erased)
          return node;
        return make_node(node->key, node->value, node->priority,
                         std::move(new_left), node->right);
      }

    if (cmp(*node->key, key))
      {
        NodePtr new_right = erase_rec(node->right, key, cmp, erased);
        if (not erased)
          return node;
        return make_node(node->key, node->value, node->priority,
                         node->left, std::move(new_right));
      }

    erased = true;
    return join_nodes(node->left, node->right);
  }

  [[nodiscard]] static std::pair<NodePtr, NodePtr> split_rec(const NodePtr &node,
                                                             const Key &pivot,
                                                             const Compare &cmp)
  {
    if (node == nullptr)
      return {nullptr, nullptr};

    if (cmp(*node->key, pivot))
      {
        auto [right_left, right_right] = split_rec(node->right, pivot, cmp);
        NodePtr left = make_node(node->key, node->value, node->priority,
                                 node->left, std::move(right_left));
        return {std::move(left), std::move(right_right)};
      }

    auto [left_left, left_right] = split_rec(node->left, pivot, cmp);
    NodePtr right = make_node(node->key, node->value, node->priority,
                              std::move(left_right), node->right);
    return {std::move(left_left), std::move(right)};
  }

  [[nodiscard]] static const Key *min_key(const NodePtr &node) noexcept
  {
    const Node *curr = node.get();
    while (curr->left != nullptr)
      curr = curr->left.get();
    return curr->key.get();
  }

  [[nodiscard]] static const Key *max_key(const NodePtr &node) noexcept
  {
    const Node *curr = node.get();
    while (curr->right != nullptr)
      curr = curr->right.get();
    return curr->key.get();
  }

  [[nodiscard]] static bool can_join(const NodePtr &left,
                                     const NodePtr &right,
                                     const Compare &cmp)
  {
    return left == nullptr or right == nullptr or cmp(*max_key(left), *min_key(right));
  }

  static void collect_keys(const NodePtr &node, Array<Key> &out)
    requires std::copy_constructible<Key>
  {
    if (node == nullptr)
      return;
    collect_keys(node->left, out);
    out.append(Key(*node->key));
    collect_keys(node->right, out);
  }

  static void collect_items(const NodePtr &node, Array<std::pair<Key, T>> &out)
    requires (std::copy_constructible<Key> and std::copy_constructible<T>)
  {
    if (node == nullptr)
      return;
    collect_items(node->left, out);
    out.append(std::pair<Key, T>{*node->key, *node->value});
    collect_items(node->right, out);
  }

  [[nodiscard]] static bool verify_rec(const NodePtr &node,
                                       const Key *lo,
                                       const Key *hi,
                                       const Compare &cmp,
                                       size_t &count)
  {
    if (node == nullptr)
      return true;
    if (node->key == nullptr or node->value == nullptr)
      return false;
    if (lo != nullptr and not cmp(*lo, *node->key))
      return false;
    if (hi != nullptr and not cmp(*node->key, *hi))
      return false;
    if (node->left != nullptr and node->left->priority < node->priority)
      return false;
    if (node->right != nullptr and node->right->priority < node->priority)
      return false;

    size_t left_count = 0;
    size_t right_count = 0;
    if (not verify_rec(node->left, lo, node->key.get(), cmp, left_count))
      return false;
    if (not verify_rec(node->right, node->key.get(), hi, cmp, right_count))
      return false;
    if (node->count != left_count + right_count + 1)
      return false;
    count = node->count;
    return true;
  }

  PersistentTreapMap(NodePtr root, Compare cmp, const std::uint64_t next_priority)
    : root_(std::move(root)), cmp_(std::move(cmp)), next_priority_(next_priority)
  {
    // Empty.
  }

public:
  /** @brief Construct an empty persistent map.
   *  @param cmp Comparator used for key ordering.
   *  @throws Nothing unless copying `cmp` throws.
   */
  explicit PersistentTreapMap(Compare cmp = Compare())
    : cmp_(std::move(cmp))
  {
    // Empty.
  }

  /** @brief Return true when the map has no bindings.
   *  @return `true` iff `size() == 0`.
   *  @throws Nothing.
   */
  [[nodiscard]] bool is_empty() const noexcept { return root_ == nullptr; }

  /** @brief Return the number of key/value bindings in this version.
   *  @return Number of bindings.
   *  @throws Nothing.
   */
  [[nodiscard]] size_t size() const noexcept { return node_size(root_); }

  /** @brief Find a mapped value.
   *  @param key Key to search.
   *  @return Pointer to the stored mapped value, or `nullptr` when absent. The
   *          pointer remains valid while this map version is alive.
   *  @throws Whatever `Compare` throws.
   */
  [[nodiscard]] const T *find(const Key &key) const
  {
    const Node *curr = root_.get();
    while (curr != nullptr)
      if (cmp_(key, *curr->key))
        curr = curr->left.get();
      else if (cmp_(*curr->key, key))
        curr = curr->right.get();
      else
        return curr->value.get();
    return nullptr;
  }

  /** @brief Test whether `key` is present.
   *  @param key Key to search.
   *  @return `true` if an equivalent key is stored.
   *  @throws Whatever `Compare` throws.
   */
  [[nodiscard]] bool contains(const Key &key) const
  {
    return find(key) != nullptr;
  }

  /** @brief Return a new version with a binding inserted if absent.
   *  @tparam KArg Type used to construct `Key`.
   *  @tparam VArg Type used to construct `T`.
   *  @param key Key to insert.
   *  @param value Mapped value to insert.
   *  @return New map version. If an equivalent key already exists, returns an
   *          unchanged version sharing the same root.
   *  @throws std::bad_alloc or whatever construction/comparison throws.
   */
  template <typename KArg, typename VArg>
    requires std::constructible_from<Key, KArg &&> and std::constructible_from<T, VArg &&>
  [[nodiscard]] PersistentTreapMap insert(KArg &&key, VArg &&value) const
  {
    auto stored_key = std::make_shared<const Key>(std::forward<KArg>(key));
    auto stored_value = std::make_shared<const T>(std::forward<VArg>(value));
    bool inserted = false;
    NodePtr root = insert_rec(root_, std::move(stored_key), std::move(stored_value),
                              detail::persistent_treap_priority(next_priority_),
                              cmp_, inserted);
    return inserted ? PersistentTreapMap(std::move(root), cmp_, next_priority_ + 1) : *this;
  }

  /** @brief Return a new version with `key` assigned to `value`.
   *  @tparam KArg Type used to construct `Key` when the key is absent.
   *  @tparam VArg Type used to construct `T`.
   *  @param key Key to insert or update.
   *  @param value Mapped value for the returned version.
   *  @return New map version with the binding present.
   *  @throws std::bad_alloc or whatever construction/comparison throws.
   */
  template <typename KArg, typename VArg>
    requires std::constructible_from<Key, KArg &&> and std::constructible_from<T, VArg &&>
  [[nodiscard]] PersistentTreapMap insert_or_assign(KArg &&key, VArg &&value) const
  {
    auto stored_key = std::make_shared<const Key>(std::forward<KArg>(key));
    auto stored_value = std::make_shared<const T>(std::forward<VArg>(value));
    bool inserted = false;
    NodePtr root = insert_or_assign_rec(root_, std::move(stored_key),
                                        std::move(stored_value),
                                        detail::persistent_treap_priority(next_priority_),
                                        cmp_, inserted);
    return PersistentTreapMap(std::move(root), cmp_, inserted ? next_priority_ + 1
                                                              : next_priority_);
  }

  /** @brief Return a new version without `key`.
   *  @param key Key to erase.
   *  @return New map version. If the key is absent, returns an unchanged
   *          version sharing the same root.
   *  @throws std::bad_alloc or whatever `Compare` throws.
   */
  [[nodiscard]] PersistentTreapMap erase(const Key &key) const
  {
    bool erased = false;
    NodePtr root = erase_rec(root_, key, cmp_, erased);
    return erased ? PersistentTreapMap(std::move(root), cmp_, next_priority_) : *this;
  }

  /** @brief Split this version around `pivot`.
   *  @param pivot Split key.
   *  @return Pair `{less, greater_equal}` where the first map contains keys
   *          strictly less than `pivot` and the second contains the rest.
   *  @throws std::bad_alloc or whatever `Compare` throws.
   */
  [[nodiscard]] std::pair<PersistentTreapMap, PersistentTreapMap>
  split(const Key &pivot) const
  {
    auto [left, right] = split_rec(root_, pivot, cmp_);
    return {PersistentTreapMap(std::move(left), cmp_, next_priority_),
            PersistentTreapMap(std::move(right), cmp_, next_priority_)};
  }

  /** @brief Join two ordered, non-overlapping map versions.
   *  @param left Map whose keys must all be less than every key in `right`.
   *  @param right Map whose keys must all be greater than every key in `left`.
   *  @return Joined map version.
   *  @throws std::domain_error if the key ranges overlap or touch.
   *  @throws std::bad_alloc if node allocation fails.
   */
  [[nodiscard]] static PersistentTreapMap join(const PersistentTreapMap &left,
                                               const PersistentTreapMap &right)
  {
    ah_domain_error_unless(can_join(left.root_, right.root_, left.cmp_))
      << "PersistentTreapMap::join(): left keys must be strictly less than right keys";
    return PersistentTreapMap(join_nodes(left.root_, right.root_), left.cmp_,
                              std::max(left.next_priority_, right.next_priority_));
  }

  /** @brief Join this version with `right`.
   *  @param right Map whose keys must all be greater than every key in this map.
   *  @return Joined map version.
   *  @throws std::domain_error if the key ranges overlap or touch.
   *  @throws std::bad_alloc if node allocation fails.
   */
  [[nodiscard]] PersistentTreapMap join(const PersistentTreapMap &right) const
  {
    return join(*this, right);
  }

  /** @brief Return all keys in sorted order.
   *  @return Aleph array with a copy of every key.
   *  @throws std::bad_alloc or whatever copying `Key` throws.
   */
  [[nodiscard]] Array<Key> keys() const
    requires std::copy_constructible<Key>
  {
    Array<Key> out;
    out.reserve(size());
    collect_keys(root_, out);
    return out;
  }

  /** @brief Return all key/value bindings in sorted-key order.
   *  @return Aleph array with a copy of every binding.
   *  @throws std::bad_alloc or whatever copying `Key` or `T` throws.
   */
  [[nodiscard]] Array<std::pair<Key, T>> items() const
    requires (std::copy_constructible<Key> and std::copy_constructible<T>)
  {
    Array<std::pair<Key, T>> out;
    out.reserve(size());
    collect_items(root_, out);
    return out;
  }

  /** @brief Verify treap, BST and cached-size invariants.
   *  @return `true` if the internal structure is consistent.
   *  @throws Nothing unless `Compare` throws.
   *  @note Intended for tests and diagnostics; it is O(n).
   */
  [[nodiscard]] bool verify() const
  {
    size_t count = 0;
    return verify_rec(root_, nullptr, nullptr, cmp_, count) and count == size();
  }
};

} // namespace Aleph

#endif // TPL_PERSISTENT_TREAP_H
