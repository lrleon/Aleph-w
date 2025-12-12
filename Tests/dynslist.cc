#include <gtest/gtest.h>

#include <tpl_dynSlist.H>

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
