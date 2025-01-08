//
// Created by lrleon on 1/6/25.
//

# include <gtest/gtest.h>

# include <ahSort.H>

using namespace Aleph;
using namespace std;
using namespace testing;

struct SimpleArray : public Test
{
  vector<int> a = { 3, 2, 1, 0, 4, 5, 6, 7, 8, 9 };
  Array<int> b = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
};

TEST_F(SimpleArray, sort)
{
  in_place_multisort<std::less<int>>(a, b);
}