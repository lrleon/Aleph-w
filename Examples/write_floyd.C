
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


/**
 * @file write_floyd.C
 * @brief Floyd-Warshall algorithm with LaTeX output generation
 * 
 * This example demonstrates the Floyd-Warshall algorithm for finding
 * shortest paths between all pairs of vertices in a weighted graph.
 * It generates step-by-step LaTeX matrices showing the algorithm's
 * progression, making it ideal for educational purposes and algorithm
 * visualization.
 *
 * ## What is Floyd-Warshall?
 *
 * ### Problem
 *
 * **All-pairs shortest paths**: Find shortest paths between every pair
 * of vertices in a weighted graph.
 *
 * ### Solution: Floyd-Warshall Algorithm
 *
 * Floyd-Warshall is a **dynamic programming** algorithm that finds shortest
 * paths between all pairs of vertices in a single run. Unlike Dijkstra
 * (single-source) or Johnson (all-pairs using Dijkstra), Floyd-Warshall
 * works directly with the adjacency matrix.
 *
 * ### Key Characteristics
 *
 * - **All-pairs**: Finds all shortest paths in one algorithm run
 * - **Matrix-based**: Works with adjacency/distance matrix
 * - **Dynamic programming**: Builds solution incrementally
 * - **Negative weights**: Handles negative edges (but not negative cycles)
 *
 * ## Algorithm Overview
 *
 * ### Dynamic Programming Recurrence
 *
 * The algorithm uses dynamic programming with the recurrence:
 * ```
 * D^(k)[i][j] = min(D^(k-1)[i][j], D^(k-1)[i][k] + D^(k-1)[k][j])
 * ```
 *
 * Where:
 * - **D^(k)[i][j]**: Shortest path from i to j using only vertices {1..k}
 * - **D^(0)[i][j]**: Direct edge weight (or ∞ if no edge)
 * - **D^(V)[i][j]**: Final shortest path distance
 *
 * ### Key Insight
 *
 * At iteration k, we consider whether going through vertex k gives a
 * shorter path than the current best path:
 * - **Option 1**: Current best path D^(k-1)[i][j]
 * - **Option 2**: Path through k: D^(k-1)[i][k] + D^(k-1)[k][j]
 * - **Choose**: Minimum of the two
 *
 * ### Algorithm Pseudocode
 *
 * ```
 * Floyd-Warshall(G):
 *   // Initialize distance matrix
 *   for each vertex i:
 *     for each vertex j:
 *       D[i][j] = weight(i,j) if edge exists, else ∞
 *   
 *   // Main algorithm
 *   for k = 1 to V:
 *     for i = 1 to V:
 *       for j = 1 to V:
 *         D[i][j] = min(D[i][j], D[i][k] + D[k][j])
 * ```
 *
 * ## Complexity
 *
 * ### Time Complexity
 *
 * - **O(V³)**: Three nested loops over vertices
 * - **Independent of edges**: Same for sparse and dense graphs
 * - **Cubic**: Can be slow for large graphs
 *
 * ### Space Complexity
 *
 * - **O(V²)**: Distance matrix
 * - **Can optimize**: Use single matrix (in-place updates)
 *
 * ### Comparison
 *
 * | Algorithm | Time | Space | Best For |
 * |-----------|------|-------|----------|
 * | Floyd-Warshall | O(V³) | O(V²) | Dense graphs |
 * | Johnson | O(V² log V + VE) | O(V²) | Sparse graphs |
 * | V × Dijkstra | O(V(V log V + E)) | O(V²) | Non-negative only |
 *
 * ## Advantages
 *
 * - **All-pairs**: Finds all shortest paths in one run
 * - **Negative weights**: Handles negative edge weights
 * - **Simple**: Easy to implement and understand
 * - **Path reconstruction**: Can reconstruct actual paths
 * - **Transitive closure**: Can find reachability (set weights to 1)
 *
 * ## Limitations
 *
 * - **No negative cycles**: Algorithm fails if graph has negative cycles
 * - **Cubic time**: Slower than Johnson for sparse graphs
 * - **Dense graphs**: Best suited for dense graphs (many edges)
 * - **Memory**: O(V²) space requirement
 *
 * ## Applications
 *
 * ### Network Routing
 * - **Communication networks**: Find shortest paths between routers
 * - **Internet routing**: All-pairs routing tables
 * - **Network analysis**: Understand network topology
 *
 * ### Graph Analysis
 * - **Graph diameter**: Longest shortest path
 * - **Eccentricity**: Maximum distance from vertex
 * - **Centrality**: Betweenness, closeness centrality
 *
 * ### Transitive Closure
 * - **Reachability**: Determine if paths exist (set weights to 1)
 * - **Connectivity**: Find strongly connected components
 * - **Dependency analysis**: Analyze dependencies
 *
 * ### Social Networks
 * - **Shortest paths**: Find shortest connection paths
 * - **Degrees of separation**: Measure social distance
 * - **Network metrics**: Analyze network structure
 *
 * ### Transportation
 * - **All-pairs routes**: Find shortest routes between all cities
 * - **Route planning**: Plan routes efficiently
 * - **Logistics**: Optimize transportation networks
 *
 * ## Output Files
 *
 * ### LaTeX Matrices
 *
 * Generates `mat-floyd.tex` containing:
 * - **D^(0)**: Initial distance matrix (direct edges only)
 * - **D^(k)**: Distance matrices after each intermediate vertex k
 * - **Path matrix**: For reconstructing actual paths
 * - **Formatted**: All as LaTeX tables ready for compilation
 *
 * ### Educational Value
 *
 * The LaTeX output shows:
 * - **Step-by-step**: How distances update at each iteration
 * - **Visualization**: Easy to see algorithm progression
 * - **Documentation**: Can be included in papers/presentations
 *
 * ## Test Graph
 *
 * ### Graph Structure
 *
 * Uses a predefined 9-node directed graph (labeled A through I) with:
 * - **Weighted edges**: Some positive, some negative
 * - **No negative cycles**: Algorithm works correctly
 * - **Mixed weights**: Demonstrates algorithm behavior
 *
 * ### Graph Properties
 *
 * - **Directed**: Edges have direction
 * - **Weighted**: Edges have weights (can be negative)
 * - **No negative cycles**: Assumed (otherwise shortest paths are undefined)
 *
 * ## Usage
 *
 * ```bash
 * # Generate LaTeX output
 * ./write_floyd
 *
 * # Compile LaTeX to PDF
 * pdflatex mat-floyd.tex
 *
 * # View results
 * evince mat-floyd.pdf  # or your PDF viewer
 * ```
 *
 * ## Path Reconstruction
 *
 * ### How to Reconstruct Paths
 *
 * The algorithm maintains a **next-hop** matrix `P`:
 * - `P[i][j]` stores the next vertex index to move to when going from `i` to `j`
 *   along a shortest path.
 *
 * A simple reconstruction procedure is:
 * ```
 * reconstruct_path(i, j):
 *   if D[i][j] == ∞:
 *     return []
 *   path = [i]
 *   while i != j:
 *     i = P[i][j]
 *     path.append(i)
 *   return path
 * ```
 *
 * **Time**: O(path_length) to reconstruct one path
 *
 * ## Negative Cycle Detection
 *
 * ### How to Detect Negative Cycles
 *
 * In general, a negative cycle implies that at least one diagonal entry becomes
 * negative (some `D[i][i] < 0`) after relaxation.
 *
 * Note: this example generates the LaTeX trace but does **not** perform an
 * explicit negative-cycle check.
 *
 * @see johnson_example.cc Johnson's algorithm (better for sparse graphs)
 * @see dijkstra_example.cc Dijkstra's algorithm (single-source)
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

# include <limits>
# include <iostream>
# include <tpl_matgraph.H>
# include <mat_latex.H>
# include <tpl_graph_utils.H>
# include <latex_floyd.H>

using namespace std;

using namespace Aleph;

# define INDENT "    "

    template <typename M>
struct C_i
{
  string operator () (M & m, const long & i) const
  {
    return m(i)->get_info().nombre;
  }
};


    template <typename M>
struct C_ij
{
  string operator () (M & m, const long & i, const long & j)
  {
    return m(m(i, j))->get_info().nombre;
  }
};


    template <typename M>
struct Di_ij
{
  string operator () (M & mat, const long & i, const long & j)
  {
    typename M::Arc_Type::Distance_Type value = mat(i, j);

    if (value == numeric_limits<double>::infinity())
      return string("{$\\infty$}");

    if (value == 0)
      return string("{$0$}");

    char buf[256]; 

    snprintf(buf, 256, "{$%d$}", (int) value);

    return string(buf);
  }
};

struct Nodo
{
  string nombre;

  Nodo() {}

  Nodo(const string & str) : nombre(str) { /* empty */ }

  Nodo(const char * str) : nombre(str) { /* empty */ }

  bool operator == (const Nodo & der) const 
  {
    return nombre == der.nombre; 
  }
};


struct Arco
{
  typedef double Distance_Type;

  static const double Max_Distance;

  static const double Zero_Distance;

  double distancia;

  Arco() : distancia(Max_Distance) { /* empty */ }

  Arco(const double & dist) : distancia(dist) 
  {
    /* empty */ 
  }

  const double & get_distance() const { return distancia; }

  operator double & () { return distancia; }
}; 

const Arco Arco_Zero(0);

const double Arco::Max_Distance = numeric_limits<double>::infinity();

const double Arco::Zero_Distance = 0;


typedef Graph_Node<Nodo> Node_Nodo;

typedef Graph_Arc<Arco> Arco_Arco;

typedef List_Digraph<Node_Nodo, Arco_Arco> Grafo;


void insertar_arco(Grafo &       grafo, 
		  const string & src_name, 
		  const string & tgt_name,
		  const double & distancia)
{
  Grafo::Node * n1 = grafo.find_node(Nodo(src_name));

  if (n1 == nullptr)
    n1 = grafo.insert_node(src_name);

  Grafo::Node * n2 = grafo.find_node(Nodo(tgt_name));

  if (n2 == nullptr)
    n2 = grafo.insert_node(tgt_name);

  grafo.insert_arc(n1, n2, Arco(distancia));
}



void build_test_graph(Grafo & g)
{
  g.insert_node("A");
  g.insert_node("B");
  g.insert_node("C");
  g.insert_node("D");
  g.insert_node("E");
  g.insert_node("F");
  g.insert_node("G");
  g.insert_node("H");
  g.insert_node("I");

  insertar_arco(g, "A", "B", 2);
  insertar_arco(g, "A", "F", 5);
  insertar_arco(g, "B", "F", 1);
  insertar_arco(g, "B", "D", 3);
  insertar_arco(g, "C", "A", 1);  
  insertar_arco(g, "C", "E", 4);  
  insertar_arco(g, "F", "D", -2);
  insertar_arco(g, "F", "C", -1);
  insertar_arco(g, "F", "E", 2);
  insertar_arco(g, "D", "F", 2);
  insertar_arco(g, "D", "H", 4);
  insertar_arco(g, "E", "G", 2);
  insertar_arco(g, "E", "I", -2);
  insertar_arco(g, "G", "D", 3);
  insertar_arco(g, "G", "F", -1);
  insertar_arco(g, "G", "H", 2);
  insertar_arco(g, "H", "D", -2);
  insertar_arco(g, "H", "G", -1);
  insertar_arco(g, "I", "G", 4);
  insertar_arco(g, "I", "H", 3);

}

void copia_Arco_Arco(Arco_Arco * arc, const long&, const long&, double& value)
{
  value = arc->get_info().get_distance();
}


int main() 
{
  Grafo list_graph;

  build_test_graph(list_graph);

  Ady_Mat<Grafo, double> cost_mat(list_graph);

  Ady_Mat<Grafo, long> path_mat(list_graph);

  ofstream out("mat-floyd.tex", ios::trunc); 

  floyd_all_shortest_paths_latex <Grafo, C_i, C_ij, Di_ij> 
    (list_graph, cost_mat, path_mat, out);
}
