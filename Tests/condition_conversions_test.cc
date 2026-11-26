
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
 * @file condition_conversions_test.cc
 * @brief What a comparison or a filter returns is taken as a condition,
 *        and its conversion there may throw
 */

// A comparison, or a filter of arcs or of items, may return a type that
// converts to bool in two ways: implicitly, where it is returned or
// assigned, and explicitly, where it is a condition. The intervals and
// their trees, the filtered iterators of the graphs and of the containers,
// Kruskal, the searches of the containers and the sorts take such a result
// as a condition, and were noexcept when the implicit conversion did not
// throw, or the call alone in the searches and the sorts: with a result
// whose conversion there throws, the program ended instead of the exception
// reaching the caller. They are now noexcept when the conversion that they
// make is. So are the functors that compare by default, as Aleph::less,
// the equality of an interval, the searches of the stack and of the queue
// of lists and those of a graph by what its nodes and arcs hold, and the
// iterators on the arcs of a node with a filter of the caller. And
// traverse() of the containers converts what its operation returns, where
// it negated it.

#include <gtest/gtest.h>

#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

#include <Kruskal.H>
#include <ahFunction.H>
#include <filter_iterator.H>
#include <htlist.H>
#include <tpl_agraph.H>
#include <tpl_array.H>
#include <tpl_arrayHeap.H>
#include <tpl_arrayQueue.H>
#include <tpl_arrayStack.H>
#include <tpl_dynArray.H>
#include <tpl_dynArrayHeap.H>
#include <tpl_dynDlist.H>
#include <tpl_dynListQueue.H>
#include <tpl_dynListStack.H>
#include <tpl_dynSkipList.H>
#include <tpl_graph.H>
#include <tpl_interval_tree.H>
#include <tpl_sgraph.H>
#include <tpl_sort_utils.H>

using namespace Aleph;

namespace
{
  struct Conversion_Error : std::runtime_error
  {
    Conversion_Error() : std::runtime_error("the conversion in a condition") {}
  };

  // What the comparison and the filters of these tests return. Its implicit
  // conversion to bool does not throw; the explicit one, which a condition
  // chooses for a result that has no name, throws while armed
  struct Sly_Bool
  {
    static inline bool armed = false;
    bool value;

    operator bool () const & noexcept { return value; }

    explicit operator bool () &&
    {
      if (armed)
        throw Conversion_Error();
      return value;
    }
  };

  struct Arm
  {
    Arm() noexcept { Sly_Bool::armed = true; }
    ~Arm() { Sly_Bool::armed = false; }
  };

  struct Sly_Less
  {
    Sly_Bool operator () (const int & a, const int & b) const noexcept { return {a < b}; }
  };

  // What the comparison of the sorts returns: it has one conversion to
  // bool, which throws while armed
  struct Loud_Bool
  {
    bool value;

    operator bool () const
    {
      if (Sly_Bool::armed)
        throw Conversion_Error();
      return value;
    }
  };

  struct Loud_Less
  {
    Loud_Bool operator () (const int & a, const int & b) const noexcept { return {a < b}; }
  };

  // An item whose comparisons do not throw, and return such a result
  struct Loud_Item
  {
    int v = 0;
    Loud_Item(const int x = 0) noexcept : v(x) {}
  };
  Loud_Bool operator < (const Loud_Item & a, const Loud_Item & b) noexcept { return {a.v < b.v}; }
  Loud_Bool operator > (const Loud_Item & a, const Loud_Item & b) noexcept { return {a.v > b.v}; }
  Loud_Bool operator <= (const Loud_Item & a, const Loud_Item & b) noexcept { return {a.v <= b.v}; }
  Loud_Bool operator >= (const Loud_Item & a, const Loud_Item & b) noexcept { return {a.v >= b.v}; }
  Loud_Bool operator == (const Loud_Item & a, const Loud_Item & b) noexcept { return {a.v == b.v}; }
  Loud_Bool operator != (const Loud_Item & a, const Loud_Item & b) noexcept { return {a.v != b.v}; }

  // A flag whose logical operators do the same
  struct Loud_Flag
  {
    bool v = false;
  };
  Loud_Bool operator && (const Loud_Flag & a, const Loud_Flag & b) noexcept { return {a.v and b.v}; }
  Loud_Bool operator || (const Loud_Flag & a, const Loud_Flag & b) noexcept { return {a.v or b.v}; }
  Loud_Bool operator ! (const Loud_Flag & a) noexcept { return {not a.v}; }

  // A predicate for unary_negate
  struct Loud_Odd
  {
    using argument_type = int;
    Loud_Bool operator () (const int & x) const noexcept { return {x % 2 != 0}; }
  };

  // Another one, whose result has a negation that does not throw and a
  // conversion that does while armed: unary_negate converts it
  struct Quiet_Not
  {
    bool value;
    explicit operator bool () const
    {
      if (Sly_Bool::armed)
        throw Conversion_Error();
      return value;
    }
    bool operator ! () const noexcept { return not value; }
  };
  struct Quiet_Not_Odd
  {
    using argument_type = int;
    Quiet_Not operator () (const int & x) const noexcept { return {x % 2 != 0}; }
  };

  // What an operation of traverse() may return: its conversion to bool
  // does not throw, and its negation throws while armed
  struct Not_Throws
  {
    bool value;
    explicit operator bool () const noexcept { return value; }
    bool operator ! () const
    {
      if (Sly_Bool::armed)
        throw Conversion_Error();
      return not value;
    }
  };

  // How many times each of the items 1 to 5 is in a container, as a number
  // of five digits: 11111 if each one is there once
  template <class Container>
  long census(const Container & c)
  {
    long digits = 0;
    c.for_each([&digits] (const int & x)
      {
        long unit = 1;
        for (int i = 1; i < x; ++i)
          unit *= 10;
        digits += unit;
      });
    return digits;
  }

  template <class Item>
  struct Sly_Show
  {
    Sly_Bool operator () (const Item &) const noexcept { return {true}; }
  };

  // the old test of the specifications says that nothing throws
  static_assert(std::is_nothrow_invocable_r_v<bool, const Sly_Less &, const int &, const int &>);
  static_assert(not NothrowPredicateWith<const Sly_Less &, const int &, const int &>);

  using Edges = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
  using Arrows = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
  using Arc_Show = Sly_Show<Edges::Arc *>;
  using Darc_Show = Sly_Show<Arrows::Arc *>;

  // a -> b -> c, and a -> c
  template <class GT>
  struct Triangle
  {
    GT g;
    typename GT::Node * a = g.insert_node(1);
    typename GT::Node * b = g.insert_node(2);
    typename GT::Node * c = g.insert_node(3);

    Triangle()
    {
      g.insert_arc(a, b, 1);
      g.insert_arc(b, c, 2);
      g.insert_arc(a, c, 3);
    }
  };

  // A filter of the arcs of a node of the kind that In_Iterator and
  // Out_Iterator give to Digraph_Iterator: it shows every arc
  template <class GT>
  struct Sly_Arc_Filter
  {
    Sly_Arc_Filter(typename GT::Node * = nullptr) noexcept {}
    Sly_Bool operator () (typename GT::Arc *) const noexcept { return {true}; }
    typename GT::Node * get_node(typename GT::Arc * a) const noexcept
    {
      return static_cast<typename GT::Node *>(a->tgt_node);
    }
  };

  // A filter that tells an item that has a name from one that has none,
  // and counts the calls with each: only the second one is noexcept
  template <class Item>
  struct Two_Calls
  {
    static inline size_t named = 0, unnamed = 0;

    bool operator () (Item *&) const
    {
      ++named;
      return true;
    }

    bool operator () (Item *&&) const noexcept
    {
      ++unnamed;
      return true;
    }
  };

  // Whether next_ne() of the iterator It, built with args and taken to
  // its end, is noexcept exactly when no call that It made of its
  // filter is the one that may throw
  template <class It, class Item, class... Args>
  bool says_what_it_calls(Args &&... args)
  {
    Two_Calls<Item>::named = Two_Calls<Item>::unnamed = 0;
    for (It it(std::forward<Args>(args)...); it.has_curr(); it.next_ne())
      ;
    const size_t named = Two_Calls<Item>::named;
    return named + Two_Calls<Item>::unnamed > 0
           and noexcept(std::declval<It &>().next_ne()) == (named == 0);
  }

  // The five filtered iterators of a graph type
  template <class GT>
  void each_iterator_says_what_it_calls()
  {
    using Arcs = Two_Calls<typename GT::Arc>;
    using Nodes = Two_Calls<typename GT::Node>;
    Triangle<GT> t;
    EXPECT_TRUE((says_what_it_calls<Node_Arc_Iterator<GT, Arcs>, typename GT::Arc>(t.a)));
    EXPECT_TRUE((says_what_it_calls<Arc_Iterator<GT, Arcs>, typename GT::Arc>(t.g)));
    EXPECT_TRUE((says_what_it_calls<Out_Iterator<GT, Arcs>, typename GT::Arc>(t.a)));
    EXPECT_TRUE((says_what_it_calls<In_Iterator<GT, Arcs>, typename GT::Arc>(t.g, t.c)));
    EXPECT_TRUE((says_what_it_calls<Node_Iterator<GT, Nodes>, typename GT::Node>(t.g)));
  }
}

// The functors that compare by default return a bool: they convert what
// the operator of the items returns
TEST(ConditionConversions, TheDefaultFunctors)
{
  const Loud_Item a(1), b(2);
  const Loud_Flag yes{true}, no{false};
  static_assert(not noexcept(Aleph::less<Loud_Item>()(a, b)));
  static_assert(not noexcept(Aleph::greater<Loud_Item>()(a, b)));
  static_assert(not noexcept(Aleph::less_equal<Loud_Item>()(a, b)));
  static_assert(not noexcept(Aleph::greater_equal<Loud_Item>()(a, b)));
  static_assert(not noexcept(Aleph::equal_to<Loud_Item>()(a, b)));
  static_assert(not noexcept(Aleph::not_equal_to<Loud_Item>()(a, b)));
  static_assert(not noexcept(Aleph::logical_and<Loud_Flag>()(yes, no)));
  static_assert(not noexcept(Aleph::logical_or<Loud_Flag>()(yes, no)));
  static_assert(not noexcept(Aleph::logical_not<Loud_Flag>()(yes)));
  // the call alone: the constructor of unary_negate says nothing
  static_assert(not noexcept(std::declval<const Aleph::unary_negate<Loud_Odd> &>()(1)));
  static_assert(not noexcept(std::declval<const Aleph::unary_negate<Quiet_Not_Odd> &>()(1)));
  // and with operators that return a bool they are, as before
  static_assert(noexcept(Aleph::less<int>()(1, 2)));
  static_assert(noexcept(Aleph::greater<int>()(1, 2)));
  static_assert(noexcept(Aleph::less_equal<int>()(1, 2)));
  static_assert(noexcept(Aleph::greater_equal<int>()(1, 2)));
  static_assert(noexcept(Aleph::equal_to<int>()(1, 2)));
  static_assert(noexcept(Aleph::not_equal_to<int>()(1, 2)));
  static_assert(noexcept(Aleph::logical_and<bool>()(true, false)));
  static_assert(noexcept(Aleph::logical_or<bool>()(true, false)));
  static_assert(noexcept(Aleph::logical_not<bool>()(true)));

  EXPECT_TRUE(Aleph::less<Loud_Item>()(a, b));
  EXPECT_FALSE(Aleph::greater<Loud_Item>()(a, b));
  EXPECT_TRUE(Aleph::less_equal<Loud_Item>()(a, a));
  EXPECT_FALSE(Aleph::greater_equal<Loud_Item>()(a, b));
  EXPECT_FALSE(Aleph::equal_to<Loud_Item>()(a, b));
  EXPECT_TRUE(Aleph::not_equal_to<Loud_Item>()(a, b));
  EXPECT_FALSE(Aleph::logical_and<Loud_Flag>()(yes, no));
  EXPECT_TRUE(Aleph::logical_or<Loud_Flag>()(yes, no));
  EXPECT_FALSE(Aleph::logical_not<Loud_Flag>()(yes));
  EXPECT_TRUE(Aleph::unary_negate<Loud_Odd>(Loud_Odd())(2));
  EXPECT_TRUE(Aleph::unary_negate<Quiet_Not_Odd>(Quiet_Not_Odd())(2));

  // a heap that compares with the default functor lets the exception out,
  // and is as it was
  ArrayHeap<Loud_Item> heap(8);
  heap.insert(b);

  const Arm arm;
  EXPECT_THROW((void) Aleph::less<Loud_Item>()(a, b), Conversion_Error);
  EXPECT_THROW((void) Aleph::greater<Loud_Item>()(a, b), Conversion_Error);
  EXPECT_THROW((void) Aleph::less_equal<Loud_Item>()(a, b), Conversion_Error);
  EXPECT_THROW((void) Aleph::greater_equal<Loud_Item>()(a, b), Conversion_Error);
  EXPECT_THROW((void) Aleph::equal_to<Loud_Item>()(a, b), Conversion_Error);
  EXPECT_THROW((void) Aleph::not_equal_to<Loud_Item>()(a, b), Conversion_Error);
  EXPECT_THROW((void) Aleph::logical_and<Loud_Flag>()(yes, no), Conversion_Error);
  EXPECT_THROW((void) Aleph::logical_or<Loud_Flag>()(yes, no), Conversion_Error);
  EXPECT_THROW((void) Aleph::logical_not<Loud_Flag>()(yes), Conversion_Error);
  EXPECT_THROW((void) Aleph::unary_negate<Loud_Odd>(Loud_Odd())(2), Conversion_Error);
  EXPECT_THROW((void) Aleph::unary_negate<Quiet_Not_Odd>(Quiet_Not_Odd())(2), Conversion_Error);
  EXPECT_THROW(heap.insert(a), Conversion_Error);
  EXPECT_EQ(heap.size(), 1u);
}

// overlaps(), contains() and is_valid() of an interval, and Interval_Less
TEST(ConditionConversions, Intervals)
{
  using I = Interval<int>;
  static_assert(not noexcept(std::declval<const I &>().overlaps(std::declval<const I &>(), Sly_Less())));
  static_assert(not noexcept(std::declval<const I &>().contains(1, Sly_Less())));
  static_assert(not noexcept(std::declval<const I &>().is_valid(Sly_Less())));
  static_assert(not noexcept(std::declval<const Interval_Less<int, Sly_Less> &>()(std::declval<const I &>(),
                                                                                std::declval<const I &>())));
  // and with a comparison whose result is a bool they are, as before
  static_assert(noexcept(std::declval<const I &>().overlaps(std::declval<const I &>())));
  static_assert(noexcept(std::declval<const I &>().contains(1)));
  static_assert(noexcept(std::declval<const I &>().is_valid()));
  static_assert(noexcept(std::declval<const Interval_Less<int> &>()(std::declval<const I &>(),
                                                                    std::declval<const I &>())));

  const I a(1, 5), b(4, 9), c(7, 8);
  const Sly_Less less;
  const Interval_Less<int, Sly_Less> before(less);
  EXPECT_TRUE(a.overlaps(b, less));
  EXPECT_FALSE(a.overlaps(c, less));
  EXPECT_TRUE(a.contains(5, less));
  EXPECT_FALSE(a.contains(6, less));
  EXPECT_TRUE(a.is_valid(less));
  EXPECT_TRUE(before(a, b));
  EXPECT_FALSE(before(b, a));

  const Arm arm;
  EXPECT_THROW((void) a.overlaps(b, less), Conversion_Error);
  EXPECT_THROW((void) a.contains(5, less), Conversion_Error);
  EXPECT_THROW((void) a.is_valid(less), Conversion_Error);
  EXPECT_THROW((void) before(a, b), Conversion_Error);
}

// The equality of two intervals, by that of their endpoints
TEST(ConditionConversions, EqualityOfIntervals)
{
  using L = Interval<Loud_Item>;
  static_assert(not noexcept(std::declval<const L &>() == std::declval<const L &>()));
  static_assert(not noexcept(std::declval<const L &>() != std::declval<const L &>()));
  static_assert(noexcept(std::declval<const Interval<int> &>() == std::declval<const Interval<int> &>()));
  static_assert(noexcept(std::declval<const Interval<int> &>() != std::declval<const Interval<int> &>()));

  const L x(Loud_Item(1), Loud_Item(5)), y(Loud_Item(1), Loud_Item(5)), z(Loud_Item(1), Loud_Item(6));
  EXPECT_TRUE(x == y);
  EXPECT_FALSE(x == z);
  EXPECT_TRUE(x != z);

  const Arm arm;
  EXPECT_THROW((void) (x == y), Conversion_Error);
  EXPECT_THROW((void) (x != z), Conversion_Error);
}

// search() of a tree of intervals
TEST(ConditionConversions, IntervalTree)
{
  using Tree = DynIntervalTree<int, Sly_Less>;
  static_assert(not noexcept(std::declval<const Tree &>().search(std::declval<const Interval<int> &>())));
  static_assert(noexcept(std::declval<const DynIntervalTree<int> &>().search(std::declval<const Interval<int> &>())));

  Tree tree;
  for (int i = 0; i < 20; ++i)
    tree.insert(Interval<int>(i, i + 3));
  EXPECT_TRUE(tree.search(Interval<int>(7, 10)));
  EXPECT_FALSE(tree.search(Interval<int>(7, 11)));

  const Arm arm;
  EXPECT_THROW((void) tree.search(Interval<int>(7, 10)), Conversion_Error);
}

// The iterators on the arcs of a node that take a filter, and the searches
// through them
TEST(ConditionConversions, ArcFiltersOfANode)
{
  using Out = Out_Iterator<Arrows, Darc_Show>;
  using In = In_Iterator<Edges, Arc_Show>;
  using Node_Arcs = Node_Arc_Iterator<Edges, Arc_Show>;
  static_assert(not nothrow_arc_filter_v<Edges, Arc_Show>);
  static_assert(nothrow_arc_filter_v<Edges, Dft_Show_Arc<Edges>>);
  static_assert(not noexcept(std::declval<Out &>().next_ne()));
  static_assert(not noexcept(std::declval<In &>().next_ne()));
  static_assert(not noexcept(std::declval<Node_Arcs &>().next_ne()));
  static_assert(noexcept(std::declval<Out_Iterator<Arrows> &>().next_ne()));
  static_assert(noexcept(std::declval<In_Iterator<Edges> &>().next_ne()));
  static_assert(noexcept(std::declval<Node_Arc_Iterator<Edges> &>().next_ne()));
  static_assert(not noexcept(search_arc(std::declval<const Edges &>(), std::declval<Edges::Node *>(),
                                        std::declval<Edges::Node *>(), Arc_Show())));
  static_assert(noexcept(search_arc(std::declval<const Edges &>(), std::declval<Edges::Node *>(),
                                    std::declval<Edges::Node *>())));

  Triangle<Arrows> d;
  Triangle<Edges> u;
  {
    Out out(d.a);
    size_t n = 0;
    for (; out.has_curr(); out.next_ne())
      ++n;
    EXPECT_EQ(n, 2u);

    In in(u.c);
    n = 0;
    for (; in.has_curr(); in.next_ne())
      ++n;
    EXPECT_EQ(n, 2u);
    EXPECT_NE(search_arc(u.g, u.a, u.c, Arc_Show()), nullptr);
  }

  Out out(d.a);
  In in(u.c);
  Node_Arcs arcs(u.a);
  const Arm arm;
  EXPECT_THROW(out.next_ne(), Conversion_Error);
  EXPECT_THROW(in.next_ne(), Conversion_Error);
  EXPECT_THROW(arcs.next_ne(), Conversion_Error);
  EXPECT_THROW(Out again(d.a), Conversion_Error);
  EXPECT_THROW((void) search_arc(u.g, u.a, u.c, Arc_Show()), Conversion_Error);
}

// The walk on the arcs into a node of a digraph, among the arcs of the graph
TEST(ConditionConversions, ArcsIntoANodeOfADigraph)
{
  using In = In_Iterator<Arrows, Darc_Show>;
  static_assert(not noexcept(std::declval<In &>().next_ne()));
  static_assert(noexcept(std::declval<In_Iterator<Arrows> &>().next_ne()));

  Triangle<Arrows> d;
  {
    size_t n = 0;
    for (In in(d.g, d.c); in.has_curr(); in.next_ne())
      ++n;
    EXPECT_EQ(n, 2u);
  }

  In in(d.g, d.c);
  const Arm arm;
  EXPECT_THROW(in.next_ne(), Conversion_Error);
}

// A specification looks at the call of the filter that the iterator
// makes, with the item as the iterator gives it: the walk on the arcs
// into a node of a digraph gives an arc that has a name, and the other
// iterators give one that has none. A filter may tell the two apart
TEST(ConditionConversions, EachFilteredIteratorLooksAtTheCallThatItMakes)
{
  using Named = Two_Calls<Arrows::Arc>;
  static_assert(not NothrowPredicateWith<Named &, Arrows::Arc *&>);
  static_assert(NothrowPredicateWith<Named &, Arrows::Arc *>);
  static_assert(not noexcept(std::declval<In_Iterator<Arrows, Named> &>().next_ne()));
  static_assert(noexcept(std::declval<Out_Iterator<Arrows, Named> &>().next_ne()));

  {
    SCOPED_TRACE("List_Graph");
    each_iterator_says_what_it_calls<Edges>();
  }
  {
    SCOPED_TRACE("List_Digraph");
    each_iterator_says_what_it_calls<Arrows>();
  }
  {
    SCOPED_TRACE("List_SGraph");
    each_iterator_says_what_it_calls<List_SGraph<Graph_Snode<int>, Graph_Sarc<int>>>();
  }
  {
    SCOPED_TRACE("List_SDigraph");
    each_iterator_says_what_it_calls<List_SDigraph<Graph_Snode<int>, Graph_Sarc<int>>>();
  }
  {
    SCOPED_TRACE("Array_Graph");
    each_iterator_says_what_it_calls<Array_Graph<Graph_Anode<int>, Graph_Aarc<int>>>();
  }
  {
    SCOPED_TRACE("Array_Digraph");
    each_iterator_says_what_it_calls<Array_Digraph<Graph_Anode<int>, Graph_Aarc<int>>>();
  }
}

// The filtered iterator of the containers
TEST(ConditionConversions, FilterIterator)
{
  using List = DynList<int>;
  using It = Filter_Iterator<List, List::Iterator, Sly_Show<int>>;
  static_assert(not noexcept(std::declval<It &>().next_ne()));

  List l = {1, 2, 3};
  {
    size_t n = 0;
    for (It it(l); it.has_curr(); it.next_ne())
      ++n;
    EXPECT_EQ(n, 3u);
  }

  It it(l);
  const Arm arm;
  EXPECT_THROW(it.next_ne(), Conversion_Error);
  EXPECT_THROW(It again(l), Conversion_Error);
}

// The searches of the containers: find_ptr(), find_index() and find_item()
TEST(ConditionConversions, SearchesOfAContainer)
{
  using List = DynList<int>;
  Sly_Show<int> show; // shows every item: the first one is found
  const auto second = [] (const int & x) noexcept { return x == 2; };
  static_assert(not noexcept(std::declval<const List &>().find_ptr(show)));
  static_assert(not noexcept(std::declval<List &>().find_ptr(show)));
  static_assert(not noexcept(std::declval<const List &>().find_index(show)));
  static_assert(not noexcept(std::declval<const List &>().find_item(show)));
  static_assert(noexcept(std::declval<const List &>().find_ptr(second)));
  static_assert(noexcept(std::declval<List &>().find_ptr(second)));
  static_assert(noexcept(std::declval<const List &>().find_index(second)));

  List l = {1, 2, 3};
  const List & cl = l;
  ASSERT_NE(cl.find_ptr(show), nullptr);
  EXPECT_EQ(*cl.find_ptr(show), 1);
  EXPECT_EQ(cl.find_index(show), 0u);
  ASSERT_NE(l.find_ptr(second), nullptr);
  EXPECT_EQ(*l.find_ptr(second), 2);
  EXPECT_TRUE(std::get<0>(cl.find_item(show)));

  const Arm arm;
  EXPECT_THROW((void) cl.find_ptr(show), Conversion_Error);
  EXPECT_THROW((void) l.find_ptr(show), Conversion_Error);
  EXPECT_THROW((void) cl.find_index(show), Conversion_Error);
  EXPECT_THROW((void) cl.find_item(show), Conversion_Error);
}

// The searches of the stack and of the queue of lists, by the == of their
// items
TEST(ConditionConversions, SearchesOfAStackAndOfAQueue)
{
  using Stack = DynListStack<Loud_Item>;
  using Queue = DynListQueue<Loud_Item>;
  const Loud_Item one(1), two(2), three(3);
  static_assert(not noexcept(std::declval<Stack &>().search(one)));
  static_assert(not noexcept(std::declval<const Stack &>().search(one)));
  static_assert(not noexcept(std::declval<const Stack &>().contains(one)));
  static_assert(not noexcept(std::declval<const Stack &>().has(one)));
  static_assert(not noexcept(std::declval<Queue &>().search(one)));
  static_assert(not noexcept(std::declval<const Queue &>().search(one)));
  static_assert(noexcept(std::declval<const DynListStack<int> &>().contains(1)));
  static_assert(noexcept(std::declval<const DynListQueue<int> &>().search(1)));

  Stack stack;
  Queue queue;
  for (const Loud_Item & item : {one, two})
    {
      stack.push(item);
      queue.put(item);
    }
  const Stack & cstack = stack;
  const Queue & cqueue = queue;
  ASSERT_NE(stack.search(one), nullptr);
  EXPECT_EQ(stack.search(one)->v, 1);
  ASSERT_NE(cstack.search(two), nullptr);
  EXPECT_TRUE(cstack.contains(two));
  EXPECT_FALSE(cstack.has(three));
  ASSERT_NE(queue.search(two), nullptr);
  EXPECT_EQ(queue.search(two)->v, 2);
  EXPECT_EQ(cqueue.search(three), nullptr);

  const Arm arm;
  EXPECT_THROW((void) stack.search(one), Conversion_Error);
  EXPECT_THROW((void) cstack.search(one), Conversion_Error);
  EXPECT_THROW((void) cstack.contains(one), Conversion_Error);
  EXPECT_THROW((void) cstack.has(one), Conversion_Error);
  EXPECT_THROW((void) queue.search(one), Conversion_Error);
  EXPECT_THROW((void) cqueue.search(one), Conversion_Error);
}

// find_node() and find_arc() of a graph, by the == of what its nodes and
// its arcs hold
TEST(ConditionConversions, SearchesOfAGraphByWhatItHolds)
{
  using Loud_Graph = List_Graph<Graph_Node<Loud_Item>, Graph_Arc<Loud_Item>>;
  const Loud_Item two(2), three(3), seven(7);
  static_assert(not noexcept(std::declval<const Loud_Graph &>().find_node(two)));
  static_assert(not noexcept(std::declval<const Loud_Graph &>().find_arc(three)));
  static_assert(noexcept(std::declval<const Edges &>().find_node(1)));
  static_assert(noexcept(std::declval<const Edges &>().find_arc(1)));

  Loud_Graph g;
  auto p = g.insert_node(Loud_Item(1));
  auto q = g.insert_node(two);
  auto a = g.insert_arc(p, q, three);
  EXPECT_EQ(g.find_node(two), q);
  EXPECT_EQ(g.find_node(seven), nullptr);
  EXPECT_EQ(g.find_arc(three), a);
  EXPECT_EQ(g.find_arc(seven), nullptr);

  const Arm arm;
  EXPECT_THROW((void) g.find_node(two), Conversion_Error);
  EXPECT_THROW((void) g.find_arc(three), Conversion_Error);
}

// The iterators on the arcs of a node with a filter of the caller: the one
// of tpl_graph.H and the one of the graph classes
TEST(ConditionConversions, ArcsOfANodeThroughAFilterOfTheCaller)
{
  using Filter = Sly_Arc_Filter<Edges>;
  using Free = Aleph::Digraph_Iterator<Edges, Filter>;
  using Member = Edges::Digraph_Iterator<Filter>;
  static_assert(not Free::shows_ne);
  static_assert(not Member::shows_ne);
  static_assert(not noexcept(std::declval<Free &>().next_ne()));
  static_assert(not noexcept(std::declval<Free &>().reset_first()));
  static_assert(not noexcept(std::declval<Member &>().next_ne()));
  static_assert(not noexcept(std::declval<Member &>().reset_first()));
  // and with the filters of the library they are, as before
  static_assert(Edges::Out_Iterator::shows_ne);
  static_assert(noexcept(std::declval<Edges::Out_Iterator &>().next_ne()));
  static_assert(noexcept(std::declval<Edges::Out_Iterator &>().reset_first()));
  static_assert(noexcept(std::declval<Edges::In_Iterator &>().next_ne()));
  static_assert(noexcept(std::declval<Edges::In_Iterator &>().reset_first()));

  Triangle<Edges> u;
  Free free(u.a);
  Member member(u.a);
  size_t n = 0;
  for (; free.has_curr(); free.next_ne())
    ++n;
  EXPECT_EQ(n, 2u);
  for (n = 0; member.has_curr(); member.next_ne())
    ++n;
  EXPECT_EQ(n, 2u);
  free.reset_first();
  member.reset_first();
  ASSERT_TRUE(free.has_curr());
  ASSERT_TRUE(member.has_curr());

  const Arm arm;
  EXPECT_THROW(free.next_ne(), Conversion_Error);
  EXPECT_THROW(free.reset_first(), Conversion_Error);
  EXPECT_THROW(member.next_ne(), Conversion_Error);
  EXPECT_THROW(member.reset_first(), Conversion_Error);
}

// traverse() takes what its operation returns as a condition: it converts
// it, where it negated it, which is another operator of the result
TEST(ConditionConversions, TraverseConvertsWhatItsOperationReturns)
{
  const auto all = [] (const int &) noexcept { return Not_Throws{true}; };
  const auto none = [] (const int &) noexcept { return Not_Throws{false}; };

  DynList<int> list;
  ArrayHeap<int> heap(8);
  DynArrayHeap<int> dyn_heap;
  MemArray<int> mem;
  ArrayStack<int> stack;
  FixedStack<int> fixed_stack;
  ArrayQueue<int> queue;
  FixedQueue<int> fixed;
  DynArray<int> array;
  DynSkipList<int> skip;
  for (int i = 1; i <= 3; ++i)
    {
      list.append(i);
      heap.insert(i);
      dyn_heap.insert(i);
      mem.append(i);
      stack.push(i);
      fixed_stack.push(i);
      queue.put(i);
      fixed.put(i);
      array.append(i);
      skip.insert(i);
    }
  static_assert(noexcept(std::declval<const DynList<int> &>().traverse(all)));
  static_assert(noexcept(std::declval<const ArrayHeap<int> &>().traverse(all)));

  const Arm arm;
  const auto check = [&all, &none] (const auto & container, const char * name)
    {
      SCOPED_TRACE(name);
      EXPECT_NO_THROW(EXPECT_TRUE(container.traverse(all)));
      EXPECT_NO_THROW(EXPECT_FALSE(container.traverse(none)));
    };
  check(list, "DynList");
  check(heap, "ArrayHeap");
  check(dyn_heap, "DynArrayHeap");
  check(mem, "MemArray");
  check(stack, "ArrayStack");
  check(fixed_stack, "FixedStack");
  check(queue, "ArrayQueue");
  check(fixed, "FixedQueue");
  check(array, "DynArray");
  check(skip, "DynSkipList");
  // and so do the containers that are not const
  EXPECT_NO_THROW(EXPECT_TRUE(list.traverse(all)));
  EXPECT_NO_THROW(EXPECT_TRUE(heap.traverse(all)));
  EXPECT_NO_THROW(EXPECT_TRUE(dyn_heap.traverse(all)));
  EXPECT_NO_THROW(EXPECT_TRUE(mem.traverse(all)));
  EXPECT_NO_THROW(EXPECT_TRUE(stack.traverse(all)));
  EXPECT_NO_THROW(EXPECT_TRUE(fixed_stack.traverse(all)));
  EXPECT_NO_THROW(EXPECT_TRUE(queue.traverse(all)));
  EXPECT_NO_THROW(EXPECT_TRUE(fixed.traverse(all)));
  EXPECT_NO_THROW(EXPECT_TRUE(array.traverse(all)));
}

// The sorts of arrays that are noexcept when the comparison is, and the
// comparisons that the other sorts go through
TEST(ConditionConversions, SortsOfArrays)
{
  int a[] = {3, 1, 2, 5, 4};
  const long last = 4;
  const size_t n = 5;
  const Loud_Less less;
  static_assert(not noexcept(selection_sort(a, n, less)));
  static_assert(not noexcept(insertion_sort(a, 0L, last, less)));
  static_assert(not noexcept(select_pivot(a, 0L, last, less)));
  static_assert(not noexcept(partition(a, 0L, last, less)));
  static_assert(not noexcept(bucket_sort_detail::insertion_sort_range(a, size_t{0}, n, less)));
  static_assert(not noexcept(timsort_detail::binary_insertion_sort(a, size_t{0}, n, size_t{1}, less)));
  static_assert(not noexcept(timsort_detail::gallop_left(a[0], a, n, size_t{0}, less)));
  static_assert(not noexcept(timsort_detail::gallop_right(a[0], a, n, size_t{0}, less)));
  static_assert(not noexcept(std::declval<const Negate_Compare<int, Loud_Less> &>()(1, 2)));
  static_assert(not noexcept(std::declval<const Inversed_Compare<int, Loud_Less> &>()(1, 2)));
  static_assert(not noexcept(std::declval<const Compare_Dnode<int, Loud_Less> &>()(
    std::declval<Dlink *>(), std::declval<Dlink *>())));
  static_assert(not noexcept(std::declval<const Compare_Dnode<int, Loud_Less> &>()(std::declval<Dlink *>(), 1)));
  // and with a comparison that returns a bool they are, as before
  static_assert(noexcept(selection_sort(a, n)));
  static_assert(noexcept(insertion_sort(a, 0L, last)));
  static_assert(noexcept(select_pivot(a, 0L, last)));
  static_assert(noexcept(partition(a, 0L, last)));
  static_assert(noexcept(timsort_detail::gallop_left(a[0], a, n, size_t{0}, Aleph::less<int>())));
  static_assert(noexcept(std::declval<const Negate_Compare<int, Aleph::less<int>> &>()(1, 2)));
  static_assert(noexcept(std::declval<const Inversed_Compare<int, Aleph::less<int>> &>()(1, 2)));
  static_assert(noexcept(std::declval<const Compare_Dnode<int, Aleph::less<int>> &>()(
    std::declval<Dlink *>(), std::declval<Dlink *>())));

  selection_sort(a, n, less);
  EXPECT_EQ(Array<int>({a[0], a[1], a[2], a[3], a[4]}), Array<int>({1, 2, 3, 4, 5}));

  // each sort lets the conversion throw, and keeps the items
  const auto fails = [&a] (auto sort)
    {
      const int unsorted[] = {3, 1, 2, 5, 4};
      for (size_t i = 0; i < 5; ++i)
        a[i] = unsorted[i];
      const Arm arm;
      EXPECT_THROW(sort(), Conversion_Error);
      long digits = 0;
      for (const int x : a)
        {
          long unit = 1;
          for (int i = 1; i < x; ++i)
            unit *= 10;
          digits += unit;
        }
      EXPECT_EQ(digits, 11111);
    };
  fails([&] { selection_sort(a, n, less); });
  fails([&] { (void) select_pivot(a, 0L, last, less); });
  fails([&] { (void) partition(a, 0L, last, less); });
  fails([&] { heapsort(a, n, less); });
  fails([&] { faster_heapsort(a, n, less); });
  fails([&] { (void) timsort_detail::gallop_left(a[0], a, n, size_t{0}, less); });
  fails([&] { (void) timsort_detail::gallop_right(a[0], a, n, size_t{0}, less); });
}

// The sorts of lists, which link the nodes again: they take the way that
// undoes when the conversion may throw, and the list keeps every item
TEST(ConditionConversions, SortsOfLists)
{
  const Loud_Less less;
  const auto expect_keeps_every_item = [] (auto & list, auto sort)
    {
      for (const int x : {3, 1, 2, 5, 4})
        list.append(x);
      {
        const Arm arm;
        EXPECT_THROW(sort(list), Conversion_Error);
      }
      EXPECT_EQ(list.size(), 5u);
      EXPECT_EQ(census(list), 11111);
      sort(list); // and it sorts them, when the conversion does not throw
      EXPECT_EQ(list.get_first(), 1);
      EXPECT_EQ(list.get_last(), 5);
      EXPECT_EQ(census(list), 11111);
    };
  DynList<int> l1, l2, l3;
  expect_keeps_every_item(l1, [&less] (DynList<int> & l) { mergesort(l, less); });
  expect_keeps_every_item(l2, [&less] (DynList<int> & l) { insertion_sort(l, less); });
  expect_keeps_every_item(l3, [&less] (DynList<int> & l) { mergeinsertsort(l, less); });
  DynDlist<int> d1, d2, d3;
  expect_keeps_every_item(d1, [&less] (DynDlist<int> & l) { mergesort(l, less); });
  expect_keeps_every_item(d2, [&less] (DynDlist<int> & l) { insertion_sort(l, less); });
  expect_keeps_every_item(d3, [&less] (DynDlist<int> & l) { mergeinsertsort(l, less); });
}

// sort_nodes() and sort_arcs() of a graph, which compare pointers
TEST(ConditionConversions, SortsOfTheNodesAndArcsOfAGraph)
{
  struct By_Info
  {
    Loud_Bool operator () (Edges::Node * p, Edges::Node * q) const noexcept
    {
      return {p->get_info() > q->get_info()};
    }

    Loud_Bool operator () (Edges::Arc * a, Edges::Arc * b) const noexcept
    {
      return {a->get_info() > b->get_info()};
    }
  };
  struct Plain
  {
    bool operator () (Edges::Node * p, Edges::Node * q) const noexcept { return p->get_info() > q->get_info(); }
  };
  static_assert(not noexcept(std::declval<Edges &>().sort_nodes(std::declval<By_Info &>())));
  static_assert(not noexcept(std::declval<Edges &>().sort_arcs(std::declval<By_Info &>())));
  static_assert(noexcept(std::declval<Edges &>().sort_nodes(std::declval<Plain &>())));

  Triangle<Edges> u;
  By_Info by_info;
  {
    const Arm arm;
    EXPECT_THROW(u.g.sort_nodes(by_info), Conversion_Error);
    EXPECT_THROW(u.g.sort_arcs(by_info), Conversion_Error);
  }
  EXPECT_EQ(u.g.get_num_nodes(), 3u);
  EXPECT_EQ(u.g.get_num_arcs(), 3u);
  size_t nodes = 0, arcs = 0;
  for (Edges::Node_Iterator it(u.g); it.has_curr(); it.next_ne())
    ++nodes;
  for (Edges::Arc_Iterator it(u.g); it.has_curr(); it.next_ne())
    ++arcs;
  EXPECT_EQ(nodes, 3u);
  EXPECT_EQ(arcs, 3u);

  u.g.sort_nodes(by_info);
  EXPECT_EQ(u.g.get_first_node()->get_info(), 3);
  u.g.sort_arcs(by_info);
  EXPECT_EQ(u.g.get_first_arc()->get_info(), 3);
}

// The filter of the arcs that Kruskal paints
TEST(ConditionConversions, PaintedArcsOfKruskal)
{
  using Kruskal = Kruskal_Min_Spanning_Tree<Edges, Dft_Dist<Edges>, Arc_Show>;
  using Filter = Kruskal::Paint_Filt<Edges, Arc_Show>;
  static_assert(not noexcept(std::declval<const Filter &>()(std::declval<Edges::Arc *>())));
  using Plain = Kruskal_Min_Spanning_Tree<Edges>::Paint_Filt<Edges, Dft_Show_Arc<Edges>>;
  static_assert(noexcept(std::declval<const Plain &>()(std::declval<Edges::Arc *>())));

  Triangle<Edges> u;
  Arc_Show show;
  const Filter painted(show);
  Edges::Arc * arc = u.g.get_first_arc();
  EXPECT_FALSE(painted(arc));

  // it gives the filter of the caller the arc by its name
  using Named = Two_Calls<Edges::Arc>;
  using Through = Kruskal::Paint_Filt<Edges, Named>;
  static_assert(not noexcept(std::declval<const Through &>()(std::declval<Edges::Arc *>())));
  Named named;
  const Through through(named);
  Named::named = Named::unnamed = 0;
  EXPECT_FALSE(through(arc));
  EXPECT_EQ(Named::named, 1u);
  EXPECT_EQ(Named::unnamed, 0u);

  const Arm arm;
  EXPECT_THROW((void) painted(arc), Conversion_Error);
}
