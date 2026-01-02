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

/** @file ahUtils.H
 *  @brief General utility functions and helpers.
 *
 *  This file provides miscellaneous utility functions including bit manipulation,
 *  memory utilities, file operations, string formatting, type name demangling,
 *  and various mathematical helpers used throughout Aleph-w.
 *
 *  @ingroup Utils
 */

# ifndef AHUTILS_H
# define AHUTILS_H

# include <sys/stat.h>
# include <cstdlib>
# include <cassert>
# include <cmath>
# include <cstdint>
# include <cstring>
# include <memory>
# include <limits>
# include <cxxabi.h>
# include <string>
# include <sstream>
# include <iostream>
# include <fstream>
# include <typeinfo>
# include <algorithm>

# include <ahFunction.H>

// build a string with the file name and line number
/** String literal with the current source position (`file:line`).

    Intended for error messages and diagnostics.

    @ingroup utils
*/
# define POSITION_TRACE (std::string(__FILE__) + ":" + std::to_string(__LINE__))

namespace Aleph
{
  /** Return a pointer to the median value among three elements.

      A pointer is returned to avoid copies and to allow pointer
      equality comparisons.

      @ingroup fcts
   */
  template <class T, class Compare = Aleph::less<T>>
  inline const T * median(const T & a, const T & b, const T & c, Compare cmp = Compare())
  {
    if (cmp(a, b))
      {
        if (cmp(b, c)) return &b; // a < b < c
        if (cmp(a, c)) return &c; // a < c <= b
        return &a; // c <= a < b
      }
    if (cmp(a, c)) return &a; // b <= a < c
    if (cmp(b, c)) return &c; // b < c <= a
    return &b; // c <= b <= a
  }


  /** Return `true` if `n` is even.

      @ingroup fcts
   */
  inline bool is_even(const long n)
  {
    return (n % 2) == 0;
  }

  /** Return `true` if `n` is odd.

      @ingroup fcts
   */
  inline bool is_odd(const long n)
  {
    return not is_even(n);
  }

  /** Convert a 4-bit nibble stored in an `int` to its hex character.

      Valid values are 0..15; the result is in `0..9A..F`.

      @ingroup fcts
   */
  inline char nibble_to_char(const int i)
  {
    assert(i >= 0 and i <= 15);

    const char ret = i < 10 ? i + '0' : i - 10 + 'A';

    assert((ret >= '0' and ret <= '9') or (ret >= 'A' and ret <= 'F'));

    return ret;
  }

  /** Convert a hex character in `0..9A..F` to its 4-bit nibble value.

      @ingroup fcts
   */
  inline int char_to_nibble(const char c)
  {
    assert((c >= '0' and c <= '9') or (c >= 'A' and c <= 'F'));

    const int ret = c < 'A' ? c - '0' : c - 'A' + 10;

    assert(ret >= 0 and ret <= 15);

    return ret;
  }

  /** Define a derived iterator type from an existing iterator base.

      This macro is used throughout the library to create a thin derived
      iterator type that forwards constructors and copy assignment.

      @param container_name Container type name.
      @param base_it_name   Base iterator type name.
      @param it_name        New iterator type name.

      @ingroup utils
  */
# define DERIVATE_ITERATOR(container_name,  base_it_name, it_name) \
  struct it_name : public base_it_name                             \
  {                                                                \
    it_name() { /* empty */ }                                      \
		                                                               \
    it_name(container_name & c) : base_it_name(c)                  \
      {                                                            \
        /* empty */                                                \
      }                                                            \
                                                                   \
    it_name(const it_name & it) : base_it_name(it)                 \
      {                                                            \
        /* empty */                                                \
      }                                                            \
                                                                   \
    it_name & operator = (const it_name & it)                      \
    {                                                              \
      return base_it_name::operator = (it);                        \
    }                                                              \
  };


  /** Resize the process stack to `new_size`.

      @param[in] new_size Requested stack size in bytes.
      @return `true` on success; `false` otherwise.

      @ingroup fcts
   */
  extern bool resize_process_stack(size_t new_size);

  /** Map a binary heap index to the index of its parent.

      For 1-based heaps, `u_index(i)` returns `i/2`.

      @param[in] i Node index.
      @return Parent index.

      @ingroup utils
  */
  inline size_t u_index(const size_t & i)
  {
    return i >> 1; // divide i entre 2
  }

  /** Map a binary heap index to the index of its left child.

      For 1-based heaps, `l_index(i)` returns `2*i`.

      @param[in] i Node index.
      @return Left child index.

      @ingroup utils
  */
  inline size_t l_index(const size_t i)
  {
    return i << 1;
  }

  /**

      Taken from http://stackoverflow.com/questions/3638431/determine-if-an-int-is-a-power-of-2-or-not-in-a-single-line

      @ingroup utils
  */
  inline bool is_power_of_2(unsigned long x)
  {
    return x && ! (x & (x - 1UL));
  }

  /**
      In x is not exact power of 2, it returns the next power of 2

      @ingroup utils
  */
  inline unsigned long next_power_of_2(unsigned long x)
  {
    if (is_power_of_2(x))
      return x;

    unsigned long ret = 1;
    while (ret < x)
      ret <<= 1;

    return ret;
  }

  /** Given a linker symbol name generated by a c++ compiler, this
    functions decodes it into a user level name.

    @ingroup fcts
   */
  inline std::string demangle(const char *name)
  {
    int status = -4; // some arbitrary value to eliminate the compiler warning
    // enable c++11 by passing the flag -std=c++11 to g++
    std::unique_ptr<char, void(*)(void *)> res
        {abi::__cxa_demangle(name, nullptr, nullptr, &status), std::free};
    return (status == 0) ? res.get() : name;
  }

  /** Given a pointer, it returns the class name

    @ingroup fcts
   */
# define CLASSNAME_TO_STRING(class_ptr) ::Aleph::demangle(typeid(*class_ptr).name())

  /** Display message and abort program execution.

     @ingroup fcts
   */
  inline void error_msg(const std::string & msg)
  {
    std::cerr << msg << '\n';
    abort();
  }

  /** Return `true` if it exists a file of `name`

     @ingroup fcts
   */
  inline bool exists_file(const std::string & name)
  {
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
  }

  /** Return a string with R specification of a vector with `name` and
     data stored in container `c`

     @ingroup fcts
   */
  template <class C>
  inline std::string Rvector(const std::string & name, const C & c)
  {
    std::ostringstream s;
    s << name << " <- c(";
    auto last_ptr = &c.get_last();
    for (auto it = c.get_it(); it.has_curr(); it.next())
      {
        auto & v = it.get_curr();
        s << v;
        if (&v != last_ptr)
          s << ", ";
      }
    s << ")";

    return s.str();
  }

  /** Return a string with an R assignment for a container interpreted as
      `[name, value1, value2, ...]`.

      The first element of `c` is used as the R variable name, and the
      remaining elements are used as the vector data.

      @ingroup fcts
   */
  template <class C>
  inline std::string Rvector(const C & c)
  {
    std::ostringstream s;
    s << c.get_first() << " <- c(";
    auto last_ptr = &c.get_last();
    for (auto it = c.get_it(1); it.has_curr(); it.next())
      {
        auto & v = it.get_curr();
        s << v;
        if (&v != last_ptr)
          s << ", ";
      }
    s << ")";

    return s.str();
  }

  /** Basic linear interpolation

     This function receives a pair of points in a plane plus a x point
     and computes the value of f(x) through linear interpolation

     @param[in] x1 minimum in x
     @param[in] x2 maximum in x
     @param[in] y1 value of f(x1)
     @param[in] y2 value of f(x2)
     @param[in] x for which f(x) should be computed
     @note Requires `x2 > x1` and `x` in [`x1`, `x2`].

    @ingroup fcts
   */
  inline
  double interpolate(const double x1, const double x2,
                     const double y1, const double y2, const double x)
  {
    assert(x2 > x1 and x <= x2 and x >= x1);
    return y1 + (y2 - y1) * (x - x1) / (x2 - x1);
  }

  /** Basic linear extrapolation

     This function receives a pair of points in a plane plus a x point
     less than `x1` and `x2` and computes the value of f(x)

     @param[in] x1 minimum in x
     @param[in] x2 maximum in x
     @param[in] y1 value of f(x1)
     @param[in] y2 value of f(x2)
     @param[in] x for which f(x) should be computed
     @note Requires `x2 > x1` and `x < x1`.

     @ingroup fcts
   */
  inline
  double extrapolate_left(const double x1, const double x2,
                          const double y1, const double y2, const double x)
  {
    assert(x2 > x1 and x < x1);
    return y1 - (y2 - y1) * (x1 - x) / (x2 - x1);
  }

  /** Return x^2
     @ingroup fcts
   */
  inline double pow2(const double x) { return x * x; }

  /** Return x^3
     @ingroup fcts
   */
  inline double pow3(const double x) { return x * pow2(x); }

  /** Basic linear extrapolation

     This function receives a pair of points in a plane plus a x point
     less than `x1` and `x2` and computes the value of f(x)

     @param[in] x1 minimum in x
     @param[in] x2 maximum in x
     @param[in] y1 value of f(x1)
     @param[in] y2 value of f(x2)
     @param[in] x for which f(x) should be computed
     @note Requires `x2 > x1` and `x > x2`.

     @ingroup fcts
   */
  inline
  double extrapolate_right(const double x1, const double x2,
                           const double y1, const double y2, const double x)
  {
    assert(x2 > x1 and x > x2);
    return y2 + (y2 - y1) * (x - x2) / (x2 - x1);
  }

  /** Return the next representable floating-point value to `val`

     @ingroup fcts
   */
  inline double next_value(const double val)
  {
    return nextafter(val, std::numeric_limits<double>::max());
  }

  /** Return the next representable floating-point value of `val` towards the
      smallest positive normal number.

      @ingroup fcts
   */
  inline double prev_value(const double val)
  {
    return nextafter(val, std::numeric_limits<double>::min());
  }

  /** Return `true` if `v1` is within absolute distance `e` of `v2`.

      @ingroup fcts
   */
  [[nodiscard]] constexpr bool are_near(const double v1, const double v2, const double e) noexcept { return std::fabs(v1 - v2) <= e; }

  /** Generate and execute a R script from a string (containing the script)

      @param[in] scr string containing the script
      @param file_name file name of temporary file where the script will be saved


     @ingroup fcts
   */
  inline
  void execute_R_script(const std::string & scr, const std::string & file_name = "tmp.R")
  {
    std::ofstream tmp(file_name);
    tmp << "#!/usr/bin/Rscript" << '\n'
        << '\n'
        << "X11()" << '\n'
        << scr << '\n'
        << "message(\"Press Return To Continue\")" << '\n'
        << "invisible(readLines(\"stdin\", n=1))" << '\n';
    const std::string cmd = "Rscript " + file_name;
    system(cmd.c_str());
  }

  /** Return `true` if a floating-point number is normal or zero.

      This excludes NaN, infinities, and subnormal.

      @param[in] n Value to classify.
      @return `true` if `n` is a finite number classified as `FP_NORMAL`
              or `FP_ZERO`.

      @ingroup fcts
  */

  inline bool is_normal_number(const double n)
  {
    if (std::numeric_limits<double>::is_iec559 &&
        sizeof(double) == sizeof(std::uint64_t))
      {
        // IEEE-754 bit inspection avoids fast-math assuming finiteness.
        std::uint64_t bits = 0;
        std::memcpy(&bits, &n, sizeof(bits));

        const std::uint64_t exp_mask = 0x7ff0000000000000ULL;
        const std::uint64_t frac_mask = 0x000fffffffffffffULL;

        const std::uint64_t exp = bits & exp_mask;
        const std::uint64_t frac = bits & frac_mask;

        if (exp == exp_mask)
          return false;

        if (exp == 0)
          return frac == 0;

        return true;
      }

    if (not std::isfinite(n))
      return false;

    const int status = std::fpclassify(n);
    return (status == FP_NORMAL or status == FP_ZERO);
  }
}


# endif // AHUTILS_H
