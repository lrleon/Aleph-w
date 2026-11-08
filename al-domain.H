
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
 *  @ingroup Utilities
 *  @author Leandro Rabindranath León
 */

# ifndef AL_DOMAIN_H
# define AL_DOMAIN_H

# include <concepts>
# include <limits>
# include <ostream>
# include <tpl_hash.H>
# include <ahSort.H>
# include <ah-errors.H>

namespace Aleph
{

/** @brief A key of a domain: of an AlDomain, and so an index of a Vector
    or of a Matrix.

    It is what every domain does with its keys: the hash table that keeps
    them makes its buckets with the default constructor, copies the keys
    into them, and compares them with `==`. Aleph_Hash checks that a key
    can be hashed, and says what a key that cannot needs.

    @see AlOrderedDomainKey, for the members that sort the keys
    @ingroup Utilities
*/
template <typename T>
concept AlDomainKey = std::default_initializable<T> and std::copyable<T>
                      and std::equality_comparable<T>;

/** @brief A key of a domain that `<` orders: what AlDomain::keys(), and
    the members that take the keys in their order, need.

    @ingroup Utilities
*/
template <typename T>
concept AlOrderedDomainKey = AlDomainKey<T> and requires (const T & a, const T & b)
{
  { a < b } -> std::convertible_to<bool>;
};

/** @brief A value that `<<` writes to a stream: what the members that
    print keys or numbers need.

    @ingroup Utilities
*/
template <typename T>
concept AlStreamable = requires (std::ostream & s, const T & x)
{
  s << x;
};

/**
 * @brief Generic domain class based on hash set.
 *
 * Stores a set of unique elements with O(1) lookup. Provides
 * sorted iteration via the keys() method.
 *
 * keys() keeps the sorted elements until the domain changes: the members
 * of AlDomain that insert or remove forget them, and keys() sorts them
 * again also when the number of elements changed by other means, as the
 * removal through an iterator.
 *
 * @tparam T Element type (default: int)
 *
 * @ingroup Utilities
 */
  template <typename T = int>
    requires AlDomainKey<T>
class AlDomain : public Aleph::HashSet<T, SetODhash>
{
  mutable bool keys_build = false;   ///< Lazy cache flag
  mutable DynList<T> domain;         ///< Cached sorted keys

public:

  using Base = Aleph::HashSet<T, SetODhash>;
  using Base::Base;
  Derived_Build(AlDomain);

  /**
   * @brief Get sorted list of all elements in the domain.
   *
   * Lazily builds and caches the sorted key list, which a later change
   * of the domain rebuilds.
   *
   * @return Const reference to sorted DynList of elements, valid until
   *         the domain changes
   */
  const DynList<T> & keys() const requires AlOrderedDomainKey<T>
  {
    if (not keys_build or domain.size() != this->size())
      {
        domain = sort(this->Base::keys());
        keys_build = true;
      }
    return domain;
  }

  /// Insert `key`, as HashSet::insert(), and forget the sorted keys.
  [[nodiscard]] T * insert(const T & key)
  {
    keys_build = false;
    return Base::insert(key);
  }

  /// \overload
  [[nodiscard]] T * insert(T && key)
  {
    keys_build = false;
    return Base::insert(std::move(key));
  }

  /// Search or insert `key`, as HashSet::search_or_insert(), and forget
  /// the sorted keys.
  [[nodiscard]] T * search_or_insert(const T & key)
  {
    keys_build = false;
    return Base::search_or_insert(key);
  }

  /// \overload
  [[nodiscard]] T * search_or_insert(T && key)
  {
    keys_build = false;
    return Base::search_or_insert(std::move(key));
  }

  /// Search or insert `key`, as HashSet::contains_or_insert(), and forget
  /// the sorted keys.
  [[nodiscard]] std::pair<T *, bool> contains_or_insert(const T & key)
  {
    keys_build = false;
    return Base::contains_or_insert(key);
  }

  /// \overload
  [[nodiscard]] std::pair<T *, bool> contains_or_insert(T && key)
  {
    keys_build = false;
    return Base::contains_or_insert(std::move(key));
  }

  /// Insert `key`, as HashSet::append(), and forget the sorted keys.
  [[nodiscard]] T * append(const T & key)
  {
    keys_build = false;
    return Base::append(key);
  }

  /// \overload
  [[nodiscard]] T * append(T && key)
  {
    keys_build = false;
    return Base::append(std::move(key));
  }

  /// Insert the keys of `l`, as HashSet::add(), and forget the sorted keys.
  void add(const DynList<T> & l)
  {
    keys_build = false;
    Base::add(l);
  }

  /// Remove `key`, as HashSet::remove(), and forget the sorted keys.
  void remove(const T & key)
  {
    keys_build = false;
    Base::remove(key);
  }

  /// Remove the key that `key` points to, as HashSet::remove_ptr(), and
  /// forget the sorted keys.
  void remove_ptr(T * key)
  {
    keys_build = false;
    Base::remove_ptr(key);
  }

  /// Remove every key, as HashSet::empty(), and forget the sorted keys.
  void empty()
  {
    keys_build = false;
    Base::empty();
  }

  /// Remove every key, as HashSet::clear(), and forget the sorted keys.
  void clear() { empty(); }

  /// Swap the keys, and the sorted keys, with `other`.
  void swap(AlDomain & other) noexcept
  {
    Base::swap(other);
    std::swap(keys_build, other.keys_build);
    domain.swap(other.domain);
  }

  /**
   * @brief Convert domain to sorted list.
   * @return Sorted DynList of all elements
   */
  DynList<T> to_list() const requires AlOrderedDomainKey<T> { return this->keys(); }

  /**
   * @brief Convert domain to string representation.
   * @return Space-separated string of sorted elements
   */
  std::string to_str() const requires AlOrderedDomainKey<T> and AlStreamable<T>
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
  template <typename T>
    requires AlOrderedDomainKey<T> and AlStreamable<T>
  inline std::ostream & operator << (std::ostream & s, const AlDomain<T> & dom)
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
   * @ingroup Utilities
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
     * @throw std::domain_error if step is not positive
     *
     * @par Example
     * @code
     * IntRange r(0, 10, 2);  // {0, 2, 4, 6, 8, 10}
     * @endcode
     */
    IntRange(const int start, const int end, const int step = 1)
    {
      ah_domain_error_if(step <= 0) << "the step must be positive";

      // long long, so that i + step does not overflow where long has 32
      // bits, as on Windows, when end is near the maximum of int
      for (long long i = start; i <= end; i += step)
        (void)insert(static_cast<int>(i));
    }

    /**
     * @brief Construct range [0, n-1].
     *
     * @param n Number of elements (0 to n-1)
     *
     * @throw std::domain_error if n - 1 does not fit in an int, as for a
     *        negative int converted to `size_t`
     *
     * @par Example
     * @code
     * IntRange r(5);  // {0, 1, 2, 3, 4}
     * @endcode
     */
    IntRange(const size_t n)
    {
      ah_domain_error_if(n > static_cast<size_t>(std::numeric_limits<int>::max()) + 1)
        << "IntRange(n): n - 1 is above the maximum of int";

      for (size_t i = 0; i < n; ++i)
        (void)insert(static_cast<int>(i));
    }

  };

} // end namespace Aleph

# endif // AL_DOMAIN_H
