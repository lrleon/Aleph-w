
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
 * @file writeHeap.C
 * @brief Demonstrates array-based heap structure and generates visualization files
 * 
 * This program creates an array-based min-heap (priority queue) with random
 * values and generates visualization files showing the heap structure in
 * various traversal orders. Heaps are fundamental data structures for
 * priority queues and efficient sorting.
 *
 * ## What is a Heap?
 *
 * A **heap** is a complete binary tree stored in an array that satisfies the
 * **heap property**:
 * - **Min-heap**: Parent ≤ children (smallest at root)
 * - **Max-heap**: Parent ≥ children (largest at root)
 *
 * **Key properties**:
 * - **Complete binary tree**: All levels full except possibly last, filled left-to-right
 * - **Heap property**: Parent-child relationship maintained
 * - **Array storage**: Efficient representation without pointers
 *
 * ## Array Representation
 *
 * ### Indexing Scheme (1-indexed)
 *
 * Heaps use a clever array indexing scheme:
 * - **Parent** of node i: `i/2` (integer division)
 * - **Left child** of node i: `2*i`
 * - **Right child** of node i: `2*i + 1`
 * - **Root**: Index 1 (index 0 unused or stores size)
 *
 * ### Example
 *
 * ```
 * Array: [-, 1, 3, 2, 5, 4, 6]
 * Index:  0  1  2  3  4  5  6
 *
 * Tree:
 *         1 (index 1)
 *        / \
 *       3   2 (indices 2, 3)
 *      / \ / \
 *     5  4 6  (indices 4, 5, 6)
 * ```
 *
 * ### Advantages
 *
 * - **Space efficient**: No pointers needed (saves memory)
 * - **Cache friendly**: Sequential memory access (better performance)
 * - **Fast navigation**: Simple arithmetic for parent/child (O(1))
 * - **Simple implementation**: Easy to code and understand
 *
 * ## Heap Operations
 *
 * ### Insertion (Bubble Up)
 *
 * ```
 * insert(value):
 *   1. Add value at end of array
 *   2. While value < parent:
 *      Swap with parent
 *      Move up tree
 * ```
 *
 * **Time**: O(log n) - height of tree
 *
 * ### Extract Minimum (Bubble Down)
 *
 * ```
 * extract_min():
 *   1. Save root value
 *   2. Move last element to root
 *   3. While heap property violated:
 *      Swap with smaller child
 *      Move down tree
 *   4. Return saved value
 * ```
 *
 * **Time**: O(log n) - height of tree
 *
 * ### Build Heap (Heapify)
 *
 * ```
 * build_heap(array):
 *   For i = n/2 down to 1:
 *     heapify_down(i)
 * ```
 *
 * **Time**: O(n) - surprisingly efficient!
 * **Why**: Most nodes are near bottom, few swaps needed
 *
 * ### Complexity Summary
 *
 * | Operation | Time Complexity | Description |
 * |-----------|-----------------|-------------|
 * | insert | O(log n) | Add element, bubble up |
 * | extract_min | O(log n) | Remove root, bubble down |
 * | peek | O(1) | Get root without removal |
 * | build_heap | O(n) | Heapify array in-place |
 * | decrease_key | O(log n) | Decrease element, bubble up |
 * | increase_key | O(log n) | Increase element, bubble down |
 *
 * ## Applications
 *
 * ### Priority Queues
 * - **Task scheduling**: Execute highest priority tasks first
 * - **Event simulation**: Process events in chronological order
 * - **Operating systems**: Process scheduling
 *
 * ### Graph Algorithms
 * - **Dijkstra's algorithm**: Find shortest paths (min-heap)
 * - **Prim's MST**: Find minimum spanning tree (min-heap)
 * - **A\* search**: Pathfinding with heuristics
 *
 * ### Sorting
 * - **Heap sort**: O(n log n) in-place sorting
 * - **Partial sort**: Sort first k elements efficiently
 *
 * ### Statistics
 * - **Median finding**: Use two heaps (min + max)
 * - **Top-k queries**: Keep k largest/smallest elements
 * - **Percentiles**: Find k-th percentile efficiently
 *
 * ## Heap vs Other Structures
 *
 * | Structure | Insert | Extract Min | Build | Best For |
 * |-----------|--------|-------------|-------|----------|
 * | Heap | O(log n) | O(log n) | O(n) | Priority queues |
 * | Sorted Array | O(n) | O(1) | O(n log n) | Static data |
 * | BST | O(log n) | O(log n) | O(n log n) | Dynamic, ordered |
 * | Linked List | O(1) | O(n) | O(n) | Simple cases |
 *
 * ## Output Files
 *
 * - **`heap-ejm-aux.Tree`**: Preorder and inorder traversals for `btreepic`
 *   - Shows tree structure
 *   - Can be visualized with btreepic tool
 *
 * - **`heap-ejm-aux.tex`**: Level-order traversal formatted for LaTeX
 *   - Shows array representation
 *   - Useful for documentation
 *
 * The tree visualization shows the heap structure, which is a complete
 * binary tree (all levels full except possibly the last, filled left-to-right).
 *
 * ## Usage
 *
 * ```bash
 * # Generate heap with 20 elements
 * writeHeap -n 20
 *
 * # Use specific seed for reproducibility
 * writeHeap -n 50 -s 42
 *
 * # Generate larger heap
 * writeHeap -n 100
 * ```
 *
 * @see tpl_arrayHeap.H Array-based heap implementation
 * @see heap_example.C Comprehensive heap examples (Binary vs Fibonacci)
 * @see btreepic.C Visualization tool for binary trees
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

# include <cstdlib>
# include <iostream>
# include <fstream>
# include <ctime>
# include <tclap/CmdLine.h>
# include <tpl_arrayHeap.H>

using namespace std;
using namespace Aleph;

// Local macros for heap navigation (1-indexed array)
# define HEAP_LLINK(i) (2*(i))
# define HEAP_RLINK(i) (2*(i) + 1)

// Output streams
ofstream output;
ofstream output_tex;

/**
 * @brief Write preorder traversal of heap
 * @param v Heap array (1-indexed)
 * @param n Number of elements
 * @param i Current index
 */
void preorder(int v[], int n, int i)
{
  if (i > n)
    return;

  output << " " << v[i];
  cout << " " << v[i];

  assert(v[i] > 0);

  preorder(v, n, HEAP_LLINK(i));
  preorder(v, n, HEAP_RLINK(i));
}

/**
 * @brief Write inorder traversal of heap
 * @param v Heap array (1-indexed)
 * @param n Number of elements
 * @param i Current index
 */
void inorder(int v[], int n, int i)
{
  if (i > n)
    return;

  assert(v[i] > 0);

  inorder(v, n, HEAP_LLINK(i));
  output << " " << v[i];
  cout << " " << v[i];
  inorder(v, n, HEAP_RLINK(i));
}

/**
 * @brief Write level-order traversal for LaTeX output
 * @param v Heap array (1-indexed)
 * @param n Number of elements
 */
void level_order(int v[], int n)
{
  for (int i = 1; i <= n; ++i)
    {
      assert(v[i] > 0);
      output_tex << v[i] << " ";
    }
}

/**
 * @brief Check if value exists in heap
 * @param heap The heap to search
 * @param x Value to find
 * @return true if found
 */
bool exists_in_heap(ArrayHeap<int>& heap, int x)
{
  for (int i = 1; i <= heap.size(); ++i)
    if (heap[i] == x)
      return true;
  return false;
}

int main(int argc, char* argv[])
{
  try
    {
      TCLAP::CmdLine cmd(
        "Demonstrate array-based heap structure.\n"
        "Creates a heap and generates visualization files.",
        ' ', "1.0");

      TCLAP::ValueArg<unsigned int> nArg("n", "count",
                                          "Number of elements",
                                          false, 10, "unsigned int");
      cmd.add(nArg);

      TCLAP::ValueArg<unsigned int> seedArg("s", "seed",
                                             "Random seed (0 = use time)",
                                             false, 0, "unsigned int");
      cmd.add(seedArg);

      cmd.parse(argc, argv);

      unsigned int n = nArg.getValue();
      unsigned int t = seedArg.getValue();

      if (t == 0)
        t = time(nullptr);

      srand(t);

      cout << "=== Array-Based Heap Demo ===" << endl;
      cout << "Elements: " << n << ", Seed: " << t << endl << endl;

      // Open output files
      output.open("heap-ejm-aux.Tree", ios::out);
      output_tex.open("heap-ejm-aux.tex", ios::out);

      if (!output.is_open() || !output_tex.is_open())
        {
          cerr << "Error: cannot open output files" << endl;
          return 1;
        }

      // Build heap with unique positive values
      ArrayHeap<int> heap;

      cout << "Inserting values: ";
      for (unsigned int i = 0; i < n; i++)
        {
          int value;
          do
            {
              value = 1 + static_cast<int>(10.0 * n * rand() / (RAND_MAX + 10.0 * n));
              assert(value > 0);
            }
          while (exists_in_heap(heap, value));

          heap.insert(value);
          cout << value << " ";
        }
      cout << endl << endl;

      cout << "Heap size: " << heap.size() << endl;
      cout << "Min element (root): " << heap.top() << endl << endl;

      // Write preorder traversal
      output << "Prefix ";
      cout << "Preorder: ";
      preorder(&heap[0], heap.size(), 1);
      cout << endl;

      // Write inorder traversal
      output << endl << "Infix ";
      cout << "Inorder:  ";
      inorder(&heap[0], heap.size(), 1);
      output << endl;
      cout << endl;

      // Write level-order for LaTeX
      cout << "Level-order: ";
      for (int i = 1; i <= heap.size(); ++i)
        cout << heap[i] << " ";
      cout << endl;
      level_order(&heap[0], heap.size());
      output_tex << endl;

      // Close files
      output.close();
      output_tex.close();

      cout << endl << "Generated files:" << endl;
      cout << "  - heap-ejm-aux.Tree (preorder + inorder for btreepic)" << endl;
      cout << "  - heap-ejm-aux.tex (level-order for LaTeX)" << endl;
    }
  catch (TCLAP::ArgException& e)
    {
      cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
      return 1;
    }

  return 0;
}
