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
    if (msg.find("size mismatch") != string::npos &&
        msg.find("test_tpl_hash_macros.cc") != string::npos) {
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