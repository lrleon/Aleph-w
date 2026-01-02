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
 * @author Leandro Rabindranath León
 */

# ifndef TRACE_H
# define TRACE_H

# include <stdio.h>

/**
 * @class Trace
 * @brief RAII helper for function entry/exit tracing
 * 
 * Prints an "ENTER" message on construction and "LEAVE" on destruction,
 * providing automatic function tracing without explicit logging calls.
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
 * messages for the current scope.
 */
# define TRACE(name) Trace __trace__##__LINE__(__FILE__, __LINE__, name)

# endif // TRACE_H
