//
// Created by lrleon on 1/6/25.
//

# include <vector>
# include <sstream>
# include <array>
# include <algorithm>
# include <functional>
# include <iostream>
# include <ranges>
# include <gtest/gtest.h>

# include <ah-string-utils.H>
# include <ahSort.H>

using namespace Aleph;
using namespace std;
using namespace testing;

struct SimpleArray : public Test
{
  vector<int> a = { 3, 2, 1, 0, 4, 5, 6, 7, 8, 9 };
  Array<int> b = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  Array<string> c = { "a", "b", "c", "d", "e", "f", "g", "h", "i", "j" };
};

TEST_F(SimpleArray, sort)
{
  in_place_multisort_arrays(std::less<int>(), a, b, c);

  ASSERT_EQ(a, vector<int>({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }));

  ASSERT_EQ(to_stdvector(b), vector<int>({ 3, 2, 1, 0, 4, 5, 6, 7, 8, 9 }));

  ASSERT_EQ(to_stdvector(c),
    vector<string>({"d", "c", "b", "a", "e", "f", "g", "h", "i", "j"}));

}