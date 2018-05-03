
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

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
# ifndef TPL_MAT_GRAPH_H
# define TPL_MAT_GRAPH_H

# include <tpl_graph.H>
# include <tpl_sort_utils.H>

using namespace Aleph;

namespace Aleph 
{

      template <typename GT> inline static 
typename GT::Node * get_node(DynArray<typename GT::Node *> & nodes, 
			     const long &                    i)
{
  if (i >= nodes.size())
    throw std::out_of_range("Index of node out of range");

  return nodes.access(i);
} 

      template <typename GT> inline static long 
node_index(DynArray<typename GT::Node *> & nodes, 
	   const long & n, typename GT::Node * p) 
{
  return Aleph::binary_search(nodes, p, 0, n - 1);
}

      inline static long 
index_array(const long & i, const long & j, const long & n) 
{
  if (i >= n or j >= n)
    throw std::out_of_range("Matrix index out of range");

  return i + j*n;
}

      template <typename Entry>
inline static Entry * read_matrix(const DynArray<Entry> & mat,
				  const long & i, const long & j, 
				  const long & n)
{
  const long index = index_array(i, j, n);
  if (not mat.exist(index)) 
    return nullptr;

  return &const_cast<Entry&>(mat.access(index));
}

      template <typename Entry>
inline static void write_matrix(DynArray<Entry> & mat,
				const long &      i, 
				const long &      j, 
				const long &      n, 
				const Entry &     entry)
{
  mat[index_array(i, j, n)] = entry;
}

        /** Matriz de adyacencia de un grafo mapeada a un grafo
            representado mediante una variante de GT.
            
            La clase Map_Matrix_Graph modeliza una matriz de adyacencia
            mapeo hacia los arcos de un grafo representado con listas de
            adyacencia. Ella constituye la más simple manera de
            construir una matriz de adyacencia en Aleph. Se trata de una
            matriz cuyas entradas son punteros hacia arcos de un grafo
            representado mediante alguna clase basada en GT. Una
            entrada con valor nullptr indica ausencia de arco.

            Para construir un objeto de tipo
            Map_Matrix_Graph se requiere tener una instancia del grafo
            basada en un objeto derivado de GT.

            El acceso a la matriz se realiza mediante el operador (i,j). 
            Si mat es un objeto de tipo Map_Matrix_Graph, entonces el
            mat(i,j) refiere al arco entre el nodo i y j. Un valor igual a nullptr
            indica que no hay arco; de lo contrario, mat(i,j) es un
            apuntador de tipo GT::Arc al arco en la representación con
            listas de adyacencia.

            Las entradas a las matriz también pueden referirse mediante
            punteros a nodos. En este sentido mat(src,tgt) refiere al
            arco entre los nodos apuntados por src y tgt.

            Puede decirse que el uso de memoria de un objeto
            Map_Matrix_Graph trata de ser lo más eficiente posible. Por lo
            general, el consumo de memoria es proporcional al número de
            arcos. Internamente, esta eficiencia se implanta a través del
            uso de un arreglo dinámico DynArray.

            No está permitido modificar una entrada de una matrix
            Map_Matrix_Graph. No hay ningún control sobre la matriz y su
            relación con el objeto List_graph asociado. Dicho de otro
            modo, cambios topológicos (inserción y eliminación de nodos y
            arcos) en el objeto GT no se verán reflejados en la
            matriz. Sin embargo, la capacidad de acceso nodos y arcos en
            la representación con listas permite sin ningún problema modificar
            cualquiera de sus atributos.

            Map_Matrix_Graph no está concebido para multigrafos o
            multidigrafos. 

            @param GT el tipo de grafo derivado de una clase basada en
            GT. 
            @see Matrix_Graph Ady_Mat Bit_Mat_Graph DynArray
            @ingroup Grafos
         */
        template <class GT, class SA = Dft_Show_Arc<GT> >
class Map_Matrix_Graph
{
public:
        /// El tipo de Graph_List asociado a la matriz
  typedef GT Graph_Type;

  /// El tipo atributo de nodo que guarda los nodos en el objeto GT
  typedef typename GT::Node_Type Node_Type;

  /// El tipo atributo de arco que guarda los nodos en el objeto GT
  typedef typename GT::Arc_Type Arc_Type;

        /// El tipo de nodo que se maneja el objeto GT
  typedef typename GT::Node Node;

        /// El tipo de arco que se maneja el objeto GT
  typedef typename GT::Arc Arc;

private:

  DynArray<Node*> nodes;
  GT * lgraph;
  mutable size_t num_nodes;
  SA & sa;

  struct Mat_Entry
  {
    Arc * arc;
    Mat_Entry(Arc * __arc = nullptr) : arc(__arc) { /* empty */ }
  };

  DynArray<Mat_Entry> mat;

public:

        /// Copia a la matriz el grafo g representado con listas de adyacencia; 
        /// los valores anteriores de la matriz son borrados.
  inline void copy_list_graph(GT & g);

    /// Constructor a partir de un grafo representado con listas de adyacencia.
  Map_Matrix_Graph(GT & g, SA && __sa = SA())
    : lgraph(&g), num_nodes(g.get_num_nodes()), sa(__sa)
  {
    copy_list_graph(g); 
  }

  Map_Matrix_Graph(GT & g, SA & __sa)
    : lgraph(&g), num_nodes(g.get_num_nodes()), sa(__sa)
  {
    copy_list_graph(g); 
  }

    /// Constructor copia.
  Map_Matrix_Graph(Map_Matrix_Graph & mat)
  {
    copy_list_graph(*(mat.lgraph));
  }

      /// Asignación de matriz de adyacencia
  inline Map_Matrix_Graph & operator = (Map_Matrix_Graph & mat);

        /** Asignación de grafo representado con listas de adyacencia.

            Limpia la matriz (toda la memoria es liberada) y construye una
            nueva matriz de adyacencia basada en el grafo representado con
            listas de adyacencia g.

            @param[in] g el grafo representado con listas a ser asignado.
            @throw bad_alloc si no hay suficiente memoria para la matriz.
         */
  inline Map_Matrix_Graph & operator = (GT & g);

       /** Retorna el puntero a nodo en la representación con listas
           correspondiente al índice i en la matriz.

           El operador (i) con i entero retorna un puntero al nodo de la
           representación con listas del grafo asociado a la matriz del
           índice i.

           @param[in] i índice de la matriz.
           @return puntero al nodo de la representación con listas del grafo
           asociado a la matriz del índice i. 
           @throw out_of_range si i es mayor o igual a la cantidad de nodos.
        */
  inline Node * operator () (const long & i);

        /** Retorna el índice en la matriz del nodo node en la representación 
            con listas.

            El operador (node) con node un puntero a nodo, retorna el índice 
            dentro de la matriz de adyacencia.

            No se hacen verificaciones sobre la correctitud del puntero.

            @param[in] node puntero a nodo en la representación con listas 
            de adyacencia. 
            @return puntero al nodo de la representación con listas del grafo
            asociado a la matriz del índice i. 
            @throw out_of_range si i es mayor o igual a la cantidad de nodos.
        */
  
  inline long operator () (Node * node) const;

        /** Retorna un puntero a un arco con nodo origen src_node y destino
            tgt_node. 

            El operador (src_node,tgt_node) lee la entrada de la matriz
            correspondiente al nodo origen src_node y nodo destino tgt_node
            y retorna un apuntador al arco. Si no existe el arco, entonces
            se retorna nullptr.

            No se realiza verificación sobre la correctitud de los
            apuntadores. 

            Este modo de acceso es logarítmico. Por esa razón, es preferible
            primero indagar los índices de los nodos y luego utilizar el
            mismo operador (i,j) con los índices de los nodos.

            @param[in] src_node puntero al nodo origen.
            @param[in] tgt_node puntero al nodo destino.
            @return puntero al arco si éste existe; nullptr de lo contrario.
         */
  inline Arc * operator () (Node * src_node, Node * tgt_node) const;
  
      /** Retorna un puntero al arco contenido en la entrada (i,j) de 
	  la matriz de adyacencia.

	  El operador (i,j) lee la entrada de la matriz correspondiente
	  y retorna un apuntador al arco. Si no existe el arco, entonces
	  se retorna nullptr.

	  Este modo de acceso es constante.

	  @param[in] i índice del nodo origen.
	  @param[in] j índice del nodo destino.
	  @throw out_of_range si i o j es mayor o igual a la cantidad de nodos 
	  del grafo.
	  @return puntero al arco si éste existe; nullptr de lo contrario.
      */
  inline Arc * operator () (const long & i, const long & j) const;  

      /// Retorna una referencia al grafo representado con listas enlazadas.
  GT & get_list_graph() { return *lgraph; }

        /// Retorna el número de nodos que tiene el grafo (equivalente a 
        /// la dimensión de la matriz.
  const size_t & get_num_nodes() const { return num_nodes; }
};


    template <class GT, class SA> typename GT::Node * 
Map_Matrix_Graph<GT, SA>::operator () (const long & i) 
{
  return get_node <GT> (nodes, i); 
}

      template <class GT, class SA> long 
Map_Matrix_Graph<GT, SA>::operator () (typename GT::Node * node) const
{
  return node_index <GT> (nodes, num_nodes, node);
}

      template <class GT, class SA> typename GT::Arc * 
Map_Matrix_Graph<GT, SA>::operator () (const long & i, const long & j) const
{ 
  Mat_Entry * mat_entry = read_matrix<Mat_Entry>(mat, i, j, num_nodes); 
  if (mat_entry == nullptr) 
    return nullptr;

  return mat_entry->arc;
} 

      template <class GT, class SA> typename GT::Arc * 
Map_Matrix_Graph<GT, SA>::operator () (Node * src_node, Node * tgt_node) const
{ 
  Mat_Entry * mat_entry = 
    read_matrix<Mat_Entry>(mat, node_index<GT>(nodes, num_nodes, src_node), 
			   node_index<GT>(nodes, num_nodes, tgt_node), 
			   num_nodes); 
  if (mat_entry == nullptr)
    return nullptr;

  return mat_entry->arc;
} 

    template <class GT, class SA> Map_Matrix_Graph<GT, SA> & 
Map_Matrix_Graph<GT, SA>::operator = (Map_Matrix_Graph & mat)
{
  if (this == &mat)
    return *this;

  copy_list_graph(*(mat.lgraph));
  return *this;
}

      template <class GT, class SA>
Map_Matrix_Graph<GT, SA> & Map_Matrix_Graph<GT, SA>::operator = (GT & g)
{
  copy_list_graph(g);
}

      template <class GT, class SA>
void Map_Matrix_Graph<GT, SA>::copy_list_graph(GT & g)
{
      // copiar atributos
  lgraph    = &g;
  num_nodes = g.get_num_nodes();
  nodes.cut(); // limpiar arreglos dinámicos de contenidos anteriores
  mat.cut();
  int i = 0; 
  for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne(), ++i)
    nodes[i] = it.get_current_node_ne();

  quicksort(nodes); // ordenar nodes para soportar búsqueda binaria
  for (typename GT::Node_Iterator nit(g); nit.has_curr(); nit.next_ne())
    {
      Node * src = nit.get_current_node_ne();
      const long src_idx = node_index<GT>(nodes, num_nodes, src);

          // para cada arco de src escribirlo en la matriz mat
      for (Node_Arc_Iterator<GT, SA> ait(src, sa); ait.has_curr(); ait.next_ne())
        {
          Arc * arc = ait.get_current_arc_ne();
          Node * tgt = g.get_connected_node(arc, src);
          const long tgt_idx = node_index<GT>(nodes, num_nodes, tgt);
          write_matrix<Mat_Entry>(mat, src_idx, tgt_idx, num_nodes, arc);
        }
    }
}
 
     /** Matriz de adyacencia de un grafo extraída de un grafo
         representado con listas de adyacencia.

	 La clase Matrix_Graph modeliza una matriz de adyacencia que
	 guarda los atributos contenidos en los nodos y arcos de un
	 grafo que previamente fue representado con listas de adyacencia
	 (derivación de la clase GT).

          Un objeto Matrix_Graph es una copia de un grafo representado con
          listas de adyacencia o de un objeto Map_Matrix_Graph (que está
          asociado a una representación con listas de adyacencia).
          A diferencia de Map_Matrix_Graph, Matrix_Graph es modificable en
          el sentido de que permite añadir nuevos arcos en el grafo por
          modificación de una entrada nula de la matriz.

          Cuando se construye un objeto Matrix_Graph, se debe especificar
          un valor constante que representa a la entrada nula; es decir, a
          la ausencia de arco. 

          Es esperado que el consumo de memoria por los arcos sea proporcional 
          a su cantidad.

          @note Puesto que la matriz guarda los atributos asociados a los
          nodos y arcos, Matrix_Graph no sirve para guardar atributos
          derivados hermanos de las clases Graph_Node o Graph_Arc.
    
          @see Map_Matrix_Graph Ady_Mat
          @ingroup Grafos
      */
      template <typename GT, class SA = Dft_Show_Arc<GT> >
class Matrix_Graph
{
public:

        /// El tipo de grafo representado con listas de adyacencia.
  typedef GT Graph_Type;

        /// El tipo de atributo que guardan los nodos del grafo
        /// representado con listas de adyacencia 
  typedef typename GT::Node_Type Node_Type;

        /// El tipo de atributo que guardan los arcos del grafo
        /// representado con listas de adyacencia 
  typedef typename GT::Arc_Type Arc_Type;

        /// El tipo de Graph_Node que usado en el GT.
  typedef typename GT::Node Node;

        /// El tipo de Graph_Arc que usado en el GT.
  typedef typename GT::Arc Arc;

private:

  DynArray<Node_Type> nodes;
  DynArray<Arc_Type>  arcs;
  mutable size_t      n;
  mutable Arc_Type    Null_Value;
  SA &                sa;
  
  void copy(Matrix_Graph & mat)
  {
    if (this == &mat)
      return;

    n          = mat.n; // copiar atributos
    Null_Value = mat.Null_value;
    nodes.cut(); // limpiar memoria de arreglos dinámicos
    arcs.cut();
    arcs.set_default_initial_value(Null_Value);

    // recorrer mat[i,j] y copiarla a nodes[], arcs[] 
    for (int i = 0; i < n; ++i) 
      {
	nodes[i] = mat.nodes[i];
	for (int j = 0; j < n; ++j)
	  {
	    const long mat_index = index_array(i, j, n);
	    if (not arcs.exist(mat_index))
	      continue;
	    arcs.touch(mat_index) = mat.arcs.access(mat_index);
	  }
      }
  }

  void copy(GT & g)
  {
    n = g.get_num_nodes();
    DynArray<typename GT::Node *> ptr; // arreglo temporal 

    int i = 0;
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne(), ++i)
      ptr[i] = it.get_current_node_ne();

    quicksort(ptr); // ordenar por si se relaciona con Map_Matrix_Graph

    arcs.set_default_initial_value(Null_Value); 

    for (i = 0; i < n; ++i) // coloca contenidos de ptr[] en nodes[]
      {
	typename GT::Node * src = ptr[i];
	nodes[i] = src->get_info();
	for (Node_Arc_Iterator<GT, SA> it(src); it.has_curr(); it.next_ne())
	  {
	    typename GT::Arc * arc = it.get_current_arc_ne();
	    typename GT::Node * tgt = it.get_tgt_node();
	    const long j = node_index<GT>(ptr, n, tgt);
	    const long mat_index = index_array(i, j, n);
	    arcs[mat_index] = arc->get_info();
	  }
      }
  }

public:

  /// Retorna el número de nodos del grafo (que es la dimensión de la matriz)
  const size_t & get_num_nodes() const { return n; }

         /// El valor constante que representa la entrada nula en la matriz.
  const Arc_Type & null_value() const { return Null_Value; }

      /** Constructor a partir de un grafo g representado con listas de
          adyacencia. 

          Este constructor recibe un grafo basado en GT g y un valor
          de tipo GT::Arc::Arc_Type correspondiente al valor que será
          considerado como entrada nula. Para cada nodo p del grafo, de tipo
          GT::Node, se copia su atributo interno obtenido mediante
          p->get_info(). De igual manera se hace para los arcos.

          @param[in] g el grafo representado con listas de adyacencia partir
          del cual se copiará la matriz.
          @param[in] null el valor que será considerado como entrada nula de
          la matriz y que indicará ausencia de arco.
	  @paramïn] __sa filtro de arcos.
          @throw bad_alloc si no hay suficiente memoria para construir la
          matriz. 
       */
  Matrix_Graph(GT & g, const Arc_Type & null, SA && __sa = SA()) 
    : Null_Value(null), sa(__sa)
  {
    copy(g);
  }
  
  Matrix_Graph(GT & g, const Arc_Type & null, SA & __sa) 
    : Null_Value(null), sa(__sa)
  {
    copy(g);
  }
  
      /** Constructor copia.

          Construye una matriz a partir de otra matriz.

          @param[in] mat la matriz a ser copiada.
          @throw bad_alloc si no hay suficiente memoria para construir la
          matriz. 
       */
  Matrix_Graph(Matrix_Graph & mat) 
  {
    copy(mat);
  }
  
      /** Asigna a una matriz otra matriz.

          Borra todo el estado de this y luego copia el grafo representado
          con la matriz de adyacencia mat. 

          @param[in] mat la matriz a ser copiada.
          @throw bad_alloc si no hay suficiente memoria para construir la
          matriz. 
          @note Los tipos de grafos GT usados para construir this y
          mat  deben corresponderse exactamente; de lo contrario ocurrirá un
          error de compilación.
       */
  Matrix_Graph & operator = (Matrix_Graph & mat) 
  {
    copy(mat);
    return *this;
  }

      /** Asigna a una matriz un grafo representado con listas de adyacencia.

          Borra todo el estado de this y luego copia el grafo representado
          con listas de adyacencia g. 

          @param[in] g el grafo representado con listas de adyacencia a ser
          copiado.
          @throw bad_alloc si no hay suficiente memoria para construir la
          matriz. 
          @note El tipo GT debe corresponderse exactamente con el tipo
          GT asociado a la matriz; de lo contrario ocurrirá un
          error de compilación.
       */
  Matrix_Graph & operator = (GT & g) 
  {
    copy(g);
    return *this;
  }

  /** Retorna una referencia al atributo de arco contenido en la entrada 
      (i,j) de la matriz de adyacencia.

      El operador (i,j) lee la entrada de la matriz correspondiente y
      retorna el atributo del arco. Si no existe el arco, entonces se
      retorna el valor Matrix_Graph::null_value().

      Este modo de acceso es constante.

      @param[in] i índice del nodo origen.
      @param[in] j índice del nodo destino.
      @throw out_of_range si i o j es mayor o igual a la cantidad de nodos 
      del grafo.
      @return referencia al atributo del arco; recuérdese que
      Matrix_Graph::null_value() indica ausencia de arco.
  */
  const Arc_Type & operator () (const long & i, const long & j) const
  {
    const long mat_index = index_array(i, j, n);
    if (not arcs.exist(mat_index)) // ¿Existe entrada?
      return Null_Value; // No ==> no hay arco

    return arcs.access(mat_index);      
  }

      /** Retorna el atributo asociado al nodo cuyo índice es i.

          El operador (i) con i entero retorna el valor de atributo asociado
          al nodo cuyo índice es i.

          @param[in] i índice de la matriz.
          @return una referencia al atributo del nodo. 
          @throw out_of_range si i es mayor o igual a la cantidad de nodos.
      */
  Node_Type & operator () (const long & i) const
  {
    if (i >= n)
      throw std::out_of_range("node index out of range");

    return const_cast<Node_Type &>(nodes.access(i));
  }

  /** Retorna una referencia al atributo de arco contenido en la 
      entrada (i,j) de la matriz de adyacencia.

      El operador (i,j) lee la entrada de la matriz
      correspondiente y retorna el atributo del arco. Si no existe 
      el arco, entonces se retorna el valor Matrix_Graph::null_value().

      Este modo de acceso es constante.

      @param[in] i índice del nodo origen.
      @param[in] j índice del nodo destino.
      @throw out_of_range si i o j es mayor o igual a la cantidad de nodos 
      del grafo.
      @return referencia al atributo del arco; recuérdese que
      Matrix_Graph::null_value() indica ausencia de arco.
  */
  Arc_Type & operator () (const long & i, const long & j) 
  {
    const long mat_index = index_array(i, j, n);

    return arcs.touch(mat_index);
  }
};

        /** Matriz de adyacencia auxiliar.

            Muchas aplicaciones sobre grafos basadas en matrices de
            adyacencia utilizan información temporal, información de tipo
            distinto al guardado en los atributos del gafo o una parte de los
            atributos. En estas tres situaciones el uso del tipo
            Matriz_Graph o es muy restringido o imposible. 
        
            Para paliar la situación anterior, el tipo Ady_Mat define una
            matriz asociada a un grafo representado con listas de
            adyacencia pero cuyas entradas son de un tipo definido por el
            usuario. 

            Al igual que con otros tipos de matrices de adyacencia,
            Ady_Mat maneja el elemento nulo Ady_Mat::null_value(), pero en
            este caso no necesariamente representa presencia o ausencia de
            arco. Aún así, entradas cuyo valor sea Ady_Mat::null_value()
            tienden a no ocupar memoria. 

            Ady_Mat<GT,__Entry_Type> maneja dos parámetros tipo:
            -# GT: el tipo de grafo representado con listas de adyacencia.
            -# __Entry_Type: el tipo de dato a guardar en la matriz

            @see Matrix_Graph Map_Matrix_Graph
            @ingroup Grafos
         */
  template <class GT, typename __Entry_Type, class SA = Dft_Show_Arc<GT> >
class Ady_Mat
{
public:

        /// El tipo de grafo representado con listas de adyacencia.
  typedef GT Graph_Type;

        /// El tipo de dato que alberga la matriz
  typedef __Entry_Type Entry_Type;

        /// El tipo de dato que guardan los nodos en el grafo representado con
        /// listas de adyacencia. 
  typedef typename GT::Node_Type Node_Type;

        /// El tipo de dato que guardan los arcos en el grafo representado con
        /// listas de adyacencia. 
  typedef typename GT::Arc_Type Arc_Type;

  /// El tipo de nodo usado en la representación con listas de adyacencia. 
  typedef typename GT::Node Node;

  /// El tipo de arco usado en la representación con listas de adyacencia. 
  typedef typename GT::Arc Arc;

private:

  GT *                 lgraph;
  DynArray<Node*>      nodes;
  DynArray<Entry_Type> mat;
  mutable size_t       num_nodes;
  mutable Entry_Type   Null_Value;

  void test_same_graph(Ady_Mat & mat)
  {
    if (lgraph == mat.lgraph) 
      return;

    throw 
      std::domain_error("mat does not refers the same GT than this");
  }

  void copy_nodes(GT & g)
  {
    lgraph     = &g;
    num_nodes  = g.get_num_nodes();
    nodes.cut(); // limpiar memoria por los nodos

          // coloca punteros a nodos en el arreglo nodes[]
    int i = 0;
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne(), ++i)
      nodes[i] = it.get_current_node_ne();

    quicksort(nodes); // ordena para luego hacer búsqueda binaria
  }

  Ady_Mat & copy(Ady_Mat & mat)
  {
    if (this == &mat) 
      return *this;

    test_same_graph(mat);
    Null_Value = mat.Null_Value;
    copy(mat.lgraph);
    
    return *this;
  }

public:

      /// retorna el puntero al nodo dentro de la representación con listas
      /// de adyacencia del índice i en la matriz auxiliar.
  Node * operator () (const long & i) 
  {
    return Aleph::get_node<GT>(nodes, i);
  }

        /// Retorna el índice dentro de la matriz auxiliar de node dentro en
        /// la representación mediante listas de adyacencia.
  long operator () (Node * node) const
  {
    return node_index<GT>(nodes, num_nodes, node);
  }

      /** Retorna una referencia a la entrada (i,j) de la matriz de
	  adyacencia auxiliar. 

	  El operador (i,j) lee la entrada de la matriz auxiliar
	  correspondiente y retorna su entrada.

	  Este modo de acceso es constante.
	  
	  @param[in] i índice del nodo origen.
	  @param[in] j índice del nodo destino.
	  @throw out_of_range si i o j es mayor o igual a la cantidad de nodos 
	  del grafo.
	  @return referencia a la entrada (i,j).
      */
  Entry_Type & operator () (const long & i, const long & j)
  {
    return mat.touch(index_array(i, j, num_nodes));
  }

  /** Retorna una referencia constante a la entrada (i,j) de la matriz de
      adyacencia auxiliar. 

      El operador (i,j) lee la entrada de la matriz auxiliar
      correspondiente y retorna su entrada.

      Este modo de acceso es constante.

      @param[in] i índice del nodo origen.
      @param[in] j índice del nodo destino.
      @throw out_of_range si i o j es mayor o igual a la cantidad de nodos 
      del grafo.
      @return una referencia constante a la entrada (i,j).
  */
  const Entry_Type & operator () (const long & i, const long & j) const
  {
    const long index = index_array(i, j, num_nodes);

    if (mat.exist(index)) 
      return mat.access(index);
        
    return Null_Value;
  }

        /** Retorna una referencia a una entrada de la matriz de
            adyacencia auxiliar dados los punteros a sus nodos origen y
            destino dentro de la representación con listas de adyacencia.  

            El operador (src,tgt) lee la entrada de la matriz auxiliar
            correspondiente a los punteros a los nodos src y tgt en la
            representación con listas de adyacencia y retorna su entrada.

            Este modo de acceso es logarítmico. Por tanto, es recomendable
            utilizar los índices de los nodos.

            @param[in] src puntero al nodo origen en la representación con
            listas de adyacencia.
            @param[in] tgt puntero al nodo destino en la representación con
            listas de adyacencia.
            @return una referencia constante a la entrada correspondiente.
         */
  Entry_Type & operator () (Node * src, Node * tgt) 
  {
    return (*this)(node_index <GT> (nodes, num_nodes, src),
		   node_index <GT> (nodes, num_nodes, tgt));
  }

        /** Retorna una referencia constante a una entrada de la matriz de
            adyacencia auxiliar dados los punteros a sus nodos origen y
            destino dentro de la representación con listas de adyacencia.  

            El operador (src,tgt) lee la entrada de la matriz auxiliar
            correspondiente a los punteros a los nodos src y tgt en la
            representación con listas de adyacencia y retorna su entrada.

            Este modo de acceso es logarítmico. Por tanto, es recomendable
            utilizar los índices de los nodos.

            @param[in] src puntero al nodo origen en la representación con
            listas de adyacencia.
            @param[in] tgt puntero al nodo destino en la representación con
            listas de adyacencia.
            @return una referencia constante a la entrada correspondiente.
         */
  const Entry_Type & operator () (Node * src, Node * tgt) const
  {
    return (*this)(node_index <GT> (nodes, num_nodes, src),
		   node_index <GT> (nodes, num_nodes, tgt));
  }

  /// Retorna una referencia al grafo representado con lista de adyacencia.
  GT & get_list_graph() { return *lgraph; }

        /// Retorna el valor considerado como nulo
  const Entry_Type & null_value() const { return Null_Value; }

        /// Retorna la cantidad de nodos del grafo (o sea, la dimensión de
        /// la matriz).  
  const size_t & get_num_nodes() const { return num_nodes; }

        /** Constructor de matriz auxiliar a partir de un grafo representado
            con listas de adyacencia. 

            Este constructor toma un grafo g representado
            con listas de adyacencia y construye una matriz auxiliar cuyas
            entradas no están inicializadas.

            Si se desea explícitamente ahorrar memoria por un elemento
            designado como valor nulo, entonces éste valor debe indicarse
            con la primitiva  Ady_Mat::set_null_value(). 

            @param[in] g el grafo representado con listas de adyacencia.
            @throw bad_alloc si no hay suficiente memoria.
            @see Ady_Mat::set_null_value() Ady_Mat::null_value()
        */
  Ady_Mat(GT & g) : lgraph(&g), num_nodes(lgraph->get_num_nodes()) 
  {
    copy_nodes(g); 
  }

  /** Constructor de matriz auxiliar a partir de un grafo representado
      con listas de adyacencia y con definición de valor nulo.

      Este constructor toma un grafo g representado con listas de
      adyacencia, un valor nulo y construye una matriz auxiliar cuyas
      entradas no están inicializadas.

      Un acceso a la entrada (i,j) retornará el valor Ady_Mat::null_value().

      @param[in] g el grafo representado con listas de adyacencia.
      @param[in]  null el valor del elemento designado como nulo.
      @throw bad_alloc si no hay suficiente memoria.
      @see Ady_Mat::set_null_value() Ady_Mat::null_value()
  */
  Ady_Mat(GT & g, const Entry_Type & null) 
    : lgraph(&g), num_nodes(lgraph->get_num_nodes()), Null_Value(null) 
  {
    copy_nodes(*lgraph); 
  }
  
       /// Declara un valor nulo para la matriz de adyacencia auxiliar.
  void set_null_value(const Entry_Type & null) { Null_Value = null; }
  
    /// Constructor copia.
  Ady_Mat(Ady_Mat & mat) 
  {
    copy(mat);
  }

        /** Efectúa una operación sobre la matriz auxiliar según cada arco
            del grafo representado con listas de adyacencia.

            operate_all_arcs_list_graph() recorre los arcos del grafo
            representado con listas de adyacencia e invoca a la operación
            Operation()(mat, arc, i, j, entry) donde:
            -# mat: es una referencia a la matrix sobre el
                    cual se está realizando la operación.
            -# arc: es un apuntador al arco dentro del GT asociado a 
            la matriz. 
            -# i: es el índice del nodo origen en la matriz.
            -# j: es el índice del nodo destino en la matriz.
            -# entry: es una referencia al contenido de la entrada
                    (i,j) en la matriz.

            El uso de este método es principalmente para escribir los
            valores iniciales dentro de la matriz según los contenidos de
            los arcos en el grafo representado con listas de adyacencia.

            Nótese que la operación sólo se ejecuta para las entradas que
            refieren a un arco. El resto de las entradas contiene el valor
            Ady_Mat::null_value() y probablemente no ocupen memoria.
         */
  template <class Operation> void operate_all_arcs_list_graph();

        /** Efectúa una operación sobre la matriz auxiliar según cada arco
            del grafo representado con listas de adyacencia y le transmite a
            la operación un puntero opaco por donde enviar y recibir
            información. 

            operate_all_arcs_list_graph() recorre los arcos del grafo
            representado con listas de adyacencia e invoca a la operación
            Operation()(mat, arc, i, j, entry, ptr) donde:
            -# mat: es una referencia a la matrix sobre el cual se está 
               realizando la operación. 
            -# arc: es un apuntador al arco dentro del GT asociado a 
               la matriz. 
            -# i: es el índice del nodo origen en la matriz.
            -# j: es el índice del nodo destino en la matriz.
            -# entry: es una referencia al contenido de la entrada
                    (i,j) en la matriz.
            -# ptr: puntero opaco.

            El uso de este método es principalmente para escribir los
            valores iniciales dentro de la matriz según los contenidos de
            los arcos en el grafo representado con listas de adyacencia.

            Nótese que la operación sólo se ejecuta para las entradas que
            refieren a un arco. El resto de las entradas contiene el valor
            Ady_Mat::null_value() y probablemente no ocupen memoria.

             @param[in] ptr puntero a información opaca que se desea
             transmitir a la operación
         */
  template <class Operation> void operate_all_arcs_list_graph(void * ptr);
  
      /** Efectúa una operación sobre todas las entradas de matriz auxiliar.

          operate_all_arcs_matrix() recorre todas las entradas de matriz
          auxiliar e invoca una operación Operation()(mat,src,tgt,s,t,entry)
          cuyos parámetros se definen así:
          -# mat: referencia a la matriz
          -# src: puntero al nodo origen.
          -# tgt: puntero al nodo destino.
          -# s: índice del nodo origen.
          -# t: índice del nodo destino.
          -# entry: referencia a la entrada de la matriz.

          Si se invoca este método, entonces la matriz ocupa el máximo de
          memoria, pues cada entrada debe ser escrita de modo tal que se
          garantice una referencia válida a mat(s,t).

          Nótese que no hay acceso directo al arco en la representación con
          listas de adyacencia.
       */
  template <class Operation> void operate_all_arcs_matrix();

      /** Efectúa una operación sobre todas las entradas de matriz auxiliar
          con un puntero opaco por donde enviar o recibir información.

          operate_all_arcs_matrix() recorre todas las entradas de matriz
          auxiliar e invoca una operación Operation()(mat,src,tgt,s,t,entry)
          cuyos parámetros se definen así:
          -# mat: referencia a la matriz
          -# src: puntero al nodo origen.
          -# tgt: puntero al nodo destino.
          -# s: índice del nodo origen.
          -# t: índice del nodo destino.
          -# entry: referencia a la entrada de la matriz.
          -# ptr: puntero opaco a pasar a la operación.

          Si se invoca este método, entonces la matriz ocupa el máximo de
          memoria, pues cada entrada debe ser escrita de modo tal que se
          garantice una referencia válida a mat(s,t).

          Nótese que no hay acceso directo al arco en la representación con
          listas de adyacencia.

          @param[in] ptr puntero a información opaca que se desea
          transmitir a la operación
       */
      template <class Operation> 
  void operate_all_arcs_matrix(void * ptr); 
}; 


      template <class GT, typename __Entry_Type, class SA>
      template <class Operation>
void Ady_Mat<GT, __Entry_Type, SA>::operate_all_arcs_list_graph()
{      // recorrer todos los nodos del grafo
  for (typename GT::Node_Iterator nit(*lgraph); nit.has_curr(); nit.next_ne())
    {
      Node * src = nit.get_current_node_ne();
      const long src_idx = node_index<Graph_Type>(nodes, num_nodes, src);

            // recorrer todos los arcos del nodo origen
      for (Node_Arc_Iterator<GT, SA> at(src); at.has_curr(); at.next_ne())
	{
	  Arc * arc = at.get_current_arc_ne();
	  const long tgt_idx = 
	    node_index<Graph_Type>(nodes, num_nodes, arc->get_tgt_node());
	  Entry_Type & entry = 
	    mat.touch(index_array(src_idx, tgt_idx, num_nodes));

	  Operation () (*this, arc, src_idx, tgt_idx, entry);
	}
    }
}

      template <class GT, typename __Entry_Type, class SA>
      template <class Operation>
void Ady_Mat<GT, __Entry_Type, SA>::operate_all_arcs_matrix()
{
  const long & n = num_nodes;
  for (int s = 0; s < n; ++s)
    {
      Node * src_node = get_node<GT>(nodes, s);
      for (int t = 0; t < n; ++t)
	{
	  Node * tgt_node = get_node<GT>(nodes, t);
	  Entry_Type & entry = mat.touch(index_array(s, t, num_nodes));

	  Operation () (*this, src_node, tgt_node, s, t, entry);
	}
    }
}

      template <class GT, typename __Entry_Type, class SA>
      template <class Operation>
void Ady_Mat<GT, __Entry_Type, SA>::operate_all_arcs_list_graph(void * ptr)
{      // recorrer todos los nodos del grafo
  for (typename GT::Node_Iterator nit(*lgraph); nit.has_curr(); nit.next_ne())
    {
      Node * src = nit.get_current_node_ne();
      const long src_idx = node_index<Graph_Type>(nodes, num_nodes, src);

            // recorrer todos los arcos del nodo origen
      for (Node_Arc_Iterator<GT, SA> at(src); at.has_curr(); at.next_ne())
	{
	  Arc * arc = at.get_current_arc_ne();
	  Node * tgt = lgraph->get_tgt_node(arc);
	  const long tgt_idx = node_index<Graph_Type>(nodes, num_nodes, tgt);
	  Entry_Type & entry = // asegurar acceso a mat(src_idx, tgt_idx)
	    mat.touch(index_array(src_idx, tgt_idx, num_nodes));

	  Operation () (*this, arc, src_idx, tgt_idx, entry, ptr);
	}
    }
}

      template <class GT, typename __Entry_Type, class SA>
      template <class Operation>
void Ady_Mat<GT, __Entry_Type, SA>::operate_all_arcs_matrix(void * ptr)
{
  const long & n = num_nodes;

  for (int s = 0; s < n; ++s)
    {
      Node * src_node = get_node<GT>(nodes, s);

      for (int t = 0; t < n; ++t)
	{
	  Node * tgt_node = get_node<GT>(nodes, t);
	  Entry_Type & entry = mat.touch(index_array(s, t, num_nodes));
	  Operation () (*this, src_node, tgt_node, s, t, entry, ptr);
	}
    }
}

        /** Matriz de bit de adyacencia de un grafo.

            El tipo Bit_Mat_Graph define la más simple matriz de
            adyacencia sobre un grafo. Al igual que los tipos de matrices
            de adyacencia de Aleph, Bit_Mat_Graph puede asociar un
            grafo representado con listas de adyacencia basado sobre
            GT, pero a diferencia de las matrices anteriores, esta
            asociación no es obligatoria.

            Se trata de una matriz de bits implementada mediante el tipo
            BitArray. Una entrada (i,j) con valor 1 indica presencia del
            arco, mientras que el valor 0 indica ausencia.

            Este tipo de matriz ocupa toda la memoria posible. Es decir,
            la dimensión de la matriz dividida entre 8.

            @see BitArray warshall_compute_transitive_clausure()
            @ingroup Grafos
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
