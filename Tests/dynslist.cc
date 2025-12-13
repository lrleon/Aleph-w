#include <gtest/gtest.h>

#include <tpl_dynSlist.H>

#include <random>
#include <vector>

using namespace Aleph;
using namespace testing;

namespace {

struct Movable
{
  static inline int move_count = 0;
  int value;

  Movable() : value(0) {}
  Movable(int v) : value(v) {}
  Movable(const Movable &) = default;
  Movable & operator=(const Movable &) = default;

  Movable(Movable && other) noexcept : value(other.value)
  {
    other.value = -1;
    ++move_count;
  }

  Movable & operator=(Movable && other) noexcept
  {
    if (this != &other)
      {
        value = other.value;
        other.value = -1;
        ++move_count;
      }
    return *this;
  }
};

} // namespace

TEST(DynSlist, InsertAccessRemove)
{
  DynSlist<int> list;
  list.insert(0, 1);
  list.insert(1, 3);
  list.insert(1, 2); // middle insert

  ASSERT_EQ(list.size(), 3u);
  EXPECT_EQ(list[0], 1);
  EXPECT_EQ(list[1], 2);
  EXPECT_EQ(list[2], 3);

  list.remove(1);
  EXPECT_EQ(list.size(), 2u);
  EXPECT_EQ(list[0], 1);
  EXPECT_EQ(list[1], 3);
}

TEST(DynSlist, InsertMoveOverload)
{
  Movable::move_count = 0;
  DynSlist<Movable> list;
  Movable m1(10);
  list.insert(0, std::move(m1));
  EXPECT_EQ(list.size(), 1u);
  EXPECT_EQ(list[0].value, 10);
  EXPECT_EQ(Movable::move_count, 1);
}

TEST(DynSlist, ThrowsOnInvalidPositions)
{
  DynSlist<int> list;
  EXPECT_THROW(list.remove(0), std::out_of_range);
  EXPECT_THROW(list.insert(2, 5), std::out_of_range);
  list.insert(0, 7);
  EXPECT_THROW((void)list[1], std::out_of_range);
}

TEST(DynSlist, IteratorTraversesInOrder)
{
  DynSlist<int> list;
  for (int i = 0; i < 5; ++i)
    list.insert(i, i + 1);

  std::vector<int> got;
  DynSlist<int>::Iterator it(list);
  for (; it.has_curr(); it.next())
    got.push_back(it.get_curr());

  EXPECT_EQ(got, (std::vector<int>{1, 2, 3, 4, 5}));
}

TEST(DynSlist, ConstOperatorBracketWorks)
{
  DynSlist<int> list;
  for (int i = 0; i < 4; ++i)
    list.insert(i, 10 + i);

  const DynSlist<int> & c = list;
  EXPECT_EQ(c[0], 10);
  EXPECT_EQ(c[1], 11);
  EXPECT_EQ(c[2], 12);
  EXPECT_EQ(c[3], 13);
  EXPECT_THROW((void)c[4], std::out_of_range);
}

TEST(DynSlist, CopyConstructorIsDeepCopy)
{
  DynSlist<int> a;
  for (int i = 0; i < 5; ++i)
    a.insert(i, i);

  DynSlist<int> b(a);
  ASSERT_EQ(b.size(), 5u);
  for (size_t i = 0; i < b.size(); ++i)
    EXPECT_EQ(b[i], static_cast<int>(i));

  a.remove(0);
  a.insert(0, 42);

  EXPECT_EQ(b[0], 0);
  EXPECT_EQ(a[0], 42);
}

TEST(DynSlist, CopyAssignmentAndSelfAssignment)
{
  DynSlist<int> a;
  for (int i = 0; i < 3; ++i)
    a.insert(i, i + 1);

  DynSlist<int> b;
  b.insert(0, 99);
  b = a;

  ASSERT_EQ(b.size(), 3u);
  EXPECT_EQ(b[0], 1);
  EXPECT_EQ(b[1], 2);
  EXPECT_EQ(b[2], 3);

  a = a;
  ASSERT_EQ(a.size(), 3u);
  EXPECT_EQ(a[0], 1);
  EXPECT_EQ(a[1], 2);
  EXPECT_EQ(a[2], 3);
}

TEST(DynSlist, MoveConstructorLeavesSourceEmpty)
{
  DynSlist<int> a;
  for (int i = 0; i < 4; ++i)
    a.insert(i, 10 + i);

  DynSlist<int> b(std::move(a));

  ASSERT_EQ(b.size(), 4u);
  EXPECT_EQ(b[0], 10);
  EXPECT_EQ(b[1], 11);
  EXPECT_EQ(b[2], 12);
  EXPECT_EQ(b[3], 13);

  EXPECT_EQ(a.size(), 0u);
  EXPECT_THROW((void)a[0], std::out_of_range);

  a.insert(0, 7);
  EXPECT_EQ(a.size(), 1u);
  EXPECT_EQ(a[0], 7);
}

TEST(DynSlist, MoveAssignmentWorks)
{
  DynSlist<int> a;
  for (int i = 0; i < 3; ++i)
    a.insert(i, i);

  DynSlist<int> b;
  b.insert(0, 77);

  b = std::move(a);

  ASSERT_EQ(b.size(), 3u);
  EXPECT_EQ(b[0], 0);
  EXPECT_EQ(b[1], 1);
  EXPECT_EQ(b[2], 2);
  EXPECT_EQ(a.size(), 0u);
}

TEST(DynSlist, NegativePositionsThrowAndDoNotMove)
{
  Movable::move_count = 0;
  DynSlist<Movable> list;
  Movable m(10);

  EXPECT_THROW(list.insert(-1, std::move(m)), std::out_of_range);
  EXPECT_EQ(Movable::move_count, 0);
  EXPECT_EQ(m.value, 10);

  EXPECT_THROW(list.remove(-1), std::out_of_range);
}

TEST(DynSlist, Property_RandomInsertRemove_MatchesVector)
{
  DynSlist<int> list;
  std::vector<int> oracle;

  std::mt19937 rng(123456);
  std::uniform_int_distribution<int> op_dist(0, 1);
  std::uniform_int_distribution<int> val_dist(-100, 100);

  for (int step = 0; step < 300; ++step)
    {
      const int op = op_dist(rng);
      if (op == 0)
        {
          const int v = val_dist(rng);
          std::uniform_int_distribution<int> pos_dist(0, static_cast<int>(oracle.size()));
          const int pos = pos_dist(rng);
          list.insert(pos, v);
          oracle.insert(oracle.begin() + pos, v);
        }
      else
        {
          if (oracle.empty())
            {
              EXPECT_THROW(list.remove(0), std::out_of_range);
            }
          else
            {
              std::uniform_int_distribution<int> pos_dist(0, static_cast<int>(oracle.size() - 1));
              const int pos = pos_dist(rng);
              list.remove(pos);
              oracle.erase(oracle.begin() + pos);
            }
        }

      ASSERT_EQ(list.size(), oracle.size());
      for (size_t i = 0; i < oracle.size(); ++i)
        EXPECT_EQ(list[i], oracle[i]);
    }
}
