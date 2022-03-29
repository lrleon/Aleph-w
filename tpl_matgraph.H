

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

      template <class GT, class SA = Dft_Show_Arc<GT> >
class Bit_Mat_Graph
{
public:

        /// El tipo de grafo GT.
  typedef GT Graph_Type;

        /// El tipo de nodo en GT.
  typedef typename GT::Node Node;

        /// El tipo de arco en GT.
  typedef typename GT::Arc Arc;

private:

  BitArray bit_array;
  GT *                         lgraph;
  DynArray<typename GT::Node*> nodes;
  mutable size_t               n;
  
  void copy_list_graph(GT & g)
  {
    n = g.get_num_nodes();
    nodes.cut(); // liberar memoria 

    // copiar todos los nodos de g al arreglo nodes[]
    int i = 0;
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne())
      nodes[i++] = static_cast<typename GT::Node*>(it.get_current_node_ne());

    quicksort(nodes); // ordenar para luego hacer búsqueda binaria

    // recorrer todos los nodos de g para asignar los arcos en bit_array
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne())
      {
	typename GT::Node * src = it.get_current_node_ne();
	const size_t src_idx = node_index<Graph_Type>(nodes, n, src);
          
	// recorrer los arcos de src para asignar entrada en matriz
	for (Node_Arc_Iterator<GT, SA> jt(src); jt.has_curr(); jt.next_ne())
	  { 
	    typename GT::Node * tgt = jt.get_tgt_node_ne();
	    const size_t tgt_idx = node_index<Graph_Type>(nodes, n, tgt);
	    bit_array[index_array(src_idx, tgt_idx, n)] = 1;
	  }
      }
  }

  struct Proxy
  {
    BitArray & bit_array;

    const size_t bit_index;

    Proxy(Bit_Mat_Graph & __bitmat, const long & i, const long & j)
      : bit_array(__bitmat.bit_array), bit_index(index_array(i, j, __bitmat.n))
    {
      // empty
    }

    Proxy& operator = (const Proxy & proxy)
    {
      bit_array[bit_index] = proxy.bit_array[proxy.bit_index];
    }
      
    Proxy& operator = (const int & i)
    {
      bit_array[bit_index] = i;

      return *this;
    }

    operator int () const
    {
      return bit_array[bit_index];
    }
  };

public: 

        /// Retorna el número de nodos del grafo (dimensión de la matriz).
  const size_t & get_num_nodes() const { return n; }

      /// Constructor vacío.
  Bit_Mat_Graph() : lgraph(nullptr) {}

       /// Constructor a partir de un grafo g representado con listas de
       /// adyacencia. 
  Bit_Mat_Graph(GT & g) 
    : bit_array(g.get_num_nodes()*g.get_num_nodes()), lgraph(&g)
  { 
    copy_list_graph(g); 
  }
  
      /// Constructor copia.
  Bit_Mat_Graph(const Bit_Mat_Graph & bitmat)
    : bit_array(bitmat.bit_array), lgraph(bitmat.lgraph), 
      nodes(bitmat.nodes), n(bitmat.n) 
  {
    // empty
  }

        /// Constructor especificando una dimensión.
  Bit_Mat_Graph(const size_t & dim) 
    : bit_array(dim*dim), lgraph(nullptr), nodes(dim), n(dim) 
  {
    // empty
  }

      /// Asocia la matriz de bits al grafo g representado con listas de
      /// adyacencia. 
  void set_list_graph(GT & g)
  {
    const size_t & n = g.get_num_nodes();
    bit_array.set_size(n*n);
    lgraph = &g;
    copy_list_graph(g);
  }

        /// Retorna un puntero al grafo representado con listas de
        /// adyacencia; si el valor es nullptr, entonces no hay grafo asociado.
  GT * get_list_graph() { return lgraph; }

         /// Asignación de matriz.
  Bit_Mat_Graph & operator = (const Bit_Mat_Graph & bitmat)
  {
    if (this == &bitmat) 
      return *this;

    lgraph    = bitmat.lgraph;
    bit_array = bitmat.bit_array;

    return *this;
  }

         /// Asignación de grafo representado con listas de adyacencia.
  Bit_Mat_Graph & operator = (GT & g)
  {
    if (&g == lgraph) 
      return *this;

    copy_list_graph(g);

    return *this;
  }
  
      /// Retorna el puntero al nodo en la representación con listas de
      /// adyacencia correspondiente al índice i.
  Node * operator () (const long & i) 
  {
    if (lgraph == nullptr)
      throw std::domain_error("There is no a GT object");

    return Aleph::get_node<GT>(nodes, i);
  }
        /// Retorna el índice en la matriz del nodo en la representación con
        /// listas de adyacencia.
  long operator () (Node * node) const
  {
    if (lgraph == nullptr)
      throw std::domain_error("There is no a GT object");

    return node_index<GT>(nodes, n, node);
  }

  Proxy operator () (const long & i, const long & j)
  {
    return Proxy(*this, i, j);
  }

  Proxy operator () (const long & i, const long & j) const
  {
    return Proxy(const_cast<Bit_Mat_Graph&>(*this), i, j);
  }

  Proxy operator () (Node * src_node, Node * tgt_node) 
  {
    if (lgraph == nullptr)
      throw std::domain_error("There is no a GT object");

    return Proxy(*this, node_index<GT>(nodes, n, src_node), 
		 node_index<GT>(nodes, n, tgt_node)); 
  }

  Proxy operator () (Node * src_node, Node * tgt_node) const
  {
    if (lgraph == nullptr)
      throw std::domain_error("There is no a GT object");

    return Proxy(*this, node_index<GT>(nodes, n, src_node), 
		 node_index<GT>(nodes, n, tgt_node)); 
  }
};

} // end namespace Aleph

# endif // TPL_MAT_GRAPH_H
