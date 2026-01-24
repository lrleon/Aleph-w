
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


/** @file ah-args-ctor.H
 *  @brief Variadic constructor macros for containers.
 *
 *  This file provides macros for generating variadic constructors that allow
 *  containers to be initialized with multiple elements at construction time.
 *
 *  @ingroup Utilities
 *  @author Leandro Rabindranath León
 */

#ifndef AH_ARGS_CTOR_H
#define AH_ARGS_CTOR_H

# define Args_Ctor(Name, Type)                  \
  template <typename ... Args>                  \
  Name(const Type & item, Args & ... args)      \
    : Name(args...)                             \
  {                                             \
    this->append(item);                         \
  }                                             \
                                                \
  template <typename ... Args>                  \
  Name(Type && item, Args & ... args)           \
    : Name(args...)                             \
  {                                             \
    this->append(item);                         \
  }

#endif
