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

/** @file ah-dry-mixin.H
 *  @brief CRTP Mixins for container functionality (DRY principle).
 *
 *  This file provides CRTP (Curiously Recurring Template Pattern) mixin
 *  classes that implement common container operations. These mixins replace
 *  the macro-based approach in ahDry.H with properly documented, type-safe
 *  template classes.
 *
 *  ## Usage
 *
 *  To use these mixins, inherit from them using CRTP:
 *
 *  ```cpp
 *  template <typename T>
 *  class MyContainer : public TraverseMixin<MyContainer<T>, T>,
 *                      public FunctionalMixin<MyContainer<T>, T>,
 *                      public LocateMixin<MyContainer<T>, T>
 *  {
 *    // Your container implementation
 *    // Must provide: get_it(), Iterator with has_curr(), get_curr(), next_ne()
 *  };
 *  ```
 *
 *  ## Requirements
 *
 *  The derived class must provide:
 *  - `Iterator` nested type with:
 *    - `has_curr()` - returns true if iterator points to valid element
 *    - `get_curr()` - returns reference to current element
 *    - `next_ne()` - advances to next element (no exception version)
 *  - `get_it()` - returns an Iterator positioned at the first element
 *
 *  @ingroup Utils
 */

#ifndef AH_DRY_MIXIN_H
#define AH_DRY_MIXIN_H

#include <cstddef>
#include <tuple>
#include <functional>
#include <utility>

#include <ah-errors.H>

namespace Aleph
{

// Forward declarations
template<typename T> class DynList;

// Dft_Map_Op is defined in ahFunctional.H, we just forward declare here
// to avoid circular dependency. When ahFunctional.H is included, it provides
// the actual definition.
template <typename T, typename R> struct Dft_Map_Op;


// =============================================================================
// TraverseMixin - Basic traversal operations
// =============================================================================

/** @brief CRTP Mixin providing traversal operations.
 *
 *  This mixin adds the `traverse()` method to containers, which is the
 *  foundation for all other functional operations.
 *
 *  @tparam Derived The derived container class (CRTP pattern).
 *  @tparam Type The element type stored in the container.
 *
 *  @par Requirements
 *  The derived class must provide:
 *  - `Iterator` nested type
 *  - `get_it()` method returning an Iterator
 *
 *  @ingroup Utils
 */
template <typename Derived, typename Type>
class TraverseMixin
{
protected:
  /// @brief Access the derived class (const version).
  [[nodiscard]] const Derived& self() const noexcept
  {
    return static_cast<const Derived&>(*this);
  }

  /// @brief Access the derived class (mutable version).
  [[nodiscard]] Derived& self() noexcept
  {
    return static_cast<Derived&>(*this);
  }

public:
  /** @brief Traverse all elements, applying an operation to each.
   *
   *  Iterates through all elements in the container, calling the operation
   *  on each element. The traversal stops early if the operation returns false.
   *
   *  @tparam Operation Callable type with signature `bool(const Type&)` or `bool(Type&)`.
   *  @param operation The operation to apply to each element.
   *  @return true if all elements were visited (operation always returned true),
   *          false if traversal was stopped early.
   *
   *  @par Complexity
   *  O(n) where n is the number of elements.
   *
   *  @par Example
   *  @code
   *  DynList<int> list = {1, 2, 3, 4, 5};
   *
   *  // Print all elements
   *  list.traverse([](const int& x) {
   *    std::cout << x << " ";
   *    return true;  // continue
   *  });
   *
   *  // Find first even number (stops early)
   *  int found = 0;
   *  list.traverse([&found](const int& x) {
   *    if (x % 2 == 0) {
   *      found = x;
   *      return false;  // stop
   *    }
   *    return true;
   *  });
   *  @endcode
   */
  template <class Operation>
  bool traverse(Operation & operation) const
  {
    for (typename Derived::Iterator it(self()); it.has_curr(); it.next_ne())
      if (not operation(it.get_curr()))
        return false;
    return true;
  }

  /// @overload
  template <class Operation>
  bool traverse(Operation & operation)
  {
    for (typename Derived::Iterator it(self()); it.has_curr(); it.next_ne())
      if (not operation(it.get_curr()))
        return false;
    return true;
  }

  /// @overload
  /// @brief Version accepting rvalue operation (temporary lambdas).
  template <class Operation>
  bool traverse(Operation && operation = Operation()) const
  {
    return traverse<Operation>(operation);
  }

  /// @overload
  template <class Operation>
  bool traverse(Operation && operation = Operation())
  {
    return traverse<Operation>(operation);
  }
};


// =============================================================================
// LocateMixin - Element location operations
// =============================================================================

/** @brief CRTP Mixin providing element location operations.
 *
 *  This mixin adds methods to find and locate elements within a container:
 *  - `nth()` / `nth_ne()` - access element by index
 *  - `find_ptr()` - find element matching a predicate
 *  - `find_item()` - find with success flag
 *
 *  @tparam Derived The derived container class (CRTP pattern).
 *  @tparam Type The element type stored in the container.
 *
 *  @par Requirements
 *  The derived class must provide `traverse()` method (typically via TraverseMixin).
 *
 *  @ingroup Utils
 */
template <typename Derived, typename Type>
class LocateMixin
{
protected:
  [[nodiscard]] const Derived& self() const noexcept
  {
    return static_cast<const Derived&>(*this);
  }

  [[nodiscard]] Derived& self() noexcept
  {
    return static_cast<Derived&>(*this);
  }

public:
  /** @brief Access the n-th element (bounds-checked).
   *
   *  Returns a reference to the element at position n (0-indexed).
   *
   *  @param n Zero-based index of the element.
   *  @return Reference to the n-th element.
   *  @throws std::out_of_range if n >= size().
   *
   *  @par Complexity
   *  O(n) - linear traversal to the n-th element.
   *
   *  @par Example
   *  @code
   *  DynList<int> list = {10, 20, 30};
   *  int x = list.nth(1);  // x = 20
   *  list.nth(1) = 25;     // list = {10, 25, 30}
   *  @endcode
   *
   *  @see nth_ne() for unchecked version.
   */
  Type & nth(const size_t n) const
  {
    Type * ptr = nullptr;
    size_t i = 0;
    self().traverse([&ptr, &i, &n] (Type & item)
    {
      if (i++ < n)
        return true;
      ptr = &item;
      return false;
    });

    ah_out_of_range_error_if(i != n + 1) << "index " << n << " out of range";

    return *ptr;
  }

  /** @brief Access the n-th element (unchecked).
   *
   *  Returns a reference to the element at position n without bounds checking.
   *
   *  @param n Zero-based index of the element.
   *  @return Reference to the n-th element.
   *
   *  @warning Undefined behavior if n >= size().
   *
   *  @par Complexity
   *  O(n) - linear traversal to the n-th element.
   *
   *  @see nth() for bounds-checked version.
   */
  Type & nth_ne(const size_t n) const noexcept
  {
    Type * ptr = nullptr;
    size_t i = 0;
    self().traverse([&ptr, &i, &n] (Type & item)
    {
      if (i++ < n)
        return true;
      ptr = &item;
      return false;
    });
    return *ptr;
  }

  /** @brief Find the first element satisfying a predicate.
   *
   *  Searches for the first element where the operation returns true.
   *
   *  @tparam Operation Callable with signature `bool(Type&)` or `bool(const Type&)`.
   *  @param operation Predicate to test each element.
   *  @return Pointer to the first matching element, or nullptr if not found.
   *
   *  @par Complexity
   *  O(n) worst case.
   *
   *  @par Example
   *  @code
   *  DynList<int> list = {1, 2, 3, 4, 5};
   *
   *  // Find first even number
   *  int* p = list.find_ptr([](int x) { return x % 2 == 0; });
   *  if (p) std::cout << *p;  // prints 2
   *
   *  // Modify through pointer
   *  *p = 20;  // list = {1, 20, 3, 4, 5}
   *  @endcode
   *
   *  @see find_item() for version returning success flag with copy.
   */
  template <class Operation>
  Type * find_ptr(Operation & operation)
  {
    Type * ptr = nullptr;
    self().traverse([&ptr, &operation] (Type & item)
    {
      if (operation(item))
      {
        ptr = &item;
        return false;
      }
      return true;
    });
    return ptr;
  }

  /// @overload
  /// @brief Const version.
  template <class Operation>
  Type * find_ptr(Operation & operation) const
  {
    Type * ptr = nullptr;
    self().traverse([&ptr, &operation] (Type & item)
    {
      if (operation(item))
      {
        ptr = &item;
        return false;
      }
      return true;
    });
    return ptr;
  }

  /// @overload
  /// @brief Version accepting rvalue operation.
  template <class Operation>
  Type * find_ptr(Operation && operation = Operation()) const
  {
    return find_ptr<Operation>(operation);
  }

  /// @overload
  template <class Operation>
  Type * find_ptr(Operation && operation = Operation())
  {
    return find_ptr<Operation>(operation);
  }

  /** @brief Find element with success flag.
   *
   *  Like find_ptr() but returns a tuple containing:
   *  - bool: true if element was found
   *  - Type: copy of the found element (or default-constructed if not found)
   *
   *  @tparam Operation Callable with signature `bool(const Type&)`.
   *  @param operation Predicate to test each element.
   *  @return Tuple of (found, element).
   *
   *  @par Example
   *  @code
   *  DynList<std::string> list = {"apple", "banana", "cherry"};
   *
   *  auto [found, item] = list.find_item([](const auto& s) {
   *    return s.starts_with("b");
   *  });
   *
   *  if (found)
   *    std::cout << item;  // prints "banana"
   *  @endcode
   *
   *  @note This returns a copy. Use find_ptr() to avoid copying.
   */
  template <class Operation>
  std::tuple<bool, Type> find_item(Operation & operation)
  {
    auto ptr = find_ptr(operation);
    return ptr ? std::make_tuple(true, *ptr) : std::make_tuple(false, Type());
  }

  /// @overload
  template <class Operation>
  std::tuple<bool, Type> find_item(Operation & operation) const
  {
    auto ptr = find_ptr(operation);
    return ptr ? std::make_tuple(true, *ptr) : std::make_tuple(false, Type());
  }

  /// @overload
  template <class Operation>
  std::tuple<bool, Type> find_item(Operation && operation = Operation())
  {
    return find_item(operation);
  }

  /// @overload
  template <class Operation>
  std::tuple<bool, Type> find_item(Operation && operation = Operation()) const
  {
    return find_item(operation);
  }
};


// =============================================================================
// FunctionalMixin - Functional programming operations
// =============================================================================

/** @brief CRTP Mixin providing functional programming operations.
 *
 *  This mixin adds functional programming methods to containers:
 *
 *  - **Iteration**: for_each(), mutable_for_each()
 *  - **Predicates**: all(), forall(), exists()
 *  - **Transformation**: maps(), filter(), partition(), rev()
 *  - **Folding**: foldl(), fold_left(), fold()
 *  - **Slicing**: take(), drop()
 *  - **Utility**: length()
 *
 *  @tparam Derived The derived container class (CRTP pattern).
 *  @tparam Type The element type stored in the container.
 *
 *  @par Requirements
 *  The derived class must provide:
 *  - `traverse()` method (typically via TraverseMixin)
 *  - `for_each()` method (provided by this mixin, but needs traverse)
 *
 *  @ingroup Utils
 */
template <typename Derived, typename Type>
class FunctionalMixin
{
protected:
  [[nodiscard]] const Derived& self() const noexcept
  {
    return static_cast<const Derived&>(*this);
  }

  [[nodiscard]] Derived& self() noexcept
  {
    return static_cast<Derived&>(*this);
  }

public:

  // ---------------------------------------------------------------------------
  // Iteration
  // ---------------------------------------------------------------------------

  /** @brief Apply an operation to each element (read-only).
   *
   *  Iterates through all elements, calling the operation on each.
   *  Unlike traverse(), always visits all elements (cannot stop early).
   *
   *  @tparam Operation Callable with signature `void(const Type&)`.
   *  @param operation The operation to apply.
   *  @return Reference to this container (for chaining).
   *
   *  @par Example
   *  @code
   *  DynList<int> list = {1, 2, 3};
   *  list.for_each([](int x) { std::cout << x << " "; });
   *  // Output: 1 2 3
   *  @endcode
   */
  template <class Operation>
  auto for_each(Operation & operation) const -> decltype(self())
  {
    self().traverse([&operation] (const Type & item)
    {
      operation(item);
      return true;
    });
    return self();
  }

  /// @overload
  template <class Operation>
  auto for_each(Operation & operation) -> decltype(self())
  {
    self().traverse([&operation] (const Type & item)
    {
      operation(item);
      return true;
    });
    return self();
  }

  /// @overload
  template <class Operation>
  auto for_each(Operation && operation = Operation()) const -> decltype(self())
  {
    return for_each<Operation>(operation);
  }

  /// @overload
  template <class Operation>
  auto for_each(Operation && operation = Operation()) -> decltype(self())
  {
    return for_each<Operation>(operation);
  }

  /** @brief Apply an operation to each element (mutable).
   *
   *  Like for_each() but allows modifying elements.
   *
   *  @tparam Operation Callable with signature `void(Type&)`.
   *  @param operation The operation to apply (can modify elements).
   *  @return Reference to this container.
   *
   *  @par Example
   *  @code
   *  DynList<int> list = {1, 2, 3};
   *  list.mutable_for_each([](int& x) { x *= 2; });
   *  // list = {2, 4, 6}
   *  @endcode
   */
  template <class Operation>
  auto mutable_for_each(Operation & operation) -> decltype(self())
  {
    self().traverse([&operation] (Type & item)
    {
      operation(item);
      return true;
    });
    return self();
  }

  /// @overload
  template <class Operation>
  auto mutable_for_each(Operation && operation = Operation()) -> decltype(self())
  {
    return mutable_for_each<Operation>(operation);
  }

  // ---------------------------------------------------------------------------
  // Predicates
  // ---------------------------------------------------------------------------

  /** @brief Test if all elements satisfy a predicate.
   *
   *  Returns true if the predicate returns true for every element.
   *  Returns true for empty containers (vacuous truth).
   *
   *  @tparam Operation Callable with signature `bool(const Type&)`.
   *  @param operation Predicate to test.
   *  @return true if all elements satisfy the predicate.
   *
   *  @par Complexity
   *  O(n) worst case, but stops early on first false.
   *
   *  @par Example
   *  @code
   *  DynList<int> list = {2, 4, 6, 8};
   *  bool allEven = list.all([](int x) { return x % 2 == 0; });  // true
   *  @endcode
   */
  template <class Operation>
  [[nodiscard]] bool all(Operation & operation) const
  {
    return self().template traverse<Operation>(operation);
  }

  /// @overload
  template <class Operation>
  [[nodiscard]] bool all(Operation && operation = Operation()) const
  {
    return all<Operation>(operation);
  }

  /** @brief Alias for all().
   *  @see all()
   */
  template <class Operation>
  [[nodiscard]] bool forall(Operation & operation) const
  {
    return all<Operation>(operation);
  }

  /// @overload
  template <class Operation>
  [[nodiscard]] bool forall(Operation && operation = Operation()) const
  {
    return all<Operation>(operation);
  }

  /** @brief Test if any element satisfies a predicate.
   *
   *  Returns true if the predicate returns true for at least one element.
   *  Returns false for empty containers.
   *
   *  @tparam Operation Callable with signature `bool(const Type&)`.
   *  @param operation Predicate to test.
   *  @return true if at least one element satisfies the predicate.
   *
   *  @par Complexity
   *  O(n) worst case, but stops early on first true.
   *
   *  @par Example
   *  @code
   *  DynList<int> list = {1, 3, 5, 6, 7};
   *  bool hasEven = list.exists([](int x) { return x % 2 == 0; });  // true (6)
   *  @endcode
   */
  template <class Operation>
  [[nodiscard]] bool exists(Operation & operation) const
  {
    return not self().traverse([&operation] (const Type & item)
    {
      return not operation(item);
    });
  }

  /// @overload
  template <class Operation>
  [[nodiscard]] bool exists(Operation && operation = Operation()) const
  {
    return exists<Operation>(operation);
  }

  // ---------------------------------------------------------------------------
  // Transformation
  // ---------------------------------------------------------------------------

  /** @brief Transform elements using a mapping function.
   *
   *  Creates a new container with each element transformed by the operation.
   *  This is the classic "map" operation from functional programming.
   *
   *  @tparam __Type Result type (default: same as input Type).
   *  @tparam Container Result container type (default: DynList).
   *  @tparam Operation Callable with signature `__Type(const Type&)`.
   *  @param operation Transformation function.
   *  @return New container with transformed elements.
   *
   *  @par Complexity
   *  O(n).
   *
   *  @par Example
   *  @code
   *  DynList<int> nums = {1, 2, 3};
   *
   *  // Square each number
   *  auto squares = nums.maps<int>([](int x) { return x * x; });
   *  // squares = {1, 4, 9}
   *
   *  // Convert to strings
   *  auto strs = nums.maps<std::string>([](int x) { return std::to_string(x); });
   *  // strs = {"1", "2", "3"}
   *  @endcode
   */
  template <typename __Type = Type,
            template <typename> class Container = Aleph::DynList,
            class Operation = Dft_Map_Op<Type, __Type>>
  [[nodiscard]] Container<__Type> maps(Operation & operation) const
  {
    Container<__Type> ret_val;
    for_each([&ret_val, &operation] (const Type & item)
    {
      ret_val.append(operation(item));
    });
    return ret_val;
  }

  /// @overload
  template <typename __Type = Type,
            template <typename> class Container = Aleph::DynList,
            class Operation = Dft_Map_Op<Type, __Type>>
  [[nodiscard]] Container<__Type> maps(Operation && operation = Operation()) const
  {
    return maps<__Type, Container, Operation>(operation);
  }

  /** @brief Left fold (reduce) with initial value.
   *
   *  Combines all elements using a binary operation, starting from an
   *  initial value. Computes: op(...op(op(init, x1), x2)..., xn).
   *
   *  @tparam __Type Accumulator type (default: same as element Type).
   *  @param init Initial value for the accumulator.
   *  @param operation Binary operation: (accumulator, element) -> new_accumulator.
   *  @return Final accumulated value.
   *
   *  @par Complexity
   *  O(n).
   *
   *  @par Example
   *  @code
   *  DynList<int> nums = {1, 2, 3, 4, 5};
   *
   *  // Sum all elements
   *  int sum = nums.foldl<int>(0, [](int acc, int x) { return acc + x; });
   *  // sum = 15
   *
   *  // Concatenate strings
   *  DynList<std::string> words = {"hello", " ", "world"};
   *  auto sentence = words.foldl<std::string>("", [](auto acc, auto s) {
   *    return acc + s;
   *  });
   *  // sentence = "hello world"
   *  @endcode
   */
  template <typename __Type = Type>
  [[nodiscard]] __Type foldl(const __Type & init,
                             std::function<__Type(const __Type&, const Type&)> operation) const
  {
    auto ret_val = init;
    for_each([&ret_val, &operation] (const Type & item)
    {
      ret_val = operation(ret_val, item);
    });
    return ret_val;
  }

  /** @brief Left fold with operation first (alternative signature).
   *  @see foldl()
   */
  template <typename __Type = Type>
  [[nodiscard]] __Type fold_left(std::function<__Type(const __Type&, const Type&)> operation,
                                 const __Type & init) const
  {
    return foldl<__Type>(init, operation);
  }

  /** @brief Simple fold with same type for accumulator and elements.
   *
   *  Simplified version of foldl() where accumulator has same type as elements.
   *
   *  @tparam Operation Binary operation type.
   *  @param init Initial value.
   *  @param operation Binary operation.
   *  @return Final accumulated value.
   */
  template <class Operation>
  [[nodiscard]] Type fold(const Type & init, Operation & operation) const
  {
    auto ret_val = init;
    for_each([&ret_val, &operation] (const Type & item)
    {
      ret_val = operation(ret_val, item);
    });
    return ret_val;
  }

  /// @overload
  template <class Operation>
  [[nodiscard]] Type fold(const Type & init, Operation && operation = Operation()) const
  {
    return fold<Operation>(init, operation);
  }

  /** @brief Filter elements by a predicate.
   *
   *  Creates a new list containing only elements for which the predicate
   *  returns true.
   *
   *  @tparam Operation Callable with signature `bool(const Type&)`.
   *  @param operation Predicate function.
   *  @return DynList containing only matching elements.
   *
   *  @par Complexity
   *  O(n).
   *
   *  @par Example
   *  @code
   *  DynList<int> nums = {1, 2, 3, 4, 5, 6};
   *  auto evens = nums.filter([](int x) { return x % 2 == 0; });
   *  // evens = {2, 4, 6}
   *  @endcode
   */
  template <class Operation>
  [[nodiscard]] DynList<Type> filter(Operation & operation) const
  {
    DynList<Type> ret_val;
    for_each([&ret_val, &operation] (const Type & item)
    {
      if (operation(item))
        ret_val.append(item);
    });
    return ret_val;
  }

  /// @overload
  template <class Operation>
  [[nodiscard]] DynList<Type> filter(Operation && operation = Operation()) const
  {
    return filter<Operation>(operation);
  }

  /** @brief Filter with position information.
   *
   *  Like filter(), but returns tuples of (element, index) for matching elements.
   *
   *  @tparam Operation Callable with signature `bool(const Type&)`.
   *  @param operation Predicate function.
   *  @return DynList of tuples (element, original_index).
   *
   *  @par Example
   *  @code
   *  DynList<int> nums = {1, 2, 3, 4, 5};
   *  auto result = nums.pfilter([](int x) { return x > 2; });
   *  // result = {(3, 2), (4, 3), (5, 4)}
   *  @endcode
   */
  template <class Operation>
  [[nodiscard]] DynList<std::tuple<Type, size_t>> pfilter(Operation & operation) const
  {
    DynList<std::tuple<Type, size_t>> ret_val;
    size_t i = 0;
    for_each([&ret_val, &operation, &i] (const Type & item)
    {
      if (operation(item))
        ret_val.append(std::make_tuple(item, i));
      ++i;
    });
    return ret_val;
  }

  /// @overload
  template <class Operation>
  [[nodiscard]] DynList<std::tuple<Type, size_t>> pfilter(Operation && operation = Operation()) const
  {
    return pfilter<Operation>(operation);
  }

  /** @brief Partition elements by a predicate.
   *
   *  Splits the container into two lists: elements satisfying the predicate
   *  go into `first`, others go into `second`.
   *
   *  @tparam Operation Callable with signature `bool(const Type&)`.
   *  @param op Predicate function.
   *  @return Pair of (matching_elements, non_matching_elements).
   *
   *  @par Example
   *  @code
   *  DynList<int> nums = {1, 2, 3, 4, 5, 6};
   *  auto [evens, odds] = nums.partition([](int x) { return x % 2 == 0; });
   *  // evens = {2, 4, 6}, odds = {1, 3, 5}
   *  @endcode
   */
  template <class Operation>
  [[nodiscard]] std::pair<DynList<Type>, DynList<Type>> partition(Operation & op) const
  {
    std::pair<DynList<Type>, DynList<Type>> ret_val;
    for_each([&ret_val, &op] (const Type & item)
    {
      if (op(item))
        ret_val.first.append(item);
      else
        ret_val.second.append(item);
    });
    return ret_val;
  }

  /// @overload
  template <class Operation>
  [[nodiscard]] std::pair<DynList<Type>, DynList<Type>> partition(Operation && op = Operation()) const
  {
    return partition<Operation>(op);
  }

  /** @brief Partition returning tuple instead of pair.
   *  @see partition()
   */
  template <class Operation>
  [[nodiscard]] std::tuple<DynList<Type>, DynList<Type>> tpartition(Operation & op) const
  {
    DynList<Type> r1, r2;
    for_each([&r1, &r2, &op] (const Type & item)
    {
      if (op(item))
        r1.append(item);
      else
        r2.append(item);
    });
    return std::make_tuple(r1, r2);
  }

  /// @overload
  template <class Operation>
  [[nodiscard]] std::tuple<DynList<Type>, DynList<Type>> tpartition(Operation && op = Operation()) const
  {
    return tpartition<Operation>(op);
  }

  // ---------------------------------------------------------------------------
  // Utility
  // ---------------------------------------------------------------------------

  /** @brief Count the number of elements.
   *
   *  @return Number of elements in the container.
   *
   *  @par Complexity
   *  O(n) - must traverse all elements.
   *
   *  @note If the container maintains a size counter, prefer using size() instead.
   */
  [[nodiscard]] size_t length() const noexcept
  {
    size_t count = 0;
    for_each([&count] (const Type &) { ++count; });
    return count;
  }

  /** @brief Create a reversed copy.
   *
   *  @tparam Container Result container type (default: DynList).
   *  @return New container with elements in reverse order.
   *
   *  @par Complexity
   *  O(n).
   *
   *  @par Example
   *  @code
   *  DynList<int> list = {1, 2, 3};
   *  auto reversed = list.rev();
   *  // reversed = {3, 2, 1}
   *  @endcode
   */
  template <template <typename> class Container = Aleph::DynList>
  [[nodiscard]] Container<Type> rev() const
  {
    Container<Type> ret_val;
    for_each([&ret_val] (const Type & item)
    {
      ret_val.insert(item);
    });
    return ret_val;
  }

  // ---------------------------------------------------------------------------
  // Slicing
  // ---------------------------------------------------------------------------

  /** @brief Take the first n elements.
   *
   *  @tparam Container Result container type (default: DynList).
   *  @param n Maximum number of elements to take.
   *  @return New container with at most n elements from the beginning.
   *
   *  @par Complexity
   *  O(min(n, size())).
   *
   *  @par Example
   *  @code
   *  DynList<int> list = {1, 2, 3, 4, 5};
   *  auto first3 = list.take(3);   // {1, 2, 3}
   *  auto all = list.take(100);    // {1, 2, 3, 4, 5}
   *  @endcode
   */
  template <template <typename> class Container = Aleph::DynList>
  [[nodiscard]] Container<Type> take(const size_t n) const
  {
    size_t i = 0;
    Container<Type> ret;
    self().traverse([&i, &ret, n] (const Type & item)
    {
      if (i++ >= n)
        return false;
      ret.append(item);
      return true;
    });
    return ret;
  }

  /** @brief Skip the first n elements.
   *
   *  @tparam Container Result container type (default: DynList).
   *  @param n Number of elements to skip.
   *  @return New container with elements after the first n.
   *
   *  @par Complexity
   *  O(size()).
   *
   *  @par Example
   *  @code
   *  DynList<int> list = {1, 2, 3, 4, 5};
   *  auto after2 = list.drop(2);   // {3, 4, 5}
   *  auto empty = list.drop(100);  // {}
   *  @endcode
   */
  template <template <typename> class Container = Aleph::DynList>
  [[nodiscard]] Container<Type> drop(const size_t n) const
  {
    size_t i = 0;
    Container<Type> ret;
    self().traverse([&i, &ret, n] (const Type & item)
    {
      if (i++ >= n)
        ret.append(item);
      return true;
    });
    return ret;
  }
};


// =============================================================================
// KeysMixin - Keys/Items extraction
// =============================================================================

/** @brief CRTP Mixin for extracting keys from set-like containers.
 *
 *  @tparam Derived The derived container class.
 *  @tparam Type The key/element type.
 */
template <typename Derived, typename Type>
class KeysMixin
{
protected:
  [[nodiscard]] const Derived& self() const noexcept
  {
    return static_cast<const Derived&>(*this);
  }

public:
  /** @brief Extract all keys as a list.
   *
   *  @tparam Container Result container type (default: DynList).
   *  @return Container with all keys.
   */
  template <template <typename> class Container = DynList>
  [[nodiscard]] Container<Type> keys() const
  {
    return self().template maps<Type, Container>([] (const Type & key)
    {
      return key;
    });
  }

  /** @brief Alias for keys().
   *  @see keys()
   */
  template <template <typename> class Container = DynList>
  [[nodiscard]] Container<Type> items() const
  {
    return keys<Container>();
  }
};


} // end namespace Aleph

#endif // AH_DRY_MIXIN_H

