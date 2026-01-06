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
 * @author Leandro Rabindranath León
 * @ingroup Ejemplos
*/

/**
 * @file writeHeap.C
 * @brief Demonstrates array-based heap structure and generates visualization files
 * 
 * This program creates an array-based min-heap with random unique values
 * and generates visualization files showing the heap in various traversal orders.
 * 
 * The heap is stored in an array where:
 * - Parent of node i is at i/2
 * - Left child of node i is at 2*i
 * - Right child of node i is at 2*i + 1
 * 
 * Output files:
 * - heap-ejm-aux.Tree: Preorder and inorder traversals for btreepic
 * - heap-ejm-aux.tex: Level-order traversal for LaTeX
 * 
 * Usage: writeHeap [-n <count>] [-s <seed>]
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
  for (int i = 1; i < heap.size(); ++i)
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
