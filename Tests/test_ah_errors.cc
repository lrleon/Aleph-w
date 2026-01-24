/**
 * @file test_ah_errors.cc
 * @brief Comprehensive tests for ah-errors.H exception handling macros
 * @author Test suite for Aleph-w
 */

#include <iostream>
#include <sstream>
#include <cassert>
#include <stdexcept>
#include <string>
#include "../ah-errors.H"

using namespace std;

// Test counter
int tests_passed = 0;
int tests_failed = 0;

// Helper macro to test exceptions
#define TEST_EXCEPTION(exception_type, code, should_throw)                   \
  do {                                                                       \
    bool caught = false;                                                     \
    string msg;                                                              \
    try {                                                                    \
      code;                                                                  \
    } catch (const exception_type& e) {                                      \
      caught = true;                                                         \
      msg = e.what();                                                        \
    } catch (...) {                                                          \
      caught = true;                                                         \
      cerr << "Wrong exception type caught!" << endl;                        \
      tests_failed++;                                                        \
      break;                                                                 \
    }                                                                        \
    if (should_throw && !caught) {                                           \
      cerr << "FAIL: Expected " #exception_type " was not thrown" << endl;   \
      tests_failed++;                                                        \
    } else if (!should_throw && caught) {                                    \
      cerr << "FAIL: Unexpected " #exception_type " was thrown" << endl;     \
      tests_failed++;                                                        \
    } else {                                                                 \
      if (should_throw) {                                                    \
        if (msg.find("test_ah_errors.cc") == string::npos) {                 \
          cerr << "FAIL: Message doesn't contain file name: " << msg << endl;\
          tests_failed++;                                                    \
        } else {                                                             \
          tests_passed++;                                                    \
        }                                                                    \
      } else {                                                               \
        tests_passed++;                                                      \
      }                                                                      \
    }                                                                        \
  } while(0)

// ============================================================================
// Tests for RANGE_ERROR macros
// ============================================================================

void test_range_error_if()
{
  cout << "Testing ah_range_error_if..." << endl;

  // Should throw when condition is true
  TEST_EXCEPTION(range_error,
    ah_range_error_if(true) << "This should throw",
    true);

  // Should not throw when condition is false
  TEST_EXCEPTION(range_error,
    ah_range_error_if(false) << "This should not throw",
    false);
}

void test_range_error_unless()
{
  cout << "Testing ah_range_error_unless..." << endl;

  // Should throw when condition is false
  TEST_EXCEPTION(range_error,
    ah_range_error_unless(false) << "This should throw",
    true);

  // Should not throw when condition is true
  TEST_EXCEPTION(range_error,
    ah_range_error_unless(true) << "This should not throw",
    false);
}

void test_range_error()
{
  cout << "Testing ah_range_error..." << endl;

  // Should always throw
  TEST_EXCEPTION(range_error,
    ah_range_error() << "Unconditional throw",
    true);
}

// ============================================================================
// Tests for RUNTIME_ERROR macros
// ============================================================================

void test_runtime_error_if()
{
  cout << "Testing ah_runtime_error_if..." << endl;

  TEST_EXCEPTION(runtime_error,
    ah_runtime_error_if(true) << "This should throw",
    true);

  TEST_EXCEPTION(runtime_error,
    ah_runtime_error_if(false) << "This should not throw",
    false);
}

void test_runtime_error_unless()
{
  cout << "Testing ah_runtime_error_unless..." << endl;

  TEST_EXCEPTION(runtime_error,
    ah_runtime_error_unless(false) << "This should throw",
    true);

  TEST_EXCEPTION(runtime_error,
    ah_runtime_error_unless(true) << "This should not throw",
    false);
}

void test_runtime_error()
{
  cout << "Testing ah_runtime_error..." << endl;

  TEST_EXCEPTION(runtime_error,
    ah_runtime_error() << "Unconditional throw",
    true);
}

// ============================================================================
// Tests for LOGIC_ERROR macros
// ============================================================================

void test_logic_error_if()
{
  cout << "Testing ah_logic_error_if..." << endl;

  TEST_EXCEPTION(logic_error,
    ah_logic_error_if(true) << "This should throw",
    true);

  TEST_EXCEPTION(logic_error,
    ah_logic_error_if(false) << "This should not throw",
    false);
}

void test_logic_error_unless()
{
  cout << "Testing ah_logic_error_unless..." << endl;

  TEST_EXCEPTION(logic_error,
    ah_logic_error_unless(false) << "This should throw",
    true);

  TEST_EXCEPTION(logic_error,
    ah_logic_error_unless(true) << "This should not throw",
    false);
}

void test_logic_error()
{
  cout << "Testing ah_logic_error..." << endl;

  TEST_EXCEPTION(logic_error,
    ah_logic_error() << "Unconditional throw",
    true);
}

// ============================================================================
// Tests for UNDERFLOW_ERROR macros
// ============================================================================

void test_underflow_error_if()
{
  cout << "Testing ah_underflow_error_if..." << endl;

  TEST_EXCEPTION(underflow_error,
    ah_underflow_error_if(true) << "This should throw",
    true);

  TEST_EXCEPTION(underflow_error,
    ah_underflow_error_if(false) << "This should not throw",
    false);
}

void test_underflow_error_unless()
{
  cout << "Testing ah_underflow_error_unless..." << endl;

  TEST_EXCEPTION(underflow_error,
    ah_underflow_error_unless(false) << "This should throw",
    true);

  TEST_EXCEPTION(underflow_error,
    ah_underflow_error_unless(true) << "This should not throw",
    false);
}

void test_underflow_error()
{
  cout << "Testing ah_underflow_error..." << endl;

  TEST_EXCEPTION(underflow_error,
    ah_underflow_error() << "Unconditional throw",
    true);
}

// ============================================================================
// Tests for OVERFLOW_ERROR macros
// ============================================================================

void test_overflow_error_if()
{
  cout << "Testing ah_overflow_error_if..." << endl;

  TEST_EXCEPTION(overflow_error,
    ah_overflow_error_if(true) << "This should throw",
    true);

  TEST_EXCEPTION(overflow_error,
    ah_overflow_error_if(false) << "This should not throw",
    false);
}

void test_overflow_error_unless()
{
  cout << "Testing ah_overflow_error_unless..." << endl;

  TEST_EXCEPTION(overflow_error,
    ah_overflow_error_unless(false) << "This should throw",
    true);

  TEST_EXCEPTION(overflow_error,
    ah_overflow_error_unless(true) << "This should not throw",
    false);
}

void test_overflow_error()
{
  cout << "Testing ah_overflow_error..." << endl;

  TEST_EXCEPTION(overflow_error,
    ah_overflow_error() << "Unconditional throw",
    true);
}

// ============================================================================
// Tests for DOMAIN_ERROR macros
// ============================================================================

void test_domain_error_if()
{
  cout << "Testing ah_domain_error_if..." << endl;

  TEST_EXCEPTION(domain_error,
    ah_domain_error_if(true) << "This should throw",
    true);

  TEST_EXCEPTION(domain_error,
    ah_domain_error_if(false) << "This should not throw",
    false);
}

void test_domain_error_unless()
{
  cout << "Testing ah_domain_error_unless..." << endl;

  TEST_EXCEPTION(domain_error,
    ah_domain_error_unless(false) << "This should throw",
    true);

  TEST_EXCEPTION(domain_error,
    ah_domain_error_unless(true) << "This should not throw",
    false);
}

void test_domain_error()
{
  cout << "Testing ah_domain_error..." << endl;

  TEST_EXCEPTION(domain_error,
    ah_domain_error() << "Unconditional throw",
    true);
}

// ============================================================================
// Tests for OUT_OF_RANGE macros
// ============================================================================

void test_out_of_range_error_if()
{
  cout << "Testing ah_out_of_range_error_if..." << endl;

  TEST_EXCEPTION(out_of_range,
    ah_out_of_range_error_if(true) << "This should throw",
    true);

  TEST_EXCEPTION(out_of_range,
    ah_out_of_range_error_if(false) << "This should not throw",
    false);
}

void test_out_of_range_error_unless()
{
  cout << "Testing ah_out_of_range_error_unless..." << endl;

  TEST_EXCEPTION(out_of_range,
    ah_out_of_range_error_unless(false) << "This should throw",
    true);

  TEST_EXCEPTION(out_of_range,
    ah_out_of_range_error_unless(true) << "This should not throw",
    false);
}

void test_out_of_range_error()
{
  cout << "Testing ah_out_of_range_error..." << endl;

  TEST_EXCEPTION(out_of_range,
    ah_out_of_range_error() << "Unconditional throw",
    true);
}

// ============================================================================
// Tests for INVALID_ARGUMENT macros
// ============================================================================

void test_invalid_argument_if()
{
  cout << "Testing ah_invalid_argument_if..." << endl;

  TEST_EXCEPTION(invalid_argument,
    ah_invalid_argument_if(true) << "This should throw",
    true);

  TEST_EXCEPTION(invalid_argument,
    ah_invalid_argument_if(false) << "This should not throw",
    false);
}

void test_invalid_argument_unless()
{
  cout << "Testing ah_invalid_argument_unless..." << endl;

  TEST_EXCEPTION(invalid_argument,
    ah_invalid_argument_unless(false) << "This should throw",
    true);

  TEST_EXCEPTION(invalid_argument,
    ah_invalid_argument_unless(true) << "This should not throw",
    false);
}

void test_invalid_argument()
{
  cout << "Testing ah_invalid_argument..." << endl;

  TEST_EXCEPTION(invalid_argument,
    ah_invalid_argument() << "Unconditional throw",
    true);
}

// ============================================================================
// Tests for LENGTH_ERROR macros
// ============================================================================

void test_length_error_if()
{
  cout << "Testing ah_length_error_if..." << endl;

  TEST_EXCEPTION(length_error,
    ah_length_error_if(true) << "This should throw",
    true);

  TEST_EXCEPTION(length_error,
    ah_length_error_if(false) << "This should not throw",
    false);
}

void test_length_error_unless()
{
  cout << "Testing ah_length_error_unless..." << endl;

  TEST_EXCEPTION(length_error,
    ah_length_error_unless(false) << "This should throw",
    true);

  TEST_EXCEPTION(length_error,
    ah_length_error_unless(true) << "This should not throw",
    false);
}

void test_length_error()
{
  cout << "Testing ah_length_error..." << endl;

  TEST_EXCEPTION(length_error,
    ah_length_error() << "Unconditional throw",
    true);
}

// ============================================================================
// Tests for FATAL_ERROR macro
// ============================================================================

void test_fatal_error()
{
  cout << "Testing ah_fatal_error..." << endl;

  TEST_EXCEPTION(runtime_error,
    ah_fatal_error() << "Fatal error",
    true);
}

// ============================================================================
// Tests for WARNING macros
// ============================================================================

void test_warning_macros()
{
  cout << "Testing warning macros..." << endl;

  ostringstream oss;

  // Test ah_warning_if
  ah_warning_if(oss, true) << "Warning when true";
  string result = oss.str();
  if (result.find("WARNING") != string::npos &&
      result.find("test_ah_errors.cc") != string::npos) {
    tests_passed++;
  } else {
    cerr << "FAIL: ah_warning_if didn't produce expected output" << endl;
    tests_failed++;
  }

  oss.str("");
  oss.clear();

  // Test ah_warning_unless
  ah_warning_unless(oss, false) << "Warning when false";
  result = oss.str();
  if (result.find("WARNING") != string::npos &&
      result.find("test_ah_errors.cc") != string::npos) {
    tests_passed++;
  } else {
    cerr << "FAIL: ah_warning_unless didn't produce expected output" << endl;
    tests_failed++;
  }

  oss.str("");
  oss.clear();

  // Test ah_warning (unconditional)
  ah_warning(oss) << "Unconditional warning";
  result = oss.str();
  if (result.find("WARNING") != string::npos &&
      result.find("test_ah_errors.cc") != string::npos) {
    tests_passed++;
  } else {
    cerr << "FAIL: ah_warning didn't produce expected output" << endl;
    tests_failed++;
  }
}

// ============================================================================
// Tests for message formatting
// ============================================================================

void test_message_formatting()
{
  cout << "Testing message formatting with stream operators..." << endl;

  try {
    int value = 42;
    ah_range_error() << "Value is " << value << " but expected " << 100;
  } catch (const range_error& e) {
    string msg = e.what();
    if (msg.find("Value is 42 but expected 100") != string::npos) {
      tests_passed++;
    } else {
      cerr << "FAIL: Message formatting incorrect: " << msg << endl;
      tests_failed++;
    }
  }
}

// ============================================================================
// Main test runner
// ============================================================================

int main()
{
  cout << "==================================================" << endl;
  cout << "Running ah-errors.H test suite" << endl;
  cout << "==================================================" << endl;
  cout << endl;

  // Test all exception types
  test_range_error_if();
  test_range_error_unless();
  test_range_error();

  test_runtime_error_if();
  test_runtime_error_unless();
  test_runtime_error();

  test_logic_error_if();
  test_logic_error_unless();
  test_logic_error();

  test_underflow_error_if();
  test_underflow_error_unless();
  test_underflow_error();

  test_overflow_error_if();
  test_overflow_error_unless();
  test_overflow_error();

  test_domain_error_if();
  test_domain_error_unless();
  test_domain_error();

  test_out_of_range_error_if();
  test_out_of_range_error_unless();
  test_out_of_range_error();

  test_invalid_argument_if();
  test_invalid_argument_unless();
  test_invalid_argument();

  test_length_error_if();
  test_length_error_unless();
  test_length_error();

  test_fatal_error();

  // Test warnings
  test_warning_macros();

  // Test message formatting
  test_message_formatting();

  // Print results
  cout << endl;
  cout << "==================================================" << endl;
  cout << "Test Results:" << endl;
  cout << "  Passed: " << tests_passed << endl;
  cout << "  Failed: " << tests_failed << endl;
  cout << "  Total:  " << (tests_passed + tests_failed) << endl;
  cout << "==================================================" << endl;

  if (tests_failed == 0) {
    cout << "SUCCESS: All tests passed!" << endl;
    return 0;
  } else {
    cout << "FAILURE: Some tests failed!" << endl;
    return 1;
  }
}