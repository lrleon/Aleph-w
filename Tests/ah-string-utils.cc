//
// Created by lrleon on 23/04/24.
//
# include <gtest/gtest.h>
# include <ah-string-utils.H>

TEST(util, concat)
{
  string s1 = "hello";
  string s2 = "world";
  string s3 = "!";
  string s = concat(s1, s2, s3);
  ASSERT_EQ(s, "hello world!");
}