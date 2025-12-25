#include <gtest/gtest.h>

#include <ah-date.H>

#include <array>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

std::tm checked_localtime(std::time_t value)
{
  std::tm *tm_ptr = std::localtime(&value);
  if (tm_ptr == nullptr)
    throw std::runtime_error("localtime failed during test");
  return *tm_ptr;
}

} // namespace

TEST(AhDateTest, LeapYearDetection)
{
  EXPECT_TRUE(Aleph::is_leap_year(2000));
  EXPECT_FALSE(Aleph::is_leap_year(1900));
  EXPECT_TRUE(Aleph::is_leap_year(2024));
  EXPECT_FALSE(Aleph::is_leap_year(2025));
}

TEST(AhDateTest, DayValidationHandlesMonthLengths)
{
  EXPECT_TRUE(Aleph::valid_day(2024, 2, 29));
  EXPECT_FALSE(Aleph::valid_day(2023, 2, 29));
  EXPECT_FALSE(Aleph::valid_day(2024, 0, 10));
  EXPECT_FALSE(Aleph::valid_day(2024, 4, 31));
  EXPECT_TRUE(Aleph::valid_day(2024, 4, 30));
}

TEST(AhDateTest, ToTimeTTripleRoundTrip)
{
  const std::time_t ts = Aleph::to_time_t(25, 12, 2022);
  const std::tm tm = checked_localtime(ts);

  EXPECT_EQ(tm.tm_mday, 25);
  EXPECT_EQ(tm.tm_mon + 1, 12);
  EXPECT_EQ(tm.tm_year + 1900, 2022);
}

TEST(AhDateTest, ToTimeTTripleRejectsInvalidDates)
{
  EXPECT_THROW(Aleph::to_time_t(31, 11, 2022), std::domain_error);
  EXPECT_THROW(Aleph::to_time_t(29, 2, 1800), std::domain_error);
}

TEST(AhDateTest, ToTimeTFromStringValidatesFormat)
{
  const std::time_t ts = Aleph::to_time_t("2024-05-10 12:34:56");
  const std::tm tm = checked_localtime(ts);

  EXPECT_EQ(tm.tm_year + 1900, 2024);
  EXPECT_EQ(tm.tm_mon + 1, 5);
  EXPECT_EQ(tm.tm_mday, 10);
  EXPECT_EQ(tm.tm_hour, 12);
  EXPECT_EQ(tm.tm_min, 34);
  EXPECT_EQ(tm.tm_sec, 56);

  EXPECT_THROW(Aleph::to_time_t("2024-05-10T12:34:56", "%Y-%m-%d %H:%M:%S"),
               std::domain_error);
}

TEST(AhDateTest, ToDaysRequiresNonNegativeTimestamps)
{
  constexpr std::time_t horizon = static_cast<std::time_t>(5 * 24 * 60 * 60);
  EXPECT_EQ(Aleph::to_days(horizon), 5u);
  EXPECT_THROW(Aleph::to_days(static_cast<std::time_t>(-1)), std::domain_error);
}

TEST(AhDateTest, ToStringFormatsAndDetectsOverflow)
{
  const std::time_t ts = Aleph::to_time_t("2025-01-01 00:00:00");
  EXPECT_EQ(Aleph::to_string(ts, "%Y-%m-%d"), "2025-01-01");

  const std::string oversized_format(256, 'X');
  EXPECT_THROW(Aleph::to_string(ts, oversized_format), std::range_error);
}
