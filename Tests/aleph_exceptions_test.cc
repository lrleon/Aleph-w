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
 * @file aleph_exceptions_test.cc
 * @brief The exceptions of `aleph-exceptions.H`, deprecated, work as before
 * until the header goes in 8.0.0.
 */

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

// aleph-exceptions.H is deprecated, but supported until 8.0.0; this test is
// its coverage. Tests/aleph_exceptions_deprecation_check.cmake checks where
// it warns.
#if defined(_MSC_VER) && !defined(__clang__)
#  pragma warning(push)
#  pragma warning(disable : 4996)
#else
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <aleph-exceptions.H>

namespace
{
  DEFINE_ALEPH_EXCEPTION(MyAlephError, "my category")

  template <class E>
  void expect_the_exception(const char * name, const std::string & category)
  {
    const size_t line = __LINE__ + 3;
    try
      {
        ALEPHTHROW(E, "the message");
      }
    catch (const AlephException & e)
      {
        EXPECT_NE(dynamic_cast<const E *>(&e), nullptr) << name;
        EXPECT_EQ(e.line_number, line) << name;
        EXPECT_EQ(e.file_name, __FILE__) << name;
        EXPECT_EQ(e.type, "E") << name;
        EXPECT_EQ(std::string(e.what()),
                  category + "|" + __FILE__ + "|" + std::to_string(line)
                  + "|E|the message") << name;
        return;
      }
    ADD_FAILURE() << name << " was not caught as an AlephException";
  }
}

// The category, as DEFINE_ALEPH_EXCEPTION takes it: the text of its argument
#define EXPECT_THE_EXCEPTION(E, category) \
  expect_the_exception<E>(#E, #category)

// Each exception of the header is an AlephException, and so a
// std::runtime_error, whose what() joins its category, where it was thrown,
// the name that ALEPHTHROW gives it and the message
TEST(AlephExceptions, EachIsAnAlephExceptionWithItsCategory)
{
  EXPECT_THE_EXCEPTION(MinMaxReversed,
                       "minimum value is greater than maximum value");
  EXPECT_THE_EXCEPTION(InvalidCsvHeader, "Invalid csv header");
  EXPECT_THE_EXCEPTION(InvalidCsvRow, "Invalid csv row");
  EXPECT_THE_EXCEPTION(InvalidValue, "Invalid value");
  EXPECT_THE_EXCEPTION(OutOfMemory, "No memory");
  EXPECT_THE_EXCEPTION(OutOfRange, "Value is out of allowed range");
  EXPECT_THE_EXCEPTION(ValueNotFound, "Value not found");
  EXPECT_THE_EXCEPTION(CommandLineError, "error parsing command line");
  EXPECT_THE_EXCEPTION(InvariantError, "A bug has been detected. "
                       "Please send a bug report to leandro.r.leon@gmail.com");
  EXPECT_THE_EXCEPTION(SizeMismatch, "Sequences have different sizes");
  EXPECT_THE_EXCEPTION(InvalidConversion, "failure in numeric conversion");
  EXPECT_THE_EXCEPTION(NameNotFound, "name not found");
  EXPECT_THE_EXCEPTION(InvalidSize, "invalid size");
  EXPECT_THE_EXCEPTION(InvalidRange, "invalid range");
  EXPECT_THE_EXCEPTION(InvalidUnit, "invalid unit");
  EXPECT_THE_EXCEPTION(DuplicatedOperation, "duplicated operation");
  EXPECT_THE_EXCEPTION(DuplicatedName, "Duplicated name");
  EXPECT_THE_EXCEPTION(InvalidRead, "Error while reading input stream");
  EXPECT_THE_EXCEPTION(FileNotFound, "file not found");
}

// DEFINE_ALEPH_EXCEPTION still defines an AlephException, which takes as
// its category the text of its second argument
TEST(AlephExceptions, DefineAlephExceptionDefinesOne)
{
  EXPECT_THE_EXCEPTION(MyAlephError, "my category");

  EXPECT_THROW(ALEPHTHROW(MyAlephError, "m"), std::runtime_error);
}

#undef EXPECT_THE_EXCEPTION

#if defined(_MSC_VER) && !defined(__clang__)
#  pragma warning(pop)
#else
#  pragma GCC diagnostic pop
#endif
