#include <gtest/gtest.h>

#include <euclidian-graph-common.H>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <tpl_graph.H>
#include <tpl_sgraph.H>

using namespace Aleph;

namespace
{
using G = List_SGraph<Graph_Snode<My_P>, Graph_Sarc<double>>;

struct ArcSnapshot
{
  int x1;
  int y1;
  int x2;
  int y2;
  double w;

  bool operator==(const ArcSnapshot & other) const
  {
    return std::tie(x1, y1, x2, y2, w) == std::tie(other.x1, other.y1, other.x2, other.y2, other.w);
  }

  bool operator<(const ArcSnapshot & other) const
  {
    return std::tie(x1, y1, x2, y2, w) < std::tie(other.x1, other.y1, other.x2, other.y2, other.w);
  }
};

std::vector<std::pair<int, int>> collect_positions(const G & g)
{
  std::vector<std::pair<int, int>> pos;
  pos.reserve(g.vsize());

  for (Node_Iterator<G> it(g); it.has_curr(); it.next_ne())
    {
      auto * n = it.get_curr();
      const auto & p = n->get_info();
      pos.emplace_back(p.x, p.y);
    }

  std::sort(pos.begin(), pos.end());
  return pos;
}

std::vector<ArcSnapshot> collect_arcs_normalized(const G & g)
{
  std::vector<ArcSnapshot> arcs;
  arcs.reserve(g.esize());

  for (Arc_Iterator<G> it(g); it.has_curr(); it.next_ne())
    {
      auto * a = it.get_curr();
      auto * s = g.get_src_node(a);
      auto * t = g.get_tgt_node(a);
      const auto & ps = s->get_info();
      const auto & pt = t->get_info();

      int x1 = ps.x, y1 = ps.y;
      int x2 = pt.x, y2 = pt.y;
      if (std::tie(x2, y2) < std::tie(x1, y1))
        std::swap(x1, x2), std::swap(y1, y2);

      arcs.push_back({x1, y1, x2, y2, a->get_info()});
    }

  std::sort(arcs.begin(), arcs.end());
  return arcs;
}
} // namespace

TEST(EuclidianGraphCommon, RejectsImpossibleUniquePlacement)
{
  // With w*h = 4, n=5 is impossible.
  EXPECT_THROW((void)gen_random_euclidian_graph<G>(5, 1, 2, 2, 123u), std::domain_error);
}

TEST(EuclidianGraphCommon, NodeCoordinatesAreUniqueAndInRange)
{
  constexpr int w = 40;
  constexpr int h = 30;
  const auto g = gen_random_euclidian_graph<G>(200, 300, w, h, 7u);

  auto pos = collect_positions(g);
  ASSERT_EQ(pos.size(), 200u);

  // Uniqueness
  EXPECT_EQ(std::unique(pos.begin(), pos.end()), pos.end());

  // Range
  for (const auto & [x, y] : pos)
    {
      EXPECT_GE(x, 0);
      EXPECT_LT(x, w);
      EXPECT_GE(y, 0);
      EXPECT_LT(y, h);
    }
}

TEST(EuclidianGraphCommon, ArcWeightsAreWithinBounds)
{
  constexpr int w = 60;
  constexpr int h = 80;
  const auto g = gen_random_euclidian_graph<G>(100, 200, w, h, 9u);

  const int max_offset = std::max(1, int(std::ceil(std::hypot(double(w), double(h)))));

  for (Arc_Iterator<G> it(g); it.has_curr(); it.next_ne())
    {
      auto * a = it.get_curr();
      auto * s = g.get_src_node(a);
      auto * t = g.get_tgt_node(a);
      const auto & ps = s->get_info();
      const auto & pt = t->get_info();

      const double dist = std::hypot(double(ps.x - pt.x), double(ps.y - pt.y));
      const double wgt = a->get_info();

      EXPECT_GE(wgt, dist);
      EXPECT_LT(wgt, dist + max_offset);
    }
}

TEST(EuclidianGraphCommon, DeterministicForFixedSeed)
{
  constexpr int w = 50;
  constexpr int h = 50;
  constexpr unsigned seed = 42u;

  const auto g1 = gen_random_euclidian_graph<G>(120, 220, w, h, seed);
  const auto g2 = gen_random_euclidian_graph<G>(120, 220, w, h, seed);

  EXPECT_EQ(collect_positions(g1), collect_positions(g2));
  EXPECT_EQ(collect_arcs_normalized(g1), collect_arcs_normalized(g2));
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
