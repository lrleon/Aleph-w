/* 
  This file is part of Aleph-w library

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018

  Leandro Rabindranath Leon / Alejandro Mujica

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <https://www.gnu.org/licenses/>.
*/
# ifndef TPL_SGRAPH_H
# define TPL_SGRAPH_H

# include <memory>
# include <htlist.H>
# include <tpl_graph.H>
# include <tpl_dynSetTree.H>

using namespace Aleph;

namespace Aleph {


    /** Nodo de grafo implantado con listas simples de adyacencia.

        Esta clase define un nodo o vértice de un grafo o digrafo
        implantado mediante listas de adyacencia.

        Básicamente, hay tres maneras de definir la información que
        pertenece a un nodo de un grafo:
        - Mediante el parámetro tipo inherente al nodo denominado \c
        Node_Info. 
        - A través de herencia de esta clase base. Esta es la manera de
        mantener diferentes tipos de nodo en un grafo.
        - Combinación de las dos anteriores.

        @param Node_Info Tipo de dato guardado en el grafo y accesible
        mediante el método \c get_info().

        @note Esta clase sólo está destinada a la definición de nodos de
        un grafo. No está destinada a ser usada por el usuario. Es
        preferible usarla a través de los métodos provistos por \c
        List_Graph. Si bien es posible usar algunos métodos o acceder
        algunos de sus atributos, es altamente recomendable no
        usarla. En su lugar, se debe utilizar el tipo \c
        List_Graph::Node y sus métodos asociados.

        Un nodo de grafo tiene tres atributos adicionales presentes por
        cada nodo y denominados del siguiente modo:
        - Bits de control (\c Bit_Fields): conjunto de bits usados por
        los algoritmos para marcar el nodo.
        - Contador (\c counter): contador generalmente usado para marcar
        un orden de visita o procesamiento.
        - \c cookie: puntero opaco hacia cualquier otra información
        temporal que se desee asociar al nodo.

        @ingroup Grafos
        @see List_Graph List_Digraph Graph_Arc Bit_Fields
     */
    template <typename Node_Info = Empty_Class> 
struct Graph_Snode : public GTNodeCommon<Node_Info>
{
  using Base = GTNodeCommon<Node_Info>;
  friend class GTNodeCommon<Node_Info>;

  DynList<void*> arc_list; // lista de adyacencia

    /** Constructor que asigna un valor de atributo.

        Crea un nodo y le asigna el valor \c info como valor de la
        información contenida en el nodo.

        Los valores de los bits de control y del contador son colocados en
        cero. El \c cookie es colocado en \c nullptr.

        @param[in] info el valor de información que se desea asignar al
        nodo recién creado.

        @note Debe estar definido el constructor copia de la clase Node_Info.
    */
  Graph_Snode(const Node_Info & info) 
  noexcept(std::is_nothrow_copy_constructible<Node_Info>::value) : Base(info)
  { 
    /* empty */ 
  }

  Graph_Snode(Node_Info && info = Node_Info()) 
  noexcept(std::is_nothrow_move_constructible<Node_Info>::value) 
  : Base(move(info))
  { 
    /* empty */ 
  }

  Graph_Snode(const Graph_Snode & node) 
  noexcept(std::is_nothrow_copy_constructible<Node_Info>::value)
    : Graph_Snode(node.node_info) 
  {
    /* empty */ 
  }

  Graph_Snode & operator = (const Graph_Snode & node)
  noexcept(std::is_nothrow_copy_assignable<Node_Info>::value)
  {
    if (&node == this)
      return *this;
    this->node_info = node.node_info;
    return *this;
  }

    /** Constructor copia a partir de un puntero a nodo.

        Crea un nodo y le asigna el valor \c node->get_info() como valor de la
        información contenida en el nodo.

        Los valores de los bits de control y del contador son colocados en
        cero. El \c cookie es colocado en \c nullptr.

        @param[in] node puntero al nodo desde el cual se desea copiar el
        valor de información que se desea asignar al nodo recién creado.

        @note Debe estar definido el constructor copia de la clase
        Node_Info.
        @note La instancia creada es un nodo distinto de \c node.
    */
  Graph_Snode(Graph_Snode * node) 
  noexcept(std::is_nothrow_copy_assignable<Node_Info>::value)
    : Base(node->get_info())
  { 
    /* empty */ 
  }
};

    /** Arco de grafo implantado con listas simples de adyacencia.

        Esta clase define un arco o eje de un grafo o digrafo
        implantado mediante listas de adyacencia simplemente enlzadas.

        Básicamente, hay tres maneras de definir la información que
        pertenece a un arco de un grafo:
        - Mediante el parámetro tipo inherente al nodo denominado \c
        Arc_Info. 
        - A través de herencia de esta clase base. Esta es la manera de
        mantener diferentes tipos de arcos en un grafo.
        - Combinación de las dos anteriores.

        @param Arc_Info Tipo de dato guardado en el grafo y accesible
        mediante el método \c get_info().

        @note Esta clase sólo está destinada a la definición de arcos de
        un grafo. No está destinada a ser usada por el usuario. Es
        preferible usarla a través de los métodos provistos por \c
        List_Graph. Si bien es posible usar algunos métodos o acceder
        algunos de sus atributos, es altamente recomendable no
        usarla. En su lugar, se debe utilizar el tipo \c
        List_Graph::Arc y sus métodos asociados.

        Un arco de grafo tiene tres atributos adicionales presentes por
        cada arco y denominados del siguiente modo:
        - Bits de control (\c Bit_Fields): conjunto de bits usados por
        los algoritmos para marcar el arco.
        - Contador (\c counter): contador generalmente usado para marcar
        un orden de visita o procesamiento.
        - \c cookie: puntero opaco hacia cualquier otra información
        temporal que se desee asociar al arco.

        @ingroup Grafos
        @see List_Graph List_Digraph Graph_Node Bit_Fields
     */
    template <typename Arc_Info = Empty_Class> 
class Graph_Sarc : public GTArcCommon<Arc_Info>
{
  using Base = GTArcCommon<Arc_Info>;

public:

  Graph_Sarc(const Arc_Info & info) 
  noexcept(std::is_nothrow_copy_constructible<Arc_Info>::value)
    : Base(info)
  { 
    /* empty */ 
  }

  Graph_Sarc(Arc_Info && info = Arc_Info()) 
  noexcept(std::is_nothrow_move_constructible<Arc_Info>::value)
  : Base(move(info))
  { 
    /* empty */ 
  }

  Graph_Sarc(const Graph_Sarc & arc) 
  noexcept(std::is_nothrow_copy_constructible<Arc_Info>::value)
    : Graph_Sarc(arc.arc_info) 
  {
    /* empty */ 
  }

  Graph_Sarc & operator = (const Graph_Sarc & arc)
  noexcept(std::is_nothrow_copy_assignable<Arc_Info>::value)
  {
    if (&arc == this)
      return *this;
    this->arc_info = arc.arc_info;
    return *this;
  }

  Graph_Sarc(void * src, void * tgt, const Arc_Info & data) 
    noexcept(std::is_nothrow_copy_constructible<Arc_Info>::value)
  : Base(src, tgt, data)
  { 
    // empty
  }

  Graph_Sarc(void * src, void * tgt, Arc_Info && data) 
    noexcept(std::is_nothrow_move_constructible<Arc_Info>::value)
  : Base(src, tgt, move(data))
  { 
    // empty
  }
};


    /** Clase grafo implementado con listas de adyacencia.

        List_SGraph<Node, Arc> es una clase que modeliza grafos
        representados mediante listas de adyacencia.

        La clase maneja dos parámetros tipo fundamentales:
        - __Graph_Node: el tipo de los nodos que debe estar definido
          mediante la clase Graph_Snode.
        - __Graph_Arc: el tipo de los nodos que debe estar definido
          mediante la clase Graph_Sarc.

        Estas clases deben haberse definido previamente.

        Una vez instanciado un List_SGraph<Node, Arc>, los nodos y
        arcos deben accederse mediante los tipos internos:
        - List_SGraph<Node, Arc>::Node
        - List_SGraph<Node, Arc>::Arc
       
        @param __Graph_Node El tipo de nodo. Debe estar definido a
        partir de la clase \c __Graph_Node, bien sea por inclusión de
        atributos, por derivación o por combinación de ambos
        @param __Graph_Arc El tipo de arco. Debe estar definido a
        partir de la clase \c __Graph_Arc, bien sea por inclusión de
        atributos, por derivación o por combinación de ambos
        @see Graph_SNode Graph_SArc
        @ingroup Grafos
        @see List_SDigraph Path 
     */
  template <typename __Graph_Node = Graph_Snode<unsigned long>, 
	    typename __Graph_Arc  = Graph_Sarc<unsigned long>>
class List_SGraph 
    : public GraphCommon<List_SGraph<__Graph_Node, __Graph_Arc>,
			 __Graph_Node, __Graph_Arc>
{
public:

  using Node = __Graph_Node;
  using Arc = __Graph_Arc;
  using Node_Type = typename Node::Node_Type;
  using Arc_Type = typename Arc::Arc_Type;

  friend class GraphCommon<List_SGraph<__Graph_Node, __Graph_Arc>,
			   __Graph_Node, __Graph_Arc>;

  using CommonBase = GraphCommon<List_SGraph<__Graph_Node, __Graph_Arc>,
				 __Graph_Node, __Graph_Arc>;

  // using FuncBase::search_arc; // TODO: delete after full dry
  using CommonBase::insert_node;
  using CommonBase::insert_arc;

private:

  using DynSetNode = DynSetTree<Node*, Rand_Tree>;
  using DynSetArc = DynSetTree<Arc*, Rand_Tree>;

  DynSetNode node_list; // lista de nodos
  DynSetArc  arc_list;  // lista de arcos

public:

  List_SGraph() noexcept { /* empty */ }

  virtual ~List_SGraph()
  {
    clear();
  }

  List_SGraph(const List_SGraph & g)
  {
    copy_graph(*this, g);
  }

  void swap(List_SGraph & g) noexcept
  {
    this->common_swap(g);
    node_list.swap(g.node_list);
    arc_list.swap(g.arc_list);
  }

  List_SGraph(List_SGraph && g) noexcept
  {
    swap(g);
  }

  List_SGraph & operator = (const List_SGraph & g)
  {
    if (&g == this)
      return *this;

    clear();

    copy_graph(*this, const_cast<List_SGraph&>(g));

    return *this;
  }

  List_SGraph & operator = (List_SGraph && g) noexcept
  {
    swap(g);

    return *this;
  }

      /** Iterador de nodos de un grafo.

          Esta clase itera sobre todos los nodos de un grafo.

          @ingroup Grafos
       */
  class Node_Iterator : public DynSetNode::Iterator
  {
  public:

       /// Tipo de elemento que retorna get_current()
    using Item_Type = Node *;

       /// El tipo de conjunto sobre el cual se itera.
    using Set_Type = List_SGraph;

    Node_Iterator() noexcept { /* empty */ }

    Node_Iterator(const List_SGraph & g) noexcept
      : DynSetNode::Iterator(g.node_list)
    {
      // empty
    }

        /// retorna el nodo actual.
    Node * get_current_node() const
    {
      return this->get_curr();
    }
  };

      /** Iterador de  arcos de un nodo de grafo.

          Esta clase itera sobre todos los arcos adyacentes de un nodo
          perteneciente a un grafo o digrafo.

          @ingroup Grafos
       */
  class Node_Arc_Iterator : public DynList<void*>::Iterator
  {
    Node * src_node; 
    
  public:

        /// El tipo de dato que retorna get_current().
    using Item_Type =  Arc *;

        /// El tipo de conjunto sobre el cual se itera.
    using Set_Type = Node *;

       /// Instancia un iterador vacío (inválido).
    Node_Arc_Iterator() noexcept { /* empty */ }

        /// Instancia un iterador sobre el nodo _src_node
    Node_Arc_Iterator(Node * src) noexcept 
      : DynList<void*>::Iterator(src->arc_list), src_node(src)
    {
      // empty 
    }

        /// Retorna el arco actual.
    Arc * get_current_arc() const
    {
      return (Arc*) this->DynList<void*>::Iterator::get_curr();
    }

        ///\overload get_current_arc()
    Arc * get_current() const { return get_current_arc(); }

        ///\overload get_current_arc()
    Arc * get_curr() const { return get_current_arc(); }

        /// Retorna el nodo destino del arco actual.
    Node * get_tgt_node() const
    {
      Arc * a = get_curr();
      return (Node*) a->get_connected_node(src_node);
    }
  };

      /** Iterador sobre arcos de un grafo.

          Este iterador recorre cada arco del grafo. El orden de visita
          puede ser alterado si previamente se ordenan los arcos.

          @see sort_arcs()
       */
  class Arc_Iterator : public DynSetArc::Iterator
  {
  public:

        /// Tipo de elemento que retorna get_current()
    using Item_Type = Arc *;

       /// El tipo de conjunto sobre el cual se itera.
    using Set_Type = List_SGraph;

    Arc_Iterator() noexcept { /* empty */ }

    Arc_Iterator(const List_SGraph & _g) noexcept
      : DynSetArc::Iterator(_g.arc_list)
    {
      // empty
    }

        /// Retorna un puntero al arco actual
    Arc * get_current_arc() const 
    {
      return this->get_curr();
    }

        /// Retorna el nodo origen del arco actual (sólo cuenta si es digrafo) 
    Node * get_src_node() const { return (Node*) get_current_arc()->src_node; }

        /// Retorna el nodo destino del arco actual (sólo cuenta si es digrafo) 
    Node * get_tgt_node() const { return (Node*) get_current_arc()->tgt_node; }
  };


      /** Inserción de un nodo cuya memoria ya ha sido apartada.

          Este método asume un nodo de tipo \c List_Graph::Node apuntado
          por el parámetro \c node y lo inserta en el grafo.

          @param[in] node puntero a un nodo ya creado que no pertenece a
          ningún grafo. 

          @return Puntero al nodo insertado.

          @note Puesto que cuando se elimine el nodo o se destruya el
          grafo se invocará al operador \c delete, \c node debe haber sido
          \b imperativamente apartado con \c new.
          @note   Por lo general, esta rutina no debe usarse. En su lugar debe
          utilizarse la otra inserción (que aparta memoria automáticamente.
       */
  virtual Node * insert_node(Node * p)
  {
    assert(p->arc_list.is_empty());

    this->num_nodes++;
    node_list.append(p);
    return p;
  }

private:

  Arc * insert_arc(Node * src, Node * tgt, void * a)
  {
    Arc * arc = (Arc*) a;

    arc->src_node = src;
    arc->tgt_node = tgt;

    src->arc_list.append(a);
    src->num_arcs++;
    if (not this->digraph and src != tgt)
      {
	tgt->arc_list.append(a);
	tgt->num_arcs++;
      }

    arc_list.append(arc);
    this->num_arcs++;

    return arc;
  }

  void disconnect_arc(Arc * arc)
  {
    Node * src = (Node*) arc->src_node;
    Node * tgt = (Node*) arc->tgt_node;

    src->arc_list.remove([arc] (auto a) { return a == arc; });
    src->num_arcs--;

    if (not this->digraph and src != tgt)
      {
	tgt->arc_list.remove([arc] (auto a) { return a == arc; });
	tgt->num_arcs--;
      }
  }

public:

      /** Elimina el arco arc.

          La operación elimina del grafo el arco \c arc y luego libera su
          memoria. 

          El arco debe pertenecer al grafo y no se realiza ninguna
          verificación al respecto.

          @param[in]  arc puntero al arco a eliminar.
       */
  virtual void remove_arc(Arc * arc) noexcept
  {
    disconnect_arc(arc);
    arc_list.remove(arc);
    this->num_arcs--;
    delete arc;
  }

  virtual void remove_node(Node * p) noexcept
  {
    DynList<Arc*> arcs_to_remove;
    arc_list.for_each([this, &arcs_to_remove, p] (auto arc)
      {
        if (this->get_src_node(arc) == p or this->get_tgt_node(arc) == p)
          {
            this->disconnect_arc(arc);
            this->num_arcs--;
    	    arcs_to_remove.append(arc);
    	  }
      });

    arcs_to_remove.for_each([this] (auto a) 
    	      {
    		arc_list.remove(a);
		delete a; 
    	      });
    node_list.remove(p);
    this->num_nodes--;
    
    delete p;
  }

  Node * get_first_node() const
  {
    return (Node*) node_list.get_first();
  }

  Arc * get_first_arc() const
  {
    return (Arc*) arc_list.get_first();
  }

  Arc * get_first_arc(Node * p) const
  {
    return (Arc*) p->arc_list.get_first();
  }

private:

  void clear() noexcept
  {
    arc_list.for_each( [] (Arc * p) { delete p; } );
    node_list.for_each( [] (Node * p) { delete p; } );
  }

      template <class Cmp>
  struct Cmp_Arc
  {
    Cmp & cmp;

    Cmp_Arc(Cmp && __cmp = Cmp()) 
    noexcept(std::is_nothrow_constructible<Cmp>::value and
	     std::is_nothrow_move_assignable<Cmp>::value)
      : cmp(__cmp) { /* empty */ }

    Cmp_Arc(Cmp & __cmp) 
    noexcept(std::is_nothrow_constructible<Cmp>::value)
      : cmp(__cmp) { /* empty */ }

    bool operator () (Slinknc * s1, Slinknc * s2) const noexcept
    {
      Arc * arc1 = (Arc*) s1;
      Arc * arc2 = (Arc*) s2;
      return cmp(arc1, arc2);
    }
  };

public:

      template <class Compare> 
  void sort_arcs(Compare & cmp) noexcept
  {
    Cmp_Arc<Compare> comp(cmp);
    mergesort<HTList, Cmp_Arc<Compare>>(arc_list, comp);
  }

        template <class Compare> 
  void sort_arcs(Compare && cmp = Compare()) noexcept
  {
    mergesort<Cmp_Arc<Compare>>(arc_list, Cmp_Arc<Compare>(cmp)); 
  }
};

    /** Clase digrafo (grafo dirigido) implementado con listas simples
	de adyacencia.  

        Esta clase modeliza un grafo dirigido. Funcionalmente es
        equivalente a la clase \c List_SGraph, a la excepción de que ésta
        maneja grafos dirigidos.
       
        @param __Graph_Snode El tipo de nodo. Debe estar definido a
        partir de la clase \c __Graph_Snode, bien sea por inclusión de
        atributos, por derivación o por combinación de ambos
        @param __Graph_Arc El tipo de arco. Debe estar definido a
        partir de la clase \c __Graph_Arc, bien sea por inclusión de
        atributos, por derivación o por combinación de ambos
        @see Graph_Snode Graph_Sarc
        @ingroup Grafos
        @see List_SGraph
     */
  template <typename __Graph_Node = Graph_Snode<int>,
	    typename __Graph_Arc  = Graph_Sarc<int>>
class List_SDigraph : public List_SGraph<__Graph_Node, __Graph_Arc>
{
  using GT = List_SGraph<__Graph_Node, __Graph_Arc>;

public:

  using Node = __Graph_Node;

  using Arc = __Graph_Arc;

  List_SDigraph() noexcept
  {
    this->digraph = true; 
  }
  
  List_SDigraph(const List_SDigraph & dg) : GT()
  {
    this->digraph = true; 
    copy_graph(*this, dg, false);
  }

  List_SDigraph & operator = (const List_SDigraph<Node, Arc> & g)
  {
    if (this == &g) 
      return *this;

    this->digraph = true;
    copy_graph(*this, g, false);

    return *this;
  }

  List_SDigraph(List_SDigraph && dg) noexcept
    : GT()
  {
    this->digraph = true;
    this->swap(dg);
  }

  List_SDigraph & operator = (List_SDigraph<Node, Arc> && g) noexcept
  {
    this->digraph = true; 
    this->swap(g);

    return *this;
  }
};


} // end namespace Aleph

# endif /* TPL_SGRAPH_H */
