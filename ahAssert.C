
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
