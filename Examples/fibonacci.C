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
 * @file fibonacci.C
 * @brief Fibonacci number computation using three different methods
 * 
 * This example demonstrates three different approaches to computing
 * Fibonacci numbers:
 * 
 * 1. **Recursive** (fib_recursive): The classic recursive definition.
 *    Simple but exponential time complexity O(2^n).
 * 
 * 2. **Iterative** (fib_iterative): Bottom-up computation using a loop.
 *    Linear time complexity O(n) and constant space O(1).
 * 
 * 3. **Stack-based with Activation Records** (fib_stack): Simulates the
 *    recursive call stack explicitly using an ArrayStack. This is the
 *    most educational approach as it shows exactly how recursion works
 *    at a low level - managing activation records, return points, and
 *    local variables manually.
 * 
 * The stack-based implementation is particularly useful for understanding:
 * - How compilers transform recursion into iteration
 * - The structure of activation records (stack frames)
 * - Return point management (continuation)
 * - How tail-call optimization could be applied
 * 
 * Usage: fibonacci -n <number> [-m <method>]
 *        Methods: all (default), recursive, iterative, stack
 */

# include <iostream>
# include <cstdlib>
# include <chrono>
# include <tclap/CmdLine.h>
# include <tpl_arrayStack.H>
# include <ah-errors.H>

using namespace std;
using namespace Aleph;

// ============================================================================
// Method 1: Classic Recursive Implementation
// ============================================================================

/**
 * @brief Compute Fibonacci number recursively
 * 
 * Classic recursive definition:
 *   fib(0) = 0
 *   fib(1) = 1
 *   fib(n) = fib(n-1) + fib(n-2)
 * 
 * Time complexity: O(2^n) - exponential
 * Space complexity: O(n) - call stack depth
 * 
 * @param n The index of the Fibonacci number to compute
 * @return The n-th Fibonacci number
 */
long long fib_recursive(int n)
{
  if (n <= 0)
    return 0;
  if (n == 1)
    return 1;

  return fib_recursive(n - 1) + fib_recursive(n - 2);
}

// ============================================================================
// Method 2: Iterative Implementation
// ============================================================================

/**
 * @brief Compute Fibonacci number iteratively
 * 
 * Bottom-up computation maintaining only the last two values.
 * 
 * Time complexity: O(n) - linear
 * Space complexity: O(1) - constant
 * 
 * @param n The index of the Fibonacci number to compute
 * @return The n-th Fibonacci number
 */
long long fib_iterative(int n)
{
  if (n <= 0)
    return 0;
  if (n == 1)
    return 1;

  long long fi_2 = 0;  // fib(i-2)
  long long fi_1 = 1;  // fib(i-1)
  long long fi = 0;    // fib(i)

  for (int i = 2; i <= n; i++)
    {
      fi = fi_1 + fi_2;
      fi_2 = fi_1;
      fi_1 = fi;
    }

  return fi;
}

// ============================================================================
// Method 3: Stack-based with Explicit Activation Records
// ============================================================================

/**
 * @brief Activation record structure for stack-based Fibonacci
 * 
 * This structure represents a stack frame (activation record) containing:
 * - n: The parameter for this invocation of fib()
 * - f1: Storage for the result of the first recursive call fib(n-1)
 * - result: The computed result to return to caller
 * - return_point: Which label to jump to when returning (P1 or P2)
 * 
 * This mirrors exactly what the compiler generates for each function call.
 */
struct Activation_Record
{
  int n;              ///< Parameter: which Fibonacci number to compute
  long long f1;       ///< Local variable: stores result of fib(n-1)
  long long result;   ///< Return value to pass to caller
  char return_point;  ///< Continuation: where to resume after return
};

/// Return point labels (simulating goto targets after recursive calls)
constexpr char P1 = 1;  ///< Return point after first recursive call fib(n-1)
constexpr char P2 = 2;  ///< Return point after second recursive call fib(n-2)

/// Accessor macros for top of stack (current activation record)
# define NUM(p)          ((p)->n)
# define F1(p)           ((p)->f1)
# define RESULT(p)       ((p)->result)
# define RETURN_POINT(p) ((p)->return_point)

/**
 * @brief Compute Fibonacci number using explicit stack management
 * 
 * This implementation manually manages what the compiler does automatically
 * for recursive calls:
 * 
 * 1. Push a new activation record before each "recursive call"
 * 2. Set up parameters in the new record
 * 3. Record the return point (continuation)
 * 4. Jump to the start of the function body
 * 5. On "return", pop the record and jump to the saved return point
 * 
 * The flow simulates this recursive code:
 * @code
 *   long long fib(int n) {
 *     if (n <= 1) return (n <= 0) ? 0 : 1;  // Base case
 *     long long f1 = fib(n - 1);            // First recursive call (P1)
 *     long long f2 = fib(n - 2);            // Second recursive call (P2)
 *     return f1 + f2;
 *   }
 * @endcode
 * 
 * Time complexity: O(2^n) - same as recursive (we're simulating it exactly)
 * Space complexity: O(n) - explicit stack instead of call stack
 * 
 * @param n The index of the Fibonacci number to compute
 * @return The n-th Fibonacci number
 */
long long fib_stack(int n)
{
  ArrayStack<Activation_Record> stack;
  
  // Create dummy caller record (to receive final result)
  Activation_Record* caller_ar = &stack.pushn();
  
  // Create first real activation record for fib(n)
  Activation_Record* current_ar = &stack.pushn();
  NUM(current_ar) = n;

  // ========== Function body starts here ==========
start:
  // Base case: fib(0) = 0, fib(1) = 1
  if (NUM(current_ar) <= 1)
    {
      RESULT(caller_ar) = (NUM(current_ar) <= 0) ? 0 : 1;
      goto return_from_fib;
    }

  // ---------- First recursive call: fib(n-1) ----------
  // Save return point for when fib(n-1) completes
  RETURN_POINT(current_ar) = P1;
  
  // Push new activation record for fib(n-1)
  NUM(&stack.pushn()) = NUM(current_ar) - 1;
  
  // Update pointers to reflect new stack state
  caller_ar = &stack.top(1);    // Previous current is now caller
  current_ar = &stack.top();    // New record is current
  
  goto start;  // "Call" fib(n-1)

p1:  // Return point after fib(n-1) completes
  // Save result of fib(n-1) in local variable f1
  F1(current_ar) = RESULT(current_ar);
  
  // ---------- Second recursive call: fib(n-2) ----------
  // Save return point for when fib(n-2) completes
  RETURN_POINT(current_ar) = P2;
  
  // Push new activation record for fib(n-2)
  NUM(&stack.pushn()) = NUM(current_ar) - 2;
  
  // Update pointers
  caller_ar = &stack.top(1);
  current_ar = &stack.top();
  
  goto start;  // "Call" fib(n-2)

p2:  // Return point after fib(n-2) completes
  // Compute final result: f1 + fib(n-2)
  // Note: RESULT(current_ar) holds fib(n-2) from the return
  RESULT(caller_ar) = F1(current_ar) + RESULT(current_ar);

  // ========== Return sequence ==========
return_from_fib:
  // Pop current activation record
  stack.pop();
  
  // If only dummy record remains, we're done
  if (stack.size() == 1)
    return RESULT(caller_ar);

  // Update pointers after pop
  caller_ar = &stack.top(1);
  current_ar = &stack.top();

  // Jump to saved return point (continuation)
  switch (RETURN_POINT(current_ar))
    {
    case P1: goto p1;
    case P2: goto p2;
    default: AH_ERROR("Invalid return point: %d", RETURN_POINT(current_ar));
    }

  return 0;  // Never reached
}

// Cleanup macros
# undef NUM
# undef F1
# undef RESULT
# undef RETURN_POINT

// ============================================================================
// Main Program
// ============================================================================

int main(int argc, char *argv[])
{
  try
    {
      TCLAP::CmdLine cmd(
        "Fibonacci number computation using three methods:\n"
        "  recursive  - Classic recursive (slow for large n)\n"
        "  iterative  - Bottom-up loop (fast)\n"
        "  stack      - Explicit activation records (educational)\n",
        ' ', "1.0");

      TCLAP::ValueArg<int> nArg("n", "number",
                                 "Fibonacci index to compute",
                                 false, 10, "int");
      cmd.add(nArg);

      vector<string> allowedMethods = {"all", "recursive", "iterative", "stack"};
      TCLAP::ValuesConstraint<string> methodConstraint(allowedMethods);
      TCLAP::ValueArg<string> methodArg("m", "method",
                                         "Method to use (all, recursive, iterative, stack)",
                                         false, "all", &methodConstraint);
      cmd.add(methodArg);

      TCLAP::SwitchArg timeArg("t", "time",
                                "Show execution time for each method",
                                false);
      cmd.add(timeArg);

      cmd.parse(argc, argv);

      int n = nArg.getValue();
      string method = methodArg.getValue();
      bool showTime = timeArg.getValue();

      cout << "Fibonacci Number Computation" << endl;
      cout << "============================" << endl;
      cout << "Computing fib(" << n << ")" << endl << endl;

      // Warning for large n with recursive method
      if ((method == "all" || method == "recursive") && n > 40)
        {
          cout << "WARNING: n > 40 with recursive method will be very slow!" << endl;
          cout << "         Consider using -m iterative or -m stack" << endl << endl;
        }

      auto measure_time = [](auto func, int n, const string& name, bool show) {
        auto start = chrono::high_resolution_clock::now();
        long long result = func(n);
        auto end = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
        
        cout << name << ": fib(" << n << ") = " << result;
        if (show)
          cout << "  [" << duration.count() << " us]";
        cout << endl;
        
        return result;
      };

      long long result_iter = 0, result_rec = 0, result_stack = 0;

      if (method == "all" || method == "iterative")
        {
          result_iter = measure_time(fib_iterative, n, "Iterative", showTime);
        }

      if (method == "all" || method == "stack")
        {
          result_stack = measure_time(fib_stack, n, "Stack    ", showTime);
        }

      if (method == "all" || method == "recursive")
        {
          if (n <= 40)  // Skip recursive for large n
            {
              result_rec = measure_time(fib_recursive, n, "Recursive", showTime);
            }
          else
            {
              cout << "Recursive: SKIPPED (n > 40 too slow)" << endl;
            }
        }

      // Verify all methods give same result
      if (method == "all" && n <= 40)
        {
          cout << endl;
          if (result_iter == result_rec && result_rec == result_stack)
            cout << "Verification: All methods agree!" << endl;
          else
            cout << "ERROR: Methods disagree!" << endl;
        }

      cout << endl << "Done." << endl;
    }
  catch (TCLAP::ArgException &e)
    {
      cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
      return 1;
    }

  return 0;
}
