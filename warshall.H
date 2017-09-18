
# ifndef WARSHALL_H
# define WARSHALL_H

# include <tpl_graph.H>
# include <tpl_matgraph.H>

namespace Aleph {

    /** Cálculo de la clausura transitiva de una matriz de adyacencia.

        Esta función calcula la clausura transitiva del grafo g mediante
        el algoritmo de Warshall. El resultado se almacena en la matriz
        de bits mat.

        Cada entrada mat(i,j) indica existencia de un camino entre el
        nodo origen con índice i y el nodo destino con índice j. Un
        valor cero indica que no hay ningún camino; un valor 1 que
        existe al menos un camino.

        El procedimiento utiliza dos matrices de bits; una de uso
        interno que es liberada al término del procedimiento y la propia
        matriz mat.
        
        @param[in] g el grafo representado mediante una variante de
        List_Graph.  
        @param[out] mat matriz de bits donde se coloca el resultado.
        @throw bad_alloc si no hay suficiente memoria. 
        @see List_Graph Bit_Mat_Graph

        @ingroup Grafos
     */
    template <class GT, class SA = Dft_Show_Arc<GT> >
void warshall_compute_transitive_clausure(GT &                    g, 
                                          Bit_Mat_Graph<GT, SA> & mat)
{
  Bit_Mat_Graph<GT, SA> mat_prev(g);
  if (mat.get_list_graph() != &g)
    mat.set_list_graph(g);
  const size_t & n = mat.get_num_nodes();
  for (int k = 0; k < n; k++) 
    {
      for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
          mat(i, j) = mat_prev(i, j) or
                      (mat_prev(i, k) and mat_prev(k, j));

      mat_prev = mat;
    }
}

    /** Cálculo de la clausura transitiva de una matriz de adyacencia.

        Esta clase calcula la clausura transitiva de un grafo mediante
        el algoritmo de Warshall. El resultado se almacena en una matriz
        de bits.

        Cada entrada(i,j) en la matriz indica existencia de un camino
        entre el nodo origen con índice i y el nodo destino con índice
        j. Un valor cero indica que no hay ningún camino; un valor 1 que
        existe al menos un camino.

        @see List_Graph Bit_Mat_Graph

        @ingroup Grafos
     */
    template <class GT, class SA = Dft_Show_Arc<GT> >
class Warshall_Compute_Transitive_Clausure
{ 
public:

  /** Invoca al cálculo de la clausura transitiva de un grafo.

      El procedimiento utiliza dos matrices de bits; una de uso
      interno que es liberada al término del procedimiento y la propia
      matriz mat.
        
      @param[in] g el grafo representado mediante una variante de
      List_Graph.  
      @param[out] mat matriz de bits donde se coloca el resultado.
      @throw bad_alloc si no hay suficiente memoria. 
   */
  void operator () (GT & g, Bit_Mat_Graph<GT> & mat) const
  {
    warshall_compute_transitive_clausure <GT, SA> (g, mat);
  }
};

} // end namespace Aleph
# endif // WARSHALL_H

