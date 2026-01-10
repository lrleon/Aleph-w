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

# ifndef AHSINGLETON_H
# define AHSINGLETON_H

/**
 * @file ahSingleton.H
 * @brief Minimal, header-only singleton helpers using the Meyers singleton pattern.
 *
 * This header provides two mechanisms for implementing the Singleton design pattern:
 *
 * 1. **Make_Singleton(T) macro**: Injects singleton functionality into any class.
 *    This is the recommended approach for most use cases.
 *
 * 2. **Singleton class**: A ready-to-use singleton class provided as an example
 *    and for cases where a minimal singleton is needed.
 *
 * ## Thread Safety
 *
 * Both mechanisms use function-local static variables (Meyers singleton). Since
 * C++11, the initialization of function-local static variables is guaranteed to
 * be thread-safe by the standard (§6.7 [stmt.dcl] paragraph 4). This means:
 *
 * - Concurrent calls to `get_instance()` are safe.
 * - The singleton instance is created exactly once.
 * - No explicit locking or synchronization is required.
 *
 * ## Object Lifetime
 *
 * The singleton instance is constructed on first call to `get_instance()` and
 * destroyed at program termination. Destruction follows the usual static
 * deinitialization rules: objects are destroyed in reverse order of their
 * construction across all translation units.
 *
 * @warning Be cautious of the "static initialization order fiasco" if your
 *          singleton depends on other static objects during construction or
 *          destruction.
 *
 * ## Usage Example
 *
 * ```cpp
 * // Using the macro (recommended)
 * class Logger
 * {
 *   Make_Singleton(Logger)
 *
 * private:
 *   Logger() { // initialize logger }
 *
 * public:
 *   void log(const std::string & msg) { // ... }
 * };
 *
 * // Usage:
 * Logger::get_instance().log("Hello, World!");
 * ```
 *
 * @ingroup Utilities
 * @author Leandro Rabindranath León
 */

/**
 * @brief Macro to inject Meyers singleton functionality into a class.
 *
 * This macro adds a static `get_instance()` method that returns a reference to
 * the unique instance of the class, and deletes copy/move constructors and
 * assignment operators to prevent duplication.
 *
 * ## Usage
 *
 * Place the macro at the beginning of your class definition:
 *
 * ```cpp
 * class MyClass
 * {
 *   Make_Singleton(MyClass)
 *
 * private:
 *   MyClass() = default;  // Constructor must be accessible from get_instance()
 *
 * public:
 *   // Public interface...
 *   void doSomething();
 * };
 * ```
 *
 * ## Requirements
 *
 * - The type must be default-constructible.
 * - The default constructor must be accessible from the `get_instance()` method.
 *   This typically means either:
 *   - Place `Make_Singleton(T)` before the `private:` section containing the
 *     constructor, or
 *   - Make the constructor `private` but place `Make_Singleton(T)` inside the
 *     class (it becomes a friend of the static method context).
 *
 * ## Guarantees
 *
 * - **Single instance**: All calls to `get_instance()` return the same object.
 * - **Thread-safe initialization**: Guaranteed by C++11 and later standards.
 * - **Non-copyable/non-movable**: Copy and move operations are explicitly deleted.
 *
 * @param name The name of the class to make into a singleton.
 *
 * @note The macro expands to `public:` visibility, so any declarations after
 *       the macro will be public unless you explicitly change visibility.
 */
# define Make_Singleton(name)                          \
  public:                                              \
                                                       \
  /** @brief Returns the unique instance of name.      \
   *  @return Reference to the singleton instance.     \
   *  Thread-safe: guaranteed by C++11 standard. */    \
  [[nodiscard]] static name & get_instance() noexcept  \
  {                                                    \
    static name instance;                              \
    return instance;                                   \
  }                                                    \
                                                       \
  name(const name &) = delete;                         \
  name(name &&) = delete;                              \
  name & operator = (const name &) = delete;           \
  name & operator = (name &&) = delete;

/**
 * @brief A minimal singleton class implementation.
 *
 * This class demonstrates the Meyers singleton pattern and can be used directly
 * when a simple singleton with no additional state or behavior is needed.
 *
 * For singletons with custom state or behavior, prefer using the `Make_Singleton`
 * macro on your own class.
 *
 * ## Example
 *
 * ```cpp
 * Singleton & s1 = Singleton::get_instance();
 * Singleton & s2 = Singleton::get_instance();
 * assert(&s1 == &s2);  // Same instance
 * ```
 *
 * ## Thread Safety
 *
 * Thread-safe initialization is guaranteed by the C++11 standard for
 * function-local static variables.
 *
 * @see Make_Singleton
 */
class Singleton
{
public:
  /**
   * @brief Returns the unique instance of the Singleton.
   *
   * The instance is created on the first call (lazy initialization) and
   * the same instance is returned on all subsequent calls.
   *
   * @return Reference to the singleton instance.
   *
   * @note Thread-safe: initialization is guaranteed to be thread-safe
   *       by the C++11 standard (function-local static).
   */
  [[nodiscard]] static Singleton & get_instance() noexcept
  {
    static Singleton instance;
    return instance;
  }

  /// @name Deleted copy and move operations
  /// @{
  Singleton(const Singleton &) = delete;
  Singleton(Singleton &&) = delete;
  Singleton & operator = (const Singleton &) = delete;
  Singleton & operator = (Singleton &&) = delete;
  /// @}

private:
  /// @brief Private constructor to prevent direct instantiation.
  Singleton() = default;
};

# endif // AHSINGLETON_H
