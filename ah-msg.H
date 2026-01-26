
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


/** @file ah-msg.H
 *  @brief Message builder for constructing formatted strings.
 *
 *  Provides MSG class for building messages using stream-style
 *  insertion operators. Useful for constructing error messages
 *  and log entries with mixed types.
 *
 *  @ingroup Utilities
 *  @author Leandro Rabindranath León
 */

# ifndef AH_MSG_H
# define AH_MSG_H

# include <sstream>
# include <string>
# include <type_traits>
# include <utility>

namespace Aleph
{
  namespace detail
  {
    inline void build_msg_impl(std::ostringstream &) {}

    template <typename T, typename... Args>
    inline void build_msg_impl(std::ostringstream & stream, T && value, Args &&... rest)
    {
      stream << std::forward<T>(value);
      build_msg_impl(stream, std::forward<Args>(rest)...);
    }

    template <typename T, typename = void>
    struct is_stream_insertable : std::false_type
    {};

    template <typename T>
    struct is_stream_insertable<T,
      std::void_t<decltype(std::declval<std::ostringstream &>()
                           << std::declval<T>())>> : std::true_type
    {};

    template <typename... Args>
    inline constexpr bool are_stream_insertable_v =
      (is_stream_insertable<std::decay_t<Args>>::value && ...);
  } // namespace detail

  template <typename T, typename = void>
  struct is_stream_insertable : detail::is_stream_insertable<T>
  {};

  template <typename... Args>
  inline constexpr bool are_stream_insertable_v =
    detail::are_stream_insertable_v<Args...>;

  /** @brief Concatenate arbitrary streamable arguments into a string.
   *
   *  This helper is primarily used for building exception messages and
   *  logging strings without incurring temporary string allocations for
   *  each component.
   *
   *  @tparam Args Variadic pack of objects insertable into @c std::ostream.
   *  @param args Values to append in order.
   *  @return The concatenated string.
   */
  template <typename... Args>
  inline std::string build_string(Args &&... args)
  {
    static_assert(detail::are_stream_insertable_v<Args...>,
                  "build_string arguments must be stream-insertable");
    std::ostringstream stream;
    detail::build_msg_impl(stream, std::forward<Args>(args)...);
    return stream.str();
  }

  /** @brief Alias kept for backwards compatibility with historic API. */
  template <typename... Args>
  inline std::string build_error_msg(Args &&... args)
  {
    return build_string(std::forward<Args>(args)...);
  }

} // namespace Aleph

# endif
