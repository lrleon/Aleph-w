
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

/** @file tpl_radix_tree.H
 *  @brief Compressed prefix tree (`Aleph::RadixTree`) mapping string keys to
 *         values.
 *
 *  `RadixTree<T, Char>` is a radix tree (a.k.a. compressed trie / PATRICIA-
 *  style trie): unlike a classic trie -- where every edge is labeled with a
 *  single character, so a chain of nodes each having exactly one child wastes
 *  a node per character -- a radix tree merges any such chain into a single
 *  edge labeled with the whole shared substring. Every internal node either
 *  stores a value (a key ends there) or has at least two children; a node
 *  that would otherwise have exactly one child and no value is never created
 *  by `insert()`, and is merged away by `erase()`.
 *
 *  @par How it differs from `prefix-tree.H`
 *  `Prefix_Tree` (in `prefix-tree.H`) is a classic uncompressed trie: one
 *  node per character, which is simple and cheap to reason about but uses
 *  O(key length) nodes per distinct branching path even when there is no
 *  actual branching. `RadixTree` uses O(number of branch/leaf points) nodes
 *  instead, which is asymptotically smaller for key sets with long shared or
 *  unique substrings (e.g. URLs, filesystem paths, IP-prefix-like keys), at
 *  the cost of a slightly more involved insert/erase (edge splitting and
 *  merging) than a plain trie.
 *
 *  @par Ownership and iterator/reference invalidation
 *  Every node is owned exactly once, via `std::unique_ptr`, by its parent
 *  (or by the tree itself, for the root); destroying the tree recursively
 *  destroys every node. `find()` returns a raw pointer into the tree: it
 *  stays valid until the next non-`const` operation on the tree (`insert`,
 *  `insert_or_assign`, `erase`, move-assignment, or destruction), exactly
 *  like an `std::map` reference/iterator. `RadixTree` has no live iteration
 *  API; `keys_with_prefix()` returns an independent `Array` of copied keys
 *  instead.
 *
 *  @par Complexity
 *  For a key of length `n`: `insert`, `erase`, `find`, `contains`, and
 *  `longest_prefix` are O(n) plus, at each of the O(number of edges on the
 *  path) branch points visited, a linear scan over that node's children
 *  (bounded by the alphabet size, but in practice small for real key sets).
 *  `keys_with_prefix` is O(prefix length) to reach the subtree plus O(size of
 *  the matching subtree) to collect it.
 *
 *  @par Thread-safety
 *  None. `RadixTree` is an ordinary sequential container; external
 *  synchronization is required for concurrent use (see
 *  `Aleph::ConcurrentHashMap` in `tpl_concurrent_hash_map.H` for a
 *  concurrency-safe associative container).
 *
 *  @see prefix-tree.H for `Aleph::Prefix_Tree`, the uncompressed character
 *       trie this type complements.
 *  @ingroup Trees
 *  @author Leandro Rabindranath Leon
 */

#ifndef TPL_RADIX_TREE_H
#define TPL_RADIX_TREE_H

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include <ah-errors.H>
#include <tpl_array.H>

namespace Aleph
{

/** @brief Compressed prefix tree mapping `std::basic_string<Char>` keys to
 *         values of type `T`.
 *
 *  See the file-level documentation in `tpl_radix_tree.H` for the full
 *  design rationale, complexity, and ownership/invalidation contract.
 *
 *  @tparam T Mapped value type. Must be move-constructible; copy-
 *          constructible only if `RadixTree` itself is copied.
 *  @tparam Char Character type of keys (default `char`). Keys are
 *          `std::basic_string<Char>`.
 */
template <typename T, typename Char = char>
class RadixTree
{
public:
  /// Key type: strings over `Char`.
  using Key = std::basic_string<Char>;

private:
  struct Node
  {
    Key edge_label;             // label of the edge from the parent to this
                                 // node; empty only for the root.
    std::optional<T> value;     // set iff some key ends exactly at this node.
    Array<std::pair<Char, std::unique_ptr<Node>>> children;
                                 // unordered; a node's children always have
                                 // pairwise-distinct first characters in
                                 // their own edge_label.

    Node() = default;
    explicit Node(Key label) : edge_label(std::move(label)) {}
  };

  static constexpr size_t npos = static_cast<size_t>(-1);

  std::unique_ptr<Node> root_;
  size_t size_ = 0;

  [[nodiscard]] static Node * find_child(Node * node, const Char c) noexcept
  {
    for (auto & kv : node->children)
      if (kv.first == c)
        return kv.second.get();
    return nullptr;
  }

  [[nodiscard]] static const Node * find_child(const Node * node,
                                                const Char c) noexcept
  {
    for (const auto & kv : node->children)
      if (kv.first == c)
        return kv.second.get();
    return nullptr;
  }

  [[nodiscard]] static size_t find_child_slot(Node * node,
                                               const Char c) noexcept
  {
    for (size_t i = 0; i < node->children.size(); ++i)
      if (node->children[i].first == c)
        return i;
    return npos;
  }

  static void add_child(Node * node, const Char c, std::unique_ptr<Node> child)
  {
    node->children.append(std::make_pair(c, std::move(child)));
  }

  /// Remove the child at `idx` in O(1) by swapping with the last slot; the
  /// (unordered) children array does not need to preserve position.
  static void remove_child_at(Node * node, const size_t idx)
  {
    const size_t last = node->children.size() - 1;
    if (idx != last)
      std::swap(node->children[idx], node->children[last]);
    std::ignore = node->children.remove_last();
  }

  /// Length of the common prefix between `label` and `key[key_off..)`.
  [[nodiscard]] static size_t common_prefix_length(const Key & label,
                                                     const Key & key,
                                                     const size_t key_off) noexcept
  {
    const size_t max_n = std::min(label.size(), key.size() - key_off);
    size_t n = 0;
    while (n < max_n and label[n] == key[key_off + n])
      ++n;
    return n;
  }

  /// Shared implementation for the copy- and move-value `insert()`
  /// overloads: `U` is deduced as `const T&` or `T&&` at the call site.
  template <typename U>
  bool insert_impl(const Key & key, U && value)
  {
    Node * cur = root_.get();
    size_t i = 0;

    for (;;)
      {
        if (i == key.size())
          {
            if (cur->value.has_value())
              return false;
            cur->value.emplace(std::forward<U>(value));
            ++size_;
            return true;
          }

        const Char c = key[i];
        const size_t slot = find_child_slot(cur, c);

        if (slot == npos)
          {
            auto leaf = std::make_unique<Node>(key.substr(i));
            leaf->value.emplace(std::forward<U>(value));
            add_child(cur, c, std::move(leaf));
            ++size_;
            return true;
          }

        Node * child = cur->children[slot].second.get();
        const Key & label = child->edge_label;
        const size_t remaining = key.size() - i;
        const size_t lcp = common_prefix_length(label, key, i);

        if (lcp == label.size())
          {
            // The whole edge matches; keep descending.
            i += lcp;
            cur = child;
            continue;
          }

        // `label` only partially matches: split it at `lcp` (lcp >= 1,
        // since `child` was found via its first character matching `c`).
        Key split_label = label.substr(0, lcp);
        Key child_suffix = label.substr(lcp);
        auto split = std::make_unique<Node>(std::move(split_label));
        split->children.reserve(lcp == remaining ? 1 : 2);

        if (lcp == remaining)
          {
            // The new key ends exactly at the split point.
            split->value.emplace(std::forward<U>(value));
          }
        else
          {
            auto leaf = std::make_unique<Node>(key.substr(i + lcp));
            leaf->value.emplace(std::forward<U>(value));
            const Char leaf_first = leaf->edge_label[0];
            add_child(split.get(), leaf_first, std::move(leaf));
          }

        child->edge_label = std::move(child_suffix);
        const Char child_first = child->edge_label[0];

        split->children.append(std::make_pair(child_first, std::unique_ptr<Node>{}));
        std::unique_ptr<Node> detached = std::move(cur->children[slot].second);
        split->children[split->children.size() - 1].second = std::move(detached);
        cur->children[slot].second = std::move(split);
        ++size_;
        return true;
      }
  }

  [[nodiscard]] const Node * find_node(const Key & key) const noexcept
  {
    const Node * cur = root_.get();
    size_t i = 0;
    while (i < key.size())
      {
        const Char c = key[i];
        const Node * child = find_child(cur, c);
        if (child == nullptr)
          return nullptr;

        const Key & label = child->edge_label;
        const size_t remaining = key.size() - i;
        if (label.size() > remaining or
            key.compare(i, label.size(), label) != 0)
          return nullptr;

        i += label.size();
        cur = child;
      }
    return cur;
  }

  static void collect_keys(const Node * node, Key & prefix_acc,
                            Array<Key> & out)
  {
    if (node->value.has_value())
      out.append(prefix_acc);
    for (const auto & kv : node->children)
      {
        const size_t old_size = prefix_acc.size();
        prefix_acc += kv.second->edge_label;
        collect_keys(kv.second.get(), prefix_acc, out);
        prefix_acc.resize(old_size);
      }
  }

  [[nodiscard]] static std::unique_ptr<Node> clone_node(const Node * src)
    requires std::is_copy_constructible_v<T>
  {
    auto dst = std::make_unique<Node>(src->edge_label);
    if (src->value.has_value())
      dst->value.emplace(*src->value);
    dst->children.reserve(src->children.size());
    for (const auto & kv : src->children)
      dst->children.append(std::make_pair(kv.first, clone_node(kv.second.get())));
    return dst;
  }

public:
  /** @brief Construct an empty tree.
   *  @throws std::bad_alloc if allocating the root node fails.
   */
  RadixTree() : root_(std::make_unique<Node>()) {}

  /// @brief Destructor. Recursively releases every node (via the
  /// `unique_ptr` ownership chain); tree depth is bounded by the number of
  /// distinct branch points, not by key length, so this does not risk stack
  /// overflow for the deep-but-narrow trees that motivated compression.
  ~RadixTree() = default;

  /** @brief Move constructor: `other` is left as a valid, empty tree.
   *  @throws std::bad_alloc if allocating the replacement empty root for
   *          `other` fails; in that case `other` is left unchanged.
   */
  RadixTree(RadixTree && other)
    : root_(std::make_unique<Node>()), size_(0)
  {
    root_.swap(other.root_);
    std::swap(size_, other.size_);
  }

  /** @brief Move assignment operator: `other` is left as a valid, empty tree.
   *  @throws std::bad_alloc if allocating the replacement empty root for
   *          `other` fails; in that case both trees are left unchanged.
   */
  RadixTree & operator=(RadixTree && other)
  {
    if (this != &other)
      {
        auto empty_root = std::make_unique<Node>();
        root_ = std::move(other.root_);
        size_ = other.size_;
        other.root_ = std::move(empty_root);
        other.size_ = 0;
      }
    return *this;
  }

  /** @brief Deep-copy constructor: every node is cloned independently.
   *  @throws Whatever `T`'s copy constructor throws, or `std::bad_alloc`.
   */
  RadixTree(const RadixTree & other)
    requires std::is_copy_constructible_v<T>
    : root_(clone_node(other.root_.get())), size_(other.size_)
  {}

  /** @brief Deep-copy assignment operator.
   *  @throws Whatever `T`'s copy constructor throws, or `std::bad_alloc`.
   */
  RadixTree & operator=(const RadixTree & other)
    requires std::is_copy_constructible_v<T>
  {
    if (this != &other)
      {
        root_ = clone_node(other.root_.get());
        size_ = other.size_;
      }
    return *this;
  }

  /** @brief Return the number of keys currently stored.
   *  @throws Nothing.
   */
  [[nodiscard]] size_t size() const noexcept { return size_; }

  /** @brief Check whether the tree holds no keys.
   *  @throws Nothing.
   */
  [[nodiscard]] bool is_empty() const noexcept { return size_ == 0; }

  /** @brief Insert `key` with a copy of `value`, only if `key` is absent.
   *
   *  @param[in] key Key to insert. The empty string is a valid key (mapped
   *             to the root's own value slot).
   *  @param[in] value Value to copy in.
   *  @return `true` if inserted, `false` if `key` was already present (the
   *          existing value is left untouched).
   *  @throws Whatever `T`'s copy constructor throws, or `std::bad_alloc`.
   */
  bool insert(const Key & key, const T & value)
  {
    return insert_impl(key, value);
  }

  /** @brief Insert `key` with `value` moved in, only if `key` is absent.
   *
   *  @param[in] key Key to insert.
   *  @param[in,out] value Value to move in.
   *  @return `true` if inserted, `false` if `key` was already present.
   *  @throws Whatever `T`'s move constructor throws, or `std::bad_alloc`.
   *  @warning On a `false` return (duplicate `key`), `value` is left
   *           unmodified: unlike `Aleph::ConcurrentHashMap`, this
   *           implementation checks for the duplicate before touching
   *           `value` (no forwarding-then-discarding path exists here).
   */
  bool insert(const Key & key, T && value)
  {
    return insert_impl(key, std::move(value));
  }

  /** @brief Insert `key` with `value`, or overwrite the existing value if
   *         `key` is already present.
   *
   *  @param[in] key Key to insert or update.
   *  @param[in] value New value.
   *  @throws Whatever `T`'s move/copy constructor or move assignment
   *          throws, or `std::bad_alloc`.
   */
  void insert_or_assign(const Key & key, T value)
  {
    T * slot = find(key);
    if (slot != nullptr)
      *slot = std::move(value);
    else
      insert_impl(key, std::move(value));
  }

  /** @brief Remove `key` if present, merging any resulting single-child,
   *         valueless node back into a compressed edge.
   *
   *  @param[in] key Key to remove.
   *  @return `true` if `key` was present and removed, `false` otherwise.
   *  @throws std::bad_alloc if merging edge labels cannot allocate; the tree
   *          remains valid, though it may be temporarily less compressed.
   */
  bool erase(const Key & key)
  {
    struct Frame { Node * parent; size_t slot; };
    Array<Frame> path;

    Node * cur = root_.get();
    size_t i = 0;
    while (i < key.size())
      {
        const Char c = key[i];
        const size_t slot = find_child_slot(cur, c);
        if (slot == npos)
          return false;

        Node * child = cur->children[slot].second.get();
        const Key & label = child->edge_label;
        const size_t remaining = key.size() - i;
        if (label.size() > remaining or
            key.compare(i, label.size(), label) != 0)
          return false;

        path.append(Frame{cur, slot});
        i += label.size();
        cur = child;
      }

    if (not cur->value.has_value())
      return false;

    cur->value.reset();
    --size_;

    // Bubble upward: prune now-empty leaves, and merge any node left with
    // exactly one child and no value back into a single compressed edge.
    while (not path.is_empty())
      {
        const Frame f = path.get_last();
        std::ignore = path.remove_last();
        Node * node = f.parent->children[f.slot].second.get();

        if (node->children.is_empty() and not node->value.has_value())
          {
            remove_child_at(f.parent, f.slot);
            continue;  // f.parent's shape changed; the next frame re-checks it.
          }

        if (node->children.size() == 1 and not node->value.has_value())
          {
            auto & only = node->children[0];
            only.second->edge_label = node->edge_label + only.second->edge_label;
            f.parent->children[f.slot].second = std::move(only.second);
          }

        break;  // Nothing above `node` changed shape; stop bubbling.
      }

    return true;
  }

  /** @brief Check whether `key` is present.
   *  @param[in] key Key to look up.
   *  @return `true` if present.
   *  @throws Nothing.
   */
  [[nodiscard]] bool contains(const Key & key) const noexcept
  {
    return find(key) != nullptr;
  }

  /** @brief Look up `key`.
   *  @param[in] key Key to look up.
   *  @return Pointer to the stored value if `key` is present, `nullptr`
   *          otherwise. Valid until the next non-`const` operation on this
   *          tree.
   *  @throws Nothing.
   */
  [[nodiscard]] const T * find(const Key & key) const noexcept
  {
    const Node * n = find_node(key);
    return (n != nullptr and n->value.has_value()) ? &*n->value : nullptr;
  }

  /// @brief Non-`const` overload of find(const Key&).
  [[nodiscard]] T * find(const Key & key) noexcept
  {
    const Node * n = find_node(key);
    return (n != nullptr and n->value.has_value())
             ? const_cast<T *>(&*n->value)
             : nullptr;
  }

  /** @brief Find the longest stored key that is a prefix of `key`.
   *
   *  @param[in] key Key to match against.
   *  @return The longest `k` such that `k` is a stored key and a prefix of
   *          `key` (which may be `key` itself), or `std::nullopt` if no
   *          stored key is a prefix of `key`. Call `find()` on the result
   *          to get the associated value.
   *  @throws Whatever allocating the returned `Key` throws.
   *
   *  @note This walks the same path `find()` would, so it is well suited
   *        to longest-prefix-match use cases (e.g. routing tables,
   *        hierarchical settings lookup) without requiring `T` to be
   *        copy-constructible (only `Key`, always a `std::basic_string`,
   *        is copied).
   */
  [[nodiscard]] std::optional<Key> longest_prefix(const Key & key) const
  {
    std::optional<Key> best;
    const Node * cur = root_.get();
    size_t i = 0;

    if (cur->value.has_value())
      best = Key{};

    while (i < key.size())
      {
        const Char c = key[i];
        const Node * child = find_child(cur, c);
        if (child == nullptr)
          break;

        const Key & label = child->edge_label;
        const size_t remaining = key.size() - i;
        if (label.size() > remaining or
            key.compare(i, label.size(), label) != 0)
          break;

        i += label.size();
        cur = child;
        if (cur->value.has_value())
          best = key.substr(0, i);
      }

    return best;
  }

  /** @brief Return every stored key that starts with `prefix`.
   *
   *  @param[in] prefix Prefix to match (the empty string matches every
   *             key).
   *  @return An independent `Array` of matching keys, in an unspecified
   *          order.
   *  @throws Whatever allocating the returned keys/array throws.
   */
  [[nodiscard]] Array<Key> keys_with_prefix(const Key & prefix) const
  {
    Array<Key> result;
    const Node * cur = root_.get();
    size_t i = 0;

    while (i < prefix.size())
      {
        const Char c = prefix[i];
        const Node * child = find_child(cur, c);
        if (child == nullptr)
          return result;

        const Key & label = child->edge_label;
        const size_t remaining = prefix.size() - i;
        const size_t lcp = common_prefix_length(label, prefix, i);

        if (lcp < label.size())
          {
            if (lcp != remaining)
              return result;  // genuine mismatch: nothing shares `prefix`.
            // `prefix` runs out partway through this edge: every key under
            // `child` still starts with `prefix` (the edge is consistent
            // with it as far as it goes).
            Key base = prefix.substr(0, i) + label;
            collect_keys(child, base, result);
            return result;
          }

        i += lcp;
        cur = child;
      }

    Key base = prefix;
    collect_keys(cur, base, result);
    return result;
  }

  /** @brief Recursively verify the tree's structural invariants.
   *
   *  Checks, for every node:
   *  - No non-root node has an empty edge label.
   *  - A child's edge label always starts with the character it is
   *    indexed under in its parent's `children` array.
   *  - No two children of the same node have edge labels starting with
   *    the same character (the invariant `find_child`/`find_child_slot`
   *    rely on to unambiguously pick a child from a single character).
   *  - No non-root node has exactly one child and no value: such a node
   *    is "wasted" (uncompressed) -- `insert_impl()` never creates one,
   *    and `erase()` must merge it back into a single edge.
   *  - The number of nodes holding a value equals `size()` (catches
   *    `size_` bookkeeping drift independently of the structural checks
   *    above).
   *
   *  @return `true` if every invariant holds.
   *  @note Not part of the type's regular contract: a debug/test-only
   *        tool to catch structural corruption that black-box testing
   *        (`find`/`contains`/`keys_with_prefix`) might not expose if it
   *        happens not to affect the specific operations exercised
   *        afterward. `Tests/radix_tree_test.cc` calls this after every
   *        mutation in its randomized parity tests.
   */
  [[nodiscard]] bool check_invariants() const
  {
    size_t counted_values = 0;
    const bool structurally_valid =
        check_invariants_rec(root_.get(), true, counted_values);
    return structurally_valid and counted_values == size_;
  }

private:
  static bool check_invariants_rec(const Node * node, const bool is_root,
                                    size_t & counted_values)
  {
    if (node->value.has_value())
      ++counted_values;

    if (not is_root)
      {
        if (node->edge_label.empty())
          return false;
        if (node->children.size() == 1 and not node->value.has_value())
          return false;
      }

    for (size_t i = 0; i < node->children.size(); ++i)
      for (size_t j = i + 1; j < node->children.size(); ++j)
        if (node->children[i].first == node->children[j].first)
          return false;

    for (const auto & kv : node->children)
      {
        if (kv.second->edge_label.empty() or kv.second->edge_label[0] != kv.first)
          return false;
        if (not check_invariants_rec(kv.second.get(), false, counted_values))
          return false;
      }

    return true;
  }
};

} // namespace Aleph

#endif // TPL_RADIX_TREE_H
