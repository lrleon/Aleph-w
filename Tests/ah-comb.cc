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

// ==== New tests for additional functions ====

TEST(AhComb, PermCountBasic)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2 = {10, 20, 30};
  DynList<DynList<int>> l = {l1, l2};

  EXPECT_EQ(perm_count(l), 6u); // 2 * 3
}

TEST(AhComb, PermCountEmpty)
{
  DynList<DynList<int>> l;
  EXPECT_EQ(perm_count(l), 1u); // One empty permutation
}

TEST(AhComb, PermCountWithEmptyList)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2; // empty
  DynList<DynList<int>> l = {l1, l2};

  EXPECT_EQ(perm_count(l), 0u); // Any empty list = 0 permutations
}

TEST(AhComb, PermCountSingleList)
{
  DynList<int> l1 = {1, 2, 3, 4, 5};
  DynList<DynList<int>> l = {l1};

  EXPECT_EQ(perm_count(l), 5u);
}

TEST(AhComb, ExistsPermFindsMatch)
{
  DynList<int> l1 = {1, 2, 3};
  DynList<int> l2 = {10, 20};
  DynList<DynList<int>> l = {l1, l2};

  bool found = exists_perm(l, [](const DynList<int> & p)
  {
    return p.get_first() == 2 && p.get_last() == 20;
  });

  EXPECT_TRUE(found);
}

TEST(AhComb, ExistsPermNoMatch)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2 = {10, 20};
  DynList<DynList<int>> l = {l1, l2};

  bool found = exists_perm(l, [](const DynList<int> & p)
  {
    return p.get_first() == 100; // Never true
  });

  EXPECT_FALSE(found);
}

TEST(AhComb, ExistsPermStopsEarly)
{
  DynList<int> l1 = {1, 2, 3};
  DynList<int> l2 = {10, 20, 30};
  DynList<DynList<int>> l = {l1, l2};

  size_t calls = 0;
  (void)exists_perm(l, [&calls](const DynList<int> & p)
  {
    ++calls;
    return p.get_first() == 1 && p.get_last() == 10; // First one matches
  });

  EXPECT_EQ(calls, 1u); // Should stop after first match
}

TEST(AhComb, AllPermAllSatisfy)
{
  DynList<int> l1 = {2, 4};
  DynList<int> l2 = {10, 20};
  DynList<DynList<int>> l = {l1, l2};

  // All sums are >= 12
  bool all = all_perm(l, [](const DynList<int> & p)
  {
    int sum = 0;
    p.for_each([&sum](int v) { sum += v; });
    return sum >= 12;
  });

  EXPECT_TRUE(all);
}

TEST(AhComb, AllPermSomeFail)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2 = {10, 20};
  DynList<DynList<int>> l = {l1, l2};

  bool all = all_perm(l, [](const DynList<int> & p)
  {
    return p.get_first() == 2; // Only half satisfy this
  });

  EXPECT_FALSE(all);
}

TEST(AhComb, AllPermStopsEarly)
{
  DynList<int> l1 = {1, 2, 3, 4};
  DynList<int> l2 = {10, 20, 30, 40};
  DynList<DynList<int>> l = {l1, l2};

  size_t calls = 0;
  (void)all_perm(l, [&calls](const DynList<int> & p)
  {
    ++calls;
    return p.get_first() != 1; // First permutation fails
  });

  EXPECT_EQ(calls, 1u); // Should stop after first failure
}

TEST(AhComb, NonePermNoneSatisfy)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2 = {10, 20};
  DynList<DynList<int>> l = {l1, l2};

  bool none = none_perm(l, [](const DynList<int> & p)
  {
    return p.get_first() == 100; // Never true
  });

  EXPECT_TRUE(none);
}

TEST(AhComb, NonePermSomeSatisfy)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2 = {10, 20};
  DynList<DynList<int>> l = {l1, l2};

  bool none = none_perm(l, [](const DynList<int> & p)
  {
    return p.get_first() == 1; // Some satisfy
  });

  EXPECT_FALSE(none);
}

TEST(AhComb, FilterPermBasic)
{
  DynList<int> l1 = {1, 2, 3};
  DynList<int> l2 = {10, 20};
  DynList<DynList<int>> l = {l1, l2};

  auto filtered = filter_perm(l, [](const DynList<int> & p)
  {
    return p.get_first() >= 2; // Only 2,10  2,20  3,10  3,20
  });

  EXPECT_EQ(filtered.size(), 4u);
}

TEST(AhComb, FilterPermEmpty)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2 = {10, 20};
  DynList<DynList<int>> l = {l1, l2};

  auto filtered = filter_perm(l, [](const DynList<int> &)
  {
    return false; // None pass
  });

  EXPECT_TRUE(filtered.is_empty());
}

TEST(AhComb, FilterPermAll)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2 = {10};
  DynList<DynList<int>> l = {l1, l2};

  auto filtered = filter_perm(l, [](const DynList<int> &)
  {
    return true; // All pass
  });

  EXPECT_EQ(filtered.size(), 2u);
}

TEST(AhComb, MapPermBasic)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2 = {10, 20};
  DynList<DynList<int>> l = {l1, l2};

  auto sums = map_perm<int>(l, [](const DynList<int> & p)
  {
    int sum = 0;
    p.for_each([&sum](int v) { sum += v; });
    return sum;
  });

  EXPECT_EQ(sums.size(), 4u);

  std::set<int> s;
  sums.for_each([&s](int v) { s.insert(v); });
  // (1,10)=11, (2,10)=12, (1,20)=21, (2,20)=22
  EXPECT_TRUE(s.contains(11));
  EXPECT_TRUE(s.contains(12));
  EXPECT_TRUE(s.contains(21));
  EXPECT_TRUE(s.contains(22));
}

TEST(AhComb, MapPermToString)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2 = {10};
  DynList<DynList<int>> l = {l1, l2};

  auto strs = map_perm<std::string>(l, [](const DynList<int> & p)
  {
    return list_to_string(p);
  });

  EXPECT_EQ(strs.size(), 2u);

  std::set<std::string> s;
  strs.for_each([&s](const std::string & v) { s.insert(v); });
  EXPECT_TRUE(s.contains("1,10"));
  EXPECT_TRUE(s.contains("2,10"));
}

TEST(AhComb, ForEachPermBasic)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2 = {10};
  DynList<DynList<int>> l = {l1, l2};

  std::set<std::string> perms;
  for_each_perm(l, [&perms](const DynList<int> & p)
  {
    perms.insert(list_to_string(p));
  });

  EXPECT_EQ(perms.size(), 2u);
  EXPECT_TRUE(perms.contains("1,10"));
  EXPECT_TRUE(perms.contains("2,10"));
}

TEST(AhComb, ForEachPermEmpty)
{
  DynList<DynList<int>> l;

  size_t calls = 0;
  for_each_perm(l, [&calls](const DynList<int> & p)
  {
    EXPECT_TRUE(p.is_empty());
    ++calls;
  });

  EXPECT_EQ(calls, 1u);
}

TEST(AhComb, TraversePermThreeLists)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2 = {10, 20};
  DynList<int> l3 = {100, 200};
  DynList<DynList<int>> l = {l1, l2, l3};

  std::set<std::string> perms;
  traverse_perm(l, [&perms](const DynList<int> & p)
  {
    perms.insert(list_to_string(p));
    return true;
  });

  EXPECT_EQ(perms.size(), 8u); // 2 * 2 * 2
}

TEST(AhComb, TraversePermSingleElement)
{
  DynList<int> l1 = {42};
  DynList<DynList<int>> l = {l1};

  std::set<std::string> perms;
  traverse_perm(l, [&perms](const DynList<int> & p)
  {
    perms.insert(list_to_string(p));
    return true;
  });

  EXPECT_EQ(perms.size(), 1u);
  EXPECT_TRUE(perms.contains("42"));
}

TEST(AhComb, TransposeSingleRow)
{
  DynList<int> r1 = {1, 2, 3};
  DynList<DynList<int>> m = {r1};

  auto t = transpose(m);
  EXPECT_EQ(t.size(), 3u); // 3 columns => 3 rows

  size_t idx = 1;
  for (auto it = t.get_it(); it.has_curr(); it.next_ne(), ++idx)
    {
      EXPECT_EQ(it.get_curr().size(), 1u);
      EXPECT_EQ(it.get_curr().get_first(), static_cast<int>(idx));
    }
}

TEST(AhComb, TransposeSingleColumn)
{
  DynList<int> r1 = {1};
  DynList<int> r2 = {2};
  DynList<int> r3 = {3};
  DynList<DynList<int>> m = {r1, r2, r3};

  auto t = transpose(m);
  EXPECT_EQ(t.size(), 1u); // 1 column => 1 row
  EXPECT_EQ(t.get_first().size(), 3u); // 3 rows => 3 columns

  DynList<int> expected = {1, 2, 3};
  EXPECT_EQ(t.get_first(), expected);
}

TEST(AhComb, InPlaceTransposeSingleRow)
{
  DynList<int> r1 = {1, 2, 3};
  DynList<DynList<int>> m = {r1};

  in_place_transpose(m);
  EXPECT_EQ(m.size(), 3u);

  auto it = m.get_it();
  EXPECT_EQ(it.get_curr().get_first(), 1);
  it.next_ne();
  EXPECT_EQ(it.get_curr().get_first(), 2);
  it.next_ne();
  EXPECT_EQ(it.get_curr().get_first(), 3);
}

TEST(AhComb, TransposeSquare)
{
  DynList<int> r1 = {1, 2};
  DynList<int> r2 = {3, 4};
  DynList<DynList<int>> m = {r1, r2};

  auto t = transpose(m);

  DynList<int> c1 = {1, 3};
  DynList<int> c2 = {2, 4};
  DynList<DynList<int>> expected = {c1, c2};

  EXPECT_EQ(t, expected);
}

TEST(AhComb, BuildCombsAllSame)
{
  DynList<int> l1 = {1};
  DynList<int> l2 = {1};
  DynList<DynList<int>> l = {l1, l2};

  auto combs = build_combs(l);
  EXPECT_EQ(combs.size(), 1u);

  DynList<int> expected = {1, 1};
  EXPECT_EQ(sort(combs.get_first()), expected);
}

TEST(AhComb, BuildCombsThreeLists)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2 = {2, 1};
  DynList<int> l3 = {1, 2};
  DynList<DynList<int>> l = {l1, l2, l3};

  auto combs = build_combs(l);

  // Possible combinations after sorting:
  // (1,1,1), (1,1,2), (1,2,2), (2,2,2)
  EXPECT_EQ(combs.size(), 4u);
}

TEST(AhComb, FoldPermMultiplyFirstElements)
{
  DynList<int> l1 = {2, 3};
  DynList<int> l2 = {10};
  DynList<DynList<int>> l = {l1, l2};

  // (2,10) and (3,10) => product of first elements = 2 * 3 = 6
  auto product = fold_perm<int, int>(1, l, [](int acu, const DynList<int> & p)
  {
    return acu * p.get_first();
  });

  EXPECT_EQ(product, 6);
}

TEST(AhComb, StressTestManyPermutations)
{
  // 3^5 = 243 permutations
  DynList<int> l1 = {1, 2, 3};
  DynList<int> l2 = {10, 20, 30};
  DynList<int> l3 = {100, 200, 300};
  DynList<int> l4 = {1000, 2000, 3000};
  DynList<int> l5 = {10000, 20000, 30000};
  DynList<DynList<int>> l = {l1, l2, l3, l4, l5};

  EXPECT_EQ(perm_count(l), 243u);

  size_t count = 0;
  traverse_perm(l, [&count](const DynList<int> & p)
  {
    EXPECT_EQ(p.size(), 5u);
    ++count;
    return true;
  });

  EXPECT_EQ(count, 243u);
}

TEST(AhComb, NodescardBuildPerms)
{
  DynList<int> l1 = {1, 2};
  DynList<DynList<int>> l = {l1};

  // Should not trigger nodiscard warning - use result
  auto perms = build_perms(l);
  EXPECT_EQ(perms.size(), 2u);
}

TEST(AhComb, NodescardBuildCombs)
{
  DynList<int> l1 = {1, 2};
  DynList<DynList<int>> l = {l1};

  // Should not trigger nodiscard warning - use result
  auto combs = build_combs(l);
  EXPECT_EQ(combs.size(), 2u);
}

TEST(AhComb, NodescardTranspose)
{
  DynList<int> r1 = {1, 2};
  DynList<DynList<int>> m = {r1};

  // Should not trigger nodiscard warning - use result
  auto t = transpose(m);
  EXPECT_EQ(t.size(), 2u);
}

TEST(AhComb, NodescardFoldPerm)
{
  DynList<int> l1 = {1, 2};
  DynList<DynList<int>> l = {l1};

  // Should not trigger nodiscard warning - use result
  auto sum = fold_perm<int, int>(0, l, [](int a, const DynList<int> & p)
  {
    return a + p.get_first();
  });
  EXPECT_EQ(sum, 3);
}

TEST(AhComb, NodescardPermCount)
{
  DynList<int> l1 = {1, 2};
  DynList<DynList<int>> l = {l1};

  // Should not trigger nodiscard warning - use result
  auto count = perm_count(l);
  EXPECT_EQ(count, 2u);
}

TEST(AhComb, NodescardExistsPerm)
{
  DynList<int> l1 = {1, 2};
  DynList<DynList<int>> l = {l1};

  // Should not trigger nodiscard warning - use result
  auto exists = exists_perm(l, [](const DynList<int> &) { return true; });
  EXPECT_TRUE(exists);
}

TEST(AhComb, NodescardAllPerm)
{
  DynList<int> l1 = {1, 2};
  DynList<DynList<int>> l = {l1};

  // Should not trigger nodiscard warning - use result
  auto all = all_perm(l, [](const DynList<int> &) { return true; });
  EXPECT_TRUE(all);
}

TEST(AhComb, NodescardNonePerm)
{
  DynList<int> l1 = {1, 2};
  DynList<DynList<int>> l = {l1};

  // Should not trigger nodiscard warning - use result
  auto none = none_perm(l, [](const DynList<int> &) { return false; });
  EXPECT_TRUE(none);
}

TEST(AhComb, NodescardFilterPerm)
{
  DynList<int> l1 = {1, 2};
  DynList<DynList<int>> l = {l1};

  // Should not trigger nodiscard warning - use result
  auto filtered = filter_perm(l, [](const DynList<int> &) { return true; });
  EXPECT_EQ(filtered.size(), 2u);
}

TEST(AhComb, NodescardMapPerm)
{
  DynList<int> l1 = {1, 2};
  DynList<DynList<int>> l = {l1};

  // Should not trigger nodiscard warning - use result
  auto mapped = map_perm<int>(l, [](const DynList<int> & p) { return p.get_first(); });
  EXPECT_EQ(mapped.size(), 2u);
}
