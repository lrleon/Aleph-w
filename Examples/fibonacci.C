
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


/**
 * @file fibonacci.C
 * @brief Fibonacci number computation: Three implementation strategies compared
 * 
 * This example demonstrates three fundamentally different approaches to computing
 * Fibonacci numbers, each illustrating important programming concepts and trade-offs.
 * The Fibonacci sequence is one of the most famous sequences in mathematics and
 * computer science, making it perfect for teaching algorithm design.
 *
 * ## The Fibonacci Sequence
 *
 * Defined recursively as:
 * - F(0) = 0
 * - F(1) = 1
 * - F(n) = F(n-1) + F(n-2) for n > 1
 *
 * Sequence: 0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, ...
 *
 * ## Three Implementation Strategies
 *
 * ### 1. Recursive (fib_recursive)
 *
 * **Approach**: Direct translation of mathematical definition
 *
 * **Code Structure**:
 * ```cpp
 * if (n <= 1) return n;
 * return fib(n-1) + fib(n-2);
 * ```
 *
 * **Complexity**:
 * - **Time**: O(2^n) - exponential! (many redundant calculations)
 * - **Space**: O(n) - call stack depth
 *
 * **Pros**:
 * - Simple, intuitive code
 * - Matches mathematical definition
 * - Easy to understand
 *
 * **Cons**:
 * - Extremely slow for large n
 * - Redundant calculations (F(3) computed many times)
 * - Stack overflow risk for large n
 *
 * **Best for**: Educational purposes, small n, understanding recursion
 *
 * ### 2. Iterative (fib_iterative)
 *
 * **Approach**: Bottom-up computation using a loop
 *
 * **Code Structure**:
 * ```cpp
 * int a = 0, b = 1;
 * for (int i = 2; i <= n; i++) {
 *   int temp = a + b;
 *   a = b;
 *   b = temp;
 * }
 * return b;
 * ```
 *
 * **Complexity**:
 * - **Time**: O(n) - linear, optimal!
 * - **Space**: O(1) - constant, only two variables
 *
 * **Pros**:
 * - Fast and efficient
 * - Minimal memory usage
 * - No stack overflow risk
 *
 * **Cons**:
 * - Less intuitive than recursive version
 * - Doesn't match mathematical definition directly
 *
 * **Best for**: Production code, large n, performance-critical applications
 *
 * ### 3. Stack-based with Activation Records (fib_stack)
 *
 * **Approach**: Simulates recursive call stack explicitly using ArrayStack
 *
 * **Key Concept**: Shows how recursion works internally
 *
 * **What It Demonstrates**:
 * - **Activation records**: Stack frames storing local variables and return addresses
 * - **Call stack management**: How function calls are tracked
 * - **Return point management**: Continuation-style programming
 * - **Compiler internals**: How compilers transform recursion
 *
 * **Complexity**:
 * - **Time**: O(n) - similar to iterative (no redundant work)
 * - **Space**: O(n) - explicit stack (like recursive, but controlled)
 *
 * **Educational Value**:
 * - Understand recursion at a low level
 * - See how tail-call optimization could work
 * - Learn about continuation-passing style
 * - Understand compiler transformations
 *
 * **Best for**: Learning how recursion works, understanding call stacks
 *
 * ## Performance Comparison
 *
 * | Method | Time | Space | Redundant Work |
 * |--------|------|-------|----------------|
 * | Recursive | O(2^n) | O(n) | Yes (exponential) |
 * | Iterative | O(n) | O(1) | No |
 * | Stack-based | O(n) | O(n) | No |
 *
 * ## When to Use Each
 *
 * - **Recursive**: Never in production (too slow), only for learning
 * - **Iterative**: Always in production (fastest, simplest)
 * - **Stack-based**: Educational purposes, understanding recursion
 *
 * ## Advanced Optimizations
 *
 * For even better performance, consider:
 * - **Matrix exponentiation**: O(log n) time using matrix powers
 * - **Memoization**: Cache computed values (O(n) time, O(n) space)
 * - **Closed-form formula**: Binet's formula (O(1) but floating-point precision issues)
 *
 * ## Usage Examples
 *
 * ```bash
 * # Compute F(10) using all methods
 * fibonacci -n 10
 *
 * # Compare performance (recursive will be slow!)
 * fibonacci -n 30
 *
 * # Use specific method
 * fibonacci -n 20 -m iterative
 * fibonacci -n 15 -m stack
 * ```
 *
 * @see fib.C Similar example with different implementation details
 * @see tpl_arrayStack.H Stack implementation used for stack-based version
 * @author Leandro Rabindranath León
 * @ingroup Examples
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
