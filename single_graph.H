# ifndef SINGLE_GRAPH_H
# define SINGLE_GRAPH_H

# include <tpl_dynSetTree.H>

namespace Aleph {

/** Determina si un grafo o digrafo es simple. Un grafo simple es uno
    que notiene arcos repetidos.

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
class Test_Single_Graph
{
  SN & sn;
  SA & sa;

  typedef typename GT::Node GT_Node;

public:

  Test_Single_Graph(SN && __sn = SN(), SA && __sa = SA()) 
    : sn(__sn), sa(__sa)
  {
    // empty 
  }

      /// retorna true si p no tiene multiarcos ni lazos
  bool test_node(typename GT::Node * p)
  {
    DynSetAvlTree<GT_Node*> nodes;

    for (Node_Arc_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next())
      {
	GT_Node * q = it.get_tgt_node();
	if (nodes.exist(q))
	  return false;
	else
	  nodes.insert(p);
      }
    
    return true;
  }

  bool operator () (GT & g)
  {
    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next())
      if (not test_node(it.get_curr()))
	return false;

    return true;
  }
};



} // end namespace Aleph

# endif // SINGLE_GRAPH_H_H
