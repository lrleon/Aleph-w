/* 
  This file is part of Aleph-w library

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018

  Leandro Rabindranath Leon / Alejandro Mujica

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <https://www.gnu.org/licenses/>.
*/

# ifndef AHASSERT_H
# define AHASSERT_H

# include <cstddef>
# include <cstdlib>
# include <cstdio>
# include <cassert>

# ifdef DEBUG

extern void _Assert(const char*, const char*, unsigned);

extern void _Assert(const char*, const char*, unsigned, const char*, ...);

extern void _Warning(const char*, const char*, unsigned);

extern void _Warning(const char*, const char*, unsigned, const char*, ...);

# define COND_assert(cond, f, args...) assert((cond) ? (f) : true)

# define DEBUGINFO(p) (p)

# define PRINT_TRACE(p) (p)


# else /* DEBUG */

# define COND_ASSERT(cond, f, args...)

# define DEBUGINFO(p) 

# define PRINT_TRACE(p) (p)


# endif /* DEBUG */

# endif /* AHASSERT_H */



