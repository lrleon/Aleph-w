
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


/** @file ah-errors.H
 *  @brief Exception handling system with formatted messages for Aleph-w
 *
 *  This file provides a complete set of macros to throw standard C++
 *  exceptions with custom messages and automatic code location tracking.
 *
 *  @section features Main Features
 *  - Macros for all standard C++ exceptions
 *  - Formatted error messages using stream operator <<
 *  - Automatic location tracking (file:line) in each exception
 *  - Optimization with [[unlikely]] for error cases
 *  - Conditional versions (_if, _unless) for each exception type
 *
 *  @section exception_types Available Exception Types
 *  - **range_error**: Values outside valid range
 *  - **runtime_error**: Errors detected at runtime
 *  - **logic_error**: Program logic errors (bugs)
 *  - **underflow_error**: Arithmetic underflow
 *  - **overflow_error**: Arithmetic overflow
 *  - **domain_error**: Values outside function domain
 *  - **out_of_range**: Out of range access in containers
 *  - **invalid_argument**: Invalid arguments (most common)
 *  - **length_error**: Exceeds size limits
 *
 *  @section usage Usage Examples
 *  @code
 *  // Validate index range
 *  ah_out_of_range_error_if(index >= size) << "Index " << index << " out of range";
 *
 *  // Validate arguments
 *  ah_invalid_argument_unless(ptr != nullptr) << "Null pointer not allowed";
 *
 *  // Validate mathematical operations
 *  ah_domain_error_if(x < 0) << "sqrt requires non-negative value";
 *
 *  // Unrecoverable fatal error
 *  ah_fatal_error() << "Corrupted data structure detected";
 *
 *  // Warnings (do not throw exception)
 *  ah_warning_if(std::cerr, size > 1000) << "Large size may impact performance";
 *  @endcode
 *
 *  @section naming_convention Naming Convention
 *  - **ah_TYPE_error_if(C)**: Throws exception if C is true
 *  - **ah_TYPE_error_unless(C)**: Throws exception if C is false
 *  - **ah_warning_if/unless**: Emits warning without throwing exception
 *
 *  @section no_else The conditional macros take no else
 *  Every `_if` and `_unless` macro, their `_constexpr` forms included,
 *  expands to a loop whose statement runs at most once: a `while` whose
 *  statement throws, a `for` of one iteration for the warnings, and
 *  `do { ... } while (false)` for the forms of `ah_bad_alloc`. A loop takes
 *  no `else`, so an `else` written after an invocation belongs to the `if`
 *  around it, and one with no `if` around it does not compile:
 *  @code
 *  if (checked)
 *    ah_domain_error_if(x < 0) << "x is negative";
 *  else       // runs when not checked
 *    use(x);
 *  @endcode
 *  The condition is evaluated once, and it converts to bool as that of an
 *  `if` does, so a scoped enumeration does not; it must be an expression,
 *  not a declaration as in `if (auto e = f())`. The `_constexpr` forms
 *  require a constant condition, and their message is compiled even where
 *  the condition is false, so it must compile for every instantiation. A
 *  message whose last `<<` does not give back a stream does not compile.
 *
 *  @section message_stream The stream of a message is out of the frame
 *  A macro that throws writes its message in a `std::ostringstream`, with
 *  the insertions of its caller, as it always did. That stream does not
 *  live in the frame of the function that checks: an object of one
 *  pointer, error_detail::Message_Stream, makes it when the macro throws
 *  and frees it when the exception leaves. A stream in that frame, 376
 *  bytes with libstdc++ on x86-64, kept GCC from inlining a checked access
 *  as `Array::operator[]`, and each access was a call.
 *
 *  The message does not change. Its items go into a stream of the same
 *  type, in their order and when they are evaluated, and only if the macro
 *  throws. Out of the message, two things do change:
 *
 *  - The stream is allocated: a macro that throws asks for one block of
 *    memory more. Without memory for it, the macro throws `std::bad_alloc`
 *    in the place of its exception, and does not evaluate its items.
 *  - The place of the error, `(file:line) | `, is written by the library,
 *    with the inserters of the standard library. An overload of `<<` for
 *    an rvalue `std::ostringstream` that is visible where a macro is used
 *    takes the items of that use, and no longer the place.
 *
 *  @author Leandro Rabindranath Leon
 *  @date 2002-2025 *
 *  @ingroup Utilities

 */

# ifndef AH_ERRORS_H
# define AH_ERRORS_H

# include <stdexcept>
# include <sstream>
# include <type_traits>

// The functions that build the stream of a message and that throw are cold
// and out of line, so that the functions that check stay small. MSVC does
// not take the attributes of GCC, which Clang takes also as clang-cl
# if defined(__GNUC__) || defined(__clang__)
#   define AH_ERROR_COLD [[gnu::cold, gnu::noinline]]
# elif defined(_MSC_VER)
#   define AH_ERROR_COLD __declspec(noinline)
# else
#   define AH_ERROR_COLD
# endif

namespace Aleph
{

  /** @brief What the macros that throw need besides ExceptionBuilder.
   *  @ingroup Utilities
   */
  namespace error_detail
  {
    /** @brief The stream of the message of a macro that throws.
     *
     *  A macro wrote its message in a temporary `std::ostringstream`,
     *  which lived in the frame of the function that checks: 376 bytes
     *  with libstdc++ on x86-64, and GCC does not inline a function whose
     *  frame grows so. This object owns the stream out of that frame, of
     *  which it takes a pointer, and frees it when the full expression of
     *  the macro ends, which is when its exception leaves.
     *
     *  stream() gives the stream as the temporary was, an rvalue of the
     *  same type: the items of a macro are written by the same insertions,
     *  at the same time and with the same types as before. The place of
     *  the error is written here, by the inserters of the standard library
     *  and before any item is evaluated.
     *
     *  @ingroup Utilities
     */
    class Message_Stream
    {
      std::ostringstream * stream_ = nullptr;

    public:
      /** @brief Makes the stream and writes in it the place of the error.
       *  @param[in] file The file of the call, as `__FILE__` gives it.
       *  @param[in] line The line of the call.
       *  @throw std::bad_alloc If there is no memory for the stream; the
       *         macro has not evaluated any of its items then.
       */
      AH_ERROR_COLD Message_Stream(const char * file, const int line)
        : stream_(new std::ostringstream)
      {
        try
          {
            *stream_ << "(" << file << ":" << line << ") | ";
          }
        catch (...)
          {
            delete stream_;
            throw;
          }
      }

      /// Frees the stream.
      AH_ERROR_COLD ~Message_Stream() { delete stream_; }

      /// A message has one stream: it is not copied.
      Message_Stream(const Message_Stream &) = delete;

      /// A message has one stream: it is not assigned.
      Message_Stream & operator = (const Message_Stream &) = delete;

      /** @brief The stream, as the temporary stream of a macro was.
       *  @return The stream of the message, as an rvalue.
       */
      [[nodiscard]] std::ostringstream && stream() noexcept
      {
        return static_cast<std::ostringstream &&>(*stream_);
      }
    };
  }

  /** @brief Exception constructor with formatted message
   *
   *  This helper class allows building exceptions with formatted messages
   *  using the stream insertion operator (<<).
   *  Used internally by the error handling macros.
   *
   *  @tparam E Exception type to throw (must derive from std::exception) *
 *  @ingroup Utilities

   */
  template<class E>
  struct ExceptionBuilder
  {
    /** @brief Operator that throws the exception with the constructed message
     *
     *  The macros put the message on its right. `<<` binds tighter, so it
     *  takes the whole message; and with a class on its left there is no
     *  built-in `<<=`, so a message that does not give back a stream, as
     *  one whose last `<<` gives back `void`, does not compile.
     *
     *  It is cold and out of line, as what makes the stream: a function
     *  that checks has of its message only the insertions of its items.
     *
     *  @param s Stream with the formatted error message
     *  @return Does not return (throws exception)
     *  @throw E Exception of the specified type with the constructed message *
 *  @ingroup Utilities

     */
    [[noreturn]] AH_ERROR_COLD int operator<<=(const std::ostream &s)
    {
      if (const auto * oss = dynamic_cast<const std::ostringstream *>(&s))
        throw E(oss->str());

      if (const auto * ss = dynamic_cast<const std::stringstream *>(&s))
        throw E(ss->str());

      throw E("Unknown stream type in Aleph::ExceptionBuilder");
    }
  };

}

// ============================================================================
// WARNING MACROS
// ============================================================================

// The variable of the loop of a warning has the line of the call in its
// name, so that it does not hide a name of the caller in C or in out
#define AH_ERRORS_JOIN_(a, b) a ## b
#define AH_ERRORS_JOIN(a, b) AH_ERRORS_JOIN_(a, b)
#define AH_WARNING_ONCE AH_ERRORS_JOIN(ah_warning_once_, __LINE__)

// The statement of the while of a macro that throws is unlikely. Clang warns
// that [[unlikely]] has no effect on a loop whose condition is constant, as
// ah_domain_error_if(true), and it gives the same code without it
# if defined(__clang__)
#   define AH_ERROR_UNLIKELY
# else
#   define AH_ERROR_UNLIKELY [[unlikely]]
# endif

/** @def ah_warning_unless(out, C)
 *  @brief Emits a warning to a stream if the condition does NOT hold
 *  @param out Output stream where to write the warning
 *  @param C Condition to evaluate
 *  @details If condition C is false, writes a warning message with file
 *           and line information. Typical use with std::cerr.
 *  @code
 *  ah_warning_unless(std::cerr, ptr != nullptr) << "Pointer is null";
 *  @endcode *
 *  @ingroup Utilities

 */
#define ah_warning_unless(out, C)                                       \
  for (bool AH_WARNING_ONCE = not (C); AH_WARNING_ONCE;                 \
       AH_WARNING_ONCE = false)                                         \
    [[unlikely]](out) << "WARNING (" << __FILE__ << ":" << __LINE__ << ") | "

/** @def ah_warning_if(out, C)
 *  @brief Emits a warning to a stream if the condition holds
 *  @param out Output stream where to write the warning
 *  @param C Condition to evaluate
 *  @details If condition C is true, writes a warning message with file
 *           and line information.
 *  @code
 *  ah_warning_if(std::cerr, size > MAX_SIZE) << "Size exceeds maximum";
 *  @endcode *
 *  @ingroup Utilities

 */
#define ah_warning_if(out, C)                                           \
  for (bool AH_WARNING_ONCE = (C) ? true : false; AH_WARNING_ONCE;      \
       AH_WARNING_ONCE = false)                                         \
    [[unlikely]](out) << "WARNING (" << __FILE__ << ":" << __LINE__ << ") | "

/** @def ah_warning(out)
 *  @brief Emits an unconditional warning to a stream
 *  @param out Output stream where to write the warning
 *  @details Writes a warning with file and line information without
 *           evaluating any condition.
 *  @code
 *  ah_warning(std::cerr) << "Deprecated function called";
 *  @endcode *
 *  @ingroup Utilities

 */
#define ah_warning(out)                                                 \
  (out) << "WARNING (" << __FILE__ << ":" << __LINE__ << ") | "

// ============================================================================
// EXCEPTION MACROS: RANGE_ERROR
// ============================================================================

/** @def ah_range_error_unless(C)
 *  @brief Throws std::range_error if condition does NOT hold
 *  @param C Condition to evaluate
 *  @throw std::range_error if C is false
 *  @details Useful for validating that a value is within a valid range.
 *           Error message includes location (file:line).
 *  @code
 *  ah_range_error_unless(index < size) << "Index " << index << " out of range";
 *  @endcode *
 *  @ingroup Utilities

 */
#define ah_range_error_unless(C)                                        \
  while (not (C)) AH_ERROR_UNLIKELY                                     \
    Aleph::ExceptionBuilder<std::range_error>() <<=                     \
      Aleph::error_detail::Message_Stream(__FILE__, __LINE__).stream()

/** @def ah_range_error_if(C)
 *  @brief Throws std::range_error if condition holds
 *  @param C Condition to evaluate
 *  @throw std::range_error if C is true
 *  @details Useful for detecting values out of range.
 *  @code
 *  ah_range_error_if(value < min || value > max) << "Value out of range";
 *  @endcode *
 *  @ingroup Utilities

 */
#define ah_range_error_if(C)                                            \
  while ((C) ? true : false) AH_ERROR_UNLIKELY                          \
    Aleph::ExceptionBuilder<std::range_error>() <<=                     \
      Aleph::error_detail::Message_Stream(__FILE__, __LINE__).stream()

/** @def ah_range_error()
 *  @brief Throws std::range_error unconditionally
 *  @throw std::range_error always
 *  @details Throws a range_error without evaluating any condition.
 *           Use when you need to throw directly.
 *  @code
 *  ah_range_error() << "Value is out of valid range";
 *  @endcode *
 *  @ingroup Utilities

 */
#define ah_range_error()                                                \
  Aleph::ExceptionBuilder<std::range_error>() <<=                       \
    Aleph::error_detail::Message_Stream(__FILE__, __LINE__).stream()

#define ah_range_error_if_constexpr(C)                                  \
  while (std::bool_constant<(C) ? true : false>::value)                 \
    ah_range_error()

#define ah_range_error_unless_constexpr(C)                              \
  while (std::bool_constant<not (C)>::value)                            \
    ah_range_error()

// ============================================================================
// EXCEPTION MACROS: RUNTIME_ERROR
// ============================================================================

/** @def ah_runtime_error_unless(C)
 *  @brief Throws std::runtime_error if condition does NOT hold
 *  @param C Condition to evaluate
 *  @throw std::runtime_error if C is false
 *  @details Useful for errors only detected at runtime.
 *  @code
 *  ah_runtime_error_unless(file.is_open()) << "Failed to open file";
 *  @endcode *
 *  @ingroup Utilities

 */
#define ah_runtime_error_unless(C)                                      \
  while (not (C)) AH_ERROR_UNLIKELY                                     \
    Aleph::ExceptionBuilder<std::runtime_error>() <<=                   \
      Aleph::error_detail::Message_Stream(__FILE__, __LINE__).stream()

/** @def ah_runtime_error_if(C)
 *  @brief Throws std::runtime_error if condition holds
 *  @param C Condition to evaluate
 *  @throw std::runtime_error if C is true
 *  @details Useful for detecting runtime error conditions.
 *  @code
 *  ah_runtime_error_if(errno != 0) << "System error: " << strerror(errno);
 *  @endcode *
 *  @ingroup Utilities

 */
#define ah_runtime_error_if(C)                                          \
  while ((C) ? true : false) AH_ERROR_UNLIKELY                          \
    Aleph::ExceptionBuilder<std::runtime_error>() <<=                   \
      Aleph::error_detail::Message_Stream(__FILE__, __LINE__).stream()

/** @def ah_runtime_error()
 *  @brief Throws std::runtime_error unconditionally
 *  @throw std::runtime_error always
 *  @details Throws a runtime_error without evaluating any condition.
 *           Use when you need to throw directly.
 *  @code
 *  ah_runtime_error() << "Unexpected runtime error occurred";
 *  @endcode *
 *  @ingroup Utilities

 */
#define ah_runtime_error()                                              \
  Aleph::ExceptionBuilder<std::runtime_error>() <<=                     \
    Aleph::error_detail::Message_Stream(__FILE__, __LINE__).stream()

#define ah_runtime_error_if_constexpr(C)                                \
  while (std::bool_constant<(C) ? true : false>::value)                 \
    ah_runtime_error()

#define ah_runtime_error_unless_constexpr(C)                            \
  while (std::bool_constant<not (C)>::value)                            \
    ah_runtime_error()

// ============================================================================
// EXCEPTION MACROS: LOGIC_ERROR
// ============================================================================

/** @def ah_logic_error_unless(C)
 *  @brief Throws std::logic_error if condition does NOT hold
 *  @param C Condition to evaluate
 *  @throw std::logic_error if C is false
 *  @details Useful for program logic errors (bugs, violated invariants).
 *  @code
 *  ah_logic_error_unless(is_valid_state()) << "Invalid object state";
 *  @endcode *
 *  @ingroup Utilities

 */
#define ah_logic_error_unless(C)                                        \
  while (not (C)) AH_ERROR_UNLIKELY                                     \
    Aleph::ExceptionBuilder<std::logic_error>() <<=                     \
      Aleph::error_detail::Message_Stream(__FILE__, __LINE__).stream()

/** @def ah_logic_error_if(C)
 *  @brief Throws std::logic_error if condition holds
 *  @param C Condition to evaluate
 *  @throw std::logic_error if C is true
 *  @details Useful for detecting program logic errors.
 *  @code
 *  ah_logic_error_if(ptr == nullptr) << "Unexpected null pointer";
 *  @endcode *
 *  @ingroup Utilities

 */
#define ah_logic_error_if(C)                                            \
  while ((C) ? true : false) AH_ERROR_UNLIKELY                          \
    Aleph::ExceptionBuilder<std::logic_error>() <<=                     \
      Aleph::error_detail::Message_Stream(__FILE__, __LINE__).stream()

/** @def ah_logic_error()
 *  @brief Throws std::logic_error unconditionally
 *  @throw std::logic_error always
 *  @details Throws a logic_error without evaluating any condition.
 *           Use when you need to throw directly.
 *  @code
 *  ah_logic_error() << "Program logic invariant violated";
 *  @endcode *
 *  @ingroup Utilities

 */
#define ah_logic_error()                                                \
  Aleph::ExceptionBuilder<std::logic_error>() <<=                       \
    Aleph::error_detail::Message_Stream(__FILE__, __LINE__).stream()

#define ah_logic_error_if_constexpr(C)                                  \
  while (std::bool_constant<(C) ? true : false>::value)                 \
    ah_logic_error()

#define ah_logic_error_unless_constexpr(C)                              \
  while (std::bool_constant<not (C)>::value)                            \
    ah_logic_error()

// ============================================================================
// EXCEPTION MACROS: UNDERFLOW_ERROR
// ============================================================================

/** @def ah_underflow_error_if(C)
 *  @brief Throws std::underflow_error if condition holds
 *  @param C Condition to evaluate
 *  @throw std::underflow_error if C is true
 *  @details Useful for arithmetic operations that result in underflow.
 *  @code
 *  ah_underflow_error_if(result < min_value) << "Arithmetic underflow";
 *  @endcode *
 *  @ingroup Utilities

 */
#define ah_underflow_error_if(C)                                        \
  while ((C) ? true : false) AH_ERROR_UNLIKELY                          \
    Aleph::ExceptionBuilder<std::underflow_error>() <<=                 \
      Aleph::error_detail::Message_Stream(__FILE__, __LINE__).stream()

/** @def ah_underflow_error_unless(C)
 *  @brief Throws std::underflow_error if condition does NOT hold
 *  @param C Condition to evaluate
 *  @throw std::underflow_error if C is false
 *  @details Useful for validating absence of arithmetic underflow.
 *  @code
 *  ah_underflow_error_unless(value >= min_limit) << "Value too small";
 *  @endcode *
 *  @ingroup Utilities

 */
#define ah_underflow_error_unless(C)                                    \
  while (not (C)) AH_ERROR_UNLIKELY                                     \
    Aleph::ExceptionBuilder<std::underflow_error>() <<=                 \
      Aleph::error_detail::Message_Stream(__FILE__, __LINE__).stream()

/** @def ah_underflow_error()
 *  @brief Throws std::underflow_error unconditionally
 *  @throw std::underflow_error always
 *  @details Throws an underflow_error without evaluating any condition.
 *           Use when you need to throw directly.
 *  @code
 *  ah_underflow_error() << "Arithmetic underflow detected";
 *  @endcode *
 *  @ingroup Utilities

 */
#define ah_underflow_error()                                            \
  Aleph::ExceptionBuilder<std::underflow_error>() <<=                   \
    Aleph::error_detail::Message_Stream(__FILE__, __LINE__).stream()

#define ah_underflow_error_if_constexpr(C)                              \
  while (std::bool_constant<(C) ? true : false>::value)                 \
    ah_underflow_error()

#define ah_underflow_error_unless_constexpr(C)                          \
  while (std::bool_constant<not (C)>::value)                            \
    ah_underflow_error()

// ============================================================================
// EXCEPTION MACROS: BAD_ALLOC (NO MESSAGE)
// ============================================================================

/** @def ah_bad_alloc()
 *  @brief Throws std::bad_alloc unconditionally (no message stream)
 *
 *  @ingroup Utilities
 */
#define ah_bad_alloc()                                                   \
  do { [[unlikely]] throw std::bad_alloc(); } while (false)

/** @def ah_bad_alloc_if(C)
 *  @brief Throws std::bad_alloc if condition holds
 *
 *  @ingroup Utilities
 */
#define ah_bad_alloc_if(C)                                              \
  do { if ((C) ? true : false) [[unlikely]] throw std::bad_alloc(); }   \
  while (false)

/** @def ah_bad_alloc_unless(C)
 *  @brief Throws std::bad_alloc if condition does NOT hold
 *
 *  @ingroup Utilities
 */
#define ah_bad_alloc_unless(C)                                           \
  do { if (!(C)) [[unlikely]] throw std::bad_alloc(); } while (false)

#define ah_bad_alloc_if_constexpr(C)                                    \
  do { if constexpr (C) throw std::bad_alloc(); } while (false)

#define ah_bad_alloc_unless_constexpr(C)                                \
  do { if constexpr (not (C)) throw std::bad_alloc(); } while (false)

// ============================================================================
// EXCEPTION MACROS: OVERFLOW_ERROR
// ============================================================================

/** @def ah_overflow_error_if(C)
 *  @brief Throws std::overflow_error if condition holds
 *  @param C Condition to evaluate
 *  @throw std::overflow_error if C is true
 *  @details Useful for arithmetic operations that result in overflow.
 *  @code
 *  ah_overflow_error_if(result > max_value) << "Arithmetic overflow";
 *  @endcode *
 *  @ingroup Utilities

 */
#define ah_overflow_error_if(C)                                         \
  while ((C) ? true : false) AH_ERROR_UNLIKELY                          \
    Aleph::ExceptionBuilder<std::overflow_error>() <<=                  \
      Aleph::error_detail::Message_Stream(__FILE__, __LINE__).stream()

/** @def ah_overflow_error_unless(C)
 *  @brief Throws std::overflow_error if condition does NOT hold
 *  @param C Condition to evaluate
 *  @throw std::overflow_error if C is false
 *  @details Useful for validating absence of arithmetic overflow.
 *  @code
 *  ah_overflow_error_unless(value <= max_limit) << "Value too large";
 *  @endcode *
 *  @ingroup Utilities

 */
#define ah_overflow_error_unless(C)                                     \
  while (not (C)) AH_ERROR_UNLIKELY                                     \
    Aleph::ExceptionBuilder<std::overflow_error>() <<=                  \
      Aleph::error_detail::Message_Stream(__FILE__, __LINE__).stream()

/** @def ah_overflow_error()
 *  @brief Throws std::overflow_error unconditionally
 *  @throw std::overflow_error always
 *  @details Throws an overflow_error without evaluating any condition.
 *           Use when you need to throw directly.
 *  @code
 *  ah_overflow_error() << "Arithmetic overflow detected";
 *  @endcode *
 *  @ingroup Utilities

 */
#define ah_overflow_error()                                             \
  Aleph::ExceptionBuilder<std::overflow_error>() <<=                    \
    Aleph::error_detail::Message_Stream(__FILE__, __LINE__).stream()

#define ah_overflow_error_if_constexpr(C)                               \
  while (std::bool_constant<(C) ? true : false>::value)                 \
    ah_overflow_error()

#define ah_overflow_error_unless_constexpr(C)                           \
  while (std::bool_constant<not (C)>::value)                            \
    ah_overflow_error()

// ============================================================================
// EXCEPTION MACROS: DOMAIN_ERROR
// ============================================================================

/** @def ah_domain_error_if(C)
 *  @brief Throws std::domain_error if condition holds
 *  @param C Condition to evaluate
 *  @throw std::domain_error if C is true
 *  @details Useful when a value is outside the valid domain of a function.
 *  @code
 *  ah_domain_error_if(x < 0) << "sqrt requires non-negative input";
 *  @endcode *
 *  @ingroup Utilities

 */
#define ah_domain_error_if(C)                                           \
  while ((C) ? true : false) AH_ERROR_UNLIKELY                          \
    Aleph::ExceptionBuilder<std::domain_error>() <<=                    \
      Aleph::error_detail::Message_Stream(__FILE__, __LINE__).stream()

/** @def ah_domain_error_unless(C)
 *  @brief Throws std::domain_error if condition does NOT hold
 *  @param C Condition to evaluate
 *  @throw std::domain_error if C is false
 *  @details Useful for validating that a value is in the valid domain.
 *  @code
 *  ah_domain_error_unless(divisor != 0) << "Division by zero";
 *  @endcode *
 *  @ingroup Utilities

 */
#define ah_domain_error_unless(C)                                       \
  while (not (C)) AH_ERROR_UNLIKELY                                     \
    Aleph::ExceptionBuilder<std::domain_error>() <<=                    \
      Aleph::error_detail::Message_Stream(__FILE__, __LINE__).stream()

/** @def ah_domain_error()
 *  @brief Throws std::domain_error unconditionally
 *  @throw std::domain_error always
 *  @details Throws a domain_error without evaluating any condition.
 *           Use when you need to throw directly.
 *  @code
 *  ah_domain_error() << "Value outside function domain";
 *  @endcode *
 *  @ingroup Utilities

 */
#define ah_domain_error()                                               \
  Aleph::ExceptionBuilder<std::domain_error>() <<=                      \
    Aleph::error_detail::Message_Stream(__FILE__, __LINE__).stream()
#define ah_domain_error_if_constexpr(C)                                 \
  while (std::bool_constant<(C) ? true : false>::value)                 \
    ah_domain_error()
#define ah_domain_error_unless_constexpr(C)                             \
  while (std::bool_constant<not (C)>::value)                            \
    ah_domain_error()

// ============================================================================
// EXCEPTION MACROS: OUT_OF_RANGE
// ============================================================================

/** @def ah_out_of_range_error_if(C)
 *  @brief Throws std::out_of_range if condition holds
 *  @param C Condition to evaluate
 *  @throw std::out_of_range if C is true
 *  @details Useful for out of range element access in containers.
 *  @code
 *  ah_out_of_range_error_if(index >= size) << "Index out of bounds";
 *  @endcode *
 *  @ingroup Utilities

 */
#define ah_out_of_range_error_if(C)                                     \
  while ((C) ? true : false) AH_ERROR_UNLIKELY                          \
    Aleph::ExceptionBuilder<std::out_of_range>() <<=                    \
      Aleph::error_detail::Message_Stream(__FILE__, __LINE__).stream()

/** @def ah_out_of_range_error_unless(C)
 *  @brief Throws std::out_of_range if condition does NOT hold
 *  @param C Condition to evaluate
 *  @throw std::out_of_range if C is false
 *  @details Useful for validating valid index access.
 *  @code
 *  ah_out_of_range_error_unless(index < size) << "Invalid index";
 *  @endcode *
 *  @ingroup Utilities

 */
#define ah_out_of_range_error_unless(C)                                 \
  while (not (C)) AH_ERROR_UNLIKELY                                     \
    Aleph::ExceptionBuilder<std::out_of_range>() <<=                    \
      Aleph::error_detail::Message_Stream(__FILE__, __LINE__).stream()

/** @def ah_out_of_range_error()
 *  @brief Throws std::out_of_range unconditionally
 *  @throw std::out_of_range always
 *  @details Throws an out_of_range error without evaluating any condition.
 *           Use when you need to throw directly.
 *  @code
 *  ah_out_of_range_error() << "Index out of bounds";
 *  @endcode *
 *  @ingroup Utilities

 */
#define ah_out_of_range_error()                                         \
  Aleph::ExceptionBuilder<std::out_of_range>() <<=                      \
    Aleph::error_detail::Message_Stream(__FILE__, __LINE__).stream()

#define ah_out_of_range_error_if_constexpr(C)                           \
  while (std::bool_constant<(C) ? true : false>::value)                 \
    ah_out_of_range_error()

#define ah_out_of_range_error_unless_constexpr(C)                       \
  while (std::bool_constant<not (C)>::value)                            \
    ah_out_of_range_error()

// ============================================================================
// EXCEPTION MACROS: INVALID_ARGUMENT
// ============================================================================

/** @def ah_invalid_argument_if(C)
 *  @brief Throws std::invalid_argument if condition holds
 *  @param C Condition to evaluate
 *  @throw std::invalid_argument if C is true
 *  @details Useful for validating function arguments. Most common exception
 *           for incorrect parameter validation.
 *  @code
 *  ah_invalid_argument_if(name.empty()) << "Name cannot be empty";
 *  @endcode *
 *  @ingroup Utilities

 */
#define ah_invalid_argument_if(C)                                       \
  while ((C) ? true : false) AH_ERROR_UNLIKELY                          \
    Aleph::ExceptionBuilder<std::invalid_argument>() <<=                \
      Aleph::error_detail::Message_Stream(__FILE__, __LINE__).stream()

/** @def ah_invalid_argument_unless(C)
 *  @brief Throws std::invalid_argument if condition does NOT hold
 *  @param C Condition to evaluate
 *  @throw std::invalid_argument if C is false
 *  @details Useful for validating that arguments meet required conditions.
 *  @code
 *  ah_invalid_argument_unless(size > 0) << "Size must be positive";
 *  @endcode *
 *  @ingroup Utilities

 */
#define ah_invalid_argument_unless(C)                                   \
  while (not (C)) AH_ERROR_UNLIKELY                                     \
    Aleph::ExceptionBuilder<std::invalid_argument>() <<=                \
      Aleph::error_detail::Message_Stream(__FILE__, __LINE__).stream()

/** @def ah_invalid_argument()
 *  @brief Throws std::invalid_argument unconditionally
 *  @throw std::invalid_argument always
 *  @details Throws an invalid_argument error without evaluating any condition.
 *           Use when you need to throw directly.
 *  @code
 *  ah_invalid_argument() << "Invalid argument provided";
 *  @endcode *
 *  @ingroup Utilities

 */
#define ah_invalid_argument()                                           \
  Aleph::ExceptionBuilder<std::invalid_argument>() <<=                  \
    Aleph::error_detail::Message_Stream(__FILE__, __LINE__).stream()

#define ah_invalid_argument_if_constexpr(C)                             \
  while (std::bool_constant<(C) ? true : false>::value)                 \
    ah_invalid_argument()

#define ah_invalid_argument_unless_constexpr(C)                         \
  while (std::bool_constant<not (C)>::value)                            \
    ah_invalid_argument()

// ============================================================================
// EXCEPTION MACROS: LENGTH_ERROR
// ============================================================================

/** @def ah_length_error_if(C)
 *  @brief Throws std::length_error if condition holds
 *  @param C Condition to evaluate
 *  @throw std::length_error if C is true
 *  @details Useful when an operation exceeds container size limits.
 *  @code
 *  ah_length_error_if(new_size > max_size()) << "Container size exceeded";
 *  @endcode *
 *  @ingroup Utilities

 */
#define ah_length_error_if(C)                                           \
  while ((C) ? true : false) AH_ERROR_UNLIKELY                          \
    Aleph::ExceptionBuilder<std::length_error>() <<=                    \
      Aleph::error_detail::Message_Stream(__FILE__, __LINE__).stream()

/** @def ah_length_error_unless(C)
 *  @brief Throws std::length_error if condition does NOT hold
 *  @param C Condition to evaluate
 *  @throw std::length_error if C is false
 *  @details Useful for validating that size is within allowed limits.
 *  @code
 *  ah_length_error_unless(size <= max_size()) << "Size exceeds limit";
 *  @endcode *
 *  @ingroup Utilities

 */
#define ah_length_error_unless(C)                                       \
  while (not (C)) AH_ERROR_UNLIKELY                                     \
    Aleph::ExceptionBuilder<std::length_error>() <<=                    \
      Aleph::error_detail::Message_Stream(__FILE__, __LINE__).stream()

/** @def ah_length_error()
 *  @brief Throws std::length_error unconditionally
 *  @throw std::length_error always
 *  @details Throws a length_error without evaluating any condition.
 *           Use when you need to throw directly.
 *  @code
 *  ah_length_error() << "Container size limit exceeded";
 *  @endcode *
 *  @ingroup Utilities

 */
#define ah_length_error()                                               \
  Aleph::ExceptionBuilder<std::length_error>() <<=                      \
    Aleph::error_detail::Message_Stream(__FILE__, __LINE__).stream()

#define ah_length_error_if_constexpr(C)                                 \
  while (std::bool_constant<(C) ? true : false>::value)                 \
    ah_length_error()

#define ah_length_error_unless_constexpr(C)                             \
  while (std::bool_constant<not (C)>::value)                            \
    ah_length_error()

// ============================================================================
// FATAL ERROR MACRO
// ============================================================================

/** @def ah_fatal_error()
 *  @brief Throws an unconditional std::runtime_error (fatal error)
 *  @throw std::runtime_error always
 *  @details Useful for situations that should never occur or unrecoverable
 *           errors. Does not require a condition.
 *  @code
 *  if (impossible_condition) {
 *    ah_fatal_error() << "This should never happen!";
 *  }
 *  @endcode *
 *  @ingroup Utilities

 */
#define ah_fatal_error()                                                \
  Aleph::ExceptionBuilder<std::runtime_error>() <<=                     \
    Aleph::error_detail::Message_Stream(__FILE__, __LINE__).stream()

# endif
