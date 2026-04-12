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
# include <utility>

# include <ah-errors.H>
# include <tpl_dynArray.H>

namespace Aleph
{
  /** @brief Stack of lexical scopes for bindings from `Key` to `Value`.
   *
   *  @tparam Key Key type used for lookup. Must be equality-comparable.
   *  @tparam Value Stored binding payload.
   */
  template <typename Key, typename Value>
    requires std::equality_comparable<Key>
  class Scope
  {
  public:
    /** @brief Binding entry stored inside one lexical frame. */
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
    /** @brief Returns the number of active lexical frames. */
    size_t depth() const noexcept
    {
      return frames.size();
    }

    /** @brief Returns whether there are no active scopes. */
    bool empty() const noexcept
    {
      return frames.is_empty();
    }

    /** @brief Removes all scopes and bindings. */
    void clear() noexcept
    {
      frames.clear();
    }

    /** @brief Pushes a new empty lexical frame.
     *
     *  @return New active depth after the push.
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
     *  @throws std::runtime_error If no scope is active.
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
     *  @throws std::runtime_error If no scope is active.
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
     *  @param key Lookup key.
     *  @param value Payload to store.
     *  @return `false` iff the key already exists in the current scope.
     *  @throws std::runtime_error If no scope is active.
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
     *  @return Pointer to the stored value, or `nullptr` if absent.
     */
    Value * lookup_local(const Key & key) noexcept
    {
      if (frames.is_empty())
        return nullptr;
      return find_value(frames.top().bindings, key);
    }

    /** @brief Looks up `key` only in the current lexical frame.
     *
     *  @param key Lookup key.
     *  @return Pointer to the stored value, or `nullptr` if absent.
     */
    const Value * lookup_local(const Key & key) const noexcept
    {
      if (frames.is_empty())
        return nullptr;
      return find_value(frames.top().bindings, key);
    }

    /** @brief Looks up `key` from the innermost scope outward.
     *
     *  @param key Lookup key.
     *  @return Pointer to the nearest visible value, or `nullptr` if absent.
     */
    Value * lookup(const Key & key) noexcept
    {
      for (size_t i = frames.size(); i > 0; --i)
        if (auto * value = find_value(frames.access(i - 1).bindings, key);
            value != nullptr)
          return value;
      return nullptr;
    }

    /** @brief Looks up `key` from the innermost scope outward.
     *
     *  @param key Lookup key.
     *  @return Pointer to the nearest visible value, or `nullptr` if absent.
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
