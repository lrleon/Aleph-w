
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

# ifndef AH_ERRORS_H
# define AH_ERRORS_H

# include <stdexcept>
# include <new>
# include <sstream>

namespace Aleph
{

  template <class E>
  struct ExceptionBuilder
  {
    int operator, (const std::ostream &s)
    {
      throw E(static_cast<const std::ostringstream &>(s).str());
      return 0;
    }
  };
  
}


#define ah_range_error_unless(C)                                        \
  if (__builtin_expect(!(C), 0))                                        \
    Aleph::ExceptionBuilder<std::range_error>(),                        \
      std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

#define ah_runtime_error_unless(C)                                      \
  if (__builtin_expect(!(C), 0))                                        \
    Aleph::ExceptionBuilder<std::runtime_error>(),                      \
      std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

#define ah_logic_error_unless(C)                                        \
  if (__builtin_expect(!(C), 0))                                        \
    Aleph::ExceptionBuilder<std::logic_error>(),                        \
      std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

#define ah_domain_error_unless(C)                                       \
  if (__builtin_expect(!(C), 0))                                        \
    Aleph::ExceptionBuilder<std::domain_error>(),                       \
      std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

#define ah_range_error_if(C)                                            \
  if (__builtin_expect((C), 0))                                         \
    Aleph::ExceptionBuilder<std::range_error>(),                        \
      std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

#define ah_runtime_error_if(C)                                          \
  if (__builtin_expect((C), 0))                                         \
    Aleph::ExceptionBuilder<std::runtime_error>(),                      \
      std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

#define ah_logic_error_if(C)                                            \
  if (__builtin_expect(!(C), 0))                                        \
    Aleph::ExceptionBuilder<std::logic_error>(),                        \
      std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "

#define ah_domain_error_if(C)                                           \
  if (__builtin_expect((C), 0))                                         \
    Aleph::ExceptionBuilder<std::domain_error>(),                       \
      std::stringstream() << "(" << __FILE__ << ":" << __LINE__ << ") | "


# endif
