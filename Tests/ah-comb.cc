# include <gtest/gtest.h>

# include <ah-comb.H>
# include <ahFunctional.H>

# include <set>
# include <sstream>

using namespace Aleph;

namespace
{
std::string list_to_string(const DynList<int> & l)
{
  std::ostringstream oss;
  bool first = true;
  l.for_each([&](int v)
  {
    if (!first)
      oss << ',';
    first = false;
    oss << v;
  });
  return oss.str();
}
}

TEST(AhComb, TransposeEmpty)
{
  DynList<DynList<int>> m;
  EXPECT_TRUE(transpose(m).is_empty());

  in_place_transpose(m);
  EXPECT_TRUE(m.is_empty());
}

TEST(AhComb, TransposeRectangular)
{
  DynList<int> r1 = {1, 2, 3};
  DynList<int> r2 = {4, 5, 6};
  DynList<DynList<int>> m = {r1, r2};

  DynList<int> c1 = {1, 4};
  DynList<int> c2 = {2, 5};
  DynList<int> c3 = {3, 6};
  DynList<DynList<int>> expected = {c1, c2, c3};

  EXPECT_EQ(transpose(m), expected);

  auto m2 = m;
  in_place_transpose(m2);
  EXPECT_EQ(m2, expected);
}

TEST(AhComb, InPlaceTransposeDynArray)
{
  DynArray<DynArray<int>> m;

  DynArray<int> r1;
  r1.reserve(3);
  r1(0) = 1;
  r1(1) = 2;
  r1(2) = 3;

  DynArray<int> r2;
  r2.reserve(3);
  r2(0) = 4;
  r2(1) = 5;
  r2(2) = 6;

  m.reserve(2);
  m(0) = r1;
  m(1) = r2;

  in_place_transpose(m);

  ASSERT_EQ(m.size(), 3u);
  ASSERT_EQ(m(0).size(), 2u);
  EXPECT_EQ(m(0)(0), 1);
  EXPECT_EQ(m(0)(1), 4);
  EXPECT_EQ(m(1)(0), 2);
  EXPECT_EQ(m(1)(1), 5);
  EXPECT_EQ(m(2)(0), 3);
  EXPECT_EQ(m(2)(1), 6);
}

TEST(AhComb, TraversePermEnumeratesAll)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2 = {10, 20};
  DynList<DynList<int>> l = {l1, l2};

  std::set<std::string> perms;
  bool ok = traverse_perm(l, [&](const DynList<int> & p)
  {
    perms.insert(list_to_string(p));
    return true;
  });

  EXPECT_TRUE(ok);
  EXPECT_EQ(perms.size(), 4u);
  EXPECT_TRUE(perms.contains("1,10"));
  EXPECT_TRUE(perms.contains("2,10"));
  EXPECT_TRUE(perms.contains("1,20"));
  EXPECT_TRUE(perms.contains("2,20"));
}

TEST(AhComb, TraversePermEarlyStop)
{
  DynList<int> l1 = {1, 2, 3};
  DynList<int> l2 = {10, 20};
  DynList<DynList<int>> l = {l1, l2};

  size_t seen = 0;
  bool ok = traverse_perm(l, [&](const DynList<int> &)
  {
    ++seen;
    return seen < 3; // stop on third call
  });

  EXPECT_FALSE(ok);
  EXPECT_EQ(seen, 3u);
}

TEST(AhComb, BuildPerms)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2 = {10};
  DynList<DynList<int>> l = {l1, l2};

  auto perms = build_perms(l);
  EXPECT_EQ(perms.size(), 2u);

  std::set<std::string> s;
  perms.for_each([&](const DynList<int> & p) { s.insert(list_to_string(p)); });
  EXPECT_TRUE(s.contains("1,10"));
  EXPECT_TRUE(s.contains("2,10"));
}

TEST(AhComb, BuildCombsDeduplicates)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2 = {2, 1};
  DynList<DynList<int>> l = {l1, l2};

  auto combs = build_combs(l);

  // Possible permutations are (1,2), (1,1), (2,2), (2,1).
  // After sorting: (1,2), (1,1), (2,2), (1,2) => unique: 3
  EXPECT_EQ(combs.size(), 3u);

  std::set<std::string> s;
  combs.for_each([&](const DynList<int> & c) { s.insert(list_to_string(c)); });
  EXPECT_TRUE(s.contains("1,1"));
  EXPECT_TRUE(s.contains("1,2"));
  EXPECT_TRUE(s.contains("2,2"));
}

TEST(AhComb, FoldPermAccumulates)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2 = {10, 20};
  DynList<DynList<int>> l = {l1, l2};

  // Sum of first elements across permutations:
  // (1,10), (2,10), (1,20), (2,20) => sum = 6
  auto sum = fold_perm<size_t, int>(0u, l, [](size_t a, const DynList<int> & p)
  {
    return a + p.get_first();
  });

  EXPECT_EQ(sum, 6u);
}

TEST(AhComb, TraversePermEmptyInputCallsOnce)
{
  DynList<DynList<int>> l;
  size_t calls = 0;
  bool ok = traverse_perm(l, [&](const DynList<int> & p)
  {
    ++calls;
    EXPECT_TRUE(p.is_empty());
    return true;
  });
  EXPECT_TRUE(ok);
  EXPECT_EQ(calls, 1u);
}

TEST(AhComb, BuildPermsEmptyInputReturnsOneEmptyPermutation)
{
  DynList<DynList<int>> l;
  auto perms = build_perms(l);
  ASSERT_EQ(perms.size(), 1u);
  EXPECT_TRUE(perms.get_first().is_empty());
}

TEST(AhComb, BuildCombsEmptyInputReturnsOneEmptyCombination)
{
  DynList<DynList<int>> l;
  auto combs = build_combs(l);
  ASSERT_EQ(combs.size(), 1u);
  EXPECT_TRUE(combs.get_first().is_empty());
}

TEST(AhComb, TransposeWithZeroColumns)
{
  DynList<int> r1;
  DynList<int> r2;
  DynList<DynList<int>> m = {r1, r2};

  EXPECT_TRUE(transpose(m).is_empty());

  auto m2 = m;
  in_place_transpose(m2);
  EXPECT_TRUE(m2.is_empty());
}

#ifndef NDEBUG
TEST(AhComb, NonRectangularTransposeDies)
{
  DynList<int> r1 = {1, 2};
  DynList<int> r2 = {3};
  DynList<DynList<int>> m = {r1, r2};

  EXPECT_DEATH((void)transpose(m), "");

  auto m2 = m;
  EXPECT_DEATH(in_place_transpose(m2), "");
}
#endif
