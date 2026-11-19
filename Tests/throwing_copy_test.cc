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

/** @file throwing_copy_test.cc
    @brief Copies, moves and constructions of the items of the user that
    throw, in the functions that were `noexcept`.

    A `noexcept` function that copies, moves or builds an item of the user
    ends the program (`std::terminate()`) when that operation throws. Each
    function tested here must do one of three things with such an item:
    not be `noexcept`, let the exception reach the caller and leave the
    structure valid; absorb the failure and leave the structure valid; or
    not copy the item at all.

    The items fail when asked to: from the k-th operation on, every copy
    (or move, or default construction) of them throws. Each test tries
    every k, until the operation completes before the k-th one, and
    counts the items alive to catch those that a failure leaks or destroys
    twice. A new node class, zipped container or tree is one more line in
    the list of its typed test.
*/

# include <stdexcept>
# include <string>
# include <tuple>
# include <type_traits>
# include <utility>

# include <gtest/gtest.h>
# include <gtest/gtest-spi.h>

# include <ah-zip.H>
# include <htlist.H>
# include <tpl_array.H>
# include <tpl_dnode.H>
# include <tpl_dynArray.H>
# include <tpl_dynMapTree.H>
# include <tpl_dynMapOhash.H>
# include <tpl_dynSetHash.H>
# include <tpl_odhash.H>
# include <tpl_olhash.H>
# include <tpl_binHeap.H>

using namespace Aleph;

namespace
{
  // What an item throws when the test asks one of its operations to fail
  struct Injected_Failure : public std::runtime_error
  {
    using std::runtime_error::runtime_error;
  };

  // The operations of the items that may fail. While a Failing_From
  // lives, they fail from the k-th on.
  struct Failures
  {
    static inline long countdown = -1;  // that still succeed; negative: none fails
    static inline long fired = 0;       // that failed
    static inline long done = 0;        // that were tried
    static inline long defaults = 0;    // of them, default constructions
    static inline long alive = 0;       // items alive

    static void hit(const char * operation)
    {
      ++done;
      if (countdown == 0)
        {
          ++fired;
          throw Injected_Failure(operation);
        }
      if (countdown > 0)
        --countdown;
    }
  };

  class Failing_From
  {
  public:
    explicit Failing_From(const long k) noexcept
    {
      Failures::countdown = k;
      Failures::fired = 0;
      Failures::done = 0;
    }

    ~Failing_From() { Failures::countdown = -1; }

    Failing_From(const Failing_From &) = delete;
    Failing_From & operator = (const Failing_From &) = delete;
  };

  // An item whose copy fails when asked to. It has no move constructor,
  // so building one from an rvalue copies it too.
  struct Copy_Fails
  {
    long value = 0;

    Copy_Fails() noexcept { ++Failures::alive; }

    Copy_Fails(const long v) noexcept : value(v) { ++Failures::alive; }

    Copy_Fails(const Copy_Fails & item) : value(item.value)
    {
      Failures::hit("copy");
      ++Failures::alive;
    }

    Copy_Fails & operator = (const Copy_Fails & item)
    {
      Failures::hit("copy assignment");
      value = item.value;
      return *this;
    }

    ~Copy_Fails() { --Failures::alive; }

    bool operator < (const Copy_Fails & item) const noexcept { return value < item.value; }

    bool operator == (const Copy_Fails & item) const noexcept { return value == item.value; }
  };

  // An item whose move fails when asked to; its copy does not fail
  struct Move_Fails
  {
    long value = 0;

    Move_Fails() noexcept { ++Failures::alive; }

    Move_Fails(const long v) noexcept : value(v) { ++Failures::alive; }

    Move_Fails(const Move_Fails & item) noexcept : value(item.value) { ++Failures::alive; }

    Move_Fails(Move_Fails && item) : value(item.value)
    {
      Failures::hit("move");
      ++Failures::alive;
    }

    Move_Fails & operator = (const Move_Fails & item) noexcept
    {
      value = item.value;
      return *this;
    }

    Move_Fails & operator = (Move_Fails && item)
    {
      Failures::hit("move assignment");
      value = item.value;
      return *this;
    }

    ~Move_Fails() { --Failures::alive; }

    bool operator < (const Move_Fails & item) const noexcept { return value < item.value; }

    bool operator == (const Move_Fails & item) const noexcept { return value == item.value; }
  };

  // An item whose default construction fails when asked to
  struct Default_Fails
  {
    long value = 0;

    Default_Fails()
    {
      ++Failures::defaults;
      Failures::hit("default construction");
      ++Failures::alive;
    }

    Default_Fails(const long v) noexcept : value(v) { ++Failures::alive; }

    Default_Fails(const Default_Fails & item) noexcept : value(item.value) { ++Failures::alive; }

    Default_Fails & operator = (const Default_Fails & item) noexcept = default;

    ~Default_Fails() { --Failures::alive; }

    bool operator < (const Default_Fails & item) const noexcept { return value < item.value; }

    bool operator == (const Default_Fails & item) const noexcept { return value == item.value; }
  };

  long value_of(const long v) noexcept { return v; }

  long value_of(const Copy_Fails & item) noexcept { return item.value; }

  size_t hash_of(const Copy_Fails & item) noexcept
  {
    return static_cast<size_t>(item.value) * 2654435761u;
  }

  // More operations than any call here makes
  constexpr long Max_Failure_Point = 100000;

  // Calls op(fixture) on a new Fixture for each k = 0, 1, ..., with the
  // operations of the items failing from the k-th on, until op completes
  // before the k-th. A failure must reach the caller if lets_through, and
  // must not otherwise. Then valid(fixture, threw) checks the fixture, and
  // destroying it must leave as many items alive as before building it.
  // An op that makes no operation that can fail tests nothing: it fails
  // the test.
  template <class Fixture, class Op, class Valid>
  void try_every_failure(const std::string & what, const bool lets_through,
                         Op op, Valid valid)
  {
    for (long k = 0; k < Max_Failure_Point; ++k)
      {
        SCOPED_TRACE(what + ", failing from the operation " + std::to_string(k));
        const long alive = Failures::alive;
        long fired = 0;
        {
          Fixture fixture;
          bool threw = false;
          {
            Failing_From failing(k);
            try
              {
                op(fixture);
              }
            catch (const Injected_Failure &)
              {
                threw = true;
              }
            fired = Failures::fired;
          }

          if (lets_through)
            EXPECT_EQ(threw, fired > 0) << "a failure did not reach the caller";
          else
            EXPECT_FALSE(threw) << "a failure reached the caller";
          valid(fixture, threw);
        }
        EXPECT_EQ(Failures::alive, alive) << "items leaked or destroyed twice";
        if (fired == 0)
          {
            if (k == 0)
              ADD_FAILURE() << what << " made no operation that can fail: nothing was tested";
            return;
          }
      }
    ADD_FAILURE() << what << " made more than " << Max_Failure_Point << " operations";
  }

  // The harness: an operation that makes no operation that can fail, as a
  // set_hash_fct() that would not change the function, completes at the
  // first try with the fixture as it was; it must fail the test instead of
  // passing it
  TEST(Failure_Harness, AnOperationThatCannotFailFailsTheTest)
  {
    struct Nothing {};
    EXPECT_NONFATAL_FAILURE(try_every_failure<Nothing>("doing nothing", true,
                                                        [] (Nothing &) {},
                                                        [] (Nothing &, bool) {}),
                            "nothing was tested");
  }

  // =========================================================================
  // The nodes of the binary trees (DECLARE_BINNODE in tpl_binNode.H)
  // =========================================================================

  // A node template and the control data of its nodes
  template <template <class> class N, class C>
  struct Node_Kind
  {
    template <class Key>
    using Node = N<Key>;

    using Control_Data = C;
  };

  // A node, once built, and a key to build it from
  template <class Node, class Key>
  struct Node_Fixture
  {
    Key key = Key(7);
    Node * node = nullptr;

    Node_Fixture() = default;
    Node_Fixture(const Node_Fixture &) = delete;
    Node_Fixture & operator = (const Node_Fixture &) = delete;
    ~Node_Fixture() { delete node; }
  };

  template <class Kind>
  struct Node_Construction : public ::testing::Test
  {
    // Node(Key &&) moves the key into the node; a key without a move
    // constructor is copied
    template <class Key>
    static void check_from_rvalue(const std::string & what)
    {
      using Node = typename Kind::template Node<Key>;
      using Fixture = Node_Fixture<Node, Key>;
      if constexpr (std::is_nothrow_constructible_v<Node, Key &&>)
        ADD_FAILURE() << what << " is noexcept, but the key may throw";
      else
        try_every_failure<Fixture>(what, true,
                                   [] (Fixture & f) { f.node = new Node(std::move(f.key)); },
                                   [] (Fixture & f, const bool threw)
                                   {
                                     EXPECT_EQ(f.key.value, 7);
                                     EXPECT_EQ(f.node == nullptr, threw);
                                     if (f.node != nullptr)
                                       {
                                         EXPECT_EQ(KEY(f.node).value, 7);
                                       }
                                   });
    }
  };

  using Node_Kinds = ::testing::Types<Node_Kind<BinNode, Empty_Node>,
                                      Node_Kind<BinNodeVtl, Empty_Node>,
                                      Node_Kind<AvlNode, AvlNode_Data>,
                                      Node_Kind<AvlNodeVtl, AvlNode_Data>,
                                      Node_Kind<AvlNodeRk, AvlNodeRk_Data>,
                                      Node_Kind<RbNode, RbNode_Data>,
                                      Node_Kind<RbNodeVtl, RbNode_Data>,
                                      Node_Kind<RbNodeRk, RbNodeRk_Data>,
                                      Node_Kind<TreapNode, TreapNode_Data>,
                                      Node_Kind<Treap_Rk_Node, TreapRkNode_Data>,
                                      Node_Kind<BinNodeXt, BinNodeXt_Data>,
                                      Node_Kind<RandNode, BinNodeXt_Data>,
                                      Node_Kind<BinHeapNode, BinHeapNode_Data>>;
  TYPED_TEST_SUITE(Node_Construction, Node_Kinds);

  TYPED_TEST(Node_Construction, FromAnRvalueLetsTheFailureThrough)
  {
    this->template check_from_rvalue<Copy_Fails>("Node(Key &&) with a key that is copied");
    this->template check_from_rvalue<Move_Fails>("Node(Key &&) with a key that is moved");
  }

  // Node(const Control_Data &) builds the key by default
  TYPED_TEST(Node_Construction, FromControlDataLetsTheFailureThrough)
  {
    using Node = typename TypeParam::template Node<Default_Fails>;
    using Control_Data = typename TypeParam::Control_Data;
    using Fixture = Node_Fixture<Node, long>;
    if constexpr (std::is_nothrow_constructible_v<Node, const Control_Data &>)
      ADD_FAILURE() << "Node(const Control_Data &) is noexcept, but the key may throw";
    else
      try_every_failure<Fixture>("Node(const Control_Data &)", true,
                                 [] (Fixture & f) { f.node = new Node(Control_Data()); },
                                 [] (Fixture & f, const bool threw)
                                 {
                                   EXPECT_EQ(f.node == nullptr, threw);
                                 });
  }

  // The keys whose move and default construction do not throw keep the
  // noexcept, and so the code that they had
  static_assert(std::is_nothrow_constructible_v<AvlNode<int>, int &&>);
  static_assert(std::is_nothrow_constructible_v<RbNode<std::string>, std::string &&>);
  static_assert(std::is_nothrow_constructible_v<BinNode<std::string>, const Empty_Node &>);

  // =========================================================================
  // Dnode (tpl_dnode.H)
  // =========================================================================

  // A list with the nodes 1, 2 and 3, the second one to move
  struct Dnode_List
  {
    Dnode<Move_Fails> head;
    Dnode<Move_Fails> * middle = nullptr;
    Dnode<Move_Fails> * moved = nullptr;  // built from middle, if it was

    Dnode_List()
    {
      for (long v = 1; v <= 3; ++v)
        {
          auto * p = new Dnode<Move_Fails>(Move_Fails(v));
          head.append(p);
          if (v == 2)
            middle = p;
        }
    }

    Dnode_List(const Dnode_List &) = delete;
    Dnode_List & operator = (const Dnode_List &) = delete;

    ~Dnode_List()
    {
      while (not head.is_empty())
        delete head.remove_first_ne();
      if (moved != nullptr)  // middle left the list
        delete middle;
    }

    // The node in the second place and whether the values are 1, 2, 3
    Dnode<Move_Fails> * second(bool & values_ok) const
    {
      Array<long> values;
      for (auto * p = head.get_next(); p != &head; p = p->get_next())
        values.append(p->get_data().value);
      values_ok = values.size() == 3 and values(0) == 1 and values(1) == 2 and values(2) == 3;
      return head.get_next()->get_next();
    }
  };

  TEST(Dnode_Construction, ByDefaultLetsTheFailureThrough)
  {
    using Node = Dnode<Default_Fails>;
    using Fixture = Node_Fixture<Node, long>;
    if constexpr (std::is_nothrow_default_constructible_v<Node>)
      ADD_FAILURE() << "Dnode() is noexcept, but the item may throw";
    else
      try_every_failure<Fixture>("Dnode()", true,
                                 [] (Fixture & f) { f.node = new Node; },
                                 [] (Fixture & f, const bool threw)
                                 {
                                   EXPECT_EQ(f.node == nullptr, threw);
                                 });
  }

  // The moved node takes the place of the other one in its list; a move
  // that fails must leave the list as it was
  TEST(Dnode_Construction, ByMoveLetsTheFailureThroughAndKeepsTheList)
  {
    if constexpr (std::is_nothrow_move_constructible_v<Dnode<Move_Fails>>)
      ADD_FAILURE() << "Dnode(Dnode &&) is noexcept, but the item may throw";
    else
      try_every_failure<Dnode_List>("Dnode(Dnode &&)", true,
                                    [] (Dnode_List & l)
                                    {
                                      l.moved = new Dnode<Move_Fails>(std::move(*l.middle));
                                    },
                                    [] (Dnode_List & l, const bool threw)
                                    {
                                      bool values_ok = false;
                                      Dnode<Move_Fails> * second = l.second(values_ok);
                                      EXPECT_TRUE(values_ok);
                                      EXPECT_EQ(l.moved == nullptr, threw);
                                      EXPECT_EQ(second, threw ? l.middle : l.moved);
                                      if (not threw)
                                        {
                                          EXPECT_TRUE(l.middle->is_empty());
                                        }
                                    });
  }

  static_assert(std::is_nothrow_default_constructible_v<Dnode<std::string>>);
  static_assert(std::is_nothrow_move_constructible_v<Dnode<std::string>>);
  static_assert(std::is_nothrow_move_constructible_v<Dnode<int>>);

  // =========================================================================
  // The zip iterators (ah-zip.H): get_curr_ne() copies the items into a
  // tuple
  // =========================================================================

  // The items 1, 2 and 3 in a container of type C
  template <class C>
  C one_two_three()
  {
    C c;
    for (long v = 1; v <= 3; ++v)
      c.append(typename C::Item_Type(v));
    return c;
  }

  template <class Tuple, size_t... I>
  bool items_are(const Tuple & t, const long v, std::index_sequence<I...>)
  {
    return (... and (value_of(std::get<I>(t)) == v));
  }

  // The containers to zip, and whether the iterator enumerates them
  template <bool Enumerated, class... Cs>
  struct Zip_Of
  {
    using Iterator = std::conditional_t<Enumerated, EnumZipIterator<Cs...>, ZipIterator<Cs...>>;

    // The containers and an iterator on their second items
    struct Fixture
    {
      std::tuple<Cs...> containers{one_two_three<Cs>()...};
      Iterator it = std::apply([] (const Cs & ... c) { return Iterator(c...); }, containers);

      Fixture() { it.next_ne(); }
    };

    // Whether the iterator is on the second items
    static void on_the_second_items(const Iterator & it)
    {
      ASSERT_TRUE(it.has_curr());
      const auto t = it.get_curr_ne();
      EXPECT_TRUE(items_are(t, 2, std::make_index_sequence<sizeof...(Cs)>()));
      if constexpr (Enumerated)
        {
          EXPECT_EQ(std::get<sizeof...(Cs)>(t), 1u);
        }
    }
  };

  template <class Zip>
  struct Zip_Current : public ::testing::Test {};

  using Zips = ::testing::Types<Zip_Of<false, DynList<Copy_Fails>>,
                                Zip_Of<false, DynList<Copy_Fails>, DynList<long>>,
                                Zip_Of<false, DynList<long>, DynArray<Copy_Fails>>,
                                Zip_Of<false, DynArray<Copy_Fails>, DynList<Copy_Fails>,
                                       DynList<Copy_Fails>>,
                                Zip_Of<true, DynList<Copy_Fails>>,
                                Zip_Of<true, DynList<Copy_Fails>, DynList<long>>,
                                Zip_Of<true, DynList<long>, DynArray<Copy_Fails>>,
                                Zip_Of<true, DynArray<Copy_Fails>, DynList<Copy_Fails>,
                                       DynList<Copy_Fails>>>;
  TYPED_TEST_SUITE(Zip_Current, Zips);

  TYPED_TEST(Zip_Current, LetsTheFailureThroughAndKeepsThePosition)
  {
    using Iterator = typename TypeParam::Iterator;
    using Fixture = typename TypeParam::Fixture;
    if constexpr (noexcept(std::declval<const Iterator &>().get_curr_ne()))
      ADD_FAILURE() << "get_curr_ne() is noexcept, but copying an item may throw";
    else
      try_every_failure<Fixture>("get_curr_ne()", true,
                                 [] (Fixture & f) { (void) f.it.get_curr_ne(); },
                                 [] (Fixture & f, bool)
                                 {
                                   TypeParam::on_the_second_items(f.it);
                                 });
  }

  static_assert(noexcept(std::declval<const ZipIterator<DynList<int>, DynList<long>> &>()
                         .get_curr_ne()));
  static_assert(noexcept(std::declval<const ZipIterator<DynList<int>> &>().get_curr_ne()));
  static_assert(noexcept(std::declval<const EnumZipIterator<DynList<int>, DynList<double>> &>()
                         .get_curr_ne()));

  // The reading of a DynArray may allocate, on a modifiable array, so its
  // iterator is not noexcept: neither is the zip that reads one
  static_assert(not noexcept(std::declval<const ZipIterator<DynList<int>, DynArray<long>> &>()
                             .get_curr_ne()));
  static_assert(not noexcept(std::declval<const EnumZipIterator<DynArray<int>> &>()
                             .get_curr_ne()));

  // =========================================================================
  // The linear hash table (tpl_linHash.H): its lists are Dnode<Key>
  // headers, and cutting the array resets those that leave it, which
  // copies a Key, inside empty() and remove(), which are noexcept
  // =========================================================================

  constexpr long Keys = 300;  // enough to make a table of 101 lists grow

  struct Grown_Table
  {
    DynSetLinHash<Copy_Fails> * table = new DynSetLinHash<Copy_Fails>(101, hash_of);

    Grown_Table()
    {
      for (long k = 0; k < Keys; ++k)
        table->insert(Copy_Fails(k));
    }

    Grown_Table(const Grown_Table &) = delete;
    Grown_Table & operator = (const Grown_Table &) = delete;

    ~Grown_Table() { delete table; }

    // Whether the table is empty and still works
    void empty_and_usable()
    {
      ASSERT_NE(table, nullptr);
      EXPECT_TRUE(table->is_empty());
      EXPECT_EQ(table->size(), 0u);
      for (long k = 0; k < Keys; ++k)
        EXPECT_FALSE(table->contains(Copy_Fails(k))) << "key " << k;
      for (long k = 0; k < Keys; ++k)
        EXPECT_NE(table->insert(Copy_Fails(k)), nullptr) << "key " << k;
      EXPECT_EQ(table->size(), static_cast<size_t>(Keys));
      for (long k = 0; k < Keys; ++k)
        EXPECT_TRUE(table->contains(Copy_Fails(k))) << "key " << k;
    }
  };

  TEST(Linear_Hash_Table, GrowsInTheseTests)
  {
    Grown_Table t;
    EXPECT_GT(t.table->capacity(), 101u);
  }

  TEST(Linear_Hash_Table, EmptyAbsorbsTheFailure)
  {
    try_every_failure<Grown_Table>("empty()", false,
                                   [] (Grown_Table & t) { t.table->empty(); },
                                   [] (Grown_Table & t, bool) { t.empty_and_usable(); });
  }

  TEST(Linear_Hash_Table, DestructorAbsorbsTheFailure)
  {
    try_every_failure<Grown_Table>("~DynSetLinHash()", false,
                                   [] (Grown_Table & t)
                                   {
                                     delete t.table;
                                     t.table = nullptr;
                                   },
                                   [] (Grown_Table & t, bool) { EXPECT_EQ(t.table, nullptr); });
  }

  // Each remove may contract the table
  TEST(Linear_Hash_Table, RemoveAbsorbsTheFailure)
  {
    try_every_failure<Grown_Table>("remove()", false,
                                   [] (Grown_Table & t)
                                   {
                                     for (long k = Keys - 1; k >= 0; --k)
                                       t.table->remove(t.table->search(Copy_Fails(k)));
                                   },
                                   [] (Grown_Table & t, bool) { t.empty_and_usable(); });
  }

  // DynMapLinHash searches by the key alone: it copies no key and builds no
  // Data
  TEST(Linear_Hash_Map, SearchCopiesNothing)
  {
    DynMapLinHash<Copy_Fails, Default_Fails> map(101, hash_of);
    for (long k = 0; k < Keys; k += 2)
      (void) map.insert(Copy_Fails(k), Default_Fails(k));
    const auto & const_map = map;

    for (long k = -1; k <= Keys; ++k)
      {
        const Copy_Fails key(k);
        const bool in = k >= 0 and k < Keys and k % 2 == 0;
        Failures::done = 0;
        auto * p = map.search(key);
        const auto * q = const_map.search(key);
        const bool has = map.has(key);
        const bool contains = map.contains(key);
        EXPECT_EQ(Failures::done, 0) << "searching for " << k << " copied a key or built a Data";
        EXPECT_EQ(p != nullptr, in) << "key " << k;
        EXPECT_EQ(q, p) << "key " << k;
        EXPECT_EQ(has, in) << "key " << k;
        EXPECT_EQ(contains, in) << "key " << k;
        if (p != nullptr)
          {
            EXPECT_EQ(p->second.value, k);
          }
      }
  }


  // =========================================================================
  // set_hash_fct() and the copies of the keys. A map of tpl_dynSetHash.H
  // given a function of pairs, which gives no hash of a key alone, searches
  // for a pair built from the key, a copy of it and a Data built by
  // default: their failure reaches the caller and the map keeps its pairs.
  // A function of the keys brings back the search that copies nothing. An
  // open table with keys places them again with a new function, as
  // rehash() does, which copies them: if a copy fails, the table keeps its
  // function and its keys.
  // =========================================================================

  size_t other_hash_of(const Copy_Fails & item) noexcept
  {
    return static_cast<size_t>(item.value) * 40503u + 17;
  }

  // Another function than hash_of(), which the maps are built with: a
  // search by the hash of the keys that the map was built with misses
  size_t pair_hash_of(const std::pair<Copy_Fails, Default_Fails> & p) noexcept
  {
    return other_hash_of(p.first);
  }

  size_t second_hash_of(const Copy_Fails & item) noexcept
  {
    return static_cast<size_t>(item.value) * 3 + 11;
  }

  // A map that searches by pairs, with the pairs (k, k) of the even k
  template <class Map>
  struct Map_By_Pairs
  {
    Map map{101, hash_of};

    Map_By_Pairs()
    {
      map.set_hash_fct(pair_hash_of);
      for (long k = 0; k < Keys; k += 2)
        (void) map.insert(Copy_Fails(k), Default_Fails(k));
    }

    void holds_its_pairs()
    {
      for (long k = -1; k <= Keys; ++k)
        {
          const Copy_Fails key(k);
          const auto * p = map.search(key);
          EXPECT_EQ(p != nullptr, k >= 0 and k < Keys and k % 2 == 0) << "key " << k;
          if (p != nullptr)
            EXPECT_EQ(p->second.value, k);
        }
    }
  };

  template <class Map>
  void search_by_pairs_lets_the_failure_through(const std::string & name)
  {
    static_assert(not noexcept(std::declval<Map &>().search(std::declval<const Copy_Fails &>())));
    for (const long key : {4L, 5L})  // a key of the map and one that is not
      try_every_failure<Map_By_Pairs<Map>>(name + "::search() by pairs", true,
                                           [key] (Map_By_Pairs<Map> & t)
                                           {
                                             const Copy_Fails k(key);
                                             (void) t.map.search(k);
                                           },
                                           [] (Map_By_Pairs<Map> & t, bool) { t.holds_its_pairs(); });
  }

  TEST(Hash_Map_By_Pairs, SearchLetsTheFailureThrough)
  {
    search_by_pairs_lets_the_failure_through<DynMapLinHash<Copy_Fails, Default_Fails>>("DynMapLinHash");
    search_by_pairs_lets_the_failure_through<DynMapHash<Copy_Fails, Default_Fails>>("DynMapHash");
  }

  template <class Map>
  void search_by_keys_copies_nothing()
  {
    Map map(101, hash_of);
    map.set_hash_fct(pair_hash_of);
    map.set_hash_fct(other_hash_of);
    for (long k = 0; k < Keys; k += 2)
      (void) map.insert(Copy_Fails(k), Default_Fails(k));

    for (long k = -1; k <= Keys; ++k)
      {
        const Copy_Fails key(k);
        Failures::done = 0;
        const auto * p = map.search(key);
        const bool has = map.has(key);
        EXPECT_EQ(Failures::done, 0) << "searching for " << k << " copied a key or built a Data";
        EXPECT_EQ(p != nullptr, k >= 0 and k < Keys and k % 2 == 0) << "key " << k;
        EXPECT_EQ(has, p != nullptr) << "key " << k;
      }
  }

  TEST(Hash_Map_By_Keys, SearchCopiesNothingAfterAFunctionOfPairs)
  {
    search_by_keys_copies_nothing<DynMapLinHash<Copy_Fails, Default_Fails>>();
    search_by_keys_copies_nothing<DynMapHash<Copy_Fails, Default_Fails>>();
  }

  // An open table with the keys [0, Keys)
  template <class Table>
  struct Open_Table
  {
    Table table = make();

    static Table make()
    {
      if constexpr (std::is_same_v<Table, ODhashTable<Copy_Fails>>)
        return Table(101, hash_of, second_hash_of);
      else
        return Table(101, hash_of);
    }

    Open_Table()
    {
      for (long k = 0; k < Keys; ++k)
        (void) table.insert(Copy_Fails(k));
    }

    void holds_its_keys()
    {
      EXPECT_EQ(table.size(), static_cast<size_t>(Keys));
      for (long k = -1; k <= Keys; ++k)
        EXPECT_EQ(table.search(Copy_Fails(k)) != nullptr, k >= 0 and k < Keys) << "key " << k;
    }
  };

  // Whether the hash function fct of a table is the function fct_ptr
  template <class Hash_Fct>
  bool is_the_function(const Hash_Fct & fct, size_t (*fct_ptr)(const Copy_Fails &))
  {
    // std-interop: the open tables keep their functions in a std::function
    const auto * target = fct.template target<size_t (*)(const Copy_Fails &)>();
    return target != nullptr and *target == fct_ptr;
  }

  // After each try, the table has the function that it had if a copy
  // failed, and the new one otherwise
  TEST(Open_Hash_Table, SetHashFctKeepsItsFunctionWhenACopyFails)
  {
    using OL = Open_Table<OLhashTable<Copy_Fails>>;
    using OD = Open_Table<ODhashTable<Copy_Fails>>;
    try_every_failure<OL>("OLhashTable::set_hash_fct()", true,
                          [] (OL & t) { t.table.set_hash_fct(other_hash_of); },
                          [] (OL & t, const bool threw)
                          {
                            t.holds_its_keys();
                            EXPECT_TRUE(is_the_function(t.table.get_hash_fct(),
                                                        threw ? hash_of : other_hash_of));
                          });
    try_every_failure<OD>("ODhashTable::set_hash_fct()", true,
                          [] (OD & t) { t.table.set_hash_fct(other_hash_of); },
                          [] (OD & t, const bool threw)
                          {
                            t.holds_its_keys();
                            EXPECT_TRUE(is_the_function(t.table.get_hash_fct(),
                                                        threw ? hash_of : other_hash_of));
                            EXPECT_TRUE(is_the_function(t.table.get_second_hash_fct(),
                                                        second_hash_of));
                          });
    try_every_failure<OD>("ODhashTable::set_second_hash_fct()", true,
                          [] (OD & t) { t.table.set_second_hash_fct(other_hash_of); },
                          [] (OD & t, const bool threw)
                          {
                            t.holds_its_keys();
                            EXPECT_TRUE(is_the_function(t.table.get_hash_fct(), hash_of));
                            EXPECT_TRUE(is_the_function(t.table.get_second_hash_fct(),
                                                        threw ? second_hash_of : other_hash_of));
                          });
  }


  // =========================================================================
  // The maps of tpl_dynMapOhash.H search for a pair built from the key, a
  // copy of it and a Data built by default, as their table searches for
  // pairs: their failure reaches the caller, and the map keeps its pairs.
  // =========================================================================

  template <class Map>
  struct Open_Map_With_Pairs
  {
    Map map{101, hash_of, second_hash_of};

    Open_Map_With_Pairs()
    {
      for (long k = 0; k < Keys; k += 2)
        (void) map.insert(Copy_Fails(k), Default_Fails(k));
    }

    void holds_its_pairs()
    {
      for (long k = -1; k <= Keys; ++k)
        {
          const Copy_Fails key(k);
          const auto * p = map.search(key);
          EXPECT_EQ(p != nullptr, k >= 0 and k < Keys and k % 2 == 0) << "key " << k;
          if (p != nullptr)
            EXPECT_EQ(p->second.value, k);
        }
    }
  };

  template <class Map>
  void open_map_search_lets_the_failure_through(const std::string & name)
  {
    using Fixture = Open_Map_With_Pairs<Map>;
    static_assert(not noexcept(std::declval<Map &>().search(std::declval<const Copy_Fails &>())));
    static_assert(not noexcept(std::declval<const Map &>().has(std::declval<const Copy_Fails &>())));
    static_assert(not noexcept(std::declval<const Map &>().contains(std::declval<Copy_Fails>())));
    for (const long key : {4L, 5L})  // a key of the map and one that is not
      {
        try_every_failure<Fixture>(name + "::search()", true,
                                   [key] (Fixture & t)
                                   {
                                     const Copy_Fails k(key);
                                     (void) t.map.search(k);
                                   },
                                   [] (Fixture & t, bool) { t.holds_its_pairs(); });
        try_every_failure<Fixture>(name + "::has()", true,
                                   [key] (Fixture & t)
                                   {
                                     const Copy_Fails k(key);
                                     (void) t.map.has(k);
                                   },
                                   [] (Fixture & t, bool) { t.holds_its_pairs(); });
      }
  }

  TEST(Open_Hash_Map, SearchLetsTheFailureThrough)
  {
    open_map_search_lets_the_failure_through<MapOLhash<Copy_Fails, Default_Fails>>("MapOLhash");
    open_map_search_lets_the_failure_through<MapODhash<Copy_Fails, Default_Fails>>("MapODhash");
    static_assert(noexcept(std::declval<MapOLhash<int, int> &>().search(1)));
    static_assert(noexcept(std::declval<const MapODhash<int, int> &>().has(1)));
  }

  // =========================================================================
  // DynMapTree (tpl_dynMapTree.H): search(), has(), contains() and find()
  // search by the key alone; they used to build a pair, with a copy of the
  // key and a Data, inside noexcept. remove_key() still builds one, out of
  // noexcept: what that throws must reach the caller.
  // =========================================================================

  // A tree type. The splay trees search for a pair built from the key, as
  // only their own search moves what it finds to the root; besides, they
  // build a whole node, with a key built by default, as the header of each
  // splay (splay_impl() of tpl_splay_tree.H and tpl_splay_treeRk.H).
  template <template <typename, class> class T, bool Splays = false>
  struct Tree_Kind
  {
    template <class Key, class Data>
    using Map = DynMapTree<Key, Data, T>;

    static constexpr bool splays = Splays;
  };

  constexpr long Pairs = 100;  // the keys 0, 2, ..., 2 * (Pairs - 1)

  bool in_the_map(const long k) noexcept
  {
    return k >= 0 and k < 2 * Pairs and k % 2 == 0;
  }

  // A map with the keys of in_the_map(), inserted out of order, so that
  // the tree is of any shape
  template <class Map>
  struct Map_Fixture
  {
    Map map;

    Map_Fixture()
    {
      for (long i = 0; i < Pairs; ++i)
        {
          const long k = 2 * ((i * 37) % Pairs);
          (void) map.insert(Copy_Fails(k), typename Map::Value_Type(k));
        }
    }
  };

  template <class Kind>
  struct Map_Search : public ::testing::Test {};

  using Tree_Kinds = ::testing::Types<Tree_Kind<BinTree>, Tree_Kind<Avl_Tree>,
                                      Tree_Kind<Avl_Tree_Rk>, Tree_Kind<Rb_Tree>,
                                      Tree_Kind<Rb_Tree_Rk>, Tree_Kind<TdRbTree>,
                                      Tree_Kind<TdRbTreeRk>, Tree_Kind<HtdRbTree>,
                                      Tree_Kind<HtdRbTreeRk>, Tree_Kind<Treap>,
                                      Tree_Kind<Treap_Rk>, Tree_Kind<Rand_Tree>,
                                      Tree_Kind<Splay_Tree, true>,
                                      Tree_Kind<Splay_Tree_Rk, true>>;
  TYPED_TEST_SUITE(Map_Search, Tree_Kinds);

  // But in the splay trees, the searches copy no key and build no Data
  TYPED_TEST(Map_Search, SearchesCopyNothing)
  {
    using Map = typename TypeParam::template Map<Copy_Fails, Default_Fails>;
    Map_Fixture<Map> f;
    auto & map = f.map;
    const auto & const_map = f.map;
    for (long k = -1; k <= 2 * Pairs; ++k)
      {
        const Copy_Fails key(k);
        const bool in = in_the_map(k);
        Failures::done = 0;
        auto * p = map.search(key);
        const auto * q = const_map.search(key);
        auto * r = map.search(Copy_Fails(k));
        const auto * s = const_map.search(Copy_Fails(k));
        // each one apart: with and, a key that is not in the map would skip
        // the overload for an rvalue
        const bool has = const_map.has(key);
        const bool has_rvalue = const_map.has(Copy_Fails(k));
        const bool contains = const_map.contains(key);
        const bool contains_rvalue = const_map.contains(Copy_Fails(k));
        if (in)
          {
            EXPECT_EQ(map.find(key).value, k);
            EXPECT_EQ(const_map.find(key).value, k);
          }
        else
          {
            EXPECT_THROW((void) map.find(key), std::domain_error) << "key " << k;
            EXPECT_THROW((void) const_map.find(key), std::domain_error) << "key " << k;
          }
        if constexpr (not TypeParam::splays)
          {
            EXPECT_EQ(Failures::done, 0) << "searching for " << k << " copied a key or built a Data";
          }
        EXPECT_EQ(p != nullptr, in) << "key " << k;
        EXPECT_TRUE(q == p and r == p and s == p) << "key " << k;
        EXPECT_EQ(has, in) << "key " << k;
        EXPECT_EQ(has_rvalue, in) << "key " << k;
        EXPECT_EQ(contains, in) << "key " << k;
        EXPECT_EQ(contains_rvalue, in) << "key " << k;
        if (p != nullptr)
          {
            EXPECT_EQ(p->second.value, k);
          }
      }
    EXPECT_TRUE(map.verify());
    EXPECT_EQ(map.size(), static_cast<size_t>(Pairs));
  }

  // What building the pair throws, in the searches of a splay tree and in
  // remove_key(), must reach the caller and leave the map as it was. The
  // data are long, so that the header of each splay, a node whose key is
  // built by default inside noexcept, never fails. The search of the other
  // trees builds nothing, so nothing of it can fail: SearchesCopyNothing
  // checks it.
  TYPED_TEST(Map_Search, SearchLetsTheFailureThrough)
  {
    using Map = typename TypeParam::template Map<Copy_Fails, long>;
    using Fixture = Map_Fixture<Map>;
    if constexpr (not TypeParam::splays)
      GTEST_SKIP() << "the search of this tree builds no pair";
    else if constexpr (noexcept(std::declval<Map &>().search(Copy_Fails(0))))
      ADD_FAILURE() << "search() is noexcept, but it copies a key that may throw";
    else
      for (const long k : {0L, 1L, 98L})
        try_every_failure<Fixture>("search(" + std::to_string(k) + ")", true,
                                   [k] (Fixture & f) { (void) f.map.search(Copy_Fails(k)); },
                                   [] (Fixture & f, bool)
                                   {
                                     EXPECT_TRUE(f.map.verify());
                                     EXPECT_EQ(f.map.size(), static_cast<size_t>(Pairs));
                                   });
  }

  // A key that is not in the map, as 99, is refused with std::domain_error
  TYPED_TEST(Map_Search, RemoveKeyLetsTheFailureThrough)
  {
    using Map = typename TypeParam::template Map<Copy_Fails, long>;
    using Fixture = Map_Fixture<Map>;
    for (const long k : {0L, 98L, 99L})
      {
        bool refused = false;  // remove_key() threw std::domain_error
        try_every_failure<Fixture>("remove_key(" + std::to_string(k) + ")", true,
                                   [k, &refused] (Fixture & f)
                                   {
                                     refused = false;
                                     try
                                       {
                                         f.map.remove_key(Copy_Fails(k));
                                       }
                                     catch (const std::domain_error &)
                                       {
                                         refused = true;
                                       }
                                   },
                                   [k, &refused] (Fixture & f, const bool threw)
                                   {
                                     if (not threw)
                                       EXPECT_EQ(refused, not in_the_map(k))
                                         << "std::domain_error, for a key not in the map alone";
                                     const bool removed = in_the_map(k) and not threw;
                                     EXPECT_EQ(f.map.has(Copy_Fails(k)),
                                               in_the_map(k) and not removed);
                                     EXPECT_EQ(f.map.size(),
                                               static_cast<size_t>(removed ? Pairs - 1 : Pairs));
                                     EXPECT_TRUE(f.map.verify());
                                   });
      }
  }

  static_assert(noexcept(std::declval<const DynMapTree<int, int> &>().search(1)));
  static_assert(noexcept(std::declval<DynMapTree<long, Array<int>, Rb_Tree> &>().has(1L)));
  static_assert(noexcept(std::declval<DynMapTree<long, int, Splay_Tree> &>().has(1L)));

  // =========================================================================
  // The removals of DynSetTree that give a copy of the key, del() and
  // remove_pos(): the node leaves the tree before that copy, which leaked
  // it and left it counted when it threw
  // =========================================================================

  // A map and a ranked set with the keys 0 to 9
  struct Removal_Fixture
  {
    DynMapTree<long, Copy_Fails> map;
    DynSetTree<Copy_Fails, Treap_Rk> ranked;

    Removal_Fixture()
    {
      for (long k = 0; k < 10; ++k)
        {
          (void) map.insert(k, Copy_Fails(k));
          (void) ranked.insert(Copy_Fails(k));
        }
    }
  };

  // remove() of a map gives the data of the pair that del() copies
  TEST(Set_Removal, DelFreesTheNodeWhenItsCopyFails)
  {
    try_every_failure<Removal_Fixture>("remove(5)", true,
                                       [] (Removal_Fixture & f)
                                       {
                                         (void) f.map.remove(5);
                                       },
                                       [] (Removal_Fixture & f, bool)
                                       {
                                         const bool removed = not f.map.contains(5);
                                         EXPECT_EQ(f.map.size(), removed ? 9u : 10u);
                                         EXPECT_TRUE(f.map.verify());
                                       });
  }

  TEST(Set_Removal, RemovePosFreesTheNodeWhenItsCopyFails)
  {
    try_every_failure<Removal_Fixture>("remove_pos(3)", true,
                                       [] (Removal_Fixture & f)
                                       {
                                         EXPECT_EQ(f.ranked.remove_pos(3).value, 3);
                                       },
                                       [] (Removal_Fixture & f, bool)
                                       {
                                         EXPECT_FALSE(f.ranked.contains(Copy_Fails(3)));
                                         EXPECT_EQ(f.ranked.size(), 9u);
                                         EXPECT_TRUE(f.ranked.verify());
                                       });
  }

  // =========================================================================
  // The zip iterators: the reading of a container may throw too, and the
  // copies of the items come from constant items
  // =========================================================================

  // A container of the user whose reading fails when asked to, of items
  // whose copy cannot throw
  struct Reading_Fails
  {
    using Item_Type = long;

    long items[3] = {1, 2, 3};

    struct Iterator
    {
      using Item_Type = long;

      const Reading_Fails * container;
      size_t i = 0;

      Iterator(const Reading_Fails & c) noexcept : container(&c) {}

      bool has_curr() const noexcept { return i < 3; }

      const long & get_curr_ne() const
      {
        Failures::hit("reading");
        return container->items[i];
      }

      const long & get_curr() const { return get_curr_ne(); }

      void next_ne() noexcept { ++i; }

      void next() noexcept { next_ne(); }
    };
  };

  // The zip iterators took the copy of a long as all that could throw, and
  // read the container inside noexcept
  TEST(Zip_Reading, LetsTheFailureOfTheReadingThrough)
  {
    using Zip = ZipIterator<Reading_Fails, DynList<long>>;
    using Enum_Zip = EnumZipIterator<Reading_Fails, DynList<long>>;
    EXPECT_FALSE(noexcept(std::declval<const Zip &>().get_curr_ne()));
    EXPECT_FALSE(noexcept(std::declval<const Enum_Zip &>().get_curr_ne()));
    EXPECT_FALSE(noexcept(std::declval<const ZipIterator<Reading_Fails> &>().get_curr_ne()));

    const Reading_Fails container;
    const auto list = one_two_three<DynList<long>>();
    const Zip zip(container, list);
    const Enum_Zip enum_zip(container, list);
    {
      Failing_From failing(0);
      EXPECT_THROW((void) zip.get_curr_ne(), Injected_Failure);
      EXPECT_THROW((void) enum_zip.get_curr_ne(), Injected_Failure);
    }
    EXPECT_EQ(std::get<0>(zip.get_curr_ne()), 1);
    EXPECT_EQ(std::get<1>(enum_zip.get_curr_ne()), 1);
  }

  // An item whose copy from a constant item cannot throw, and whose copy
  // from one that is not constant fails when asked to
  struct Copy_From_Mutable_Fails
  {
    long value = 0;

    Copy_From_Mutable_Fails(const long v) noexcept : value(v) {}

    Copy_From_Mutable_Fails(const Copy_From_Mutable_Fails & item) noexcept
      : value(item.value) {}

    Copy_From_Mutable_Fails(Copy_From_Mutable_Fails & item) : value(item.value)
    {
      Failures::hit("copy from an item that is not constant");
    }

    Copy_From_Mutable_Fails &
    operator = (const Copy_From_Mutable_Fails &) noexcept = default;
  };

  // get_curr_ne() of the zip of several containers concatenates the copy
  // of the first item with the others: it copied it again from a tuple that
  // was not constant, with the copy that the noexcept did not check
  TEST(Zip_Current_Copies, CopiesFromConstantItems)
  {
    using C = DynList<Copy_From_Mutable_Fails>;
    const C a = one_two_three<C>(), b = one_two_three<C>(), c = one_two_three<C>();
    const ZipIterator<C, C, C> zip(a, b, c);
    const EnumZipIterator<C, C, C> enum_zip(a, b, c);
    Failing_From failing(0);
    const auto t = zip.get_curr_ne();
    const auto u = enum_zip.get_curr_ne();
    EXPECT_EQ(Failures::fired, 0);
    EXPECT_EQ(std::get<0>(t).value + std::get<1>(t).value + std::get<2>(t).value, 3);
    EXPECT_EQ(std::get<2>(u).value, 1);
  }

  // An item that can be copied only from one that is not constant, with a
  // copy that fails when asked to, and moved
  struct Mutable_Copy_Only
  {
    long value = 0;

    Mutable_Copy_Only(const long v) noexcept : value(v) {}

    Mutable_Copy_Only(Mutable_Copy_Only & item) : value(item.value)
    {
      Failures::hit("copy from an item that is not constant");
    }

    Mutable_Copy_Only(const Mutable_Copy_Only &) = delete;

    Mutable_Copy_Only(Mutable_Copy_Only && item) noexcept : value(item.value) {}
  };

  // A container of the items 1, 2 and 3, which its iterator gives by value
  struct By_Value_Items
  {
    using Item_Type = Mutable_Copy_Only;

    struct Iterator
    {
      using Item_Type = Mutable_Copy_Only;

      long i = 0;

      Iterator(const By_Value_Items &) noexcept {}

      bool has_curr() const noexcept { return i < 3; }

      Mutable_Copy_Only get_curr_ne() const noexcept { return Mutable_Copy_Only(i + 1); }

      Mutable_Copy_Only get_curr() const noexcept { return get_curr_ne(); }

      void next_ne() noexcept { ++i; }

      void next() noexcept { next_ne(); }
    };
  };

  // The zips of several containers took such an item from a tuple that was
  // not constant, with T(T &); copying it from a constant one, with the
  // copy that it does not have, did not compile
  TEST(Zip_Current_Copies, CopiesAnItemThatCopiesOnlyFromOneNotConstant)
  {
    using List = DynList<long>;
    using Zip = ZipIterator<By_Value_Items, List>;
    using Enum_Zip = EnumZipIterator<By_Value_Items, List>;
    EXPECT_FALSE(noexcept(std::declval<const Zip &>().get_curr_ne()));
    EXPECT_FALSE(noexcept(std::declval<const Enum_Zip &>().get_curr_ne()));

    const By_Value_Items items;
    const auto list = one_two_three<List>();
    long sum = 0;
    for (Zip it(items, list); it.has_curr(); it.next_ne())
      sum += std::get<0>(it.get_curr_ne()).value * std::get<1>(it.get_curr_ne());
    for (Enum_Zip it(items, list); it.has_curr(); it.next_ne())
      sum += std::get<0>(it.get_curr_ne()).value + static_cast<long>(std::get<2>(it.get_curr_ne()));
    EXPECT_EQ(sum, (1 + 4 + 9) + (1 + 3 + 5));

    const Zip zip(items, list);
    const Enum_Zip enum_zip(items, list);
    {
      Failing_From failing(0);
      EXPECT_THROW((void) zip.get_curr_ne(), Injected_Failure);
      EXPECT_THROW((void) enum_zip.get_curr_ne(), Injected_Failure);
    }
    EXPECT_EQ(std::get<0>(zip.get_curr_ne()).value, 1);
  }

  // =========================================================================
  // A base of the nodes that throws, as a user may give to DECLARE_BINNODE
  // =========================================================================

  // A base whose default construction and copy fail when asked to
  struct Failing_Base
  {
    Failing_Base() { Failures::hit("default construction of the base"); }

    Failing_Base(const Failing_Base &) { Failures::hit("copy of the base"); }

    Failing_Base & operator = (const Failing_Base &) noexcept = default;

    void reset() noexcept {}
  };

  DECLARE_BINNODE(Failing_Base_Node, 64, Failing_Base);

  // Node(Key &&) builds the base by default and Node(const Control_Data &)
  // copies it: they were noexcept as the key, which does not throw, and
  // what the base threw ended the program
  TEST(Node_Construction_Base, LetsTheFailureOfTheBaseThrough)
  {
    using Node = Failing_Base_Node<long>;
    EXPECT_FALSE((std::is_nothrow_constructible_v<Node, long &&>));
    EXPECT_FALSE((std::is_nothrow_constructible_v<Node, const Failing_Base &>));

    const Failing_Base base;
    {
      Failing_From failing(0);
      EXPECT_THROW((void) Node(7L), Injected_Failure);
    }
    {
      Failing_From failing(0);
      EXPECT_THROW((void) Node(base), Injected_Failure);
    }

    // the bases of the library do not throw: their nodes keep the noexcept
    EXPECT_TRUE((std::is_nothrow_constructible_v<AvlNode<long>, long &&>));
    EXPECT_TRUE((std::is_nothrow_constructible_v<BinNode<long>, const Empty_Node &>));
  }
}
