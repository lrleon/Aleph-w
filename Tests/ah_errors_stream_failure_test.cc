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
 * @file ah_errors_stream_failure_test.cc
 * @brief The macros of ah-errors.H that throw, when their stream fails.
 *
 * A macro that throws allocates the stream of its message. This program
 * replaces the global operator new, so that a test can make an allocation
 * fail, and declares before ah-errors.H an inserter that the place of a
 * message takes, so that a test can make that place throw. Without memory
 * for its stream, a macro throws std::bad_alloc and evaluates no item;
 * whatever allocation fails, it throws std::bad_alloc or its exception;
 * and a place that throws frees the stream.
 */

# include <gtest/gtest.h>

# include "allocation_failure.H"

# include <cstdio>
# include <ios>
# include <new>
# include <ostream>
# include <sstream>
# include <stdexcept>
# include <string>

namespace
{
  struct Place_Failure {};

  bool place_throws = false;  // while true, the inserter below throws
  int streams_dead = 0;

  void count_a_dead_stream(const std::ios_base::event event, std::ios_base &,
                           int)
  {
    if (event == std::ios_base::erase_event)
      ++streams_dead;
  }
}

// The constructor of the stream of a message finds this inserter for the
// strings of the place: it is declared before ah-errors.H and takes a
// std::ostringstream, where that of the standard library takes its base.
// The items of a macro go into an rvalue, which it does not take.
std::ostringstream & operator << (std::ostringstream & out, const char * str)
{
  if (place_throws)
    {
      out.register_callback(count_a_dead_stream, 0);
      throw Place_Failure();
    }
  static_cast<std::ostream &>(out) << str;
  return out;
}

# include <ah-errors.H>

namespace
{
  int evaluated = 0;

  int item() { return ++evaluated; }

  // A macro that throws, with two items that count their evaluations
  void raise()
  {
    ah_domain_error() << "an item that is longer than a short string, "
                      << item() << " and " << item();
  }

  // The message of a std::domain_error, in an array: a string would ask
  // for memory while a test makes the allocations fail. The message
  // carries the path of this file: the array has room for it, however
  // long, and for the rest
  using Text = char[sizeof(__FILE__) + 128];

  // What raise() did: 'm' if it threw std::bad_alloc; 'e' if it threw
  // std::domain_error, whose message goes to `text`; and '?' otherwise
  char outcome_of_raise(Text & text)
  {
    text[0] = '\0';
    try
      {
        raise();
      }
    catch (const std::bad_alloc &)
      {
        return 'm';
      }
    catch (const std::domain_error & e)
      {
        std::snprintf(text, sizeof(text), "%s", e.what());
        return 'e';
      }
    catch (...)
      {
      }
    return '?';
  }
}

// The tests need the operator new of allocation_failure.H
struct ErrorStreamFailure : public ::testing::Test
{
  void SetUp() override
  {
    if (ALEPH_ALLOCATION_FAILURE_UNDER_TSAN)
      GTEST_SKIP() << "ThreadSanitizer replaces the global operator new";
  }
};

// The stream is the first thing that a macro that throws allocates.
// Without memory for it, the macro throws std::bad_alloc in the place of
// its exception, and has not evaluated any item.
TEST_F(ErrorStreamFailure, WithoutMemoryForItsStreamAMacroThrowsBadAlloc)
{
  Text text;
  evaluated = 0;
  char outcome = '?';
  size_t refused = 0;
  {
    Fail_At the_first(1);
    outcome = outcome_of_raise(text);
    refused = refused_allocations;
  }
  EXPECT_EQ(outcome, 'm');
  EXPECT_EQ(refused, 1u);
  EXPECT_EQ(evaluated, 0);

  {
    No_Memory no_memory;
    outcome = outcome_of_raise(text);
    refused = refused_allocations;
  }
  EXPECT_EQ(outcome, 'm');
  EXPECT_EQ(refused, 1u);
  EXPECT_EQ(evaluated, 0);

  EXPECT_EQ(outcome_of_raise(text), 'e');  // with memory, its exception
  EXPECT_EQ(evaluated, 2);
}

// Whatever allocation fails while a macro throws, the macro throws
// std::bad_alloc or its exception, whose message is then the start of the
// whole one, and its items were all evaluated or, without a stream, none.
TEST_F(ErrorStreamFailure, AnyAllocationThatFailsGivesBadAllocOrTheException)
{
  Text text;
  evaluated = 0;
  ASSERT_EQ(outcome_of_raise(text), 'e');
  const std::string whole = text;
  const size_t at = whole.find(") | ");
  ASSERT_NE(at, std::string::npos);
  EXPECT_EQ(whole.substr(at + 4),
            "an item that is longer than a short string, 1 and 2");

  size_t without_memory = 0;
  for (size_t k = 1; k <= 64; ++k)
    {
      evaluated = 0;
      char outcome = '?';
      size_t refused = 0;
      {
        Fail_At the_kth(k);
        outcome = outcome_of_raise(text);
        refused = refused_allocations;
      }
      if (refused == 0)  // the macro asks for less than k allocations
        {
          EXPECT_EQ(outcome, 'e');
          EXPECT_EQ(text, whole);
          EXPECT_EQ(evaluated, 2);
          EXPECT_GT(without_memory, 0u);
          return;
        }
      const std::string message = text;
      if (outcome == 'm')
        ++without_memory;
      else
        {
          EXPECT_EQ(outcome, 'e') << "allocation " << k;
          EXPECT_EQ(whole.compare(0, message.size(), message), 0)
            << "allocation " << k << ": " << message;
        }
      if (evaluated == 0)
        EXPECT_EQ(outcome, 'm') << "allocation " << k;
      else
        EXPECT_EQ(evaluated, 2) << "allocation " << k;
    }
  ADD_FAILURE() << "a macro that throws asks for more than 64 allocations";
}

// A place that throws leaves through the constructor of the stream, which
// frees it: the inserter above made the stream tell when it dies. The
// macro has not evaluated any item.
TEST_F(ErrorStreamFailure, APlaceThatThrowsFreesTheStream)
{
  evaluated = 0;
  streams_dead = 0;
  bool thrown = false;
  place_throws = true;
  try
    {
      raise();
    }
  catch (const Place_Failure &)
    {
      thrown = true;
    }
  place_throws = false;
  EXPECT_TRUE(thrown);
  EXPECT_EQ(streams_dead, 1);
  EXPECT_EQ(evaluated, 0);

  Text text;
  EXPECT_EQ(outcome_of_raise(text), 'e');  // and the next message is whole
  EXPECT_NE(std::string(text).find("1 and 2"), std::string::npos) << text;
}
