
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
 * @file trace.H
 * @brief Simple RAII-based function tracing utility
 * 
 * This file provides a lightweight mechanism for tracing function
 * entry and exit points during debugging. It uses RAII to automatically
 * print entry/exit messages without explicit logging calls.
 * 
 * ## Features
 * 
 * - **Automatic scope tracking**: Entry logged on construction, exit on destruction
 * - **File and line information**: Shows source location of trace point
 * - **Named traces**: Custom names for identifying trace points
 * - **Zero configuration**: Just include and use the macro
 * 
 * ## Usage Example
 * 
 * ```cpp
 * void myFunction() {
 *     TRACE("myFunction");  // Prints: ****ENTER myFunction in file.cpp:123
 *     
 *     // ... function body ...
 *     
 * }  // Prints: ****LEAVE myFunction (on scope exit)
 * ```
 * 
 * ## Output Format
 * 
 * ```
 * ****ENTER <name> in <file>:<line>
 * ****LEAVE <name>
 * ```
 * 
 * ## Implementation Notes
 * 
 * The TRACE macro creates a unique local variable using __LINE__ to avoid
 * naming conflicts when multiple TRACE calls appear in the same scope.
 * 
 * @note Output goes to stdout via printf.
 * @note For production code, consider using a proper logging framework.
 * 
 * @author Leandro Rabindranath León *
 *  @ingroup Utilities

 */

# ifndef TRACE_H
# define TRACE_H

# include <stdio.h>

/**
 * @class Trace
 * @brief RAII helper for function entry/exit tracing
 * 
 * Prints an "ENTER" message on construction and "LEAVE" on destruction,
 * providing automatic function tracing without explicit logging calls. *
 *  @ingroup Utilities

 */
class Trace
{
  char * file;
  int line;

  char * name;

public:

  Trace(char * _file, int _line, char * _name) :
    file(_file), line(_line), name(_name)
  {
    printf("****ENTER %s in %s:%d\n", name, file, line);
  }

  /// Destructor - prints LEAVE message
  ~Trace()
  {
    printf("****LEAVE %s\n", name);
  }
};

/**
 * @def TRACE(name)
 * @brief Macro to create a Trace object with current file/line info
 * @param name String identifier for this trace point
 * 
 * Creates a uniquely-named Trace object that will print entry/exit
 * messages for the current scope. *
 *  @ingroup Utilities

 */
# define TRACE(name) Trace __trace__##__LINE__(__FILE__, __LINE__, name)

# endif // TRACE_H
