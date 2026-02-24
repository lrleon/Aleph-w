
#ifndef TESTS_TEST_HELPERS_H
#define TESTS_TEST_HELPERS_H

#include <gtest/gtest.h>
#include <initializer_list>
#include <tpl_array.H>

namespace Aleph::test_helpers
{
  /** @brief Asserts that an Aleph::Array equals an initializer_list of values. */
  inline void expect_array_eq(const Array<size_t> & a,
                              std::initializer_list<size_t> expected)
  {
    ASSERT_EQ(a.size(), expected.size());
    size_t i = 0;
    for (const size_t value : expected)
      EXPECT_EQ(a[i++], value);
  }
}

#endif // TESTS_TEST_HELPERS_H
