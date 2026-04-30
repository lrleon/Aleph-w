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
 *  @brief Lexical scope management using frame stacks.
 *
 *  This header provides the `Scope` class, a fundamental structure for
 *  compilers and interpreters that need to manage name visibility (bindings)
 *  following lexical shadowing rules.
 *
 *  The mission of this module is to allow the user to define visibility "levels".
 *  When entering a new scope (e.g., a function or a block `{}`), a frame is
 *  pushed. Insertions occur in the current frame, and lookups traverse the
 *  stack from the inside out.
 *
 *  @ingroup Utilities
 */

#ifndef TPL_SCOPE_H
#define TPL_SCOPE_H

#include <concepts>
#include <cstddef>
#include <utility>

#include <ah-errors.H>
#include <tpl_dynArray.H>

namespace Aleph {
/** @brief Lexical scope stack for `Key` to `Value` associations.
 *
 *  The `Scope` class implements a hierarchical dictionary. It is ideal for
 *  symbol tables where declarations in inner blocks hide those from the
 *  outside.
 *
 *  ### Usage example:
 *  @code
 *  Scope<string, Symbol*> symbols;
 *  symbols.enter_scope(); // Enter global scope
 *  symbols.insert("x", var1);
 *
 *  symbols.enter_scope(); // Enter a block { ... }
 *  symbols.insert("x", var2); // Shadows global "x"
 *  auto* s = symbols.lookup("x"); // Returns var2
 *
 *  symbols.leave_scope(); // Leave block
 *  s = symbols.lookup("x"); // Returns var1
 *  @endcode
 *
 *  @tparam Key Search key type. Must be equality comparable.
 *  @tparam Value Payload associated with the key.
 *
 *  @note Thread-safety: Not thread-safe for concurrent mutation operations.
 *        Const operations (lookup, depth, empty) are safe for concurrent
 *        readers only if there are no active mutations.
 *  @note Complexity: O(1) for construction.
 */
template <typename Key, typename Value>
  requires std::equality_comparable<Key>
class Scope
{
public:
  /** @brief Individual entry in a lexical frame.
   *
   *  Represents a key-value pair stored in the current visibility level.
   */
  struct Binding
  {
    Key key;      ///< Search key (e.g., variable name).
    Value value;  ///< Associated value (e.g., type or address).
  };

private:
  struct Frame
  {
    DynArray<Binding> bindings;
  };

  DynArray<Frame> frames;

  static Value *find_value(DynArray<Binding> &bindings, const Key &key) noexcept
  {
    for (size_t i = bindings.size(); i > 0; --i)
      if (bindings.access(i - 1).key == key)
        return &bindings.access(i - 1).value;
    return nullptr;
  }

  static const Value *find_value(const DynArray<Binding> &bindings, const Key &key) noexcept
  {
    for (size_t i = bindings.size(); i > 0; --i)
      if (bindings.access(i - 1).key == key)
        return &bindings.access(i - 1).value;
    return nullptr;
  }

  Frame &current_frame()
  {
    ah_runtime_error_unless(frames.size() > 0) << "Scope::current_frame(): no active scope";
    return frames.top();
  }

  const Frame &current_frame() const
  {
    ah_runtime_error_unless(frames.size() > 0) << "Scope::current_frame(): no active scope";
    return frames.top();
  }

public:
  /** @brief Default constructor. Creates an instance without active frames.
   *
   *  `enter_scope()` must be called before performing the first insertion.
   */
  Scope() = default;

  /** @brief Queries the current depth of the scope stack.
   *
   *  @return Number of active frames (scopes).
   *  @note A return of 0 indicates that nothing can be inserted until
   *        `enter_scope()` is called.
   */
  [[nodiscard]] size_t depth() const noexcept
  {
    return frames.size();
  }

  /** @brief Indicates if the structure is empty of frames.
   *
   *  @return `true` if there is no active scope.
   */
  [[nodiscard]] bool empty() const noexcept
  {
    return frames.is_empty();
  }

  /** @brief Resets the structure by removing all frames and bindings.
   *
   *  Useful for reusing the `Scope` object in a new translation unit.
   */
  void clear() noexcept
  {
    frames.clear();
  }

  /** @brief Creates a new visibility level.
   *
   *  Pushes an empty frame. All subsequent `insert()` calls will affect
   *  this frame until `leave_scope()` is called.
   *
   *  @return The new depth of the stack.
   */
  size_t enter_scope()
  {
    // Reinitialize reused slots explicitly because DynArray::pop() moves the
    // last frame out before shrinking, and a later append may reuse that
    // moved-from storage.
    frames.append(Frame());
    return frames.size();
  }

  /** @brief Destroys the current visibility level and all its associations.
   *
   *  Restores the visibility of the previous (outer) frame.
   *
   *  @throws std::runtime_error If called without active frames.
   *  @note Use this method when encountering a block closure (e.g., '}').
   */
  void leave_scope()
  {
    ah_runtime_error_unless(frames.size() > 0) << "Scope::leave_scope(): no active scope";
    (void) frames.pop();
  }

  /** @brief Registers a new association in the current scope.
   *
   *  The name is added to the innermost frame. If the name already exists
   *  in an outer frame, this method will "shadow" it without error. If it
   *  already exists in the **current** frame, the insertion will fail to
   *  prevent collisions.
   *
   *  @param key Key to register (e.g., variable name).
   *  @param value Value to associate (e.g., symbol metadata).
   *  @return `true` if the insertion was successful, `false` if the key
   *          already exists in the current visibility level (redefinition
   *          forbidden).
   *  @throws std::runtime_error If there is no active scope.
   */
  bool insert(const Key &key, const Value &value)
  {
    auto &frame = current_frame();
    if (find_value(frame.bindings, key) != nullptr)
      return false;
    frame.bindings.append({key, value});
    return true;
  }

  /** @brief Move version of `insert()`.
   *
   *  Avoids expensive copies of keys and values.
   *  @see insert(const Key&, const Value&)
   */
  bool insert(Key &&key, Value &&value)
  {
    auto &frame = current_frame();
    if (find_value(frame.bindings, key) != nullptr)
      return false;
    frame.bindings.append({std::move(key), std::move(value)});
    return true;
  }

  /** @brief Searches for a key only in the innermost (current) scope.
   *
   *  Useful for detecting redefinitions in the same code block without
   *  being affected by what happens in outer scopes.
   *
   *  @param key Key to search.
   *  @return Pointer to the value if found, `nullptr` otherwise.
   */
  Value *lookup_local(const Key &key) noexcept
  {
    if (frames.is_empty())
      return nullptr;
    return find_value(frames.top().bindings, key);
  }

  /** @brief Constant version of `lookup_local()`. */
  const Value *lookup_local(const Key &key) const noexcept
  {
    if (frames.is_empty())
      return nullptr;
    return find_value(frames.top().bindings, key);
  }

  /** @brief Searches for a key traversing all active scopes (from inside out).
   *
   *  Implements standard lexical lookup: searches first in the current
   *  frame, then in the immediately superior one, and so on until the
   *  global one. This allows name shadowing.
   *
   *  @param key Key to search.
   *  @return Pointer to the nearest association value, or `nullptr`
   *          if the key is not defined in any accessible level.
   */
  Value *lookup(const Key &key) noexcept
  {
    for (size_t i = frames.size(); i > 0; --i)
      if (auto *value = find_value(frames.access(i - 1).bindings, key); value != nullptr)
        return value;
    return nullptr;
  }

  /** @brief Constant version of `lookup()`. */
  const Value *lookup(const Key &key) const noexcept
  {
    for (size_t i = frames.size(); i > 0; --i)
      if (const auto *value = find_value(frames.access(i - 1).bindings, key); value != nullptr)
        return value;
    return nullptr;
  }
};
}  // namespace Aleph

#endif  // TPL_SCOPE_H
