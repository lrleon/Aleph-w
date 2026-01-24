
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