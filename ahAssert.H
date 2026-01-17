
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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


/** @file ahAssert.H
    @brief Debug assertion and warning utilities.

    Provides debug-only assertion and warning macros that are compiled
    out in release builds. When DEBUG is defined, assertions print
    diagnostic information and abort the program. Warnings print
    diagnostic information without aborting.

    @note These macros are only active when the DEBUG preprocessor
          symbol is defined.

    @ingroup Utilities
 *  @author Leandro Rabindranath León
*/

#ifndef AHASSERT_H
#define AHASSERT_H

#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <cassert>

#ifdef DEBUG

/** @brief Print assertion failure message and abort.

    Called when an assertion fails. Prints the failed expression,
    file name, and line number, then aborts the program.

    @param exp The expression that failed (as a string).
    @param fileName The source file name.
    @param lineNumber The line number in the source file.
*/
extern void _assert(const char* exp, const char* fileName, unsigned lineNumber);

/** @brief Print assertion failure message with custom format and abort.

    Called when an assertion fails. Prints the failed expression,
    file name, line number, and a custom formatted message, then
    aborts the program.

    @param exp The expression that failed (as a string).
    @param fileName The source file name.
    @param lineNumber The line number in the source file.
    @param format Printf-style format string.
    @param ... Arguments for the format string.
*/
extern void _assert(const char* exp, const char* fileName, unsigned lineNumber,
                    const char* format, ...);

/** @brief Print warning message without aborting.

    Called to issue a warning. Prints the condition, file name, and
    line number, but does not abort the program.

    @param exp The condition (as a string).
    @param fileName The source file name.
    @param lineNumber The line number in the source file.
*/
extern void _Warning(const char* exp, const char* fileName, unsigned lineNumber);

/** @brief Print warning message with custom format without aborting.

    Called to issue a warning. Prints the condition, file name, line
    number, and a custom formatted message, but does not abort the
    program.

    @param exp The condition (as a string).
    @param fileName The source file name.
    @param lineNumber The line number in the source file.
    @param format Printf-style format string.
    @param ... Arguments for the format string.
*/
extern void _Warning(const char* exp, const char* fileName, unsigned lineNumber,
                     const char* format, ...);

/** @brief Conditional assertion macro.

    Evaluates the assertion function `f` only if the condition `cond`
    is true. Otherwise, evaluates to true (no assertion failure).

    @param cond The condition to test.
    @param f The assertion function to call if cond is true.
    @param args Additional arguments for the assertion function.
*/
#define COND_assert(cond, f, args...) assert((cond) ? (f) : true)

/** @brief Debug information macro.

    Executes the statement `p` only in debug builds.

    @param p The statement to execute.
*/
#define DEBUGINFO(p) (p)

/** @brief Print trace macro.

    Executes the statement `p` for tracing purposes.
    Active in both debug and release builds.

    @param p The statement to execute.
*/
#define PRINT_TRACE(p) (p)


#else /* DEBUG not defined */

/** @brief Conditional assertion macro (no-op in release). */
#define COND_ASSERT(cond, f, args...)

/** @brief Debug information macro (no-op in release). */
#define DEBUGINFO(p)

/** @brief Print trace macro (active in release builds).
    @param p The statement to execute.
*/
#define PRINT_TRACE(p) (p)


#endif /* DEBUG */

#endif /* AHASSERT_H */
