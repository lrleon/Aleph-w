
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

# ifndef AH_DATE_H
# define AH_DATE_H

/** @file ah-date.H
 *  @brief Lightweight helpers for validating and formatting civil dates.
 *
 *  These functions wrap POSIX @c strptime/@c mktime functionality with
 *  Aleph-w error handling in order to surface invalid inputs early and to
 *  provide a consistent interface for common conversions involving @c time_t.
 *
 *  @ingroup Utilities
 *  @author Leandro Rabindranath León
 */

# include <array>
# include <ctime>
# include <limits>
# include <string>

# include <ah-errors.H>

namespace Aleph
{
  namespace detail
  {
    inline constexpr size_t tm_epoch_year = 1900u;
    inline constexpr size_t max_supported_year =
      static_cast<size_t>(std::numeric_limits<int>::max()) + tm_epoch_year;
    inline constexpr time_t seconds_per_day =
      static_cast<time_t>(24 * 60 * 60);
  } // namespace detail

  /** @brief Return true if the provided year is a leap year. */
  [[nodiscard]] inline bool is_leap_year(const size_t yy) noexcept
  {
    return (yy % 4 == 0 and yy % 100 != 0) or (yy % 400 == 0);
  }

  /** @brief Check whether the month number is within the [1, 12] range. */
  [[nodiscard]] inline bool valid_month(const size_t mm) noexcept
  {
    return mm >= 1 and mm <= 12;
  }

  /** @brief Check whether a given day exists for the supplied month/year. */
  [[nodiscard]] inline bool valid_day(const size_t yy, const size_t mm, const size_t dd) noexcept
  {
    if (not valid_month(mm) or dd == 0)
      return false;

    static constexpr std::array<size_t, 12> month_lengths =
      {31u, 28u, 31u, 30u, 31u, 30u, 31u, 31u, 30u, 31u, 30u, 31u};

    const size_t base = month_lengths[mm - 1];
    const size_t extra = (mm == 2 and is_leap_year(yy)) ? 1u : 0u;

    return dd <= base + extra;
  }

  /** @brief Convert a (day, month, year) triple to a POSIX @c time_t value. */
  inline time_t to_time_t(const size_t dd, const size_t mm, const size_t yy)
  {
    ah_domain_error_if(yy < detail::tm_epoch_year or
                       yy > detail::max_supported_year)
      << "Year " << yy << " is outside the supported range ["
      << detail::tm_epoch_year << ", " << detail::max_supported_year << ']';

    ah_domain_error_if(not valid_day(yy, mm, dd))
      << "Invalid date components"
      << " (dd=" << dd << ", mm=" << mm << ", yy=" << yy << ')';

    std::tm tm{};
    tm.tm_mday = static_cast<int>(dd);
    tm.tm_mon = static_cast<int>(mm) - 1;
    tm.tm_year = static_cast<int>(yy) - 1900;
    tm.tm_isdst = -1;

    return std::mktime(&tm);
  }

  /** @brief Convert a formatted string to @c time_t using the provided format. */
  inline time_t to_time_t(const std::string & s, const std::string & format)
  {
    std::tm tm{};
    const char *res = ::strptime(s.c_str(), format.c_str(), &tm);
    ah_domain_error_if(res == nullptr or *res != '\0')
      << "Input string '" << s << "' does not match format '" << format << '\'';

    tm.tm_isdst = -1;
    return std::mktime(&tm);
  }

  /** @brief Convenience overload for ISO strings (YYYY-MM-DD HH:MM:SS). */
  inline time_t to_time_t(const std::string & s)
  {
    return to_time_t(s, "%Y-%m-%d %H:%M:%S");
  }

  /** @brief Return the number of whole days represented by @p t. */
  inline size_t to_days(const time_t t)
  {
    ah_domain_error_if(t < 0)
      << "to_days() expects non-negative timestamps";

    return static_cast<size_t>(t / detail::seconds_per_day);
  }

  /** @brief Format a @c time_t value into a string using @p format. */
  inline std::string to_string(const time_t t, const std::string & format)
  {
    const std::tm *tm_ptr = std::localtime(&t);
    ah_runtime_error_if(tm_ptr == nullptr) << "localtime() failed";

    std::tm tm = *tm_ptr;
    char buff[128]{};
    const size_t written = std::strftime(buff, sizeof(buff), format.c_str(), &tm);
    ah_range_error_if(written == 0)
      << "Unable to format time using pattern '" << format << "'";

    return std::string(buff, written);
  }

}

# endif
