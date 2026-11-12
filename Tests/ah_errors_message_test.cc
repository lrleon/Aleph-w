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
 * @file ah_errors_message_test.cc
 * @brief Tests of the messages of the macros of ah-errors.H that throw.
 *
 * A macro writes its message in a stream that an object of one pointer
 * owns out of the frame of the function that checks. These tests check
 * that the text is the one that a stream gives for the same items, behind
 * the place of the call; that each item is evaluated once, in order and
 * only to throw, and read when it is inserted; that the stream has the
 * type of the temporary stream that the macros had, and is freed also
 * when an item throws; and that the checked accesses of the arrays still
 * throw with their text.
 */

# include <gtest/gtest.h>

# include <cstring>
# include <iomanip>
# include <ostream>
# include <sstream>
# include <stdexcept>
# include <string>
# include <string_view>
# include <type_traits>
# include <typeinfo>
# include <utility>

# include <ah-errors.H>
# include <bitArray.H>
# include <tpl_array.H>
# include <tpl_arrayQueue.H>
# include <tpl_dynArray.H>
# include <tpl_memArray.H>
# include <tpl_small_vector.H>

namespace
{
  // The head of the message of a macro invoked at `line` of this file
  std::string place(const int line)
  {
    return "(" + std::string(__FILE__) + ":" + std::to_string(line) + ") | ";
  }

  // The message of the exception that raise() throws, which must be an E
  // itself, and not of a type derived from E
  template <class E, class Raise>
  std::string message_of(Raise && raise)
  {
    try
      {
        std::forward<Raise>(raise)();
      }
    catch (const E & e)
      {
        EXPECT_TRUE(typeid(e) == typeid(E)) << "thrown: " << typeid(e).name();
        return e.what();
      }
    catch (...)
      {
        ADD_FAILURE() << "an exception of another type";
        return "";
      }
    ADD_FAILURE() << "no exception";
    return "";
  }

  // What follows the place in a message, and the file of the place
  std::string text_of(const std::string & message)
  {
    const size_t at = message.find(") | ");
    return at == std::string::npos ? message : message.substr(at + 4);
  }

  std::string file_of(const std::string & message)
  {
    const size_t at = message.find(") | ");
    if (message.empty() or message.front() != '(' or at == std::string::npos)
      return "";
    const std::string where = message.substr(1, at - 1);  // file:line
    return where.substr(0, where.rfind(':'));
  }

  bool ends_with(const std::string & text, const std::string & tail)
  {
    return text.size() >= tail.size() and
      text.compare(text.size() - tail.size(), tail.size(), tail) == 0;
  }

  struct Point
  {
    int x;
    int y;
  };

  std::ostream & operator << (std::ostream & out, const Point & p)
  {
    return out << "(" << p.x << ", " << p.y << ")";
  }

  enum Color { Red, Green = 5 };

  struct Fields
  {
    unsigned low : 3;
    unsigned high : 5;
  };

  std::string twice(const std::string & s) { return s + s; }

  // Throws while an item of a message is evaluated
  std::string throw_if_odd(const int n)
  {
    if (n % 2 != 0)
      throw std::underflow_error("the evaluation of an item");
    return "even";
  }

  // Its text is overwritten when it dies, so that a message formatted
  // after the end of its expression would not show it
  struct Tracked
  {
    std::string text;

    explicit Tracked(std::string t) : text(std::move(t)) {}

    ~Tracked() { text.assign(text.size(), '#'); }
  };

  std::ostream & operator << (std::ostream & out, const Tracked & t)
  {
    return out << t.text;
  }

  // Its insertion throws, after writing in the stream
  struct Throws_When_Inserted {};

  std::ostream & operator << (std::ostream & out, const Throws_When_Inserted &)
  {
    out << "half an item";
    throw std::length_error("the insertion of an item");
  }

  // Throws and catches a message of its own, and gives its text
  std::string text_of_another_message(const int n)
  {
    try
      {
        ah_domain_error_if(n > 0) << "inner " << n << " of " << twice("in");
      }
    catch (const std::domain_error & e)
      {
        return text_of(e.what());
      }
    return "nothing";
  }
}

// The message of `call items` is the place of the call, with its line,
// followed by what a stream gets from `items`, which are evaluated twice.
// The whole invocation goes on one line: the compilers do not agree on the
// line that __LINE__ gives in an invocation that spans several.
# define EXPECT_THE_WHOLE(E, call, items)                               \
  do                                                                    \
    {                                                                   \
      std::ostringstream expected;                                      \
      expected items;                                                   \
      EXPECT_EQ(message_of<E>([&] { call items; }),                     \
                place(__LINE__) + expected.str());                      \
    }                                                                   \
  while (false)

// The text of the message of `call items`, behind its place, is what a
// stream gets from `items`, which are evaluated twice. The invocation may
// span several lines: of the place, only the file is checked.
# define EXPECT_THE_ITEMS(E, call, items)                               \
  do                                                                    \
    {                                                                   \
      std::ostringstream expected;                                      \
      expected items;                                                   \
      const std::string message = message_of<E>([&] { call items; });   \
      EXPECT_EQ(file_of(message), __FILE__);                            \
      EXPECT_EQ(text_of(message), expected.str());                      \
    }                                                                   \
  while (false)

// The text of the message of `statement`, behind its place, is `text`.
# define EXPECT_THE_TEXT(E, statement, text)                            \
  do                                                                    \
    {                                                                   \
      const std::string message = message_of<E>([&] { statement; });    \
      EXPECT_EQ(file_of(message), __FILE__);                            \
      EXPECT_EQ(text_of(message), text);                                \
    }                                                                   \
  while (false)

// The five forms of a macro, whose names start with `base`.
# define EXPECT_THE_MESSAGES_OF(E, base)                                \
  do                                                                    \
    {                                                                   \
      EXPECT_THE_WHOLE(E, base ## _if(n == 7), ITEMS);                  \
      EXPECT_THE_WHOLE(E, base ## _unless(n != 7), ITEMS);              \
      EXPECT_THE_WHOLE(E, base(), ITEMS);                               \
      EXPECT_THE_WHOLE(E, base ## _if_constexpr(true), ITEMS);          \
      EXPECT_THE_WHOLE(E, base ## _unless_constexpr(false), ITEMS);     \
    }                                                                   \
  while (false)

TEST(ErrorMessage, EveryMacroGivesItsPlaceAndItsItems)
{
  const int n = 7;
  const std::string s = "seven";
# define ITEMS << "value " << n << ", " << s << ' ' << 2.5

  EXPECT_THE_MESSAGES_OF(std::range_error, ah_range_error);
  EXPECT_THE_MESSAGES_OF(std::runtime_error, ah_runtime_error);
  EXPECT_THE_MESSAGES_OF(std::logic_error, ah_logic_error);
  EXPECT_THE_MESSAGES_OF(std::underflow_error, ah_underflow_error);
  EXPECT_THE_MESSAGES_OF(std::overflow_error, ah_overflow_error);
  EXPECT_THE_MESSAGES_OF(std::domain_error, ah_domain_error);
  EXPECT_THE_MESSAGES_OF(std::out_of_range, ah_out_of_range_error);
  EXPECT_THE_MESSAGES_OF(std::invalid_argument, ah_invalid_argument);
  EXPECT_THE_MESSAGES_OF(std::length_error, ah_length_error);
  EXPECT_THE_WHOLE(std::runtime_error, ah_fatal_error(), ITEMS);

# undef ITEMS
}

TEST(ErrorMessage, AMessageWithoutItemsIsItsPlace)
{
  EXPECT_THE_WHOLE(std::range_error, ah_range_error_if(true), << "");
  EXPECT_THE_WHOLE(std::length_error, ah_length_error(), << "");
  EXPECT_THE_TEXT(std::domain_error, ah_domain_error_unless(false), "");
}

TEST(ErrorMessage, TakesTheItemsThatAStreamTakes)
{
  const char * const text = "a pointer to chars";
  const char chars[] = "an array of chars";
  const std::string str = "a string";
  const std::string_view view = "a view";
  const Point point = {3, -4};
  const Fields fields = {5, 17};
  const int number = 42;
  const int * const address = &number;
  const void * const nothing = nullptr;

  EXPECT_THE_ITEMS(std::domain_error, ah_domain_error(), << "a literal");
  EXPECT_THE_ITEMS(std::domain_error, ah_domain_error(),
                   << text << ' ' << chars << ' ' << str << ' ' << view);
  EXPECT_THE_ITEMS(std::domain_error, ah_domain_error(),
                   << 'c' << static_cast<signed char>('s')
                   << static_cast<unsigned char>('u') << true << false);
  EXPECT_THE_ITEMS(std::domain_error, ah_domain_error(),
                   << static_cast<short>(-3) << ' ' << 7u << ' ' << -9l << ' '
                   << 10ul << ' ' << -11ll << ' ' << 12ull);
  EXPECT_THE_ITEMS(std::domain_error, ah_domain_error(),
                   << 2.5f << ' ' << 0.1 << ' ' << 3.0l << ' ' << 1e300);
  EXPECT_THE_ITEMS(std::domain_error, ah_domain_error(),
                   << address << ' ' << nothing << ' '
                   << static_cast<const void *>(text));
  EXPECT_THE_ITEMS(std::domain_error, ah_domain_error(),
                   << point << ' ' << Green << ' ' << fields.low << ' '
                   << fields.high);
}

TEST(ErrorMessage, TakesTheManipulatorsOfAStream)
{
  EXPECT_THE_ITEMS(std::domain_error, ah_domain_error(),
                   << std::hex << 255 << ' ' << std::dec << 255 << ' '
                   << std::oct << 8 << ' ' << std::boolalpha << true);
  EXPECT_THE_ITEMS(std::domain_error, ah_domain_error(),
                   << std::setw(6) << std::setfill('*') << 42 << ' '
                   << std::setprecision(3) << 3.14159 << ' ' << std::fixed
                   << 2.5 << ' ' << std::showpos << 7);
  EXPECT_THE_ITEMS(std::domain_error, ah_domain_error(),
                   << "a line" << std::endl << "and another" << std::flush);
}

TEST(ErrorMessage, KeepsTheTemporariesOfItsItemsUntilItThrows)
{
  const std::string ab = "ab";

  EXPECT_THE_TEXT(std::runtime_error,
                  ah_runtime_error() << twice(ab) << ' ' << std::string(3, 'z'),
                  "abab zzz");
  EXPECT_THE_TEXT(std::runtime_error,
                  ah_runtime_error_if(ab.size() == 2)
                    << Tracked("alive") << ' ' << Tracked(twice(ab)),
                  "alive abab");
}

TEST(ErrorMessage, EvaluatesItsItemsOnceInOrderAndOnlyToThrow)
{
  int calls = 0;
  const auto next = [&calls] { return ++calls; };

  EXPECT_THE_TEXT(std::logic_error,
                  ah_logic_error_if(calls == 0)
                    << next() << ' ' << next() << ' ' << next(),
                  "1 2 3");
  EXPECT_EQ(calls, 3);

  ah_logic_error_if(calls == 0) << next() << next();
  ah_logic_error_unless(calls == 3) << next();
  EXPECT_EQ(calls, 3);
}

// An item is read when it is inserted, not when the exception is thrown:
// what a later item of the message does to it does not reach its text.
TEST(ErrorMessage, ReadsEachItemWhenItIsInserted)
{
  int i = 1;
  EXPECT_THE_TEXT(std::logic_error,
                  ah_logic_error() << i << ' ' << ++i << ' ' << i, "1 2 2");

  std::string value = "before";
  EXPECT_THE_TEXT(std::logic_error,
                  ah_logic_error() << value << '|' << (value = "after"),
                  "before|after");

  const char * word = "first";
  EXPECT_THE_TEXT(std::logic_error,
                  ah_logic_error() << word << ' ' << (word = "second"),
                  "first second");

  char buffer[8] = "one";
  EXPECT_THE_TEXT(std::logic_error,
                  ah_logic_error() << buffer << '|'
                    << std::strcpy(buffer, "two"),
                  "one|two");

  Point point = {1, 2};
  const Point other = {3, 4};
  EXPECT_THE_TEXT(std::logic_error,
                  ah_logic_error() << point << " then " << (point = other),
                  "(1, 2) then (3, 4)");
}

TEST(ErrorMessage, GoesThroughAMacroThatTakesItAsAnArgument)
{
  const int limit = 3;

  EXPECT_THROW(ah_domain_error_if(limit == 3) << "limit " << limit,
               std::domain_error);
  EXPECT_NO_THROW(ah_domain_error_if(limit != 3) << "limit " << limit);
  EXPECT_THROW(ah_out_of_range_error() << limit << ", " << limit,
               std::out_of_range);
}

// Of a message, the frame of the function that checks holds a pointer. A
// stream in it took 376 bytes with libstdc++, and a function with such a
// frame is not inlined. And the stream is given as the temporary stream
// of the macros was, so that the items go into it as they did.
TEST(ErrorMessage, TheStreamOfAMessageTakesAPointerOfTheFrame)
{
  using Aleph::error_detail::Message_Stream;

  static_assert(sizeof(Message_Stream) == sizeof(void *));
  static_assert(not std::is_copy_constructible_v<Message_Stream>);
  static_assert(not std::is_copy_assignable_v<Message_Stream>);

  using Former_Head = decltype(std::ostringstream() << "(" << __FILE__ << ":"
                               << __LINE__ << ") | ");
  static_assert(std::is_same_v<Former_Head, std::ostringstream &&>);
  using Head = decltype(std::declval<Message_Stream>().stream());
  static_assert(std::is_same_v<Head, Former_Head>);

  Message_Stream message("a file", 7);
  std::ostringstream && stream = message.stream();
  EXPECT_EQ(stream.str(), "(a file:7) | ");
  stream << "an item";
  EXPECT_EQ(message.stream().str(), "(a file:7) | an item");
}

// An exception thrown by an item leaves, in the place of that of the
// macro, and the stream of the message is freed: the sanitizers would
// report it.
TEST(ErrorMessage, AnItemThatThrowsFreesTheStream)
{
  for (int i = 0; i < 100; ++i)
    {
      EXPECT_THROW(ah_runtime_error() << "before " << Throws_When_Inserted()
                   << " after", std::length_error);
      EXPECT_THROW(ah_runtime_error_if(i >= 0) << throw_if_odd(2 * i + 1),
                   std::underflow_error);
    }
}

// A message that an item of another throws and catches does not touch
// that other.
TEST(ErrorMessage, AMessageInsideAnotherKeepsBoth)
{
  EXPECT_THE_TEXT(std::runtime_error,
                  ah_runtime_error() << "outer, " << text_of_another_message(7)
                    << ", " << text_of_another_message(0) << ", end",
                  "outer, inner 7 of inin, nothing, end");
}

TEST(ErrorMessage, ACheckedAccessOutOfRangeThrowsWithItsText)
{
  {
    Aleph::Array<int> array = {1, 2, 3};
    const std::string message =
      message_of<std::out_of_range>([&] { (void) array[3]; });
    EXPECT_EQ(text_of(message), "access out of range");
    EXPECT_TRUE(ends_with(file_of(message), "tpl_memArray.H")) << message;
    EXPECT_EQ(array[2], 3);
  }
  {
    Aleph::MemArray<int> array;
    array.put(1);
    const std::string message =
      message_of<std::out_of_range>([&] { (void) array[1]; });
    EXPECT_EQ(text_of(message), "access out of range");
    EXPECT_EQ(array[0], 1);
  }
  {
    Aleph::SmallVector<int, 4> vector;
    vector.append(1);
    const std::string message =
      message_of<std::out_of_range>([&] { (void) vector[1]; });
    EXPECT_EQ(text_of(message), "SmallVector: index out of range");
    EXPECT_TRUE(ends_with(file_of(message), "tpl_small_vector.H")) << message;
    EXPECT_EQ(vector[0], 1);
  }
  {
    Aleph::DynArray<int> array;
    array.touch(2) = 7;
    const std::string message =
      message_of<std::out_of_range>([&] { (void) array.read(5); });
    EXPECT_EQ(text_of(message),
              "DynArray::read(5): index out of range; size() is 3");
    EXPECT_TRUE(ends_with(file_of(message), "tpl_dynArray.H")) << message;
    EXPECT_EQ(array.read(2), 7);
  }
  {
    Aleph::ArrayQueue<int> queue;
    queue.put(1);
    const std::string message =
      message_of<std::out_of_range>([&] { (void) queue[1]; });
    EXPECT_EQ(text_of(message), "queue index out of range");
    EXPECT_TRUE(ends_with(file_of(message), "tpl_arrayQueue.H")) << message;
    EXPECT_EQ(queue[0], 1);
  }
  {
    Aleph::BitArray bits(8);
    const std::string message =
      message_of<std::out_of_range>([&] { (void) bits.read_bit(8); });
    EXPECT_EQ(text_of(message), "index out of range");
    EXPECT_TRUE(ends_with(file_of(message), "bitArray.H")) << message;
    EXPECT_EQ(bits.read_bit(7), 0);
  }
}
