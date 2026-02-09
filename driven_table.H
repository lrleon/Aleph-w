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


/** @file driven_table.H
 *  @brief Event-driven table abstraction for event-driven simulations.
 *
 *  This file provides type-safe event tables for managing event handlers in
 *  event-driven simulations and frameworks. Events can be registered as function
 *  pointers, lambdas, functors, or std::function objects, with compile-time
 *  type safety for arguments and return values.
 *
 *  ## Overview
 *
 *  Event tables map integer indices to event handler callables. Two
 *  implementations are provided:
 *  - **Static_Event_Table**: Fixed-size table (array-based)
 *  - **Dynamic_Event_Table**: Growable table (dynamic array-based)
 *
 *  Both implementations are templated on the event signature, allowing
 *  type-safe event handlers with arbitrary arguments and return types.
 *
 *  ## Use Cases
 *
 *  - Event-driven simulations (discrete event simulation)
 *  - State machines with numbered states/transitions
 *  - Command dispatch tables
 *  - Plugin systems with dynamic event registration
 *
 *  @par Modern Example (with lambdas and type safety)
 *  @code
 *  // Define type-safe event table for events that take an int and return void
 *  Dynamic_Event_Table<void(int)> table;
 *
 *  // Register lambda
 *  auto start_id = table.register_event([](int x) {
 *    std::cout << "Starting with " << x << "\n";
 *  });
 *
 *  // Register function
 *  auto stop_id = table.register_event([](int x) {
 *    std::cout << "Stopping with " << x << "\n";
 *  });
 *
 *  // Execute events with type-safe arguments
 *  table.execute_event(start_id, 42);
 *  table.execute_event(stop_id, 100);
 *  @endcode
 *
 *  @par Legacy Example (backward compatible)
 *  @code
 *  // Old style still works for compatibility
 *  void* on_start(void* data) {
 *    std::cout << "Starting...\n";
 *    return nullptr;
 *  }
 *
 *  Legacy_Event_Table table;  // or Dynamic_Event_Table<void*(void*)>
 *  auto id = table.register_event(on_start);
 *  table.execute_event(id, nullptr);
 *  @endcode
 *
 *  @ingroup Utilities
 *  @author Leandro Rabindranath León
 */

# ifndef DRIVEN_TABLE_H
# define DRIVEN_TABLE_H

# include <aleph.H>
# include <tpl_dynArray.H>
# include <ah-errors.H>
# include <functional>
# include <memory>
# include <type_traits>

/** @brief Type alias for legacy event handler functions.
 *
 *  Event functions take a `void*` input parameter and return a `void*` result.
 *  This allows arbitrary data to be passed to and returned from events.
 *
 *  @deprecated Use templated Event_Table with std::function instead for type safety.
 */
typedef void * (*Event_Fct)(void *);

/** @brief Abstract base class for type-safe event tables.
 *
 *  Provides an interface for registering, unregistering, checking, and executing
 *  events with compile-time type safety. Concrete implementations must provide
 *  a storage mechanism (static array or dynamic array).
 *
 *  @tparam Signature Function signature for events (e.g., void(int), int(double, bool))
 *
 *  ## Operations
 *
 *  - **register_event(index, fct)**: Register event at specific index
 *  - **register_event(fct)**: Append event and return auto-assigned index
 *  - **unregister_event(index)**: Remove event at index
 *  - **execute_event(index, args...)**: Execute event with type-safe arguments
 *  - **is_registered(index)**: Check if an event is registered at index
 *
 *  @par Thread Safety
 *  Not thread-safe. Use external synchronization if accessing from multiple threads.
 *
 *  @ingroup Utilities
 */
template <typename Signature>
class Event_Table
{
public:
  using Event_Handler = std::function<Signature>;

protected:
  [[nodiscard]] virtual const Event_Handler* read_table(size_t i) const = 0;

  virtual void write_table(size_t i, Event_Handler&& event_fct) = 0;

  virtual void clear_slot(size_t i) = 0;

public:
  /** @brief Get the number of event slots in the table.
   *  @return Total number of slots (including unused ones)
   */
  [[nodiscard]] virtual size_t size() const = 0;

  /** @brief Virtual destructor for proper cleanup in derived classes. */
  virtual ~Event_Table() = default;

private:
  /** @brief Append an event to the end of the table (internal use).
   *  @param fct Event handler to append
   */
  void append_event_to_table(Event_Handler&& fct)
  {
    write_table(size(), std::move(fct));
  }

public:
  /** @brief Register an event at a specific index.
   *
   *  The index must be valid and must not already contain an event.
   *
   *  @tparam Callable Any callable compatible with Signature (lambda, function, functor)
   *  @param index Slot index where the event will be registered
   *  @param fct Event handler to register
   *  @throw range_error if the index is out of range or already in use
   */
  template <typename Callable>
  void register_event(const size_t index, Callable&& fct)
  {
    ah_range_error_if(index >= size()) << "Index out of range";
    ah_range_error_if(read_table(index) != nullptr) << "Index is already used";

    write_table(index, Event_Handler(std::forward<Callable>(fct)));
  }

  /** @brief Append a new event and return its auto-assigned index.
   *
   *  The event is registered at the next available slot at the end of the table.
   *
   *  @tparam Callable Any callable compatible with Signature (lambda, function, functor)
   *  @param fct Event handler to register
   *  @return Index where the event was registered
   */
  template <typename Callable>
  size_t register_event(Callable&& fct)
  {
    append_event_to_table(Event_Handler(std::forward<Callable>(fct)));
    return size() - 1;
  }

  /** @brief Unregister an event at the given index.
   *
   *  The slot is cleared and can be reused later.
   *
   *  @param index Slot index to clear
   *  @throw range_error if the index is out of range or slot is not in use
   */
  void unregister_event(const size_t index)
  {
    ah_range_error_if(index >= size()) << "Index out of range";
    ah_range_error_if(read_table(index) == nullptr) << "Index is not used";

    clear_slot(index);
  }

  /** @brief Check if an event is registered at the given index.
   *
   *  @param index Slot index to check
   *  @return true if an event is registered at the index, false otherwise
   */
  bool is_registered(const size_t index) const
  {
    return index < size() and read_table(index) != nullptr;
  }

  /** @brief Legacy check method (for backward compatibility).
   *
   *  For templated event tables, this always returns false since we cannot
   *  compare std::function objects. Use is_registered() instead.
   *
   *  @deprecated Use is_registered() instead
   */
  template <typename F>
  bool check(const size_t, F) const
  {
    return false;
  }

  /** @brief Execute the event at the given index with type-safe arguments.
   *
   *  Calls the registered event handler with the provided arguments.
   *
   *  @tparam Args Argument types (automatically deduced)
   *  @param index Slot index of event to execute
   *  @param args Arguments to pass to the event handler
   *  @return Result returned by the event handler
   *  @throw range_error if index is not in use
   */
  template <typename... Args>
  auto execute_event(const size_t index, Args&&... args) const
  {
    ah_range_error_if(index >= size())
      << "index " << index << " out of range [0, " << size() << ')';

    const auto* event_fct = read_table(index);

    ah_range_error_if(event_fct == nullptr) << "Index is not used";

    return (*event_fct)(std::forward<Args>(args)...);
  }
};


/** @brief Specialization of Event_Table for legacy void*(void*) signature.
 *
 *  Provides backward compatibility with the old Event_Table interface.
 */
template <>
class Event_Table<void*(void*)>
{
public:
  using Event_Handler = std::function<void*(void*)>;

protected:
  [[nodiscard]] virtual const Event_Handler* read_table(size_t i) const = 0;

  virtual void write_table(size_t i, Event_Handler&& event_fct) = 0;

  virtual void clear_slot(size_t i) = 0;

public:
  [[nodiscard]] virtual size_t size() const = 0;

  virtual ~Event_Table() = default;

private:
  void append_event_to_table(Event_Handler&& fct)
  {
    write_table(size(), std::move(fct));
  }

public:
  template <typename Callable>
  void register_event(const size_t index, Callable&& fct)
  {
    ah_range_error_if(index >= size()) << "Index out of range";
    ah_range_error_if(read_table(index) != nullptr) << "Index is already used";

    write_table(index, Event_Handler(std::forward<Callable>(fct)));
  }

  template <typename Callable>
  size_t register_event(Callable&& fct)
  {
    append_event_to_table(Event_Handler(std::forward<Callable>(fct)));
    return size() - 1;
  }

  void unregister_event(const size_t index)
  {
    ah_range_error_if(index >= size()) << "Index out of range";
    ah_range_error_if(read_table(index) == nullptr) << "Index is not used";

    clear_slot(index);
  }

  bool is_registered(const size_t index) const
  {
    return index < size() and read_table(index) != nullptr;
  }

  // Legacy check for void* function pointers - compares function addresses
  bool check(const size_t index, void* (*fct)(void*)) const
  {
    if (index >= size() || !is_registered(index))
      return false;

    const auto* handler = read_table(index);
    if (!handler)
      return false;

    // Get the target function pointer from std::function
    auto target = handler->template target<void* (*)(void*)>();
    return target && *target == fct;
  }

  // Special override for void*(void*) with default nullptr argument
  void* execute_event(const size_t index, void* input = nullptr) const
  {
    ah_range_error_if(index >= size())
      << "index " << index << " out of range [0, " << size() << ')';

    const auto* event_fct = read_table(index);

    ah_range_error_if(event_fct == nullptr) << "Index is not used";

    return (*event_fct)(input);
  }
};


/** @brief Fixed-size event table implementation.
 *
 *  Uses a fixed-size array of event handlers. The size is determined at
 *  construction and cannot be changed afterward. Suitable when the number
 *  of possible events is known in advance.
 *
 *  @warning The auto-append overload `register_event(fct)` (without an
 *  explicit index) is **not supported** on static tables and will always
 *  throw `range_error`.  Use `register_event(index, fct)` instead.
 *
 *  @tparam Signature Function signature for events (e.g., void(int), int(double, bool))
 *
 *  ## Memory Management
 *
 *  - Owns the underlying array (managed by std::unique_ptr)
 *  - Non-copyable (events may contain non-copyable captures)
 *  - Moveable (transfers ownership of the array)
 *
 *  @par Complexity
 *  - Space: O(n) where n is table size
 *  - register_event: O(1)
 *  - unregister_event: O(1)
 *  - execute_event: O(1)
 *
 *  @ingroup Utilities
 */
template <typename Signature>
class Static_Event_Table : public Event_Table<Signature>
{
public:
  using Event_Handler = typename Event_Table<Signature>::Event_Handler;

private:
  std::unique_ptr<Event_Handler[]> table; ///< Array of event handlers
  size_t size_table; ///< Fixed size of the table

  /** @brief Verify index is within valid range.
   *  @param i Index to verify
   *  @throw range_error if the index is out of bounds
   */
  void verify_index(const size_t i) const
  {
    ah_range_error_if(i >= size_table) << "index out of range";
  }

  /// Read event at index (implements Event_Table virtual method)
  [[nodiscard]] const Event_Handler* read_table(const size_t i) const override
  {
    verify_index(i);
    return table[i] ? &table[i] : nullptr;
  }

  /// Write event at index (implements Event_Table virtual method)
  void write_table(const size_t i, Event_Handler&& event_fct) override
  {
    verify_index(i);
    table[i] = std::move(event_fct);
  }

  /// Clear event at index (implements Event_Table virtual method)
  void clear_slot(size_t i) override
  {
    verify_index(i);
    table[i] = Event_Handler();
  }

public:
  /// Get table size (implements Event_Table virtual method)
  [[nodiscard]] size_t size() const override { return size_table; }

  /** @brief Construct a static event table with a fixed size.
   *
   *  All event slots are initialized to empty.
   *
   *  @param num_events Number of event slots to allocate (default: 0)
   */
  explicit Static_Event_Table(const size_t num_events = 0)
    : table(num_events > 0 ? new Event_Handler[num_events] : nullptr),
      size_table(num_events)
  {
  }

  /// Copying disabled (events may contain non-copyable captures)
  Static_Event_Table(const Static_Event_Table&) = delete;

  Static_Event_Table& operator=(const Static_Event_Table&) = delete;

  /** @brief Move constructor - transfers ownership of the array.
   *
   *  The moved-from object is left in a valid but empty state.
   *
   *  @param other Table to move from (will be emptied)
   */
  Static_Event_Table(Static_Event_Table&& other) noexcept
    : table(std::move(other.table)), size_table(other.size_table)
  {
    other.size_table = 0;
  }

  /** @brief Move assignment - transfers ownership of the array.
   *
   *  Deletes the current array and takes ownership of other's array.
   *
   *  @param other Table to move from (will be emptied)
   *  @return Reference to this
   */
  Static_Event_Table& operator=(Static_Event_Table&& other) noexcept
  {
    if (this != &other)
      {
        table = std::move(other.table);
        size_table = other.size_table;
        other.size_table = 0;
      }
    return *this;
  }

  /** @brief Destructor - automatically frees the underlying array. */
  ~Static_Event_Table() override = default;
};


/** @brief Dynamic (growable) event table implementation.
 *
 *  Uses a dynamic array that can grow as events are added. Suitable when
 *  the number of events is not known in advance or when events are added
 *  dynamically at runtime.
 *
 *  @tparam Signature Function signature for events (e.g., void(int), int(double, bool))
 *
 *  ## Memory Management
 *
 *  - Backed by DynArray (automatic memory management)
 *  - Moveable (events may contain non-copyable captures)
 *  - Automatically grows when events are appended beyond current size
 *
 *  ## Growth Strategy
 *
 *  When appending beyond current size, the underlying DynArray grows
 *  according to its own growth strategy (typically doubling capacity).
 *
 *  @par Complexity
 *  - Space: O(n) where n is current size
 *  - register_event (at end): O(1) amortized
 *  - register_event (at index): O(1)
 *  - unregister_event: O(1)
 *  - execute_event: O(1)
 *
 *  @par Example
 *  @code
 *  Dynamic_Event_Table<void(int)> table;  // starts empty
 *
 *  // Events can be added dynamically (lambdas, functions, etc.)
 *  auto id1 = table.register_event([](int x) { std::cout << x << "\n"; });
 *  auto id2 = table.register_event([](int x) { std::cout << x * 2 << "\n"; });
 *
 *  table.execute_event(id1, 42);  // prints 42
 *  table.execute_event(id2, 21);  // prints 42
 *  @endcode
 *
 *  @ingroup Utilities
 */
template <typename Signature>
class Dynamic_Event_Table : public Event_Table<Signature>
{
public:
  using Event_Handler = typename Event_Table<Signature>::Event_Handler;

private:
  DynArray<Event_Handler> table; ///< Dynamic array of event handlers

  /// Read event at index (implements Event_Table virtual method)
  [[nodiscard]] const Event_Handler* read_table(const size_t i) const override
  {
    const Event_Handler& handler = table[i];
    return handler ? &handler : nullptr;
  }

  /// Write event at index (implements Event_Table virtual method)
  void write_table(const size_t i, Event_Handler&& event_fct) override
  {
    table[i] = std::move(event_fct);
  }

  /// Clear event at index (implements Event_Table virtual method)
  void clear_slot(size_t i) override
  {
    table[i] = Event_Handler();
  }

public:
  /// Get the current table size (implements Event_Table virtual method)
  size_t size() const override { return table.size(); }

  /** @brief Construct a dynamic event table.
   *
   *  @param num_events Initial number of event slots (default: 0)
   *
   *  All initial slots are initialized to empty. The table can grow
   *  beyond this initial size as events are added.
   */
  explicit Dynamic_Event_Table(const size_t num_events = 0) : table(num_events)
  {
    // Initialize all slots with empty handlers to avoid DynArray access exceptions
    for (size_t i = 0; i < num_events; ++i)
      table[i] = Event_Handler();
  }

  /// Copying disabled (events may contain non-copyable captures)
  Dynamic_Event_Table(const Dynamic_Event_Table&) = delete;

  Dynamic_Event_Table& operator=(const Dynamic_Event_Table&) = delete;

  /// Move constructor
  Dynamic_Event_Table(Dynamic_Event_Table&&) noexcept = default;

  /// Move assignment
  Dynamic_Event_Table& operator=(Dynamic_Event_Table&&) noexcept = default;

  /// Default destructor
  ~Dynamic_Event_Table() override = default;
};


// =============================================================================
// Legacy Type Aliases for Backward Compatibility
// =============================================================================

/** @brief Legacy event table type (uses void* for input/output).
 *
 *  @deprecated Use templated Event_Table<Signature> for type safety.
 *
 *  This alias maintains backward compatibility with code using the old
 *  Event_Table interface with void* event handlers.
 */
using Legacy_Static_Event_Table = Static_Event_Table<void*(void*)>;

/** @brief Legacy dynamic event table type (uses void* for input/output).
 *
 *  @deprecated Use templated Dynamic_Event_Table<Signature> for type safety.
 *
 *  This alias maintains backward compatibility with code using the old
 *  Dynamic_Event_Table interface with void* event handlers.
 */
using Legacy_Dynamic_Event_Table = Dynamic_Event_Table<void*(void*)>;


# endif
