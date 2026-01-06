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

/** @file pointer_table.H
 *  @brief Pointer table with index reuse for efficient pointer management.
 *
 *  This file provides `Pointer_Table`, a data structure that manages a
 *  dynamic collection of void pointers with automatic index recycling.
 *  When pointers are removed, their indices are tracked and reused for
 *  subsequent insertions, minimizing memory fragmentation and index growth.
 *
 *  @section pt_features Main Features
 *  - O(1) insertion with automatic index assignment
 *  - O(1) removal with index recycling
 *  - O(1) pointer verification by index
 *  - Automatic compaction when removing from the top of the heap
 *  - Threshold-based memory management to prevent excessive shrinking
 *
 *  @section pt_usage Usage Example
 *  @code
 *  Pointer_Table table(100);  // Initial capacity of 100
 *
 *  int* p1 = new int(42);
 *  int* p2 = new int(100);
 *
 *  long idx1 = table.insert_pointer(p1);  // Returns 0
 *  long idx2 = table.insert_pointer(p2);  // Returns 1
 *
 *  table.remove_pointer(idx1);  // Index 0 now available for reuse
 *
 *  int* p3 = new int(200);
 *  long idx3 = table.insert_pointer(p3);  // Returns 0 (reused)
 *
 *  // Verify a pointer matches its index
 *  void* verified = table.verify_pointer(idx2, p2);  // Returns p2
 *  @endcode
 *
 *  @section pt_design Design
 *  The table uses a two-array approach:
 *  - `pointer_table`: Stores the actual pointers indexed by position
 *  - `free_table`: Stack of available indices for reuse
 *
 *  A `heap_index` tracks the "high water mark" - all indices below it
 *  have been used at some point. When removing the top pointer, the
 *  heap contracts automatically, releasing memory.
 *
 *  @ingroup Utilidades
 *  @author Leandro Rabindranath Leon
 */

# ifndef POINTER_TABLE_H
# define POINTER_TABLE_H

# include <tpl_dynArray.H>
# include <ah-errors.H>
# include <iostream>

/** @class Pointer_Table
 *  @brief A dynamic table for managing void pointers with index recycling.
 *
 *  `Pointer_Table` provides efficient storage and retrieval of void pointers
 *  using integer indices. When pointers are removed, their indices are
 *  recycled for future insertions, preventing unbounded index growth.
 *
 *  The data structure maintains two internal arrays:
 *  - A pointer array that maps indices to pointers
 *  - A free list that tracks available (recyclable) indices
 *
 *  @par Thread Safety
 *  This class is NOT thread-safe. External synchronization is required
 *  for concurrent access.
 *
 *  @par Exception Safety
 *  - `insert_pointer()`: Strong guarantee (no state change on failure)
 *  - `remove_pointer()`: Strong guarantee (throws on invalid index)
 *  - `verify_pointer()`: Strong guarantee (throws on mismatch)
 *
 *  @ingroup Utilidades
 *  @author Leandro Rabindranath León
 */
class Pointer_Table
{
public:
  /** @brief Sentinel value indicating an invalid or null index.
   *
   *  This constant is used internally to indicate that no valid
   *  index is available (e.g., when the free table is empty).
   */
  static constexpr long Null_Index = -1;

private:
  DynArray<void *> pointer_table;  ///< Array mapping indices to pointers
  DynArray<long> free_table;       ///< Stack of available indices for reuse
  long num_pointers;               ///< Count of currently stored pointers
  long heap_index;                 ///< Next available index at the heap top
  long threshold_size;             ///< Minimum size to maintain (prevents excessive shrinking)

  /** @brief Allocates an index from the free table.
   *
   *  Attempts to reuse a previously freed index from the free table.
   *  This supports index recycling to minimize index growth.
   *
   *  @return A recyclable index, or `Null_Index` if none available.
   *
   *  @par Complexity
   *  O(1) amortized.
   */
  long allocate_above_heap()
  {
    if (free_table.size() == 0)
      return Null_Index;

    long ret_val = free_table[free_table.size() - 1];

    assert(ret_val >= 0 and ret_val < heap_index);
    assert(pointer_table[ret_val] == nullptr);

    free_table.cut(free_table.size() - 1);

    return ret_val;
  }

  /** @brief Adds an index to the free table for future reuse.
   *
   *  Marks the given index as available by setting its pointer to nullptr
   *  and adding it to the free table stack.
   *
   *  @param[in] i The index to mark as free. Must be valid and in use.
   *
   *  @pre `0 <= i < heap_index`
   *  @pre `pointer_table[i] != nullptr` (index must be in use)
   *
   *  @par Complexity
   *  O(1) amortized.
   */
  void insert_in_free_table(long i)
  {
    assert(i >= 0 and i < heap_index);

    pointer_table[i] = nullptr;
    free_table[free_table.size()] = i;
  }

  /** @brief Removes indices from free_table that are >= new_heap_index.
   *
   *  When the heap contracts, indices that were in the free table but
   *  are now beyond the heap boundary must be removed. This maintains
   *  the invariant that all free_table indices are valid.
   *
   *  @param[in] new_heap_index The new heap boundary.
   *
   *  @par Complexity
   *  O(n) where n is the size of free_table.
   */
  void cleanup_free_table(long new_heap_index)
  {
    if (free_table.size() == 0)
      return;

    // Remove all indices >= new_heap_index from free_table
    // We need to compact the free_table, keeping only valid indices
    long write_pos = 0;
    for (long read_pos = 0; read_pos < static_cast<long>(free_table.size()); ++read_pos)
      {
        if (free_table[read_pos] < new_heap_index)
          {
            if (write_pos != read_pos)
              free_table[write_pos] = free_table[read_pos];
            ++write_pos;
          }
      }

    if (write_pos < static_cast<long>(free_table.size()))
      free_table.cut(write_pos);
  }

  /** @brief Checks if an index is within valid bounds.
   *
   *  An index is valid if it is non-negative and less than the
   *  current heap index (the high water mark).
   *
   *  @param[in] i The index to validate.
   *  @return `true` if the index is within bounds, `false` otherwise.
   *
   *  @par Complexity
   *  O(1).
   */
  bool is_valid_index(long i) const noexcept
  {
    return i >= 0 and i < heap_index;
  }

  /** @brief Checks if a pointer matches the one stored at an index.
   *
   *  @param[in] i   The index to check. Must be valid.
   *  @param[in] ptr The pointer to compare against.
   *  @return `true` if the stored pointer equals `ptr`, `false` otherwise.
   *
   *  @pre `is_valid_index(i)` must be true.
   *
   *  @par Complexity
   *  O(1).
   */
  bool pointer_matches_with_index(long i, const void *ptr) const noexcept
  {
    assert(is_valid_index(i));
    return pointer_table(i) == ptr;
  }

  /** @brief Validates the internal consistency of the data structure.
   *
   *  Checks that all invariants hold:
   *  - If empty: heap_index within threshold, free_table empty
   *  - If not empty: top of heap contains a non-null pointer
   *  - All free_table indices are valid and point to nullptr
   *  - num_pointers equals (heap_index - free_table.size())
   *
   *  @return `true` if all invariants hold, `false` otherwise.
   *
   *  @note This method is intended for debugging and assertions.
   *        It has O(n) complexity due to free_table validation.
   */
  bool invariant() const
  {
    // Basic count check
    if (num_pointers < 0)
      return false;

    // When empty
    if (num_pointers == 0)
      return heap_index == 0 and free_table.size() == 0;

    // Non-empty: top of heap must be non-null
    if (heap_index <= 0 or pointer_table(heap_index - 1) == nullptr)
      return false;

    // Verify count consistency
    if (num_pointers != heap_index - static_cast<long>(free_table.size()))
      return false;

    // Verify all free_table indices are valid and point to nullptr
    for (size_t j = 0; j < free_table.size(); ++j)
      {
        long idx = free_table(j);
        if (idx < 0 or idx >= heap_index)
          return false;
        if (pointer_table(idx) != nullptr)
          return false;
      }

    return true;
  }

public:
  /** @brief Constructs a Pointer_Table with optional initial capacity.
   *
   *  Creates an empty pointer table with the specified initial capacity.
   *  The `initial_size` sets the threshold below which the table will
   *  not shrink, preventing excessive memory reallocation for tables
   *  that frequently grow and shrink.
   *
   *  @param[in] initial_size Initial capacity and shrink threshold.
   *                          Defaults to 0 (fully dynamic).
   *
   *  @par Complexity
   *  O(initial_size) for initialization.
   *
   *  @par Example
   *  @code
   *  Pointer_Table small_table;        // Fully dynamic
   *  Pointer_Table large_table(1000);  // Pre-allocated, won't shrink below 1000
   *  @endcode
   */
  Pointer_Table(size_t initial_size = 0)
    : pointer_table(initial_size), free_table(0),
      num_pointers(0), heap_index(0), threshold_size(static_cast<long>(initial_size))
  {
    // DynArray initializes to default (nullptr for pointers)
    // No explicit initialization loop needed
  }

  /** @brief Returns the current capacity of the pointer table.
   *
   *  This is the size of the underlying array, not the number
   *  of pointers currently stored.
   *
   *  @return The current capacity (allocated size).
   *
   *  @see busies() for the number of stored pointers.
   *
   *  @par Complexity
   *  O(1).
   */
  long size() const noexcept { return pointer_table.size(); }

  /** @brief Returns the number of pointers currently stored.
   *
   *  @return The count of non-null pointer entries.
   *
   *  @par Complexity
   *  O(1).
   */
  long busies() const noexcept { return num_pointers; }

  /** @brief Returns the number of recyclable indices.
   *
   *  These are indices that were previously used but are now
   *  available for reuse via `insert_pointer()`.
   *
   *  @return The count of free (recyclable) indices.
   *
   *  @par Complexity
   *  O(1).
   */
  long frees() const noexcept { return free_table.size(); }

  /** @brief Returns the current heap index (high water mark).
   *
   *  The heap index represents the next available index at the
   *  top of the allocation. All valid indices are in [0, heap_index).
   *
   *  @return The current heap index.
   *
   *  @par Complexity
   *  O(1).
   */
  long get_heap_index() const noexcept { return heap_index; }

  /** @brief Returns the threshold size.
   *
   *  The threshold is the minimum size the table maintains.
   *  The table will not shrink below this size.
   *
   *  @return The threshold size.
   *
   *  @par Complexity
   *  O(1).
   */
  long get_threshold() const noexcept { return threshold_size; }

  /** @brief Checks if the table is empty.
   *
   *  @return `true` if no pointers are stored, `false` otherwise.
   *
   *  @par Complexity
   *  O(1).
   */
  bool is_empty() const noexcept { return num_pointers == 0; }

  /** @brief Inserts a pointer and returns its assigned index.
   *
   *  Adds the given pointer to the table, either reusing a previously
   *  freed index or allocating a new one at the heap top.
   *
   *  @param[in] ptr The pointer to insert. May be nullptr (though
   *                 this is discouraged as it complicates removal).
   *
   *  @return The index assigned to the pointer. Use this index
   *          for subsequent `remove_pointer()` or `verify_pointer()` calls.
   *
   *  @warning Inserting nullptr is allowed but discouraged. It makes
   *           it impossible to distinguish between a free slot and
   *           an intentionally stored nullptr.
   *
   *  @par Complexity
   *  O(1) amortized.
   *
   *  @par Example
   *  @code
   *  Pointer_Table table;
   *  int* p = new int(42);
   *  long idx = table.insert_pointer(p);
   *  // idx can now be used to retrieve or remove p
   *  @endcode
   */
  long insert_pointer(void *ptr)
  {
    assert(invariant());

    long ret_val = allocate_above_heap();

    if (ret_val == Null_Index)
      ret_val = heap_index++;

    pointer_table[ret_val] = ptr;
    ++num_pointers;

    assert(invariant());

    return ret_val;
  }

  /** @brief Removes the pointer at the given index.
   *
   *  Removes the pointer at index `i` from the table. The index
   *  becomes available for reuse by subsequent `insert_pointer()` calls.
   *
   *  If the removed index is at the top of the heap, the heap contracts
   *  automatically, potentially freeing memory.
   *
   *  @param[in] i The index of the pointer to remove.
   *
   *  @throws std::range_error If `i` is out of valid range.
   *  @throws std::domain_error If index `i` is already free (nullptr).
   *
   *  @par Complexity
   *  O(1) amortized for typical removals. O(n) worst case when
   *  removing from heap top triggers cleanup of free_table.
   *
   *  @par Example
   *  @code
   *  Pointer_Table table;
   *  int* p = new int(42);
   *  long idx = table.insert_pointer(p);
   *
   *  delete p;
   *  table.remove_pointer(idx);  // Index now available for reuse
   *  @endcode
   */
  void remove_pointer(long i)
  {
    assert(invariant());

    if (not is_valid_index(i))
      ah_range_error_if(true) << "index " << i << " out of range [0, "
                              << heap_index << ")";

    if (pointer_table[i] == nullptr)
      ah_domain_error_if(true) << "index " << i << " is not busy (already free)";

    if (i == heap_index - 1)
      {
        // Removing from top of heap - contract the heap
        pointer_table[heap_index - 1] = nullptr;

        // Find new heap top (first non-null from the top)
        while (heap_index > 0 and pointer_table[heap_index - 1] == nullptr)
          --heap_index;

        // CRITICAL FIX: Remove invalid indices from free_table
        cleanup_free_table(heap_index);
      }
    else
      insert_in_free_table(i);

    // Shrink pointer_table if below threshold
    if (heap_index <= threshold_size and pointer_table.size() > static_cast<size_t>(threshold_size))
      pointer_table.cut(threshold_size > 0 ? threshold_size : 0);

    --num_pointers;

    assert(invariant());
  }

  /** @brief Retrieves the pointer at the given index.
   *
   *  Returns the pointer stored at index `i` without any validation.
   *  Use `verify_pointer()` if you need to confirm the pointer matches
   *  an expected value.
   *
   *  @param[in] i The index to retrieve.
   *
   *  @return The pointer at index `i`, or nullptr if the slot is free.
   *
   *  @throws std::range_error If `i` is out of valid range.
   *
   *  @par Complexity
   *  O(1).
   */
  void * get_pointer(long i) const
  {
    if (not is_valid_index(i))
      ah_range_error_if(true) << "index " << i << " out of range [0, "
                              << heap_index << ")";
    return pointer_table(i);
  }

  /** @brief Verifies that a pointer matches the one stored at an index.
   *
   *  This method is useful for validating that a cached index still
   *  refers to the expected pointer, detecting use-after-free or
   *  index reuse scenarios.
   *
   *  @param[in] i   The index to verify.
   *  @param[in] ptr The expected pointer value.
   *
   *  @return The pointer if verification succeeds (same as `ptr`).
   *
   *  @throws std::range_error If `i` is out of valid range.
   *  @throws std::domain_error If the stored pointer doesn't match `ptr`.
   *
   *  @par Complexity
   *  O(1).
   *
   *  @par Example
   *  @code
   *  Pointer_Table table;
   *  int* p = new int(42);
   *  long idx = table.insert_pointer(p);
   *
   *  // Later, verify the pointer is still valid
   *  try {
   *    void* verified = table.verify_pointer(idx, p);
   *    // Safe to use verified
   *  } catch (const std::domain_error& e) {
   *    // Pointer was replaced or removed
   *  }
   *  @endcode
   */
  void * verify_pointer(long i, void *ptr) const
  {
    if (not is_valid_index(i))
      ah_range_error_if(true) << "index " << i << " out of range [0, "
                              << heap_index << ")";

    if (not pointer_matches_with_index(i, ptr))
      ah_domain_error_if(true) << "pointer at index " << i
                               << " does not match expected value";

    return ptr;
  }

  /** @brief Clears all pointers from the table.
   *
   *  Removes all pointers and resets the table to its initial state.
   *  Does NOT delete the pointed-to objects - that is the caller's
   *  responsibility.
   *
   *  @par Complexity
   *  O(n) where n is the current capacity.
   *
   *  @par Example
   *  @code
   *  Pointer_Table table;
   *  // ... insert many pointers ...
   *
   *  // Clear without deleting pointed objects
   *  table.clear();
   *
   *  assert(table.is_empty());
   *  @endcode
   */
  void clear()
  {
    pointer_table.cut(threshold_size > 0 ? threshold_size : 0);
    free_table.cut(0);
    num_pointers = 0;
    heap_index = 0;

    assert(invariant());
  }

# ifdef DEBUG
  /** @brief Prints internal state for debugging.
   *
   *  Outputs the current values of all internal variables to stdout.
   *  Only available when DEBUG is defined.
   *
   *  @par Example Output
   *  @code
   *  Number of pointers = 5
   *  Pointer table size = 10
   *  Free table size    = 2
   *  Threshold          = 10
   *  Heap index         = 7
   *  pointer_table[6]   = 0x7fff5fbff8c0
   *  @endcode
   */
  void print_parameters() const
  {
    std::cout << "Number of pointers = " << num_pointers << '\n'
              << "Pointer table size = " << pointer_table.size() << '\n'
              << "Free table size    = " << free_table.size() << '\n'
              << "Threshold          = " << threshold_size << '\n'
              << "Heap index         = " << heap_index << '\n';
    if (heap_index > 0)
      std::cout << "pointer_table[" << heap_index - 1 << "]= "
                << pointer_table(heap_index - 1) << '\n';
  }

  /** @brief Validates the internal invariant (debug only).
   *
   *  @return `true` if all invariants hold, `false` otherwise.
   */
  bool check_invariant() const { return invariant(); }
# endif
};


# endif // POINTER_TABLE_H