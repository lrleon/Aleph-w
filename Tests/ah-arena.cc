
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

# include <gsl/gsl_rng.h>

# include <tclap/CmdLine.h>

# include <gtest/gtest.h>

# include <ah-arena.H>
# include <tpl_dynSetTree.H>

using namespace std;
using namespace Aleph;
using namespace testing;
using namespace TCLAP;

struct StaticArenaFixture : public Test
{
  static constexpr size_t sz = 1021;
  unique_ptr<char[]> buffer;
  AhArenaAllocator arena;

  StaticArenaFixture()
    : buffer(new char[sz]), arena(buffer.get(), sz) {}
};

TEST_F(StaticArenaFixture, simple_fail)
{
  void * ptr = arena.alloc(sz);
  ASSERT_EQ(ptr, nullptr);
}

TEST_F(StaticArenaFixture, free_under_lifo_order)
{
  void * ptr = arena.alloc(10);
  ASSERT_NE(ptr, nullptr);

  arena.dealloc(ptr, 10);
  ASSERT_EQ(arena.allocated_size(), 0);
}

TEST_F(StaticArenaFixture, one_alloc_next_fail)
{
  void * ptr = arena.alloc(sz - 1);
  ASSERT_NE(ptr, nullptr);
  ASSERT_EQ(ptr, arena.base_addr());
  ASSERT_EQ((void*) ((char*) ptr + sz), arena.end_addr());

  void * ptr1 = arena.alloc(1);
  ASSERT_EQ(ptr1, nullptr);
}

CmdLine cmd = { "ah_arena", ' ', "0" };

ValueArg<unsigned long> seed =
  { "s", "seed", "seed for random generator", false, 0,
    "seed for random generator", cmd };

TEST_F(StaticArenaFixture, random_allocs)
{
  DynList<pair<const char*, size_t>> blocks;
  gsl_rng * r = gsl_rng_alloc(gsl_rng_mt19937);
  gsl_rng_set(r, seed.getValue() % gsl_rng_max(r));
  size_t size = 0;
  while (true)
    {
      size = 1 + gsl_rng_uniform_int(r, sz - 2);
      const char * addr = (const char*) arena.alloc(size);
      if (addr == nullptr)
        break;
    }

  cout << "Available size = " << arena.available_size() << endl
       << "Last requested size = " << size << endl;

  ASSERT_EQ(arena.alloc(arena.available_size() + 1), nullptr);

  gsl_rng_free(r);
}

TEST_F(StaticArenaFixture, object_alloc)
{
  struct Foo
  {
    string * data_ptr = nullptr;
    int i = -1;

    Foo(const char * data, int i)
      : data_ptr(new string(data)), i(i)
    {
      cout << "Foo Constructor" << endl;
    }

    ~Foo()
    {
      if (data_ptr != nullptr)
        delete data_ptr;
      cout << "Foo Destructor" << endl;
    }
  };

  DynList<Foo*> chunks;
  for (int i = 0; true; ++i)
    {
      Foo * ptr = allocate<Foo>(arena, "hello", i);
      if (ptr == nullptr)
        break;

      chunks.append(ptr);
      cout << "Allocated " << i << " node" << endl;
    }

  ASSERT_EQ(chunks.size() * sizeof(Foo), arena.allocated_size());

  chunks.reverse().for_each([this] (Foo * ptr)
  {
    cout << "Freeing " << ptr->i << endl;
    dealloc<Foo>(arena, ptr);
  });

  ASSERT_EQ(arena.allocated_size(), 0);
  ASSERT_EQ(arena.available_size(), sz);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  cmd.parse(argc, argv);

  return RUN_ALL_TESTS();
}
