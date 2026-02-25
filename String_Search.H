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


/** @file String_Search.H
 *  @brief Classical pattern searching algorithms over strings.
 *
 *  This header provides single-pattern matching algorithms commonly used in
 *  competitive programming and production text processing:
 *  - Knuth-Morris-Pratt (KMP)
 *  - Z-algorithm
 *  - Boyer-Moore-Horspool
 *  - Rabin-Karp (rolling hash with exact verification)
 *
 *  All APIs return all match positions (0-based), including overlaps.
 *
 *  @example kmp_example.cc
 *  @example z_algorithm_example.cc
 *  @example horspool_example.cc
 *  @example rabin_karp_example.cc
 *
 *  @ingroup Algorithms
 *  @author Leandro Rabindranath Leon
 */

# ifndef STRING_SEARCH_H
# define STRING_SEARCH_H

# include <algorithm>
# include <string>
# include <string_view>

# include <tpl_array.H>

namespace Aleph
{
  namespace string_search_detail
  {
    /** @brief Generate an array of all positions from 0 to text_size.
     *  @param[in] text_size The size of the text.
     *  @return Array of positions.
     */
    inline Array<size_t> all_match_positions(const size_t text_size)
    {
      Array<size_t> matches;
      matches.reserve(text_size + 1);
      for (size_t i = 0; i <= text_size; ++i)
        matches.append(i);
      return matches;
    }


    /** @brief Find a character not present in either string.
     *  @param[in] a First string.
     *  @param[in] b Second string.
     *  @param[out] delimiter The unused character found.
     *  @return true if an unused character was found, false otherwise.
     */
    inline bool find_unused_delimiter(const std::string_view a,
                                      const std::string_view b,
                                      char & delimiter)
    {
      bool used[256] = {false};
      for (const unsigned char c : a)
        used[c] = true;

      for (const unsigned char c : b)
        used[c] = true;

      for (size_t i = 1; i < 256; ++i)
        if (not used[i])
          {
            delimiter = static_cast<char>(i);
            return true;
          }

      return false;
    }


    /** @brief Check if pattern matches text at given position.
     *  @param[in] text The text to search in.
     *  @param[in] pattern The pattern to match.
     *  @param[in] pos The position in text to check.
     *  @return true if matches, false otherwise.
     */
    inline bool equals_at(const std::string_view text,
                          const std::string_view pattern,
                          const size_t pos)
    {
      for (size_t i = 0; i < pattern.size(); ++i)
        if (text[pos + i] != pattern[i])
          return false;
      return true;
    }
  } // namespace string_search_detail


  /** @brief Prefix-function used by KMP.
   *
   *  For each index `i`, returns the length of the longest proper prefix of
   *  `pattern[0..i]` which is also a suffix of `pattern[0..i]`.
   *
   *  @param[in] pattern Pattern to preprocess.
   *  @return Prefix-function array with size `pattern.size()`.
   *  @throws None.
   *
   *  @par Complexity
   *  O(m) time and O(m) space, where `m = pattern.size()`.
   */
  inline Array<size_t> kmp_prefix_function(const std::string_view pattern)
  {
    Array<size_t> pi;
    if (pattern.empty())
      return pi;

    pi = Array<size_t>::create(pattern.size());
    pi[0] = 0;

    size_t j = 0;
    for (size_t i = 1; i < pattern.size(); ++i)
      {
        while (j > 0 and pattern[i] != pattern[j])
          j = pi[j - 1];

        if (pattern[i] == pattern[j])
          ++j;

        pi[i] = j;
      }

    return pi;
  }


  /** @brief Find all occurrences of a pattern using KMP.
   *
   *  @param[in] text Text where matches are searched.
   *  @param[in] pattern Pattern to find.
   *  @return Array with all match start indices (0-based).
   *  @throws None.
   *
   *  @note If `pattern` is empty, returns all positions in `[0, text.size()]`.
   *
   *  @par Complexity
   *  O(n + m) time and O(m) extra space.
   */
  inline Array<size_t> kmp_search(const std::string_view text,
                                  const std::string_view pattern)
  {
    if (pattern.empty())
      return string_search_detail::all_match_positions(text.size());

    Array<size_t> matches;
    if (text.size() < pattern.size())
      return matches;

    const Array<size_t> pi = kmp_prefix_function(pattern);

    size_t j = 0;
    for (size_t i = 0; i < text.size(); ++i)
      {
        while (j > 0 and text[i] != pattern[j])
          j = pi[j - 1];

        if (text[i] == pattern[j])
          ++j;

        if (j == pattern.size())
          {
            matches.append(i + 1 - pattern.size());
            j = pi[j - 1];
          }
      }

    return matches;
  }


  /** @brief Compute Z-array for a string.
   *
   *  `z[i]` equals the longest length `L` such that
   *  `s[0..L-1] == s[i..i+L-1]`.
   *
   *  @param[in] s Input string.
   *  @return Z-array with size `s.size()`. When non-empty, `z[0] = s.size()`.
   *  @throws None.
   *
   *  @par Complexity
   *  O(n) time and O(n) space.
   */
  inline Array<size_t> z_algorithm(const std::string_view s)
  {
    Array<size_t> z;
    if (s.empty())
      return z;

    z = Array<size_t>::create(s.size());
    z[0] = s.size();

    size_t l = 0;
    size_t r = 0;

    for (size_t i = 1; i < s.size(); ++i)
      {
        if (i <= r)
          z[i] = std::min(r - i + 1, z[i - l]);
        else
          z[i] = 0;

        while (i + z[i] < s.size() and s[z[i]] == s[i + z[i]])
          ++z[i];

        if (const size_t reach = i + z[i]; reach > r + 1)
          {
            l = i;
            r = reach - 1;
          }
      }

    return z;
  }


  /** @brief Find all occurrences of a pattern using the Z-algorithm.
   *
   *  @param[in] text Text where matches are searched.
   *  @param[in] pattern Pattern to find.
   *  @return Array with all match start indices (0-based).
   *  @throws None.
   *
   *  @note If no delimiter byte is available between `pattern` and `text`,
   *        this function falls back to `kmp_search`.
   *  @note If `pattern` is empty, returns all positions in `[0, text.size()]`.
   *
   *  @par Complexity
   *  O(n + m) time and O(n + m) space.
   */
  inline Array<size_t> z_search(const std::string_view text,
                                const std::string_view pattern)
  {
    if (pattern.empty())
      return string_search_detail::all_match_positions(text.size());

    if (text.size() < pattern.size())
      return {};

    char delimiter = 0;
    if (not string_search_detail::find_unused_delimiter(pattern, text, delimiter))
      return kmp_search(text, pattern);

    std::string combined;
    combined.reserve(pattern.size() + text.size() + 1);
    combined += pattern;
    combined += delimiter;
    combined += text;

    const Array<size_t> z = z_algorithm(combined);

    Array<size_t> matches;
    const size_t offset = pattern.size() + 1;
    for (size_t i = offset; i < combined.size(); ++i)
      if (z[i] >= pattern.size())
        matches.append(i - offset);

    return matches;
  }


  /** @brief Find all occurrences using Boyer-Moore-Horspool.
   *
   *  @param[in] text Text where matches are searched.
   *  @param[in] pattern Pattern to find.
   *  @return Array with all match start indices (0-based).
   *  @throws None.
   *
   *  @note If `pattern` is empty, returns all positions in `[0, text.size()]`.
   *
   *  @par Complexity
   *  Average-case sublinear. Worst-case O(nm). Extra space O(1)
   *  (fixed 256-byte alphabet table).
   */
  inline Array<size_t> boyer_moore_horspool_search(const std::string_view text,
                                                   const std::string_view pattern)
  {
    if (pattern.empty())
      return string_search_detail::all_match_positions(text.size());

    Array<size_t> matches;
    const size_t n = text.size();
    const size_t m = pattern.size();

    if (n < m)
      return matches;

    Array<size_t> shift = Array<size_t>::create(256);
    for (size_t i = 0; i < 256; ++i)
      shift[i] = m;

    for (size_t i = 0; i + 1 < m; ++i)
      shift[static_cast<unsigned char>(pattern[i])] = m - 1 - i;

    size_t i = 0;
    while (i + m <= n)
      {
        size_t j = m;
        while (j > 0 and pattern[j - 1] == text[i + j - 1])
          --j;

        if (j == 0)
          matches.append(i);

        const auto c = static_cast<unsigned char>(text[i + m - 1]);
        const size_t step = shift[c] == 0 ? 1 : shift[c];
        i += step;
      }

    return matches;
  }


  /** @brief Find all occurrences using Rabin-Karp with rolling hash.
   *
   *  Hash collisions are filtered by exact character-by-character verification,
   *  so returned matches are exact.
   *
   *  @param[in] text Text where matches are searched.
   *  @param[in] pattern Pattern to find.
   *  @param[in] base Rolling-hash base.
   *  @return Array with all match start indices (0-based).
   *  @throws None.
   *
   *  @note If `pattern` is empty, returns all positions in `[0, text.size()]`.
   *  @note The rolling hash uses implicit modulo via `uint64_t` overflow
   *        (no explicit prime modulus).  This is sufficient for general use
   *        but may produce more collisions on adversarial inputs.
   *
   *  @par Complexity
   *  Expected O(n + m), worst-case O(nm) due to adversarial collisions.
   */
  inline Array<size_t> rabin_karp_search(const std::string_view text,
                                         const std::string_view pattern,
                                         const uint64_t base = 911382323ull)
  {
    if (pattern.empty())
      return string_search_detail::all_match_positions(text.size());

    Array<size_t> matches;
    const size_t n = text.size();
    const size_t m = pattern.size();

    if (n < m)
      return matches;

    uint64_t power = 1;
    for (size_t i = 1; i < m; ++i)
      power *= base;

    uint64_t pattern_hash = 0;
    uint64_t window_hash = 0;

    for (size_t i = 0; i < m; ++i)
      {
        pattern_hash = pattern_hash * base
                       + static_cast<unsigned char>(pattern[i]) + 1ull;
        window_hash = window_hash * base
                      + static_cast<unsigned char>(text[i]) + 1ull;
      }

    if (window_hash == pattern_hash
        and string_search_detail::equals_at(text, pattern, 0))
      matches.append(0);

    for (size_t i = m; i < n; ++i)
      {
        const uint64_t old_c = static_cast<unsigned char>(text[i - m]) + 1ull;
        const uint64_t new_c = static_cast<unsigned char>(text[i]) + 1ull;

        window_hash = (window_hash - old_c * power) * base + new_c;

        if (const size_t pos = i + 1 - m; window_hash == pattern_hash
                                          and string_search_detail::equals_at(text, pattern, pos))
          matches.append(pos);
      }

    return matches;
  }
} // namespace Aleph

# endif // STRING_SEARCH_H
