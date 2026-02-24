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


/** @file String_DP.H
 *  @brief Dynamic-programming algorithms for string similarity and alignment.
 *
 *  Includes:
 *  - Levenshtein edit distance
 *  - Damerau-Levenshtein edit distance (full transposition model)
 *  - LCS (Longest Common Subsequence)
 *  - Longest Common Substring
 *
 *  @example edit_distance_example.cc
 *  @example damerau_levenshtein_example.cc
 *  @example lcs_longest_common_substring_example.cc
 *
 *  @ingroup Algorithms
 *  @author Leandro Rabindranath Leon
 */

# ifndef STRING_DP_H
# define STRING_DP_H

# include <algorithm>
# include <string>
# include <string_view>

# include <tpl_array.H>

namespace Aleph
{
  namespace string_dp_detail
  {
    inline Array<size_t> make_zero_row(const size_t cols)
    {
      Array<size_t> row = Array<size_t>::create(cols);
      for (size_t j = 0; j < cols; ++j)
        row[j] = 0;
      return row;
    }


    inline Array<Array<size_t>> make_zero_matrix(const size_t rows,
                                                 const size_t cols)
    {
      Array<Array<size_t>> mat;
      mat.reserve(rows);
      for (size_t i = 0; i < rows; ++i)
        mat.append(make_zero_row(cols));
      return mat;
    }
  } // namespace string_dp_detail


  /** @brief Levenshtein distance (insert/delete/substitute each cost 1).
   *
   *  @param[in] a First string.
   *  @param[in] b Second string.
   *  @return Edit distance.
   *
   *  @par Complexity
   *  O(nm) time and O(min(n,m)) space.
   */
  inline size_t levenshtein_distance(const std::string_view a,
                                     const std::string_view b)
  {
    if (a.empty())
      return b.size();
    if (b.empty())
      return a.size();

    std::string_view s1 = a;
    std::string_view s2 = b;
    if (s1.size() > s2.size())
      std::swap(s1, s2);

    Array<size_t> prev = Array<size_t>::create(s1.size() + 1);
    Array<size_t> curr = Array<size_t>::create(s1.size() + 1);

    for (size_t j = 0; j <= s1.size(); ++j)
      {
        prev[j] = 0;
        curr[j] = 0;
      }

    for (size_t j = 0; j <= s1.size(); ++j)
      prev[j] = j;

    for (size_t i = 1; i <= s2.size(); ++i)
      {
        curr[0] = i;

        for (size_t j = 1; j <= s1.size(); ++j)
          {
            const size_t cost = s2[i - 1] == s1[j - 1] ? 0 : 1;
            curr[j] = std::min({
                                 prev[j] + 1,
                                 curr[j - 1] + 1,
                                 prev[j - 1] + cost
                               });
          }

        prev.swap(curr);
      }

    return prev[s1.size()];
  }


  /** @brief Alias for Levenshtein distance. */
  inline size_t edit_distance(const std::string_view a,
                              const std::string_view b)
  {
    return levenshtein_distance(a, b);
  }


  /** @brief Damerau-Levenshtein distance with adjacent transpositions.
   *
   *  This implementation follows the "full" dynamic-programming model using
   *  last-seen positions and supports multiple transpositions.
   *
   *  @param[in] a First string.
   *  @param[in] b Second string.
   *  @return Damerau-Levenshtein distance.
   *
   *  @par Complexity
   *  O(nm) time and O(nm) space.
   */
  inline size_t damerau_levenshtein_distance(const std::string_view a,
                                             const std::string_view b)
  {
    const size_t n = a.size();
    const size_t m = b.size();

    const size_t inf = n + m;
    Array<Array<size_t>> d =
        string_dp_detail::make_zero_matrix(n + 2, m + 2);

    d[0][0] = inf;
    for (size_t i = 0; i <= n; ++i)
      {
        d[i + 1][1] = i;
        d[i + 1][0] = inf;
      }

    for (size_t j = 0; j <= m; ++j)
      {
        d[1][j + 1] = j;
        d[0][j + 1] = inf;
      }

    size_t last_row[256] = {0};

    for (size_t i = 1; i <= n; ++i)
      {
        size_t last_match_col = 0;

        for (size_t j = 1; j <= m; ++j)
          {
            const size_t i1 = last_row[static_cast<unsigned char>(b[j - 1])];
            const size_t j1 = last_match_col;

            size_t cost = 1;
            if (a[i - 1] == b[j - 1])
              {
                cost = 0;
                last_match_col = j;
              }

            d[i + 1][j + 1] = std::min({
                                         d[i][j] + cost,
                                         d[i + 1][j] + 1,
                                         d[i][j + 1] + 1,
                                         d[i1][j1] + (i - i1 - 1) + 1 + (j - j1 - 1)
                                       });
          }

        last_row[static_cast<unsigned char>(a[i - 1])] = i;
      }

    return d[n + 1][m + 1];
  }


  /** @brief Result for Longest Common Subsequence (LCS). */
  struct LCS_Result
  {
    size_t length = 0;
    std::string subsequence;
  };


  /** @brief Compute Longest Common Subsequence.
   *
   *  @param[in] a First string.
   *  @param[in] b Second string.
   *  @return LCS length and one optimal subsequence.
   *
   *  @par Complexity
   *  O(nm) time and O(nm) space.
   */
  inline LCS_Result longest_common_subsequence(const std::string_view a,
                                               const std::string_view b)
  {
    const size_t n = a.size();
    const size_t m = b.size();

    Array<Array<size_t>> dp =
        string_dp_detail::make_zero_matrix(n + 1, m + 1);

    for (size_t i = 1; i <= n; ++i)
      for (size_t j = 1; j <= m; ++j)
        if (a[i - 1] == b[j - 1])
          dp[i][j] = dp[i - 1][j - 1] + 1;
        else
          dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);

    std::string seq;
    seq.reserve(dp[n][m]);

    size_t i = n;
    size_t j = m;
    while (i > 0 and j > 0)
      if (a[i - 1] == b[j - 1])
        {
          seq.push_back(a[i - 1]);
          --i;
          --j;
        }
      else if (dp[i - 1][j] >= dp[i][j - 1])
        --i;
      else
        --j;

    std::reverse(seq.begin(), seq.end());

    return LCS_Result{dp[n][m], seq};
  }


  /** @brief Result for longest common substring. */
  struct Longest_Common_Substring_Result
  {
    size_t length = 0;
    size_t begin_a = 0;
    size_t begin_b = 0;
    std::string substring;
  };


  /** @brief Compute the longest common substring (contiguous) between two strings.
   *
   *  @param[in] a First string.
   *  @param[in] b Second string.
   *  @return Length, starting indices, and substring.
   *
   *  @par Complexity
   *  O(nm) time and O(m) space.
   */
  inline Longest_Common_Substring_Result
  longest_common_substring(const std::string_view a,
                           const std::string_view b)
  {
    const size_t n = a.size();
    const size_t m = b.size();

    Array<size_t> prev = Array<size_t>::create(m + 1);
    Array<size_t> curr = Array<size_t>::create(m + 1);

    for (size_t j = 0; j <= m; ++j)
      {
        prev[j] = 0;
        curr[j] = 0;
      }

    size_t best_len = 0;
    size_t end_a = 0;
    size_t end_b = 0;

    for (size_t i = 1; i <= n; ++i)
      {
        for (size_t j = 1; j <= m; ++j)
          if (a[i - 1] == b[j - 1])
            {
              curr[j] = prev[j - 1] + 1;
              if (curr[j] > best_len)
                {
                  best_len = curr[j];
                  end_a = i;
                  end_b = j;
                }
            }
          else
            curr[j] = 0;

        prev.swap(curr);
        for (size_t j = 0; j <= m; ++j)
          curr[j] = 0;
      }

    Longest_Common_Substring_Result result;
    result.length = best_len;

    if (best_len == 0)
      {
        result.substring = "";
        return result;
      }

    result.begin_a = end_a - best_len;
    result.begin_b = end_b - best_len;
    result.substring = std::string(a.substr(result.begin_a, best_len));

    return result;
  }
} // namespace Aleph

# endif // STRING_DP_H
