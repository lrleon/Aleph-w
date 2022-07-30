
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
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

# include <gmock/gmock.h>

# include <iostream>

# include <array_utils.H>
# include <ah-string-utils.H>
# include <htlist.H>

using namespace std;
using namespace testing;
using namespace Aleph;

struct SimpleArray : public Test
{
  size_t n = 17;
  int * a = nullptr;
  SimpleArray() : a(new int[n])
  {
    for (size_t i = 0; i < n; ++i)
      a[i] = i;
  }
  ~SimpleArray() { delete [] a; }
  void print() const noexcept
  {
    cout << "a =";
    for (size_t i = 0; i < n; ++i)
      cout << " " << a[i];
    cout << endl
	 << endl;
  }
};

TEST_F(SimpleArray, simple_open_gap_by_copy_left)
{
  EXPECT_GT(n, 0);
  open_gap(a, n, 0, 1);

  EXPECT_EQ(a[0], 0);

  for (size_t i = 1; i < n; ++i)
    EXPECT_EQ(a[i], i - 1);
}

TEST_F(SimpleArray, simple_open_gap_by_copy_right)
{
  EXPECT_GT(n, 0);
  open_gap(a, n, n - 2, 1);

  for (size_t i = 0; i < n - 2; ++i)
    EXPECT_EQ(a[i], i);

  EXPECT_EQ(a[n - 2], n - 2);

  for (size_t i = n - 1; i < n; ++i)
    EXPECT_EQ(a[i], i - 1);
}

TEST_F(SimpleArray, n_open_gap_by_copy_left)
{
  EXPECT_GT(n, 0);
  open_gap(a, n, 0, 3);
  for (size_t i = 0; i < 3; ++i)
    EXPECT_EQ(a[i], i);

  for (size_t i = 3; i < n; ++i)
    EXPECT_EQ(a[i], i - 3);
}

TEST_F(SimpleArray, open_gap_out_of_range)
{
  EXPECT_GT(n, 0);
  EXPECT_THROW(open_gap(a, n, 0, n + 1), out_of_range);
  EXPECT_THROW(open_gap(a, n, 0, n + 2), out_of_range);
  EXPECT_NO_THROW(open_gap(a, n, 0, n));
  
  EXPECT_NO_THROW(open_gap(a, n, n - 1, 1));
  EXPECT_NO_THROW(open_gap(a, n, n - 1, 0));

  EXPECT_THROW(open_gap(a, n, n, 0), out_of_range);
  EXPECT_NO_THROW(open_gap(a, n, n - 1, 0));

  EXPECT_NO_THROW(open_gap(a, n, n - 3, 3));
  EXPECT_NO_THROW(open_gap(a, n, n - 3 - 1, 3));
  EXPECT_NO_THROW(open_gap(a, n, n - 3, 2));
}

TEST_F(SimpleArray, close_gap_by_copy_left)
{
  EXPECT_GT(n, 0);
  close_gap(a, n, 0, 1);
  for (size_t i = 0; i < n - 1; ++i)
    EXPECT_EQ(a[i], i + 1);
  EXPECT_THAT(a[n-1], n - 1);
}

TEST_F(SimpleArray, close_gap_by_copy_right)
{
  EXPECT_GT(n, 0);
  close_gap(a, n, n - 2, 1);
  for (size_t i = 0; i < n - 2; ++i)
    EXPECT_EQ(a[i], i);
  EXPECT_THAT(a[n - 1], n - 1);
  EXPECT_THAT(a[n - 1], n - 1);
}

TEST_F(SimpleArray, n_close_gap_by_copy_left)
{
  EXPECT_GT(n, 0);
  close_gap(a, n, 0, 3);
  for (size_t i = 0; i < n - 3; ++i)
    EXPECT_EQ(a[i], i + 3);
  for (size_t i = n - 3; i < n; ++i)
    EXPECT_EQ(a[i], i);
}

TEST_F(SimpleArray,  n_close_gap_by_copy_right)
{
  EXPECT_GT(n, 0);
  close_gap(a, n, n - 4, 2);
  for (size_t i = 0; i < n - 4; ++i)
    EXPECT_EQ(a[i], i);
  for (size_t i = n - 4; i < n - 2; ++i)
    EXPECT_EQ(a[i], i + 2);
  for (size_t i = n - 2; i < n; ++i)
    EXPECT_EQ(a[i], i);
}

TEST_F(SimpleArray, close_gap_out_of_range)
{
  EXPECT_GT(n, 0);
  EXPECT_THROW(close_gap(a, n, n - 1, 2), out_of_range);
  EXPECT_NO_THROW(close_gap(a, n, n - 1, 0));

  EXPECT_THROW(close_gap(a, n, 0, n + 1), out_of_range);
  EXPECT_NO_THROW(close_gap(a, n, 0, n - 1));
}

TEST_F(SimpleArray, reverse)
{
  EXPECT_GT(n, 0);
  reverse(a, n);
  size_t k = 0;
  for (size_t i = 0; i < n; ++i, ++k)
    EXPECT_EQ(a[i], n - i - 1);
  EXPECT_EQ(k, n);
}

TEST_F(SimpleArray, rotate_left)
{
  EXPECT_GT(n, 0);

  rotate_left(a, n, 3);
  for (size_t i = 0; i < n; ++i)
    EXPECT_EQ(a[i], (i + 3) % n);
}

TEST_F(SimpleArray, rotate_right)
{
  EXPECT_GT(n, 0);

  rotate_right(a, n, 3);
  for (size_t i = 0; i < n; ++i)
    EXPECT_EQ(a[i], (n + i - 3) % n);
}

struct ComplexArray : public Test
{
  size_t n = 19;
  DynList<int> * a = nullptr;
  ComplexArray() : a(new DynList<int>[n])
  {
    for (size_t i = 0; i < n; ++i)
      {
	a[i].append(i);
	a[i].append({1, 2, 3});
      }
  }
  ~ComplexArray() { delete [] a; }
  void print() const
  {
    cout << "a =";
    for (size_t i = 0; i < n; ++i)
      cout << "(" << join(a[i], ", ") << ")";
    cout << endl
	 << endl;
  }
};

TEST_F(ComplexArray, simple_open_gap_by_copy_left)
{
  EXPECT_GT(n, 0);
  open_gap(a, n, 0, 1);
  EXPECT_EQ(a[0].get_first(), n - 1);
  for (size_t i = 1; i < n; ++i)
    {
      EXPECT_EQ(a[i].get_first(), i - 1);
    }
}

TEST_F(ComplexArray, simple_open_gap_by_copy_right)
{
  EXPECT_GT(n, 0);
  open_gap(a, n, n - 2, 1);
  for (size_t i = 0; i < n - 2; ++i)
    EXPECT_EQ(a[i].get_first(), i);

  EXPECT_EQ(a[n - 2].get_first(), n - 1);

  for (size_t i = n - 1; i < n; ++i)
    EXPECT_EQ(a[i].get_first(), i - 1);
}

TEST_F(ComplexArray, n_open_gap_by_copy_left)
{
  EXPECT_GT(n, 0);
  open_gap(a, n, 0, 3);
  for (size_t i = 3; i < n; ++i)
    EXPECT_EQ(a[i].get_first(), i - 3);
}

TEST_F(ComplexArray, close_gap_by_copy_left)
{
  EXPECT_GT(n, 0);
  close_gap(a, n, 0, 1);
  for (size_t i = 0; i < n - 1; ++i)
    EXPECT_EQ(a[i].get_first(), i + 1);
}

TEST_F(ComplexArray, close_gap_by_copy_right)
{
  EXPECT_GT(n, 0);
  close_gap(a, n, n - 2, 1);
  for (size_t i = 0; i < n - 2; ++i)
    EXPECT_EQ(a[i].get_first(), i);
}

TEST_F(ComplexArray, n_close_gap_by_copy_left)
{
  EXPECT_GT(n, 0);
  close_gap(a, n, 0, 3);
  for (size_t i = 0; i < n - 3; ++i)
    EXPECT_EQ(a[i].get_first(), i + 3);
}

TEST_F(ComplexArray, n_close_gap_by_copy_right)
{
  EXPECT_GT(n, 0);
  close_gap(a, n, n - 4, 2);
  for (size_t i = 0; i < n - 4; ++i)
    EXPECT_EQ(a[i].get_first(), i);
  for (size_t i = n - 4; i < n - 2; ++i)
    EXPECT_EQ(a[i].get_first(), i + 2);
  for (size_t i = n - 2; i < n; ++i)
    EXPECT_EQ(a[i].get_first(), i - 2);
}

TEST_F(ComplexArray, open_gap_out_of_range)
{
  EXPECT_GT(n, 0);
  EXPECT_THROW(open_gap(a, n, 0, n + 1), out_of_range);
  EXPECT_THROW(open_gap(a, n, 0, n + 2), out_of_range);
  EXPECT_NO_THROW(open_gap(a, n, 0, n));
  
  EXPECT_NO_THROW(open_gap(a, n, n - 1, 1));
  EXPECT_NO_THROW(open_gap(a, n, n - 1, 0));

  EXPECT_THROW(open_gap(a, n, n, 0), out_of_range);
  EXPECT_NO_THROW(open_gap(a, n, n - 1, 0));

  EXPECT_NO_THROW(open_gap(a, n, n - 3, 3));
  EXPECT_NO_THROW(open_gap(a, n, n - 3 - 1, 3));
  EXPECT_NO_THROW(open_gap(a, n, n - 3, 2));
}

TEST_F(ComplexArray, reverse)
{
  EXPECT_GT(n, 0);
  reverse(a, n);
  size_t k = 0;
  for (size_t i = 0; i < n; ++i, ++k)
    EXPECT_EQ(a[i].get_first(), n - i - 1);
  EXPECT_EQ(k, n);
}

TEST_F(ComplexArray, rotate_left)
{
  EXPECT_GT(n, 0);

  rotate_left(a, n, 3);
  for (size_t i = 0; i < n; ++i)
    EXPECT_EQ(a[i].get_first(), (i + 3) % n);
}

TEST_F(ComplexArray, rotate_right)
{
  EXPECT_GT(n, 0);

  rotate_right(a, n, 3);
  for (size_t i = 0; i < n; ++i)
    EXPECT_EQ(a[i].get_first(), (n + i - 3) % n);
}
