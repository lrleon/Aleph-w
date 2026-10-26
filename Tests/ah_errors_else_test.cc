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
 * @file ah_errors_else_test.cc
 * @brief Tests that an else written after a conditional macro of
 *        ah-errors.H, parse_utils.H or ah-trace.H belongs to the if around
 *        the macro.
 *
 * The macros used to be if statements without braces, so such an else
 * belonged to the if of the macro. Every if of this file whose statement is
 * a macro, with an else or without one, must compile without
 * -Wdangling-else; that the else cannot follow a macro with no if around it
 * is checked by ah_errors_else_check.cmake.
 */

# include <gtest/gtest.h>

# include <new>
# include <sstream>
# include <stdexcept>
# include <string>
# include <type_traits>
# include <utility>

# include <ah-errors.H>
# include <ah-trace.H>
# include <parse_utils.H>

# if defined(__clang__)
#   pragma clang diagnostic error "-Wdangling-else"
# elif defined(__GNUC__)
#   pragma GCC diagnostic error "-Wdangling-else"
# endif

namespace
{
  int evaluations = 0; // of the conditions given to the macros

  bool counted(bool value)
  {
    ++evaluations;
    return value;
  }

  size_t occurrences(const std::string & text, const std::string & part)
  {
    size_t count = 0;
    for (size_t pos = text.find(part); pos != std::string::npos;
         pos = text.find(part, pos + part.size()))
      ++count;
    return count;
  }
}

// MACRO throws E when its condition is THROWS, with TAIL after it. For each
// value of the condition and of the if around the macro, the else after the
// macro runs only when that if does not, the macro throws only from inside
// that if, and the condition is evaluated once. Without the else, the same.
# define TEST_THE_ELSE_AFTER(NAME, MACRO, E, THROWS, TAIL, HAS_MESSAGE)      \
  TEST(AhErrorsElse, NAME)                                                   \
  {                                                                          \
    for (bool around : {false, true})                                        \
      for (bool condition : {false, true})                                   \
        {                                                                    \
          SCOPED_TRACE(testing::Message() << "around " << around             \
                       << ", condition " << condition);                      \
          bool else_ran = false;                                             \
          bool thrown = false;                                               \
          std::string what;                                                  \
          evaluations = 0;                                                   \
          try                                                                \
            {                                                                \
              if (around)                                                    \
                MACRO(counted(condition)) TAIL;                              \
              else                                                           \
                else_ran = true;                                             \
            }                                                                \
          catch (const E & e)                                                \
            {                                                                \
              thrown = true;                                                 \
              what = e.what();                                               \
            }                                                                \
          EXPECT_EQ(else_ran, not around);                                   \
          EXPECT_EQ(thrown, around and condition == THROWS);                 \
          EXPECT_EQ(evaluations, around ? 1 : 0);                            \
          if (thrown and HAS_MESSAGE)                                        \
            {                                                                \
              EXPECT_EQ(occurrences(what, "ah_errors_else_test.cc:"), 1u)    \
                << what;                                                     \
              EXPECT_EQ(occurrences(what, ") | the message"), 1u) << what;   \
            }                                                                \
                                                                             \
          thrown = false;                                                    \
          evaluations = 0;                                                   \
          try                                                                \
            {                                                                \
              if (around)                                                    \
                MACRO(counted(condition)) TAIL;                              \
            }                                                                \
          catch (const E &)                                                  \
            {                                                                \
              thrown = true;                                                 \
            }                                                                \
          EXPECT_EQ(thrown, around and condition == THROWS);                 \
          EXPECT_EQ(evaluations, around ? 1 : 0);                            \
        }                                                                    \
  }

// The same for the _constexpr form MACRO, which takes constant conditions
# define TEST_THE_ELSE_AFTER_CONSTEXPR(NAME, MACRO, E, THROWS, TAIL)         \
  TEST(AhErrorsElse, NAME)                                                   \
  {                                                                          \
    for (bool around : {false, true})                                        \
      {                                                                      \
        SCOPED_TRACE(testing::Message() << "around " << around);             \
        bool else_ran = false;                                               \
        bool thrown = false;                                                 \
        try                                                                  \
          {                                                                  \
            if (around)                                                      \
              MACRO(THROWS) TAIL;                                            \
            else                                                             \
              else_ran = true;                                               \
          }                                                                  \
        catch (const E &)                                                    \
          {                                                                  \
            thrown = true;                                                   \
          }                                                                  \
        EXPECT_EQ(else_ran, not around);                                     \
        EXPECT_EQ(thrown, around);                                           \
                                                                             \
        else_ran = false;                                                    \
        thrown = false;                                                      \
        try                                                                  \
          {                                                                  \
            if (around)                                                      \
              MACRO(not THROWS) TAIL;                                        \
            else                                                             \
              else_ran = true;                                               \
          }                                                                  \
        catch (const E &)                                                    \
          {                                                                  \
            thrown = true;                                                   \
          }                                                                  \
        EXPECT_EQ(else_ran, not around);                                     \
        EXPECT_FALSE(thrown);                                                \
                                                                             \
        thrown = false;                                                      \
        try                                                                  \
          {                                                                  \
            if (around)                                                      \
              MACRO(THROWS) TAIL;                                            \
          }                                                                  \
        catch (const E &)                                                    \
          {                                                                  \
            thrown = true;                                                   \
          }                                                                  \
        EXPECT_EQ(thrown, around);                                           \
      }                                                                      \
  }

// The same for MACRO, which writes to its stream when its condition is
// WRITES, and puts the location of the call first if LOCATED
# define TEST_THE_ELSE_AFTER_A_MESSAGE(NAME, MACRO, WRITES, LOCATED)         \
  TEST(AhErrorsElse, NAME)                                                   \
  {                                                                          \
    for (bool around : {false, true})                                        \
      for (bool condition : {false, true})                                   \
        {                                                                    \
          SCOPED_TRACE(testing::Message() << "around " << around             \
                       << ", condition " << condition);                      \
          const bool writes = around and condition == WRITES;                \
          std::ostringstream out;                                            \
          bool else_ran = false;                                             \
          evaluations = 0;                                                   \
          if (around)                                                        \
            MACRO(out, counted(condition)) << "the message";                 \
          else                                                               \
            else_ran = true;                                                 \
          EXPECT_EQ(else_ran, not around);                                   \
          EXPECT_EQ(evaluations, around ? 1 : 0);                            \
          EXPECT_EQ(occurrences(out.str(), "the message"), writes ? 1u : 0u) \
            << out.str();                                                    \
          if (writes and LOCATED)                                            \
            {                                                                \
              EXPECT_EQ(occurrences(out.str(), "ah_errors_else_test.cc:"),   \
                        1u) << out.str();                                    \
            }                                                                \
                                                                             \
          std::ostringstream alone;                                          \
          evaluations = 0;                                                   \
          if (around)                                                        \
            MACRO(alone, counted(condition)) << "the message";               \
          EXPECT_EQ(evaluations, around ? 1 : 0);                            \
          EXPECT_EQ(occurrences(alone.str(), "the message"),                 \
                    writes ? 1u : 0u) << alone.str();                        \
        }                                                                    \
  }

# define WITH_MESSAGE << "the message"
# define WITHOUT_MESSAGE

// ah-errors.H
TEST_THE_ELSE_AFTER(RangeErrorIf, ah_range_error_if, std::range_error, true,
                    WITH_MESSAGE, true)
TEST_THE_ELSE_AFTER(RangeErrorUnless, ah_range_error_unless, std::range_error,
                    false, WITH_MESSAGE, true)
TEST_THE_ELSE_AFTER(RuntimeErrorIf, ah_runtime_error_if, std::runtime_error,
                    true, WITH_MESSAGE, true)
TEST_THE_ELSE_AFTER(RuntimeErrorUnless, ah_runtime_error_unless,
                    std::runtime_error, false, WITH_MESSAGE, true)
TEST_THE_ELSE_AFTER(LogicErrorIf, ah_logic_error_if, std::logic_error, true,
                    WITH_MESSAGE, true)
TEST_THE_ELSE_AFTER(LogicErrorUnless, ah_logic_error_unless, std::logic_error,
                    false, WITH_MESSAGE, true)
TEST_THE_ELSE_AFTER(UnderflowErrorIf, ah_underflow_error_if,
                    std::underflow_error, true, WITH_MESSAGE, true)
TEST_THE_ELSE_AFTER(UnderflowErrorUnless, ah_underflow_error_unless,
                    std::underflow_error, false, WITH_MESSAGE, true)
TEST_THE_ELSE_AFTER(OverflowErrorIf, ah_overflow_error_if,
                    std::overflow_error, true, WITH_MESSAGE, true)
TEST_THE_ELSE_AFTER(OverflowErrorUnless, ah_overflow_error_unless,
                    std::overflow_error, false, WITH_MESSAGE, true)
TEST_THE_ELSE_AFTER(DomainErrorIf, ah_domain_error_if, std::domain_error,
                    true, WITH_MESSAGE, true)
TEST_THE_ELSE_AFTER(DomainErrorUnless, ah_domain_error_unless,
                    std::domain_error, false, WITH_MESSAGE, true)
TEST_THE_ELSE_AFTER(OutOfRangeErrorIf, ah_out_of_range_error_if,
                    std::out_of_range, true, WITH_MESSAGE, true)
TEST_THE_ELSE_AFTER(OutOfRangeErrorUnless, ah_out_of_range_error_unless,
                    std::out_of_range, false, WITH_MESSAGE, true)
TEST_THE_ELSE_AFTER(InvalidArgumentIf, ah_invalid_argument_if,
                    std::invalid_argument, true, WITH_MESSAGE, true)
TEST_THE_ELSE_AFTER(InvalidArgumentUnless, ah_invalid_argument_unless,
                    std::invalid_argument, false, WITH_MESSAGE, true)
TEST_THE_ELSE_AFTER(LengthErrorIf, ah_length_error_if, std::length_error,
                    true, WITH_MESSAGE, true)
TEST_THE_ELSE_AFTER(LengthErrorUnless, ah_length_error_unless,
                    std::length_error, false, WITH_MESSAGE, true)
TEST_THE_ELSE_AFTER(BadAllocIf, ah_bad_alloc_if, std::bad_alloc, true,
                    WITHOUT_MESSAGE, false)
TEST_THE_ELSE_AFTER(BadAllocUnless, ah_bad_alloc_unless, std::bad_alloc,
                    false, WITHOUT_MESSAGE, false)

TEST_THE_ELSE_AFTER_CONSTEXPR(RangeErrorIfConstexpr,
                              ah_range_error_if_constexpr, std::range_error,
                              true, WITH_MESSAGE)
TEST_THE_ELSE_AFTER_CONSTEXPR(RangeErrorUnlessConstexpr,
                              ah_range_error_unless_constexpr,
                              std::range_error, false, WITH_MESSAGE)
TEST_THE_ELSE_AFTER_CONSTEXPR(RuntimeErrorIfConstexpr,
                              ah_runtime_error_if_constexpr,
                              std::runtime_error, true, WITH_MESSAGE)
TEST_THE_ELSE_AFTER_CONSTEXPR(RuntimeErrorUnlessConstexpr,
                              ah_runtime_error_unless_constexpr,
                              std::runtime_error, false, WITH_MESSAGE)
TEST_THE_ELSE_AFTER_CONSTEXPR(LogicErrorIfConstexpr,
                              ah_logic_error_if_constexpr, std::logic_error,
                              true, WITH_MESSAGE)
TEST_THE_ELSE_AFTER_CONSTEXPR(LogicErrorUnlessConstexpr,
                              ah_logic_error_unless_constexpr,
                              std::logic_error, false, WITH_MESSAGE)
TEST_THE_ELSE_AFTER_CONSTEXPR(UnderflowErrorIfConstexpr,
                              ah_underflow_error_if_constexpr,
                              std::underflow_error, true, WITH_MESSAGE)
TEST_THE_ELSE_AFTER_CONSTEXPR(UnderflowErrorUnlessConstexpr,
                              ah_underflow_error_unless_constexpr,
                              std::underflow_error, false, WITH_MESSAGE)
TEST_THE_ELSE_AFTER_CONSTEXPR(OverflowErrorIfConstexpr,
                              ah_overflow_error_if_constexpr,
                              std::overflow_error, true, WITH_MESSAGE)
TEST_THE_ELSE_AFTER_CONSTEXPR(OverflowErrorUnlessConstexpr,
                              ah_overflow_error_unless_constexpr,
                              std::overflow_error, false, WITH_MESSAGE)
TEST_THE_ELSE_AFTER_CONSTEXPR(DomainErrorIfConstexpr,
                              ah_domain_error_if_constexpr, std::domain_error,
                              true, WITH_MESSAGE)
TEST_THE_ELSE_AFTER_CONSTEXPR(DomainErrorUnlessConstexpr,
                              ah_domain_error_unless_constexpr,
                              std::domain_error, false, WITH_MESSAGE)
TEST_THE_ELSE_AFTER_CONSTEXPR(OutOfRangeErrorIfConstexpr,
                              ah_out_of_range_error_if_constexpr,
                              std::out_of_range, true, WITH_MESSAGE)
TEST_THE_ELSE_AFTER_CONSTEXPR(OutOfRangeErrorUnlessConstexpr,
                              ah_out_of_range_error_unless_constexpr,
                              std::out_of_range, false, WITH_MESSAGE)
TEST_THE_ELSE_AFTER_CONSTEXPR(InvalidArgumentIfConstexpr,
                              ah_invalid_argument_if_constexpr,
                              std::invalid_argument, true, WITH_MESSAGE)
TEST_THE_ELSE_AFTER_CONSTEXPR(InvalidArgumentUnlessConstexpr,
                              ah_invalid_argument_unless_constexpr,
                              std::invalid_argument, false, WITH_MESSAGE)
TEST_THE_ELSE_AFTER_CONSTEXPR(LengthErrorIfConstexpr,
                              ah_length_error_if_constexpr, std::length_error,
                              true, WITH_MESSAGE)
TEST_THE_ELSE_AFTER_CONSTEXPR(LengthErrorUnlessConstexpr,
                              ah_length_error_unless_constexpr,
                              std::length_error, false, WITH_MESSAGE)
TEST_THE_ELSE_AFTER_CONSTEXPR(BadAllocIfConstexpr, ah_bad_alloc_if_constexpr,
                              std::bad_alloc, true, WITHOUT_MESSAGE)
TEST_THE_ELSE_AFTER_CONSTEXPR(BadAllocUnlessConstexpr,
                              ah_bad_alloc_unless_constexpr, std::bad_alloc,
                              false, WITHOUT_MESSAGE)

TEST_THE_ELSE_AFTER_A_MESSAGE(WarningIf, ah_warning_if, true, true)
TEST_THE_ELSE_AFTER_A_MESSAGE(WarningUnless, ah_warning_unless, false, true)

// parse_utils.H
TEST_THE_ELSE_AFTER(ParseErrorIf, ah_parse_error_if, Aleph::ParseError, true,
                    WITH_MESSAGE, true)
TEST_THE_ELSE_AFTER(ParseErrorUnless, ah_parse_error_unless,
                    Aleph::ParseError, false, WITH_MESSAGE, true)

// ah-trace.H
TEST_THE_ELSE_AFTER_A_MESSAGE(TraceIf, ah_trace_if, true, true)
TEST_THE_ELSE_AFTER_A_MESSAGE(TraceUnless, ah_trace_unless, false, true)
TEST_THE_ELSE_AFTER_A_MESSAGE(MsgIf, ah_msg_if, true, false)
TEST_THE_ELSE_AFTER_A_MESSAGE(MsgUnless, ah_msg_unless, false, false)

namespace
{
  int chain(int k, int x)
  {
    int r = 0;
    if (k == 0)
      ah_domain_error_if(x < 0) << "x is negative";
    else if (k == 1)
      ah_domain_error_unless(x > 0) << "x is not positive";
    else
      r = 2;
    return r;
  }
}

TEST(AhErrorsElse, ElseIfChain)
{
  EXPECT_EQ(chain(0, 1), 0);
  EXPECT_THROW(chain(0, -1), std::domain_error);
  EXPECT_EQ(chain(1, 1), 0);
  EXPECT_THROW(chain(1, 0), std::domain_error);
  EXPECT_EQ(chain(2, -5), 2);
}

// A break or a continue after a macro is that of the switch or the loop of
// the caller
TEST(AhErrorsElse, BreakAndContinueAfterAMacro)
{
  int r = 0;
  for (int i = 0; i < 6; ++i)
    {
      switch (i % 3)
        {
        case 0:
          ah_domain_error_if(i < 0) << "never";
          r += 1;
          break;
        default:
          ah_domain_error_unless(i >= 0) << "never";
          if (i == 4)
            continue;
          r += 10;
        }
      r += 100;
    }
  EXPECT_EQ(r, 1 + 100 + 2 * (10 + 100) + 1 + 100 + 10 + 100);
}

namespace
{
  template <class T>
  int not_a_pointer(bool checked)
  {
    if (checked)
      ah_invalid_argument_if_constexpr(std::is_pointer_v<T>) << "a pointer";
    else
      return 2;
    return 1;
  }
}

TEST(AhErrorsElse, ConstexprFormInATemplate)
{
  EXPECT_EQ(not_a_pointer<int>(false), 2);
  EXPECT_EQ(not_a_pointer<int>(true), 1);
  EXPECT_EQ(not_a_pointer<int *>(false), 2);
  EXPECT_THROW(not_a_pointer<int *>(true), std::invalid_argument);
}

namespace
{
  // What a message gives back when its last << does not give back a stream
  struct Not_A_Stream {};

  template <class Tail>
  constexpr bool throws_with = requires (Tail && tail)
    {
      Aleph::ExceptionBuilder<std::domain_error>(), std::forward<Tail>(tail);
    };
}

// Otherwise the comma of the macro would not throw, and its while would not
// end
static_assert(throws_with<std::ostringstream &>);
static_assert(throws_with<std::ostream &>);
static_assert(not throws_with<Not_A_Stream>);
static_assert(not throws_with<int>);
