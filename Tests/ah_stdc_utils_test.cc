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
*/

/**
 * @file ah_stdc_utils_test.cc
 * @brief Unit tests for ah_stdc++_utils.H
 *
 * Tests cover:
 * - verify_container_and_iterator: single iterator validation
 * - verify_iterators: pair of iterators validation
 * - verify_container_and_iterators: multiple overloads
 * - Exception throwing on invalid cases
 */

#include <gtest/gtest.h>

#include <ah_stdc++_utils.H>

#include <stdexcept>

using namespace Aleph;

// ============================================================================
// Mock Container and Iterator for testing
// ============================================================================

namespace
{

// Simple mock container
class MockContainer
{
  int id_;

public:
  explicit MockContainer(int id) : id_(id) {}
  int id() const { return id_; }
};

// Mock iterator that tracks which container it belongs to
class MockIterator
{
  const MockContainer* container_;

public:
  explicit MockIterator(const MockContainer* c = nullptr) : container_(c) {}

  // Verify this iterator belongs to the given container
  bool verify(const MockContainer& c) const
  {
    return container_ == &c;
  }

  // Verify this iterator and another belong to the same container
  bool verify(const MockIterator& other) const
  {
    return container_ == other.container_;
  }

  const MockContainer* container() const { return container_; }
};

} // namespace

// ============================================================================
// verify_container_and_iterator Tests
// ============================================================================

TEST(VerifyContainerAndIterator, ValidIteratorDoesNotThrow)
{
  MockContainer container(1);
  MockIterator iter(&container);

  EXPECT_NO_THROW(verify_container_and_iterator(container, iter));
}

TEST(VerifyContainerAndIterator, InvalidIteratorThrows)
{
  MockContainer container1(1);
  MockContainer container2(2);
  MockIterator iter(&container1);

  EXPECT_THROW(verify_container_and_iterator(container2, iter), std::domain_error);
}

TEST(VerifyContainerAndIterator, NullIteratorThrows)
{
  MockContainer container(1);
  MockIterator iter(nullptr);

  EXPECT_THROW(verify_container_and_iterator(container, iter), std::domain_error);
}

TEST(VerifyContainerAndIterator, ExceptionMessageIsDescriptive)
{
  MockContainer container1(1);
  MockContainer container2(2);
  MockIterator iter(&container1);

  try
    {
      verify_container_and_iterator(container2, iter);
      FAIL() << "Expected exception was not thrown";
    }
  catch (const std::domain_error& e)
    {
      std::string msg = e.what();
      EXPECT_TRUE(msg.find("Iterator") != std::string::npos ||
                  msg.find("iterator") != std::string::npos);
    }
}

// ============================================================================
// verify_iterators Tests
// ============================================================================

TEST(VerifyIterators, SameContainerDoesNotThrow)
{
  MockContainer container(1);
  MockIterator iter1(&container);
  MockIterator iter2(&container);

  EXPECT_NO_THROW(verify_iterators(iter1, iter2));
}

TEST(VerifyIterators, DifferentContainersThrows)
{
  MockContainer container1(1);
  MockContainer container2(2);
  MockIterator iter1(&container1);
  MockIterator iter2(&container2);

  EXPECT_THROW(verify_iterators(iter1, iter2), std::domain_error);
}

TEST(VerifyIterators, SameIteratorDoesNotThrow)
{
  MockContainer container(1);
  MockIterator iter(&container);

  // Same iterator compared to itself should pass
  EXPECT_NO_THROW(verify_iterators(iter, iter));
}

TEST(VerifyIterators, BothNullIteratorsDoNotThrow)
{
  MockIterator iter1(nullptr);
  MockIterator iter2(nullptr);

  // Both null - same "container" (none)
  EXPECT_NO_THROW(verify_iterators(iter1, iter2));
}

TEST(VerifyIterators, OneNullIteratorThrows)
{
  MockContainer container(1);
  MockIterator iter1(&container);
  MockIterator iter2(nullptr);

  EXPECT_THROW(verify_iterators(iter1, iter2), std::domain_error);
}

// ============================================================================
// verify_container_and_iterators (2-iterator overload) Tests
// ============================================================================

TEST(VerifyContainerAndIterators2, BothIteratorsValidDoesNotThrow)
{
  MockContainer container(1);
  MockIterator iter1(&container);
  MockIterator iter2(&container);

  EXPECT_NO_THROW(verify_container_and_iterators(container, iter1, iter2));
}

TEST(VerifyContainerAndIterators2, FirstIteratorInvalidThrows)
{
  MockContainer container1(1);
  MockContainer container2(2);
  MockIterator iter1(&container2);  // Wrong container
  MockIterator iter2(&container1);

  EXPECT_THROW(verify_container_and_iterators(container1, iter1, iter2),
               std::domain_error);
}

TEST(VerifyContainerAndIterators2, SecondIteratorInvalidThrows)
{
  MockContainer container1(1);
  MockContainer container2(2);
  MockIterator iter1(&container1);
  MockIterator iter2(&container2);  // Wrong container

  EXPECT_THROW(verify_container_and_iterators(container1, iter1, iter2),
               std::domain_error);
}

TEST(VerifyContainerAndIterators2, BothIteratorsInvalidThrows)
{
  MockContainer container1(1);
  MockContainer container2(2);
  MockIterator iter1(&container2);
  MockIterator iter2(&container2);

  EXPECT_THROW(verify_container_and_iterators(container1, iter1, iter2),
               std::domain_error);
}

// ============================================================================
// verify_container_and_iterators (3-iterator overload) Tests
// ============================================================================

TEST(VerifyContainerAndIterators3, ValidDistinctIteratorsDoNotThrow)
{
  MockContainer dest_container(1);
  MockContainer src_container(2);

  MockIterator dest_iter(&dest_container);
  MockIterator src_iter1(&src_container);
  MockIterator src_iter2(&src_container);

  EXPECT_NO_THROW(verify_container_and_iterators(
      dest_container, dest_iter, src_iter1, src_iter2));
}

TEST(VerifyContainerAndIterators3, DestIteratorInvalidThrows)
{
  MockContainer dest_container(1);
  MockContainer src_container(2);
  MockContainer other_container(3);

  MockIterator dest_iter(&other_container);  // Wrong!
  MockIterator src_iter1(&src_container);
  MockIterator src_iter2(&src_container);

  EXPECT_THROW(verify_container_and_iterators(
      dest_container, dest_iter, src_iter1, src_iter2), std::domain_error);
}

TEST(VerifyContainerAndIterators3, SourceIteratorsFromDifferentContainersThrows)
{
  MockContainer dest_container(1);
  MockContainer src_container1(2);
  MockContainer src_container2(3);

  MockIterator dest_iter(&dest_container);
  MockIterator src_iter1(&src_container1);
  MockIterator src_iter2(&src_container2);  // Different from src_iter1!

  EXPECT_THROW(verify_container_and_iterators(
      dest_container, dest_iter, src_iter1, src_iter2), std::domain_error);
}

TEST(VerifyContainerAndIterators3, DestAndSourceSameContainerThrows)
{
  MockContainer container(1);

  MockIterator dest_iter(&container);
  MockIterator src_iter1(&container);  // Same as dest!
  MockIterator src_iter2(&container);

  // This should throw because dest and source are the same container
  EXPECT_THROW(verify_container_and_iterators(
      container, dest_iter, src_iter1, src_iter2), std::domain_error);
}

// ============================================================================
// Integration with real Aleph containers (if available)
// ============================================================================

// Note: Additional tests with real DynList/DynArray iterators could be added
// here to verify integration with actual Aleph container types.

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
