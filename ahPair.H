
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
 *  @ingroup Utilities
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


