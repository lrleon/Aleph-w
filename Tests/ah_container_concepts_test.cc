#include <list>
#include <map>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <ah-stl-functional.H>
#include <ah-stl-zip.H>
#include <ah-uni-functional.H>
#include <ah-zip-utils.H>
#include <htlist.H>
#include <tpl_array.H>
#include <tpl_dynArray.H>
#include <tpl_dynDlist.H>
#include <tpl_dynMapTree.H>
#include <tpl_dynSlist.H>
#include <tpl_dynSetTree.H>
#include <tpl_flat_set.H>
#include <tpl_lhash.H>
#include <tpl_memArray.H>

// Canonical container/iterator hierarchy (Phase 3 of aleph-concepts-plan.md)
// and the detectors that replaced the former void_t traits. The detector
// values pinned here are the ones the void_t versions produced: they select
// iteration strategies, so a change would silently switch behavior.

using namespace Aleph;

static_assert(AlephSequence<DynList<int>> and AlephSequence<DynDlist<int>>);
static_assert(AlephSequence<Array<int>> and AlephSequence<DynArray<int>>);
static_assert(AlephIterableContainer<DynMapTree<int, int>> and not AlephSequence<DynMapTree<int, int>>);
static_assert(AlephTraversable<FlatSet<int>> and not AlephIterableContainer<FlatSet<int>>);
static_assert(AlephIterator<DynList<int>::Iterator>);
// These iterators offered next() but not next_ne() and so were rejected by
// generic code written against AlephIterator.
static_assert(AlephIterator<Slist<int>::Iterator> and AlephIterator<DynSlist<int>::Iterator>);
static_assert(AlephIterator<decltype(uni_zip_it(std::declval<DynList<int> &>(),
                                                std::declval<std::vector<int> &>()))>);
static_assert(AlephIterator<decltype(stl_zip_it(std::declval<std::vector<int> &>(),
                                                std::declval<std::list<int> &>()))>);
static_assert(AlephIterator<StlEnumerateIterator<std::vector<int>>>);

// AlephIterable: the single "iterate it the Aleph way" test of both functional
// layers. The iterator is built from the container, so types without
// get_it() qualify, and get_curr() need not be const (chained hash tables).
static_assert(AlephIterable<DynList<int>> and AlephIterable<DynSetTree<int>>);
static_assert(AlephIterable<MemArray<int>> and AlephIterable<LhashTable<int>>);
static_assert(not AlephIterable<std::vector<int>> and not AlephIterable<FlatSet<int>>);
static_assert(not AlephTraversable<std::vector<int>> and not AlephIterator<int>);

static_assert(StlIterableContainer<std::vector<int>> and StlIterableContainer<std::map<int, int>>);
// Aleph containers expose begin()/end() but no value_type: not STL-iterable
// for dispatch purposes, exactly as before.
static_assert(not StlIterableContainer<DynList<int>>);

namespace U = uni_functional_detail;
namespace Z = uni_zip_detail;
namespace F = stl_detail;

static_assert(U::has_stl_iterator<std::vector<int>>::value and not U::has_stl_iterator<DynList<int>>::value);
static_assert(Z::has_stl_iterator<std::list<int>>::value and not Z::has_stl_iterator<int>::value);
static_assert(U::has_aleph_iterator<DynList<int>>::value and not U::has_aleph_iterator<std::vector<int>>::value);
// Both layers now share one definition.
static_assert(U::has_aleph_iterator<MemArray<int>>::value == Z::has_aleph_iterator<MemArray<int>>::value);
static_assert(Z::has_aleph_iterator<DynArray<int>>::value and not Z::has_aleph_iterator<std::vector<int>>::value);
static_assert(U::has_reverse_iterator<std::vector<int>>::value and not U::has_reverse_iterator<DynList<int>>::value);
static_assert(F::has_size<std::string>::value and not F::has_size<int>::value);
static_assert(F::is_std_hashable<std::string>::value and not F::is_std_hashable<DynList<int>>::value);
static_assert(F::has_reverse_iterators<std::list<int>>::value and not F::has_reverse_iterators<int>::value);

TEST(AhContainerConceptsTest, HierarchyHoldsForLiveContainers)
{
  DynList<int> l = {1, 2, 3};
  static_assert(AlephSequence<decltype(l)>);
  size_t n = 0;
  for (auto it = l.get_it(); it.has_curr(); it.next_ne())
    ++n;
  EXPECT_EQ(n, l.size());
}

// uni_* used to call get_it(), which MemArray and the raw hash tables lack.
TEST(AhContainerConceptsTest, UniFunctionalIteratesContainersWithoutGetIt)
{
  MemArray<int> m;
  for (int i : {1, 2, 3})
    m.append(i);
  int sum = 0;
  uni_for_each([&sum](int x) { sum += x; }, m);
  EXPECT_EQ(sum, 6);
}

// The zip wrapper read get_curr() through a const iterator, which the chained
// hash tables' iterators do not offer.
TEST(AhContainerConceptsTest, ZipIteratesChainedHashTables)
{
  LhashTable<int> t;
  t.insert(new LhashTable<int>::Bucket(7));
  DynList<int> l = {10};
  int pairs = 0;
  for (auto it = uni_zip_it(l, t); it.has_curr(); it.next())
    {
      auto [x, bucket] = it.get_curr();
      EXPECT_EQ(x, 10);
      EXPECT_EQ(bucket->get_key(), 7);
      ++pairs;
    }
  EXPECT_EQ(pairs, 1);
}

namespace
{
  // Generic code written only against AlephIterator.
  template <AlephIterator It>
  size_t count_with_next_ne(It it)
  {
    size_t n = 0;
    for (; it.has_curr(); it.next_ne())
      ++n;
    return n;
  }
}

TEST(AhContainerConceptsTest, NextNeMatchesNextOnSlistIterators)
{
  DynSlist<int> l;
  for (int i = 0; i < 4; ++i)
    l.insert(i, i * 10);

  DynList<int> by_next, by_next_ne;
  for (DynSlist<int>::Iterator it(l); it.has_curr(); it.next())
    by_next.append(it.get_curr());
  for (DynSlist<int>::Iterator it(l); it.has_curr(); it.next_ne())
    by_next_ne.append(it.get_curr());
  EXPECT_EQ(by_next, by_next_ne);
  EXPECT_EQ(by_next, DynList<int>({0, 10, 20, 30}));

  EXPECT_EQ(count_with_next_ne(Slist<int>::Iterator(l)), 4u);
  DynSlist<int> empty;
  EXPECT_EQ(count_with_next_ne(DynSlist<int>::Iterator(empty)), 0u);
}

TEST(AhContainerConceptsTest, ZipAdaptorsWorkInGenericIteratorCode)
{
  DynList<int> a = {1, 2, 3};
  std::vector<int> b = {4, 5};
  std::list<int> c = {6, 7, 8};
  // Zips stop at the shortest sequence.
  EXPECT_EQ(count_with_next_ne(uni_zip_it(a, b)), 2u);
  EXPECT_EQ(count_with_next_ne(stl_zip_it(b, c)), 2u);
  EXPECT_EQ(count_with_next_ne(StlEnumerateIterator<std::list<int>>(c)), 3u);
}
