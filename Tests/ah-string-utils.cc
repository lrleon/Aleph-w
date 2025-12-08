//
// Created by lrleon on 23/04/24.
//
# include <gtest/gtest.h>

# include <ah-string-utils.H>

using namespace std;
using namespace testing;
using namespace Aleph;

TEST(util, concat)
{
  string s1 = "hello";
  string s2 = "world";
  string blank = " ";
  string s3 = "!";
  string s = concat(s1, blank, s2, s3);
  ASSERT_EQ(s, "hello world!");
}