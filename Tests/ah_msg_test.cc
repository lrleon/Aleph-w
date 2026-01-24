
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
 * @file ah_msg_test.cc
 * @brief Tests for Ah Msg
 */

#include <gtest/gtest.h>

#include <ah-msg.H>

#include <string>

namespace
{

struct NonStreamable
{
  int value;
};

} // namespace

TEST(AhMsgTest, BuildStringConcatenatesArguments)
{
  const std::string msg = Aleph::build_string("Value=", 42, ", flag=", true);
  EXPECT_EQ(msg, "Value=42, flag=1");

  const std::string alias = Aleph::build_error_msg("[", msg, "]");
  EXPECT_EQ(alias, "[Value=42, flag=1]");
}

TEST(AhMsgTest, BuildStringSupportsMovableTypes)
{
  std::string dynamic = "dynamic";
  const std::string msg = Aleph::build_string("copy-", dynamic, "-move-", std::string("temp"));

  EXPECT_EQ(msg, "copy-dynamic-move-temp");
  EXPECT_EQ(dynamic, "dynamic"); // ensure lvalue untouched
}

TEST(AhMsgTest, BuildStringRejectsNonStreamableAtCompileTime)
{
  static_assert(!Aleph::detail::is_stream_insertable<NonStreamable>::value,
                "NonStreamable should not be considered stream insertable");
  static_assert(!Aleph::detail::are_stream_insertable_v<NonStreamable>,
                "Variadic helper must reject NonStreamable");
}
