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

/** @file ahDefs.H
    @brief Core definitions, constants, and utility macros for Aleph-w.

    This header provides fundamental type definitions, utility classes,
    global configuration variables, and diagnostic macros used throughout
    the Aleph-w library.

    @ingroup Utils
 *  @author Leandro Rabindranath León
*/

#ifndef AHDEFS_H
#define AHDEFS_H

#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <cstdlib>
#include <cstdio>
#include <cstdarg>

#include <stdexcept>
#include <utility>
#include <iostream>

/** @brief Tag type for empty/default construction.

    Used to select constructors that perform minimal or no initialization.

    @ingroup Utils
*/
enum EmptyCtor { emptyCtor };

/** @brief Tag type for no-exception construction.

    Used to select constructors that guarantee no exceptions will be thrown.

    @ingroup Utils
*/
enum NoExceptionCtor { no_exception_ctor };

/** @brief Tag type for sentinel node construction.

    Used to select constructors that create sentinel/guard nodes
    in linked data structures.

    @ingroup Utils
*/
enum SentinelCtor { sentinelCtor };

namespace Aleph
{

/** @brief Type alias for IPv4 addresses stored as 32-bit integers.
    @ingroup Utils
*/
typedef uint32_t IPv4_Address;

/** @brief Null/invalid IPv4 address constant.
    @ingroup Utils
*/
extern const IPv4_Address Null_IPv4_Address;

/** @brief Empty placeholder class with no data members.

    Useful as a default template parameter or placeholder type
    when no actual data is needed. Provides equality comparison
    and stream operators for compatibility.

    @ingroup Utils
*/
struct Empty_Class
{
  /** @brief Default constructor. */
  Empty_Class() { /* empty */ }

  /** @brief Empty constructor variant.
      @param Unused tag parameter.
  */
  Empty_Class(EmptyCtor) { /* empty */ }

  /** @brief Sentinel constructor variant.
      @param Unused tag parameter.
  */
  Empty_Class(SentinelCtor) { /* empty */ }

  /** @brief Copy constructor.
      @param Unused source object.
  */
  Empty_Class(const Empty_Class &) = default;

  /** @brief Copy assignment operator.
      @param Unused source object.
      @return Reference to this object.
  */
  Empty_Class & operator=(const Empty_Class &) = default;

  /** @brief Equality comparison (always true).
      @return true, since all Empty_Class instances are equal.
  */
  bool operator == (const Empty_Class &) const { return true; }

  /** @brief Inequality comparison (always false).
      @return false, since all Empty_Class instances are equal.
  */
  bool operator != (const Empty_Class &) const { return false; }

  /** @brief Stream output operator (outputs nothing).
      @param o Output stream.
      @return The output stream unchanged.
  */
  friend std::ostream & operator << (std::ostream & o, const Empty_Class&)
  {
    return o;
  }

  /** @brief Stream input operator (reads nothing).
      @param i Input stream.
      @return The input stream unchanged.
  */
  friend std::istream & operator >> (std::istream & i, const Empty_Class&)
  {
    return i;
  }
};

/** @brief Constant indicating unknown size. */
const int UnknownSize = -1;

/** @brief Constant indicating unknown line number. */
const int UnknownLine = -1;

/** @brief Constant indicating unknown load factor. */
const int UnknownLoad = -1;

/** @brief Flag indicating if the process is running as a daemon.
    @ingroup Utils
*/
extern bool daemonized;

/** @brief Flag enabling verbose output.
    @ingroup Utils
*/
extern bool verbose;

/** @brief Maximum allowed execution time (in seconds).
    @ingroup Utils
*/
extern double max_time;

/** @brief Print an informational message with file and line info.

    @param file Source file name (use __FILE__).
    @param line Line number (use __LINE__).
    @param format Printf-style format string.
    @param ... Format arguments.

    @ingroup Utils
*/
extern void message(const char * file, int line, const char *format, ...);

/** @brief Print an error message with file and line info.

    @param file Source file name (use __FILE__).
    @param line Line number (use __LINE__).
    @param format Printf-style format string.
    @param ... Format arguments.

    @ingroup Utils
*/
extern void error(const char * file, int line, const char *format, ...);

/** @brief Print a message and exit the program.

    @param file Source file name (use __FILE__).
    @param line Line number (use __LINE__).
    @param format Printf-style format string.
    @param ... Format arguments.

    @ingroup Utils
*/
extern void exit(const char * file, int line, const char *format, ...);

/** @brief Print a warning message with file and line info.

    @param file Source file name (use __FILE__).
    @param line Line number (use __LINE__).
    @param format Printf-style format string.
    @param ... Format arguments.

    @ingroup Utils
*/
extern void warning(const char * file, int line, const char *format, ...);

/** @brief Write a message to syslog.

    @param format Printf-style format string.
    @param ap Variable argument list.

    @ingroup Utils
*/
extern void syslog(const char * format, va_list ap);

} // end namespace Aleph

#ifdef MESSAGES

/** @brief Print an informational message (enabled when MESSAGES is defined).
    @param format Printf-style format string.
    @param args Format arguments.
    @ingroup Utils
*/
#define MESSAGE(format, args...) \
        Aleph::message(__FILE__, __LINE__, format , ##args)

/** @brief Print a warning message (enabled when MESSAGES is defined).
    @param format Printf-style format string.
    @param args Format arguments.
    @ingroup Utils
*/
#define WARNING(format, args...) \
        Aleph::warning(__FILE__, __LINE__, format , ##args)

#else

/** @brief Print an informational message (no-op when MESSAGES is not defined). */
#define MESSAGE(format, args...)

/** @brief Print a warning message (no-op when MESSAGES is not defined). */
#define WARNING(format, args...)

#endif

/** @brief Print an error message (always enabled).
    @param format Printf-style format string.
    @param args Format arguments.
    @ingroup Utils
*/
#define AH_ERROR(format, args...) \
        Aleph::error(__FILE__, __LINE__, format , ##args)

/** @brief Print a message and exit the program (always enabled).
    @param format Printf-style format string.
    @param args Format arguments.
    @ingroup Utils
*/
#define EXIT(format, args...) \
        Aleph::exit(__FILE__, __LINE__, format , ##args)

/** @brief Check if a pointer was found (not null).
    @param ptr Pointer to check.
    @return true if ptr is not nullptr.
    @ingroup Utils
*/
#define POINTER_HAS_BEEN_FOUND(ptr) ((ptr) not_eq nullptr)

/** @brief Check if a pointer was not found (is null).
    @param ptr Pointer to check.
    @return true if ptr is nullptr.
    @ingroup Utils
*/
#define POINTER_HAS_NOT_BEEN_FOUND(ptr) ((ptr) == nullptr)

/** @brief Exception specification macro (deprecated in modern C++).
    @param list Exception types that may be thrown.
    @deprecated Exception specifications are deprecated in C++17 and removed in C++20.
    @ingroup Utils
*/
#define Exception_Prototypes(list...) throw(std::exception, ##list)

#endif // AHDEFS_H
