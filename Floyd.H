

/*
                          Aleph_w

  Data structures & Algorithms
  version 1.9d
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2022 Leandro Rabindranath Leon

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

    template <class GT,
              class Distance = Dft_Dist<GT>,
              class SA       = Dft_Show_Arc<GT>>
class Floyd_All_Shortest_Paths
{
  typedef typename GT::Node Node;
  typedef typename GT::Arc Arc;
  typedef typename Distance::Distance_Type Distance_Type;

  DynArray<Node*> nodes;
  GT & g;
  const long n;
  const Distance_Type Inf;
  bool negative_cycle = false;
  DynMatrix<long> path_mat;
  DynMatrix<Distance_Type> dist;
  SA & sa;

public:

  bool has_negative_cycle() const noexcept { return negative_cycle; }

  const DynMatrix<long> & get_path_mat() const noexcept { return path_mat; }

  const DynMatrix<Distance_Type> & get_dist_mat() const noexcept
  {
    return dist;
  }

  const DynArray<Node*> get_nodes() const noexcept { return nodes; }

  /// Retorna el nodo correspondiente al índice i en una matriz de
  /// adyacencia para el algoritmo de Floyd-Warshall
  Node * select_node(long i) const noexcept { return nodes(i); }

  /// Retorna el ìndice dentro de una matriz de adyacencia que tendría
  /// el nodo p
  long index_node(Node * p) const noexcept
  {
    auto i = binary_search(nodes, p);
    if (i < 0 or i > nodes.size() or nodes(i) != p)
      throw domain_error("Floyd_All_Shortest_Paths::index_node() not found");
    return i;
  }

public:

  Floyd_All_Shortest_Paths(const GT & __g, SA & __sa)
  : g(const_cast<GT&>(__g)), n(g.get_num_nodes()),
    Inf(std::numeric_limits<Distance_Type>::max()),
    path_mat(n, n), dist(n, n), sa(__sa)
  {
    int i = 0;
    nodes.reserve(g.get_num_nodes());
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne())
      nodes.access(i++) = it.get_curr_ne();
    in_place_sort(nodes); // ordena por punteros a nodo

    dist.allocate();
    path_mat.allocate();

    {    // inicializa matrices
      IndexArc<GT, Rand_Tree, SA> arcs(g, true, sa);

      for (long i = 0; i < n; ++i)
        {
          Node * src = nodes(i);
          for (long j = 0; j < n; ++j)
            {
              if (i == j)
                {
                  dist(i, j) = 0;
                  continue;
                }

              Node * tgt = nodes(j);
              Arc * arc  = arcs.search_directed(src, tgt);
              if (arc == nullptr)
                {
                  dist(i, j) = Inf;
                  continue;
                }

              dist(i, j) = Distance () (arc);
              path_mat(i, j) = j;
            }
        }
    }

    for (long k = 0; k < n; ++k)
      for (long i = 0; i < n; ++i)
        {
          const Distance_Type & dik = dist(i, k);
          for (long j = 0; j < n; ++j)
            {
              const Distance_Type & dij = dist(i, j);
              if (dik == Inf)
                continue;

              const Distance_Type & dkj = dist(k, j);
              if (dkj == Inf)
                continue;

    	          // calcule nueva distancia pasando por k
              Distance_Type new_dist = dik + dkj;
              if (new_dist < dij)
                {
                  dist(i, j) = new_dist; // actualice menor distancia
                  path_mat(i, j) = path_mat(i, k);
                }
            }
        }

    for (i = 0; i < n; ++i)
      if (dist(i, i) < 0) // ciclo negativo?
        {
          negative_cycle = true;
          break;
        }
  }

  Floyd_All_Shortest_Paths(const GT & g, SA && sa = SA())
    : Floyd_All_Shortest_Paths(g, sa) {}

  string entry(const Distance_Type & e)
  {
    if (e == Inf)
      return "Inf";

    std::stringstream ss;
    ss << e;
    return ss.str();
  }

  static void print(DynMatrix<Distance_Type> & dist)
  {
    Distance_Type Inf = std::numeric_limits<Distance_Type>::max();
    const int n = dist.rows();
    for (int i = 0; i < n; ++i)
      {
        for (int j = 0; j < n; ++j)
          if (dist.access(i,j) == Inf)
            cout << "inf ";
          else
            cout << dist.access(i,j) << " ";
        cout << endl;
      }
    cout << endl;
  }

  Path<GT> get_min_path(const long src_idx, const long tgt_idx) const
  {
    auto src = nodes(src_idx);

    Path<GT> path(g, src);

    if (src_idx == tgt_idx)
      return path;

    long i = src_idx;
    while (true)
      {
        const auto & k = path_mat(i, tgt_idx);
        auto p = nodes(k);
        path.append_directed(p);
        if (k == tgt_idx)
          break;
        else
          i = k;
      }

    return path;
  }

  Path<GT> get_min_path(typename GT::Node * src,
                        typename GT::Node * tgt) const
  {
    return get_min_path(index_node(src), index_node(tgt));
  }
};

} // end namespace Aleph

# endif // FLOYD_H

