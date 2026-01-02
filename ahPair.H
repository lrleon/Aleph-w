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
/*
 *
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Hewlett-Packard Company makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 *
 * Copyright (c) 1996,1997
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */

/** @file ahPair.H
 *  @brief Pair utilities and make_pair function.
 *
 *  Provides pair-related utilities compatible with std::pair.
 *  Based on SGI STL implementation with Aleph-w extensions.
 *
 *  @ingroup Utilidades
 *  @author Leandro Rabindranath León
 */

# ifndef AHPAIR_H
# define AHPAIR_H 1

# include <utility>

namespace Aleph
{
  /** @brief Alias to `std::pair` kept for backwards compatibility.
   *
   * Aleph historically provided its own `pair` type prior to C++11. Modern
   * compilers already ship a standards-compliant implementation in
   * `<utility>`, so this header simply re-exports that type within the
   * Aleph namespace to avoid code churn in legacy headers.
   *
   * All constructors, assignment operators and comparison operators behave
   * exactly like the ones provided by `std::pair`.
   *
   * @tparam First First element type.
   * @tparam Second Second element type.
   */
  template<typename First, typename Second>
  using pair = std::pair<First, Second>;

  /// Re-export `std::make_pair` so existing code can refer to Aleph::std::make_pair.
  using std::make_pair;
} // namespace Aleph

# endif /* AHPAIR_H */


