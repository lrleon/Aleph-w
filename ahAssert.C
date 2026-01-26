
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


#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ahAssert.H>
#include <ahDefs.H>

/*
  These debug functions are only defined in ahAssert.H if DEBUG flag is
  defined. However, since ahAssert.o is destined to be a library and that
  is not frequently compiled, we do not use the preprocessor for
  conditioning the functions definitions. Because they are in library,
  they are only used if they are inquired. So, we minimize misuse
  e.g. if we forget to define DEBUG, we get a "undefined reference"
  message while linking symbol using DEBUG.
*/

void _assert(const char *exp, const char* fileName, unsigned lineNumber)
{
  MESSAGE("Assertion %s in line %u file %s violated\n",
          exp, lineNumber, fileName);
  abort();
}

void _assert(const char *exp,
             const char*  fileName,
             unsigned int lineNumber,
             const char*  format,
             ...)
{
  MESSAGE("Assertion %s in line %u file %s violated\n",
          exp, lineNumber, fileName);

  va_list ap;
  va_start(ap, format);
  vprintf(format, ap);
  va_end(ap);

  abort();
}

void _Warning(const char *exp, const char* fileName, unsigned lineNumber)
{
  MESSAGE("Warning: condition %s in line %u file %s has been violated\n"
          "(Programmer decided not to abort on this violation)\n",
          exp, lineNumber, fileName);
}

void _Warning(const char *exp,
              const char*  fileName,
              unsigned int lineNumber,
              const char*  format,
              ...)
{
  MESSAGE("Warning: condition %s in line %u file %s has been violated\n"
          "(Programmer decided not to abort on this violation)\n",
          exp, lineNumber, fileName);

  va_list ap;
  va_start(ap, format);
  vprintf(format, ap);
  va_end(ap);
  // Note: _Warning does NOT abort, unlike _assert
}
