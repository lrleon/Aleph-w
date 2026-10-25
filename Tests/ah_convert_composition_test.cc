/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

/**
 * @file ah_convert_composition_test.cc
 * @brief ah-convert.H and ah-stl-utils.H in the same program.
 *
 * Both headers defined to_vector(), vector_to_DynList() and
 * vector_to_DynArray(), so a translation unit could not include the two.
 * This one includes ah-convert.H first; ah_convert_composition_other.cc, the
 * other translation unit of this program, includes ah-stl-utils.H first, and
 * both instantiate the same conversions.
 */

#include <ah-convert.H>
#include <ah-stl-utils.H>

#include <gtest/gtest.h>

#include <vector>

// Defined in ah_convert_composition_other.cc
std::vector<int> other_unit_to_vector(const Aleph::DynList<int> & l);
Aleph::DynList<int> other_unit_vector_to_DynList(const std::vector<int> & v);
Aleph::DynArray<int> other_unit_vector_to_DynArray(const std::vector<int> & v);

using namespace Aleph;

namespace
{
  template <class C>
  std::vector<int> items(const C & c)
  {
    std::vector<int> ret;
    for (auto it = c.get_it(); it.has_curr(); it.next_ne())
      ret.push_back(it.get_curr());
    return ret;
  }

  // Only get_it(), which ah-convert.H accepted
  struct Only_Aleph_Iterator
  {
    using Item_Type = int;

    struct Iterator
    {
      const std::vector<int> * v;
      size_t i = 0;
      bool has_curr() const noexcept { return i < v->size(); }
      void next_ne() noexcept { ++i; }
      int get_curr() const { return (*v)[i]; }
    };

    std::vector<int> v;
    Iterator get_it() const { return Iterator{&v}; }
  };

  // Only begin() and end(), which ah-stl-utils.H accepted
  struct Only_Stl_Iterator
  {
    using Item_Type = int;

    std::vector<int> v;
    auto begin() const { return v.begin(); }
    auto end() const { return v.end(); }
  };
}

TEST(AhConvertComposition, BothUnitsConvertTheSameWay)
{
  const std::vector<int> v = {5, 3, 8, 1};

  DynList<int> l;
  for (int i : v)
    l.append(i);

  EXPECT_EQ(to_vector(l), v);
  EXPECT_EQ(other_unit_to_vector(l), v);

  EXPECT_EQ(items(vector_to_DynList(v)), v);
  EXPECT_EQ(items(other_unit_vector_to_DynList(v)), v);

  const DynArray<int> a = vector_to_DynArray(v);
  const DynArray<int> b = other_unit_vector_to_DynArray(v);
  ASSERT_EQ(a.size(), v.size());
  ASSERT_EQ(b.size(), v.size());
  for (size_t i = 0; i < v.size(); ++i)
    {
      EXPECT_EQ(a(i), v[i]);
      EXPECT_EQ(b(i), v[i]);
    }
}

TEST(AhConvertComposition, ToVectorTakesWhatEitherHeaderTook)
{
  EXPECT_EQ(to_vector(Only_Aleph_Iterator{{1, 2, 3}}), (std::vector<int>{1, 2, 3}));
  EXPECT_EQ(to_vector(Only_Stl_Iterator{{4, 5}}), (std::vector<int>{4, 5}));

  DynSetTree<int> s;
  for (int i : {3, 1, 2})
    s.insert(i);
  EXPECT_EQ(to_vector(s), (std::vector<int>{1, 2, 3}));

  EXPECT_TRUE(to_vector(DynList<int>()).empty());
}
