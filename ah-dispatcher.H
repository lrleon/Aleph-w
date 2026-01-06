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

/** @file ah-dispatcher.H
 *  @brief Command dispatcher pattern implementation.
 *
 *  This file provides two dispatcher classes that map keys to callable
 *  operations, enabling dynamic command dispatching based on runtime keys.
 *
 *  ## Overview
 *
 *  A dispatcher is a registry that associates keys with operations (functions,
 *  lambdas, or function objects). At runtime, you can execute an operation
 *  by providing its key. This is useful for:
 *
 *  - Command-line argument handling
 *  - Event-driven systems
 *  - Plugin architectures
 *  - State machines
 *  - Menu systems
 *
 *  ## Available Dispatchers
 *
 *  | Class | Backing Store | Lookup Time |
 *  |-------|---------------|-------------|
 *  | AHDispatcher | DynMapTree (balanced tree) | O(log n) |
 *  | AhHashDispatcher | ODhashTable (hash table) | O(1) average |
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  #include <ah-dispatcher.H>
 *  #include <functional>
 *
 *  // Define operations
 *  int add(int a, int b) { return a + b; }
 *  int sub(int a, int b) { return a - b; }
 *
 *  // Create dispatcher with string keys and function pointers
 *  AHDispatcher<std::string, int(*)(int, int)> calc;
 *  calc.insert("add", add);
 *  calc.insert("sub", sub);
 *
 *  // Execute operations by key
 *  int result = calc.run("add", 5, 3);  // returns 8
 *
 *  // Check if key exists
 *  if (calc.valid_key("mul"))
 *    calc.run("mul", 2, 3);
 *  ```
 *
 *  ## With Lambdas
 *
 *  ```cpp
 *  using Op = std::function<void(const std::string&)>;
 *  AHDispatcher<std::string, Op> menu;
 *  
 *  menu.insert("help", [](const std::string&) { 
 *    std::cout << "Available commands: help, quit\n"; 
 *  });
 *  menu.insert("quit", [](const std::string&) { 
 *    exit(0); 
 *  });
 *  
 *  menu.run("help", "");
 *  ```
 *
 *  @see DynMapTree Tree-based map for AHDispatcher
 *  @see ODhashTable Hash table for AhHashDispatcher
 *
 *  @ingroup Utilidades
 *  @author Leandro Rabindranath León
 */

#ifndef AH_DISPATCHER_H
#define AH_DISPATCHER_H

# include <iostream>
# include <tpl_dynMapTree.H>
# include <tpl_odhash.H>

/**
 * @brief Tree-based command dispatcher.
 *
 * Maps keys to operations using a balanced tree (DynMapTree).
 * Provides O(log n) lookup, insertion, and removal.
 *
 * @tparam Key The key type (must be comparable with <)
 * @tparam Operation The callable type (function pointer, std::function, etc.)
 *
 * ## Example
 *
 * ```cpp
 * // Calculator dispatcher
 * AHDispatcher<char, double(*)(double, double)> ops;
 * ops.insert('+', [](double a, double b) { return a + b; });
 * ops.insert('-', [](double a, double b) { return a - b; });
 * ops.insert('*', [](double a, double b) { return a * b; });
 * ops.insert('/', [](double a, double b) { return a / b; });
 *
 * double result = ops.run('+', 10.0, 5.0);  // 15.0
 * ```
 *
 * @ingroup Utilidades
 */
template <typename Key, class Operation>
class AHDispatcher
{
  DynMapTree<Key, Operation> tbl;

public:

  /** @brief Default constructor. Creates an empty dispatcher. */
  AHDispatcher() = default;

  /**
   * @brief Variadic constructor for initializing with key-operation pairs.
   *
   * @param key First key to register.
   * @param op Operation associated with the key.
   * @param pairs Additional key-operation pairs.
   *
   * ## Example
   *
   * ```cpp
   * AHDispatcher<std::string, void(*)()> cmds(
   *   "init", init_func,
   *   "run", run_func,
   *   "stop", stop_func
   * );
   * ```
   */
  template <typename ... Pairs>
  AHDispatcher(const Key & key, Operation op, Pairs ... pairs)
    : AHDispatcher(pairs...)
  {
    tbl.insert(key, op);
  }

  /**
   * @brief Register a key-operation pair.
   *
   * If the key already exists, its operation is replaced.
   *
   * @param key The key to register.
   * @param op The operation to associate with the key.
   */
  void insert(const Key & key, Operation op)
  {
    tbl.insert(key, op);
  }

  /**
   * @brief Execute the operation associated with a key.
   *
   * @tparam Args Types of arguments to pass to the operation.
   * @param key The key identifying the operation.
   * @param args Arguments to forward to the operation.
   * @return The return value of the operation.
   *
   * @throws Aborts if the key is not registered.
   *
   * @note Use valid_key() to check before calling if unsure.
   */
  template <typename ... Args>
  auto run(const Key & key, Args && ... args) const
  {
    auto ptr = tbl.search(key);
    if (ptr == nullptr)
      {
        std::cerr << "Tried to run dispatcher with a key"
             << " which has not been registered" << std::endl;
        abort();
      }
    return (ptr->second)(std::forward<Args>(args)...);
  }

  /**
   * @brief Remove a key-operation pair.
   *
   * @param key The key to remove.
   */
  void remove(const Key & key)
  {
    tbl.remove(key);
  }

  /**
   * @brief Check if a key is registered.
   *
   * @param key The key to check.
   * @return true if the key exists, false otherwise.
   */
  bool valid_key(const Key & key) { return tbl.has(key); }

  /**
   * @brief Get all registered keys.
   *
   * @return A DynList containing all keys.
   */
  DynList<Key> keys() const { return tbl.keys(); }

  /**
   * @brief Get the number of registered operations.
   *
   * @return The number of key-operation pairs.
   */
  size_t size() const { return tbl.size(); }

  /**
   * @brief Check if the dispatcher is empty.
   *
   * @return true if no operations are registered.
   */
  bool is_empty() const { return tbl.is_empty(); }
};


/**
 * @brief Hash-based command dispatcher.
 *
 * Maps keys to operations using a hash table (ODhashTable).
 * Provides O(1) average lookup, insertion, and removal.
 *
 * @tparam Key The key type (must be hashable)
 * @tparam Operation The callable type
 *
 * Use this when:
 * - You have many operations and need fast lookup
 * - Keys are easily hashable (strings, integers)
 *
 * Use AHDispatcher instead when:
 * - You need ordered iteration over keys
 * - Key comparison is simpler than hashing
 *
 * @ingroup Utilidades
 */
template <typename Key, class Operation>
class AhHashDispatcher
{
  using P = std::pair<Key, Operation>;

  struct Equal
  {
    bool operator () (const P & p1, const P & p2) const
    {
      return p1.first == p2.first;
    }
  };

  static size_t fst_hash(const P & p) { return dft_hash_fct(p.first); }
  static size_t snd_hash(const P & p) { return snd_hash_fct(p.first); }

  ODhashTable<P, Equal> tbl;

public:

  /** @brief Default constructor. */
  AhHashDispatcher() 
    : tbl(Primes::DefaultPrime, fst_hash, snd_hash) {}

  /**
   * @brief Register a key-operation pair.
   *
   * @param key The key to register.
   * @param op The operation to associate with the key.
   */
  void insert(const Key & key, Operation op)
  {
    (void) tbl.insert(P(key, op));
  }

  /**
   * @brief Execute the operation associated with a key.
   *
   * @tparam Args Types of arguments to pass to the operation.
   * @param key The key identifying the operation.
   * @param args Arguments to forward to the operation.
   * @return The return value of the operation.
   *
   * @throws std::domain_error if the key is not found.
   */
  template <typename ... Args>
  auto run(const Key & key, Args ... args) const
  {
    P p; p.first = key;
    return tbl.find(p).second(args...);
  }

  /**
   * @brief Remove a key-operation pair.
   *
   * @param key The key to remove.
   */
  void remove(const Key & key)
  {
    P p; p.first = key;
    tbl.remove(p);
  }

  /**
   * @brief Check if a key is registered.
   *
   * @param key The key to check.
   * @return true if the key exists.
   */
  bool has(const Key & key) const
  {
    P p; p.first = key;
    return tbl.has(p);
  }
};

#endif // AH_DISPATCHER_H
