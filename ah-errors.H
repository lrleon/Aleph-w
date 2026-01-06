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
 *  @author Leandro Rabindranath Leon
 *  @date 2002-2025 *
 *  @ingroup Utilidades

 */

# ifndef AH_ERRORS_H
# define AH_ERRORS_H

# include <stdexcept>
# include <sstream>

namespace Aleph
{

  /** @brief Exception constructor with formatted message
   *
   *  This helper class allows building exceptions with formatted messages
   *  using the stream insertion operator (<<).
   *  Used internally by the error handling macros.
   *
   *  @tparam E Exception type to throw (must derive from std::exception) *
 *  @ingroup Utilidades

   */
  template<class E>
  struct ExceptionBuilder
  {
    /** @brief Comma operator that throws the exception with the constructed message
     *
     *  @param s Stream with the formatted error message
     *  @return Does not return (throws exception)
     *  @throw E Exception of the specified type with the constructed message *
 *  @ingroup Utilidades

     */
    int operator,(const std::ostream &s)
    {
      throw E(static_cast<const std::ostringstream &>(s).str());
      return 0;
    }
  };

}

// ============================================================================
// WARNING MACROS
// ============================================================================

/** @def ah_warning_unless(out, C)
 *  @brief Emits a warning to a stream if the condition does NOT hold
 *  @param out Output stream where to write the warning
 *  @param C Condition to evaluate
 *  @details If condition C is false, writes a warning message with file
 *           and line information. Typical use with std::cerr.
 *  @code
 *  ah_warning_unless(std::cerr, ptr != nullptr) << "Pointer is null";
 *  @endcode *
 *  @ingroup Utilidades

 */
#define ah_warning_unless(out, C)                                       \
  if (not (C))                                                          \
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
 *  @ingroup Utilidades

 */
#define ah_warning_if(out, C)                                           \
  if (C)                                                                \
    [[unlikely]](out) << "WARNING (" << __FILE__ << ":" << __LINE__ << ") | "

/** @def ah_warning(out)
 *  @brief Emits an unconditional warning to a stream
 *  @param out Output stream where to write the warning
 *  @details Writes a warning with file and line information without
 *           evaluating any condition.
 *  @code
 *  ah_warning(std::cerr) << "Deprecated function called";
 *  @endcode *
 *  @ingroup Utilidades

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
 *  @ingroup Utilidades

 */
#define ah_range_error_unless(C)                                        \
  if (not (C))                                                          \
    [[unlikely]]Aleph::ExceptionBuilder<std::range_error>(),            \
      std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

/** @def ah_range_error_if(C)
 *  @brief Throws std::range_error if condition holds
 *  @param C Condition to evaluate
 *  @throw std::range_error if C is true
 *  @details Useful for detecting values out of range.
 *  @code
 *  ah_range_error_if(value < min || value > max) << "Value out of range";
 *  @endcode *
 *  @ingroup Utilidades

 */
#define ah_range_error_if(C)                                            \
  if (C)                                                                \
    [[unlikely]]Aleph::ExceptionBuilder<std::range_error>(),            \
      std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

/** @def ah_range_error()
 *  @brief Throws std::range_error unconditionally
 *  @throw std::range_error always
 *  @details Throws a range_error without evaluating any condition.
 *           Use when you need to throw directly.
 *  @code
 *  ah_range_error() << "Value is out of valid range";
 *  @endcode *
 *  @ingroup Utilidades

 */
#define ah_range_error()                                                \
  Aleph::ExceptionBuilder<std::range_error>(),                          \
    std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

#define ah_range_error_if_constexpr(C)                                  \
  if constexpr (C)                                                      \
    ah_range_error()

#define ah_range_error_unless_constexpr(C)                              \
  if constexpr (not (C))                                                \
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
 *  @ingroup Utilidades

 */
#define ah_runtime_error_unless(C)                                      \
  if (not (C))                                                          \
    [[unlikely]]Aleph::ExceptionBuilder<std::runtime_error>(),          \
      std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

/** @def ah_runtime_error_if(C)
 *  @brief Throws std::runtime_error if condition holds
 *  @param C Condition to evaluate
 *  @throw std::runtime_error if C is true
 *  @details Useful for detecting runtime error conditions.
 *  @code
 *  ah_runtime_error_if(errno != 0) << "System error: " << strerror(errno);
 *  @endcode *
 *  @ingroup Utilidades

 */
#define ah_runtime_error_if(C)                                          \
  if (C)                                                                \
    [[unlikely]]Aleph::ExceptionBuilder<std::runtime_error>(),          \
      std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

/** @def ah_runtime_error()
 *  @brief Throws std::runtime_error unconditionally
 *  @throw std::runtime_error always
 *  @details Throws a runtime_error without evaluating any condition.
 *           Use when you need to throw directly.
 *  @code
 *  ah_runtime_error() << "Unexpected runtime error occurred";
 *  @endcode *
 *  @ingroup Utilidades

 */
#define ah_runtime_error()                                              \
  Aleph::ExceptionBuilder<std::runtime_error>(),                        \
    std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

#define ah_runtime_error_if_constexpr(C)                                \
  if constexpr (C)                                                      \
    ah_runtime_error()

#define ah_runtime_error_unless_constexpr(C)                            \
  if constexpr (not (C))                                                \
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
 *  @ingroup Utilidades

 */
#define ah_logic_error_unless(C)                                        \
  if (not (C))                                                          \
    [[unlikely]]Aleph::ExceptionBuilder<std::logic_error>(),            \
      std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

/** @def ah_logic_error_if(C)
 *  @brief Throws std::logic_error if condition holds
 *  @param C Condition to evaluate
 *  @throw std::logic_error if C is true
 *  @details Useful for detecting program logic errors.
 *  @code
 *  ah_logic_error_if(ptr == nullptr) << "Unexpected null pointer";
 *  @endcode *
 *  @ingroup Utilidades

 */
#define ah_logic_error_if(C)                                            \
  if (C)                                                                \
    [[unlikely]]Aleph::ExceptionBuilder<std::logic_error>(),            \
      std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

/** @def ah_logic_error()
 *  @brief Throws std::logic_error unconditionally
 *  @throw std::logic_error always
 *  @details Throws a logic_error without evaluating any condition.
 *           Use when you need to throw directly.
 *  @code
 *  ah_logic_error() << "Program logic invariant violated";
 *  @endcode *
 *  @ingroup Utilidades

 */
#define ah_logic_error()                                                \
  Aleph::ExceptionBuilder<std::logic_error>(),                          \
    std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

#define ah_logic_error_if_constexpr(C)                                  \
  if constexpr (C)                                                      \
    ah_logic_error()

#define ah_logic_error_unless_constexpr(C)                              \
  if constexpr (not (C))                                                \
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
 *  @ingroup Utilidades

 */
#define ah_underflow_error_if(C)                                        \
  if (C)                                                                \
    [[unlikely]]Aleph::ExceptionBuilder<std::underflow_error>(),        \
      std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

/** @def ah_underflow_error_unless(C)
 *  @brief Throws std::underflow_error if condition does NOT hold
 *  @param C Condition to evaluate
 *  @throw std::underflow_error if C is false
 *  @details Useful for validating absence of arithmetic underflow.
 *  @code
 *  ah_underflow_error_unless(value >= min_limit) << "Value too small";
 *  @endcode *
 *  @ingroup Utilidades

 */
#define ah_underflow_error_unless(C)                                    \
  if (not (C))                                                          \
    [[unlikely]]Aleph::ExceptionBuilder<std::underflow_error>(),        \
      std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

/** @def ah_underflow_error()
 *  @brief Throws std::underflow_error unconditionally
 *  @throw std::underflow_error always
 *  @details Throws an underflow_error without evaluating any condition.
 *           Use when you need to throw directly.
 *  @code
 *  ah_underflow_error() << "Arithmetic underflow detected";
 *  @endcode *
 *  @ingroup Utilidades

 */
#define ah_underflow_error()                                            \
  Aleph::ExceptionBuilder<std::underflow_error>(),                      \
    std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

#define ah_underflow_error_if_constexpr(C)                              \
  if constexpr (C)                                                      \
    ah_underflow_error()

#define ah_underflow_error_unless_constexpr(C)                          \
  if constexpr (not (C))                                                \
    ah_underflow_error()

// ============================================================================
// EXCEPTION MACROS: BAD_ALLOC (NO MESSAGE)
// ============================================================================

/** @def ah_bad_alloc()
 *  @brief Throws std::bad_alloc unconditionally (no message stream)
 *
 *  @ingroup Utilidades
 */
#define ah_bad_alloc()                                                   \
  do { [[unlikely]] throw std::bad_alloc(); } while (false)

/** @def ah_bad_alloc_if(C)
 *  @brief Throws std::bad_alloc if condition holds
 *
 *  @ingroup Utilidades
 */
#define ah_bad_alloc_if(C)                                               \
  do { if (C) [[unlikely]] throw std::bad_alloc(); } while (false)

/** @def ah_bad_alloc_unless(C)
 *  @brief Throws std::bad_alloc if condition does NOT hold
 *
 *  @ingroup Utilidades
 */
#define ah_bad_alloc_unless(C)                                           \
  do { if (!(C)) [[unlikely]] throw std::bad_alloc(); } while (false)

#define ah_bad_alloc_if_constexpr(C)                                    \
  if constexpr (C)                                                      \
    ah_bad_alloc()

#define ah_bad_alloc_unless_constexpr(C)                                \
  if constexpr (not (C))                                                \
    ah_bad_alloc()

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
 *  @ingroup Utilidades

 */
#define ah_overflow_error_if(C)                                         \
  if (C)                                                                \
    [[unlikely]]Aleph::ExceptionBuilder<std::overflow_error>(),         \
      std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

/** @def ah_overflow_error_unless(C)
 *  @brief Throws std::overflow_error if condition does NOT hold
 *  @param C Condition to evaluate
 *  @throw std::overflow_error if C is false
 *  @details Useful for validating absence of arithmetic overflow.
 *  @code
 *  ah_overflow_error_unless(value <= max_limit) << "Value too large";
 *  @endcode *
 *  @ingroup Utilidades

 */
#define ah_overflow_error_unless(C)                                     \
  if (not (C))                                                          \
    [[unlikely]]Aleph::ExceptionBuilder<std::overflow_error>(),         \
      std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

/** @def ah_overflow_error()
 *  @brief Throws std::overflow_error unconditionally
 *  @throw std::overflow_error always
 *  @details Throws an overflow_error without evaluating any condition.
 *           Use when you need to throw directly.
 *  @code
 *  ah_overflow_error() << "Arithmetic overflow detected";
 *  @endcode *
 *  @ingroup Utilidades

 */
#define ah_overflow_error()                                             \
  Aleph::ExceptionBuilder<std::overflow_error>(),                       \
    std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

#define ah_overflow_error_if_constexpr(C)                               \
  if constexpr (C)                                                      \
    ah_overflow_error()

#define ah_overflow_error_unless_constexpr(C)                           \
  if constexpr (not (C))                                                \
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
 *  @ingroup Utilidades

 */
#define ah_domain_error_if(C)                                           \
  if (C)                                                                \
    [[unlikely]]Aleph::ExceptionBuilder<std::domain_error>(),           \
      std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

/** @def ah_domain_error_unless(C)
 *  @brief Throws std::domain_error if condition does NOT hold
 *  @param C Condition to evaluate
 *  @throw std::domain_error if C is false
 *  @details Useful for validating that a value is in the valid domain.
 *  @code
 *  ah_domain_error_unless(divisor != 0) << "Division by zero";
 *  @endcode *
 *  @ingroup Utilidades

 */
#define ah_domain_error_unless(C)                                       \
  if (not (C))                                                          \
    [[unlikely]]Aleph::ExceptionBuilder<std::domain_error>(),           \
      std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

/** @def ah_domain_error()
 *  @brief Throws std::domain_error unconditionally
 *  @throw std::domain_error always
 *  @details Throws a domain_error without evaluating any condition.
 *           Use when you need to throw directly.
 *  @code
 *  ah_domain_error() << "Value outside function domain";
 *  @endcode *
 *  @ingroup Utilidades

 */
#define ah_domain_error()                                               \
  Aleph::ExceptionBuilder<std::domain_error>(),                         \
    std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "
#define ah_domain_error_if_constexpr(C)                                 \
  if constexpr (C)                                                      \
    ah_domain_error()
#define ah_domain_error_unless_constexpr(C)                             \
  if constexpr (not (C))                                                \
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
 *  @ingroup Utilidades

 */
#define ah_out_of_range_error_if(C)                                     \
  if (C)                                                                \
    [[unlikely]]Aleph::ExceptionBuilder<std::out_of_range>(),           \
      std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

/** @def ah_out_of_range_error_unless(C)
 *  @brief Throws std::out_of_range if condition does NOT hold
 *  @param C Condition to evaluate
 *  @throw std::out_of_range if C is false
 *  @details Useful for validating valid index access.
 *  @code
 *  ah_out_of_range_error_unless(index < size) << "Invalid index";
 *  @endcode *
 *  @ingroup Utilidades

 */
#define ah_out_of_range_error_unless(C)                                 \
  if (not (C))                                                          \
    [[unlikely]]Aleph::ExceptionBuilder<std::out_of_range>(),           \
      std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

/** @def ah_out_of_range_error()
 *  @brief Throws std::out_of_range unconditionally
 *  @throw std::out_of_range always
 *  @details Throws an out_of_range error without evaluating any condition.
 *           Use when you need to throw directly.
 *  @code
 *  ah_out_of_range_error() << "Index out of bounds";
 *  @endcode *
 *  @ingroup Utilidades

 */
#define ah_out_of_range_error()                                         \
  Aleph::ExceptionBuilder<std::out_of_range>(),                         \
    std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

#define ah_out_of_range_error_if_constexpr(C)                           \
  if constexpr (C)                                                      \
    ah_out_of_range_error()

#define ah_out_of_range_error_unless_constexpr(C)                       \
  if constexpr (not (C))                                                \
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
 *  @ingroup Utilidades

 */
#define ah_invalid_argument_if(C)                                       \
  if (C)                                                                \
    [[unlikely]]Aleph::ExceptionBuilder<std::invalid_argument>(),       \
      std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

/** @def ah_invalid_argument_unless(C)
 *  @brief Throws std::invalid_argument if condition does NOT hold
 *  @param C Condition to evaluate
 *  @throw std::invalid_argument if C is false
 *  @details Useful for validating that arguments meet required conditions.
 *  @code
 *  ah_invalid_argument_unless(size > 0) << "Size must be positive";
 *  @endcode *
 *  @ingroup Utilidades

 */
#define ah_invalid_argument_unless(C)                                   \
  if (not (C))                                                          \
    [[unlikely]]Aleph::ExceptionBuilder<std::invalid_argument>(),       \
      std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

/** @def ah_invalid_argument()
 *  @brief Throws std::invalid_argument unconditionally
 *  @throw std::invalid_argument always
 *  @details Throws an invalid_argument error without evaluating any condition.
 *           Use when you need to throw directly.
 *  @code
 *  ah_invalid_argument() << "Invalid argument provided";
 *  @endcode *
 *  @ingroup Utilidades

 */
#define ah_invalid_argument()                                           \
  Aleph::ExceptionBuilder<std::invalid_argument>(),                     \
    std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

#define ah_invalid_argument_if_constexpr(C)                             \
  if constexpr (C)                                                      \
    ah_invalid_argument()

#define ah_invalid_argument_unless_constexpr(C)                         \
  if constexpr (not (C))                                                \
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
 *  @ingroup Utilidades

 */
#define ah_length_error_if(C)                                           \
  if (C)                                                                \
    [[unlikely]]Aleph::ExceptionBuilder<std::length_error>(),           \
      std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

/** @def ah_length_error_unless(C)
 *  @brief Throws std::length_error if condition does NOT hold
 *  @param C Condition to evaluate
 *  @throw std::length_error if C is false
 *  @details Useful for validating that size is within allowed limits.
 *  @code
 *  ah_length_error_unless(size <= max_size()) << "Size exceeds limit";
 *  @endcode *
 *  @ingroup Utilidades

 */
#define ah_length_error_unless(C)                                       \
  if (not (C))                                                          \
    [[unlikely]]Aleph::ExceptionBuilder<std::length_error>(),           \
      std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

/** @def ah_length_error()
 *  @brief Throws std::length_error unconditionally
 *  @throw std::length_error always
 *  @details Throws a length_error without evaluating any condition.
 *           Use when you need to throw directly.
 *  @code
 *  ah_length_error() << "Container size limit exceeded";
 *  @endcode *
 *  @ingroup Utilidades

 */
#define ah_length_error()                                               \
  Aleph::ExceptionBuilder<std::length_error>(),                         \
    std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

#define ah_length_error_if_constexpr(C)                                 \
  if constexpr (C)                                                      \
    ah_length_error()

#define ah_length_error_unless_constexpr(C)                             \
  if constexpr (not (C))                                                \
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
 *  @ingroup Utilidades

 */
#define ah_fatal_error()                                                \
  Aleph::ExceptionBuilder<std::runtime_error>(),                        \
    std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

# endif
