/**
 * @file array_utils_example.cc
 * @brief Educational examples for array utility functions
 * 
 * WHAT ARE ARRAY UTILITIES?
 * ==========================
 * Low-level functions for efficient array manipulation including:
 * - Gap operations (insert/delete space)
 * - Reversing and rotation
 * - In-place transformations
 * 
 * WHY USE THESE UTILITIES?
 * ========================
 * - O(n) performance for common operations
 * - In-place algorithms (minimal memory overhead)
 * - Foundation for data structure implementations
 * - Used internally by DynArray, DynList, etc.
 * 
 * COMPILE & RUN:
 * ==============
 * g++ -std=c++20 -I.. -o array_utils_example array_utils_example.cc
 * ./array_utils_example
 */

#include <iostream>
#include <array_utils.H>
#include <tpl_dynArray.H>

using namespace std;
using namespace Aleph;

int main()
{
  cout << "=== Array Utilities: Educational Examples ===\n\n";
  
  // =========================================================================
  // EXAMPLE 1: Reversing Arrays
  // =========================================================================
  // CONCEPT: Reverse array elements in-place with O(n) time, O(1) space
  // APPLICATION: String reversal, palindrome checking, undo/redo stacks
  {
    cout << "--- Example 1: Reversing Arrays ---\n\n";
    
    // Create a simple array
    int arr[] = {1, 2, 3, 4, 5, 6, 7, 8};
    const size_t n = 8;
    
    cout << "Original array: ";
    for (size_t i = 0; i < n; ++i)
      cout << arr[i] << " ";
    cout << "\n";
    
    // STEP 1: Reverse the entire array
    // Algorithm: Swap elements from both ends moving towards center
    // Time: O(n), Space: O(1)
    Aleph::reverse(arr, n);
    
    cout << "After reverse:  ";
    for (size_t i = 0; i < n; ++i)
      cout << arr[i] << " ";
    cout << "\n";
    
    // LESSON: This is the most efficient way to reverse an array
    // No extra memory needed, just n/2 swaps
    cout << "\nLESSON: Reversal uses only n/2 swaps - very efficient!\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 2: Array Rotation
  // =========================================================================
  // CONCEPT: Circular shift of array elements
  // REAL-WORLD: Circular buffers, sliding windows, scheduling algorithms
  {
    cout << "--- Example 2: Array Rotation ---\n\n";
    
    char letters[] = {'A', 'B', 'C', 'D', 'E', 'F'};
    const size_t n = 6;
    
    cout << "Original: ";
    for (size_t i = 0; i < n; ++i)
      cout << letters[i] << " ";
    cout << "\n";
    
    // STEP 1: Rotate LEFT by 2 positions
    // [A B C D E F] -> [C D E F A B]
    // Complexity: O(n) time using reversal algorithm
    Aleph::rotate_left(letters, n, 2);
    
    cout << "Rotate left 2:  ";
    for (size_t i = 0; i < n; ++i)
      cout << letters[i] << " ";
    cout << "\n";
    
    // STEP 2: Rotate RIGHT by 3 positions
    // [C D E F A B] -> [F A B C D E]
    Aleph::rotate_right(letters, n, 3);
    
    cout << "Rotate right 3: ";
    for (size_t i = 0; i < n; ++i)
      cout << letters[i] << " ";
    cout << "\n";
    
    // KEY INSIGHT: Rotation uses clever reversal trick:
    // To rotate left by k: reverse(0,k), reverse(k,n), reverse(0,n)
    cout << "\nKEY ALGORITHM: Rotation = 3 reversals (Bentley's algorithm)\n";
    cout << "  1. Reverse first k elements\n";
    cout << "  2. Reverse remaining n-k elements\n";
    cout << "  3. Reverse entire array\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 3: Gap Operations (Advanced)
  // =========================================================================
  // CONCEPT: Insert/remove space in arrays efficiently
  // APPLICATION: Text editors, memory allocators, dynamic data structures
  {
    cout << "--- Example 3: Gap Operations (Insert/Delete Space) ---\n\n";
    
    // Start with array with room for expansion
    int buffer[10] = {10, 20, 30, 40, 50};
    size_t used = 5;  // Current number of elements
    
    cout << "Initial buffer (5 elements): ";
    for (size_t i = 0; i < used; ++i)
      cout << buffer[i] << " ";
    cout << "\n";
    
    // STEP 1: Open a gap to insert new elements
    // Want to insert 2 elements at position 2
    // [10 20 30 40 50] -> [10 20 __ __ 30 40 50]
    const size_t insert_pos = 2;
    const size_t gap_size = 2;
    
    cout << "\nOpening gap of size " << gap_size << " at position " << insert_pos << "...\n";
    Aleph::open_gap(buffer, used, insert_pos, gap_size);
    
    // STEP 2: Fill the gap with new values
    buffer[insert_pos] = 25;
    buffer[insert_pos + 1] = 27;
    used += gap_size;
    
    cout << "After inserting 25, 27: ";
    for (size_t i = 0; i < used; ++i)
      cout << buffer[i] << " ";
    cout << "\n";
    
    // STEP 3: Close a gap (remove elements)
    // Remove 2 elements starting at position 3
    cout << "\nClosing gap: removing 2 elements at position 3...\n";
    Aleph::close_gap(buffer, used, 3, 2);
    used -= 2;
    
    cout << "After removal: ";
    for (size_t i = 0; i < used; ++i)
      cout << buffer[i] << " ";
    cout << "\n";
    
    // PRACTICAL USAGE: This is how text editors manage insertion/deletion
    cout << "\nREAL-WORLD: Text editors use gap buffers for efficient editing\n";
    cout << "  - Gap moves with cursor\n";
    cout << "  - Insert/delete at gap position is O(1)\n";
    cout << "  - Moving gap is O(distance)\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 4: Using with DynArray
  // =========================================================================
  // CONCEPT: Integrate array utils with Aleph-w containers
  {
    cout << "--- Example 4: Integration with DynArray ---\n\n";
    
    DynArray<int> arr = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    cout << "DynArray: ";
    arr.for_each([](int x) { cout << x << " "; });
    cout << "\n";
    
    // DynArray provides access method for internal array
    size_t n = arr.size();
    
    // Can iterate and reverse conceptually
    cout << "Reversed (manual): ";
    for (int i = n - 1; i >= 0; --i)
      cout << arr[i] << " ";
    cout << "\n\n";
  }
  
  cout << "=== SUMMARY: Key Concepts ===\n";
  cout << "\n1. EFFICIENCY:\n";
  cout << "   All operations are O(n) time, O(1) space\n";
  cout << "   In-place algorithms minimize memory usage\n";
  cout << "\n2. ROTATION ALGORITHM (Bentley):\n";
  cout << "   Three reversals achieve rotation\n";
  cout << "   More efficient than naive circular shifting\n";
  cout << "\n3. GAP BUFFERS:\n";
  cout << "   Core technique for text editors\n";
  cout << "   Efficient insertion/deletion at cursor\n";
  cout << "\n4. WHEN TO USE:\n";
  cout << "   - Implementing custom containers\n";
  cout << "   - Performance-critical array manipulation\n";
  cout << "   - Building higher-level data structures\n";
  cout << "\n5. COMPLEXITY SUMMARY:\n";
  cout << "   reverse():    O(n) time, O(1) space\n";
  cout << "   rotate():     O(n) time, O(1) space\n";
  cout << "   open_gap():   O(n) time, O(1) space\n";
  cout << "   close_gap():  O(n) time, O(1) space\n";
  
  return 0;
}
