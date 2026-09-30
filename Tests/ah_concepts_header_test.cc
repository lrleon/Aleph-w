#include <ah-concepts.H>

#include <gtest/gtest.h>

// Regression test for aleph-concepts.md §2.1: ah-concepts.H must compile
// when included first, on its own, without relying on another header
// (htlist.H, tpl_array.H, ...) having pulled in <cstddef> already.

struct IntLess
{
  bool operator()(const int & a, const int & b) const { return a < b; }
};

struct IntEqual
{
  bool operator()(const int & a, const int & b) const { return a == b; }
};

static_assert(Aleph::BinaryPredicate<IntLess, int>);
static_assert(Aleph::StrictWeakOrder<IntLess, int>);
static_assert(Aleph::EqualityComparator<IntEqual, int>);

// A comparator whose operator() is not const satisfies std::strict_weak_order
// but is unusable by a const tree; StrictWeakOrder must reject it (see
// aleph-concepts.md §2.2).
struct NonConstLess
{
  bool operator()(const int & a, const int & b) { return a < b; }
};

static_assert(not Aleph::StrictWeakOrder<NonConstLess, int>);
static_assert(not Aleph::EqualityComparator<NonConstLess, int>);

// A functor with DIFFERENT const and non-const operator() overloads: the
// const overload alone satisfies the old (const-only) StrictWeakOrder, but
// mutating BST paths (Gen_Avl_Tree::insert -> search_and_stack_avl) invoke
// cmp as a non-const lvalue and select the non-const overload instead,
// which doesn't return bool. Caught during a codex review of this branch;
// reproduced against Aleph::Avl_Tree<int, MixedConstCmp>::insert(), which
// fails at tpl_avl.H:153 if this concept only checks the const shape.
struct MixedConstCmp
{
  bool operator()(const int & a, const int & b) const { return a < b; }
  void operator()(const int & a, const int & b) { (void) a; (void) b; }
};

static_assert(not Aleph::StrictWeakOrder<MixedConstCmp, int>);
static_assert(not Aleph::EqualityComparator<MixedConstCmp, int>);

TEST(AhConceptsHeaderTest, IsSelfContained)
{
  IntLess less;
  EXPECT_TRUE(less(1, 2));
  EXPECT_FALSE(less(2, 1));
}
