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
 *  This file provides abstract base classes for managing event tables in
 *  event-driven simulations and frameworks. Events are registered as function
 *  pointers and can be executed by index with arbitrary input/output.
 *
 *  ## Overview
 *
 *  Event tables map integer indices to event handler functions. Two
 *  implementations are provided:
 *  - **Static_Event_Table**: Fixed-size table (array-based)
 *  - **Dynamic_Event_Table**: Growable table (dynamic array-based)
 *
 *  ## Use Cases
 *
 *  - Event-driven simulations (discrete event simulation)
 *  - State machines with numbered states/transitions
 *  - Command dispatch tables
 *  - Plugin systems with dynamic event registration
 *
 *  @par Example
 *  @code
 *  // Define event handlers
 *  void* on_start(void* data) {
 *    std::cout << "Starting...\n";
 *    return nullptr;
 *  }
 *
 *  void* on_stop(void* data) {
 *    std::cout << "Stopping...\n";
 *    return nullptr;
 *  }
 *
 *  // Create event table
 *  Dynamic_Event_Table table;
 *  auto start_id = table.register_event(on_start);
 *  auto stop_id = table.register_event(on_stop);
 *
 *  // Execute events
 *  table.execute_event(start_id);
 *  table.execute_event(stop_id);
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

/** @brief Type alias for event handler functions.
 *
 *  Event functions take a `void*` input parameter and return a `void*` result.
 *  This allows arbitrary data to be passed to and returned from events.
 */
typedef void * (*Event_Fct)(void *);

/** @brief Abstract base class for event tables.
 *
 *  Provides interface for registering, unregistering, checking, and executing
 *  events. Concrete implementations must provide storage mechanism (static
 *  array or dynamic array).
 *
 *  ## Operations
 *
 *  - **register_event(index, fct)**: Register event at specific index
 *  - **register_event(fct)**: Append event and return auto-assigned index
 *  - **unregister_event(index)**: Remove event at index
 *  - **execute_event(index, input)**: Execute event with optional input
 *  - **check(index, fct)**: Verify event at index matches expected function
 *
 *  @par Thread Safety
 *  Not thread-safe. Use external synchronization if accessing from multiple threads.
 *
 *  @ingroup Utilities
 */
class Event_Table
{
  [[nodiscard]] virtual Event_Fct read_table(int i) const = 0;

  virtual void write_table(int i, Event_Fct event_fct) = 0;

  virtual void cut_table(size_t dim) = 0;

public:
  /** @brief Get the number of event slots in the table.
   *  @return Total number of slots (including unused ones)
   */
  virtual size_t size() const = 0;

  /** @brief Virtual destructor for proper cleanup in derived classes. */
  virtual ~Event_Table() = default;

private:
  /** @brief Append an event to the end of the table (internal use).
   *  @param fct Event function to append (must not be nullptr)
   */
  void append_event_to_table(Event_Fct fct)
  {
    assert(fct not_eq nullptr);

    write_table(size(), fct);
  }

protected:
  /** @brief Initialize table slots to nullptr (for use by derived classes).
   *  @param num_events Number of slots to initialize
   */
  void fill_table_to_null(const size_t num_events)
  {
    for (int i = 0; i < num_events; i++)
      write_table(i, nullptr);
  }

public:
  /** @brief Register an event at a specific index.
   *
   *  The index must be valid and must not already contain an event.
   *
   *  @param index Slot index where the event will be registered
   *  @param fct Event function to register (must not be nullptr)
   *  @throw domain_error if the index is out of range or already in use
   */
  void register_event(const size_t index, Event_Fct fct)
  {
    assert(fct not_eq nullptr);

    ah_domain_error_if(index >= size()) << "Index out of range";
    ah_domain_error_if(read_table(index) not_eq nullptr) << "Index is already used";

    write_table(index, fct);
  }

  /** @brief Append a new event and return its auto-assigned index.
   *
   *  The event is registered at the next available slot at the end of the table.
   *
   *  @param fct Event function to register (must not be nullptr)
   *  @return Index where the event was registered
   */
  size_t register_event(Event_Fct fct)
  {
    assert(fct not_eq nullptr);

    append_event_to_table(fct);

    return size() - 1;
  }

  /** @brief Unregister an event at the given index.
   *
   *  The slot is cleared (set to nullptr). If this is the last slot,
   *  the table may be shrunk.
   *
   *  @param index Slot index to clear
   *  @throw domain_error if index is out of range or slot is not in use
   */
  void unregister_event(const size_t index)
  {
    ah_domain_error_if(index >= size()) << "Index out of range";

    ah_domain_error_if(read_table(index) == nullptr) << "Index is not used";

    write_table(index, nullptr);

    if (index == size())
      cut_table(index - 1);
  }

  /** @brief Check if an event at the given index matches the expected function.
   *
   *  @param index Slot index to check
   *  @param fct Expected event function
   *  @return true if the index is valid and the registered function matches, false otherwise
   */
  bool check(const size_t index, Event_Fct fct) const
  {
    return index < size() and read_table(index) == fct;
  }

  /** @brief Execute the event at the given index.
   *
   *  Calls the registered event function with the provided input parameter.
   *
   *  @param index Slot index of event to execute
   *  @param input Optional input parameter to pass to event function (default: nullptr)
   *  @return Result returned by the event function
   *  @throw domain_error if index is not in use
   */
  void * execute_event(const size_t index, void *input = nullptr) const
  {
    if (read_table(index) == nullptr)
      ah_domain_error_if(true) << "Index is not used";

    return (*read_table(index))(input);
  }
};


/** @brief Fixed-size event table implementation.
 *
 *  Uses a statically-allocated array of function pointers. The size is
 *  determined at construction and cannot be changed afterward. Suitable
 *  when the number of possible events is known in advance.
 *
 *  ## Memory Management
 *
 *  - Owns the underlying array (allocated with `new[]`)
 *  - Non-copyable (would require deep copy)
 *  - Moveable (transfers ownership of array)
 *
 *  @par Complexity
 *  - Space: O(n) where n is table size
 *  - register_event: O(1)
 *  - unregister_event: O(1)
 *  - execute_event: O(1)
 *
 *  @ingroup Utilities
 */
class Static_Event_Table : public Event_Table
{
  Event_Fct *table; ///< Array of event function pointers
  size_t size_table; ///< Fixed size of the table

  /** @brief Verify index is within valid range.
   *  @param i Index to verify
   *  @throw range_error if the index is out of bounds
   */
  void verify_index(const int i) const
  {
    if (i < 0 or i >= size_table)
      ah_range_error_if(true) << "index out of range";
  }

  /// Read event at index (implements Event_Table virtual method)
  [[nodiscard]] Event_Fct read_table(int i) const override
  {
    verify_index(i);
    return table[i];
  }

  /// Write event at index (implements Event_Table virtual method)
  void write_table(const int i, Event_Fct event_fct) override
  {
    verify_index(i);
    table[i] = event_fct;
  }

  /// No-op for static table (size is fixed, cannot shrink)
  void cut_table(size_t) override
  { /* empty */
  }

public:
  /// Get table size (implements Event_Table virtual method)
  size_t size() const override { return size_table; }

  /** @brief Construct a static event table with fixed size.
   *
   *  All event slots are initialized to nullptr.
   *
   *  @param num_events Number of event slots to allocate (default: 0)
   */
  Static_Event_Table(const size_t num_events = 0)
    : table(new Event_Fct [num_events]), size_table(num_events)
  {
    fill_table_to_null(num_events);
  }

  /// Copying disabled (would require deep copy of array)
  Static_Event_Table(const Static_Event_Table &) = delete;

  Static_Event_Table &operator=(const Static_Event_Table &) = delete;

  /** @brief Move constructor - transfers ownership of array.
   *
   *  The moved-from object is left in a valid but empty state.
   *
   *  @param other Table to move from (will be emptied)
   */
  Static_Event_Table(Static_Event_Table && other) noexcept
    : table(other.table), size_table(other.size_table)
  {
    other.table = nullptr;
    other.size_table = 0;
  }

  /** @brief Move assignment - transfers ownership of the array.
   *
   *  Deletes the current array and takes ownership of other's array.
   *
   *  @param other Table to move from (will be emptied)
   *  @return Reference to this
   */
  Static_Event_Table &operator=(Static_Event_Table && other) noexcept
  {
    if (this != &other)
      {
        delete [] table;
        table = other.table;
        size_table = other.size_table;
        other.table = nullptr;
        other.size_table = 0;
      }
    return *this;
  }

  /** @brief Destructor - frees the underlying array. */
  ~Static_Event_Table() override
  {
    delete [] table;
  }
};


/** @brief Dynamic (growable) event table implementation.
 *
 *  Uses a dynamic array that can grow as events are added. Suitable when
 *  the number of events is not known in advance or when events are added
 *  dynamically at runtime.
 *
 *  ## Memory Management
 *
 *  - Backed by DynArray (automatic memory management)
 *  - Copyable and moveable (uses DynArray's semantics)
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
 *  Dynamic_Event_Table table;  // starts empty
 *
 *  // Events can be added dynamically
 *  auto id1 = table.register_event(handler1);
 *  auto id2 = table.register_event(handler2);
 *  auto id3 = table.register_event(handler3);
 *
 *  table.execute_event(id2);
 *  @endcode
 *
 *  @ingroup Utilities
 */
class Dynamic_Event_Table : public Event_Table
{
  DynArray<Event_Fct> table; ///< Dynamic array of event function pointers

  /// Read event at index (implements Event_Table virtual method)
  Event_Fct read_table(const int i) const override { return table[i]; }

  /// Write event at index (implements Event_Table virtual method)
  void write_table(const int i, Event_Fct event_fct) override
  {
    table[i] = event_fct;
  }

  /// Shrink table to given size (implements Event_Table virtual method)
  void cut_table(const size_t dim) override { table.cut(dim); }

public:
  /// Get the current table size (implements Event_Table virtual method)
  size_t size() const override { return table.size(); }

  /** @brief Construct a dynamic event table.
   *
   *  @param num_events Initial number of event slots (default: 0)
   *
   *  All initial slots are initialized to nullptr. The table can grow
   *  beyond this initial size as events are added.
   */
  Dynamic_Event_Table(size_t num_events = 0) : table(num_events)
  {
    fill_table_to_null(num_events);
  }
};


# endif
