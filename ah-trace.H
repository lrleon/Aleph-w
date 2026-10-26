
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


/** @file ah-trace.H
 *  @brief Conditional tracing macros for debugging.
 *
 *  Provides ah_trace_unless and ah_trace_if macros that output
 *  file:line information when conditions are met. Useful for
 *  debugging without full assertion overhead.
 *
 *  Each conditional macro is a `for` of one iteration, which takes no
 *  `else`: an `else` written after an invocation belongs to the `if` around
 *  it, and one with no `if` around it does not compile.
 *
 *  @ingroup Utilities
 *  @author Leandro Rabindranath León
 */

# ifndef AH_TRACE_H
# define AH_TRACE_H

# include <iostream>


#define ah_trace_unless(out, C)                                         \
  for (bool ah_trace_once_ = not (C);                                   \
       __builtin_expect(ah_trace_once_, 0); ah_trace_once_ = false)     \
    out << __FILE__ << ":" << __LINE__ << " | "

#define ah_trace_if(out, C)                                             \
  for (bool ah_trace_once_ = static_cast<bool>(C);                      \
       __builtin_expect(ah_trace_once_, 0); ah_trace_once_ = false)     \
    out << __FILE__ << ":" << __LINE__ << " | "

#define ah_msg(out)                             \
  out << ""

#define ah_msg_if(out, C)                                               \
  for (bool ah_msg_once_ = static_cast<bool>(C); ah_msg_once_;          \
       ah_msg_once_ = false)                                            \
    [[unlikely]](out) << ""

#define ah_msg_unless(out, C)                                           \
  for (bool ah_msg_once_ = not (C); ah_msg_once_; ah_msg_once_ = false) \
    [[unlikely]](out) << ""

# endif
