
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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
