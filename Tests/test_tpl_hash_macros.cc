
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
 * @file test_tpl_hash_macros.cc
 * @brief Tests for Test Tpl Hash Macros
 */

/**
 * @file test_tpl_hash_macros.cc
 * @brief Quick test to verify tpl_hash.H works with new error macros
 */

#include <iostream>
#include <cassert>
#include "../tpl_hash.H"

using namespace std;
using namespace Aleph;

int main()
{
  cout << "Testing tpl_hash.H with new error macros..." << endl;

  // Test 1: Normal usage should work
  HashMap<int, string> map1({1, 2, 3}, {"one", "two", "three"});
  assert(map1.size() == 3);
  cout << "Test 1 passed: Normal constructor works" << endl;

  // Test 2: Mismatched sizes should throw range_error
  try {
    HashMap<int, string> map2({1, 2, 3}, {"one", "two"}); // Mismatched!
    cerr << "FAIL: Expected range_error was not thrown" << endl;
    return 1;
  } catch (const range_error& e) {
    string msg = e.what();
    // Error message should contain "size mismatch" - location can be in the
    // header file where the error is thrown
    if (msg.find("size mismatch") != string::npos) {
      cout << "Test 2 passed: range_error thrown correctly" << endl;
      cout << "  Message: " << msg << endl;
    } else {
      cerr << "FAIL: Wrong error message: " << msg << endl;
      return 1;
    }
  }

  cout << endl << "SUCCESS: All tests passed!" << endl;
  return 0;
}