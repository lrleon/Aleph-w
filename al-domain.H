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

/** @file al-domain.H
 *  @brief Integer domain classes for sparse data structures.
 *
 *  This file provides domain classes representing sets of values, primarily
 *  used as index domains for sparse matrices and vectors.
 *
 *  ## Features
 *
 *  - Hash-based set for O(1) membership testing
 *  - Sorted key iteration
 *  - Range-based domain construction
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  // Create domain [0, 9]
 *  IntRange domain(0, 9);
 *
 *  // Create domain {0, 1, ..., n-1}
 *  IntRange indices(100);
 *
 *  // Create domain with step: {0, 2, 4, 6, 8, 10}
 *  IntRange even(0, 10, 2);
 *  ```
 *
 *  @see tpl_dynMat.H Sparse matrix using domains
 *
 *  @ingroup Utilidades
 *  @author Leandro Rabindranath León
 */

# ifndef AL_DOMAIN_H
# define AL_DOMAIN_H

# include <tpl_hash.H>
# include <ah-errors.H>

namespace Aleph
{

/**
 * @brief Generic domain class based on hash set.
 *
 * Stores a set of unique elements with O(1) lookup. Provides
 * sorted iteration via the keys() method.
 *
 * @tparam T Element type (default: int)
 *
 * @ingroup Utilidades
 */
  template <typename T = int>
class AlDomain : public Aleph::HashSet<T, SetODhash>
{
  mutable bool keys_build = false;   ///< Lazy cache flag
  mutable DynList<T> domain;         ///< Cached sorted keys

public:

  using Base = Aleph::HashSet<T, SetODhash>;
  using Base::Base;

  /**
   * @brief Get sorted list of all elements in the domain.
   *
   * Lazily builds and caches the sorted key list.
   *
   * @return Const reference to sorted DynList of elements
   */
  const DynList<T> & keys() const
  {
    if (not keys_build)
      {
        domain = sort(this->Base::keys());
        keys_build = true;
      }
    return domain;
  }

  /**
   * @brief Convert domain to sorted list.
   * @return Sorted DynList of all elements
   */
  DynList<T> to_list() const { return this->keys(); }

  /**
   * @brief Convert domain to string representation.
   * @return Space-separated string of sorted elements
   */
  std::string to_str() const
  {
    return sort(to_list()).template foldl<std::string>
      ("", [] (const std::string & s, const T & item)
       {
         return s + " " + Aleph::to_str(item);
       });
  }
  };

  /**
   * @brief Output stream operator for AlDomain.
   *
   * @tparam T Element type
   * @param s Output stream
   * @param dom Domain to output
   * @return Reference to output stream
   */
  template <typename T> inline
  std::ostream & operator << (std::ostream & s, const AlDomain<T> & dom)
  {
    return s << dom.to_str();
  }

  /**
   * @brief Integer range domain [start, end] with optional step.
   *
   * Specialized domain for integer ranges, commonly used as indices
   * for sparse matrices and vectors.
   *
   * ## Construction Examples
   *
   * | Constructor | Result |
   * |-------------|--------|
   * | IntRange(5) | {0, 1, 2, 3, 4} |
   * | IntRange(0, 4) | {0, 1, 2, 3, 4} |
   * | IntRange(0, 10, 2) | {0, 2, 4, 6, 8, 10} |
   *
   * @ingroup Utilidades
   */
  struct IntRange : public AlDomain<int>
  {
    /// Deleted default constructor (range required)
    IntRange() = delete;

    /**
     * @brief Construct range [start, end] with step.
     *
     * @param start First value in range
     * @param end Last value in range (inclusive)
     * @param step Increment between values (default 1)
     *
     * @throw std::domain_error if step is negative
     *
     * @par Example
     * @code
     * IntRange r(0, 10, 2);  // {0, 2, 4, 6, 8, 10}
     * @endcode
     */
    IntRange(const int start, const int end, const int step = 1)
    {
      ah_domain_error_if(step < 0) << "negative step";

      for (long i = start; i <= end; i += step)
        (void)insert(i);
    }

    /**
     * @brief Construct range [0, n-1].
     *
     * @param n Number of elements (0 to n-1)
     *
     * @par Example
     * @code
     * IntRange r(5);  // {0, 1, 2, 3, 4}
     * @endcode
     */
    IntRange(const size_t n)
    {
      for (size_t i = 0; i < n; ++i)
        (void)insert(i);
    }

  };

} // end namespace Aleph

# endif // AL_DOMAIN_H
