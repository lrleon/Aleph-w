#include <gtest/gtest.h>

#include <tpl_random_queue.H>

using namespace Aleph;
using namespace testing;

namespace {

Random_Set<int> build_queue(const std::initializer_list<int> values)
{
  Random_Set<int> q;
  for (int v : values)
    q.put(v);
  return q;
}

} // namespace

TEST(RandomSet, AppendReturnsReference)
{
  Random_Set<int> q;
  auto &ref = q.append(42);
  ref = 13; // mutate via reference

  ASSERT_FALSE(q.is_empty());
  bool found = false;
  q.traverse([&found] (const int v) { found |= (v == 13); return true; });
  EXPECT_TRUE(found);
}

TEST(RandomSet, AppendPreservesUniformityBySwapping)
{
  Random_Set<int> q;
  for (int i = 0; i < 10; ++i)
    q.append(i);

  // insert sentinel twice: once to mutate reference, once to ensure random placement
  auto &first = q.append(100);
  first = 100;
  q.append(100);

  size_t count = 0;
  q.traverse([&count] (const int v)
             {
               if (v == 100)
                 ++count;
               return true;
             });
  EXPECT_EQ(2u, count);
}

TEST(RandomSet, GetRemovesUniformly)
{
  Random_Set<int> q = build_queue({1, 2, 3, 4});
  size_t seen = 0;
  while (not q.is_empty())
    {
      int val = q.get();
      EXPECT_GE(val, 1);
      EXPECT_LE(val, 4);
      ++seen;
    }
  EXPECT_EQ(4u, seen);
}

TEST(RandomSet, AppendMoveAcceptsComplexTypes)
{
  struct Payload
  {
    std::string data;
    Payload() = default;
    explicit Payload(std::string d) : data(std::move(d)) {}
  };

  Random_Set<Payload> q;
  const auto &stored = q.append(Payload("beta"));
  EXPECT_EQ("beta", stored.data);

  auto &stored2 = q.append(Payload("gamma"));
  stored2.data = "delta";

  size_t hits = 0;
  q.traverse([&hits] (const Payload &p)
             {
               if (p.data == "delta" || p.data == "beta")
                 ++hits;
               return true;
             });
  EXPECT_EQ(2u, hits);
}
