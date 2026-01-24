
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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


/**
 * @file linear_structures_example.C
 * @brief Linear data structures: Stack, Queue, DynArray, DynList
 *
 * This example demonstrates the fundamental linear (one-dimensional) data
 * structures in Aleph-w. These are the building blocks for more complex
 * algorithms and data structures. Understanding these is essential for
 * effective programming.
 *
 * ## Linear Data Structures Overview
 *
 * Linear data structures organize elements in a sequence where each element
 * (except the first and last) has exactly one predecessor and one successor.
 * They differ in how elements are accessed and modified.
 *
 * ## 1. Stack (LIFO - Last In First Out)
 *
 * A stack is like a stack of plates: you add to the top and remove from the top.
 *
 * ### Implementations
 *
 * #### DynListStack<T>
 * - **Backend**: Linked list
 * - **Capacity**: Unlimited (grows as needed)
 * - **Memory**: O(n) - pointer overhead per element
 * - **Best for**: Unknown size, frequent growth
 *
 * #### ArrayStack<T>
 * - **Backend**: Fixed-size array
 * - **Capacity**: Fixed at creation
 * - **Memory**: O(n) - no pointer overhead
 * - **Best for**: Known size, maximum performance
 *
 * ### Operations
 * - **push(x)**: Add element to top - O(1)
 * - **pop()**: Remove top element - O(1)
 * - **top()**: View top element - O(1)
 * - **is_empty()**: Check if empty - O(1)
 *
 * ### Applications
 * - **Expression evaluation**: Infix to postfix conversion
 * - **Function calls**: Call stack management
 * - **Undo/Redo**: Operation history
 * - **Backtracking**: DFS, maze solving
 * - **Syntax parsing**: Matching parentheses, brackets
 *
 * ## 2. Queue (FIFO - First In First Out)
 *
 * A queue is like a line at a store: first person in is first person out.
 *
 * ### Implementations
 *
 * #### DynListQueue<T>
 * - **Backend**: Circular linked list
 * - **Capacity**: Unlimited
 * - **Memory**: O(n) - pointer overhead
 * - **Best for**: Unknown size, dynamic growth
 *
 * #### ArrayQueue<T>
 * - **Backend**: Circular array
 * - **Capacity**: Fixed size
 * - **Memory**: O(n) - no pointer overhead
 * - **Best for**: Known size, performance-critical
 *
 * ### Operations
 * - **put(x)**: Add element to rear - O(1)
 * - **get()**: Remove element from front - O(1)
 * - **front()**: View front element - O(1)
 * - **rear()**: View rear element - O(1)
 * - **is_empty()**: Check if empty - O(1)
 *
 * ### Applications
 * - **Task scheduling**: Process queues, job queues
 * - **BFS traversal**: Graph algorithms
 * - **Print spooling**: Print job management
 * - **Message queues**: Inter-process communication
 * - **Event handling**: Event queue processing
 *
 * ## 3. Dynamic Array (DynArray<T>)
 *
 * A resizable array that grows automatically as needed.
 *
 * ### Characteristics
 * - **Random access**: O(1) - access any element by index
 * - **Append**: O(1) amortized - occasionally O(n) for resizing
 * - **Insert/Delete middle**: O(n) - must shift elements
 * - **Memory**: Contiguous block, cache-friendly
 *
 * ### Operations
 * - **append(x)**: Add to end - O(1) amortized
 * - **insert(i, x)**: Insert at position i - O(n)
 * - **remove(i)**: Remove at position i - O(n)
 * - **operator[](i)**: Access element i - O(1)
 * - **Functional ops**: map, filter, fold - O(n)
 *
 * ### Applications
 * - **General-purpose storage**: When random access needed
 * - **Mathematical vectors**: Linear algebra operations
 * - **Buffers**: Data buffering, streaming
 * - **Lookup tables**: Index-based access
 *
 * ## 4. Dynamic List (DynList<T> / DynDlist<T>)
 *
 * Linked lists that grow dynamically with efficient insertion/deletion.
 *
 * ### Implementations
 *
 * #### DynList<T> (Singly Linked)
 * - **Links**: Each node points to next only
 * - **Traversal**: Forward only
 * - **Memory**: O(n) - one pointer per element
 * - **Best for**: Simple forward iteration
 *
 * #### DynDlist<T> (Doubly Linked)
 * - **Links**: Each node points to both next and previous
 * - **Traversal**: Forward and backward
 * - **Memory**: O(n) - two pointers per element
 * - **Best for**: Bidirectional iteration, frequent deletion
 *
 * ### Operations
 * - **append(x)**: Add to end - O(1)
 * - **insert_after(it, x)**: Insert after iterator - O(1)
 * - **remove(it)**: Remove at iterator - O(1)
 * - **Access by index**: O(n) - must traverse
 * - **Iteration**: O(n) - forward (both) or backward (DynDlist only)
 *
 * ### Applications
 * - **Frequent insertion/deletion**: When order matters
 * - **Unknown size**: Grows dynamically
 * - **No random access needed**: Sequential access only
 * - **Memory efficiency**: When array resizing is expensive
 *
 * ## Complexity Comparison
 *
 * | Operation | Array | List | Stack | Queue |
 * |-----------|-------|------|-------|-------|
 * | Access by index | O(1) | O(n) | N/A | N/A |
 * | Insert at end | O(1)* | O(1) | O(1) | O(1) |
 * | Insert at middle | O(n) | O(1) | N/A | N/A |
 * | Delete at middle | O(n) | O(1) | N/A | N/A |
 * | Memory overhead | Low | High | Medium | Medium |
 *
 * *Amortized for dynamic arrays
 *
 * ## When to Use Each
 *
 * - **Stack**: LIFO access needed (undo, backtracking, parsing)
 * - **Queue**: FIFO access needed (BFS, scheduling, buffering)
 * - **DynArray**: Random access needed, mostly append operations
 * - **DynList**: Frequent insertion/deletion, sequential access only
 *
 * @see tpl_dynListStack.H Stack implementations
 * @see tpl_dynListQueue.H Queue implementations
 * @see tpl_dynArray.H Dynamic array implementation
 * @see tpl_dynList.H Linked list implementations
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

#include <iostream>
#include <string>
#include <tpl_dynListStack.H>
#include <tpl_dynListQueue.H>
#include <tpl_dynArray.H>
#include <tpl_dynList.H>
#include <tpl_dynDlist.H>
#include <tpl_arrayStack.H>
#include <tpl_arrayQueue.H>

using namespace std;
using namespace Aleph;

// =============================================================================
// Example 1: Stack (LIFO)
// =============================================================================

void demo_stack()
{
  cout << "\n";
  cout << "╔══════════════════════════════════════════════════════════════════╗\n";
  cout << "║      EXAMPLE 1: Stack (LIFO - Last In First Out)                 ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════╝\n\n";

  cout << "Stack follows LIFO principle: last element pushed is first popped.\n\n";

  // Dynamic stack (unlimited size)
  DynListStack<string> stack;

  cout << "--- DynListStack operations ---\n\n";

  // Push elements
  cout << "Pushing: Apple, Banana, Cherry, Date\n";
  stack.push("Apple");
  stack.push("Banana");
  stack.push("Cherry");
  stack.push("Date");

  cout << "Stack size: " << stack.size() << "\n";
  cout << "Top element: " << stack.top() << "\n\n";

  // Pop elements
  cout << "Popping elements (LIFO order):\n";
  while (!stack.is_empty())
    cout << "  Pop: " << stack.pop() << "\n";

  cout << "\n--- Practical application: Balanced parentheses ---\n\n";

  auto check_balanced = [](const string& expr) {
    DynListStack<char> s;
    for (char c : expr) {
      if (c == '(' || c == '[' || c == '{')
        s.push(c);
      else if (c == ')' || c == ']' || c == '}') {
        if (s.is_empty()) return false;
        char top = s.pop();
        if ((c == ')' && top != '(') ||
            (c == ']' && top != '[') ||
            (c == '}' && top != '{'))
          return false;
      }
    }
    return s.is_empty();
  };

  auto test_expr = [&](const string& expr) {
    cout << "  \"" << expr << "\" → " 
         << (check_balanced(expr) ? "BALANCED" : "UNBALANCED") << "\n";
  };

  test_expr("((a+b)*c)");
  test_expr("{[a+(b*c)]}");
  test_expr("((a+b)");
  test_expr("([a+b)]");
}

// =============================================================================
// Example 2: Queue (FIFO)
// =============================================================================

void demo_queue()
{
  cout << "\n";
  cout << "╔══════════════════════════════════════════════════════════════════╗\n";
  cout << "║      EXAMPLE 2: Queue (FIFO - First In First Out)                ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════╝\n\n";

  cout << "Queue follows FIFO principle: first element added is first removed.\n\n";

  DynListQueue<string> queue;

  cout << "--- DynListQueue operations ---\n\n";

  // Enqueue elements
  cout << "Enqueueing: Task1, Task2, Task3, Task4\n";
  queue.put("Task1");
  queue.put("Task2");
  queue.put("Task3");
  queue.put("Task4");

  cout << "Queue size: " << queue.size() << "\n";
  cout << "Front element: " << queue.front() << "\n";
  cout << "Rear element: " << queue.rear() << "\n\n";

  // Dequeue elements
  cout << "Dequeueing elements (FIFO order):\n";
  while (!queue.is_empty())
    cout << "  Dequeue: " << queue.get() << "\n";

  cout << "\n--- Practical application: Print job scheduler ---\n\n";

  struct PrintJob {
    string name;
    int pages;
  };

  DynListQueue<PrintJob> printer;

  // Add print jobs
  printer.put({"Report.pdf", 10});
  printer.put({"Photo.jpg", 1});
  printer.put({"Manual.pdf", 50});
  printer.put({"Letter.doc", 2});

  cout << "Print queue:\n";
  int total_pages = 0;
  while (!printer.is_empty()) {
    PrintJob job = printer.get();
    cout << "  Printing: " << job.name << " (" << job.pages << " pages)\n";
    total_pages += job.pages;
  }
  cout << "Total pages printed: " << total_pages << "\n";
}

// =============================================================================
// Example 3: Dynamic Array
// =============================================================================

void demo_dynarray()
{
  cout << "\n";
  cout << "╔══════════════════════════════════════════════════════════════════╗\n";
  cout << "║      EXAMPLE 3: DynArray (Resizable Array)                       ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════╝\n\n";

  cout << "DynArray provides O(1) random access with dynamic resizing.\n\n";

  DynArray<int> arr;

  cout << "--- Basic operations ---\n\n";

  // Append elements
  cout << "Appending: 10, 20, 30, 40, 50\n";
  arr.append(10);
  arr.append(20);
  arr.append(30);
  arr.append(40);
  arr.append(50);

  cout << "Size: " << arr.size() << "\n";
  cout << "Elements: ";
  for (size_t i = 0; i < arr.size(); ++i)
    cout << arr(i) << " ";
  cout << "\n\n";

  // Random access
  cout << "Random access:\n";
  cout << "  arr(0) = " << arr(0) << "\n";
  cout << "  arr(2) = " << arr(2) << "\n";
  cout << "  arr(4) = " << arr(4) << "\n\n";

  // Modify element
  cout << "Modifying arr(2) = 300\n";
  arr(2) = 300;
  cout << "Elements: ";
  for (size_t i = 0; i < arr.size(); ++i)
    cout << arr(i) << " ";
  cout << "\n\n";

  cout << "--- Functional operations ---\n\n";

  DynArray<int> numbers;
  for (int i = 1; i <= 10; ++i)
    numbers.append(i);

  cout << "Original: ";
  numbers.for_each([](int x) { cout << x << " "; });
  cout << "\n";

  // Filter even numbers
  auto evens = numbers.filter([](int x) { return x % 2 == 0; });
  cout << "Evens:    ";
  evens.for_each([](int x) { cout << x << " "; });
  cout << "\n";

  // Map: square each number
  auto squared = numbers.maps<int>([](int x) { return x * x; });
  cout << "Squared:  ";
  squared.for_each([](int x) { cout << x << " "; });
  cout << "\n";

  // Fold: sum all numbers
  int sum = numbers.foldl(0, [](int acc, int x) { return acc + x; });
  cout << "Sum:      " << sum << "\n";
}

// =============================================================================
// Example 4: Dynamic Lists
// =============================================================================

void demo_dynlist()
{
  cout << "\n";
  cout << "╔══════════════════════════════════════════════════════════════════╗\n";
  cout << "║      EXAMPLE 4: DynList and DynDlist (Linked Lists)              ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════╝\n\n";

  cout << "Linked lists allow O(1) insertion/deletion at any known position.\n\n";

  // Singly linked list
  cout << "--- DynList (singly linked) ---\n\n";

  DynList<int> slist;

  // Insert at front (O(1))
  cout << "Inserting at front: 3, 2, 1\n";
  slist.insert(3);
  slist.insert(2);
  slist.insert(1);

  // Append at end
  cout << "Appending at end: 4, 5\n";
  slist.append(4);
  slist.append(5);

  cout << "List: ";
  slist.for_each([](int x) { cout << x << " "; });
  cout << "\n";
  cout << "Size: " << slist.size() << "\n\n";

  // Doubly linked list
  cout << "--- DynDlist (doubly linked) ---\n\n";

  DynDlist<string> dlist;

  cout << "Inserting: First, Second, Third\n";
  dlist.append("First");
  dlist.append("Second");
  dlist.append("Third");

  cout << "Forward:  ";
  dlist.for_each([](const string& s) { cout << s << " "; });
  cout << "\n";

  // Remove from front and back
  cout << "\nRemoving first: " << dlist.remove_first() << "\n";
  cout << "Removing last: " << dlist.remove_last() << "\n";

  cout << "Remaining: ";
  dlist.for_each([](const string& s) { cout << s << " "; });
  cout << "\n";
}

// =============================================================================
// Example 5: Comparison of Stack vs Queue
// =============================================================================

void demo_comparison()
{
  cout << "\n";
  cout << "╔══════════════════════════════════════════════════════════════════╗\n";
  cout << "║      EXAMPLE 5: Stack vs Queue Comparison                        ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════╝\n\n";

  DynListStack<int> stack;
  DynListQueue<int> queue;

  // Add same elements to both
  cout << "Adding elements 1, 2, 3, 4, 5 to both structures:\n\n";
  for (int i = 1; i <= 5; ++i) {
    stack.push(i);
    queue.put(i);
  }

  // Remove and compare order
  cout << "Removal order:\n";
  cout << "  Stack (LIFO): ";
  while (!stack.is_empty())
    cout << stack.pop() << " ";
  cout << "\n";

  cout << "  Queue (FIFO): ";
  while (!queue.is_empty())
    cout << queue.get() << " ";
  cout << "\n";

  cout << "\nUse Stack for: undo/redo, recursion, backtracking\n";
  cout << "Use Queue for: scheduling, BFS, buffering\n";
}

// =============================================================================
// Main
// =============================================================================

int main()
{
  cout << "\n";
  cout << "╔══════════════════════════════════════════════════════════════════╗\n";
  cout << "║         Linear Data Structures in Aleph-w Library                ║\n";
  cout << "║                                                                  ║\n";
  cout << "║     Aleph-w Library - https://github.com/lrleon/Aleph-w          ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════╝\n";

  demo_stack();
  demo_queue();
  demo_dynarray();
  demo_dynlist();
  demo_comparison();

  cout << "\n";
  cout << "╔══════════════════════════════════════════════════════════════════╗\n";
  cout << "║                         Summary                                  ║\n";
  cout << "╠══════════════════════════════════════════════════════════════════╣\n";
  cout << "║  DynListStack:  Dynamic LIFO stack (linked list based)          ║\n";
  cout << "║  DynListQueue:  Dynamic FIFO queue (circular list based)        ║\n";
  cout << "║  DynArray:      Resizable array with O(1) access                ║\n";
  cout << "║  DynList:       Singly linked list                              ║\n";
  cout << "║  DynDlist:      Doubly linked list                              ║\n";
  cout << "║                                                                  ║\n";
  cout << "║  All support functional operations: map, filter, fold, for_each ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════╝\n\n";

  return 0;
}
