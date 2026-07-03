
/*
                          Aleph_w

  Data structures & Algorithms
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
 * @file generator_example.cc
 * @brief Aleph::Generator<T>: lazy sequences with C++20 coroutines.
 *
 * ## Overview
 *
 * `Aleph::Generator<T>` (see `ah-generator.H`) turns a function that
 * `co_yield`s values into a lazy, single-pass sequence you can drive with an
 * ordinary range-`for`. This example walks through the three reasons to
 * reach for it:
 *
 * - **Laziness**: sequences that are infinite, or merely huge, can be
 *   expressed directly — nothing is computed until the consumer asks for
 *   the next value, and `break` stops production outright.
 * - **Exception propagation**: a coroutine body can validate as it goes and
 *   throw partway through; the exception surfaces at the point the consumer
 *   resumed it, exactly like a hand-written iterator would.
 * - **Composability**: a generator function can itself consume another
 *   generator with a plain range-`for` and re-`co_yield`, chaining lazy
 *   stages the way `map`/`filter` chain in `ahFunctional.H`.
 *
 * @author Leandro Rabindranath Leon
 * @ingroup Examples
 */

#include <cctype>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <ah-generator.H>
#include <print_rule.H>

using namespace Aleph;

namespace
{

// An infinite sequence: 0, 1, 2, 3, ... forever. There is no eager
// equivalent — you cannot materialize an infinite std::vector.
Generator<long> natural_numbers()
{
  for (long n = 0; ; ++n)
    co_yield n;
}

void demo_laziness()
{
  std::cout << "[1] Laziness: an infinite sequence, truncated on demand\n";
  print_rule();

  std::cout << "First 5 multiples of 7, taken from an unbounded generator:\n";
  long checked = 0;
  long found = 0;
  for (long n : natural_numbers())
    {
      ++checked;
      if (n % 7 != 0)
        continue;
      std::cout << "  " << n << "\n";
      if (++found == 5)
        break;   // stops the coroutine outright; the rest is never produced
    }
  std::cout << "Candidates examined before stopping: " << checked << "\n";
  std::cout << "The generator function never runs past that point — there is\n"
               "no way to \"materialize\" natural_numbers() into a container.\n\n";
}

// True if `tok` is a full optional-sign, all-digits integer literal.
bool is_plain_integer(const std::string &tok)
{
  if (tok.empty())
    return false;
  size_t i = (tok[0] == '+' or tok[0] == '-') ? 1 : 0;
  if (i == tok.size())
    return false;
  for (; i < tok.size(); ++i)
    if (not std::isdigit(static_cast<unsigned char>(tok[i])))
      return false;
  return true;
}

// A validating generator: parses each token to an int, and throws on the
// first one that isn't a valid number instead of silently skipping it.
//
// `tokens` is taken BY VALUE on purpose, not by reference: a coroutine's
// body doesn't run until the first resume (initial_suspend), so a
// reference parameter bound to a caller's temporary (e.g.
// `parse_ints({"10", "20"})`) would already be dangling by the time this
// loop actually executes. Taking `tokens` by value sidesteps that pitfall
// the same way `ah-comb-generators.H`'s `lazy_permutations`/
// `lazy_combinations` do.
Generator<int> parse_ints(std::vector<std::string> tokens)
{
  for (const std::string &tok : tokens)
    {
      ah_invalid_argument_if(not is_plain_integer(tok))
        << "not a valid integer: \"" << tok << "\"";
      int value = std::stoi(tok);
      co_yield value;
    }
}

void demo_exception_propagation()
{
  std::cout << "[2] Exception propagation across suspension points\n";
  print_rule();

  const std::vector<std::string> tokens = {"10", "20", "30", "oops", "40"};
  std::cout << "Parsing tokens: 10, 20, 30, oops, 40\n";

  int sum = 0;
  try
    {
      for (int v : parse_ints(tokens))
        {
          std::cout << "  parsed " << v << "\n";
          sum += v;
        }
    }
  catch (const std::invalid_argument &e)
    {
      std::cout << "  caught: " << e.what() << "\n";
    }
  std::cout << "Sum of values parsed before the error: " << sum << " (expect 60)\n";
  std::cout << "The three valid tokens were already consumed and summed; the\n"
               "exception surfaced exactly where the fourth co_yield would\n"
               "have been, just like a hand-written iterator would throw.\n\n";
}

// Two small composable stages, each a generator consuming another.
Generator<long> evens_only(Generator<long> src)
{
  for (long x : src)
    if (x % 2 == 0)
      co_yield x;
}

Generator<long> squared(Generator<long> src)
{
  for (long x : src)
    co_yield x * x;
}

void demo_composition()
{
  std::cout << "[3] Composition: chaining lazy stages\n";
  print_rule();

  std::cout << "squared(evens_only(natural_numbers())), first 6 values:\n";
  int count = 0;
  for (long v : squared(evens_only(natural_numbers())))
    {
      std::cout << "  " << v << "\n";
      if (++count == 6)
        break;
    }
  std::cout << "Expect: 0, 4, 16, 36, 64, 100 — each stage only pulls as\n"
               "many values from its source as the next stage asks for.\n\n";
}
} // namespace

int main()
{
  std::cout << "\n=== Aleph::Generator<T>: lazy sequences ===\n\n";

  demo_laziness();
  demo_exception_propagation();
  demo_composition();

  std::cout << "Done.\n";
  return 0;
}