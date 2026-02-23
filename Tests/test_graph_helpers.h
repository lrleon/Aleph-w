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

# ifndef ALEPH_TEST_GRAPH_HELPERS_H
# define ALEPH_TEST_GRAPH_HELPERS_H

# include <cstddef>
# include <initializer_list>
# include <random>
# include <utility>
# include <vector>

# include <tpl_array.H>

namespace Aleph_Test_Helpers
{
  template <class GT>
  Aleph::Array<typename GT::Node *>
  build_graph_with_unit_arcs(GT & g,
                             const size_t n,
                             const std::vector<std::pair<size_t, size_t>> & edges)
  {
    using Node = typename GT::Node;

    auto nodes = Aleph::Array<Node *>::create(n);
    for (size_t i = 0; i < n; ++i)
      nodes(i) = g.insert_node(static_cast<int>(i));

    for (const auto & [u, v] : edges)
      {
        if (u >= n or v >= n)
          continue;
        g.insert_arc(nodes(u), nodes(v), 1);
      }

    return nodes;
  }

  template <class GT>
  Aleph::Array<typename GT::Node *>
  build_graph_with_unit_arcs(
      GT & g,
      const size_t n,
      const std::initializer_list<std::pair<size_t, size_t>> & edges)
  {
    return build_graph_with_unit_arcs(
        g, n, std::vector<std::pair<size_t, size_t>>(edges));
  }

  template <class GT>
  Aleph::Array<typename GT::Node *>
  build_graph_with_unit_arcs(GT & g,
                             const size_t n,
                             const Aleph::Array<std::pair<size_t, size_t>> & edges)
  {
    using Node = typename GT::Node;

    auto nodes = Aleph::Array<Node *>::create(n);
    for (size_t i = 0; i < n; ++i)
      nodes(i) = g.insert_node(static_cast<int>(i));

    for (typename Aleph::Array<std::pair<size_t, size_t>>::Iterator it(edges);
         it.has_curr(); it.next_ne())
      {
        const auto & e = it.get_curr();
        const size_t u = e.first;
        const size_t v = e.second;
        if (u >= n or v >= n)
          continue;
        g.insert_arc(nodes(u), nodes(v), 1);
      }

    return nodes;
  }

  struct Positive_Arc_Filter
  {
    template <class Arc>
    bool operator()(Arc * a) const noexcept
    {
      return a->get_info() > 0;
    }
  };

  template <class EdgeContainer>
  EdgeContainer make_random_tree_edges(const size_t n, std::mt19937 & rng)
  {
    EdgeContainer edges;
    if constexpr (requires(EdgeContainer e, size_t cap) { e.reserve(cap); })
      edges.reserve(n > 0 ? n - 1 : 0);

    for (size_t v = 1; v < n; ++v)
      {
        std::uniform_int_distribution<size_t> pick(0, v - 1);
        auto e = std::make_pair(v, pick(rng));
        if constexpr (requires(EdgeContainer c, const std::pair<size_t, size_t> & p)
                      {
                        c.append(p);
                      })
          edges.append(e);
        else
          edges.emplace_back(e);
      }

    return edges;
  }
}

# endif
