
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
 * @file moved_from.H
 * @brief The pattern test of the moved-from objects.
 *
 * A class that keeps a size or a parameter in a member of its own, beside
 * the array that it describes, is left inconsistent by the implicit moves:
 * they take the array, which is left empty in the source (or exchanged, in
 * a move assignment), but copy the scalar. The moved-from object then
 * reads or writes storage that it no longer has. These typed tests move
 * from objects of each class given to them and use the sources and the
 * destinations, so that a class with the pattern fails: with an exception
 * or an assertion of a checked access, and under AddressSanitizer with a
 * report of the unchecked accesses.
 *
 * Each class is given by specializing Moved_From_Case for it, and the
 * suite is instantiated with a list of them:
 * @code
 * template <> struct Moved_From_Case<Some_Class>
 * {
 *   static Some_Class make(size_t n);       // an object of about n items
 *   static bool is_empty(Some_Class & x);   // what a move leaves
 *   static void use(Some_Class & x);        // ordinary operations
 *   static auto fingerprint(const Some_Class & x);  // optional, see below
 * };
 * using Classes = ::testing::Types<Some_Class, ...>;
 * INSTANTIATE_TYPED_TEST_SUITE_P(Family, MovedFrom, Classes);
 * @endcode
 *
 * `is_empty()` may take the object as `const` or not.
 *
 * `use()` must do what an ordinary user does with any object of the class,
 * an empty one included: walk the positions that the object reports, read
 * and write them, insert, query and clear. A use that only destroys or
 * assigns the object does not show the pattern.
 *
 * The destination of a move must have what the source had. The tests
 * compare `fingerprint()` of the source before the move with that of the
 * destination after it; a class without one is compared by its `size()`,
 * if it has one. A move assignment may leave in the source the previous
 * state of the destination (an exchange) or an empty object, so the tests
 * do not compare the source after it.
 */

#ifndef ALEPH_TESTS_MOVED_FROM_H
#define ALEPH_TESTS_MOVED_FROM_H

#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

/** @brief How the moved-from tests build, check and use objects of a class.
 *  @tparam T The class; each one tested specializes it.
 *
 *  A specialization may also give `static auto fingerprint(const T & x)`,
 *  a value that a move must carry to its destination; without one, the
 *  size of the object stands for it, if the class reports one.
 */
template <class T> struct Moved_From_Case;

/** @brief What a move must carry from its source to its destination.
 *  @tparam Case The Moved_From_Case of the class.
 *  @tparam T The class.
 *  @param[in] x The object.
 *  @return `Case::fingerprint(x)` if the case gives one, else the size of
 *          `x` if it has one, else 0 (nothing to compare).
 */
template <class Case, class T>
auto moved_from_fingerprint(const T & x)
{
  if constexpr (requires { Case::fingerprint(x); })
    return Case::fingerprint(x);
  else if constexpr (requires { { x.size() } -> std::convertible_to<std::size_t>; })
    return static_cast<std::size_t>(x.size());
  else
    return std::size_t(0);
}

/** @brief The typed suite of the moved-from tests.
 *  @tparam T The class tested, with its Moved_From_Case.
 */
template <class T> class MovedFrom : public ::testing::Test {};

TYPED_TEST_SUITE_P(MovedFrom);

// The source of a move construction is an empty object that can be used,
// and the destination has what the source had
TYPED_TEST_P(MovedFrom, ConstructionLeavesAnEmptyUsableSource)
{
  using Case = Moved_From_Case<TypeParam>;
  TypeParam src = Case::make(100);
  const auto print = moved_from_fingerprint<Case>(src);
  TypeParam dst(std::move(src));
  EXPECT_EQ(moved_from_fingerprint<Case>(dst), print);
  EXPECT_TRUE(Case::is_empty(src));
  Case::use(src);
  Case::use(dst);
  Case::use(src);  // the second use runs on what the first one left
}

// A move assignment, from a larger object and from a smaller one, gives
// the destination what the source had and leaves both objects usable:
// with an exchange, the source gets the state of the destination. Classes
// that cannot be assigned skip it
TYPED_TEST_P(MovedFrom, AssignmentLeavesBothObjectsUsable)
{
  if constexpr (std::is_move_assignable_v<TypeParam>)
    {
      using Case = Moved_From_Case<TypeParam>;
      for (const std::size_t n : {std::size_t(2), std::size_t(1000)})
        {
          TypeParam dst = Case::make(n);
          TypeParam src = Case::make(n == 2 ? 1000 : 2);
          const auto print = moved_from_fingerprint<Case>(src);
          dst = std::move(src);
          EXPECT_EQ(moved_from_fingerprint<Case>(dst), print);
          Case::use(src);
          Case::use(dst);
        }
    }
  else
    GTEST_SKIP() << "the class has no move assignment";
}

REGISTER_TYPED_TEST_SUITE_P(MovedFrom, ConstructionLeavesAnEmptyUsableSource,
                            AssignmentLeavesBothObjectsUsable);

#endif // ALEPH_TESTS_MOVED_FROM_H
