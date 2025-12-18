
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
# ifndef GRAPH_TO_TREE_H
# define GRAPH_TO_TREE_H

# include <tpl_tree_node.H>
# include <tpl_graph.H>
# include <tpl_graph_utils.H>
 # include <ah-errors.H>

namespace Aleph {

    template <class GT, typename Key, class Convert> static
Tree_Node<Key> * graph_to_tree_node(GT & g, typename GT::Node * groot);
    template <class GT, typename Key, class Convert> static void 
__graph_to_tree_node(GT & g, typename GT::Node * groot, 
                     Tree_Node<Key> * troot);

  template <class GT, typename Key, typename Convert, class SA> static inline
void __graph_to_tree_node(GT & g, typename GT::Node * groot,
                          Tree_Node<Key> * troot);

     /** Convierte un árbol representado en una representación de grafo a un 
         árbol Tree_Node.

         graph_to_tree_node() toma un árbol representado mediante algún
         tipo de grafo representado con listas de adyacencia y un nodo 
         groot que se asume raíz del árbol y lo convierte a un árbol
         representado con el tipo Tree_Node<Key>.

         El interés de esta rutina es obtener una representación del
         árbol en la cual se puedan aplicar otras técnicas, clases y
         algoritmos. Un ejemplo representativo es el dibujado de árboles
         inherentes a los grafos; en la ocurrencia, árboles
         abarcadores. 

         El procedimiento utiliza el bit spanning_tree para marcar los nodos 
         y arcos ya visitados.

         Como medio de representación, el tipo Tree_Node<Key> es menos
         versátil que cualquier tipo basado en un grafo representado con
         listas de adyacencia; por ejemplo, List_graph. Note, por
         instancia, que List_Graph<Node,Arc> estipula un tipo para los
         arcos, mientras que no ocurre lo mismo para Tree_Node<Key>. En
         este sentido, como representación, Tree_Node<Key> no tiene
         ningún medio para guardar los datos asociados a los
         arcos. Consecuentemente, sólo se podría manejar los datos
         contenidos en los nodos. Por otra parte, el tipo guardado en
         GT::Node puede ser de índole distinta al guardado en Tree_Node<Key>.

         A menudo (ese es el caso típico del dibujado), el tipo de dato
         a guardar en Tree_Node<Key> es de índole distinta al guardado
         en GT::Node. Otras veces, lo que se desea guardar en cada
         Tree_Node<Key> es un subconjunto de lo que está guardado en
         GT::Node. Puesto que es imposible generizar todos los casos
         posibles, la escritura de cada Tree_Node<Key> se delega a una
         clase cuyo operador ()() es invocado por la rutina de
         conversión para cada nodo del grafo. La clase en cuestión es el
         parámetro tipo Convert.

         La clase Convert se invoca de la siguiente manera:  

             void Convert::operator () (gnode, tnode)

             donde:
             -# gnode es de tipo GT::Node
             -# tnode de tipo Tree_Node<Key>

         La rutina hace una prueba de aciclicidad sobre g mediante la
         función is_graph_acyclique(), lo que no afecta la complejidad
         algorítmica de pero que toma tiempo adicional.

         El procedimiento maneja los siguientes tipos parametrizados:
         -# GT: el tipo de grafo, el cual debe ser derivado de List_Graph.
         -# Key: el tipo de dato que albergará el árbol en su
         representación Tree_Node<Key>.
         -# Convert: clase de conversión de GT::Node a Key, cuyo
         operador ()(gnode,tnode) es invocado por cada nodo de g cuando
         se realiza la conversión a uno Tree_Node.
         -# SA: filtro de arcos.

         @param[in] g el árbol de tipo GT (derivado de List_Graph) que
         se desea convertir.
         @param[in] groot el nodo de g que se considera raíz del árbol.
         @return la raíz de un árbol de tipo Tree_Node<Key>
         correspondiente a la conversión
         @throw bad_alloc si no hay memoria para apartar el árbol de
         tipo Tree_Node<Key>
         @ingroup Grafos
         @see Tree_Node is_graph_acyclique() 
      */
    template <class GT, typename Key, 
              class Convert, class SA = Dft_Show_Arc<GT>> inline
Tree_Node<Key> * graph_to_tree_node(GT & g, typename GT::Node * groot)
{ 
  ah_domain_error_if(not is_graph_acyclique<GT>(g))
    << "Graph is not a tree (not acyclique)";

  Tree_Node<Key> * troot = new Tree_Node<Key>; // apartar memoria raíz

  Convert () (groot, troot); //convertir de groot y copiar a troot

  __graph_to_tree_node <GT, Key, Convert, SA> (g, groot, troot);

  return troot;
}

     /** Convierte un árbol representado en una representación de grafo a un 
         árbol Tree_Node.

         graph_to_tree_node() toma un árbol representado mediante algún
         tipo de grafo representado con listas de adyacencia y un nodo 
         groot que se asume raíz del árbol y lo convierte a un árbol
         representado con el tipo Tree_Node<Key>.

         El interés de esta rutina es obtener una representación del
         árbol en la cual se puedan aplicar otras técnicas, clases y
         algoritmos. Un ejemplo representativo es el dibujado de árboles
         inherentes a los grafos; en la ocurrencia, árboles
         abarcadores. 

         El procedimiento utiliza el bit spanning_tree para marcar los nodos 
         y arcos ya visitados.

         Como medio de representación, el tipo Tree_Node<Key> es menos
         versátil que cualquier tipo basado en un grafo representado con
         listas de adyacencia; por ejemplo, List_graph. Note, por
         instancia, que List_Graph<Node,Arc> estipula un tipo para los
         arcos, mientras que no ocurre lo mismo para Tree_Node<Key>. En
         este sentido, como representación, Tree_Node<Key> no tiene
         ningún medio para guardar los datos asociados a los
         arcos. Consecuentemente, sólo se podría manejar los datos
         contenidos en los nodos. Por otra parte, el tipo guardado en
         GT::Node puede ser de índole distinta al guardado en Tree_Node<Key>.

         A menudo (ese es el caso típico del dibujado), el tipo de dato
         a guardar en Tree_Node<Key> es de índole distinta al guardado
         en GT::Node. Otras veces, lo que se desea guardar en cada
         Tree_Node<Key> es un subconjunto de lo que está guardado en
         GT::Node. Puesto que es imposible generizar todos los casos
         posibles, la escritura de cada Tree_Node<Key> se delega a una
         clase cuyo operador ()() es invocado por la rutina de
         conversión para cada nodo del grafo. La clase en cuestión es el
         parámetro tipo Convert.

         La clase Convert se invoca de la siguiente manera:  

             void Convert::operator () (gnode, tnode)

             donde:
             -# gnode es de tipo GT::Node
             -# tnode de tipo Tree_Node<Key>

         La rutina hace una prueba de aciclicidad sobre g mediante la
         función is_graph_acyclique(), lo que no afecta la complejidad
         algorítmica de pero que toma tiempo adicional.

         El procedimiento maneja los siguientes tipos parametrizados:
         -# GT: el tipo de grafo, el cual debe ser derivado de List_Graph.
         -# Key: el tipo de dato que albergará el árbol en su
         representación Tree_Node<Key>.
         -# Convert: clase de conversión de GT::Node a Key, cuyo
         operador ()(gnode,tnode) es invocado por cada nodo de g cuando
         se realiza la conversión a uno Tree_Node.
         -# SA: el filtro de arcos.

         @ingroup Grafos
         @see Tree_Node is_graph_acyclique() 
      */
    template <class GT, typename Key, 
	      class Convert, 
	      class SA = Dft_Show_Arc<GT> > 
class Graph_To_Tree_Node
{
  SA        sa;
  Convert * convert = nullptr;

  void graph_to_tree(typename GT::Node * groot, Tree_Node<Key> * troot)
  {           
    typedef typename GT::Node Node;
    typedef typename GT::Arc Arc;

        // recorrer arcos de groot y construir recursivamente
    for (Node_Arc_Iterator<GT, SA> it(groot, sa); it.has_curr(); it.next_ne())
      {
	Arc * arc = it.get_current_arc_ne();
	if (IS_ARC_VISITED(arc, Convert_Tree)) 
	  continue;

	ARC_BITS(arc).set_bit(Convert_Tree, true); // arc visitado 
	Node * gtgt = it.get_tgt_node();
	Tree_Node<Key> * ttgt = new Tree_Node<Key>; 

	(*convert) (gtgt, ttgt); // asignarle la clave

	troot->insert_rightmost_child(ttgt); // insertarlo como hijo 

	graph_to_tree(gtgt, ttgt);
      }
  }

  Tree_Node<Key> * graph_to_tree(GT & g,
				 typename GT::Node * groot,
				 Convert & conv)
  { 
    ah_domain_error_if(not is_graph_acyclique<GT>(g))
      << "Graph is not a tree (not acyclique)";

    Tree_Node<Key> * troot = new Tree_Node<Key>; // apartar memoria raíz

    convert = &conv;

    (*convert) (groot, troot); //convertir de groot y copiar a troot

    graph_to_tree(groot, troot);

    return troot;
  }

public:

  Graph_To_Tree_Node(SA __sa = SA()) : sa(__sa) { /* empty */ }

  /** Invoca a la conversión de grafo hacia árbol abarcador.

      @param[in] g el árbol de tipo GT (derivado de List_Graph) que
      se desea convertir.
      @param[in] groot el nodo de g que se considera raíz del árbol.
      @param[in] conv clase de conversión.
      @return la raíz de un árbol de tipo Tree_Node<Key>
      correspondiente a la conversión
      @throw bad_alloc si no hay memoria para apartar el árbol de
      tipo Tree_Node<Key>      
   */
      Tree_Node<Key> * 
  operator () (GT & g, typename GT::Node * groot, Convert && conv = Convert())
  { 
    return graph_to_tree(g, groot, conv);
  }

      Tree_Node<Key> * 
  operator () (GT & g, typename GT::Node * groot, Convert & conv)
  { 
    return graph_to_tree(g, groot, conv);
  }
};


  template <class GT, typename Key, typename Convert, class SA> static inline
void __graph_to_tree_node(GT & g, typename GT::Node * groot,
                          Tree_Node<Key> * troot)
{           
  typedef typename GT::Node Node;
  typedef typename GT::Arc Arc;

      // recorrer arcos de groot y construir recursivamente
  for (Node_Arc_Iterator<GT, SA> it(groot); it.has_curr(); it.next_ne())
    {
      Arc * arc = it.get_current_arc_ne();
      if (IS_ARC_VISITED(arc, Convert_Tree)) 
        continue;

      ARC_BITS(arc).set_bit(Convert_Tree, true); // arc visitado 
      Node * gtgt = it.get_tgt_node();
      Tree_Node<Key> * ttgt = new Tree_Node<Key>; 

      Convert () (gtgt, ttgt); // asignarle la clave

      troot->insert_rightmost_child(ttgt); // insertarlo como hijo 

      __graph_to_tree_node <GT, Key, Convert, SA> (g, gtgt, ttgt);
    }
}

} // end namespace Aleph

# endif // GRAPH_TO_TREE_H
