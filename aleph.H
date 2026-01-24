
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


/** @file aleph.H
 *  @brief Core header for the Aleph-w library.
 *
 *  This is the main include file for the Aleph-w library. It provides
 *  fundamental includes for definitions, utilities, and assertions that
 *  are used throughout the library.
 *
 *  @ingroup Utilities
 *  @author Leandro Rabindranath León
 */

# ifndef ALEPH_H
# define ALEPH_H

# include <stdexcept>
# include <new>
# include <ahDefs.H>
# include <ahUtils.H>
# include <ahAssert.H>

template <class Itor> inline
void put_itor_at_the_end(Itor & it) noexcept
{
  it.reset_last();
  if (it.has_curr()) // Test for non empty container
    it.next_ne();
}

# endif /* ALEPH_H */

