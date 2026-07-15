/*
                          Aleph_w

  Data structures & Algorithms
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file bench_r_tree.cc
 * @brief Informative benchmark for RTree/RStarTree spatial indexes.
 *
 * Compares dynamic RTree/RStarTree insertion and intersection-query throughput
 * against a brute-force rectangle scan and Aleph's static AABBTree.
 *
 * Usage:
 *   bench_r_tree [entry_count] [query_count]
 */

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <vector>

#include <geom_algorithms.H>
#include <tpl_r_star_tree.H>
#include <tpl_r_tree.H>

using namespace Aleph;

namespace
{

volatile size_t sink = 0;

Rectangle rect(const int x1, const int y1, const int x2, const int y2)
{
  return Rectangle(Geom_Number(x1), Geom_Number(y1),
                   Geom_Number(x2), Geom_Number(y2));
}

template <class Fn>
double best_ms(Fn fn, const int repeats = 3)
{
  double best = 1e300;
  for (int r = 0; r < repeats; ++r)
    {
      const auto t0 = std::chrono::steady_clock::now();
      fn();
      const auto t1 = std::chrono::steady_clock::now();
      const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
      best = std::min(best, ms);
    }
  return best;
}

void row(const char *index, const char *op, const double ms)
{
  std::printf("  %-12s %-28s %10.2f ms\n", index, op, ms);
}

std::vector<Rectangle> make_rectangles(const size_t n, const unsigned seed)
{
  std::mt19937 rng(seed);
  std::uniform_int_distribution<int> coord(0, 20000);
  std::uniform_int_distribution<int> extent(1, 80);

  std::vector<Rectangle> out;
  out.reserve(n);
  for (size_t i = 0; i < n; ++i)
    {
      const int x = coord(rng);
      const int y = coord(rng);
      out.push_back(rect(x, y, x + extent(rng), y + extent(rng)));
    }
  return out;
}

RTree<size_t> build_rtree(const std::vector<Rectangle> &rects)
{
  RTree<size_t> tree;
  for (size_t i = 0; i < rects.size(); ++i)
    tree.insert(rects[i], i);
  return tree;
}

RStarTree<size_t> build_rstar(const std::vector<Rectangle> &rects)
{
  RStarTree<size_t> tree;
  for (size_t i = 0; i < rects.size(); ++i)
    tree.insert(rects[i], i);
  return tree;
}

AABBTree build_aabb(const std::vector<Rectangle> &rects)
{
  Array<AABBTree::Entry> entries;
  entries.reserve(rects.size());
  for (size_t i = 0; i < rects.size(); ++i)
    entries.append(AABBTree::Entry{rects[i], i});

  AABBTree tree;
  tree.build(entries);
  return tree;
}

size_t brute_query_count(const std::vector<Rectangle> &rects,
                         const std::vector<Rectangle> &queries)
{
  size_t total = 0;
  for (const Rectangle &q : queries)
    for (const Rectangle &b : rects)
      total += b.intersects(q);
  return total;
}

template <class Tree>
size_t rtree_query_count(const Tree &tree, const std::vector<Rectangle> &queries)
{
  size_t total = 0;
  for (const Rectangle &q : queries)
    tree.for_each_intersecting(q,
      [&total](const Rectangle &, const size_t &) { ++total; });
  return total;
}

size_t aabb_query_count(const AABBTree &tree, const std::vector<Rectangle> &queries)
{
  size_t total = 0;
  for (const Rectangle &q : queries)
    total += tree.query(q).size();
  return total;
}

void validate_counts(const std::vector<Rectangle> &rects,
                     const std::vector<Rectangle> &queries,
                     const RTree<size_t> &rtree,
                     const RStarTree<size_t> &rstar,
                     const AABBTree &aabb)
{
  const size_t brute = brute_query_count(rects, queries);
  const size_t rt = rtree_query_count(rtree, queries);
  const size_t rs = rtree_query_count(rstar, queries);
  const size_t ab = aabb_query_count(aabb, queries);
  if (brute != rt or brute != rs or brute != ab)
    {
      std::fprintf(stderr,
                   "validation failed: brute=%zu rtree=%zu rstar=%zu aabb=%zu\n",
                   brute, rt, rs, ab);
      std::exit(2);
    }
  sink += brute + rt + rs + ab;
}

}  // namespace

int main(int argc, char *argv[])
{
  const size_t entry_count = argc > 1 ? std::strtoul(argv[1], nullptr, 10)
                                      : 10000;
  const size_t query_count = argc > 2 ? std::strtoul(argv[2], nullptr, 10)
                                      : 2000;

  const std::vector<Rectangle> rects = make_rectangles(entry_count, 0xA1E9u);
  const std::vector<Rectangle> queries = make_rectangles(query_count, 0xBEEFu);

  const RTree<size_t> rtree = build_rtree(rects);
  const RStarTree<size_t> rstar = build_rstar(rects);
  const AABBTree aabb = build_aabb(rects);
  validate_counts(rects, queries, rtree, rstar, aabb);

  std::printf("Spatial index benchmark (best of 3 runs, %zu entries, %zu queries)\n",
              entry_count, query_count);
  row("RTree", "incremental insert",
      best_ms([&] { const RTree<size_t> t = build_rtree(rects); sink += t.size(); }));
  row("RStarTree", "incremental insert",
      best_ms([&] { const RStarTree<size_t> t = build_rstar(rects); sink += t.size(); }));
  row("AABBTree", "static build",
      best_ms([&] { const AABBTree t = build_aabb(rects); sink += t.size(); }));

  row("brute", "intersect queries",
      best_ms([&] { sink += brute_query_count(rects, queries); }));
  row("RTree", "intersect queries",
      best_ms([&] { sink += rtree_query_count(rtree, queries); }));
  row("RStarTree", "intersect queries",
      best_ms([&] { sink += rtree_query_count(rstar, queries); }));
  row("AABBTree", "intersect queries",
      best_ms([&] { sink += aabb_query_count(aabb, queries); }));

  std::printf("\n(sink=%zu)\n", static_cast<size_t>(sink));
  return 0;
}
