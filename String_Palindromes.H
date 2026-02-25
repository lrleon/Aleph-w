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


/** @file String_Palindromes.H
 *  @brief Palindrome algorithms over strings.
 *
 *  Includes Manacher's algorithm in O(n) for the longest palindromic substring and
 *  radius arrays for odd/even centers.
 *
 *  @example manacher_example.cc
 *
 *  @ingroup Algorithms
 *  @author Leandro Rabindranath Leon
 */

# ifndef STRING_PALINDROMES_H
# define STRING_PALINDROMES_H

# include <algorithm>
# include <string>
# include <string_view>

# include <tpl_array.H>

namespace Aleph
{
  /** @brief Result object for Manacher's algorithm.
   *
   *  Radius conventions:
   *  - `odd_radius[i]`: radius around center `i` (length = `2*r-1`).
   *  - `even_radius[i]`: radius around center between `i-1` and `i`
   *    (length = `2*r`).
   */
  struct Manacher_Result
  {
    Array<size_t> odd_radius;
    Array<size_t> even_radius;

    size_t longest_begin = 0;
    size_t longest_length = 0;
    std::string longest_palindrome;
  };


  /** @brief Compute palindromic radii and the longest palindrome with Manacher.
   *
   *  @param[in] text Input text.
   *  @return Full Manacher result.
   *
   *  @par Complexity
   *  O(n) time and O(n) space.
   */
  inline Manacher_Result manacher(const std::string_view text)
  {
    Manacher_Result result;
    const size_t n = text.size();

    if (n == 0)
      {
        result.longest_palindrome = "";
        return result;
      }

    result.odd_radius = Array<size_t>::create(n);
    result.even_radius = Array<size_t>::create(n);

    for (size_t i = 0; i < n; ++i)
      {
        result.odd_radius[i] = 0;
        result.even_radius[i] = 0;
      }

    long long l = 0;
    long long r = -1;

    for (size_t i = 0; i < n; ++i)
      {
        size_t k = 1;
        if (static_cast<long long>(i) <= r)
          {
            const size_t mirror = l + r - static_cast<long long>(i);
            k = std::min(result.odd_radius[mirror],
                         static_cast<size_t>(r - static_cast<long long>(i) + 1));
          }

        while (static_cast<long long>(i) - static_cast<long long>(k) >= 0
               and i + k < n
               and text[i - k] == text[i + k])
          ++k;

        result.odd_radius[i] = k;

        const long long new_l = static_cast<long long>(i) - static_cast<long long>(k) + 1;
        const long long new_r = static_cast<long long>(i) + static_cast<long long>(k) - 1;
        if (new_r > r)
          {
            l = new_l;
            r = new_r;
          }
      }

    l = 0;
    r = -1;

    for (size_t i = 0; i < n; ++i)
      {
        size_t k = 0;
        if (static_cast<long long>(i) <= r)
          {
            const auto mirror = static_cast<size_t>(l + r - static_cast<long long>(i) + 1);
            k = std::min(result.even_radius[mirror],
                         static_cast<size_t>(r - static_cast<long long>(i) + 1));
          }

        while (static_cast<long long>(i) - static_cast<long long>(k) - 1 >= 0
               and i + k < n
               and text[i - k - 1] == text[i + k])
          ++k;

        result.even_radius[i] = k;

        const long long new_l = static_cast<long long>(i) - static_cast<long long>(k);
        const long long new_r = static_cast<long long>(i) + static_cast<long long>(k) - 1;
        if (new_r > r)
          {
            l = new_l;
            r = new_r;
          }
      }

    size_t best_len = 0;
    size_t best_begin = 0;

    for (size_t i = 0; i < n; ++i)
      {
        const size_t odd_len = 2 * result.odd_radius[i] - 1;
        const size_t odd_begin = i + 1 - result.odd_radius[i];
        if (odd_len > best_len)
          {
            best_len = odd_len;
            best_begin = odd_begin;
          }

        const size_t even_len = 2 * result.even_radius[i];
        const size_t even_begin = i - result.even_radius[i];
        if (even_len > best_len)
          {
            best_len = even_len;
            best_begin = even_begin;
          }
      }

    result.longest_begin = best_begin;
    result.longest_length = best_len;
    result.longest_palindrome = std::string(text.substr(best_begin, best_len));

    return result;
  }


  /** @brief Convenience wrapper returning only the longest palindromic substring.
   *
   *  @param[in] text Input text.
   *  @return Longest palindromic substring.
   */
  inline std::string longest_palindromic_substring(const std::string_view text)
  {
    return manacher(text).longest_palindrome;
  }
} // namespace Aleph

# endif // STRING_PALINDROMES_H
