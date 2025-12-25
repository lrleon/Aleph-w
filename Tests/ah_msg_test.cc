#include <gtest/gtest.h>

#include <ah-msg.H>

#include <string>

namespace
{

struct NonStreamable
{
  int value;
};

} // namespace

TEST(AhMsgTest, BuildStringConcatenatesArguments)
{
  const std::string msg = Aleph::build_string("Value=", 42, ", flag=", true);
  EXPECT_EQ(msg, "Value=42, flag=1");

  const std::string alias = Aleph::build_error_msg("[", msg, "]");
  EXPECT_EQ(alias, "[Value=42, flag=1]");
}

TEST(AhMsgTest, BuildStringSupportsMovableTypes)
{
  std::string dynamic = "dynamic";
  const std::string msg = Aleph::build_string("copy-", dynamic, "-move-", std::string("temp"));

  EXPECT_EQ(msg, "copy-dynamic-move-temp");
  EXPECT_EQ(dynamic, "dynamic"); // ensure lvalue untouched
}

TEST(AhMsgTest, BuildStringRejectsNonStreamableAtCompileTime)
{
  static_assert(!Aleph::detail::is_stream_insertable<NonStreamable>::value,
                "NonStreamable should not be considered stream insertable");
  static_assert(!Aleph::detail::are_stream_insertable_v<NonStreamable>,
                "Variadic helper must reject NonStreamable");
}
