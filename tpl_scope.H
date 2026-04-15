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


/** @file tpl_scope.H
 *  @brief Generic nested-scope stack for compiler-oriented name lookup.
 *
 *  `Scope<Key, Value>` stores bindings in nested lexical frames and supports
 *  local lookup, recursive lookup, shadowing, and scope push/pop operations.
 *
 *  The implementation is intentionally simple and deterministic: lookups are
 *  linear in the number of bindings per active frame, which is adequate for the
 *  current compiler-support MVP and keeps the component lightweight.
 *
 *  @ingroup Utilities
 */

# ifndef TPL_SCOPE_H
# define TPL_SCOPE_H

# include <concepts>
# include <cstddef>
# include <utility>

# include <ah-errors.H>
# include <tpl_dynArray.H>

namespace Aleph
{
  /** @brief Stack of lexical scopes for bindings from `Key` to `Value`.
   *
   *  Maintains a stack of frames, where each frame holds key-value bindings.
   *  Lookups search from the innermost (current) frame outward.
   *
   *  @tparam Key Key type used for lookup. Must be equality-comparable.
   *  @tparam Value Stored binding payload.
   *
   *  @note Thread-safety: Not thread-safe for concurrent mutating operations.
   *        Const operations (lookup, depth, empty) are safe for concurrent 
   *        readers only if external synchronization is provided.
   *  @note Complexity: O(1) for construction.
   */
  template <typename Key, typename Value>
    requires std::equality_comparable<Key>
  class Scope
  {
  public:
    /** @brief Binding entry stored inside one lexical frame.
     *
     *  @note Thread-safety: Not thread-safe.
     *  @note Complexity: O(1) storage and retrieval per binding.
     */
    struct Binding
    {
      Key key; ///< Lookup key.
      Value value; ///< Stored payload.
    };

  private:
    struct Frame
    {
      DynArray<Binding> bindings;
    };

    DynArray<Frame> frames;

    static Value *
    find_value(DynArray<Binding> & bindings, const Key & key) noexcept
    {
      for (size_t i = bindings.size(); i > 0; --i)
        if (bindings.access(i - 1).key == key)
          return &bindings.access(i - 1).value;
      return nullptr;
    }

    static const Value *
    find_value(const DynArray<Binding> & bindings, const Key & key) noexcept
    {
      for (size_t i = bindings.size(); i > 0; --i)
        if (bindings.access(i - 1).key == key)
          return &bindings.access(i - 1).value;
      return nullptr;
    }

    Frame &
    current_frame()
    {
      ah_runtime_error_unless(frames.size() > 0)
        << "Scope::current_frame(): no active scope";
      return frames.top();
    }

    const Frame &
    current_frame() const
    {
      ah_runtime_error_unless(frames.size() > 0)
        << "Scope::current_frame(): no active scope";
      return frames.top();
    }

  public:
    /** @brief Default constructor.
     *
     *  @note Thread-safety: Not thread-safe.
     *  @note Complexity: O(1).
     */
    Scope() = default;

    /** @brief Returns the number of active lexical frames.
     *
     *  @return Current stack depth (number of active scopes).
     *  @note Thread-safety: Safe for concurrent readers.
     *  @note Complexity: O(1).
     */
    [[nodiscard]] size_t depth() const noexcept
    {
      return frames.size();
    }

    /** @brief Returns whether there are no active scopes.
     *
     *  @return `true` iff `depth() == 0`.
     *  @note Thread-safety: Safe for concurrent readers.
     *  @note Complexity: O(1).
     */
    [[nodiscard]] bool empty() const noexcept
    {
      return frames.is_empty();
    }

    /** @brief Removes all scopes and bindings.
     *
     *  After this call, `depth() == 0`.
     *  @note Thread-safety: Not thread-safe.
     *  @note Complexity: O(total_bindings) where total_bindings is the sum of 
     *        all bindings across all frames.
     */
    void clear() noexcept
    {
      frames.clear();
    }

    /** @brief Pushes a new empty lexical frame.
     *
     *  All subsequent `insert()` calls add bindings to this new frame.
     *  @return New active depth after the push.
     *  @note Thread-safety: Not thread-safe.
     *  @note Complexity: O(1).
     */
    size_t enter_scope()
    {
      // Reinitialize reused slots explicitly because DynArray::pop() moves the
      // last frame out before shrinking, and a later append may reuse that
      // moved-from storage.
      frames.append(Frame());
      return frames.size();
    }

    /** @brief Pops the current lexical frame.
     *
     *  All bindings in the current frame are destroyed.
     *  @throws std::runtime_error If `depth() == 0`.
     *  @note Thread-safety: Not thread-safe.
     *  @note Complexity: O(1).
     */
    void leave_scope()
    {
      ah_runtime_error_unless(frames.size() > 0)
        << "Scope::leave_scope(): no active scope";
      (void) frames.pop();
    }

    /** @brief Inserts a binding in the current scope.
     *
     *  @param key Lookup key.
     *  @param value Payload to store.
     *  @return `false` iff the key already exists in the current scope.
     *  @throws std::runtime_error If `depth() == 0`.
     *  @note Thread-safety: Not thread-safe.
     *  @note Complexity: O(k) where k is the number of bindings currently in 
     *        the active frame (linear search for duplicate detection).
     */
    bool insert(const Key & key, const Value & value)
    {
      auto & frame = current_frame();
      if (find_value(frame.bindings, key) != nullptr)
        return false;
      frame.bindings.append({key, value});
      return true;
    }

    /** @brief Inserts a moved binding in the current scope.
     *
     *  @param key Lookup key (moved).
     *  @param value Payload to store (moved).
     *  @return `false` iff the key already exists in the current scope.
     *  @throws std::runtime_error If `depth() == 0`.
     *  @note Thread-safety: Not thread-safe.
     *  @note Complexity: O(k) where k is the number of bindings currently in 
     *        the active frame (linear search for duplicate detection).
     */
    bool insert(Key && key, Value && value)
    {
      auto & frame = current_frame();
      if (find_value(frame.bindings, key) != nullptr)
        return false;
      frame.bindings.append({std::move(key), std::move(value)});
      return true;
    }

    /** @brief Looks up `key` only in the current lexical frame.
     *
     *  @param key Lookup key.
     *  @return Pointer to the stored value, or `nullptr` if absent or no frame
     *          is active.
     *  @note Thread-safety: Not thread-safe for concurrent mutation.
     *  @note Complexity: O(k) where k is the number of bindings in the 
     *        current frame (linear search, innermost binding first).
     */
    Value * lookup_local(const Key & key) noexcept
    {
      if (frames.is_empty())
        return nullptr;
      return find_value(frames.top().bindings, key);
    }

    /** @brief Looks up `key` only in the current lexical frame (const).
     *
     *  @param key Lookup key.
     *  @return Pointer to the stored value, or `nullptr` if absent or no frame
     *          is active.
     *  @note Thread-safety: Safe for concurrent readers.
     *  @note Complexity: O(k) where k is the number of bindings in the 
     *        current frame (linear search, innermost binding first).
     */
    const Value * lookup_local(const Key & key) const noexcept
    {
      if (frames.is_empty())
        return nullptr;
      return find_value(frames.top().bindings, key);
    }

    /** @brief Looks up `key` from the innermost scope outward.
     *
     *  Searches the current frame, then outer frames in order, returning the
     *  first (innermost) match found. Implements lexical shadowing.
     *
     *  @param key Lookup key.
     *  @return Pointer to the nearest visible value, or `nullptr` if absent.
     *  @note Thread-safety: Not thread-safe for concurrent mutation.
     *  @note Complexity: O(m*k) where m is the number of frames and k is the 
     *        average number of bindings per frame (worst-case linear search 
     *        through all bindings across all frames).
     */
    Value * lookup(const Key & key) noexcept
    {
      for (size_t i = frames.size(); i > 0; --i)
        if (auto * value = find_value(frames.access(i - 1).bindings, key);
            value != nullptr)
          return value;
      return nullptr;
    }

    /** @brief Looks up `key` from the innermost scope outward (const).
     *
     *  Searches the current frame, then outer frames in order, returning the
     *  first (innermost) match found. Implements lexical shadowing.
     *
     *  @param key Lookup key.
     *  @return Pointer to the nearest visible value, or `nullptr` if absent.
     *  @note Thread-safety: Safe for concurrent readers.
     *  @note Complexity: O(m*k) where m is the number of frames and k is the 
     *        average number of bindings per frame (worst-case linear search 
     *        through all bindings across all frames).
     */
    const Value * lookup(const Key & key) const noexcept
    {
      for (size_t i = frames.size(); i > 0; --i)
        if (const auto * value = find_value(frames.access(i - 1).bindings, key);
            value != nullptr)
          return value;
      return nullptr;
    }
  };
}

# endif // TPL_SCOPE_H
