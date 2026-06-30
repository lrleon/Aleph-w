/*
                          Aleph_w

  Data structures & Algorithms
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
 * @file ah_cpp_compat_test.cc
 * @brief Tests for the C++20/23 compatibility layer (ah-cpp-compat.H).
 *
 * Exercises `Aleph::expected` through the same surface regardless of whether it
 * resolves to `std::expected` (C++23) or the in-house polyfill (C++20/17).
 */

#include <string>

#include <gtest/gtest.h>

#include <ah-cpp-compat.H>

using Aleph::expected;
using Aleph::unexpected;

namespace
{
// A small parse-like function returning an expected, the canonical use case.
expected<int, std::string> parse_positive(int raw)
{
  if (raw <= 0)
    return unexpected<std::string>("must be positive");
  return raw * 2;
}
}  // namespace

TEST(AhCppCompat, DetectionMacrosAreDefined)
{
  // The four macros must always be defined to 0 or 1.
  EXPECT_TRUE(ALEPH_HAS_STD_EXPECTED == 0 or ALEPH_HAS_STD_EXPECTED == 1);
  EXPECT_TRUE(ALEPH_HAS_STD_GENERATOR == 0 or ALEPH_HAS_STD_GENERATOR == 1);
  EXPECT_TRUE(ALEPH_HAS_STD_FLAT_MAP == 0 or ALEPH_HAS_STD_FLAT_MAP == 1);
  EXPECT_TRUE(ALEPH_HAS_STD_MDSPAN == 0 or ALEPH_HAS_STD_MDSPAN == 1);
}

TEST(AhCppCompat, HoldsValue)
{
  expected<int, std::string> e = 41;
  ASSERT_TRUE(e.has_value());
  ASSERT_TRUE(static_cast<bool>(e));
  EXPECT_EQ(e.value(), 41);
  EXPECT_EQ(*e, 41);
  EXPECT_EQ(e.value_or(-1), 41);
}

TEST(AhCppCompat, HoldsError)
{
  expected<int, std::string> e = unexpected<std::string>("boom");
  ASSERT_FALSE(e.has_value());
  ASSERT_FALSE(static_cast<bool>(e));
  EXPECT_EQ(e.error(), "boom");
  EXPECT_EQ(e.value_or(-1), -1);
}

TEST(AhCppCompat, ValueOnErrorThrows)
{
  expected<int, std::string> e = unexpected<std::string>("nope");
  EXPECT_THROW((void) e.value(), Aleph::bad_expected_access<std::string>);
}

TEST(AhCppCompat, ParseLikeRoundTrip)
{
  auto ok = parse_positive(10);
  ASSERT_TRUE(ok.has_value());
  EXPECT_EQ(*ok, 20);

  auto bad = parse_positive(-3);
  ASSERT_FALSE(bad.has_value());
  EXPECT_EQ(bad.error(), "must be positive");
}

TEST(AhCppCompat, Transform)
{
  expected<int, std::string> e = 21;
  auto doubled = e.transform([](int v) { return v * 2; });
  ASSERT_TRUE(doubled.has_value());
  EXPECT_EQ(*doubled, 42);

  expected<int, std::string> err = unexpected<std::string>("e");
  auto still_err = err.transform([](int v) { return v * 2; });
  ASSERT_FALSE(still_err.has_value());
  EXPECT_EQ(still_err.error(), "e");
}

TEST(AhCppCompat, AndThen)
{
  expected<int, std::string> e = 5;
  auto chained = e.and_then(
      [](int v) -> expected<int, std::string> { return v + 100; });
  ASSERT_TRUE(chained.has_value());
  EXPECT_EQ(*chained, 105);

  expected<int, std::string> err = unexpected<std::string>("x");
  auto skipped = err.and_then(
      [](int v) -> expected<int, std::string> { return v + 100; });
  ASSERT_FALSE(skipped.has_value());
  EXPECT_EQ(skipped.error(), "x");
}

TEST(AhCppCompat, OrElse)
{
  expected<int, std::string> err = unexpected<std::string>("recoverable");
  auto recovered = err.or_else(
      [](const std::string &) -> expected<int, std::string> { return 7; });
  ASSERT_TRUE(recovered.has_value());
  EXPECT_EQ(*recovered, 7);
}

TEST(AhCppCompat, CopyAndMovePreserveState)
{
  expected<std::string, int> v = std::string("hello");
  auto copy = v;                       // copy of a value
  ASSERT_TRUE(copy.has_value());
  EXPECT_EQ(*copy, "hello");

  expected<std::string, int> e = unexpected<int>(404);
  auto moved = std::move(e);           // move of an error
  ASSERT_FALSE(moved.has_value());
  EXPECT_EQ(moved.error(), 404);

  // Assignment that switches the active member (value -> error).
  copy = unexpected<int>(500);
  ASSERT_FALSE(copy.has_value());
  EXPECT_EQ(copy.error(), 500);
}

TEST(AhCppCompat, VoidExpectedBasic)
{
  expected<void, std::string> ok;
  ASSERT_TRUE(ok.has_value());
  ASSERT_TRUE(static_cast<bool>(ok));
  EXPECT_NO_THROW(ok.value());

  expected<void, std::string> err = unexpected<std::string>("fail");
  ASSERT_FALSE(err.has_value());
  ASSERT_FALSE(static_cast<bool>(err));
  EXPECT_EQ(err.error(), "fail");
  EXPECT_ANY_THROW(err.value());
}

TEST(AhCppCompat, VoidExpectedMonadic)
{
  expected<void, std::string> ok;
  auto chained = ok.and_then([]() -> expected<int, std::string> { return 42; });
  ASSERT_TRUE(chained.has_value());
  EXPECT_EQ(*chained, 42);

  expected<void, std::string> err = unexpected<std::string>("e");
  auto recovered = err.or_else(
      [](const std::string &) -> expected<void, std::string> { return {}; });
  ASSERT_TRUE(recovered.has_value());
}

namespace
{
struct ThrowOnCopy
{
  int val = 0;
  ThrowOnCopy(int v = 0) : val(v) {}
  ThrowOnCopy(const ThrowOnCopy &) { throw std::runtime_error("copy threw"); }
  ThrowOnCopy(ThrowOnCopy &&) noexcept = default;
  ThrowOnCopy &operator=(const ThrowOnCopy &) = delete;
  ThrowOnCopy &operator=(ThrowOnCopy &&) noexcept = default;
};
}  // namespace

TEST(AhCppCompat, AssignmentExceptionSafety)
{
  expected<int, ThrowOnCopy> original = 100;
  ASSERT_TRUE(original.has_value());

  expected<int, ThrowOnCopy> other = unexpected<ThrowOnCopy>(ThrowOnCopy(42));
  ASSERT_FALSE(other.has_value());

  EXPECT_THROW(original = other, std::runtime_error);
  ASSERT_TRUE(original.has_value());
  EXPECT_EQ(*original, 100);
}