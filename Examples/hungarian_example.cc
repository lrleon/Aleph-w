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

# include <iostream>
# include <Hungarian.H>
# include <tpl_dynMat.H>

using namespace std;
using namespace Aleph;

/**
 * @brief Demonstrates a 4x4 workers-to-tasks assignment using Hungarian_Assignment.
 *
 * Prints a labeled 4x4 cost matrix, constructs a Hungarian_Assignment<int> with
 * those costs, outputs the optimal total cost and the worker->task assignments,
 * and then prints the individual per-assignment costs using a local constexpr matrix.
 */
void example_basic_assignment()
{
  cout << "=== Basic Assignment (4x4 workers-to-tasks) ===" << endl;
  cout << endl;
  cout << "Cost matrix:" << endl;
  cout << "  Task0 Task1 Task2 Task3" << endl;
  cout << "W0:  82    83    69    92" << endl;
  cout << "W1:  77    37    49    92" << endl;
  cout << "W2:  11    69     5    86" << endl;
  cout << "W3:   8     9    98    23" << endl;
  cout << endl;

  const Hungarian_Assignment<int> ha({
    {82, 83, 69, 92},
    {77, 37, 49, 92},
    {11, 69,  5, 86},
    { 8,  9, 98, 23}
  });

  cout << "Optimal total cost: " << ha.get_total_cost() << endl;
  cout << "Assignments:" << endl;
  for (auto [r, c] : ha.get_assignments())
    cout << "  Worker " << r << " -> Task " << c << endl;
  cout << endl;

  // Show individual costs
  constexpr int costs[4][4] = {
    {82, 83, 69, 92},
    {77, 37, 49, 92},
    {11, 69,  5, 86},
    { 8,  9, 98, 23}
  };
  cout << "Detailed:" << endl;
  for (auto [r, c] : ha.get_assignments())
    cout << "  Worker " << r << " -> Task " << c
         << " (cost " << costs[r][c] << ")" << endl;
  cout << endl;
}

/**
 * @brief Demonstrates maximizing total profit for a 3x3 profit matrix and prints the results.
 *
 * Constructs a 3x3 DynMatrix of integer profits, computes a maximum-weight assignment
 * using hungarian_max_assignment, and writes the profit matrix, the maximum total profit,
 * and each worker->job assignment with its profit to standard output.
 */
void example_maximization()
{
  cout << "=== Maximization (3x3 profit matrix) ===" << endl;
  cout << endl;
  cout << "Profit matrix:" << endl;
  cout << "  Job0 Job1 Job2" << endl;
  cout << "W0: 10    5   13" << endl;
  cout << "W1:  3    9   18" << endl;
  cout << "W2: 10    6   12" << endl;
  cout << endl;

  auto mat = DynMatrix<int>(3, 3, 0);
  mat.allocate();
  int data[3][3] = {{10, 5, 13}, {3, 9, 18}, {10, 6, 12}};
  for (size_t i = 0; i < 3; ++i)
    for (size_t j = 0; j < 3; ++j)
      mat(i, j) = data[i][j];

  auto result = hungarian_max_assignment(mat);

  cout << "Maximum total profit: " << result.total_cost << endl;
  cout << "Assignments:" << endl;
  for (auto [r, c] : result.get_pairs())
    cout << "  Worker " << r << " -> Job " << c
         << " (profit " << data[r][c] << ")" << endl;
  cout << endl;
}

/**
 * @brief Demonstrates rectangular assignment (3 workers, 5 tasks) using Hungarian_Assignment.
 *
 * @details Prints a labeled 3x5 cost matrix to stdout, constructs a Hungarian_Assignment<int>
 * with the hard-coded costs, then prints the optimal total cost and the worker→task assignments
 * (two tasks remain unassigned).
 */
void example_rectangular()
{
  cout << "=== Rectangular (3 workers, 5 tasks) ===" << endl;
  cout << endl;
  cout << "Cost matrix:" << endl;
  cout << "  T0 T1 T2 T3 T4" << endl;
  cout << "W0: 10  3  7  2  8" << endl;
  cout << "W1:  5  9  1  6  4" << endl;
  cout << "W2: 12 11  6  3  7" << endl;
  cout << endl;

  Hungarian_Assignment<int> ha({
    {10, 3, 7, 2, 8},
    { 5, 9, 1, 6, 4},
    {12, 11, 6, 3, 7}
  });

  cout << "Optimal total cost: " << ha.get_total_cost() << endl;
  const size_t unassigned_count = 5 - ha.get_assignments().size();
  cout << "Assignments (" << unassigned_count << " tasks left unassigned):" << endl;
  for (auto [r, c] : ha.get_assignments())
    cout << "  Worker " << r << " -> Task " << c << endl;
  cout << endl;
}

/**
 * @brief Runs the three Hungarian algorithm example demonstrations.
 *
 * Calls example_basic_assignment(), example_maximization(), and example_rectangular()
 * to print example matrices, optimal assignments, and total costs.
 *
 * @return int 0 on success.
 */
int main()
{
  example_basic_assignment();
  example_maximization();
  example_rectangular();

  return 0;
}