/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

# ifndef TPL_DYNLISTSTACK_H
# define TPL_DYNLISTSTACK_H

# include <ahDry.H>
# include <ahIterator.H>
# include <ah-args-ctor.H>
# include <htlist.H>

namespace Aleph {

/** @brief Dynamic stack of elements of generic type T based on a singly
 *         linked list.
 *
 *  DynListStack<T> models a dynamic LIFO (Last-In-First-Out) stack with
 *  O(1) time complexity for push, pop, and top operations. The stack
 *  grows and shrinks dynamically as elements are added or removed.
 *
 *  ## Features
 *
 *  - **O(1) operations**: push, pop, top, size, is_empty
 *  - **Unbounded capacity**: limited only by available memory
 *  - **Move semantics**: efficient transfer of ownership
 *  - **Functional interface**: supports traverse, maps, filter, foldl, etc.
 *  - **STL-compatible iteration**: supports range-based for loops
 *
 *  ## Example Usage
 *
 *  @code
 *  DynListStack<int> stack;
 *  stack.push(10);
 *  stack.push(20);
 *  stack.push(30);
 *
 *  while (!stack.is_empty())
 *    std::cout << stack.pop() << " ";  // Output: 30 20 10
 *
 *  // Using initializer list
 *  DynListStack<std::string> names = {"Alice", "Bob", "Charlie"};
 *
 *  // Functional operations
 *  auto doubled = stack.maps([](int x) { return x * 2; });
 *  @endcode
 *
 *  ## Thread Safety
 *
 *  This class is not thread-safe. External synchronization is required
 *  for concurrent access from multiple threads.
 *
 *  @tparam T Type of elements stored in the stack. Must be copy-constructible
 *            or move-constructible.
 *
 *  @see DynListQueue For a FIFO queue implementation
 *  @see ArrayStack For a fixed-capacity array-based stack
 *  @see FixedStack For a compile-time fixed-size stack
 *
 *  @ingroup Sequences
 */
template <typename T>
class DynListStack
      : public LocateFunctions<DynListStack<T>, T>,
        public FunctionalMethods<DynListStack<T>, T>,
        public GenericKeys<DynListStack<T>, T>,
        public EqualToMethod<DynListStack<T>>,
        public StlAlephIterator<DynListStack<T>>
{
  DynList<T> s;
  size_t num_items = 0;

public:

  /// @brief The container type (self-reference for generic programming).
  using Set_Type = DynListStack;

  /// @brief The type of elements stored in the stack.
  using Item_Type = T;

  /// @brief Alias for Item_Type, required by EqualToMethod mixin.
  using Key_Type = T;

  /** @brief Swap the contents of this stack with another.
   *
   *  Exchanges the contents of this stack with those of @p other in
   *  constant time O(1). No elements are copied or moved individually.
   *
   *  @param[in,out] other The stack to swap with.
   *
   *  @note This operation is noexcept and provides the strong exception
   *        guarantee (either succeeds completely or has no effect).
   */
  void swap(DynListStack & other) noexcept
  {
    std::swap(num_items, other.num_items);
    s.swap(other.s);
  }

  /** @brief Construct an empty stack.
   *
   *  Creates a stack with no elements. The stack is ready to accept
   *  elements via push() or emplace().
   *
   *  Time complexity: O(1)
   */
  DynListStack() noexcept : num_items(0) { /* empty */ }

  /** @brief Copy constructor.
   *
   *  Constructs a new stack as a copy of @p other. All elements are
   *  copied in LIFO order.
   *
   *  Time complexity: O(n) where n is the size of @p other.
   *
   *  @param[in] other The stack to copy from.
   *  @throw std::bad_alloc If memory allocation fails.
   *  @throw Any exception thrown by T's copy constructor.
   */
  DynListStack(const DynListStack & other)
    : s(other.s), num_items(other.num_items)
  {
    // empty
  }

  /** @brief Move constructor.
   *
   *  Constructs a new stack by transferring ownership of resources from
   *  @p other. After this operation, @p other is left in a valid but
   *  unspecified state (typically empty).
   *
   *  Time complexity: O(1)
   *
   *  @param[in,out] other The stack to move from.
   */
  DynListStack(DynListStack && other) noexcept
  {
    swap(other);
  }

  /// Enable special constructors from ah-args-ctor.H (initializer_list, etc.)
  Special_Ctors(DynListStack, T);

  /** @brief Copy assignment operator.
   *
   *  Replaces the contents of this stack with a copy of @p rhs.
   *
   *  Time complexity: O(n + m) where n is the current size and m is the
   *  size of @p rhs.
   *
   *  @param[in] rhs The stack to copy from.
   *  @return Reference to this stack.
   *  @throw std::bad_alloc If memory allocation fails.
   *  @throw Any exception thrown by T's copy constructor.
   */
  DynListStack & operator = (const DynListStack & rhs)
  {
    if (this == &rhs)
      return *this;

    s = rhs.s;
    num_items = rhs.num_items;

    return *this;
  }

  /** @brief Move assignment operator.
   *
   *  Replaces the contents of this stack by transferring ownership from
   *  @p rhs. After this operation, @p rhs is left in a valid but
   *  unspecified state.
   *
   *  Time complexity: O(1) (plus destruction of current elements).
   *
   *  @param[in,out] rhs The stack to move from.
   *  @return Reference to this stack.
   */
  DynListStack & operator = (DynListStack && rhs) noexcept
  {
    std::swap(num_items, rhs.num_items);
    s.swap(rhs.s);
    return *this;
  }

  /** @brief Return the number of elements in the stack.
   *
   *  Time complexity: O(1)
   *
   *  @return The number of elements currently in the stack.
   */
  [[nodiscard]] size_t size() const noexcept { return num_items; }

  /** @brief Check if the stack is empty.
   *
   *  Time complexity: O(1)
   *
   *  @return true if the stack contains no elements, false otherwise.
   */
  [[nodiscard]] bool is_empty() const noexcept { return num_items == 0; }

  /** @brief Push an item by copy onto the top of the stack.
   *
   *  Adds a copy of @p data to the top of the stack.
   *
   *  Time complexity: O(1)
   *
   *  @param[in] data The item to be copied and pushed.
   *  @return A modifiable reference to the newly pushed item.
   *  @throw std::bad_alloc If memory allocation fails.
   *  @throw Any exception thrown by T's copy constructor.
   */
  T & push(const T & data)
  {
    T & ret_val = s.insert(data);
    ++num_items;
    return ret_val;
  }

  /** @brief Push an item by move onto the top of the stack.
   *
   *  Moves @p data onto the top of the stack, avoiding a copy.
   *
   *  Time complexity: O(1)
   *
   *  @param[in] data The item to be moved and pushed.
   *  @return A modifiable reference to the newly pushed item.
   *  @throw std::bad_alloc If memory allocation fails.
   */
  T & push(T && data)
  {
    T & ret_val = s.insert(std::forward<T>(data));
    ++num_items;
    return ret_val;
  }

  /** @brief Construct an item in place at the top of the stack.
   *
   *  Constructs an element directly at the top of the stack using the
   *  provided arguments. This avoids the overhead of creating a temporary
   *  object and then copying or moving it.
   *
   *  Time complexity: O(1)
   *
   *  @tparam Args Types of the constructor arguments.
   *  @param[in] args Arguments to forward to the constructor of T.
   *  @return A modifiable reference to the newly constructed item.
   *  @throw std::bad_alloc If memory allocation fails.
   *  @throw Any exception thrown by T's constructor.
   *
   *  @note Internally, this constructs the object and then moves it.
   *        For true in-place construction, the underlying container would
   *        need to support allocator-aware construction.
   */
  template <typename... Args>
  T & emplace(Args&&... args)
  {
    return push(T(std::forward<Args>(args)...));
  }

  /** @brief Alias for push() - required by Special_Ctors macro.
   *  @param[in] data The item to append.
   *  @return Reference to the newly added item.
   */
  T & append(const T & data) { return push(data); }

  /// @overload append(const T & data)
  T & append(T && data) { return push(std::forward<T>(data)); }

  /** @brief Alias for push() - for compatibility with queue-like interfaces.
   *  @param[in] data The item to put.
   *  @return Reference to the newly added item.
   */
  T & put(const T & data) { return push(data); }

  /// @overload put(const T & data)
  T & put(T && data) { return push(std::forward<T>(data)); }

  /** @brief Alias for push() - for STL-like insert semantics.
   *  @param[in] data The item to insert.
   *  @return Reference to the newly added item.
   */
  T & insert(const T & data) { return push(data); }

  /// @overload insert(const T & data)
  T & insert(T && data) { return push(std::forward<T>(data)); }

  /** @brief Remove and return the top item of the stack.
   *
   *  Removes the most recently pushed item from the stack and returns it.
   *
   *  Time complexity: O(1)
   *
   *  @return A copy of the removed item.
   *  @throw std::underflow_error If the stack is empty.
   *
   *  @warning Calling pop() on an empty stack throws an exception.
   *           Use is_empty() to check before calling.
   */
  [[nodiscard]] T pop()
  {
    T ret_val = s.remove_first();
    --num_items;
    return ret_val;
  }

  /** @brief Alias for pop() - removes and returns the top item.
   *  @return A copy of the removed item.
   *  @throw std::underflow_error If the stack is empty.
   */
  [[nodiscard]] T get() { return pop(); }

  /** @brief Return a modifiable reference to the top item of the stack.
   *
   *  Provides access to the top element without removing it.
   *
   *  Time complexity: O(1)
   *
   *  @return A modifiable reference to the top item.
   *  @throw std::underflow_error If the stack is empty.
   *
   *  @warning The reference is invalidated if the top element is removed.
   */
  [[nodiscard]] T & top()
  {
    return s.get_first();
  }

  /** @brief Return a const reference to the top item of the stack.
   *
   *  Provides read-only access to the top element without removing it.
   *
   *  Time complexity: O(1)
   *
   *  @return A const reference to the top item.
   *  @throw std::underflow_error If the stack is empty.
   */
  [[nodiscard]] const T & top() const
  {
    return s.get_first();
  }

  /** @brief Alias for top() - returns reference to top item.
   *  @return Reference to the top item.
   *  @throw std::underflow_error If the stack is empty.
   */
  [[nodiscard]] T & peek() { return top(); }

  /** @brief Alias for top() const - returns const reference to top item.
   *  @return Const reference to the top item.
   *  @throw std::underflow_error If the stack is empty.
   */
  [[nodiscard]] const T & peek() const { return top(); }

  /** @brief Remove all elements from the stack.
   *
   *  After this call, size() returns 0 and is_empty() returns true.
   *
   *  Time complexity: O(n) where n is the number of elements.
   */
  void empty() noexcept
  {
    s.empty();
    num_items = 0;
  }

  /** @brief Alias for empty() - removes all elements.
   *
   *  Provided for compatibility with STL-style interfaces.
   */
  void clear() noexcept { empty(); }

  /** @brief Traverse all elements from top to bottom.
   *
   *  Executes @p operation on each element in LIFO order (top to bottom).
   *  The traversal stops early if @p operation returns false.
   *
   *  @tparam Operation A callable type with signature `bool(T&)` or `bool(const T&)`.
   *  @param[in] operation The operation to perform on each element.
   *  @return true if @p operation returned true for all elements;
   *          false if traversal was stopped early.
   *
   *  @note The operation receives elements in the order they would be
   *        popped from the stack.
   */
  template <class Operation>
  bool traverse(Operation & operation)
  {
    return s.traverse(operation);
  }

  /// @overload traverse(Operation & operation)
  template <class Operation>
  bool traverse(Operation & operation) const
  {
    return s.traverse(operation);
  }

  /// @overload traverse(Operation & operation)
  template <class Operation>
  bool traverse(Operation && operation = Operation()) const
  {
    return s.traverse(std::forward<Operation>(operation));
  }

  /// @overload traverse(Operation & operation)
  template <class Operation>
  bool traverse(Operation && operation = Operation())
  {
    return s.traverse(std::forward<Operation>(operation));
  }

  /** @brief Search for an item in the stack using equality comparison.
   *
   *  Performs a linear search from top to bottom looking for an element
   *  equal to @p key.
   *
   *  Time complexity: O(n)
   *
   *  @param[in] key The value to search for.
   *  @return Pointer to the item if found, nullptr otherwise.
   *
   *  @note This method is required by the EqualToMethod mixin.
   *  @warning The returned pointer is invalidated if the element is removed.
   */
  T * search(const T & key) noexcept
  {
    return this->find_ptr([&key](const T & item) { return item == key; });
  }

  /// @overload search(const T & key)
  const T * search(const T & key) const noexcept
  {
    return const_cast<DynListStack*>(this)->search(key);
  }

  /** @brief Check if the stack contains a specific value.
   *
   *  Time complexity: O(n)
   *
   *  @param[in] key The value to search for.
   *  @return true if @p key is found in the stack, false otherwise.
   */
  [[nodiscard]] bool contains(const T & key) const noexcept
  {
    return search(key) != nullptr;
  }

  /// @brief Alias for contains().
  [[nodiscard]] bool has(const T & key) const noexcept
  {
    return contains(key);
  }

  /** @brief Iterator for traversing elements of the stack.
   *
   *  Provides forward iteration over stack elements in LIFO order
   *  (from top to bottom). This is the same order in which elements
   *  would be retrieved via successive pop() calls.
   *
   *  ## Example Usage
   *
   *  @code
   *  DynListStack<int> stack = {1, 2, 3};
   *
   *  // Using explicit iterator
   *  for (auto it = stack.get_it(); it.has_curr(); it.next())
   *    std::cout << it.get_curr() << " ";
   *
   *  // Using range-based for loop
   *  for (const auto & item : stack)
   *    std::cout << item << " ";
   *  @endcode
   *
   *  @note The iterator provides read-only access to elements.
   *        Modifying elements during iteration may invalidate the iterator.
   *
   *  @ingroup Sequences
   */
  struct Iterator : public DynList<T>::Iterator
  {
    /// @brief The container type this iterator operates on.
    using Set_Type = DynListStack<T>;

    /// @brief Base iterator type.
    using Base = DynList<T>::Iterator;

    /// Inherit constructors from base.
    using Base::Base;

    /** @brief Construct an iterator for a stack.
     *
     *  Creates an iterator positioned at the top of the stack.
     *
     *  @param[in] stack The stack to iterate over.
     */
    Iterator(const DynListStack<T> & stack) noexcept : Base(stack.s) {}
  };
};

} // end namespace Aleph
# endif /* TPL_DYNLISTSTACK_H */