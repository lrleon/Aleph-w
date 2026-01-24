
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
 * @file fib.C
 * @brief Fibonacci sequence: Three implementation strategies compared
 *
 * This example demonstrates three different approaches to computing the
 * Fibonacci sequence, each illustrating important programming concepts.
 * The Fibonacci sequence is one of the most famous sequences in mathematics
 * and provides excellent examples of different algorithmic approaches.
 *
 * ## Fibonacci Sequence
 *
 * ### Mathematical Definition
 *
 * The Fibonacci sequence is defined recursively as:
 * - **F(0) = 1**
 * - **F(1) = 1**
 * - **F(n) = F(n-1) + F(n-2)** for n > 1
 *
 * ### Sequence Values
 *
 * Sequence: 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, ...
 *
 * ### Mathematical Properties
 *
 * - **Golden ratio**: Ratio F(n+1)/F(n) approaches φ ≈ 1.618
 * - **Binet's formula**: Closed-form expression exists
 * - **Many applications**: Nature, art, mathematics, computer science
 *
 * ## Three Implementations
 *
 * ### 1. Iterative (`fib_it`)
 *
 * **Strategy**: Use loop with two variables to track previous values
 *
 * **Algorithm**:
 * ```
 * fib_it(n):
 *   if n <= 1: return 1
 *   prev2 = 1, prev1 = 1
 *   for i = 2 to n:
 *     current = prev1 + prev2
 *     prev2 = prev1
 *     prev1 = current
 *   return current
 * ```
 *
 * **Characteristics**:
 * - **Time**: O(n) - linear time
 * - **Space**: O(1) - constant space
 * - **Efficiency**: Most efficient
 * - **Best for**: Production code, large n
 *
 * ### 2. Recursive (`fib_rec`)
 *
 * **Strategy**: Direct translation of mathematical definition
 *
 * **Algorithm**:
 * ```
 * fib_rec(n):
 *   if n <= 1: return 1
 *   return fib_rec(n-1) + fib_rec(n-2)
 * ```
 *
 * **Problem**: Many redundant calculations!
 *
 * **Call tree for F(5)**:
 * ```
 *                    F(5)
 *                   /    \
 *                F(4)    F(3)
 *               /   \    /  \
 *            F(3)  F(2) F(2) F(1)
 *           ... (many duplicate calls)
 * ```
 *
 * **Characteristics**:
 * - **Time**: O(2^n) - exponential! (terrible)
 * - **Space**: O(n) - call stack depth
 * - **Redundancy**: Calculates same values many times
 * - **Best for**: Educational purposes, small n only
 *
 * ### 3. Stack-based (`fib_st`)
 *
 * **Strategy**: Simulate recursion using explicit stack
 *
 * **Algorithm**:
 * ```
 * fib_st(n):
 *   stack.push({n, return_point=P0})
 *   while stack not empty:
 *     item = stack.top()
 *     if item.n <= 1:
 *       item.result = 1
 *       stack.pop()
 *     else if item.return_point == P0:
 *       item.return_point = P1
 *       stack.push({n-1, return_point=P0})
 *     else if item.return_point == P1:
 *       item.f1 = result from previous call
 *       item.return_point = P2
 *       stack.push({n-2, return_point=P0})
 *     else:  // P2
 *       item.result = item.f1 + result from previous call
 *       stack.pop()
 * ```
 *
 * **Characteristics**:
 * - **Time**: O(n) - similar to iterative
 * - **Space**: O(n) - explicit stack
 * - **Educational**: Shows how recursion works internally
 * - **Best for**: Understanding recursion mechanics
 *
 * ## Complexity Comparison
 *
 * | Implementation | Time | Space | Redundancy | Best For |
 * |----------------|------|-------|------------|----------|
 * | Iterative | O(n) | O(1) | None | Production |
 * | Recursive | O(2^n) | O(n) | Massive | Education |
 * | Stack-based | O(n) | O(n) | None | Learning |
 *
 * ### Performance Example
 *
 * Computing F(40):
 * - **Iterative**: ~40 operations, instant
 * - **Recursive**: ~2^40 operations, very slow!
 * - **Stack-based**: ~40 operations, fast
 *
 * ## Educational Value
 *
 * ### Concepts Demonstrated
 *
 * This example teaches:
 * - **Algorithm design**: Different approaches to same problem
 * - **Performance analysis**: Time/space complexity comparison
 * - **Recursion mechanics**: How call stacks work internally
 * - **Stack data structures**: Using stacks to simulate recursion
 * - **Optimization**: Why naive recursion is inefficient
 *
 * ### Learning Outcomes
 *
 * After studying this example, you understand:
 * - Why naive recursion can be inefficient
 * - How to optimize recursive algorithms
 * - How recursion is implemented internally
 * - When to use iterative vs recursive approaches
 *
 * ## Optimizations
 *
 * ### Memoization (Not Shown)
 *
 * Can optimize recursive version with memoization:
 * ```
 * memo = {}
 * fib_memo(n):
 *   if n in memo: return memo[n]
 *   if n <= 1: return 1
 *   memo[n] = fib_memo(n-1) + fib_memo(n-2)
 *   return memo[n]
 * ```
 *
 * **Time**: O(n) - each value calculated once
 * **Space**: O(n) - memoization table
 *
 * ### Matrix Exponentiation (Advanced)
 *
 * Can compute F(n) in O(log n) time using matrix exponentiation:
 * ```
 * [F(n+1)]   [1 1]^n [1]
 * [F(n)  ] = [1 0]   [1]
 * ```
 *
 * **Time**: O(log n) - very fast!
 *
 * ## Applications of Fibonacci
 *
 * ### Nature
 * - **Phyllotaxis**: Leaf arrangement, flower petals
 * - **Spiral patterns**: Pine cones, sunflowers
 * - **Growth patterns**: Population growth models
 *
 * ### Computer Science
 * - **Algorithm analysis**: Example of exponential recursion
 * - **Dynamic programming**: Classic DP example
 * - **Golden ratio search**: Optimization algorithms
 *
 * ### Mathematics
 * - **Number theory**: Many interesting properties
 * - **Combinatorics**: Counting problems
 * - **Graph theory**: Fibonacci graphs
 *
 * ## Usage
 *
 * ```bash
 * # Compute F(10) using all three methods
 * ./fib 10
 *
 * # Compare performance (recursive will be slow!)
 * ./fib 20
 *
 * # See exponential growth of recursive version
 * ./fib 30  # Recursive takes much longer
 * ```
 *
 * ## Performance Warnings
 *
 * ⚠️ **Warning**: The recursive version is extremely slow for large n!
 * - F(30): Takes seconds
 * - F(40): Takes minutes
 * - F(50): Takes hours/days
 *
 * Use iterative or stack-based for large values.
 *
 * @see fibonacci.C More comprehensive Fibonacci examples
 * @see tpl_arrayStack.H Stack implementation used for stack-based version
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

# include <stdlib.h>
# include <iostream>
# include <tpl_arrayStack.H>

using namespace std;
using namespace Aleph;

int fib_it(int n)
{
  if (n <= 1)
    return 1;

  int fi_1 = 1, fi_2 = 1, fi = 0, i;

  for (i = 2; i <= n; i++, fi_2 = fi_1, fi_1 = fi)
    fi = fi_1 + fi_2;

  return fi;
}

int fib_rec(int n)
{
  if (n <= 1)
    return 1;

  int f1 = fib_rec(n - 1);
  int f2 = fib_rec(n - 2);
  return f1 + f2;
}


# define P0 0
# define P1 1
# define P2 2

struct Item
{
  int n;
  int f1;
  int result;
  char return_point;
};

# define N  (stack.top().n)
# define F1 (stack.top().f1)
# define RESULT (stack.top().result)
# define RETURN_POINT (stack.top().return_point)

int fib_st(int n)
{
  ArrayStack<Item> stack;
  Item first_activation_register;
  Item current_activation_register;

  first_activation_register.n = n;

  stack.push(first_activation_register);

  while (1)
    {
      if (N <= 1)
	{
	  current_activation_register = stack.pop();
	  current_activation_register.result = 1;
	  stack.push(current_activation_register);
	  goto exit_from_fib;
	}

      { /* Esta es la llamada a fib(n - 1) */
	Item new_activation_register;
	new_activation_register.n     = N - 1;
	new_activation_register.return_point = P1;
	stack.push(new_activation_register);
	continue;
      }

    p1: /* Esta es la salida de fib(n - 1) */
      current_activation_register = stack.pop();
      current_activation_register.f1 = current_activation_register.result; /* int f1 = fib(n - 1) */
      stack.push(current_activation_register);

      { /* Esta es la llamada a fib(n - 2) */
	Item new_activation_register;
	new_activation_register.n = N - 2;
	new_activation_register.return_point = P2;
	stack.push(new_activation_register);
	continue;
      }

    p2: /* Esta es la salida de fib(n - 2) */
      current_activation_register = stack.pop();
      current_activation_register.result =
	current_activation_register.f1 + current_activation_register.result;
      stack.push(current_activation_register);

    exit_from_fib:
      if (stack.size() == 1)
	return RESULT;

      current_activation_register = stack.pop();
      char return_point = current_activation_register.return_point;

      // Propagate result to caller
      int res = current_activation_register.result;
      Item caller = stack.pop();
      caller.result = res;
      stack.push(caller);

      switch (return_point)
	{
	case P1: goto p1;
	case P2: goto p2;
	}
    }
}



int main(int argn, char * argv[])
{
  if (argn != 2) {
    cout << "usage: " << argv[0] << " n" << endl;
    return 1;
  }
  int n = atoi(argv[1]);

  cout << "fib(" << n << ") = " << fib_rec(n) << " = " << fib_it(n)
       << " = " << fib_st(n) << endl;
}
