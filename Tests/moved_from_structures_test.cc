
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

/** @file moved_from_structures_test.cc
 *  @brief The moved-from tests (moved_from.H) of the data structures and
 *         the sketches.
 */

#include "moved_from.H"

#include <DP_Optimizations.H>
#include <compact-cuckoo-filter.H>
#include <compact-quotient-filter.H>
#include <count-min-sketch.H>
#include <cuckoo-filter.H>
#include <hyperloglog.H>
#include <minhash.H>
#include <modular_combinatorics.H>
#include <pointer_table.H>
#include <quotient-filter.H>
#include <reservoir-sampling.H>
#include <tpl_b_tree.H>
#include <tpl_bplus_tree.H>
#include <tpl_cartesian_tree.H>
#include <tpl_disjoint_sparse_table.H>
#include <tpl_dynArrayHeap.H>
#include <tpl_dynSkipList.H>
#include <tpl_fenwick_tree.H>
#include <tpl_persistent_hash_map.H>
#include <tpl_persistent_vector.H>
#include <tpl_rand_tree.H>
#include <tpl_random_queue.H>
#include <tpl_segment_tree.H>
#include <tpl_sparse_table.H>

// SkipList is left out with MSVC, as in random_structures_test.cc: its
// nodes end in an array of zero length
#if defined(_MSC_VER) && !defined(__clang__)
#  define MOVED_FROM_SKIP_LIST 0
#else
#  define MOVED_FROM_SKIP_LIST 1
#  include <tpl_skipList.H>
#endif

#include <algorithm>
#include <cstdint>
#include <limits>
#include <new>
#include <optional>
#include <stdexcept>

using namespace Aleph;

template <> struct Moved_From_Case<HyperLogLog<int>>
{
  static HyperLogLog<int> make(const size_t n)
  {
    HyperLogLog<int> h(10);
    for (size_t i = 0; i < n; ++i)
      h.update(static_cast<int>(i));
    return h;
  }
  static bool is_empty(const HyperLogLog<int> & h) { return h.estimate() == 0.0; }
  static void use(HyperLogLog<int> & h)
  {
    (void) h.estimate();
    h.update(7);
    h.clear();
    h.update(3);
    EXPECT_GT(h.estimate(), 0.0);
  }
};

template <> struct Moved_From_Case<Count_Min_Sketch<int>>
{
  static Count_Min_Sketch<int> make(const size_t n)
  {
    Count_Min_Sketch<int> s(n < 10 ? 8 : 64, 4);
    for (size_t i = 0; i < n; ++i)
      s.update(static_cast<int>(i));
    return s;
  }
  static bool is_empty(const Count_Min_Sketch<int> & s) { return s.total_count() == 0; }
  static void use(Count_Min_Sketch<int> & s)
  {
    s.update(7, 2);
    EXPECT_GE(s.estimate(7), 2u);
    s.clear();
    EXPECT_EQ(s.estimate(7), 0u);
  }
};

template <> struct Moved_From_Case<MinHash<int>>
{
  static MinHash<int> make(const size_t n)
  {
    MinHash<int> m(n < 10 ? 4 : 64);
    for (size_t i = 0; i < n; ++i)
      m.update(static_cast<int>(i));
    return m;
  }
  static bool is_empty(const MinHash<int> & m)
  {
    for (size_t i = 0; i < m.size(); ++i)
      if (m.get_signature()[i] != std::numeric_limits<uint64_t>::max())
        return false;
    return true;
  }
  static void use(MinHash<int> & m)
  {
    m.update(7);
    MinHash<int> other = m;
    EXPECT_DOUBLE_EQ(m.similarity(other), 1.0);
    m.clear();
  }
};

template <> struct Moved_From_Case<Reservoir_Sampler<int>>
{
  static Reservoir_Sampler<int> make(const size_t n)
  {
    Reservoir_Sampler<int> r(n < 10 ? 2 : 8, 1);
    for (size_t i = 0; i < n; ++i)
      r.update(static_cast<int>(i));
    return r;
  }
  static bool is_empty(const Reservoir_Sampler<int> & r) { return r.total_seen() == 0; }
  static void use(Reservoir_Sampler<int> & r)
  {
    for (int i = 0; i < 20; ++i)
      r.update(i);
    EXPECT_EQ(r.get_sample().size(), r.sample_size());
  }
};

template <> struct Moved_From_Case<Fenwick_Tree<int>>
{
  static Fenwick_Tree<int> make(const size_t n)
  {
    Fenwick_Tree<int> f(n);
    for (size_t i = 0; i < n; ++i)
      f.update(i, 1);
    return f;
  }
  static bool is_empty(const Fenwick_Tree<int> & f) { return f.is_empty(); }
  static void use(Fenwick_Tree<int> & f)
  {
    for (size_t i = 0; i < f.size(); ++i)
      f.update(i, 1);
    if (not f.is_empty())
      (void) f.prefix(f.size() - 1);
  }
};

template <> struct Moved_From_Case<Range_Fenwick_Tree<int>>
{
  static Range_Fenwick_Tree<int> make(const size_t n)
  {
    Range_Fenwick_Tree<int> f(n);
    if (n > 0)
      f.update(0, n - 1, 1);
    return f;
  }
  static bool is_empty(const Range_Fenwick_Tree<int> & f) { return f.is_empty(); }
  static void use(Range_Fenwick_Tree<int> & f)
  {
    if (f.is_empty())
      return;
    f.update(0, f.size() - 1, 2);
    (void) f.query(0, f.size() - 1);
  }
};

// The segment trees of point updates: Sum_, Min_ and Max_Segment_Tree
template <class Tree> struct Point_Segment_Tree_Case
{
  static Tree make(const size_t n) { return Tree(n, 1); }
  static bool is_empty(const Tree & t) { return t.is_empty(); }
  static void use(Tree & t)
  {
    for (size_t i = 0; i < t.size(); ++i)
      {
        t.set(i, static_cast<int>(i));
        EXPECT_EQ(t.get(i), static_cast<int>(i));
        t.update(i, 1);
      }
    if (not t.is_empty())
      (void) t.query(0, t.size() - 1);
    EXPECT_EQ(t.values().size(), t.size());
  }
};

template <> struct Moved_From_Case<Sum_Segment_Tree<int>>
  : Point_Segment_Tree_Case<Sum_Segment_Tree<int>> {};
template <> struct Moved_From_Case<Min_Segment_Tree<int>>
  : Point_Segment_Tree_Case<Min_Segment_Tree<int>> {};
template <> struct Moved_From_Case<Max_Segment_Tree<int>>
  : Point_Segment_Tree_Case<Max_Segment_Tree<int>> {};

// The lazy segment trees, whose updates add to a range
template <class Tree> struct Lazy_Segment_Tree_Case
{
  static Tree make(const size_t n) { return Tree(n, 1); }
  static bool is_empty(const Tree & t) { return t.is_empty(); }
  static void use(Tree & t)
  {
    if (not t.is_empty())
      t.update(0, t.size() - 1, 2);
    for (size_t i = 0; i < t.size(); ++i)
      {
        t.set(i, static_cast<int>(i));
        t.point_update(i, 1);
        EXPECT_EQ(t.get(i), static_cast<int>(i) + 1);
      }
    if (not t.is_empty())
      (void) t.query(0, t.size() - 1);
    EXPECT_EQ(t.values().size(), t.size());
  }
};

template <> struct Moved_From_Case<Lazy_Sum_Segment_Tree<int>>
  : Lazy_Segment_Tree_Case<Lazy_Sum_Segment_Tree<int>> {};
template <> struct Moved_From_Case<Lazy_Min_Segment_Tree<int>>
  : Lazy_Segment_Tree_Case<Lazy_Min_Segment_Tree<int>> {};
template <> struct Moved_From_Case<Lazy_Max_Segment_Tree<int>>
  : Lazy_Segment_Tree_Case<Lazy_Max_Segment_Tree<int>> {};

template <> struct Moved_From_Case<Segment_Tree_Beats<int>>
{
  static Segment_Tree_Beats<int> make(const size_t n) { return Segment_Tree_Beats<int>(n, 5); }
  static bool is_empty(const Segment_Tree_Beats<int> & t) { return t.is_empty(); }
  static void use(Segment_Tree_Beats<int> & t)
  {
    if (not t.is_empty())
      {
        const size_t last = t.size() - 1;
        t.chmin(0, last, 3);
        t.chmax(0, last, 1);
        EXPECT_EQ(t.query_max(0, last), 3);
        EXPECT_EQ(t.query_min(0, last), 3);
        EXPECT_EQ(t.query_sum(0, last), 3 * static_cast<int>(t.size()));
      }
    for (size_t i = 0; i < t.size(); ++i)
      EXPECT_EQ(t.get(i), 3);
    EXPECT_EQ(t.values().size(), t.size());
  }
};

// The sparse tables of minimum and of maximum, of equal values
template <class Table> struct Sparse_Table_Case
{
  static Table make(const size_t n) { return Table(n, 7); }
  static bool is_empty(const Table & t) { return t.is_empty() and t.num_levels() == 0; }
  static void use(Table & t)
  {
    for (size_t i = 0; i < t.size(); ++i)
      EXPECT_EQ(t.get(i), 7);
    if (not t.is_empty())
      {
        EXPECT_EQ(t.query(0, t.size() - 1), 7);
      }
    EXPECT_EQ(t.values().size(), t.size());
  }
};

template <> struct Moved_From_Case<Sparse_Table<int>> : Sparse_Table_Case<Sparse_Table<int>> {};
template <> struct Moved_From_Case<Max_Sparse_Table<int>>
  : Sparse_Table_Case<Max_Sparse_Table<int>> {};

template <> struct Moved_From_Case<Sum_Disjoint_Sparse_Table<int>>
{
  static Sum_Disjoint_Sparse_Table<int> make(const size_t n)
  {
    return Sum_Disjoint_Sparse_Table<int>(n, 1);
  }
  static bool is_empty(const Sum_Disjoint_Sparse_Table<int> & t)
  {
    return t.is_empty() and t.num_levels() == 0;
  }
  static void use(Sum_Disjoint_Sparse_Table<int> & t)
  {
    for (size_t i = 0; i < t.size(); ++i)
      EXPECT_EQ(t.get(i), 1);
    if (not t.is_empty())
      {
        EXPECT_EQ(t.query(0, t.size() - 1), static_cast<int>(t.size()));
      }
    EXPECT_EQ(t.values().size(), t.size());
  }
};

namespace
{
  // The values of the Cartesian trees: a fixed scramble, with repetitions
  Array<int> scrambled_values(const size_t n)
  {
    auto a = Array<int>::create(n);
    for (size_t i = 0; i < n; ++i)
      a(i) = static_cast<int>((i * 37 + 11) % 101);
    return a;
  }
} // namespace

template <> struct Moved_From_Case<Cartesian_Tree<int>>
{
  static Cartesian_Tree<int> make(const size_t n) { return Cartesian_Tree<int>(scrambled_values(n)); }
  static bool is_empty(const Cartesian_Tree<int> & t)
  {
    return t.is_empty() and t.root() == Cartesian_Tree<int>::NONE;
  }
  static void use(Cartesian_Tree<int> & t)
  {
    for (size_t i = 0; i < t.size(); ++i)
      {
        if (const size_t p = t.parent_of(i); p != Cartesian_Tree<int>::NONE)
          {
            EXPECT_LE(t.data_at(p), t.data_at(i));
          }
        (void) t.left_child(i);
        (void) t.right_child(i);
        (void) t.is_leaf(i);
      }
    EXPECT_EQ(t.inorder().size(), t.size());
    EXPECT_EQ(t.values().size(), t.size());
    EXPECT_LE(t.height(), t.size());
  }
};

template <> struct Moved_From_Case<Euler_Tour_LCA<int>>
{
  static Euler_Tour_LCA<int> make(const size_t n) { return Euler_Tour_LCA<int>(scrambled_values(n)); }
  static bool is_empty(const Euler_Tour_LCA<int> & l)
  {
    return l.is_empty() and l.euler_tour_size() == 0;
  }
  static void use(Euler_Tour_LCA<int> & l)
  {
    EXPECT_EQ(l.euler_tour().size(), l.euler_tour_size());
    for (size_t i = 0; i < l.size(); ++i)
      {
        EXPECT_LE(l.depth_of(l.lca(0, i)), l.depth_of(i));
        (void) l.distance(0, i);
      }
    EXPECT_EQ(l.tree().size(), l.size());
  }
};

template <> struct Moved_From_Case<Cartesian_Tree_RMQ<int>>
{
  static Cartesian_Tree_RMQ<int> make(const size_t n)
  {
    return Cartesian_Tree_RMQ<int>(scrambled_values(n));
  }
  static bool is_empty(const Cartesian_Tree_RMQ<int> & q) { return q.is_empty(); }
  static void use(Cartesian_Tree_RMQ<int> & q)
  {
    int least = std::numeric_limits<int>::max();
    for (size_t i = 0; i < q.size(); ++i)
      {
        least = std::min(least, q.get(i));
        EXPECT_EQ(q.query(0, i), least);
        EXPECT_EQ(q.get(q.query_idx(0, i)), least);
      }
    EXPECT_EQ(q.values().size(), q.size());
  }
};

// The cuckoo filters, with a fixed seed so that every run kicks the same
template <class Filter> struct Cuckoo_Filter_Case
{
  static Filter make(const size_t n)
  {
    Filter f(n < 10 ? 8 : n, 1);
    for (size_t i = 0; i < n; ++i)
      (void) f.insert(static_cast<int>(i));
    return f;
  }
  static bool is_empty(const Filter & f) { return f.is_empty(); }
  static void use(Filter & f)
  {
    if (f.is_empty())  // as a moved-from filter: one bucket, or none until it inserts
      {
        EXPECT_FALSE(f.contains(7));
        EXPECT_FALSE(f.remove(7));
        f.clear();
        Filter copy(f);
        EXPECT_TRUE(copy.insert(7));
        EXPECT_TRUE(copy.contains(7));
      }
    EXPECT_TRUE(f.insert(7));
    EXPECT_TRUE(f.contains(7));
    EXPECT_TRUE(f.remove(7));
    f.clear();
    EXPECT_TRUE(f.is_empty());
    EXPECT_TRUE(f.insert(3));
    EXPECT_TRUE(f.contains(3));
  }
};

template <> struct Moved_From_Case<Cuckoo_Filter<int>> : Cuckoo_Filter_Case<Cuckoo_Filter<int>> {};
template <> struct Moved_From_Case<Compact_Cuckoo_Filter<int>>
  : Cuckoo_Filter_Case<Compact_Cuckoo_Filter<int>> {};

// The quotient filters: 16 slots for a small one, 4096 for a large one
template <class Filter> struct Quotient_Filter_Case
{
  static Filter make(const size_t n)
  {
    Filter f(n < 10 ? 4 : 12, 8);
    for (size_t i = 0; i < n; ++i)
      f.insert(static_cast<int>(i));
    return f;
  }
  static bool is_empty(const Filter & f) { return f.is_empty(); }
  static void use(Filter & f)
  {
    if (f.is_empty())  // as a moved-from filter, of two slots
      {
        EXPECT_FALSE(f.contains(7));
        EXPECT_FALSE(f.remove(7));
        f.clear();
        Filter copy(f), other(f.q(), f.r(), f.seed());
        copy.merge(f);
        other.insert(5);
        f.merge(other);
        EXPECT_TRUE(f.contains(5));
        EXPECT_TRUE(f.remove(5));
      }
    f.insert(7);
    EXPECT_TRUE(f.contains(7));
    EXPECT_TRUE(f.remove(7));
    f.clear();
    EXPECT_TRUE(f.is_empty());
    f.insert(3);
    EXPECT_TRUE(f.contains(3));
  }
};

template <> struct Moved_From_Case<Quotient_Filter<int>>
  : Quotient_Filter_Case<Quotient_Filter<int>> {};
template <> struct Moved_From_Case<Compact_Quotient_Filter<int>>
  : Quotient_Filter_Case<Compact_Quotient_Filter<int>> {};

template <> struct Moved_From_Case<DynArrayHeap<int>>
{
  static DynArrayHeap<int> make(const size_t n)
  {
    DynArrayHeap<int> h;
    for (size_t i = 0; i < n; ++i)
      (void) h.insert(static_cast<int>((i * 37 + 11) % 101));
    return h;
  }
  static bool is_empty(const DynArrayHeap<int> & h) { return h.is_empty(); }
  static void use(DynArrayHeap<int> & h)
  {
    size_t count = 0;
    h.for_each([&count] (const int) { ++count; });
    EXPECT_EQ(count, h.size());
    (void) h.insert(-1);
    (void) h.insert(500);
    EXPECT_EQ(h.top(), -1);
    EXPECT_EQ(h.getMin(), -1);
  }
};

// The B-Tree and the B+ Tree
template <class Tree> struct Ordered_Tree_Case
{
  static Tree make(const size_t n)
  {
    Tree t;
    for (size_t i = 0; i < n; ++i)
      (void) t.insert(static_cast<int>(i));
    return t;
  }
  static bool is_empty(const Tree & t) { return t.is_empty() and t.verify(); }
  static void use(Tree & t)
  {
    EXPECT_TRUE(t.verify());
    EXPECT_EQ(t.keys().size(), t.size());
    EXPECT_EQ(t.min_key().has_value(), not t.is_empty());
    EXPECT_TRUE(t.insert(-1));
    EXPECT_TRUE(t.contains(-1));
    EXPECT_EQ(t.min_key(), std::optional<int>(-1));
    EXPECT_TRUE(t.remove(-1));
    EXPECT_TRUE(t.verify());
  }
};

template <> struct Moved_From_Case<B_Tree<int>> : Ordered_Tree_Case<B_Tree<int>> {};
template <> struct Moved_From_Case<BPlus_Tree<int>> : Ordered_Tree_Case<BPlus_Tree<int>> {};

template <> struct Moved_From_Case<PersistentVector<int>>
{
  static PersistentVector<int> make(const size_t n)
  {
    PersistentVector<int> v;
    for (size_t i = 0; i < n; ++i)
      v = v.push_back(static_cast<int>(i));
    return v;
  }
  static bool is_empty(const PersistentVector<int> & v) { return v.is_empty() and v.verify(); }
  static void use(PersistentVector<int> & v)
  {
    EXPECT_TRUE(v.verify());
    for (size_t i = 0; i < v.size(); ++i)
      (void) v.get(i);
    EXPECT_EQ(v.to_array().size(), v.size());
    const PersistentVector<int> w = v.push_back(7);
    EXPECT_EQ(w.size(), v.size() + 1);
    EXPECT_EQ(w[v.size()], 7);
    if (not v.is_empty())
      {
        EXPECT_EQ(v.set(0, 9)[0], 9);
        EXPECT_EQ(v.pop_back().size(), v.size() - 1);
      }
    v = w;
  }
};

template <> struct Moved_From_Case<PersistentHashMap<int, int>>
{
  static PersistentHashMap<int, int> make(const size_t n)
  {
    PersistentHashMap<int, int> m;
    for (size_t i = 0; i < n; ++i)
      m = m.insert(static_cast<int>(i), static_cast<int>(i));
    return m;
  }
  static bool is_empty(const PersistentHashMap<int, int> & m)
  {
    return m.is_empty() and m.verify();
  }
  static void use(PersistentHashMap<int, int> & m)
  {
    EXPECT_TRUE(m.verify());
    const Array<int> keys = m.keys();
    EXPECT_EQ(keys.size(), m.size());
    for (size_t i = 0; i < keys.size(); ++i)
      EXPECT_NE(m.find(keys(i)), nullptr);
    m = m.insert_or_assign(-1, 7);
    ASSERT_NE(m.find(-1), nullptr);
    EXPECT_EQ(*m.find(-1), 7);
    m = m.erase(-1);
    EXPECT_FALSE(m.contains(-1));
  }
};

template <> struct Moved_From_Case<DynSkipList<int>>
{
  static DynSkipList<int> make(const size_t n)
  {
    DynSkipList<int> l(1UL);
    for (size_t i = 0; i < n; ++i)
      (void) l.insert(static_cast<int>(i));
    return l;
  }
  static bool is_empty(const DynSkipList<int> & l) { return l.is_empty(); }
  static void use(DynSkipList<int> & l)
  {
    size_t count = 0;
    l.for_each([&count] (const int) { ++count; });
    EXPECT_EQ(count, l.size());
    EXPECT_NE(l.insert(-1), nullptr);
    EXPECT_NE(l.insert(-2), nullptr);
    EXPECT_TRUE(l.has(-1));
    EXPECT_EQ(l.remove(-1), 1u);
    l.clear();
    EXPECT_TRUE(l.is_empty());
    EXPECT_NE(l.insert(5), nullptr);
  }
};

// A randomized tree does not own its nodes: use() frees them, as its user
// would, so that none is left at the end
template <> struct Moved_From_Case<Rand_Tree<int>>
{
  using Node = Rand_Tree<int>::Node;

  static Rand_Tree<int> make(const size_t n)
  {
    Rand_Tree<int> t(1);
    for (size_t i = 0; i < n; ++i)
      (void) t.insert(new Node(static_cast<int>(i)));
    return t;
  }
  static bool is_empty(const Rand_Tree<int> & t) { return t.is_empty() and t.size() == 0; }
  static void use(Rand_Tree<int> & t)
  {
    EXPECT_TRUE(t.verify());
    for (size_t i = 0; i < t.size(); ++i)
      (void) t.select(i);
    Node * p = new Node(-1);
    Node * q = new Node(-2);
    EXPECT_EQ(t.insert(p), p);
    EXPECT_EQ(t.insert(q), q);
    EXPECT_EQ(t.search(-1), p);
    EXPECT_EQ(t.remove(-2), q);
    delete q;
    EXPECT_TRUE(t.verify());
    destroyRec(t.getRoot());
    EXPECT_TRUE(t.is_empty());
  }
};

template <> struct Moved_From_Case<Random_Set<int>>
{
  static Random_Set<int> make(const size_t n)
  {
    Random_Set<int> q;
    q.set_seed(1);
    for (size_t i = 0; i < n; ++i)
      (void) q.append(static_cast<int>(i));
    return q;
  }
  static bool is_empty(const Random_Set<int> & q) { return q.is_empty(); }
  static void use(Random_Set<int> & q)
  {
    size_t count = 0;
    q.for_each([&count] (const int) { ++count; });
    EXPECT_EQ(count, q.size());
    (void) q.append(-1);
    q.put(-2);
    (void) q.pick();
    const size_t before = q.size();
    (void) q.get();
    EXPECT_EQ(q.size(), before - 1);
  }
};

namespace
{
  // The pointers kept by the tables, which are never followed
  void * pointer_cell(const size_t i)
  {
    static int cells[1024];
    return &cells[i];
  }
} // namespace

template <> struct Moved_From_Case<Pointer_Table>
{
  static Pointer_Table make(const size_t n)
  {
    Pointer_Table t(n < 10 ? 0 : 64);
    for (size_t i = 0; i < n; ++i)
      (void) t.insert_pointer(pointer_cell(i));
    return t;
  }
  static bool is_empty(const Pointer_Table & t)
  {
    return t.is_empty() and t.get_heap_index() == 0 and t.frees() == 0;
  }
  static void use(Pointer_Table & t)
  {
    for (long i = 0; i < t.get_heap_index(); ++i)
      if (void * p = t.get_pointer(i); p != nullptr)
        {
          EXPECT_EQ(t.verify_pointer(i, p), p);
        }
    const long i = t.insert_pointer(pointer_cell(0));
    EXPECT_EQ(t.get_pointer(i), pointer_cell(0));
    t.remove_pointer(i);
    t.clear();
    EXPECT_TRUE(t.is_empty());
    (void) t.insert_pointer(pointer_cell(1));
  }
};

template <> struct Moved_From_Case<Li_Chao_Tree<long>>
{
  static Li_Chao_Tree<long> make(const size_t n)
  {
    Li_Chao_Tree<long> t(-1000, 1000);
    for (size_t i = 0; i < n; ++i)
      t.add_line(static_cast<long>(i % 7) - 3, static_cast<long>(i));
    return t;
  }
  static bool is_empty(const Li_Chao_Tree<long> & t)
  {
    return t.is_empty() and t.node_count() == 0;
  }
  static void use(Li_Chao_Tree<long> & t)
  {
    if (not t.is_empty())
      (void) t.query(0);
    t.add_line(0, -5000); // below the lines of make() on the whole domain
    EXPECT_EQ(t.query(7), -5000);
    t.clear();
    EXPECT_TRUE(t.is_empty());
    t.add_line(2, 1);
    EXPECT_EQ(t.query(3), 7);
  }
};

template <> struct Moved_From_Case<Convex_Hull_Trick<long>>
{
  static Convex_Hull_Trick<long> make(const size_t n)
  {
    Convex_Hull_Trick<long> h;
    for (size_t i = 0; i < n; ++i)
      {
        const auto k = static_cast<long>(i);
        h.add_line(-k, k * k); // every line is on the hull, the k-th about x = 2k
      }
    if (n > 0) // the cursor of the monotone queries goes to the last line
      (void) h.query_monotone(static_cast<long>(2 * n));
    return h;
  }
  static bool is_empty(const Convex_Hull_Trick<long> & h) { return h.is_empty(); }
  static void use(Convex_Hull_Trick<long> & h)
  {
    if (not h.is_empty())
      {
        EXPECT_EQ(h.query_monotone(1000000), h.query(1000000));
      }
    h.clear();
    h.add_line(1, 0);
    h.add_line(0, 2);
    h.add_line(-1, 5);
    EXPECT_EQ(h.query(0), 0);
    EXPECT_EQ(h.query_monotone(10), -5);
  }
};

// Two moduli, so that an object with the modulus of one and the tables of
// the other is seen: below the modulus, n choose 0, 1 and n are 1, n and 1
template <> struct Moved_From_Case<ModularCombinatorics>
{
  static ModularCombinatorics make(const size_t n)
  {
    return ModularCombinatorics(n, n < 10 ? 7 : 1000003);
  }
  static bool is_empty(const ModularCombinatorics & m)
  {
    try
      {
        (void) m.nCk(0, 0);
      }
    catch (const std::out_of_range &)
      {
        return true; // no table
      }
    return false;
  }
  static void use(ModularCombinatorics & m)
  {
    for (uint64_t n = 0; ; ++n) // up to the end of the tables
      {
        uint64_t none = 0;
        try
          {
            none = m.nCk(n, 0);
          }
        catch (const std::out_of_range &)
          {
            break;
          }
        EXPECT_EQ(none, 1u) << "n = " << n;
        EXPECT_EQ(m.nCk(n, n), 1u) << "n = " << n;
        if (n > 0)
          {
            EXPECT_EQ(m.lucas_nCk(n, 1), n) << "n = " << n;
          }
      }
    EXPECT_EQ(m.nCk(3, 5), 0u);
  }
};

using Structures = ::testing::Types<HyperLogLog<int>, Count_Min_Sketch<int>, MinHash<int>,
                                    Reservoir_Sampler<int>, Fenwick_Tree<int>,
                                    Range_Fenwick_Tree<int>, Sum_Segment_Tree<int>,
                                    Min_Segment_Tree<int>, Max_Segment_Tree<int>,
                                    Lazy_Sum_Segment_Tree<int>, Lazy_Min_Segment_Tree<int>,
                                    Lazy_Max_Segment_Tree<int>, Segment_Tree_Beats<int>,
                                    Sparse_Table<int>, Max_Sparse_Table<int>,
                                    Sum_Disjoint_Sparse_Table<int>, Cartesian_Tree<int>,
                                    Euler_Tour_LCA<int>, Cartesian_Tree_RMQ<int>,
                                    Cuckoo_Filter<int>, Compact_Cuckoo_Filter<int>,
                                    Quotient_Filter<int>, Compact_Quotient_Filter<int>,
                                    DynArrayHeap<int>, B_Tree<int>, BPlus_Tree<int>,
                                    PersistentVector<int>, PersistentHashMap<int, int>,
                                    DynSkipList<int>, Rand_Tree<int>, Random_Set<int>,
                                    Pointer_Table, Li_Chao_Tree<long>,
                                    Convex_Hull_Trick<long>, ModularCombinatorics>;
INSTANTIATE_TYPED_TEST_SUITE_P(Structures, MovedFrom, Structures);

#if MOVED_FROM_SKIP_LIST
// The sentinel key of a skip list of int, which its user gives
template <>
int SkipList<int, int>::Node::computeMaxKey() noexcept
{
  return std::numeric_limits<int>::max();
}

namespace
{
  using Skip_List = SkipList<int, int>;

  // A node of a skip list ends in its forward pointers: it is made with
  // room for them
  Skip_List::Node * new_skip_node(Skip_List & l, const int key)
  {
    const int level = l.generateRandomLevel();
    void * mem = ::operator new(sizeof(Skip_List::Node) + sizeof(Skip_List::Node *) * level);
    return new (mem) Skip_List::Node(key, key, level);
  }
} // namespace

// A skip list does not own its nodes either: use() removes and frees them
template <> struct Moved_From_Case<Skip_List>
{
  static Skip_List make(const size_t n)
  {
    Skip_List l(1UL);
    for (size_t i = 0; i < n; ++i)
      (void) l.insert(new_skip_node(l, static_cast<int>(i)));
    return l;
  }
  static bool is_empty(const Skip_List & l) { return not Skip_List::Iterator(l).has_curr(); }
  static void use(Skip_List & l)
  {
    Array<Skip_List::Node *> nodes;
    for (Skip_List::Iterator it(l); it.has_curr(); it.next_ne())
      nodes.append(it.get_curr_ne());
    for (size_t i = 1; i < nodes.size(); ++i)
      EXPECT_LT(nodes(i - 1)->get_key(), nodes(i)->get_key());
    Skip_List::Node * p = l.insert(new_skip_node(l, -1));
    EXPECT_EQ(l.search(-1), p);
    EXPECT_TRUE(l.checkSkipList());
    nodes.append(p);
    for (size_t i = 0; i < nodes.size(); ++i)
      {
        EXPECT_EQ(l.remove(nodes(i)->get_key()), nodes(i));
        nodes(i)->~Node();
        ::operator delete(nodes(i));
      }
    EXPECT_TRUE(is_empty(l));
  }
};

INSTANTIATE_TYPED_TEST_SUITE_P(Skip_List, MovedFrom, ::testing::Types<Skip_List>);
#endif
