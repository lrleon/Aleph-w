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
 * @ingroup Examples
*/

/**
 * @file hash_resize.C
 * @brief Example demonstrating hash table with automatic resizing
 * 
 * This example demonstrates how `MapOLhash` (Open Addressing Hash Table)
 * automatically resizes itself to maintain good performance as elements
 * are inserted. Automatic resizing is crucial for maintaining O(1) average
 * performance in hash tables.
 *
 * ## Why Resizing Matters
 *
 * ### Load Factor Impact
 *
 * Hash table performance degrades when the **load factor** (elements/buckets)
 * becomes too high:
 *
 * - **Low load factor** (< 0.5): Many empty buckets, wasted memory
 * - **Optimal load factor** (0.7-0.8): Good balance of speed and memory
 * - **High load factor** (> 0.9): Many collisions, performance degrades
 *
 * ### Performance Degradation
 *
 * Without resizing:
 * - **Operations degrade**: From O(1) average to O(n) worst case
 * - **Collisions increase**: More elements compete for same buckets
 * - **Linear probing**: Longer probe sequences
 *
 * ### Common Thresholds
 *
 * | Hash Table Type | Resize Threshold | Reason |
 * |-----------------|------------------|--------|
 * | Open addressing | Load factor > 0.7-0.8 | Collisions become frequent |
 * | Separate chaining | Load factor > 1.0-2.0 | Chains become too long |
 *
 * ## Automatic Resizing Strategy
 *
 * ### How MapOLhash Resizes
 *
 * `MapOLhash` uses open addressing (linear probing or double hashing):
 *
 * 1. **Initial size**: Starts with a small number of buckets
 * 2. **Load monitoring**: Tracks load factor during insertions
 * 3. **Threshold check**: Checks if load factor exceeds threshold
 * 4. **Automatic resize**: When threshold exceeded:
 *    - Allocates larger bucket array (typically 2× size)
 *    - Rehashes all existing elements (O(n) operation)
 *    - Updates hash table structure
 *    - Continues insertion
 *
 * ### Resize Operation
 *
 * ```
 * Resize(old_size, new_size):
 *   1. Allocate new bucket array of size new_size
 *   2. For each element in old array:
 *      - Compute new hash (using new_size)
 *      - Insert into new array
 *   3. Deallocate old array
 *   4. Update hash table size
 * ```
 *
 * **Cost**: O(n) where n = number of elements
 * **Frequency**: O(log n) times (each resize doubles size)
 * **Amortized cost**: O(1) per insertion
 *
 * ## What This Example Demonstrates
 *
 * 1. **Insertion**: Adding many elements to the hash table
 * 2. **Automatic resizing**: Observing resize operations as load increases
 * 3. **Verification**: Ensuring all elements are still accessible after resize
 * 4. **Performance**: Demonstrating O(1) average access maintained
 * 5. **Load factor tracking**: Monitoring load factor over time
 *
 * ## Key Operations
 *
 * - **`insert(key, value)`**: Insert key-value pair (may trigger resize)
 * - **`search(key)`**: Find value by key (O(1) average)
 * - **`size()`**: Get current number of elements
 * - **`capacity()`**: Get current number of buckets
 * - **`load_factor()`**: Get current load factor
 *
 * ## Resize Behavior
 *
 * ### When Resize Occurs
 *
 * - **Trigger**: Load factor exceeds threshold (typically 0.75)
 * - **Frequency**: Logarithmic (each resize doubles size)
 * - **Cost**: O(n) per resize, but amortized O(1) per insertion
 *
 * ### Resize Size
 *
 * Common strategies:
 * - **Double**: new_size = 2 × old_size (most common)
 * - **Prime**: Use next prime number (better for some hash functions)
 * - **Power of 2**: new_size = 2^k (fast modulo)
 *
 * ## Performance Characteristics
 *
 * | Operation | Before Resize | After Resize | Amortized |
 * |-----------|---------------|--------------|-----------|
 * | Insert | O(1) → O(n) | O(1) | O(1) |
 * | Search | O(1) → O(n) | O(1) | O(1) |
 * | Delete | O(1) → O(n) | O(1) | O(1) |
 *
 * ## Usage
 *
 * ```bash
 * # Insert 1000 elements and observe resizing
 * ./hash_resize -n 1000
 *
 * # Insert 10000 elements
 * ./hash_resize -n 10000
 * ```
 *
 * ## Expected Output
 *
 * The program prints progress during insertion and then verifies that every
 * inserted element is still accessible after all insertions (resizes, if any,
 * are handled internally by `MapOLhash`).
 *
 * @see tpl_dynMapOhash.H Open addressing hash map implementation
 * @see hash_tables_example.C Hash table implementations comparison
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

# include <tclap/CmdLine.h>
# include <tpl_dynMapOhash.H>
# include <iostream>
# include <cassert>

using namespace std;
using namespace Aleph;

struct Foo
{
  string val;
  int i;

  Foo() : val(), i(0) {}
  Foo(int num) : val(to_string(num)), i(num) {}

  bool operator == (const Foo & foo) const
  {
    return i == foo.i and val == foo.val;
  }
};

MapOLhash<int, Foo> tbl;
DynArray<Foo> backup;

void fill(size_t n)
{
  cout << "Inserting " << n << " elements into hash table..." << endl;
  for (size_t i = 0; i < n; ++i)
    {
      Foo foo(i);
      auto ptr = tbl.insert(i, foo);  // Insert copy
      assert(ptr != nullptr);
      backup.append(foo);  // Append another copy
      
      if (i % 100 == 0)
        cout << "  Inserted " << i << " elements" << endl;
    }
  cout << "Insertion complete. Table size: " << tbl.size() << endl;
}

void verify()
{
  cout << "Verifying all elements..." << endl;
  size_t count = 0;
  for (auto it = backup.get_it(); it.has_curr(); it.next())
    {
      const Foo & foo = it.get_curr();
      auto ptr = tbl.search(foo.i);
      assert(ptr != nullptr);
      assert(ptr->first == foo.i);
      assert(ptr->second == foo);
      ++count;
    }
  cout << "Verification complete. " << count << " elements verified." << endl;
}

int main(int argc, char **argv)
{
  try 
    {
      TCLAP::CmdLine cmd("Hash table resize example", ' ', "1.0");

      TCLAP::ValueArg<size_t> nArg("n", "count", 
                                    "Number of keys to insert",
                                    false, 1000, "size_t");
      cmd.add(nArg);

      cmd.parse(argc, argv);

      size_t n = nArg.getValue();

      cout << "Hash Resize Example" << endl;
      cout << "===================" << endl;
      cout << "Testing with " << n << " elements" << endl << endl;

      fill(n);
      cout << endl;
      verify();

      cout << endl << "Test passed successfully!" << endl;
    }
  catch (TCLAP::ArgException &e)
    {
      cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
      return 1;
    }

  return 0;
}
