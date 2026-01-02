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

/** @file ah-std::string-utils.H
 *  @brief String manipulation utilities.
 *
 *  This file provides a comprehensive set of std::string utilities including
 *  trimming, splitting, joining, case conversion, numeric parsing, and
 *  various std::string transformation functions.
 *
 *  @ingroup Utils
 */

# ifndef AH_STRING_UTILS_H
# define AH_STRING_UTILS_H

# include <algorithm>
# include <cctype>
# include <cerrno>
# include <cstdlib>
# include <iomanip>
# include <limits>
# include <stdexcept>
# include <string_view>
# include <type_traits>
# include <cstring>
# include <memory>
# include <string>
# include <sstream>
# include <iterator>
# include <vector>
# include <htlist.H>
# include <tpl_array.H>
# include <ah-zip.H>

namespace Aleph
{
  /** \file ah-std::string-utils.H

      \brief Assorted std::string utilities.

      This header provides small, header-only helpers for:
      - Converting containers to strings
      - Trimming and case conversion
      - Tokenizing/splitting
      - Simple numeric validation
      - Text layout (justify/align)

      Most functions operate on byte strings and use C locale `ctype` functions.
  */

  /** Convert a `std::vector<T>` to a comma-separated std::string.

      \tparam T Element type streamable to `std::ostream`.
      \param[in] v Vector to stringify.
      \return Comma-separated representation (empty std::string if `v` is empty).
  */
  template <typename T>
  std::string to_string(const std::vector<T> & v)
  {
    std::ostringstream s;
    for (size_t i = 0; i < v.size(); ++i)
      {
        s << v[i];
        if (i < v.size() - 1)
          s << ", ";
      }
    return s.str();
  }

  /** Convert an `Aleph::Array<T>` to a comma-separated std::string.

      \tparam T Element type streamable to `std::ostream`.
      \param[in] v Array to stringify.
      \return Comma-separated representation (empty std::string if `v` is empty).
  */
  template <typename T>
  std::string to_string(const Array<T> & v)
  {
    std::ostringstream s;
    for (auto it = v.get_it(); it.has_curr(); it.next_ne())
      {
        s << it.get_curr();
        if (not it.is_last())
          s << ", ";
      }
    return s.str();
  }

  // left trim
  /** Remove leading whitespace from a std::string in-place.

      Whitespace is defined by `std::isspace` in the current C locale.

      \param[in,out] s String to modify.
  */
  inline void ltrim(std::string & s)
  {
    s.erase(s.begin(), std::ranges::find_if(s,
                                       [](const unsigned char ch)
                                         {
                                           return not std::isspace(ch);
                                         }));
  }

  // trim from end
  /** Remove trailing whitespace from a std::string in-place.

      Whitespace is defined by `std::isspace` in the current C locale.

      \param[in,out] s String to modify.
  */
  inline void rtrim(std::string & s)
  {
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         [](const unsigned char ch)
                           {
                             return not std::isspace(ch);
                           }).base(), s.end());
  }

  // fast concatenation of several strings
  /** Concatenate multiple streamable arguments into a single std::string.

      This is implemented via `std::ostringstream` and stream insertion.

      \tparam Args Types streamable to `std::ostream`.
      \param[in] args Values to concatenate.
      \return Concatenated std::string.
  */
  template <class... Args>
  inline std::string concat(const Args &... args)
  {
    std::ostringstream s;
    (s << ... << args);
    return s.str();
  }

  // trim from both ends (in place)
  /** Return a trimmed copy of a std::string (leading + trailing whitespace removed).

      \param[in] s Input std::string.
      \return Trimmed copy of `s`.
  */
  inline std::string trim(const std::string & s)
  {
    std::string ret = s;
    ltrim(ret);
    rtrim(ret);
    return ret;
  }

  /** Trim a std::string in-place (leading + trailing whitespace removed).

      \param[in,out] s String to modify.
      \return Reference to `s`.
  */
  inline std::string &trim_in_place(std::string & s)
  {
    ltrim(s);
    rtrim(s);
    return s;
  }

  /** Check if `substr` appears inside `str`.

      \param[in] str Haystack.
      \param[in] substr Needle.
      \return `true` if `substr` is a substring of `str`.
  */
  inline bool contains(const std::string_view & str, const std::string_view & substr)
  {
    return str.find(substr) != std::string::npos;
  }

  /** Format a `double` using fixed notation with the given precision.

      \param[in] d Value to format.
      \param[in] precision Number of digits after the decimal point.
      \return Formatted std::string.
  */
  inline std::string to_string(const double d, const size_t precision)
  {
    std::ostringstream s;
    s.precision(precision);
    s << std::fixed << d;
    return s.str();
  }

  /** Convert `double` to a std::string with maximum round-trip precision.

      Uses `std::numeric_limits<double>::max_digits10`.

      \param[in] d Value to format.
      \return String representation.
  */
  inline std::string to_str(const double d)
  {
    return to_string(d, std::numeric_limits<double>::max_digits10);
  }

  /** Convert a C std::string to lower-case.

      \param[in] str Null-terminated input std::string.
      \return Lowercased copy.
  */
  inline std::string tolower(const char *str)
  {
    std::string ret;
    for (const char *ptr = str; *ptr; ++ptr)
      ret.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(*ptr))));
    return ret;
  }

  /** Convert a C std::string to upper-case.

      \param[in] str Null-terminated input std::string.
      \return Uppercased copy.
  */
  inline std::string toupper(const char *str)
  {
    std::string ret;
    for (const char *ptr = str; *ptr; ++ptr)
      ret.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(*ptr))));
    return ret;
  }

  /** Convert a `std::string` to lower-case.

      \param[in] str Input std::string.
      \return Lowercased copy.
  */
  inline std::string tolower(const std::string & str)
  {
    return tolower(str.c_str());
  }

  /** Convert a `std::string` to upper-case.

      \param[in] str Input std::string.
      \return Uppercased copy.
  */
  inline std::string toupper(const std::string & str)
  {
    return toupper(str.c_str());
  }

  /** Convert a std::string to lower-case in-place.

      \param[in,out] str String to modify.
      \return Reference to `str`.
  */
  inline std::string &mutable_tolower(std::string & str)
  {
    for (auto & c: str)
      c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return str;
  }

  /** Convert a std::string to upper-case in-place.

      \param[in,out] str String to modify.
      \return Reference to `str`.
  */
  inline std::string &mutable_toupper(std::string & str)
  {
    for (auto & c: str)
      c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return str;
  }

  /** Extract alphanumeric ASCII characters and normalize letters to lower-case.

      - Keeps digits `0..9`
      - Keeps letters `a..z` as-is
      - Converts letters `A..Z` to lower-case
      - Drops any other character

      \param[in] str Input std::string.
      \return Normalized std::string.
  */
  inline std::string only_alpha(const std::string & str)
  {
    std::string ret;
    ret.reserve(str.size() + 1);
    for (const char c: str)
      switch (c)
        {
        case '0'...'9':
        case 'a'...'z':
          ret.push_back(c);
          break;
        case 'A'...'Z':
          ret.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
          break;
        default:
          break;
        }

    return ret;
  }

  /** Remove all whitespace characters from a std::string.

      Whitespace is defined by `std::isspace` in the current C locale.

      \param[in] str Input std::string.
      \return Copy of `str` with whitespace removed.
  */
  inline std::string remove_spaces(const std::string & str)
  {
    const size_t n = str.size();
    std::string ret;
    ret.reserve(n + 1);
    for (const auto c: str)
      if (not std::isspace(static_cast<unsigned char>(c)))
        ret.push_back(c);
    return ret;
  }

  /** Remove any character appearing in `symbols`.

      \param[in] str Input std::string.
      \param[in] symbols Set of characters to remove.
      \return Copy of `str` without any character present in `symbols`.
  */
  inline std::string remove_symbols(const std::string & str, const std::string & symbols)
  {
    const size_t n = str.size();
    std::string ret;
    ret.reserve(n + 1);
    for (const auto c: str)
      if (symbols.find(c) == std::string::npos)
        ret.push_back(c);

    return ret;
  }

  /** Join elements of an Aleph-style container into a stream.

      The container is expected to support `is_empty()`, `get_last()`, and
      `get_it()` with `has_curr()/next_ne()/get_curr()`.

      \tparam C Container type.
      \param[in] c Container to join.
      \param[in] sep Separator inserted between elements.
      \param[in,out] out Output stream.
      \return Reference to `out`.
  */
  template <class C>
  inline std::ostream &join(const C & c, const std::string & sep, std::ostream & out)
  {
    if (c.is_empty())
      return out;

    auto & last = c.get_last();
    for (auto it = c.get_it(); it.has_curr(); it.next_ne())
      {
        auto & curr = it.get_curr();
        out << curr;
        if (&curr != &last)
          out << sep;
      }
    return out;
  }

  /** Join elements of an Aleph-style container into a std::string.

      \tparam C Container type.
      \param[in] c Container to join.
      \param[in] sep Separator inserted between elements (defaults to a space).
      \return Joined std::string.
  */
  template <class C>
  inline std::string join(const C & c, const std::string & sep = " ")
  {
    std::ostringstream s;
    join(c, sep, s);
    return s.str();
  }

  // return true if str can be converted to a double
  /** Check whether a std::string fully parses as a finite `double`.

      Parses using `std::strtod` and requires that the entire std::string is consumed.
      Values that overflow/underflow (ERANGE) are rejected.

      \param[in] str Input std::string.
      \return `true` if `str` fully parses as a `double` and is in range.
  */
  inline bool is_double(const std::string & str)
  {
    const char *begin = str.c_str();
    char *endptr = nullptr;
    errno = 0;
    (void) std::strtod(begin, &endptr);
    if (endptr == begin or *endptr != '\0')
      return false;
    return errno != ERANGE;
  }

  /** Check whether a std::string fully parses as a finite `float`.

      Parses using `std::strtof` and requires that the entire std::string is consumed.
      Values that overflow/underflow (ERANGE) are rejected.

      \param[in] str Input std::string.
      \return `true` if `str` fully parses as a `float` and is in range.
  */
  inline bool is_float(const std::string & str)
  {
    const char *begin = str.c_str();
    char *endptr = nullptr;
    errno = 0;
    (void) std::strtof(begin, &endptr);
    if (endptr == begin or *endptr != '\0')
      return false;
    return errno != ERANGE;
  }

  /** Check whether a std::string fully parses as a `long`.

      Parses using `std::strtol` and requires that the entire std::string is consumed.
      Values that overflow/underflow (ERANGE) are rejected.

      \param[in] str Input std::string.
      \return `true` if `str` fully parses as a `long` and is in range.
  */
  inline bool is_long(const std::string & str)
  {
    const char *begin = str.c_str();
    char *endptr = nullptr;
    errno = 0;
    (void) std::strtol(begin, &endptr, 10);
    if (endptr == begin or *endptr != '\0')
      return false;
    return errno != ERANGE;
  }

  /** Check whether a std::string fully parses as a non-negative `size_t`.

      Parses using `std::strtoull` and requires that the entire std::string is consumed.
      Negative strings are rejected.

      \param[in] str Input std::string.
      \return `true` if `str` parses as `size_t` and is in range.
  */
  inline bool is_size_t(const std::string & str)
  {
    if (str.empty() or str[0] == '-')
      return false;

    const char *begin = str.c_str();
    char *endptr = nullptr;
    errno = 0;
    const auto val = std::strtoull(begin, &endptr, 10);
    if (endptr == begin or *endptr != '\0')
      return false;
    if (errno == ERANGE)
      return false;
    return val <= std::numeric_limits<size_t>::max();
  }


  /** Convert a std::string to `long` and throw on parse errors.

      \param[in] s Input std::string.
      \return Parsed `long` value.
      \throw std::runtime_error If `s` does not contain a valid `long`.
  */
  inline long safe_atol(const std::string & s)
  {
    errno = 0;
    char *endptr = nullptr;
    const auto val = std::strtol(s.c_str(), &endptr, 10);
    const bool invalid =
        (errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) ||
        (errno != 0 && val == 0) ||
        (endptr == s.c_str());

    ah_runtime_error_if(invalid) << "invalid std::string for long: " << s;
    return val;
  }

  /** Convert a std::string to `double` and throw on parse errors.

      \param[in] s Input std::string.
      \return Parsed `double` value.
      \throw std::runtime_error If `s` does not contain a valid `double`.
  */
  inline double safe_atof(const std::string & s)
  {
    errno = 0;
    char *endptr = nullptr;
    const auto val = std::strtod(s.c_str(), &endptr);
    const bool invalid =
        (errno == ERANGE and (val == HUGE_VAL or val == HUGE_VALF or val == HUGE_VALL)) ||
        (errno != 0 && val == 0) ||
        endptr == s.c_str();

    ah_runtime_error_if(invalid) << "invalid std::string for double: " << s;
    return val;
  }

  /** Check whether `prefix` is a prefix of `str`.

      \param[in] str Input std::string.
      \param[in] prefix Prefix to test.
      \return `true` if `str` begins with `prefix`.
  */
  inline bool is_prefix(const std::string & str, const std::string & prefix)
  {
    if (str.size() < prefix.size())
      return false;
    return strncmp(str.data(), prefix.data(), prefix.size()) == 0;
  }

  /** Remove `prefix` from `str` if present.

      \param[in,out] str String to modify.
      \param[in] prefix Prefix to remove.
      \return Reference to `str`.
  */
  inline std::string remove_prefix(std::string & str, const std::string & prefix)
  {
    if (is_prefix(str, prefix))
      str = str.substr(prefix.size());
    return str;
  }

  /** Convert a std::string to lower-case (byte-wise).

      \param[in] str Input std::string.
      \return Lowercased copy.
  */
  inline std::string to_lower(const std::string & str)
  {
    std::string ret;
    for (const char c: str)
      ret.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    return ret;
  }

  /** Convert a std::string to upper-case (byte-wise).

      \param[in] str Input std::string.
      \return Uppercased copy.
  */
  inline std::string to_upper(const std::string & str)
  {
    std::string ret;
    for (const char c: str)
      ret.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    return ret;
  }

  /** Uppercase the first character of `str` and return the resulting copy.

      \param[in] str Input std::string.
      \return Copy with first character converted to upper-case.
  */
  inline std::string to_name(const std::string & str)
  {
    if (str.empty())
      return str;

    std::string ret;
    ret.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(str[0]))));

    for (size_t i = 1; i < str.size(); ++i)
      ret.push_back(str[i]);

    return ret;
  }

  /** Split a camelCase / PascalCase std::string into tokens.

      A new token starts at each upper-case character.

      \param[in] str Null-terminated input std::string.
      \return List of tokens.
  */
  inline DynList<std::string> split_camel_case(const char *const str)
  {
    DynList<std::string> ret;
    if (str == nullptr or *str == '\0')
      return ret;
    const char *ptr = str;
    std::string curr(1, *ptr);
    while (*++ptr)
      if (std::isupper(static_cast<unsigned char>(*ptr)))
        {
          ret.append(curr);
          curr = std::string(1, *ptr);
        }
      else
        curr.push_back(*ptr);

    if (not curr.empty())
      ret.append(curr);

    return ret;
  }

  /** Split a camelCase / PascalCase std::string into tokens.

      \param[in] str Input std::string.
      \return List of tokens.
  */
  inline DynList<std::string> split_camel_case(const std::string & str)
  {
    return split_camel_case(str.data());
  }

  /** Fill all the content of std::string with a defined char.

      Use for deleting sensitive data; for example, passwords

  */
  inline void fill_string(std::string & str, char sym)
  {
    for (char & c: str)
      c = sym;
  }

  /** Split a std::string by a single delimiter character.

      This overload appends tokens to an existing vector.

      \param[in] s Input std::string.
      \param[in] delim Delimiter character.
      \param[in,out] elems Output vector; tokens are appended.
      \return Reference to `elems`.
  */
  inline std::vector<std::string> &
  split(const std::string & s, char delim, std::vector<std::string> & elems)
  {
    std::stringstream ss(s);
    std::string item;
    while (getline(ss, item, delim))
      elems.push_back(item);

    return elems;
  }

  /** Split a std::string by a single delimiter character.

      \param[in] s Input std::string.
      \param[in] delim Delimiter character.
      \return Vector of tokens (including empty tokens, as `getline` produces).
  */
  inline std::vector<std::string> split(const std::string & s, char delim)
  {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
  }

  /** Split a std::string by a set of delimiter characters.

      The delimiter std::string is treated as a set: any character in `delim`
      separates tokens.

      \tparam Container Output container template accepting `std::string` and
              supporting `append(const std::string&)`.
      \param[in] s Input std::string.
      \param[in] delim Delimiter character set.
      \return Container of tokens. Empty input yields an empty container.
  */
  template <template <typename> class Container = DynList>
  inline Container<std::string> split_string(const std::string & s, const std::string & delim)
  {
    Container<std::string> elems;
    if (s.empty())
      return elems;

    if (delim.empty())
      {
        elems.append(s);
        return elems;
      }

    char *saveptr = nullptr;
    char **p = &saveptr;

    std::string data = s;
    const char *d = delim.data();
    const auto str = data.data();

    for (char *token = strtok_r(str, d, p); token;
         token = strtok_r(nullptr, d, p))
      elems.append(token);

    return elems;
  }

  /** Split a std::string into an `Aleph::DynList<std::string>`.

      \param[in] s Input std::string.
      \param[in] delim Delimiter character set.
      \return List of tokens.
  */
  inline DynList<std::string> split_to_list(const std::string & s,
                                            const std::string & delim)
  {
    return split_string<DynList>(s, delim);
  }

  /** Convert an identifier-like std::string to PascalCase.

      Splits on the delimiter set `" _-"` and uppercases the first character
      of each token.

      \param[in] str Input std::string.
      \return PascalCased std::string.
  */
  inline std::string to_Pascalcase(const std::string & str)
  {
    DynList<std::string> tokens = split_to_list(str, " _-");

    tokens.mutable_for_each([](std::string & token)
                              {
                                token[0] = ::toupper(token[0]);
                              });

    return tokens.foldl<std::string>("", [](const std::string & acu, const std::string & token)
                                  {
                                    return acu + token;
                                  });
  }

  /** Split a std::string into an `Aleph::Array<std::string>`.

      \param[in] s Input std::string.
      \param[in] delim Delimiter character set.
      \return Array of tokens.
  */
  inline Array<std::string> split_to_array(const std::string & s,
                                           const std::string & delim)
  {
    return split_string<Array>(s, delim);
  }

  /** Split a std::string at a fixed position.

      \param[in] str Input std::string.
      \param[in] pos Split position (0..str.size()).
      \return Pair `{left, right}` where `left.size() == pos`.
      \throw std::range_error If `pos > str.size()`.
  */
  inline std::pair<std::string, std::string> split_pos(const std::string & str, size_t pos)
  {
    if (pos > str.size())
      {
        std::ostringstream s;
        s << "split_pos(" << str << ", " << pos << "): position " << pos
            << " is larger than std::string size " << str.size();
        ah_range_error_if(pos > str.size()) << s.str();
      }

    const char *ptr = str.data();
    const char *end1 = ptr + pos;

    std::string s1;
    while (ptr != end1)
      s1.push_back(*ptr++);

    std::string s2;
    while (*ptr)
      s2.push_back(*ptr++);

    return {s1, s2};
  }

  /** Split a std::string into `n` parts.

      The first `n-1` parts have length `str.size() / n`. The last part
      receives the remainder.

      \param[in] str Input std::string.
      \param[in] n Number of parts.
      \return List with exactly `n` elements.
      \throw std::range_error If `n == 0` or `n > str.size()`.
  */
  inline DynList<std::string> split_n(const std::string & str, size_t n)
  {
    if (n == 0)
      ah_range_error_if(n == 0) << "split_n(): number of parts cannot be 0";

    if (n > str.size())
      {
        std::ostringstream s;
        s << "split_n(" << str << ", " << n << "): number of parts " << n
            << " is larger than std::string size " << str.size();
        ah_range_error_if(n > str.size()) << s.str();
      }

    const size_t sz = str.size() / n;

    const char *ptr = str.data();

    DynList<std::string> ret;

    for (size_t i = 0; i < n; ++i)
      {
        std::string s;
        if (i + 1 == n)
          {
            while (*ptr)
              s.push_back(*ptr++);
          }
        else
          {
            for (size_t k = 0; k < sz and *ptr; ++k)
              s.push_back(*ptr++);
          }
        ret.append(s);
      }

    return ret;
  }

  /** Pad all rows of a matrix to the maximum row length.

      Each row is extended with default-constructed `T()` until all rows
      have equal size.

      \tparam T Cell type.
      \param[in,out] m Matrix to normalize.
      \return New matrix with completed rows.
  */
  template <typename T>
  inline DynList<DynList<T>> complete_rows(DynList<DynList<T>> & m)
  {
    size_t max_sz = 0;
    DynList<size_t> sizes =
        m.template maps<size_t>([&max_sz](auto & l)
                                  {
                                    const size_t sz = l.size();
                                    max_sz = std::max(max_sz, sz);
                                    return sz;
                                  });

    DynList<DynList<T>> ret;
    for (auto it = zip_it(m, sizes); it.has_curr(); it.next_ne())
      {
        auto t = it.get_curr();
        DynList<T> & l = get<0>(t);
        const size_t sz = get<1>(t);
        const long n = max_sz - sz;
        for (auto i = 0; i < n; ++i)
          l.append(T());
        ret.append(std::move(l));
      }

    return ret;
  }

  /**
     @param[in] lens parallel row containing the maximum lenght og each colunm
  */
  inline DynList<DynList<std::string>>
  format_string(const DynList<size_t> & lens,
                const DynList<DynList<std::string>> & mat)
  {
    return mat.maps<DynList<std::string>>([&lens](const DynList<std::string> & l)
                                       {
                                         return zip(lens, l).template maps<std::string>([](const auto & p)
                                              {
                                                const std::string blanks(p.first > p.second.size() ?
                                                                      p.first - p.second.size() :
                                                                      0, ' ');
                                                return blanks + p.second + " ";
                                              });
                                       });
  }

  /** Write a formatted matrix to a stream.

      \param[in,out] out Output stream.
      \param[in] lens Column widths.
      \param[in] mat Matrix.
      \return Reference to `out`.
  */
  inline std::ostream &format_string(std::ostream & out, const DynList<size_t> & lens,
                                const DynList<DynList<std::string>> & mat)
  {
    for (auto it = mat.get_it(); it.has_curr(); it.next_ne())
      {
        const auto & row = it.get_curr();
        auto line = zip(lens, row).
            template maps<std::string>([](const auto & p)
                                    {
                                      const std::string blanks(p.first > p.second.size() ?
                                                            p.first - p.second.size() :
                                                            0, ' ');
                                      return blanks + p.second + " ";
                                    });
        line.for_each([&out](const auto & s)
                        {
                          out << s;
                        });
        out << '\n';
      }

    return out;
  }

  /** Format a matrix by computing column widths.

      \param[in] mat Input matrix.
      \return Matrix of padded strings.
  */
  inline DynList<DynList<std::string>>
  format_string(const DynList<DynList<std::string>> & mat)
  {
    if (mat.is_empty())
      return mat;

    const DynList<size_t> ilens = rep<size_t>(mat.nth(0).size(), 0);
    const DynList<size_t> maxs =
        mat.foldl(ilens, [](const DynList<size_t> & acu,
                            const DynList<std::string> & l)
                    {
                      return zip(acu, l).
                          maps<size_t>([](const auto & p)
                                         {
                                           return std::max(p.first, p.second.size());
                                         });
                    });

    return format_string(maxs, mat);
  }

  /** Produce a CSV-like matrix (commas added to all but last element in each row).

      \param[in] mat Input matrix.
      \return Matrix where each row has `","` appended to non-last cells.
  */
  inline DynList<DynList<std::string>>
  format_string_csv(const DynList<DynList<std::string>> & mat)
  {
    DynList<DynList<std::string>> ret;
    for (auto row_it = mat.get_it(); row_it.has_curr(); row_it.next_ne())
      {
        const DynList<std::string> & curr_row = row_it.get_curr();
        const std::string & last = curr_row.get_last();
        DynList<std::string> row;
        for (auto it = curr_row.get_it(); it.has_curr(); it.next_ne())
          {
            const std::string & s = it.get_curr();
            if (&s == &last)
              row.append(s);
            else
              row.append(s + ",");
          }
        ret.append(row);
      }

    return ret;
  }

  /** Convert a matrix of strings to a single std::string.

      Rows are concatenated as-is and separated by newlines.

      \param[in] mat Input matrix.
      \return String representation.
  */
  inline std::string to_string(const DynList<DynList<std::string>> & mat)
  {
    std::ostringstream s;
    mat.for_each([&s](const auto & row)
                   {
                     row.for_each([&s](const std::string & str)
                                    {
                                      s << str;
                                    });
                     s << '\n';
                   });
    return s.str();
  }

  /** Split a text into whitespace-separated words.

      \param[in] text Input text.
      \return List of words.
  */
  inline DynList<std::string> split_text_into_words(const std::string & text)
  {
    DynList<std::string> words;
    std::istringstream in(text);
    for (std::string word; in >> word;)
      words.append(word);
    return words;
  }

  /** Split a text into lines by `"\n"`.

      \param[in] text Input text.
      \return List of lines.
  */
  inline DynList<std::string> split_text_into_lines(const std::string & text)
  {
    return split_to_list(text, "\n");
  }

  /** Join lines into a single std::string separated by newlines.

      \param[in] lines Lines to join.
      \return Text with `"\n"` separators.
  */
  inline std::string to_string(const DynList<std::string> & lines)
  {
    std::ostringstream s;
    for (auto it = lines.get_it(); it.has_curr(); it.next_ne())
      {
        const auto & line = it.get_curr();
        s << line;
        if (line != lines.get_last())
          s << '\n';
      }

    return s.str();
  }

  /** Justify a single line by inserting extra spaces.

      \param[in] line Input line.
      \param[in] page_width Desired width.
      \return Justified line.
  */
  inline std::string justify_line(std::string line, const size_t page_width)
  {
    size_t pos = line.find_first_of(' ');
    if (pos != std::string::npos)
      {
        while (line.size() < page_width)
          {
            pos = line.find_first_not_of(' ', pos);
            if (pos == std::string::npos)
              break;
            line.insert(pos, " ");
            pos = line.find_first_of(' ', pos + 1);
            if (pos == std::string::npos)
              pos = line.find_first_of(' ');
          }
      }
    return line;
  }

  /** Justify a text to a target width.

      \param[in] text Input text.
      \param[in] width Target width.
      \param[in] left_margin Optional left margin (spaces).
      \return Justified text with newlines.
  */
  inline std::string justify_text(const std::string & text,
                             const size_t width,
                             const size_t left_margin = 0)
  {
    auto words = split_text_into_words(text);

    std::ostringstream s;
    std::string line;
    for (const std::string & word: words)
      if (line.size() + word.size() + 1 > width)
        {
          // next word doesn't fit into the line
          if (not line.empty())
            {
              s << std::string(left_margin, ' ') << justify_line(line, width) << '\n';
              line.clear();
            }
          line = word;
        }
      else
        {
          if (not line.empty())
            line.append(" ");
          line.append(word);
        }

    s << std::string(left_margin, ' ') << line;

    return s.str();
  }

  /** Justify all lines except the first one.

      \param[in] text Input text.
      \param[in] width Target width.
      \param[in] left_margin Left margin for lines after the first.
      \return Formatted text.
  */
  inline std::string justify_line_except_first(const std::string & text,
                                          const size_t width,
                                          const size_t left_margin = 0)
  {
    const auto formatted = justify_text(text, width);
    auto lines = split_text_into_lines(formatted);
    std::ostringstream s;
    s << lines.remove_first() << '\n';
    s << justify_text(to_string(lines), width, left_margin);
    return s.str();
  }

  /** Align text to the left by wrapping lines at `page_width`.

      \param[in] text Input text.
      \param[in] page_width Maximum line width.
      \param[in] left_margin Optional left margin (spaces).
      \return Wrapped text.
  */
  inline std::string align_text_to_left(const std::string & text,
                                   const size_t page_width,
                                   const size_t left_margin = 0)
  {
    auto words = split_text_into_words(text);

    const std::string margin(left_margin, ' ');
    std::ostringstream s;
    std::string line;
    for (const std::string & word: words)
      if (line.size() + word.size() + 1 > page_width)
        {
          // next word doesn't fit into the line
          if (not line.empty())
            {
              s << margin << line << '\n';
              line.clear();
            }
          line = word;
        }
      else
        {
          if (not line.empty())
            line.append(" ");
          line.append(word);
        }

    s << margin << line;

    return s.str();
  }

  /** Align all lines except the first one.

      \param[in] text Input text.
      \param[in] width Maximum line width.
      \param[in] left_margin Left margin for lines after the first.
      \return Wrapped text.
  */
  inline std::string align_text_to_left_except_first(const std::string & text,
                                                const size_t width,
                                                const size_t left_margin = 0)
  {
    const auto formatted = align_text_to_left(text, width);
    auto lines = split_text_into_lines(formatted);
    std::ostringstream s;
    s << lines.remove_first() << '\n';
    s << align_text_to_left(to_string(lines), width, left_margin);
    return s.str();
  }

  /** Indent every line in a multi-line std::string by `n` spaces.

      \param[in] str Input text.
      \param[in] n Number of spaces.
      \return Indented text.
  */
  inline std::string shift_lines_to_left(const std::string & str, const size_t n)
  {
    std::ostringstream s;
    const std::string fill(n, ' ');
    const auto lines = split_to_list(str, "\n");
    for (auto it = lines.get_it(); it.has_curr(); it.next_ne())
      {
        const auto & line = it.get_curr();
        s << fill << line;
        if (line != lines.get_last())
          s << '\n';
      }
    return s.str();
  }

  /** Base case for `build_pars_list(std::string&, ...)`.

      \param[in,out] Unused; terminates recursion.
  */
  inline void build_pars_list(std::string &)
  {}

  template <typename T>
    requires requires(std::ostream & out, const T & v)
      {
        out << v;
      }

  /** Append a single parameter to a comma-separated parameter list.

      \tparam T Type streamable to `std::ostream`.
      \param[in,out] str Accumulator.
      \param[in] item Value to append.
  */
  inline void build_pars_list(std::string & str, const T & item)
  {
    std::ostringstream s;
    if (not str.empty())
      s << ", ";
    s << item;
    str.append(s.str());
  }

  /** Append multiple parameters to a comma-separated parameter list.

      \tparam T First value type.
      \tparam Args Remaining value types.
      \param[in,out] str Accumulator.
      \param[in] item First value.
      \param[in] args Remaining values.
  */
  template <class T, class... Args>
  inline void build_pars_list(std::string & str, const T & item, Args... args)
  {
    build_pars_list(str, item);
    build_pars_list(str, args...);
  }

  /** Build a sringficated parameters' lists

   */
  template <class First, class... Rest>
    requires (not requires(const std::remove_reference_t<First> & c)
      {
        c.is_empty();
        c.get_last();
        c.get_it();
      })
  /** Build a comma-separated list from variadic parameters.

      This overload is intended for non-container arguments.

      \tparam First First value type.
      \tparam Rest Remaining value types.
      \param[in] first First value.
      \param[in] rest Remaining values.
      \return Comma-separated list.
  */
  inline std::string build_pars_list(const First & first, const Rest &... rest)
  {
    std::string ret;
    build_pars_list(ret, first, rest...);
    return ret;
  }

  template <class C>
    requires requires(const C & c)
      {
        c.is_empty();
        c.get_last();
        c.get_it();
      }
  /** Build a separated list from an Aleph-style container.

      \tparam C Container type.
      \param[in] c Container.
      \param[in] sep Separator (default: comma).
      \return Joined elements.
  */
  inline std::string build_pars_list(const C & c, const std::string & sep = ",")
  {
    if (c.is_empty())
      return "";

    auto & last = c.get_last();
    std::ostringstream s;
    for (auto it = c.get_it(); it.has_curr(); it.next_ne())
      {
        auto & curr = it.get_curr();
        s << curr;
        if (&curr != &last)
          s << sep;
      }
    return s.str();
  }
}

# endif // AH_STRING_UTILS_H
