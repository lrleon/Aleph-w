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
# include <gtest/gtest.h>

# include <ahSort.H>
# include <ah-string-utils.H>
# include <al-vector.H>

using namespace std;
using namespace testing;

struct SmallDomain : public testing::Test
{
  AlDomain<char> chars = { 'a', 'b', 'c', 'd', 'e'};
  using VType = Vector<char, int>;
};

TEST_F(SmallDomain, basic)
{
  Vector<char, int> v1(chars);
  Vector<char, int> v2(chars, { 0, 1, 2, 3, 4 });
  auto v3 = v2;

  EXPECT_TRUE(v1.get_domain().keys().
	      all([&v1] (char c) { return v1.search_entry(c) == nullptr; }));

  EXPECT_TRUE(eq(sort(v1.get_domain().to_list()),
		 build_dynlist<char>('a', 'b', 'c', 'd', 'e')));
  EXPECT_EQ(v2, v3);

  auto v4 = v1;
  v4 += v2;

  EXPECT_EQ(v2, v4);

  EXPECT_EQ(v2 + v3, 2*v2);

  Vector<char, int> zero(chars);

  EXPECT_EQ(v2 - v3, zero);

  Vector<char, int> v5(chars, { 1, 2, 3, 4, 5 });
  EXPECT_TRUE(v5.get_domain().keys().
	      all([&v5] (char c) { return v5.search_entry(c); }));
}

TEST_F(SmallDomain, validations)
{
  EXPECT_THROW(VType(chars, { 1, 2, 3, 4 }), length_error);
  VType v = { chars, { 1, 2, 3, 4, 5 } };

  EXPECT_NO_THROW(VType v1 = { chars });

  AlDomain<char> auxd = { '1', '2', '3', '4' };
  VType vaux = { auxd };

  EXPECT_THROW(vaux = v, domain_error);
  EXPECT_THROW(vaux = VType(chars), domain_error);
  EXPECT_NO_THROW(vaux = VType(auxd));

  VType vaux1 = { auxd };
  EXPECT_NO_THROW(vaux = vaux1);
}

TEST(Vector, big)
{
  constexpr size_t N = 10000;
  const AlDomain<int> r = range<int>(N);
  const DynList<int> & D = r.keys();
  const Vector<int> zero(r);

  Vector<int> odd = { r, D.maps<double>([] (auto i)
    { return (i % 2) == 0 ? 0 : i; }) };
  Vector<int> even = { r, D.maps<double>([] (auto i)
    { return (i % 2) == 0 ? i : 0; }) };
  Vector<int> full = { r, range<double>(N) };
  Vector<int> I = { r, rep<double>(N, 1) };

  EXPECT_EQ(odd + even, full);
  EXPECT_EQ(odd*even, 0);

  EXPECT_EQ(I*I, N);
  EXPECT_EQ(full*I, N*(N - 1)/2);
  EXPECT_EQ(even*I, N*(N-2)/4);
  EXPECT_EQ(odd*I, N*N/4);
}
