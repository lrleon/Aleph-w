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
# ifndef EUCLIDIAN_GRAPH_COMMON_H
# define  EUCLIDIAN_GRAPH_COMMON_H

# include <cmath>
# include <cstddef>
# include <limits>

# include <gsl/gsl_rng.h>

# include <tpl_sgraph.H>
# include <io_graph.H>
# include <random_graph.H>

namespace Aleph
{
  /** @brief Simple integer point used as node payload by the helper functors in this header. */
  struct My_P
  {
    long x, y;
  };

  /** @brief Internal RNG handle used by the helper functors in this header.
   *
   * It is set by gen_random_euclidian_graph() and cleared before returning.
   * This is not thread-safe.
   *
   * @note This variable is defined in euclidian-graph-common.C.
   */
  extern gsl_rng *rand_gen;

  template <class GT>
  struct Init_P
  {
    long W, H;

    DynSetAvlTree<std::pair<int, int>> puntos;

    /** @brief Create a node initializer that assigns unique random positions.
     *
     * @param[in] w Maximum x coordinate range (width).
     * @param[in] h Maximum y coordinate range (height).
     */
    Init_P(const long w, const long h)
      : W(w), H(h)
    {
      ah_domain_error_if(W <= 0 or H <= 0)
        << "Init_P(): width and height must be > 0";
    }

    void operator ()(GT &, typename GT::Node *p)
    {
      long x, y;
      while (true)
        {
          x = gsl_rng_uniform_int(rand_gen, W);
          y = gsl_rng_uniform_int(rand_gen, H);
          std::pair<int, int> q(x, y);
          if (puntos.search(q) != nullptr)
            continue;

          puntos.insert(q);
          break;
        }

      My_P & my_p = p->get_info();
      my_p.x = x;
      my_p.y = y;
    }
  };

  template <class GT>
  struct Init_Arc
  {
    int max_offset;

    Init_Arc(const int max) : max_offset(max) {}

    void operator ()(GT & g, typename GT::Arc *a)
    {
      typename GT::Node *src = g.get_src_node(a);
      typename GT::Node *tgt = g.get_tgt_node(a);

      const My_P & psrc = src->get_info();
      const My_P & ptgt = tgt->get_info();

      const double dist = std::hypot(static_cast<double>(psrc.x - ptgt.x),
                                     static_cast<double>(psrc.y - ptgt.y));

      const int offset = max_offset > 0 ? static_cast<int>(gsl_rng_uniform_int(rand_gen, max_offset)) : 0;

      a->get_info() = dist + offset;
    }
  };

  template <class GT>
  struct Wnode
  {
    void operator ()(std::ostream & output, GT &, typename GT::Node *p)
    {
      output << p->get_info().x << " " << p->get_info().y << std::endl;
    }
  };

  template <class GT>
  struct Rnode
  {
    void operator ()(std::istream & input, GT &, typename GT::Node *p)
    {
      input >> p->get_info().x;
      input >> p->get_info().y;
    }
  };

  template <class GT>
  struct Warc
  {
    void operator ()(std::ostream & output, GT &, typename GT::Arc *a)
    {
      output << a->get_info() << std::endl;
    }
  };

  template <class GT>
  struct Rarc
  {
    void operator ()(std::istream & input, GT &, typename GT::Arc *a)
    {
      input >> a->get_info();
    }
  };


  template <class GT>
  inline
  GT gen_random_euclidian_graph(size_t n, size_t m, int w, int h,
                                unsigned int seed)
  {
    ah_domain_error_if(w <= 0 or h <= 0)
      << "gen_random_euclidian_graph(): width and height must be > 0";

    const auto max_unique = static_cast<size_t>(w) * static_cast<size_t>(h);
    ah_domain_error_if(n > max_unique)
      << "gen_random_euclidian_graph(): requested n exceeds available unique grid points";

    rand_gen = gsl_rng_alloc(gsl_rng_mt19937);
    ah_bad_alloc_if(rand_gen == nullptr);
    gsl_rng_set(rand_gen, seed % gsl_rng_max(rand_gen));

    Init_P<GT> initp(w, h);
    const int max_offset = std::max(1,
                                    static_cast<int>(std::ceil(std::hypot(static_cast<double>(w),
                                                                          static_cast<double>(h)))));
    Init_Arc<GT> initarc(max_offset);

    GT g = Random_Graph<GT, Init_P<GT>, Init_Arc<GT>>(seed, initp, initarc)(n, m);

    gsl_rng_free(rand_gen);
    rand_gen = nullptr;

    return g;
  }
} // end namespace Aleph 

# endif //  EUCLIDIAN_GRAPH_COMMON_H
