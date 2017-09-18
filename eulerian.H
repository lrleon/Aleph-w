# ifndef EULERIAN_H
# define EULERIAN_H

# include <tpl_graph.H>

namespace Aleph
{

    /** Determina si un grafo o digrafo es euleriano

	La clase Test_Eulerian efectúa verifica si el grafo o digrafo es
	euleriano; es decir, si contiene un ciclo euleriano.

	La clase recibe tres parámetros tipo:
	-# GT: el grafo
	-# SN: clase filtro para el iterador de nodos.
	-# SA: clase filtro para el iterador de arcos.
	.

	@ingroup Grafos
	@author Leandro R. León
     */
template <class GT, 
	  class SN = Dft_Show_Node<GT>,
	  class SA = Dft_Show_Arc<GT> >
class Test_Eulerian
{
  SN & sn;
  SA & sa;

  bool test_graph(GT & g)
  {
    assert(not g.is_digraph());

    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next())
      if ((g.get_num_arcs(it.get_curr()) % 2) == 1)
	return false;

    return true;
  }

  bool test_digraph(GT & g)
  {
    assert(g.is_digraph());

    g.reset_counter_nodes();

      // una primera pasada sobre arcos para contar grados de entrada
    for (Arc_Iterator<GT, SA> it(g, sa); it.has_current(); it.next())
      NODE_COUNTER(it.get_tgt_node())++;

    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next())
      {
	typename GT::Node * p = it.get_curr();

	if (g.get_num_arcs(p) != NODE_COUNTER(p))
	  return false;
      }

    return true;
  }

public:

  Test_Eulerian(SN && __sn = SN(), SA && __sa = SA()) 
    : sn(__sn), sa(__sa)
  {
    // empty 
  }

      // retorna true si el grafo el euleriano
  bool operator () (GT & g)
  {
    if (g.is_digraph())
      return test_digraph(g);
    else
      return test_graph(g);
  }
};


} // end namespace Aleph

# endif // EULERIAN_H
