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

/** @file tpl_persistent_vector.H
 *  @brief Immutable bitmapped-vector trie with structural sharing.
 *
 *  `PersistentVector<T>` is an in-memory persistent sequence. `push_back`,
 *  `set` and `pop_back` return new vector versions and never mutate existing
 *  versions. The implementation is a bitmapped vector trie with branching
 *  factor 32, so updates copy O(log_32 n) trie nodes and share the rest.
 *
 *  This container is volatile in-memory persistence, not file-backed durable
 *  storage. Published versions may be read concurrently by different threads;
 *  publishing a new version into shared state still requires external
 *  synchronization.
 *
 *  @ingroup Sequences
 */

#ifndef TPL_PERSISTENT_VECTOR_H
#define TPL_PERSISTENT_VECTOR_H

#include <array>
#include <concepts>
#include <cstddef>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

#include <ah-errors.H>
#include <tpl_array.H>

namespace Aleph
{

/** @brief Immutable vector backed by a 32-way bitmapped trie.
 *
 *  @details
 *  ### What is a Persistent Vector?
 *  Unlike a standard `std::vector` which stores elements in a contiguous memory 
 *  block and requires an O(N) copy to duplicate, a `PersistentVector` is structured 
 *  as a shallow, wide tree (a Trie) where each node has up to 32 children.
 *  Because $32^6 > 10^9$, the tree depth never exceeds 6 levels for any realistic 
 *  number of elements. Thus, index lookups and updates require at most 6 pointer 
 *  dereferences, providing "effectively O(1)" performance.
 *
 *  "Persistence" in this context refers to **immutability with history** 
 *  (structural sharing or path-copying), not disk storage. When you call 
 *  `push_back` or `set`, the original vector is not mutated. Instead, the 
 *  container copies only the $O(\log_{32} N)$ nodes from the root down to the 
 *  modified leaf. The other 31 branches at each level remain untouched and are 
 *  shared between the old and new vectors via smart pointers (`std::shared_ptr`).
 *
 *  ### Benefits
 *  - **Lock-free Concurrency:** Because older versions are never overwritten, 
 *    multiple threads can read safely without locks.
 *  - **Move-only Types:** Values are held inside `std::shared_ptr<const T>`. 
 *    This allows the vector to store move-only types, and ensures that updating 
 *    one index does not force the copy constructor of unchanged heavy elements.
 *
 *  @tparam T Value type stored by the vector. Values are held through
 *          `std::shared_ptr<const T>`, so updating one index does not copy
 *          untouched values and move-only `T` is supported for `push_back`
 *          and `set`.
 */
template <typename T>
class PersistentVector
{
  static constexpr size_t branch_bits = 5;
  static constexpr size_t branch_factor = size_t{1} << branch_bits;
  static constexpr size_t branch_mask = branch_factor - 1;

  using ValuePtr = std::shared_ptr<const T>;

  struct Node
  {
    explicit Node(const bool leaf = true) noexcept : is_leaf(leaf) {}

    bool is_leaf = true;
    std::array<std::shared_ptr<const Node>, branch_factor> children{};
    std::array<ValuePtr, branch_factor> values{};
  };

  using NodePtr = std::shared_ptr<const Node>;

  NodePtr root_;
  size_t size_ = 0;
  size_t shift_ = 0;  // 0 means root is a leaf; otherwise high child shift.

  [[nodiscard]] static size_t child_index(const size_t index, const size_t shift) noexcept
  {
    return (index >> shift) & branch_mask;
  }

  [[nodiscard]] static size_t capacity_for_shift(const size_t shift) noexcept
  {
    constexpr size_t digits = std::numeric_limits<size_t>::digits;
    if (shift >= digits - branch_bits)
      return std::numeric_limits<size_t>::max();
    return size_t{1} << (shift + branch_bits);
  }

  [[nodiscard]] static NodePtr set_rec(const NodePtr &node,
                                       const size_t shift,
                                       const size_t index,
                                       ValuePtr value)
  {
    auto copy = node == nullptr ? std::make_shared<Node>(shift == 0)
                                : std::make_shared<Node>(*node);
    copy->is_leaf = shift == 0;

    if (shift == 0)
      {
        copy->values[child_index(index, 0)] = std::move(value);
        return copy;
      }

    const size_t slot = child_index(index, shift);
    copy->children[slot] = set_rec(copy->children[slot], shift - branch_bits,
                                   index, std::move(value));
    return copy;
  }

  [[nodiscard]] static bool leaf_has_values(const std::shared_ptr<Node> &node) noexcept
  {
    for (const auto &value : node->values)
      if (value != nullptr)
        return true;
    return false;
  }

  [[nodiscard]] static bool internal_has_children(const std::shared_ptr<Node> &node) noexcept
  {
    for (const auto &child : node->children)
      if (child != nullptr)
        return true;
    return false;
  }

  [[nodiscard]] static NodePtr clear_rec(const NodePtr &node,
                                         const size_t shift,
                                         const size_t index)
  {
    if (node == nullptr)
      return nullptr;

    auto copy = std::make_shared<Node>(*node);
    copy->is_leaf = shift == 0;

    if (shift == 0)
      {
        copy->values[child_index(index, 0)] = nullptr;
        return leaf_has_values(copy) ? NodePtr(copy) : nullptr;
      }

    const size_t slot = child_index(index, shift);
    copy->children[slot] = clear_rec(copy->children[slot], shift - branch_bits, index);
    return internal_has_children(copy) ? NodePtr(copy) : nullptr;
  }

  [[nodiscard]] static const T *get_ptr(const NodePtr &root,
                                        size_t shift,
                                        const size_t index) noexcept
  {
    const Node *curr = root.get();
    while (curr != nullptr and shift > 0)
      {
        curr = curr->children[child_index(index, shift)].get();
        shift -= branch_bits;
      }
    if (curr == nullptr)
      return nullptr;
    const ValuePtr &value = curr->values[child_index(index, 0)];
    return value == nullptr ? nullptr : value.get();
  }

  [[nodiscard]] static size_t count_values_rec(const NodePtr &node,
                                               const size_t shift,
                                               bool &ok) noexcept
  {
    if (node == nullptr)
      return 0;
    if (node->is_leaf != (shift == 0))
      {
        ok = false;
        return 0;
      }

    size_t count = 0;
    if (shift == 0)
      {
        for (const auto &child : node->children)
          if (child != nullptr)
            ok = false;
        for (const auto &value : node->values)
          if (value != nullptr)
            ++count;
        return count;
      }

    for (const auto &value : node->values)
      if (value != nullptr)
        ok = false;
    for (const auto &child : node->children)
      count += count_values_rec(child, shift - branch_bits, ok);
    return count;
  }

  explicit PersistentVector(NodePtr root, const size_t n, const size_t shift) noexcept
    : root_(std::move(root)), size_(n), shift_(shift)
  {
    // Empty.
  }

public:
  /** @brief Construct an empty persistent vector.
   *  @throws Nothing.
   */
  PersistentVector() noexcept = default;

  /** @brief Return the trie branching factor.
   *  @return Always 32.
   *  @throws Nothing.
   */
  [[nodiscard]] static constexpr size_t branching_factor() noexcept
  {
    return branch_factor;
  }

  /** @brief Return true when the vector has no values.
   *  @return `true` iff `size() == 0`.
   *  @throws Nothing.
   */
  [[nodiscard]] bool is_empty() const noexcept { return size_ == 0; }

  /** @brief Return the number of values in this version.
   *  @return Logical vector length.
   *  @throws Nothing.
   */
  [[nodiscard]] size_t size() const noexcept { return size_; }

  /** @brief Read a value by index.
   *  @param index Zero-based index.
   *  @return Constant reference to the stored value.
   *  @throws std::out_of_range if `index >= size()`.
   */
  [[nodiscard]] const T &get(const size_t index) const
  {
    ah_out_of_range_error_if(index >= size_)
      << "PersistentVector::get(): index out of range";
    const T *value = get_ptr(root_, shift_, index);
    ah_logic_error_if(value == nullptr)
      << "PersistentVector::get(): missing value inside vector trie";
    return *value;
  }

  /** @brief Read a value by index.
   *  @param index Zero-based index.
   *  @return Constant reference to the stored value.
   *  @throws std::out_of_range if `index >= size()`.
   */
  [[nodiscard]] const T &operator [] (const size_t index) const
  {
    return get(index);
  }

  /** @brief Return a new version with `value` appended by copy.
   *  @param value Value to append.
   *  @return New vector version with size increased by one.
   *  @throws std::length_error if the vector cannot grow further.
   *  @throws std::bad_alloc or whatever copying `T` throws.
   */
  [[nodiscard]] PersistentVector push_back(const T &value) const
  {
    return push_back(ValuePtr(std::make_shared<const T>(value)));
  }

  /** @brief Return a new version with `value` appended by move.
   *  @param value Value to move into the returned version.
   *  @return New vector version with size increased by one.
   *  @throws std::length_error if the vector cannot grow further.
   *  @throws std::bad_alloc or whatever moving `T` throws.
   */
  [[nodiscard]] PersistentVector push_back(T &&value) const
  {
    return push_back(ValuePtr(std::make_shared<const T>(std::move(value))));
  }

  /** @brief Return a new version with one index replaced by copy.
   *  @param index Index to replace.
   *  @param value New value.
   *  @return New vector version with the same size.
   *  @throws std::out_of_range if `index >= size()`.
   *  @throws std::bad_alloc or whatever copying `T` throws.
   */
  [[nodiscard]] PersistentVector set(const size_t index, const T &value) const
  {
    ah_out_of_range_error_if(index >= size_)
      << "PersistentVector::set(): index out of range";
    return set(index, ValuePtr(std::make_shared<const T>(value)));
  }

  /** @brief Return a new version with one index replaced by move.
   *  @param index Index to replace.
   *  @param value New value.
   *  @return New vector version with the same size.
   *  @throws std::out_of_range if `index >= size()`.
   *  @throws std::bad_alloc or whatever moving `T` throws.
   */
  [[nodiscard]] PersistentVector set(const size_t index, T &&value) const
  {
    ah_out_of_range_error_if(index >= size_)
      << "PersistentVector::set(): index out of range";
    return set(index, ValuePtr(std::make_shared<const T>(std::move(value))));
  }

  /** @brief Return a new version without the last value.
   *  @return New vector version with size decreased by one.
   *  @throws std::underflow_error if the vector is empty.
   *  @throws std::bad_alloc if a path-copy node allocation fails.
   */
  [[nodiscard]] PersistentVector pop_back() const
  {
    ah_underflow_error_if(is_empty())
      << "PersistentVector::pop_back(): empty vector";

    if (size_ == 1)
      return PersistentVector();

    const size_t new_size = size_ - 1;
    NodePtr root = clear_rec(root_, shift_, new_size);
    size_t shift = shift_;

    while (shift > 0 and new_size <= capacity_for_shift(shift - branch_bits))
      {
        root = root == nullptr ? nullptr : root->children[0];
        shift -= branch_bits;
      }

    return PersistentVector(std::move(root), new_size, shift);
  }

  /** @brief Return all values in an Aleph array.
   *  @return Array containing a copy of every value in index order.
   *  @throws std::bad_alloc or whatever copying `T` throws.
   *  @note `T` must also satisfy `Array<T>`'s movable-value requirements.
   */
  [[nodiscard]] Array<T> to_array() const
    requires (std::is_copy_constructible_v<T> and std::is_copy_assignable_v<T> and
              std::is_move_constructible_v<T> and std::is_move_assignable_v<T>)
  {
    Array<T> out;
    out.reserve(size_);
    for (size_t i = 0; i < size_; ++i)
      {
        const T &value = get(i);
        out.append(value);
      }
    return out;
  }

  /** @brief Verify trie shape and logical prefix invariants.
   *  @return `true` if the internal structure is consistent.
   *  @throws Nothing.
   *  @note Intended for tests and diagnostics; it is O(n log_32 n).
   */
  [[nodiscard]] bool verify() const noexcept
  {
    if (size_ == 0)
      return root_ == nullptr and shift_ == 0;
    if (root_ == nullptr)
      return false;
    if (size_ > capacity_for_shift(shift_))
      return false;

    bool ok = true;
    if (const size_t count = count_values_rec(root_, shift_, ok); not ok or count != size_)
      return false;
    for (size_t i = 0; i < size_; ++i)
      if (get_ptr(root_, shift_, i) == nullptr)
        return false;
    return true;
  }

private:
  [[nodiscard]] PersistentVector push_back(ValuePtr value) const
  {
    ah_length_error_if(size_ == std::numeric_limits<size_t>::max())
      << "PersistentVector::push_back(): size_t capacity exhausted";

    if (root_ == nullptr)
      return PersistentVector(set_rec(nullptr, 0, 0, std::move(value)), 1, 0);

    NodePtr root = root_;
    size_t shift = shift_;
    if (size_ == capacity_for_shift(shift_))
      {
        ah_length_error_if(shift_ >= std::numeric_limits<size_t>::digits - branch_bits)
          << "PersistentVector::push_back(): trie depth exhausted";
        auto grown = std::make_shared<Node>(false);
        grown->children[0] = root_;
        root = grown;
        shift += branch_bits;
      }

    root = set_rec(root, shift, size_, std::move(value));
    return PersistentVector(std::move(root), size_ + 1, shift);
  }

  [[nodiscard]] PersistentVector set(const size_t index, ValuePtr value) const
  {
    ah_out_of_range_error_if(index >= size_)
      << "PersistentVector::set(): index out of range";
    return PersistentVector(set_rec(root_, shift_, index, std::move(value)),
                            size_, shift_);
  }
};

} // namespace Aleph

#endif // TPL_PERSISTENT_VECTOR_H
