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

/** @file tpl_persistent_hash_map.H
 *  @brief Immutable path-copying hash map backed by a HAMT.
 *
 *  `PersistentHashMap` is an in-memory persistent collection: operations that
 *  look like updates return a new collection version and never mutate the old
 *  one. Unchanged trie nodes are shared through `std::shared_ptr<const Node>`.
 *
 *  This is unrelated to Aleph's file-backed persistence. Values are volatile
 *  in-memory objects and are not durable storage.
 *
 *  @par Complexity
 *  Lookup, insertion and erasure are expected O(1) for well-distributed hashes
 *  because the trie depth is bounded by the width of `size_t`. Keys with the
 *  same full hash are stored in a collision node and cost O(k) inside that
 *  collision group.
 *
 *  @par Thread safety
 *  Published versions are immutable. Different threads may read distinct
 *  collection objects that share the same trie without synchronization. The
 *  publication of a new version into a shared variable is still the user's
 *  responsibility and must be synchronized externally.
 *
 *  @ingroup Hash
 */

#ifndef TPL_PERSISTENT_HASH_MAP_H
#define TPL_PERSISTENT_HASH_MAP_H

#include <bit>
#include <cstdint>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

#include <ah-errors.H>
#include <ahFunction.H>
#include <hash-fct.H>
#include <tpl_array.H>

namespace Aleph
{

/** @brief Immutable unordered map backed by a Hash Array Mapped Trie (HAMT).
 *
 *  @details 
 *  ### What is a Persistent HAMT?
 *  A HAMT (Hash Array Mapped Trie) is a search trie where keys are not used 
 *  directly for the search path; instead, they are routed using fragments of 
 *  their hash value. At each level of the tree, 5 bits of the hash are consumed 
 *  (allowing 32 possible branches per level).
 *
 *  "Persistence" in this context means **immutability with history** 
 *  (structural sharing / path-copying), not disk storage. When an element is 
 *  inserted or erased, the original container does not mutate; instead, a 
 *  **new** map containing the modification is returned. Nodes that were not 
 *  affected by the operation are recycled and shared between the old and new 
 *  versions using smart pointers (`std::shared_ptr`).
 *  This makes copying the container an immediate O(1) operation and allows 
 *  multiple threads to read different versions without using locks.
 *
 *  ### How does compression work (BitmapNode)?
 *  To avoid wasting memory with arrays of 32 pointers filled with nulls, 
 *  a 32-bit bitmap is used. If an internal node has only 3 children, its 
 *  `bitmap` will have exactly 3 bits set, and it will store an array of 
 *  exactly 3 pointers. The real position of the pointer is calculated 
 *  super-efficiently using the hardware instruction `std::popcount`.
 * 
 *  ### Collisions
 *  If two distinct keys produce exactly the same 64-bit hash, they are 
 *  stored in a special collision node (`CollisionNode`), which acts as 
 *  a small linear array.
 *
 *  @tparam Key Key type. It must be copy-constructible and copy-assignable.
 *  @tparam T Mapped value type. It must be copy-constructible and copy-assignable
 *          so path-copying updates can rebuild shared collision nodes without
 *          moving from old versions.
 *  @tparam Cmp Equality predicate. If `cmp(a, b)` is true, the hash function
 *          supplied to the constructor must return the same hash for `a` and
 *          `b`.
 */
template <typename Key, typename T, class Cmp = Aleph::equal_to<Key>>
class PersistentHashMap
{
  static_assert(std::is_copy_constructible_v<Key>,
                "PersistentHashMap requires copy-constructible keys");
  static_assert(std::is_copy_assignable_v<Key>,
                "PersistentHashMap requires copy-assignable keys");
  static_assert(std::is_copy_constructible_v<T>,
                "PersistentHashMap requires copy-constructible mapped values");
  static_assert(std::is_copy_assignable_v<T>,
                "PersistentHashMap requires copy-assignable mapped values");
  static_assert(std::is_copy_constructible_v<Cmp>,
                "PersistentHashMap requires a copy-constructible comparator");

public:
  /// @brief Hash function pointer type used by this map.
  using Hash_Fct_Ptr = size_t (*)(const Key &);

private:
  static constexpr size_t BITS_PER_LEVEL = 5;
  static constexpr size_t BRANCHING_FACTOR = 1ULL << BITS_PER_LEVEL;
  static constexpr size_t LEVEL_MASK = BRANCHING_FACTOR - 1;

  /// @brief Types of polymorphic nodes in the HAMT.
  enum class NodeType
  {
    LEAF,      ///< Terminal leaf node that stores exactly one (key, value) pair.
    BITMAP,    ///< Compressed internal node that uses a 32-bit bitmap for routing.
    COLLISION  ///< Terminal node that stores multiple (key, value) pairs with the exact same hash.
  };

  /// @brief Polymorphic base node managed by std::shared_ptr for structural sharing.
  struct Node
  {
    NodeType type;

    explicit Node(const NodeType t) noexcept
      : type(t)
    {
      // Empty.
    }

    virtual ~Node() = default;
  };

  using NodePtr = std::shared_ptr<const Node>;

  struct LeafNode : public Node
  {
    Key key;
    T value;
    size_t hash;

    LeafNode(Key k, T v, const size_t h)
      : Node(NodeType::LEAF), key(std::move(k)), value(std::move(v)), hash(h)
    {
      // Empty.
    }
  };

  struct BitmapNode : public Node
  {
    std::uint32_t bitmap = 0;
    Array<NodePtr> children;

    BitmapNode(const std::uint32_t b, Array<NodePtr> c)
      : Node(NodeType::BITMAP), bitmap(b), children(std::move(c))
    {
      // Empty.
    }
  };

  struct CollisionNode : public Node
  {
    size_t hash = 0;
    Array<std::pair<Key, T>> entries;

    CollisionNode(const size_t h, Array<std::pair<Key, T>> e)
      : Node(NodeType::COLLISION), hash(h), entries(std::move(e))
    {
      // Empty.
    }
  };

  NodePtr root_;
  Hash_Fct_Ptr hash_fct_;
  Cmp cmp_;
  size_t size_ = 0;

  explicit PersistentHashMap(NodePtr root,
                             const Hash_Fct_Ptr hash_fct,
                             Cmp cmp,
                             const size_t size)
    : root_(std::move(root)), hash_fct_(hash_fct), cmp_(std::move(cmp)), size_(size)
  {
    // Empty.
  }

  [[nodiscard]] static size_t bitpos(const size_t hash, const size_t shift) noexcept
  {
    return (hash >> shift) & LEVEL_MASK;
  }

  [[nodiscard]] static std::uint32_t bit(const size_t pos) noexcept
  {
    return std::uint32_t{1} << pos;
  }

  [[nodiscard]] static size_t index(const std::uint32_t bitmap,
                                    const std::uint32_t bit_value) noexcept
  {
    return std::popcount(bitmap & (bit_value - 1));
  }

  [[nodiscard]] static NodePtr make_leaf(const Key &key, const T &value, const size_t hash)
  {
    return std::make_shared<LeafNode>(key, value, hash);
  }

  [[nodiscard]] static NodePtr make_leaf(const Key &key, T &&value, const size_t hash)
  {
    return std::make_shared<LeafNode>(key, std::move(value), hash);
  }

  [[nodiscard]] static NodePtr make_leaf_from_value(const Key &key,
                                                    const T *value,
                                                    T *movable_value,
                                                    const size_t hash)
  {
    return movable_value != nullptr
      ? make_leaf(key, std::move(*movable_value), hash)
      : make_leaf(key, *value, hash);
  }

  static void append_entry(Array<std::pair<Key, T>> &entries,
                           const Key &key,
                           const T *value,
                           T *movable_value)
  {
    if (movable_value != nullptr)
      entries.append(std::pair<Key, T>{key, std::move(*movable_value)});
    else
      entries.append(std::pair<Key, T>{key, *value});
  }

  [[nodiscard]] NodePtr merge_leaves(const NodePtr &n1,
                                     const NodePtr &n2,
                                     const size_t shift) const
  {
    const size_t h1 = n1->type == NodeType::LEAF
      ? static_cast<const LeafNode *>(n1.get())->hash
      : static_cast<const CollisionNode *>(n1.get())->hash;
    const size_t h2 = n2->type == NodeType::LEAF
      ? static_cast<const LeafNode *>(n2.get())->hash
      : static_cast<const CollisionNode *>(n2.get())->hash;

    const size_t p1 = bitpos(h1, shift);
    const size_t p2 = bitpos(h2, shift);

    if (p1 != p2)
      {
        Array<NodePtr> children;
        children.reserve(2);
        if (p1 < p2)
          {
            children.append(n1);
            children.append(n2);
          }
        else
          {
            children.append(n2);
            children.append(n1);
          }
        return std::make_shared<BitmapNode>(bit(p1) | bit(p2), std::move(children));
      }

    Array<NodePtr> children;
    children.append(merge_leaves(n1, n2, shift + BITS_PER_LEVEL));
    return std::make_shared<BitmapNode>(bit(p1), std::move(children));
  }

  [[nodiscard]] NodePtr insert_impl(const NodePtr &node,
                                    const Key &key,
                                    const T *value,
                                    T *movable_value,
                                    const size_t hash,
                                    const size_t shift,
                                    const bool replace,
                                    bool &added,
                                    bool &changed) const
  {
    if (node == nullptr)
      {
        added = true;
        changed = true;
        return make_leaf_from_value(key, value, movable_value, hash);
      }

    if (node->type == NodeType::LEAF)
      {
        const auto leaf = static_cast<const LeafNode *>(node.get());
        if (cmp_(key, leaf->key))
          {
            added = false;
            if (not replace)
              {
                changed = false;
                return node;
              }

            changed = true;
            return make_leaf_from_value(leaf->key, value, movable_value, leaf->hash);
          }

        if (leaf->hash == hash)
          {
            Array<std::pair<Key, T>> entries;
            entries.reserve(2);
            entries.append(std::pair<Key, T>{leaf->key, leaf->value});
            append_entry(entries, key, value, movable_value);
            added = true;
            changed = true;
            return std::make_shared<CollisionNode>(hash, std::move(entries));
          }

        NodePtr new_leaf = make_leaf_from_value(key, value, movable_value, hash);
        added = true;
        changed = true;
        return merge_leaves(node, new_leaf, shift);
      }

    if (node->type == NodeType::BITMAP)
      {
        const auto bnode = static_cast<const BitmapNode *>(node.get());
        const size_t pos = bitpos(hash, shift);
        const std::uint32_t b = bit(pos);
        const size_t idx = index(bnode->bitmap, b);

        if ((bnode->bitmap & b) == 0)
          {
            Array<NodePtr> new_children;
            new_children.reserve(bnode->children.size() + 1);
            for (size_t i = 0; i < idx; ++i)
              new_children.append(bnode->children[i]);
            new_children.append(make_leaf_from_value(key, value, movable_value, hash));
            for (size_t i = idx; i < bnode->children.size(); ++i)
              new_children.append(bnode->children[i]);

            added = true;
            changed = true;
            return std::make_shared<BitmapNode>(bnode->bitmap | b, std::move(new_children));
          }

        const NodePtr child = bnode->children[idx];
        NodePtr new_child = insert_impl(child, key, value, movable_value, hash,
                                        shift + BITS_PER_LEVEL, replace, added, changed);
        if (not changed)
          return node;

        Array<NodePtr> new_children = bnode->children;
        new_children[idx] = std::move(new_child);
        return std::make_shared<BitmapNode>(bnode->bitmap, std::move(new_children));
      }

    if (node->type == NodeType::COLLISION)
      {
        const auto cnode = static_cast<const CollisionNode *>(node.get());
        if (hash != cnode->hash)
          {
            NodePtr new_leaf = make_leaf_from_value(key, value, movable_value, hash);
            added = true;
            changed = true;
            return merge_leaves(node, new_leaf, shift);
          }

        if (not replace)
          for (const auto &entry : cnode->entries)
            if (cmp_(key, entry.first))
              {
                added = false;
                changed = false;
                return node;
              }

        Array<std::pair<Key, T>> new_entries;
        new_entries.reserve(cnode->entries.size() + 1);
        bool found = false;
        for (const auto &entry : cnode->entries)
          if (cmp_(key, entry.first) and not found)
            {
              append_entry(new_entries, entry.first, value, movable_value);
              found = true;
            }
          else
            new_entries.append(entry);

        if (not found)
          {
            append_entry(new_entries, key, value, movable_value);
            added = true;
          }
        else
          added = false;

        changed = true;
        return std::make_shared<CollisionNode>(hash, std::move(new_entries));
      }

    changed = false;
    return node;
  }

  [[nodiscard]] NodePtr erase_impl(const NodePtr &node,
                                   const Key &key,
                                   const size_t hash,
                                   const size_t shift,
                                   bool &removed) const
  {
    if (node == nullptr)
      return nullptr;

    if (node->type == NodeType::LEAF)
      {
        const auto leaf = static_cast<const LeafNode *>(node.get());
        if (cmp_(key, leaf->key))
          {
            removed = true;
            return nullptr;
          }
        return node;
      }

    if (node->type == NodeType::BITMAP)
      {
        const auto bnode = static_cast<const BitmapNode *>(node.get());
        const size_t pos = bitpos(hash, shift);
        const std::uint32_t b = bit(pos);
        if ((bnode->bitmap & b) == 0)
          return node;

        const size_t idx = index(bnode->bitmap, b);
        const NodePtr child = bnode->children[idx];
        NodePtr new_child = erase_impl(child, key, hash, shift + BITS_PER_LEVEL, removed);
        if (new_child == child)
          return node;

        if (new_child == nullptr)
          {
            if (bnode->bitmap == b)
              return nullptr;

            if (std::popcount(bnode->bitmap) == 2)
              {
                const size_t other_idx = idx == 0 ? 1 : 0;
                NodePtr other_child = bnode->children[other_idx];
                if (other_child->type == NodeType::LEAF or
                    other_child->type == NodeType::COLLISION)
                  return other_child;
              }

            Array<NodePtr> new_children;
            new_children.reserve(bnode->children.size() - 1);
            for (size_t i = 0; i < bnode->children.size(); ++i)
              if (i != idx)
                new_children.append(bnode->children[i]);
            return std::make_shared<BitmapNode>(bnode->bitmap & ~b, std::move(new_children));
          }

        Array<NodePtr> new_children = bnode->children;
        new_children[idx] = std::move(new_child);
        if (new_children.size() == 1 and
            (new_children[0]->type == NodeType::LEAF or
             new_children[0]->type == NodeType::COLLISION))
          return new_children[0];
        return std::make_shared<BitmapNode>(bnode->bitmap, std::move(new_children));
      }

    if (node->type == NodeType::COLLISION)
      {
        const auto cnode = static_cast<const CollisionNode *>(node.get());
        if (hash != cnode->hash)
          return node;

        Array<std::pair<Key, T>> new_entries;
        new_entries.reserve(cnode->entries.size());
        for (const auto &entry : cnode->entries)
          if (cmp_(key, entry.first))
            removed = true;
          else
            new_entries.append(entry);

        if (not removed)
          return node;
        if (new_entries.size() == 1)
          return std::make_shared<LeafNode>(new_entries[0].first,
                                            new_entries[0].second,
                                            hash);
        return std::make_shared<CollisionNode>(hash, std::move(new_entries));
      }

    return node;
  }

  [[nodiscard]] const T *find_impl(const NodePtr &node,
                                   const Key &key,
                                   const size_t hash,
                                   const size_t shift) const
  {
    if (node == nullptr)
      return nullptr;

    if (node->type == NodeType::LEAF)
      {
        const auto leaf = static_cast<const LeafNode *>(node.get());
        return cmp_(key, leaf->key) ? &leaf->value : nullptr;
      }

    if (node->type == NodeType::BITMAP)
      {
        const auto bnode = static_cast<const BitmapNode *>(node.get());
        const size_t pos = bitpos(hash, shift);
        const std::uint32_t b = bit(pos);
        if ((bnode->bitmap & b) == 0)
          return nullptr;
        return find_impl(bnode->children[index(bnode->bitmap, b)],
                         key, hash, shift + BITS_PER_LEVEL);
      }

    if (node->type == NodeType::COLLISION)
      {
        const auto cnode = static_cast<const CollisionNode *>(node.get());
        if (hash != cnode->hash)
          return nullptr;
        for (const auto &entry : cnode->entries)
          if (cmp_(key, entry.first))
            return &entry.second;
      }

    return nullptr;
  }

  [[nodiscard]] bool hashes_match_slot(const NodePtr &node,
                                       const size_t shift,
                                       const size_t pos) const
  {
    if (node == nullptr)
      return false;
    if (node->type == NodeType::LEAF)
      return bitpos(static_cast<const LeafNode *>(node.get())->hash, shift) == pos;
    if (node->type == NodeType::COLLISION)
      return bitpos(static_cast<const CollisionNode *>(node.get())->hash, shift) == pos;

    const auto bnode = static_cast<const BitmapNode *>(node.get());
    for (size_t i = 0; i < bnode->children.size(); ++i)
      if (not hashes_match_slot(bnode->children[i], shift, pos))
        return false;
    return true;
  }

  [[nodiscard]] bool collision_keys_are_unique(const CollisionNode &node) const
  {
    for (size_t i = 0; i < node.entries.size(); ++i)
      for (size_t j = i + 1; j < node.entries.size(); ++j)
        if (cmp_(node.entries[i].first, node.entries[j].first))
          return false;
    return true;
  }

  [[nodiscard]] bool verify_rec(const NodePtr &node, const size_t shift, size_t &count) const
  {
    count = 0;
    if (node == nullptr)
      return true;

    if (node->type == NodeType::LEAF)
      {
        const auto leaf = static_cast<const LeafNode *>(node.get());
        if (hash_fct_(leaf->key) != leaf->hash)
          return false;
        count = 1;
        return true;
      }

    if (node->type == NodeType::COLLISION)
      {
        const auto cnode = static_cast<const CollisionNode *>(node.get());
        if (cnode->entries.size() < 2 or not collision_keys_are_unique(*cnode))
          return false;
        for (const auto &entry : cnode->entries)
          if (hash_fct_(entry.first) != cnode->hash)
            return false;
        count = cnode->entries.size();
        return true;
      }

    if (node->type == NodeType::BITMAP)
      {
        const auto bnode = static_cast<const BitmapNode *>(node.get());
        const size_t child_count = std::popcount(bnode->bitmap);
        if (child_count == 0 or child_count != bnode->children.size())
          return false;
        if (bnode->children.size() == 1 and
            (bnode->children[0]->type == NodeType::LEAF or
             bnode->children[0]->type == NodeType::COLLISION))
          return false;

        size_t idx = 0;
        size_t total = 0;
        for (size_t pos = 0; pos < BRANCHING_FACTOR; ++pos)
          {
            const std::uint32_t b = bit(pos);
            if ((bnode->bitmap & b) == 0)
              continue;

            if (idx >= bnode->children.size())
              return false;
            const NodePtr &child = bnode->children[idx++];
            if (child == nullptr or not hashes_match_slot(child, shift, pos))
              return false;

            size_t subtree_count = 0;
            if (not verify_rec(child, shift + BITS_PER_LEVEL, subtree_count))
              return false;
            ah_overflow_error_if(total > std::numeric_limits<size_t>::max() - subtree_count)
              << "PersistentHashMap::verify(): node count would overflow";
            total += subtree_count;
          }

        if (idx != bnode->children.size())
          return false;
        count = total;
        return true;
      }

    return false;
  }

  static void collect_keys(const NodePtr &node, Array<Key> &out)
  {
    if (node == nullptr)
      return;
    if (node->type == NodeType::LEAF)
      {
        out.append(static_cast<const LeafNode *>(node.get())->key);
        return;
      }
    if (node->type == NodeType::COLLISION)
      {
        const auto cnode = static_cast<const CollisionNode *>(node.get());
        for (const auto &entry : cnode->entries)
          out.append(entry.first);
        return;
      }

    const auto bnode = static_cast<const BitmapNode *>(node.get());
    for (size_t i = 0; i < bnode->children.size(); ++i)
      collect_keys(bnode->children[i], out);
  }

  static void collect_items(const NodePtr &node, Array<std::pair<Key, T>> &out)
  {
    if (node == nullptr)
      return;
    if (node->type == NodeType::LEAF)
      {
        const auto leaf = static_cast<const LeafNode *>(node.get());
        out.append(std::pair<Key, T>{leaf->key, leaf->value});
        return;
      }
    if (node->type == NodeType::COLLISION)
      {
        const auto cnode = static_cast<const CollisionNode *>(node.get());
        for (const auto &entry : cnode->entries)
          out.append(entry);
        return;
      }

    const auto bnode = static_cast<const BitmapNode *>(node.get());
    for (size_t i = 0; i < bnode->children.size(); ++i)
      collect_items(bnode->children[i], out);
  }

  [[nodiscard]] size_t size_after_insert(const bool added) const
  {
    if (not added)
      return size_;
    ah_overflow_error_if(size_ == std::numeric_limits<size_t>::max())
      << "PersistentHashMap: size would overflow";
    return size_ + 1;
  }

public:
  /** @brief Construct an empty persistent hash map.
   *  @param hash_fct Hash function used to route keys through the HAMT.
   *  @param cmp Equality predicate used to compare keys.
   *  @throws std::domain_error if `hash_fct` is null.
   *  @throws Whatever copying `cmp` throws.
   *  @pre If `cmp(a, b)` is true, `hash_fct(a) == hash_fct(b)` must also be true.
   */
  explicit PersistentHashMap(const Hash_Fct_Ptr hash_fct = dft_hash_ptr_fct<Key>,
                             const Cmp &cmp = Cmp())
    : hash_fct_(hash_fct), cmp_(cmp)
  {
    ah_domain_error_unless(hash_fct_ != nullptr)
      << "PersistentHashMap: hash function must not be null";
  }

  /** @brief Return the number of bindings stored in this version.
   *  @return Number of bindings.
   *  @throws Nothing.
   */
  [[nodiscard]] size_t size() const noexcept { return size_; }

  /** @brief Return true when this version has no bindings.
   *  @return `true` iff `size() == 0`.
   *  @throws Nothing.
   */
  [[nodiscard]] bool is_empty() const noexcept { return size_ == 0; }

  /** @brief Return a new version with `key` inserted if absent.
   *  @param key Key to insert.
   *  @param value Mapped value to copy into the new version when `key` is absent.
   *  @return New map version. If an equivalent key already exists, returns an
   *          unchanged version sharing the same root.
   *  @throws std::bad_alloc or whatever copying/comparison/hashing throws.
   */
  [[nodiscard]] PersistentHashMap insert(const Key &key, const T &value) const
  {
    bool added = false;
    bool changed = false;
    NodePtr new_root = insert_impl(root_, key, &value, nullptr, hash_fct_(key), 0,
                                   false, added, changed);
    if (not changed)
      return *this;
    return PersistentHashMap(std::move(new_root), hash_fct_, cmp_, size_after_insert(added));
  }

  /** @brief Return a new version with `key` inserted if absent, moving `value`.
   *  @param key Key to insert.
   *  @param value Mapped value to move into the new version when `key` is absent.
   *  @return New map version. If an equivalent key already exists, returns an
   *          unchanged version and does not move from `value`.
   *  @throws std::bad_alloc or whatever copying/moving/comparison/hashing throws.
   */
  [[nodiscard]] PersistentHashMap insert(const Key &key, T &&value) const
  {
    bool added = false;
    bool changed = false;
    NodePtr new_root = insert_impl(root_, key, nullptr, &value, hash_fct_(key), 0,
                                   false, added, changed);
    if (not changed)
      return *this;
    return PersistentHashMap(std::move(new_root), hash_fct_, cmp_, size_after_insert(added));
  }

  /** @brief Return a new version with `key` bound to `value`.
   *  @param key Key to insert or update.
   *  @param value Mapped value to copy into the returned version.
   *  @return New map version with an equivalent key present.
   *  @throws std::bad_alloc or whatever copying/comparison/hashing throws.
   */
  [[nodiscard]] PersistentHashMap insert_or_assign(const Key &key, const T &value) const
  {
    bool added = false;
    bool changed = false;
    NodePtr new_root = insert_impl(root_, key, &value, nullptr, hash_fct_(key), 0,
                                   true, added, changed);
    return PersistentHashMap(std::move(new_root), hash_fct_, cmp_, size_after_insert(added));
  }

  /** @brief Return a new version with `key` bound to moved `value`.
   *  @param key Key to insert or update.
   *  @param value Mapped value to move into the returned version.
   *  @return New map version with an equivalent key present.
   *  @throws std::bad_alloc or whatever copying/moving/comparison/hashing throws.
   */
  [[nodiscard]] PersistentHashMap insert_or_assign(const Key &key, T &&value) const
  {
    bool added = false;
    bool changed = false;
    NodePtr new_root = insert_impl(root_, key, nullptr, &value, hash_fct_(key), 0,
                                   true, added, changed);
    return PersistentHashMap(std::move(new_root), hash_fct_, cmp_, size_after_insert(added));
  }

  /** @brief Return a new version without `key`.
   *  @param key Key to erase.
   *  @return New map version. If the key is absent, returns an unchanged
   *          version sharing the same root.
   *  @throws std::bad_alloc or whatever comparison/hashing throws.
   */
  [[nodiscard]] PersistentHashMap erase(const Key &key) const
  {
    bool removed = false;
    NodePtr new_root = erase_impl(root_, key, hash_fct_(key), 0, removed);
    if (not removed)
      return *this;
    return PersistentHashMap(std::move(new_root), hash_fct_, cmp_, size_ - 1);
  }

  /** @brief Test whether `key` is present.
   *  @param key Key to search.
   *  @return `true` if an equivalent key is stored.
   *  @throws Whatever comparison or hashing throws.
   */
  [[nodiscard]] bool contains(const Key &key) const
  {
    return find(key) != nullptr;
  }

  /** @brief Find a mapped value.
   *  @param key Key to search.
   *  @return Pointer to the stored mapped value, or `nullptr` when absent. The
   *          pointer remains valid while this map version is alive.
   *  @throws Whatever comparison or hashing throws.
   */
  [[nodiscard]] const T *find(const Key &key) const
  {
    return find_impl(root_, key, hash_fct_(key), 0);
  }

  /** @brief Return all keys in unspecified order.
   *  @return Aleph array with a copy of every key.
   *  @throws std::bad_alloc or whatever copying `Key` throws.
   */
  [[nodiscard]] Array<Key> keys() const
  {
    Array<Key> out;
    out.reserve(size());
    collect_keys(root_, out);
    return out;
  }

  /** @brief Return all key/value bindings in unspecified order.
   *  @return Aleph array with a copy of every binding.
   *  @throws std::bad_alloc or whatever copying `Key` or `T` throws.
   */
  [[nodiscard]] Array<std::pair<Key, T>> items() const
  {
    Array<std::pair<Key, T>> out;
    out.reserve(size());
    collect_items(root_, out);
    return out;
  }

  /** @brief Verify HAMT routing, collision, uniqueness and size invariants.
   *  @return `true` if the internal structure is consistent.
   *  @throws std::overflow_error if a diagnostic node count overflows.
   *  @throws Whatever `hash_fct_` or `Cmp` throws.
   *  @note Intended for tests and diagnostics; it is O(n) plus O(k^2) inside
   *        each full-hash collision node.
   */
  [[nodiscard]] bool verify() const
  {
    size_t count = 0;
    return verify_rec(root_, 0, count) and count == size_;
  }
};

} // namespace Aleph

#endif // TPL_PERSISTENT_HASH_MAP_H
