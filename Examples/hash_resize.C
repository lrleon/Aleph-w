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
 * @file hash_resize.C
 * @brief Example demonstrating hash table with automatic resizing
 * 
 * This example shows how MapOLhash handles insertions and automatic
 * resizing when the load factor increases.
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
