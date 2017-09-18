# ifndef FLOYD_H
# define FLOYD_H

# include <sstream>
# include <ahFunction.H>
# include <ahSort.H>
# include <tpl_indexArc.H>
# include <tpl_dynMat.H>

namespace Aleph
{

      /** Calcula las matriz de costes de los caminos mínimos
          entre todos pares de nodos de un grafo g y la matriz de caminos
          mínimos según el algoritmo de Floy-Warshall.

          Esta clase utiliza el algoritmo de Floy-Warshall para
          calcular dos matrices de tipo Ady_Mat que se especifican del
          siguiente modo:
          -# dist: matriz de costes mínimos entre todos los pares de
          nodos. Cada entrada dist(i,j) almacena el coste total mínimo
          para ir del nodo con índice i hacia el nodo con índice j. \n
          Recuérdese que un índice de nodo en la matriz dist puede
          calcularse mediante dist(node), donde node es un puntero a
          nodo en una representación con listas enlzadas derivada del
          tipo de grafo
          -# path: matriz de caminos mínimos. Cada entrada path(i,j)
          almacena el nodo k que permitió al algoritmo de Floyd-Warshall 
          encontrar  el mínimo valor de dist(i,j). De este modo, inspecciones 
          sucesivas a  dist(k,j) permiten encontrar y construir el camino
          hacia el nodo j. \n
          La función find_min_path(g,i,j,path) realiza la función
          anterior.

          El algoritmo de Floyd-Warshall maneja pesos negativos, pero no 
          opera correctamente si el grafo contiene ciclos
          negativos. Úsese el algoritmo de Bellman-Ford 
          q_bellman_ford_min_spanning_tree() si se sospecha su presencia.

          El algoritmo utiliza dos matrices internas adicionales.

          El procedimiento es parametrizado con las siguientes
          especificaciones:
          -# GT: el tipo de grafo
          -# Distance<GT>: La clase de lectura del peso del arco  que debe
          exportar los siguientes atributos:
            -# typedef Distance<GT>::Distance_Type: el tipo de dato que
            representa un peso en un arco.
            -# Distance<GT>::Distance_Type operator()(typename GT::Arc *a):
            que retorna el valor del peso en el arco a.
            -# Distance<GT>::Max_Distance: constante estática
            correspondiente al valor de distancia máximo que un algoritmo
            consideraría como valor infinito. 
            -# typename Distance<GT>::Zero_Distance: constante estática
            correspondiente al elemento neutro de la suma. Tradicionalmente,
            en la inmensa mayoría de casos, este será el cero.
            .
          -# SA: filtro de arcos.
          .

          @see dijkstra_min_spanning_tree() dijkstra_min_path() 
          find_path_depth_first() 
          find_min_path() bellman_ford_min_spanning_tree() 
          q_bellman_ford_min_spanning_tree() 
          @ingroup Grafos
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
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next())
      nodes.access(i++) = it.get_curr();
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

  Path<GT> get_min_path(typename GT::Node * src, typename GT::Node * tgt) const
  {
    return get_min_path(index_node(src), index_node(tgt));
  }
};

} // end namespace Aleph

# endif // FLOYD_H

