
/* Aleph-w

   / \  | | ___ _ __ | |__      __      __
   / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
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
# ifndef TPL_NET_H
# define TPL_NET_H

# include <limits>
# include <set>

# include <tpl_dynDlist.H>
# include <tpl_dynListStack.H>
# include <tpl_dynBinHeap.H>
# include <tpl_dynSetTree.H>
# include <tpl_dynSetHash.H>
# include <tpl_random_queue.H>
# include <tpl_graph_utils.H>
# include <tpl_find_path.H>
# include <graph-traverse.H>


  
using namespace Aleph;

namespace Aleph {

  template <typename Arc_Info, typename Flow_Type>
  struct Net_Arc_Info : public Arc_Info
  {
    /// valor de capacidad
    Flow_Type cap; 

    /// valor de flujo
    Flow_Type flow; 

    Net_Arc_Info() {}

    Net_Arc_Info(const Arc_Info & info, Flow_Type __cap, Flow_Type __flow)
      : Arc_Info(info), cap(__cap), flow(__flow)
    {
      // empty
    }
  };

  /** Arco de una red de flujo implementada mediante listas de adyacencia. 
        
      Net_Arc modeliza un arco de red capacitada con información de
      tipo Arc_Info y tipo numérico F_Type para representar capacidad
      y flujo. 

      La clase Net_Arc sólo se destina a modelizar la red. Ella no
      debe instanciarse. Úsese insert_arc() de la clase Net_Graph
      para crear arcos de una red.

      @see Array_graph Net_Graph Net_Node 
      @ingroup Redes
  */
  template <typename Arc_Info, typename F_Type = double>
  struct Net_Arc : public Graph_Aarc<Arc_Info> // Tipo que representa el flujo 
  {
    using Base = Graph_Aarc<Arc_Info>;

    using Flow_Type = F_Type;

    /// valor de capacidad
    Flow_Type cap = 0; 

    /// valor de flujo
    Flow_Type flow = 0; 

    /// Retorna true si los valores de capacidad y flujo del arco
    /// satisfacen las condiciones de flujo (flujo menor o igualo que la
    /// capacidad). 
    bool check_arc() const noexcept { return flow >= 0 and flow <= cap; }

    Net_Arc(const Arc_Info & info) 
      : Base(info) { /* empty */ } 

    Net_Arc(const Net_Arc & arc) 
      : Base(arc.arc_info), cap(arc.cap), flow(arc.flow)
    {
      // empty
    }

    Net_Arc() { /* empty */ }

    Net_Arc & operator = (const Net_Arc & arc)
    {
      if (this == &arc) 
        return *this;

      *(Base*) this = arc;
      cap           = arc.cap;
      flow          = arc.flow;

      return *this;
    }
  };


  template <class Net>
  bool is_residual(typename Net::Node * src, typename Net::Arc * a) noexcept
  {
    assert(a->src_node == src or a->tgt_node == src);
    return a->tgt_node == src;
  }

  /** Filtro de arcos para redes de flujo.

      @ingroup Redes
  */
  template <class Net>
  struct Net_Filt
  {
    typename Net::Node * p = nullptr;

    void * cookie = nullptr;

    void set_cookie(void * __cookie) noexcept { cookie = __cookie; }

    Net_Filt(typename Net::Node * s = nullptr) noexcept
      : p(s) { /* empty */ }

    bool operator () (typename Net::Arc * a) const noexcept
    {
      assert(p);
      assert(a->src_node == p or a->tgt_node == p);
      auto src = (typename Net::Node*) a->src_node;
      if (src == p)
        return a->cap - a->flow > 0; // normal arc
    
      assert(is_residual<Net>(p, a));
      return a->flow > 0; // residual arc
    }

    bool operator () (const Net &, typename Net::Arc * arc) noexcept
    {
      return (*this)(arc);
    }

    typename Net::Node* get_node(typename Net::Arc * a) const noexcept
    {
      assert(p);
      assert(a->src_node == p or a->tgt_node == p);
      return (typename Net::Node*) (a->src_node == p ? a->tgt_node : a->src_node);
    }
  };


  template <class Net>
  using __Net_Iterator = Digraph_Iterator<Net, Net_Filt<Net>>;

  template <class Net, class Show_Arc = Dft_Show_Arc<Net>>
  using Net_Iterator = 
    Filter_Iterator<typename Net::Node*, __Net_Iterator<Net>, Show_Arc>;


  /** Retorna el flujo restante según que a respecto al nodo p sea o no un
      arco residual.

      @ingroup Redes
  */
  template <class Net>
  typename Net::Flow_Type
  remaining_flow(typename Net::Node * src, typename Net::Arc * a) noexcept
  {
    return is_residual<Net>(src, a) ? a->flow : a->cap - a->flow;
  }

  template <typename Node_Info = Empty_Class>
  using Net_Node = Graph_Anode<Node_Info>;


  /** Red de flujo implementada mediante listas de adyacencia.

      El tipo Net_Graph modeliza una red capacitada, principal
      instrumento del cálculo de flujo máximo y vehículo de una
      extensa e importante familia de algoritmos sobre grafos. 

      La clase recibe dos parámetros tipo:
      -# NodeT: el tipo de nodo de la red, el cual debe ser
      descendiente de la clase Net_Node.
      -# ArcT: el tipo de arco de la red, el cual debe ser
      descendiente de la clase Net_Arc. 

      @ingroup Redes
  */
  template <class NodeT = Net_Node<Empty_Class>, 
            class ArcT  = Net_Arc<Empty_Class, double>>
  struct Net_Graph : public Array_Graph<NodeT, ArcT>
  {
    using Net = Net_Graph<NodeT, ArcT>;

    using Base = Array_Graph<NodeT, ArcT>;

    using Base::Base;

    using Graph = Base;
  
    /// Tipo de arco.
    using Arc = ArcT;

    /// Tipo de node.
    using Node = NodeT;

    /// Tipo que representa la capacidad y el flujo
    using Flow_Type = typename Arc::Flow_Type;

    /// Tipo de atributo que almacena un nodo
    using Node_Type = typename Node::Node_Type;

    /// Tipo de atributo que almacena un arco
    using Arc_Type = typename Arc::Arc_Type;

    mutable Flow_Type Infinity;

    /// Arcos que salen desde p
    DynList<Arc*> out_arcs(Node * p) const noexcept
    {
      return Aleph::out_arcs<Net_Graph>(p);
    }

    // Nodos a los cuales se les accede desde p
    DynList<Node*> out_nodes(Node * p) const noexcept
    {
      return out_pairs<Net_Graph>(p).template maps<Node*>
        ([] (const ArcPair<Net_Graph> & p) { return get<1>(p); });
    }

    // Arcos que llegan a p
    DynList<Arc*> in_arcs(Node * p) const noexcept
    {
      return Aleph::in_arcs<Net_Graph>(p);
    }

    // Nodos desde los cuales se llega a p
    DynList<Node*> in_nodes(Node * p) const noexcept
    {
      return in_pairs<Net_Graph>(p).template maps<Node*>
        ([] (const ArcPair<Net_Graph> & p) { return get<1>(p); });
    }

    /// Retorna la capacidad total de entrada del nodo node.
    Flow_Type get_in_cap(Node * node) const
    {
      Flow_Type sum = 0.0;
      for (__In_Iterator<Net_Graph> it(node); it.has_curr(); it.next_ne())
        sum += it.get_curr()->cap;
      return sum;
    }

    /// Retorna la capacidad total de salida del nodo node.
    Flow_Type get_out_cap(Node * node) const
    {
      Flow_Type sum = 0.0;
      for (__Out_Iterator<Net_Graph> it(node); it.has_curr(); it.next_ne())
        sum += it.get_curr()->cap;
      return sum;
    }

    /// Retorna el grado de entrada del nodo (cantidad de arcos que
    /// inciden sobre él.
    size_t get_in_degree(Node * p) const noexcept
    {
      return this->in_degree(p); 
    }

    /// Retorna el grado de salida del nodo (cantidad de arcos que
    /// salen él.
    size_t get_out_degree(Node * p) const noexcept
    {
      return this->out_degree(p);
    }

    /// Retorna el valor de flujo de salida del nodo.
    Flow_Type get_out_flow(Node * node) const
    {
      Flow_Type sum = 0;
      for (__Out_Iterator<Net_Graph> it(node); it.has_curr(); it.next_ne())
        sum += it.get_curr()->flow;
      return sum;
    }

    /// Retorna el valor de flujo de entrada del nodo.
    Flow_Type get_in_flow(Node * node) const
    {
      Flow_Type sum = 0;
      for (__In_Iterator<Net_Graph> it(node); it.has_curr(); it.next_ne())
        sum += it.get_curr()->flow;
      return sum;
    }

    /// Retorna true si node es fuente.
    bool is_source(Node * node) noexcept { return src_nodes.contains(node); }

    /// Retorna true si node es sumidero
    bool is_sink(Node * node) noexcept { return sink_nodes.contains(node); }

    /// Retorna true si la red es de un solo fuente
    bool is_single_source() const noexcept { return src_nodes.size() == 1; }

    /// Retorna true si la red es de un solo sumidero
    bool is_single_sink() const noexcept { return sink_nodes.size() == 1; }

    /// Retorna true si el nodo node está conectado. El propósito de este
    /// método es de validación, como parte de la verificación de que una
    /// red sea conexa. 
    bool is_connected(Node * p) const noexcept
    {
      return get_in_degree(p) != 0 or get_out_degree(p) != 0;
    }

    /// Retorna true si el nodo node satisface las condiciones de flujo;
    /// es decir, que el flujo de entrada sea igual al de salida.
    bool check_node(Node * node) noexcept
    {
      if (not is_connected(node))
        return false;

      auto out_flow = get_out_flow(node);
      auto in_flow = get_in_flow(node);

      if (is_sink(node))
        return out_flow == 0 and in_flow >= 0;

      if (is_source(node))
        return in_flow == 0 and out_flow >= 0;

      return out_flow == in_flow;
    }

  private:

    DynSetTree<Node*> src_nodes;
    DynSetTree<Node*> sink_nodes;

  public:

    /// Retorna el conjunto de nodos fuente que contiene la red.
    const DynSetTree<Node*> & get_src_nodes() const noexcept { return src_nodes; }

    /// Retorna el conjunto de nodos sumidero que contiene la red.
    const DynSetTree<Node*> & get_sink_nodes() const noexcept
    {
      return sink_nodes;
    }

  public:

    /// Convierte una red con varios fuentes a una red con un sólo nodo
    /// supra-fuente.  Dispara excepción si la red no contiene un nodo
    /// fuente o si no hay suficiente memoria.
    void make_super_source()
    {
      if (src_nodes.size() == 1)
        return;

      if (src_nodes.size() == 0)
        throw std::domain_error("network has not source node (it has cicles)");

      Node * super_source = insert_node();
      src_nodes.for_each([super_source, this] (Node * p)
                         {
                           insert_arc(super_source, p, get_out_cap(p));
                         });

      with_super_source = true;
    }

    /// Restaura una red con un nodo supra-fuente a la red original con
    /// varios nodos fuente.
    void unmake_super_source() noexcept
    {
      if (not with_super_source)
        return;

      assert(src_nodes.size() == 1);

      remove_node(src_nodes.get_item());
      with_super_source = false;
    }

    /// Convierte una red con varios sumideros a una red con un sólo nodo
    /// supra-sumidero.  Dispara excepción si la red no contiene un nodo
    /// sumidero  o si no hay suficiente memoria.
    void make_super_sink()
    {
      if (sink_nodes.size() == 1) 
        return;

      if (sink_nodes.size() == 0)
        throw std::domain_error("network has not sink node (it has cicles)");

      Node * super_sink = insert_node(); 
      sink_nodes.for_each([super_sink, this] (Node * p)
                          {
                            insert_arc(p, super_sink, get_in_cap(p));
                          });
      with_super_sink = true;
    }

    /// Restaura una red con un nodo supra-sumidero a la red original con
    /// varios nodos sumidero.
    void unmake_super_sink() noexcept
    {
      if (not with_super_sink) 
        return;

      assert(sink_nodes.size() == 1);

      remove_node(sink_nodes.get_item());
      with_super_sink = false;
    }

    /// Convierte una red con varios nodos fuente y sumideros a una
    /// red con un solo nodo supra-fuente y uno supra-sumidero.  
    void make_super_nodes()
    {
      make_super_source();
      try
        {
          make_super_sink();
        }
      catch (std::bad_alloc)
        {
          unmake_super_source();
        }
    }

    /// Restaura una red con un nodo supra-fuente y uno solo
    /// supra-sumidero a la red original con varios nodos fuentes
    /// y sumideros.
    void unmake_super_nodes()
    {
      unmake_super_source();
      unmake_super_sink();
    }

    /// Retorna un nodo fuente de la red
    Node * get_source() const { return src_nodes.get_item(); }

    /// Retorna un nodo sumidero de la red
    Node * get_sink() const { return sink_nodes.get_item(); }

    /** Inserta un nuevo nodo en la red.

        insert_node(node_info) aparta memoria para un nodo de red, le
        copia la información asociada node_info e inserta el nodo en la
        red this.

        @param[in] node_info la información a copiar en el nodo.
        @return puntero al nuevo nodo insertado en la red.
        @throw bad_alloc si no hay suficiente memoria.
    */
    Node * insert_node(const Node_Type & node_info)
    {
      auto p = Graph::insert_node(node_info); 
      try
        {
          src_nodes.insert(p);
          try
            {
              sink_nodes.insert(p);
              return p;
            }
          catch (bad_alloc)
            {
              src_nodes.remove(p);
              Graph::remove_node(p);
              throw;
            }
        }
      catch (bad_alloc)
        {
          Graph::remove_node(p);
          throw; // propagar excepción 
        }
    }

    /// Inserta un nuevo nodo en la red. El valor de información es
    /// indeterminado. Dispara bad_alloc si no hay suficiente memoria.
    Node * insert_node(Node_Type && info = Node_Type()) 
    { 
      return insert_node(info); 
    }

    template <typename ...Args>
    Node * emplace_node(Args && ... args) 
    { 
      return insert_node(Node_Type(args...)); 
    }

  

    /** Inserta un nuevo nodo en la red copia de otro nodo p.

        insert_node(p) aparta memoria para un nodo de red, le
        copia la información contenida en p e inserta el nuevo nodo en la
        red this.

        @param[in] p otro nodo de la misma o de otra red del cual se
        copiará la información asociada al nodo.
        @return puntero al nuevo nodo insertado en la red.
        @throw bad_alloc si no hay suficiente memoria.
    */
    Node * insert_node(Node * p)
    {
      Graph::insert_node(p); 
      try
        {
          src_nodes.insert(p);
          try
            {
              sink_nodes.insert(p);
              return p;
            }
          catch (bad_alloc)
            {
              src_nodes.remove(p);
              Graph::remove_node(p);
              throw;
            }
        }
      catch (bad_alloc)
        {
          Graph::remove_node(p);
          throw; // propagar excepción 
        }
    }

    /** Inserta un arco en una red de flujo.

        insert_arc() crea un nuevo arco con valor de atributo
        arc_info, con valor de capacidad cap y flujo flow,
        desde el nodo src_node hacia el nodo tgt_node.

        @param[in] src_node puntero hacia el nodo origen.
        @param[in] tgt_node puntero hacia el nodo destino.
        @param[in] cap valor de capacidad del arco.
        @param[in] flow valor de flujo del arco. 
        @param[in] arc_info valor de atributo a guardar en el nuevo
        arco.
        @return puntero hacia el nuevo arco insertado
        @exception std::bad_alloc si no hay suficiente memoria para crear
        el arco.
        @exception std::overflow_error si el valor de flujo es mayor
        que el de la capacidad.
    */
    Arc * insert_arc(Node * src_node, Node * tgt_node, 
                     const Flow_Type & cap, const Flow_Type & flow,
                     const typename Arc::Arc_Type & arc_info = Arc_Type())
    {     // inserción en clase base
      auto arc = Graph::insert_arc(src_node, tgt_node, arc_info); 

      src_nodes.remove(tgt_node);  // actualización de source/sink
      sink_nodes.remove(src_node); 

      arc->cap  = cap;            // ajuste capacidad y flujo de arco
      arc->flow = flow;

      if (not arc->check_arc())
        throw std::overflow_error("flow is greater than capacity");

      return arc;
    }

    template <typename ...Args>
    Arc * emplace_arc(Node * src_node, Node * tgt_node, 
                      const Flow_Type & cap, const Flow_Type & flow,
                      Args && ... args)
    {   
      return insert_arc(src_node, tgt_node, cap, flow, Arc_Type(args...));
    }

    /** Conecta un arco de red previamente insertado y desconectado.

        Este método toma un puntero a un arco arc, previamente
        desconectado mediante, desconnect_arc(), y lo re-inserta en el
        grafo. 

        El arco, por supuesto, debe haber sido previamente insertado en
        el grafo. A este respecto, no se hace ninguna verificación. 

        No se realiza ninguna verificación de existencia previa de un
        arco entre los nodos involucrados (esto es necesario para operar
        con multigrafos).

        @param[in] arc puntero al arco a re-insertar.
        @return puntero al arco re-insertado
    */
    Arc * connect_arc(Arc * arc)
    {     
      Graph::connect_arc(arc); 

      auto src = this->get_src_node(arc);
      auto tgt = this->get_tgt_node(arc);

      src_nodes.remove(tgt);        // elimina destino de src_nodes
      sink_nodes.remove(src);       // elimina fuente de sink_nodes

      return arc;
    }

    /** Inserta un arco en una red de flujo.

        insert_arc() crea un nuevo arco con valor de atributo
        arc_info, con valor de capacidad cap
        desde el nodo src_node hacia el nodo tgt_node.

        El valor de flujo es cero.

        @param[in] src_node puntero hacia el nodo origen.
        @param[in] tgt_node puntero hacia el nodo destino.
        @param[in] cap valor de capacidad del arco.
        @return puntero hacia el nuevo arco insertado
        @exception std::bad_alloc si no hay suficiente memoria para crear
        el arco.
        @exception std::overflow_error si el valor de flujo es mayor
        que el de la capacidad.
    */
    Arc * insert_arc(Node * src_node, Node * tgt_node, const Flow_Type & cap)
    {
      return insert_arc(src_node, tgt_node, cap, 0, Arc_Type());
    }

    /** Inserta un arco en una red de flujo.

        insert_arc() crea un nuevo arco con valor de atributo
        arc_info desde el nodo src_node hacia el nodo tgt_node.

        El arco se crea con valores de capacidad y flujo de cero.

        @param[in] src_node puntero hacia el nodo origen.
        @param[in] tgt_node puntero hacia el nodo destino.
        @param[in] arc_info valor de atributo a guardar en el nuevo
        arco.
        @return puntero hacia el nuevo arco insertado
        @exception std::bad_alloc si no hay suficiente memoria para crear
        el arco.
        @exception std::overflow_error si el valor de flujo es mayor
        que el de la capacidad.
    */
    Arc * insert_arc(Node * src_node, Node * tgt_node, 
                     const typename Arc::Arc_Type & arc_info = Arc_Type())
    {
      return insert_arc(src_node, tgt_node, 0, 0, arc_info);
    }

    /// Elimina de la red el arco arc.
    virtual void remove_arc(Arc * arc)
    {
      auto src = this->get_src_node(arc);
      auto tgt = this->get_tgt_node(arc);
      if (get_in_degree(tgt) == 1)
        src_nodes.insert(tgt);  // tgt deviene un nodo fuente 

      Graph::remove_arc(arc); // eliminar en clase base

      if (get_out_degree(src) == 0)
        sink_nodes.insert(src); // src deviene un nodo sumidero
    }

    /** Desconecta de la red el arco arc.

        La operación desconecta del grafo el arco arc. Eventualmente,
        el arco puede guardarse y luego reinsertarse mediante
        connect_arc(). 

        El arco debe pertenecer al grafo y no se realiza ninguna
        verificación al respecto.

        @param[in]  arc puntero al arco a desconectar
    */
    void disconnect_arc(Arc * arc) noexcept
    {
      auto src = this->get_src_node(arc);
      auto tgt = this->get_tgt_node(arc);
      if (get_in_degree(tgt) == 1)
        src_nodes.insert(tgt); // tgt deviene un nodo fuente 

      Graph::disconnect_arc(arc); // desconeción en clase base

      if (get_out_degree(src) == 0)
        sink_nodes.insert(src); // src deviene un nodo sumidero
    }

    /// Elimina un nodo de una red de flujo junto con todos sus arcos.
    virtual void remove_node(Node * p) noexcept
    {
      Graph::remove_node(p);  // eliminación en clase base
      src_nodes.remove(p);
      sink_nodes.remove(p);
    }

    /// Construye una red copia de la red net. Dispara bad_alloc si no
    /// hay suficiente memoria.
    Net_Graph(const Net_Graph & net) 
      : Array_Graph<NodeT, ArcT>::Array_Graph(),
      Infinity(numeric_limits<typename Arc::Flow_Type>::max()),
      with_super_source(net.with_super_source),
      with_super_sink(net.with_super_sink)
    {
      copy_graph(*this, net, false); // copia sin mapear
    
      using Pair = std::pair<typename Net_Graph::Arc*, typename Net_Graph::Arc*>;
      zip(this->arcs(), net.arcs()).for_each([] (const Pair & p)
                                             {
                                               auto atgt = p.first;
                                               auto asrc = p.second;
                                               atgt->cap = asrc->cap;
                                               atgt->flow = asrc->flow;
                                             });
    }

    /// Coloca valor de capacidad a un arco. 
    void set_cap(Arc * arc, const Flow_Type & cap)
    {
      if (cap < arc->flow)
        throw std::out_of_range("capacity value is smaller than flow");

      arc->cap = cap;
    }

    /// Coloca valor de flujo a un arco. Dispara excepción si el valor es
    /// mayor que la capacidad.
    void set_flow(Arc * arc, const Flow_Type & flow)
    {
      if (flow > arc->cap)
        throw std::out_of_range("flow value is greater than capacity");

      arc->flow = flow;
    }

    /// retorna el valor de capacidad del arco.
    const Flow_Type & get_flow(Arc * arc) const noexcept { return arc->flow; }

    /// Retorna el valor de flujo de un arco.
    const Flow_Type & get_cap(Arc * arc) const noexcept { return arc->cap; }

    void reset()
    {
      for (Arc_Iterator<Net_Graph> it(*this); it.has_curr(); it.next_ne())
        it.get_curr()->flow = 0;
    }

    /** Verifica si una red capacitada satisface las condiciones de
        definición.   

        check_network() recorre todos los nodos y arcos de una red
        capacitada en búsqueda de inconsistencias de
        definición. Básicamente, se verifica que para todo nodo no
        fuente o no sumidero la cantidad de flujo entrante sea igual a
        la saliente. Por cada arco inspeccionado se verifica que el
        flujo sea menor o igual que su capacidad.

        @return true si la red es valida; false de lo contrario.
    */
    bool check_network()
    {
      return this->nodes().all([this] (Node * p) { return check_node(p); }) and
        this->arcs().all([] (Arc * a) { return a->check_arc(); }) and
        get_out_flow(get_source()) == get_in_flow(get_sink());
    }

    /// Retorna el valor de flujo de la red visto desde el nodo fuente o
    /// sumidero p. Dispara excepción std::domain_error si p no es ni
    /// fuente ni sumidero.
    Flow_Type flow_value() const
    {
      assert(get_in_flow(get_source()) == get_out_flow(get_sink()));
      return get_out_flow(get_source()); 
    }

    /// true si a la red se le ha añadido un supra fuente.
    bool with_super_source; 

    /// true si a la red se le ha añadido un supra sumidero.
    bool with_super_sink;

    Net_Graph() 
      : Infinity(numeric_limits<typename Arc::Flow_Type>::max()),
        with_super_source(false), with_super_sink(false)
    { /* empty */ }

    friend ostream & operator << (ostream & s, const Path<Net_Graph> & path)
    {
      if (path.is_empty())
        return s << "Path is Empty";

      const Net_Graph & net = path.get_graph();
      typename Path<Net_Graph>::Iterator it(path);
      s << it.get_current_node()->get_info();
      for (; it.has_current_arc(); it.next_ne())
        {
          typename Net_Graph::Arc * a = it.get_current_arc_ne();
          s << "(" << a->cap << "," << a->flow << ")" 
            << net.get_connected_node(a, it.get_current_node_ne())-> get_info();
        }
      return s;
    }
  };

  /** Define un arco que pertenece a un semicamino
   
      @ingroup Redes
  */
  template <class Net>
  using Parc = tuple<typename Net::Arc*, bool>; // 2do campo dice si normal

  /** Define un semicamino compuesto por:
      -. bool: dise si el camino fue o no encontrado
      -. Flow_Type: el valor del slack del camino
      -. Lista de pares arco e indicación de si es normal o reducido. true
      ==> normal

      @ingroup Redes
  */
  template <class Net>
  using SemiPath = tuple<bool, typename Net::Flow_Type, DynList<Parc<Net>>>;

  template <class Net>
  inline void print(const DynList<Parc<Net>> & sp)
  {
    if (sp.is_empty())
      {
        cout << "Semi path is Empty";
        return;
      }

    for (typename DynList<Parc<Net>>::Iterator it(sp); it.has_curr();
         it.next_ne())
      {
        const Parc<Net> & pa = it.get_curr();
        auto a = get<0>(pa);
        auto s = (typename Net::Node *) a->src_node;
        auto t = (typename Net::Node *) a->tgt_node;
        cout << s->get_info() << "(" << a->flow << "," << a->cap << ")" 
             << t->get_info() << " " << (get<1>(pa) ? "Normal" : "Reduced") 
             << endl;
      }
  }

  /** Incrementa el flujo de una red según un camino de aumento.

      increase_flow(net, path) toma el camino de aumento contenido en path
      e incrementa el valor de flujo de net en el eslabón mínimo
      (slack) del camino. 

      La biblioteca permite calcular caminos de aumento sobre la red
      residual de dos maneras:
      -# find_path_depth_first<Net, Res_F<Net> >(net, source, sink, path)
      -# find_path_breadth_first<Net, Res_F<Net> >(net, source, sink, path)
      .
      Para que tenga sentido cualquiera de estas maneras de encontrar
      caminos de aumento, la red residual sobre net debe estar
      instanciada. Pare ello, debe haberse llamado previamente a
      make_residual_net(). 

      @param[in] net la red capacitada por donde se desea incrementar
      el flujo por el camino de aumento.
      @param[in] path el camino de tipo Path donde se quiere
      almacenar el camino de aumento.
      @return el valor en que ha sido aumentado el flujo.
      @see Path find_path_depth_first() find_path_breadth_first() make_residual_net()
      @throw domain_error si la red residual no ha sido creada 
      @ingroup Redes
  */
  template <class Net>
  typename Net::Flow_Type increase_flow(Net & net, const Path<Net> & path)
  {
    typename Net::Flow_Type slack = net.Infinity; // eslabón mínimo
    using Tuple = tuple<typename Net::Node*, typename Net::Arc*>;

    // calcular el eslabón mínimo del camino de aumento
    for (typename Path<Net>::Iterator it(path); it.has_current_arc();
         it.next_ne())
      {
        Tuple t = it.get_tuple_ne();
        auto p = get<0>(t);
        auto arc = get<1>(t);
        const auto w = remaining_flow<Net>(p, arc);
        if (w < slack)
          slack = w;
      }

    // aumentar el flujo de la red por el camino de aumento
    for (typename Path<Net>::Iterator it(path); it.has_current_arc(); it.next_ne())
      {
        auto t = it.get_tuple_ne();
        auto p = get<0>(t);
        auto arc = get<1>(t);

        if (is_residual<Net>(p, arc))
          arc->flow -= slack; 
        else
          arc->flow += slack; 

        assert(arc->check_arc());
      }

    return slack;
  }


  /** Incrementa el flujo de una red según un camino de aumento.

      increase_flow(net, semi_path) toma el camino de aumento
      contenido en path 
      e incrementa el valor de flujo de net en el eslabón mínimo
      (slack) del camino. 

      @param[in] net la red capacitada por donde se desea incrementar
      el flujo por el camino de aumento.
      @param[in] semi_path semicamino por donde se aumentará el flujo
      @param[in] slack valor de aumento dl camino. Debe ser menor o
      igual al slack mínimo del camino. 
      @warning No se valida el slack
      @ingroup Redes
  */
  template <class Net>
  void increase_flow(Net & net, 
                     const DynList<Parc<Net>> & semi_path,
                     const typename Net::Flow_Type slack)
  {
    // aumentar el flujo de la red por el camino de aumento
    for (typename DynList<Parc<Net>>::Iterator it(semi_path); it.has_curr(); 
         it.next_ne())
      {
        auto p = it.get_curr();
        auto arc = get<0>(p);
        if (get<1>(p)) // es arco de normal?
          arc->flow += slack; 
        else
          arc->flow -= slack; 

        assert(arc->check_arc());
      }
    assert(net.check_network());
  }
  
  /** Decrementa el flujo de una red según un camino de aumento.

      decrease_flow(net, semi_path) toma el camino de aumento
      contenido en path 
      y decrementa el valor de flujo de net en el eslabón mínimo 
      (slack) del camino. 

      @param[in] net la red capacitada por donde se desea incrementar
      el flujo por el camino de aumento.
      @param[in] semi_path semicamino por donde se aumentará el flujo
      @param[in] slack valor de aumento dl camino. Debe ser menor o
      igual al slack mínimo del camino. 
      @warning No se valida el slack
      @ingroup Redes
  */
  template <class Net>
  void decrease_flow(Net & net, const DynList<Parc<Net>> & semi_path,
                     typename Net::Flow_Type slack)
  {
    increase_flow(net, semi_path, slack);
  }


  /** Búsqueda de caminos de aumento sobre grafos dirigidos definidos
      mediante una clase grafo (no digrafo).

      @ingroup Redes
  */
  template <class Net, template <typename T> class Q>
  class Find_Aumenting_Path
  {
    const Net & net;

    // returna nodo end si se encuentra un camino
    typename Net::Node * search(typename Net::Node* start,
                                typename Net::Node* end,
                                typename Net::Flow_Type min_slack)
    {
      using Itor = Net_Iterator<Net>;
      net.reset_nodes();
      net.reset_arcs();

      start->set_state(Processed);
      Q<typename Net::Arc*> q;
      for (Itor it(start); it.has_curr(); it.next_ne())
        {
          auto a = it.get_curr();
          if (remaining_flow<Net>(start, a) < min_slack)
            continue;
          auto tgt = net.get_connected_node(a, start);
          tgt->set_state(Processing);
          a->set_state(Processing);
          q.put(a);
        }

      typename Net::Node * curr = nullptr;
      while (not q.is_empty())
        {
          auto arc = q.get();
          assert(arc->state() == Processing);
          arc->set_state(Processed);

          auto s = net.get_src_node(arc);
          auto t = net.get_tgt_node(arc);

          if (s->state() == Processed and t->state() == Processed)
            continue;

          curr = s->state() == Processed ? t : s;
          assert(curr->state() == Processing);
          curr->set_state(Processed);
          NODE_COOKIE(curr) = net.get_connected_node(arc, curr);

          if (curr == end)
            return curr;

          for (Itor it(curr); it.has_curr(); it.next_ne())
            {
              auto a = it.get_curr();
              if (remaining_flow<Net>(curr, a) < min_slack)
                continue;
              a->set_state(Processing);

              auto tgt = net.get_connected_node(a, curr);
              if (tgt->state() == Processed)
                continue;

              if (tgt->state() != Processed)
                {
                  q.put(a);
                  tgt->set_state(Processing);
                }
              else
                a->set_state(Processed);
            }
        } // end while

      return nullptr;
    }

    Path<Net> find(typename Net::Node * start, typename Net::Node * end,
                   const typename Net::Flow_Type & min_slack = 0.0)
    {
      auto curr = search(start, end, min_slack);

      Path<Net> ret(net);
      if (not curr)
        return ret;

      assert(curr == end);

      while (curr != start)
        {
          ret.insert(curr);
          curr = (typename Net::Node*) NODE_COOKIE(curr);
        }
      ret.insert(start);

      return ret;
    }

    SemiPath<Net> find_path(typename Net::Node * start,
                            typename Net::Node * end,
                            typename Net::Flow_Type min_slack = 0.0)
    {
      auto t = search(start, end, min_slack);
      if (not t)
        return make_tuple(false, 0, DynList<Parc<Net>>());

      assert(t == end);

      DynList<Parc<Net>> semi_path;
      auto m = std::numeric_limits<typename Net::Flow_Type>::max();
      while (t != start)
        {
          auto s = (typename Net::Node*) NODE_COOKIE(t);
          auto a_ptr = net.arcs(t).find_ptr([s, t] (typename Net::Arc * a)
          {
            return ((a->src_node == s and a->tgt_node == t) or
                    (a->src_node == t and a->tgt_node == s));
          });
          assert(a_ptr);
          auto a = *a_ptr;
          bool normal = a->tgt_node == t;
          auto slack = normal ? a->cap - a->flow : a->flow;
          m = std::min(m, slack);
          semi_path.insert(make_tuple(a, normal));
          t = s;
        }

      return make_tuple(true, m, move(semi_path));
    }

  public:

    /// Encuentra un camino de aumento de al menos un slack
    SemiPath<Net> find_aum_path(typename Net::Flow_Type min_slack = 0.0)
    {
      return find_path(net.get_source(), net.get_sink(), min_slack);
    }

    /// Encuentra un camino de decremento de al menos un slack
    SemiPath<Net> find_dec_path(typename Net::Flow_Type min_slack = 0.0)
    {
      return find_path(net.get_sink(), net.get_source(), min_slack);
    }

    Find_Aumenting_Path(const Net & __g) : net(__g)
    {
      // empty
    }

    Path<Net> operator () (typename Net::Node *    start,
                           typename Net::Node *    end,
                           typename Net::Flow_Type min_slack = 0)
    {
      return find(start, end, min_slack);
    }

    SemiPath<Net> operator () (typename Net::Flow_Type min_slack = 0)
    {
      return find_aum_path(min_slack);
    }

    typename Net::Flow_Type
    semi_path(typename Net::Node *            start,
              typename Net::Node *            end,
              DynList<Parc<Net>> &            semi_path,
              const typename Net::Flow_Type & min_slack = 0)
    {
      return find_semi_path(start, end, semi_path, min_slack);
    }
  };


  template <class Net>
  using Find_Aumenting_Path_DFS = Find_Aumenting_Path<Net, DynListStack>;


  template <class Net>
  using Find_Aumenting_Path_BFS = Find_Aumenting_Path<Net, DynListQueue>;


  /** Encuentra un camino de aumento mediante una búsqueda en profundidad
      o en amplitud según sea el valor del parámetro plantilla Q. Q puede
      ser una pila, en cuyo caso la búsqueda será en profundidad, o una
      cola, en cuyo caso la búsqueda será en amplitud

      @param[in] net red capacitada de flujo
      @param[in] min_slack valor mínimo de decremento del camino
      @param[in] sa valor del filtro de arcos

      @ingroup Redes
  */
  template <class Net, template <typename T> class Q>
  Path<Net> find_aumenting_path(const Net &                     net,
                                const typename Net::Flow_Type & min_slack)
  {
    auto s = net.get_source();
    auto t = net.get_sink();
    return Find_Aumenting_Path<Net, Q> (net) (s, t, min_slack);
  }


  /** Encuentra un camino de aumento mediante una búsqueda en profundidad.

      @param[in] net red capacitada de flujo
      @param[in] min_slack valor mínimo de aumento del camino
      @param[in] sa valor del filtro de arcos

      @ingroup Redes
  */
  template <class Net>
  Path<Net> find_aumenting_path_dfs(const Net &                     net,
                                    const typename Net::Flow_Type & min_slack)
  {
    return find_aumenting_path<Net, DynListStack> (net, min_slack);
  }


  /** Encuentra un camino de aumento mediante una búsqueda en amplitud.

      @param[in] net red capacitada de flujo
      @param[in] min_slack valor mínimo de aumento del camino
      @param[in] sa valor del filtro de arcos

      @ingroup Redes
  */
  template <class Net>
  Path<Net> find_aumenting_path_bfs(const Net &                     net,
                                    const typename Net::Flow_Type & min_slack)
  {
    return find_aumenting_path<Net, DynListQueue> (net, min_slack);
  }


  /** Encuentra un camino de aumento en forma de semicamino mediante una
      búsqueda en profundidad o en amplitud según sea el valor del
      parámetro plantilla Q. Q puede ser una pila, en cuyo caso la
      búsqueda será en profundidad, o una cola, en cuyo caso la búsqueda
      será en amplitud

      @param[in] net red capacitada de flujo
      @param[out] semi_path lista de pares (Arc,bool) que describe el semicamino
      @param[in] min_slack valor mínimo de decremento del camino
      @param[in] sa valor del filtro de arcos
      @return en slack del semicamino. Un valor igual al dado en parámetro
      indica que el camino fue encontrado satisfactoriamente

      @ingroup Redes
  */
  template <class Net, template <typename T> class Q>
  SemiPath<Net> find_aumenting_semi_path(const Net &                     net,
                                         const typename Net::Flow_Type & slack)
  {
    return Find_Aumenting_Path<Net, Q> (net). find_aum_path(slack);
  }


  /** Encuentra un camino de aumento en forma de semicamino mediante una
      búsqueda en profundidad.

      @param[in] net red capacitada de flujo
      @param[out] semi_path lista de pares (Arc,bool) que describe el semicamino
      @param[in] min_slack valor mínimo de decremento del camino
      @param[in] sa valor del filtro de arcos
      @return en slack del semicamino. Un valor igual al dado en parámetro
      indica que el camino fue encontrado satisfactoriamente

      @ingroup Redes
  */
  template <class Net>
  SemiPath<Net>
  find_aumenting_semi_path_dfs(const Net &                     net,
                               const typename Net::Flow_Type & slack)
  {
    return find_aumenting_semi_path<Net, DynListStack> (net, slack);
  }


  /** Encuentra un camino de aumento en forma de semicamino mediante una
      búsqueda en amplitud.

      @param[in] net red capacitada de flujo
      @param[out] semi_path lista de pares (Arc,bool) que describe el semicamino
      @param[in] min_slack valor mínimo de decremento del camino
      @param[in] sa valor del filtro de arcos
      @return en slack del semicamino. Un valor igual al dado en parámetro
      indica que el camino fue encontrado satisfactoriamente

      @ingroup Redes
  */
  template <class Net>
  SemiPath<Net>
  find_aumenting_semi_path_bfs(const Net &                     net,
                               const typename Net::Flow_Type & slack)
  {
    return find_aumenting_semi_path<Net, DynListQueue> (net, slack);
  }


  /** Encuentra un camino de decremento mediante una búsqueda en profundidad
      o en amplitud según sea el valor del parámetro plantilla Q. Q puede
      ser una pila, en cuyo caso la búsqueda será en profundidad, o una
      cola, en cuyo caso la búsqueda será en amplitud

      @param[in] net red capacitada de flujo
      @param[in] min_slack valor mínimo de decremento del camino
      @param[out] semi_path lista de pares arco,forward que caracteriza el
      semi-camino
      @param[in] sa valor del filtro de arcos
      @return el valor del slack. Si es igual al solicitado, entonces el
      semi-camino fue encontrado

      @ingroup Redes
  */
  template <class Net, template <typename T> class Q>
  SemiPath<Net>
  find_decrementing_path(const Net &                     net,
                         const typename Net::Flow_Type & slack)
  {
    return Find_Aumenting_Path<Net, Q>(net).find_dec_path(slack);
  }


  /** Encuentra un camino de aumento mediante una búsqueda en amplitud.

      @param[in] net red capacitada de flujo
      @param[in] min_slack valor mínimo de aumento del camino
      @param[in] sa valor del filtro de arcos

      @ingroup Redes
  */
  template <class Net>
  SemiPath<Net>
  find_decrementing_path_dfs(const Net &                     net,
                             const typename Net::Flow_Type & slack)
  {
    return find_decrementing_path<Net, DynListStack> (net, slack);
  }


  /** Encuentra un camino de aumento mediante una búsqueda en amplitud.

      @param[in] net red capacitada de flujo
      @param[in] min_slack valor mínimo de aumento del camino
      @param[in] sa valor del filtro de arcos

      @ingroup Redes
  */
  template <class Net>
  SemiPath<Net>
  find_decrementing_path_bfs(const Net &                     net,
                             const typename Net::Flow_Type & slack)
  {
    return find_decrementing_path<Net, DynListQueue> (net, slack);
  }


  // Residual node for using is preflow-push algorithms
  template <class Net>
  struct PP_Res_Node : public Net_Node<Empty_Class>
  {
    using Base = Net_Node<Empty_Class>;
    using Base::Base;

    typename Net::Flow_Type in_flow;
    typename Net::Flow_Type out_flow;
  };

  template <class Net>
  struct __Res_Arc : public Net_Arc<Empty_Class, typename Net::Flow_Type>
  {
    using Base = Net_Arc<Empty_Class, typename Net::Flow_Type>;
    using Base::Base;

    typename Net::Arc * img = nullptr; // nullptr indicates residual arc
    __Res_Arc * dup = nullptr;

    bool is_residual() const { return img == nullptr; }
  };


  template <class Net>
  using AP_Res_Net = Array_Digraph<Net_Node<Empty_Class>, __Res_Arc<Net>>;

  // Residual net for preflow-push algorithms
  template <class Net>
  using PP_Res_Net = Array_Digraph<PP_Res_Node<Net>, __Res_Arc<Net>>;

  // precondition: a->src and a->tgt are mapped to residual net
  template <class Net> inline
  void create_residual_arc(const Net & net, PP_Res_Net<Net> & rnet,
                           typename Net::Arc * a)
  {
    using Rnet = PP_Res_Net<Net>;
    auto s = net.get_src_node(a);
    auto t = net.get_tgt_node(a);

    assert(NODE_COOKIE(s) and NODE_COOKIE(t));

    auto src = (typename Rnet::Node*) NODE_COOKIE(s);
    auto tgt = (typename Rnet::Node*) NODE_COOKIE(t);

    auto arc = rnet.insert_arc(src, tgt);
    auto dup = rnet.insert_arc(tgt, src);

    arc->img = a;          // mark it as not residual
    arc->cap = a->cap;
    arc->flow = a->flow;
    arc->dup = dup;

    dup->cap = arc->cap;
    dup->flow = arc->cap - arc->flow;
    dup->dup = arc;
  }

  template <class Rnet>
  struct Res_F
  {
    Res_F(typename Rnet::Node *) noexcept {}
    Res_F() noexcept {}

    bool operator () (typename Rnet::Arc * a) const
    {
      return a->cap > a->flow;
    }
  };

  template <class Net> inline
  tuple<PP_Res_Net<Net>, typename PP_Res_Net<Net>::Node*,
        typename PP_Res_Net<Net>::Node*>
  preflow_create_residual_net(Net & net)
  {
    using Rnet = PP_Res_Net<Net>;
    net.reset_nodes();
    Rnet rnet;

    for (typename Net::Node_Iterator it(net); it.has_curr(); it.next_ne())
      {
        auto p = it.get_curr();
        auto q = rnet.insert_node();
        q->in_flow = net.get_in_flow(p);
        q->out_flow = net.get_out_flow(p);
        map_nodes<typename Rnet::Node, typename Net::Node>(q, p);
      }

    for (typename Net::Arc_Iterator it(net); it.has_curr(); it.next_ne())
      create_residual_arc(net, rnet, it.get_curr());

    return make_tuple(std::move(rnet),
                      (typename Rnet::Node*) NODE_COOKIE(net.get_source()),
                      (typename Rnet::Node*) NODE_COOKIE(net.get_sink()));
  }

  template <class Rnet> inline
  void update_flow(const Rnet & rnet)
  {
    for (typename Rnet::Arc_Iterator it(rnet); it.has_curr(); it.next_ne())
      {
        auto arc = it.get_curr();
        auto img = arc->img;
        if (img == nullptr)
          continue;
        img->flow = arc->flow;
      }
  }

  /** Maximiza el flujo de la red según búsquedas de caminos de aumento.

      aumenting_path_maximum_flow() recibe una red capacitada y
      maximiza su valor de flujo mediante búsquedas sucesivas de
      caminos de aumento.

      El método recibe tres parámetros tipo:
      -# Net: el tipo de red cuyo flujo se desea maximizar y que debe
      estar derivado de Net_Graph.
      -# Find_Path: la clase de búsqueda de camino.

      La red puede tener múltiples fuentes y sumideros.

      @param[in,out] net la red capacitada cuyo flujo se desea
      maximizar.
      @param[in] sa el estado del filtro de arcos
      @return el valor de flujo maximizado de la red
      @ingroup Redes
  */
  template <class Net,
            template <class> class Find_Path>
  typename Net::Flow_Type aumenting_path_maximum_flow(Net & net)
  {
    if (not (net.is_single_source() and net.is_single_sink()))
      throw std::domain_error("Network is not single source and single sink");

    while (true) // mientras exista un camino de aumento
      {
        SemiPath<Net> semi_path = Find_Path<Net> (net) ();
        if (not get<0>(semi_path))
          break;

        increase_flow <Net> (net, get<2>(semi_path), get<1>(semi_path));
      }

    return net.get_out_flow(net.get_source());
  }


  /** Maximiza el flujo de la red según el algoritmo de
      Ford-Fulkerson.

      ford_fulkerson_maximum_flow() recibe una red capacitada y
      maximiza su valor de flujo mediante el algoritmo de
      Ford-Fulkerson.

      La red puede tener múltiples fuentes y sumideros.

      Durante el cálculo se construye una red residual parcial, cuyo
      consumo en espacio es proporcional a la cantidad de arcos.

      @param[in,out] net la red capacitada cuyo flujo se desea
      maximizar.
      @param[in] sa el filtro de arcos
      @return el valor de flujo maximizado de la red
      @exception bad_alloc si no hay suficiente memoria para la red
      residual y para los caminos de aumento internos. En este caso,
      el estado de la red se corresponde con el último incremento de
      flujo realizado antes de que ocurra excepción.
      @throw domain_error si la red residual ya ha sido calculada.
      @ingroup Redes
  */
  template <class Net>
  typename Net::Flow_Type ford_fulkerson_maximum_flow(Net & net)
  {
    return aumenting_path_maximum_flow <Net, Find_Aumenting_Path_DFS> (net);
  }


  /** Clase invocante al algoritmo de Ford Fulkerson.

      @see ford_fulkerson_maximum_flow()
      @ingroup Redes
  */
  template <class Net>
  struct Ford_Fulkerson_Maximum_Flow
  {
    /** Invoca a ford_fulkerson_maximum_flow().

        @param[in,out] net la red capacitada cuyo flujo se desea
        maximizar.
        @param[in] sa el filtro de arcos
        @return el valor de flujo maximizado de la red
    */
    typename Net::Flow_Type operator () (Net & net) const
    {
      return ford_fulkerson_maximum_flow(net);
    }
  };

  /** Maximiza el flujo de la red según el algoritmo de
      Edmonds-Karp.

      edmonds_karp_maximum_flow() recibe una red
      capacitada y maximiza su valor de flujo mediante el algoritmo
      de Edmonds-Karp.

      La red puede tener múltiples fuentes y sumideros.

      @param[in,out] net la red capacitada cuyo flujo se desea
      maximizar.
      @param[in] sa el filtro de arcos
      @return el valor de flujo maximizado de la red
      @ingroup Redes
  */
  template <class Net>
  typename Net::Flow_Type edmonds_karp_maximum_flow(Net & net)
  {
    return aumenting_path_maximum_flow <Net, Find_Aumenting_Path_BFS> (net);
  }


  /** Clase invocante al algoritmo de Ford Fulkerson.

      @see edmonds_karp_maximum_flow()
      @ingroup Redes
  */
  template <class Net>
  struct Edmonds_Karp_Maximum_Flow
  {
    /** Invoca a edmonds_karp_maximum_flow().

        @param[in,out] net la red capacitada cuyo flujo se desea
        maximizar.
        @param[in] sa el filtro de arcos
        @return el valor de flujo maximizado de la red
    */
    typename Net::Flow_Type operator () (Net & net) const
    {
      return edmonds_karp_maximum_flow<Net>(net);
    }
  };

  template <class Net> static inline
  bool is_node_active(const Net & net, typename Net::Node * p)
  {
    return net.get_in_flow(p) != net.get_out_flow(p);
  }

  template <class Rnet> static inline
  bool is_node_active(typename Rnet::Node * p)
  {
    return p->in_flow != p->out_flow;
  }


  template <class Net> static inline
  long & node_height(typename Net::Node * p) { return NODE_COUNTER(p); }


  template <class Net> static inline
  void init_height_in_nodes(Net & net)
  {
    Graph_Traverse_BFS<Net, Node_Arc_Iterator<Net>>(net).
      exec(net.get_sink(),
           [&net] (typename Net::Node * p, typename Net::Arc * a)
           {
             if (a)
               node_height<Net>(p) = node_height<Net>(net.get_tgt_node(a)) + 1;
             return true;
           });
  }


  template <class Q_Type> static inline
  void put_in_active_queue(Q_Type & q, typename Q_Type::Item_Type & p)
  {
    if (NODE_BITS(p).get_bit(Aleph::Maximum_Flow))
      return; // the node is already into the queue
    NODE_BITS(p).set_bit(Aleph::Maximum_Flow, true);
    q.put(p);
  }


  template <class Q_Type> static inline
  typename Q_Type::Item_Type get_from_active_queue(Q_Type & q)
  {
    auto p = q.get();
    assert(NODE_BITS(p).get_bit(Aleph::Maximum_Flow));
    NODE_BITS(p).set_bit(Aleph::Maximum_Flow, false);
    return p;
  }


  template <class Q_Type> static inline
  void remove_from_active_queue(Q_Type & q, typename Q_Type::Item_Type & p)
  {
    assert(NODE_BITS(p).get_bit(Aleph::Maximum_Flow));
    NODE_BITS(p).set_bit(Aleph::Maximum_Flow, false);
    q.remove(p);
  }


  /** Flujo máximo según algoritmo genérico de empuje de preflujo
      orientado hacia nodos.

      generic_preflow_vertex_push_maximum_flow(net) calcula el flujo
      máximo de la red net según el algoritmo genérico de Goldberg y
      Tarjan. La función emplea dos parámetros tipo:
      -# Net: una clase de red capacitada derivada del tipo Net_Graph.
      -# Q_Type: una clase conjunto de "nodos activos". Q_Type debe
      manejar punteros de tipo typename Net::Node y debe soportar
      las siguientes operaciones:
      -# get(): extracción de un nodo.
      -# put(p): inserción del nodo p en el conjunto.
      .
      .

      Durante la ejecución del algoritmo se emplean los contadores y
      cookies de todos los nodos de la red. El bit Maximum_Flow es
      usado para los arcos.

      @param[in,out] net una red capacitada a maximizar su flujo. net
      puede tener varios fuentes y sumideros.
      @param[in] leave_residual indica si la red residual requerida
      para los cálculos parciales debe dejarse instanciada o no. Por
      omisión, la red residual, así como el supra grafo en caso de
      que la red tenga varios fuentes o sumideros, son liberados al
      final del cálculo. Si el parámetro es true, entonces la red
      residual queda instanciada y eventualmente utilizable por
      otros algoritmos.
      @return el valor de flujo maximizado de la red
      @throw bad_alloc si no hay suficiente memoria.
      @throw domain_error si la red residual ya está calculada.
      @see fifo_preflow_maximum_flow() heap_preflow_maximum_flow() random_preflow_maximum_flow()
      @ingroup Redes
  */
  template <class Net, class Q_Type> typename Net::Flow_Type
  generic_preflow_vertex_push_maximum_flow(Net & net)
  {
    if (not (net.is_single_source() and net.is_single_sink()))
      throw std::domain_error("Network is not single source and single sink");

    net.reset_nodes();         // especially assures that counters are in zero
    init_height_in_nodes(net); // set height to minimum distance in nodes to sink

    auto source = net.get_source();
    auto sink   = net.get_sink();
    node_height<Net>(source) = net.vsize(); // except the source
    const auto Max = numeric_limits<long>::max();

    using Itor = __Net_Iterator<Net>;
    Q_Type q;
    for (Itor it(source); it.has_curr(); it.next_ne()) // initial preflow
      {
        auto arc  = it.get_curr();
        arc->flow = arc->cap; // saturate arc
        auto tgt = net.get_tgt_node(arc);
        put_in_active_queue(q, tgt);
      }

    while (not q.is_empty()) // while there are active nodes
      {
        auto src = get_from_active_queue(q);
        auto excess = net.get_in_flow(src) - net.get_out_flow(src);
        long m = Max;
        bool was_eligible_arc = false;
        for (Itor it(src); it.has_curr() and excess > 0; it.next_ne())
          {
            auto arc = it.get_curr();
            auto tgt = net.get_connected_node(arc, src);
            if (node_height<Net>(src) != node_height<Net>(tgt) + 1)
              {
                m = std::min(m, node_height<Net>(tgt));
                continue;
              }

            was_eligible_arc = true;
            typename Net::Flow_Type flow_to_push;
            if (is_residual<Net>(src, arc))
              {
                flow_to_push = std::min(arc->flow, excess);
                arc->flow -= flow_to_push;
              }
            else
              {
                flow_to_push = std::min(arc->cap - arc->flow, excess);
                arc->flow += flow_to_push;
              }
            excess -= flow_to_push;
            if (tgt != source and tgt != sink)
              {
                assert(is_node_active(net, tgt));
                put_in_active_queue(q, tgt);
              }
          }

        if (excess > 0) // is src still active
          {
            if (not was_eligible_arc)
              node_height<Net>(src) = m + 1;
            put_in_active_queue(q, src);
          }
      }

    assert(net.check_network());

    return net.flow_value(); 
  }


  template <class Rnet> inline
  void init_height_in_nodes(Rnet & rnet, typename Rnet::Node * sink)
  {
    Graph_Traverse_BFS<Rnet, Node_Arc_Iterator<Rnet>>(rnet).
      exec(sink, [&rnet] (typename Rnet::Node * p, typename Rnet::Arc * a)
                 {
                   if (a)
                     node_height<Rnet>(p) = node_height<Rnet>(rnet.get_src_node(a)) + 1;
                   return true;
                 });
  }


  /** Flujo máximo de empuje de preflujo con cola FIFO de nodos.

      fifo_preflow_vertex_push_maximum_flow(net) calcula el flujo
      máximo de la red net según el algoritmo genérico de Goldberg y
      Tarjan en donde los nodos activos, aquellos cuyo flujo de
      entrada es mayor que el de salida son almacenados y procesados
      según una cola FIFO. Este algoritmo procesa empuja el preflujo
      por los arcos según una heurística en profundidad.

      Durante la ejecución del algoritmo se emplean los contadores y
      cookies de todos los nodos de la red. El bit Maximum_Flow es
      usado para los arcos.

      @param[in,out] net una red capacitada a maximizar su flujo. net
      puede tener varios fuentes y sumideros. 
      @param[in] leave_residual indica si la red residual requerida
      para los cálculos parciales debe dejarse instanciada o no. Por
      omisión, la red residual, así como el supra grafo en caso de
      que la red tenga varios fuentes o sumideros, son liberados al
      final del cálculo. Si el parámetro es true, entonces la red
      residual queda instanciada y eventualmente utilizable por
      otros algoritmos.
      @return el valor de flujo maximizado de la red
      @throw bad_alloc si no hay suficiente memoria.

      @ingroup Redes
  */
  template <class Net> typename Net::Flow_Type
  fifo_preflow_maximum_flow(Net & net)
  {
    using Node = typename Net::Node;
    return generic_preflow_vertex_push_maximum_flow
      <Net, DynListQueue<Node*>>(net);
  }

  /** Clase invocante a fifo_preflow_maximum_flow()

      @see fifo_preflow_maximum_flow()
      @ingroup Redes
  */
  template <class Net> 
  struct Fifo_Preflow_Maximum_Flow
  {
    /** Invoca a fifo_preflow_maximum_flow().

        @param[in,out] net la red capacitada cuyo flujo se desea
        maximizar. 
        @param[in] leave_residual indica si la red residual requerida
        para los cálculos parciales debe dejarse instanciada o no. Por
        omisión, la red residual, así como el supra grafo en caso de
        que la red tenga varios fuentes o sumideros, son liberados al
        final del cálculo. Si el parámetro es true, entonces la red
        residual queda instanciada y eventualmente utilizable por
        otros algoritmos.
        @return el valor de flujo maximizado de la red
    */
    typename Net::Flow_Type 
    operator () (Net & net) const
    {
      return fifo_preflow_maximum_flow(net);
    }
  };


  template <class Net> 
  struct Compare_Height
  {
    bool operator () (typename Net::Node * n1, typename Net::Node * n2) const
    {
      return node_height<Net>(n1) > node_height<Net>(n2);
    }
  };

  /** Flujo máximo de empuje de preflujo con cola de prioridad según
      la altura de los nodos. 

      heap_preflow_vertex_push_maximum_flow(net) calcula el flujo
      máximo de la red net en donde los nodos activos, aquellos cuyo
      flujo de entrada es mayor que el de salida son almacenados en
      una cola de prioridad. El preflujo se empuja desde el fuente
      hasta el sumidero según una heurística en amplitud.

      heap_preflow_vertex_push_maximum_flow() es uno de los mejores
      algoritmos generales de cálculo de flujo máximo. Su desempeño
      para el peor caso es de \f$O(V^3)\f$.

      Durante la ejecución del algoritmo se emplean los contadores y
      cookies de todos los nodos de la red. El bit Maximum_Flow es
      usado para los arcos.

      @param[in,out] net una red capacitada a maximizar su flujo. net
      puede tener varios fuentes y sumideros. 
      @param[in] leave_residual indica si la red residual requerida
      para los cálculos parciales debe dejarse instanciada o no. Por
      omisión, la red residual, así como el supra grafo en caso de
      que la red tenga varios fuentes o sumideros, son liberados al
      final del cálculo. Si el parámetro es true, entonces la red
      residual queda instanciada y eventualmente utilizable por
      otros algoritmos.
      @return el valor de flujo maximizado de la red
      @throw bad_alloc si no hay suficiente memoria.

      @ingroup Redes
  */
  template <class Net> typename Net::Flow_Type
  heap_preflow_maximum_flow(Net & net)
  {
    using Node = typename Net::Node;
    return generic_preflow_vertex_push_maximum_flow
      <Net, DynBinHeap<Node*, Compare_Height<Net>>>(net);
  }

  /** Clase invocante a heap_preflow_maximum_flow()

      @see heap_preflow_maximum_flow()
      @ingroup Redes
  */
  template <class Net> 
  struct Heap_Preflow_Maximum_Flow
  {
    /** Invoca a heap_preflow_maximum_flow().

        @param[in,out] net la red capacitada cuyo flujo se desea maximizar. 
        @param[in] leave_residual indica si la red residual requerida
        para los cálculos parciales debe dejarse instanciada o no. Por
        omisión, la red residual, así como el supra grafo en caso de
        que la red tenga varios fuentes o sumideros, son liberados al
        final del cálculo. Si el parámetro es true, entonces la red
        residual queda instanciada y eventualmente utilizable por
        otros algoritmos.
        @return el valor de flujo maximizado de la red
    */
    typename Net::Flow_Type operator () (Net & net) const
    {
      return heap_preflow_maximum_flow(net);
    }
  };

  /** Flujo máximo de empuje de preflujo con cola aleatoria de nodos
      activos. 

      random_preflow_vertex_push_maximum_flow(net) calcula el flujo
      máximo de la red net en donde los nodos activos, aquellos cuyo
      flujo de entrada es mayor que el de salida son almacenados en
      una cola aleatoria. 

      Durante la ejecución del algoritmo se emplean los contadores y
      cookies de todos los nodos de la red. El bit Maximum_Flow es
      usado para los arcos.

      @param[in,out] net una red capacitada a maximizar su flujo. net
      puede tener varios fuentes y sumideros. 
      @param[in] leave_residual indica si la red residual requerida
      para los cálculos parciales debe dejarse instanciada o no. Por
      omisión, la red residual, así como el supra grafo en caso de
      que la red tenga varios fuentes o sumideros, son liberados al
      final del cálculo. Si el parámetro es true, entonces la red
      residual queda instanciada y eventualmente utilizable por
      otros algoritmos.
      @return el valor de flujo maximizado de la red
      @throw bad_alloc si no hay suficiente memoria.

      @ingroup Redes
  */
  template <class Net> typename Net::Flow_Type
  random_preflow_maximum_flow(Net & net)
  {
    using Node = typename Net::Node;
    return generic_preflow_vertex_push_maximum_flow<Net, Random_Set<Node*>>(net);
  }


  /** Clase invocante a random_preflow_maximum_flow()

      @see random_preflow_maximum_flow()
      @ingroup Redes
  */
  template <class Net> 
  struct Random_Preflow_Maximum_Flow
  {
    /** Invoca a random_preflow_maximum_flow().

        @param[in,out] net la red capacitada cuyo flujo se desea maximizar. 
        @param[in] leave_residual indica si la red residual requerida
        para los cálculos parciales debe dejarse instanciada o no. Por
        omisión, la red residual, así como el supra grafo en caso de
        que la red tenga varios fuentes o sumideros, son liberados al
        final del cálculo. Si el parámetro es true, entonces la red
        residual queda instanciada y eventualmente utilizable por
        otros algoritmos.
        @return el valor de flujo maximizado de la red
    */
    typename Net::Flow_Type operator () (Net & net) const
    {
      return random_preflow_maximum_flow(net);
    }
  };


  /** Calcula el flujo máximo de una red capacitada y determina el corte
      mínimo.  

      min_cut() recibe una red capacitada cuyo flujo es maximizado y
      luego, a partir del flujo máximo, se calcula el corte mínimo. La
      rutina recibe dos parámetros tipo:
      -# Net: la clase de red, la cual debe ser derivada de la clase
      Net_Graph.
      -# Maxflow: la clase del algoritmo de cálculo de flujo máximo que
      se desea emplear. 

      @param[in,out] net la red a maximizar el flujo y de la cual se
      desea calcular un corte mínimo
      @param[out] vs el conjunto de nodos \f$V_s\f$.
      @param[out] vt el conjunto de nodos \f$V_t\f$.
      @param[out] cuts el conjunto de arcos que van de \f$V_s\f$ hacia
      \f$V_t\f$. Este es el corte.
      @param[out] cutt el conjunto de arcos que van de \f$V_t\f$ hacia
      \f$V_s\f$.
      @param leave_residual leave_residual indica si la red residual requerida
      para los cálculos parciales debe dejarse instanciada o no. Por
      omisión, la red residual, así como el supra grafo en caso de
      que la red tenga varios fuentes o sumideros, son liberados al
      final del cálculo. Si el parámetro es true, entonces la red
      residual queda instanciada y eventualmente utilizable por
      otros algoritmos.
      @return el valor del flujo maximizado (que es igual a la capacidad
      del corte mínimo) 

      @see Edmonds_Karp_Maximum_Flow Random_First_Preflow_Maximum_Flow Priority_First_Preflow_Maximum_Flow Breadth_First_Preflow_Maximum_Flow Depth_First_Preflow_Maximum_Flow Random_Preflow_Maximum_Flow Heap_Preflow_Maximum_Flow  Fifo_Preflow_Maximum_Flow Ford_Fulkerson_Maximum_Flow
      @ingroup  Redes
  */
  template <class Net, template <class> class Maxflow>
  typename Net::Flow_Type min_cut(Net &                             net, 
                                  DynSetTree<typename Net::Node*> & vs, 
                                  DynSetTree<typename Net::Node*> & vt,
                                  DynList<typename Net::Arc *> &    cuts,
                                  DynList<typename Net::Arc *> &    cutt)
  {
    Maxflow <Net> () (net); // calcula flujo máximo

    typename Net::Node * source = net.get_source();

    // Recorre en profundidad la red residual desde source. Todo lo
    // alcanzable pertenece a vs y por tanto es insertado
    (Graph_Traverse_BFS<Net, Net_Iterator<Net>> (net))
      (source, [&vs] (typename Net::Node * p)
               {
                 vs.insert(p); return true;
               });

    // Calcula vt por el complemento
    const size_t size_vt = net.get_num_nodes() - vs.size();
    for (Node_Iterator<Net> it(net); it.has_curr() and 
           vt.size() < size_vt; it.next_ne())
      {
        auto p = it.get_curr();
        if (p->state() == Unprocessed)
          vt.insert(p);
      }     

    for (Arc_Iterator<Net> it(net); it.has_curr(); it.next_ne())
      {
        auto arc = it.get_curr();
        if (arc->flow == 0) // ¿candidato a arco de retroceso?
          if (vt.contains(net.get_src_node(arc)) and // ¿de vt hacia vs?
              vs.contains(net.get_tgt_node(arc)) > 0)
            {
              cutt.insert(arc);
              continue;
            }

        if (arc->flow == arc->cap) // ¿candidato a arco de cruce?
          if (vs.contains(net.get_src_node(arc)) and // ¿de vs hacia vt?
              vt.contains(net.get_tgt_node(arc)) > 0)
            cuts.insert(arc); 
      }

    return net.flow_value();
  }


  /** Clase invocante del algoritmo de cálculo de corte mínimo según el
      flujo máximo. 

      Min_Cut() recibe una red capacitada cuyo flujo es maximizado y
      luego, a partir del flujo máximo, se calcula el corte mínimo. La
      rutina recibe dos parámetros tipo:
      -# Net: la clase de red, la cual debe ser derivada de la clase
      Net_Graph.
      -# Maxflow: la clase del algoritmo de cálculo de flujo máximo que
      se desea emplear. 

      @see min_cut()
      @ingroup  Redes
  */
  template <class Net, 
            template <class> class Maxflow = Heap_Preflow_Maximum_Flow>
  struct Min_Cut
  {
    /** Invoca a min_cut().

        @param[in,out] net la red a maximizar el flujo y de la cual se
        desea calcular un corte mínimo
        @param[out] vs el conjunto de nodos \f$V_s\f$.
        @param[out] vt el conjunto de nodos \f$V_t\f$.
        @param[out] cuts el conjunto de arcos que van de \f$V_s\f$ hacia
        \f$V_t\f$. Este es el corte.
        @param[out] cutt el conjunto de arcos que van de \f$V_t\f$ hacia
        \f$V_s\f$.
        @return el valor del flujo maximizado (que es igual a la capacidad
        del corte mínimo) 
    */
    typename Net::Flow_Type operator () (Net &                             net, 
                                         DynSetTree<typename Net::Node*> & vs, 
                                         DynSetTree<typename Net::Node*> & vt,
                                         DynList<typename Net::Arc *> &    cuts,
                                         DynList<typename Net::Arc *> &    cutt)
    {
      return min_cut <Net, Maxflow> (net, vs, vt, cuts, cutt);
    }
  };


} // end namespace Aleph

# endif // TPL_NET_H
