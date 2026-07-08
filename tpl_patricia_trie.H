
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

/** @file tpl_patricia_trie.H
 *  @brief PATRICIA/crit-bit set and map for fixed-width unsigned integer keys.
 *
 *  `PatriciaSet<UInt>` stores unsigned integral keys in a compressed binary
 *  trie. Internal nodes keep only the first bit where keys in their two
 *  subtrees differ; leaves store the complete key. This is the fixed-width
 *  integer variant requested by the missing-data-structures plan as the first
 *  PATRICIA deliverable after `RadixTree`.
 *
 *  `PatriciaMap<UInt, T>` uses the same bitwise structure and attaches a
 *  mapped value to each stored key, making it the fixed-width integer-key
 *  dictionary counterpart to `RadixTree`.
 *
 *  @par Relationship with `RadixTree`
 *  `RadixTree<T, Char>` compresses string edges and maps keys to values.
 *  `PatriciaSet<UInt>` is a set-only structure over machine integers, and
 *  `PatriciaMap<UInt, T>` is the analogous integer-key map. Both Patricia
 *  variants use individual bits rather than characters. They are a good fit
 *  for dense routing tables, bit prefixes, integer identifiers, and other
 *  fixed-width key workloads.
 *
 *  @par Complexity
 *  Let `w = std::numeric_limits<UInt>::digits`. Exact lookup, insertion, and
 *  erasure are O(w). The depth is bounded by `w`, independent of the number of
 *  stored keys. Enumerating `keys()` is O(n), where `n == size()`.
 *
 *  @par Ownership and invalidation
 *  Nodes are owned by `std::unique_ptr`. `PatriciaMap::find()` returns a raw
 *  pointer into the map; it is invalidated by the next non-const operation on
 *  that map. Copying performs a deep clone; moving transfers ownership and
 *  leaves the source empty.
 *
 *  @par Thread-safety
 *  None. This is an ordinary sequential container; concurrent callers must
 *  provide external synchronization.
 *
 *  @ingroup Trees
 *  @author Leandro Rabindranath Leon
 */

#ifndef TPL_PATRICIA_TRIE_H
#define TPL_PATRICIA_TRIE_H

#include <array>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

#include <tpl_array.H>

namespace Aleph
{

/** @brief Compressed bitwise set for unsigned integral keys.
 *
 *  `PatriciaSet` is a PATRICIA/crit-bit tree over the fixed-width bit pattern
 *  of `UInt`. It stores unique keys only; use `RadixTree` when string keys or
 *  mapped values are required.
 *
 *  @tparam UInt Unsigned integral key type. `bool` is rejected because it has
 *          only two values and does not benefit from a trie representation.
 */
template <typename UInt>
requires (std::is_integral_v<UInt> and std::is_unsigned_v<UInt> and
          not std::is_same_v<UInt, bool>)
class PatriciaSet
{
public:
  /// Key type stored by the set.
  using Key = UInt;

  /// Number of significant bits in `Key`.
  static constexpr size_t bit_width = std::numeric_limits<Key>::digits;

private:
  struct Node
  {
    bool leaf = true;
    Key key = 0;
    size_t bit_index = 0;
    std::array<std::unique_ptr<Node>, 2> child{};

    explicit Node(const Key k) noexcept : leaf(true), key(k) {}
    explicit Node(const size_t bit) noexcept : leaf(false), bit_index(bit) {}
  };

  std::unique_ptr<Node> root_;
  size_t size_ = 0;

  [[nodiscard]] static bool bit_at(const Key key, const size_t bit_index) noexcept
  {
    return ((key >> (bit_width - 1 - bit_index)) & Key{1}) != Key{0};
  }

  [[nodiscard]] static size_t first_differing_bit(const Key lhs,
                                                   const Key rhs) noexcept
  {
    const Key diff = lhs ^ rhs;
    for (size_t i = 0; i < bit_width; ++i)
      if (bit_at(diff, i))
        return i;
    return bit_width;
  }

  [[nodiscard]] static const Node * leaf_for(const Node * node,
                                             const Key key) noexcept
  {
    while (node != nullptr and not node->leaf)
      node = node->child[bit_at(key, node->bit_index) ? 1 : 0].get();
    return node;
  }

  static void collect_keys(const Node * node, Array<Key> & out)
  {
    if (node == nullptr)
      return;
    if (node->leaf)
      {
        out.append(node->key);
        return;
      }
    collect_keys(node->child[0].get(), out);
    collect_keys(node->child[1].get(), out);
  }

  [[nodiscard]] static std::unique_ptr<Node> clone_node(const Node * src)
  {
    if (src == nullptr)
      return nullptr;
    auto dst = src->leaf ? std::make_unique<Node>(src->key)
                         : std::make_unique<Node>(src->bit_index);
    if (not src->leaf)
      {
        dst->child[0] = clone_node(src->child[0].get());
        dst->child[1] = clone_node(src->child[1].get());
      }
    return dst;
  }

public:
  /** @brief Construct an empty set.
   *  @throws Nothing.
   */
  PatriciaSet() = default;

  /** @brief Destructor. Recursively releases every node.
   *  @throws Nothing.
   */
  ~PatriciaSet() = default;

  /** @brief Deep-copy constructor.
   *  @param[in] other Set to copy.
   *  @throws std::bad_alloc if cloning nodes fails.
   */
  PatriciaSet(const PatriciaSet & other)
    : root_(clone_node(other.root_.get())), size_(other.size_)
  {}

  /** @brief Deep-copy assignment.
   *  @param[in] other Set to copy.
   *  @return `*this`.
   *  @throws std::bad_alloc if cloning nodes fails.
   */
  PatriciaSet & operator=(const PatriciaSet & other)
  {
    if (this != &other)
      {
        root_ = clone_node(other.root_.get());
        size_ = other.size_;
      }
    return *this;
  }

  /** @brief Move constructor.
   *  @param[in,out] other Source set, left empty.
   *  @throws Nothing.
   */
  PatriciaSet(PatriciaSet && other) noexcept
    : root_(std::move(other.root_)), size_(other.size_)
  {
    other.size_ = 0;
  }

  /** @brief Move assignment.
   *  @param[in,out] other Source set, left empty.
   *  @return `*this`.
   *  @throws Nothing.
   */
  PatriciaSet & operator=(PatriciaSet && other) noexcept
  {
    if (this != &other)
      {
        root_ = std::move(other.root_);
        size_ = other.size_;
        other.size_ = 0;
      }
    return *this;
  }

  /** @brief Return the number of stored keys.
   *  @return Current cardinality.
   *  @throws Nothing.
   */
  [[nodiscard]] size_t size() const noexcept
  {
    return size_;
  }

  /** @brief Check whether the set has no keys.
   *  @return `true` iff `size() == 0`.
   *  @throws Nothing.
   */
  [[nodiscard]] bool is_empty() const noexcept
  {
    return size_ == 0;
  }

  /** @brief Remove every key from the set.
   *  @throws Nothing.
   */
  void clear() noexcept
  {
    root_.reset();
    size_ = 0;
  }

  /** @brief Check whether `key` is stored.
   *  @param[in] key Key to query.
   *  @return `true` if `key` is present.
   *  @throws Nothing.
   */
  [[nodiscard]] bool contains(const Key key) const noexcept
  {
    const Node * leaf = leaf_for(root_.get(), key);
    return leaf != nullptr and leaf->key == key;
  }

  /** @brief Insert `key` if absent.
   *  @param[in] key Key to insert.
   *  @return `true` if inserted, `false` if it was already present.
   *  @throws std::bad_alloc if allocating a new leaf or internal node fails.
   */
  bool insert(const Key key)
  {
    if (root_ == nullptr)
      {
        root_ = std::make_unique<Node>(key);
        size_ = 1;
        return true;
      }

    const Node * existing = leaf_for(root_.get(), key);
    if (existing->key == key)
      return false;

    const size_t diff_bit = first_differing_bit(key, existing->key);
    auto * link = &root_;
    while ((*link)->leaf == false and (*link)->bit_index < diff_bit)
      link = &(*link)->child[bit_at(key, (*link)->bit_index) ? 1 : 0];

    auto branch = std::make_unique<Node>(diff_bit);
    const size_t side = bit_at(key, diff_bit) ? 1 : 0;
    branch->child[side] = std::make_unique<Node>(key);
    branch->child[1 - side] = std::move(*link);
    *link = std::move(branch);
    ++size_;
    return true;
  }

  /** @brief Remove `key` if present.
   *  @param[in] key Key to erase.
   *  @return `true` if erased, `false` if absent.
   *  @throws Nothing.
   */
  bool erase(const Key key) noexcept
  {
    if (root_ == nullptr)
      return false;

    if (root_->leaf)
      {
        if (root_->key != key)
          return false;
        root_.reset();
        size_ = 0;
        return true;
      }

    std::unique_ptr<Node> * parent_link = nullptr;
    std::unique_ptr<Node> * link = &root_;
    while ((*link)->leaf == false)
      {
        parent_link = link;
        link = &(*link)->child[bit_at(key, (*link)->bit_index) ? 1 : 0];
      }

    if ((*link)->key != key)
      return false;

    Node * parent = parent_link->get();
    const size_t side = bit_at(key, parent->bit_index) ? 1 : 0;
    *parent_link = std::move(parent->child[1 - side]);
    --size_;
    return true;
  }

  /** @brief Return all stored keys.
   *  @return Independent `Array` containing every key in unspecified order.
   *  @throws std::bad_alloc if the returned array cannot grow.
   */
  [[nodiscard]] Array<Key> keys() const
  {
    Array<Key> result;
    result.reserve(size_);
    collect_keys(root_.get(), result);
    return result;
  }

  /** @brief Verify structural invariants recursively.
   *
   *  Checks that internal nodes have two children, internal bit indexes
   *  strictly increase down every path, each child subtree matches the
   *  routing bit used to reach it, and the number of leaves equals `size()`.
   *
   *  @return `true` iff all invariants hold.
   *  @throws Nothing.
   */
  [[nodiscard]] bool check_invariants() const noexcept
  {
    size_t counted = 0;
    const bool ok = check_invariants_rec(root_.get(), 0, false, counted);
    return ok and counted == size_;
  }

private:
  [[nodiscard]] static bool subtree_matches(const Node * node,
                                            const size_t bit_index,
                                            const bool expected) noexcept
  {
    if (node == nullptr)
      return false;
    if (node->leaf)
      return bit_at(node->key, bit_index) == expected;
    return subtree_matches(node->child[0].get(), bit_index, expected) and
           subtree_matches(node->child[1].get(), bit_index, expected);
  }

  [[nodiscard]] static bool check_invariants_rec(const Node * node,
                                                  const size_t parent_bit,
                                                  const bool has_parent,
                                                  size_t & counted) noexcept
  {
    if (node == nullptr)
      return true;

    if (node->leaf)
      {
        ++counted;
        return true;
      }

    if (node->bit_index >= bit_width)
      return false;
    if (has_parent and node->bit_index <= parent_bit)
      return false;
    if (node->child[0] == nullptr or node->child[1] == nullptr)
      return false;
    if (not subtree_matches(node->child[0].get(), node->bit_index, false))
      return false;
    if (not subtree_matches(node->child[1].get(), node->bit_index, true))
      return false;

    return check_invariants_rec(node->child[0].get(), node->bit_index, true,
                                counted) and
           check_invariants_rec(node->child[1].get(), node->bit_index, true,
                                counted);
  }
};

/** @brief Compressed bitwise map for unsigned integral keys.
 *
 *  `PatriciaMap` is the mapped-value counterpart of `PatriciaSet`. It stores
 *  unique fixed-width unsigned integer keys and one value per key. Internal
 *  nodes only record the first discriminating bit; leaves store the complete
 *  key and its associated value.
 *
 *  @tparam UInt Unsigned integral key type. `bool` is rejected because it has
 *          only two values and does not benefit from a trie representation.
 *  @tparam T Mapped value type. Must be move-constructible for insertion and
 *          copy-constructible only if the map itself is copied.
 */
template <typename UInt, typename T>
requires (std::is_integral_v<UInt> and std::is_unsigned_v<UInt> and
          not std::is_same_v<UInt, bool>)
class PatriciaMap
{
public:
  /// Key type stored by the map.
  using Key = UInt;

  /// Mapped value type stored at each leaf.
  using Value = T;

  /// Number of significant bits in `Key`.
  static constexpr size_t bit_width = std::numeric_limits<Key>::digits;

private:
  struct Node
  {
    bool leaf = true;
    Key key = 0;
    size_t bit_index = 0;
    std::optional<Value> value;
    std::array<std::unique_ptr<Node>, 2> child{};

    template <typename U>
    Node(const Key k, U && v) : leaf(true), key(k), value(std::forward<U>(v)) {}

    explicit Node(const size_t bit) noexcept : leaf(false), bit_index(bit) {}
  };

  std::unique_ptr<Node> root_;
  size_t size_ = 0;

  [[nodiscard]] static bool bit_at(const Key key, const size_t bit_index) noexcept
  {
    return ((key >> (bit_width - 1 - bit_index)) & Key{1}) != Key{0};
  }

  [[nodiscard]] static size_t first_differing_bit(const Key lhs,
                                                   const Key rhs) noexcept
  {
    const Key diff = lhs ^ rhs;
    for (size_t i = 0; i < bit_width; ++i)
      if (bit_at(diff, i))
        return i;
    return bit_width;
  }

  [[nodiscard]] static const Node * leaf_for(const Node * node,
                                             const Key key) noexcept
  {
    while (node != nullptr and not node->leaf)
      node = node->child[bit_at(key, node->bit_index) ? 1 : 0].get();
    return node;
  }

  [[nodiscard]] static Node * leaf_for(Node * node, const Key key) noexcept
  {
    return const_cast<Node *>(leaf_for(static_cast<const Node *>(node), key));
  }

  static void collect_keys(const Node * node, Array<Key> & out)
  {
    if (node == nullptr)
      return;
    if (node->leaf)
      {
        out.append(node->key);
        return;
      }
    collect_keys(node->child[0].get(), out);
    collect_keys(node->child[1].get(), out);
  }

  [[nodiscard]] static std::unique_ptr<Node> clone_node(const Node * src)
    requires std::is_copy_constructible_v<Value>
  {
    if (src == nullptr)
      return nullptr;
    auto dst = src->leaf ? std::make_unique<Node>(src->key, *src->value)
                         : std::make_unique<Node>(src->bit_index);
    if (not src->leaf)
      {
        dst->child[0] = clone_node(src->child[0].get());
        dst->child[1] = clone_node(src->child[1].get());
      }
    return dst;
  }

  template <typename U>
  bool insert_impl(const Key key, U && value)
  {
    if (root_ == nullptr)
      {
        root_ = std::make_unique<Node>(key, std::forward<U>(value));
        size_ = 1;
        return true;
      }

    const Node * existing = leaf_for(root_.get(), key);
    if (existing->key == key)
      return false;

    const size_t diff_bit = first_differing_bit(key, existing->key);
    auto * link = &root_;
    while ((*link)->leaf == false and (*link)->bit_index < diff_bit)
      link = &(*link)->child[bit_at(key, (*link)->bit_index) ? 1 : 0];

    auto branch = std::make_unique<Node>(diff_bit);
    const size_t side = bit_at(key, diff_bit) ? 1 : 0;
    branch->child[side] = std::make_unique<Node>(key, std::forward<U>(value));
    branch->child[1 - side] = std::move(*link);
    *link = std::move(branch);
    ++size_;
    return true;
  }

public:
  /** @brief Construct an empty map.
   *  @throws Nothing.
   */
  PatriciaMap() = default;

  /** @brief Destructor. Recursively releases every node.
   *  @throws Nothing.
   */
  ~PatriciaMap() = default;

  /** @brief Deep-copy constructor.
   *  @param[in] other Map to copy.
   *  @throws Whatever `T`'s copy constructor throws, or `std::bad_alloc`.
   */
  PatriciaMap(const PatriciaMap & other)
    requires std::is_copy_constructible_v<Value>
    : root_(clone_node(other.root_.get())), size_(other.size_)
  {}

  /** @brief Deep-copy assignment.
   *  @param[in] other Map to copy.
   *  @return `*this`.
   *  @throws Whatever `T`'s copy constructor throws, or `std::bad_alloc`.
   */
  PatriciaMap & operator=(const PatriciaMap & other)
    requires std::is_copy_constructible_v<Value>
  {
    if (this != &other)
      {
        root_ = clone_node(other.root_.get());
        size_ = other.size_;
      }
    return *this;
  }

  /** @brief Move constructor.
   *  @param[in,out] other Source map, left empty.
   *  @throws Nothing.
   */
  PatriciaMap(PatriciaMap && other) noexcept
    : root_(std::move(other.root_)), size_(other.size_)
  {
    other.size_ = 0;
  }

  /** @brief Move assignment.
   *  @param[in,out] other Source map, left empty.
   *  @return `*this`.
   *  @throws Nothing.
   */
  PatriciaMap & operator=(PatriciaMap && other) noexcept
  {
    if (this != &other)
      {
        root_ = std::move(other.root_);
        size_ = other.size_;
        other.size_ = 0;
      }
    return *this;
  }

  /** @brief Return the number of stored key-value pairs.
   *  @return Current cardinality.
   *  @throws Nothing.
   */
  [[nodiscard]] size_t size() const noexcept
  {
    return size_;
  }

  /** @brief Check whether the map has no key-value pairs.
   *  @return `true` iff `size() == 0`.
   *  @throws Nothing.
   */
  [[nodiscard]] bool is_empty() const noexcept
  {
    return size_ == 0;
  }

  /** @brief Remove every key-value pair from the map.
   *  @throws Nothing.
   */
  void clear() noexcept
  {
    root_.reset();
    size_ = 0;
  }

  /** @brief Insert `key` with a copy of `value`, only if `key` is absent.
   *
   *  @param[in] key Key to insert.
   *  @param[in] value Value to copy into the new leaf.
   *  @return `true` if inserted, `false` if `key` was already present.
   *  @throws Whatever `T`'s copy constructor throws, or `std::bad_alloc`.
   */
  bool insert(const Key key, const Value & value)
  {
    return insert_impl(key, value);
  }

  /** @brief Insert `key` with `value` moved in, only if `key` is absent.
   *
   *  @param[in] key Key to insert.
   *  @param[in,out] value Value to move into the new leaf.
   *  @return `true` if inserted, `false` if `key` was already present.
   *  @throws Whatever `T`'s move constructor throws, or `std::bad_alloc`.
   *
   *  @warning If `key` is already present, `value` is left untouched. The
   *           duplicate check happens before the value is forwarded.
   */
  bool insert(const Key key, Value && value)
  {
    return insert_impl(key, std::move(value));
  }

  /** @brief Insert `key` or overwrite the mapped value if `key` exists.
   *
   *  @param[in] key Key to insert or update.
   *  @param[in] value New value. It is moved into place.
   *  @throws Whatever `T`'s move/copy constructor or move assignment throws,
   *          or `std::bad_alloc`.
   */
  void insert_or_assign(const Key key, Value value)
  {
    Value * slot = find(key);
    if (slot != nullptr)
      *slot = std::move(value);
    else
      insert_impl(key, std::move(value));
  }

  /** @brief Remove `key` if present.
   *  @param[in] key Key to erase.
   *  @return `true` if erased, `false` if absent.
   *  @throws Nothing.
   */
  bool erase(const Key key) noexcept
  {
    if (root_ == nullptr)
      return false;

    if (root_->leaf)
      {
        if (root_->key != key)
          return false;
        root_.reset();
        size_ = 0;
        return true;
      }

    std::unique_ptr<Node> * parent_link = nullptr;
    std::unique_ptr<Node> * link = &root_;
    while ((*link)->leaf == false)
      {
        parent_link = link;
        link = &(*link)->child[bit_at(key, (*link)->bit_index) ? 1 : 0];
      }

    if ((*link)->key != key)
      return false;

    Node * parent = parent_link->get();
    const size_t side = bit_at(key, parent->bit_index) ? 1 : 0;
    *parent_link = std::move(parent->child[1 - side]);
    --size_;
    return true;
  }

  /** @brief Check whether `key` is stored.
   *  @param[in] key Key to query.
   *  @return `true` if `key` is present.
   *  @throws Nothing.
   */
  [[nodiscard]] bool contains(const Key key) const noexcept
  {
    return find(key) != nullptr;
  }

  /** @brief Look up `key`.
   *  @param[in] key Key to look up.
   *  @return Pointer to the stored value if `key` is present, `nullptr`
   *          otherwise. Valid until the next non-`const` operation on this map.
   *  @throws Nothing.
   */
  [[nodiscard]] const Value * find(const Key key) const noexcept
  {
    const Node * leaf = leaf_for(root_.get(), key);
    return (leaf != nullptr and leaf->key == key) ? &*leaf->value : nullptr;
  }

  /** @brief Mutable lookup overload.
   *  @param[in] key Key to look up.
   *  @return Mutable pointer to the stored value if `key` is present,
   *          `nullptr` otherwise. Valid until the next non-`const` operation
   *          on this map.
   *  @throws Nothing.
   */
  [[nodiscard]] Value * find(const Key key) noexcept
  {
    Node * leaf = leaf_for(root_.get(), key);
    return (leaf != nullptr and leaf->key == key) ? &*leaf->value : nullptr;
  }

  /** @brief Return all stored keys.
   *  @return Independent `Array` containing every key in unspecified order.
   *  @throws std::bad_alloc if the returned array cannot grow.
   */
  [[nodiscard]] Array<Key> keys() const
  {
    Array<Key> result;
    result.reserve(size_);
    collect_keys(root_.get(), result);
    return result;
  }

  /** @brief Verify structural invariants recursively.
   *
   *  Checks that internal nodes have two children and no value, internal bit
   *  indexes strictly increase down every path, each child subtree matches
   *  the routing bit used to reach it, every leaf has a value, and the number
   *  of leaves equals `size()`.
   *
   *  @return `true` iff all invariants hold.
   *  @throws Nothing.
   */
  [[nodiscard]] bool check_invariants() const noexcept
  {
    size_t counted = 0;
    const bool ok = check_invariants_rec(root_.get(), 0, false, counted);
    return ok and counted == size_;
  }

private:
  [[nodiscard]] static bool subtree_matches(const Node * node,
                                            const size_t bit_index,
                                            const bool expected) noexcept
  {
    if (node == nullptr)
      return false;
    if (node->leaf)
      return bit_at(node->key, bit_index) == expected;
    return subtree_matches(node->child[0].get(), bit_index, expected) and
           subtree_matches(node->child[1].get(), bit_index, expected);
  }

  [[nodiscard]] static bool check_invariants_rec(const Node * node,
                                                  const size_t parent_bit,
                                                  const bool has_parent,
                                                  size_t & counted) noexcept
  {
    if (node == nullptr)
      return true;

    if (node->leaf)
      {
        if (not node->value.has_value())
          return false;
        ++counted;
        return true;
      }

    if (node->value.has_value())
      return false;
    if (node->bit_index >= bit_width)
      return false;
    if (has_parent and node->bit_index <= parent_bit)
      return false;
    if (node->child[0] == nullptr or node->child[1] == nullptr)
      return false;
    if (not subtree_matches(node->child[0].get(), node->bit_index, false))
      return false;
    if (not subtree_matches(node->child[1].get(), node->bit_index, true))
      return false;

    return check_invariants_rec(node->child[0].get(), node->bit_index, true,
                                counted) and
           check_invariants_rec(node->child[1].get(), node->bit_index, true,
                                counted);
  }
};

} // namespace Aleph

#endif // TPL_PATRICIA_TRIE_H
