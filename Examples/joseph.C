
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
 * @file joseph.C
 * @brief Josephus Problem: Classic elimination puzzle solved with linked lists
 *
 * The Josephus problem is a famous theoretical problem dating back to
 * ancient times. It asks: if n people stand in a circle and every k-th
 * person is eliminated, which position survives? This problem has fascinated
 * mathematicians for centuries and demonstrates elegant algorithmic solutions.
 *
 * ## Historical Context
 *
 * ### The Legend
 *
 * Named after **Flavius Josephus**, a Jewish historian who, according to legend,
 * used this strategy to save himself and a friend from capture during the
 * Siege of Yodfat (67 CE). The problem has been studied for centuries and
 * appears in many mathematical contexts.
 *
 * ### Mathematical Significance
 *
 * The Josephus problem is:
 * - **Historically important**: One of the oldest algorithmic problems
 * - **Mathematically interesting**: Has closed-form solutions
 * - **Algorithmically elegant**: Demonstrates circular data structures
 * - **Recursively solvable**: Can be solved with recurrence relations
 *
 * ## Problem Statement
 *
 * ### Setup
 *
 * Given n people numbered 1 to n arranged in a circle:
 * 1. Start counting from person 1
 * 2. Count k people clockwise
 * 3. Eliminate the k-th person
 * 4. Continue counting from the next person
 * 5. Repeat until only one person remains
 *
 * ### Question
 *
 * **What is the position of the survivor?**
 *
 * ### Example: n=7, k=3
 *
 * ```
 * Round 1: [1, 2, 3, 4, 5, 6, 7]
 *          Count: 1→2→3, eliminate 3
 *          Remaining: [1, 2, 4, 5, 6, 7]
 *
 * Round 2: [1, 2, 4, 5, 6, 7]
 *          Count: 4→5→6, eliminate 6
 *          Remaining: [1, 2, 4, 5, 7]
 *
 * Round 3: [1, 2, 4, 5, 7]
 *          Count: 7→1→2, eliminate 2
 *          Remaining: [1, 4, 5, 7]
 *
 * ... continue until one remains
 * ```
 *
 * ## Algorithm
 *
 * ### Implementation Strategy
 *
 * This implementation uses `DynDlist` (doubly-linked list) to simulate the
 * elimination process, and manually wraps the iterator to the first element
 * when it reaches the end (to model the circle):
 *
 * ```
 * josephus(n, k):
 *   1. Create list with persons 1 to n
 *   2. current = first person
 *   3. While list.size() > 1:
 *      a. Advance iterator k-1 positions (skip k-1 people)
 *      b. Remove person at current position
 *      c. Continue from next person (circular wrap-around)
 *   4. Return remaining person
 * ```
 *
 * ### Why a List + Wrap-around Iterator?
 *
 * - **Natural fit**: Problem is inherently circular
 * - **Efficient removal**: O(1) removal at the iterator position
 * - **Wrap-around**: Implemented by resetting the iterator to the first element
 * - **Simple**: Easy to implement and understand
 *
 * ## Time Complexity
 *
 * ### Approaches
 *
 * | Approach | Time Complexity | Notes |
 * |-----------|----------------|-------|
 * | Naive simulation | O(n × k) | Simulate each elimination |
 * | Circular list | O(n × k) | But efficient in practice |
 * | Recurrence relation | O(n) | Mathematical solution |
 * | Closed form | O(1) | Formula-based (complex) |
 *
 * ### This Implementation
 *
 * - **Time**: O(n × k) - n eliminations, k steps each
 * - **Space**: O(n) - store n people
 * - **Practical**: Efficient for reasonable n and k
 *
 * ## Mathematical Solution
 *
 * ### Recurrence Relation
 *
 * The Josephus problem has a recurrence relation:
 * ```
 * J(n, k) = (J(n-1, k) + k) mod n
 * J(1, k) = 0
 * ```
 *
 * Where J(n, k) is the 0-indexed position of the survivor.
 *
 * ### Closed-Form Formula
 *
 * For k=2, there's a closed-form solution:
 * ```
 * J(n, 2) = 2 × (n - 2^floor(log2(n))) + 1
 * ```
 *
 * For general k, the formula is more complex.
 *
 * ## Applications
 *
 * ### Algorithm Design
 * - **Circular data structures**: Demonstrates circular lists
 * - **Elimination algorithms**: Pattern for elimination problems
 * - **Recursive thinking**: Shows recursive problem structure
 *
 * ### Game Theory
 * - **Elimination games**: Model elimination tournaments
 * - **Strategies**: Analyze optimal strategies
 * - **Fairness**: Study fairness of elimination processes
 *
 * ### Cryptography
 * - **Encryption schemes**: Some schemes use similar patterns
 * - **Key generation**: Generate keys using elimination patterns
 *
 * ### Resource Allocation
 * - **Round-robin elimination**: Allocate resources fairly
 * - **Process scheduling**: Schedule processes in rounds
 * - **Load balancing**: Distribute load in circular fashion
 *
 * ### Real-World Examples
 * - **Musical chairs**: Children's game
 * - **Elimination tournaments**: Sports tournaments
 * - **Survivor shows**: TV show elimination format
 *
 * ## Example Walkthrough
 *
 * ### n=7, k=3 (Detailed)
 *
 * ```
 * Initial: [1, 2, 3, 4, 5, 6, 7]
 * 
 * Round 1: Count 1→2→3, eliminate 3
 *          [1, 2, 4, 5, 6, 7]
 * 
 * Round 2: Count 4→5→6, eliminate 6
 *          [1, 2, 4, 5, 7]
 * 
 * Round 3: Count 7→1→2, eliminate 2
 *          [1, 4, 5, 7]
 * 
 * Round 4: Count 4→5→7, eliminate 7
 *          [1, 4, 5]
 * 
 * Round 5: Count 1→4→5, eliminate 5
 *          [1, 4]
 * 
 * Round 6: Count 1→4→1, eliminate 1
 *          [4]
 * 
 * Survivor: Position 4
 * ```
 *
 * ## Usage
 *
 * ```bash
 * # 7 people, eliminate every 3rd
 * joseph -n 7 -p 3
 *
 * # Classic problem: 41 people, every 3rd eliminated
 * joseph -n 41 -p 3
 *
 * # Different elimination step
 * joseph -n 10 -p 5
 * ```
 *
 * ## Special Cases
 *
 * ### k=1
 * - **Trivial**: Eliminate sequentially
 * - **Survivor**: Last person (position n)
 *
 * ### k=2
 * - **Special case**: Has closed-form solution
 * - **Pattern**: Survivor position follows pattern
 *
 * ### k ≥ n
 * - **Wrap-around**: Counting wraps around circle
 * - **Equivalent**: k mod n
 *
 * ## Extensions
 *
 * ### Variations
 *
 * - **Reverse counting**: Count counter-clockwise
 * - **Skip pattern**: Variable skip count
 * - **Multiple survivors**: Keep k survivors instead of 1
 * - **Different starting**: Start from different position
 *
 * @see tpl_dynDlist.H Circular doubly-linked list implementation
 * @see linear_structures_example.C List structures (related)
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

#include "tpl_dynDlist.H"
#include <tclap/CmdLine.h>
#include <cstdio>
#include <iostream>

using namespace Aleph;

void avanceItor(DynDlist<unsigned>::Iterator& itor, unsigned s)
{
  for (unsigned i = 0; i < s; i++)
    {
      if (not itor.has_curr())
        itor.reset_first();

      itor.next();
    }

  if (not itor.has_curr())
    itor.reset_first();
}
void orden_ejecucion(unsigned num_personas, unsigned paso)
{
  DynDlist<unsigned> list;

  if (paso == 0)
    {
      printf("Step size must be >= 1\n");
      return;
    }

  for (unsigned i = 1; i <= num_personas; i++)
    list.append(i);
      
  DynDlist<unsigned>::Iterator itor(list);
  for (/* nothing */; num_personas > 1; num_personas--)
    {
      avanceItor(itor, paso - 1);
      printf("%u ", itor.get_curr());
      itor.del();
    }

  printf("\nSurvivor is %u\n", list.get_first());
}

int main(int argc, char** argv)
{
  try {
    TCLAP::CmdLine cmd("Josephus problem", ' ', "1.0");

    TCLAP::ValueArg<unsigned> numPerArg("n", "num-persons",
                                         "Number of persons",
                                         false, 20, "unsigned");
    cmd.add(numPerArg);

    TCLAP::ValueArg<unsigned> pasoArg("p", "paso",
                                        "Step size",
                                        false, 7, "unsigned");
    cmd.add(pasoArg);

    cmd.parse(argc, argv);

    unsigned numPer = numPerArg.getValue(),
             paso   = pasoArg.getValue();

    orden_ejecucion(numPer, paso);
  } catch (TCLAP::ArgException &e) {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
  }
  return 0;
}
