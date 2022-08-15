
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
# ifndef TPL_MAXFLOW_MINCOST_H
# define TPL_MAXFLOW_MINCOST_H

# include <Bellman_Ford.H>
# include <tpl_net.H>
# include <generate_graph.H>

using namespace Aleph;

namespace Aleph {

template <typename Node_Info = Empty_Class>
using Net_Cost_Node = Net_Node<Node_Info>;

    /** Definición de arco para red de máximo flujo al mínimo coste.

        Net_Cost_Arc modeliza un arco pertenciente a una red de flujo
        con costes asociados en sus arcos.

        La clase maneja dos parámetros tipo:
        -# Arc_Info: el tipo que representa los atributos asociados al
        arco.
        -# F_Type: el tipo de dato con que se representa la capacidad,
        el flujo y el coste. Este tipo debe el mismo que para los
        acumulados de flujo de los nodos.

        @ingroup Grafos Redes
    */                               
    template <typename Arc_Info = Empty_Class, typename F_Type = double>
struct Net_Cost_Arc : public Net_Arc<Arc_Info, F_Type>
{ 
  using Base = Net_Arc<Arc_Info, F_Type>;

  using Base::Base; // hereda constructores

       /// Tipo que representa el flujo 
  using Flow_Type = F_Type;

      /// coste por unidad de flujo (negativo si el arco es residual)
  Flow_Type cost = 0;

      /// Retorna el coste del flujo circulante
  Flow_Type flow_cost() const noexcept { return this->flow*cost; }

  Net_Cost_Arc(const Net_Cost_Arc & a)
    : Base(a), cost(a->cost) {}

  Net_Cost_Arc & operator = (const Net_Cost_Arc & a) 
  {
    if (&a == this)
      return *this;
    *(Base*) this = a;
    cost = a.cost;
    return *this;
  }
};

  /** Definición de red capacitada de flujo con costes asociados a los
      flujos.  

      Este tipo, derivado de Net_Graph, modeliza una red capacitada en
      la que a cada arco se le define un coste por unidad de flujo. Este
      modelo, extensión del de red capacitada, amplía considerablemente
      la gama de problemas de optimización combinatoria al plantear una
      optimización tipo max-min. En este caso, el máximo flujo a coste
      mínimo.

      Siendo derivación de Net_Graph, su interfaz es muy similar, con la
      añadidura de parámetros y métodos para manejar el coste de cada
      arco.

      @see max_flow_min_cost_by_cycle_canceling Max_Flow_Min_Cost

      @ingroup Redes
   */
  template <class NodeT = Net_Cost_Node<Empty_Class>, 
	    class ArcT = Net_Cost_Arc<Empty_Class, double>>
struct Net_Cost_Graph : public Net_Graph<NodeT, ArcT>
{
  using Base = Net_Graph<NodeT, ArcT>;

  Net_Cost_Graph(const Net_Cost_Graph & net) : Base(net)
  {
    zip(this->arcs(), net.arcs()).for_each([] (const auto & p)
      {
  	auto atgt = p.first;
  	auto asrc = p.second;
  	atgt->cost = asrc->cost;
      });
  }

  Net_Cost_Graph() {}

      /// El tipo de red de flujo sobre el cual se sustenta este modelo
      /// con coste  
  using Net = Net_Graph<NodeT, ArcT>;

  using Net_MFMC = Net_Cost_Graph<NodeT, ArcT>;

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

      /// Retorna una referencia modificable al coste de un arco.
  Flow_Type & get_cost(Arc * a) noexcept { return a->cost; }

      /// Retorna una referencia al coste de un arco:
  const Flow_Type & flow_cost(Arc * a) const noexcept { return a->flow_cost(); }

  /** Crea e inserta un arco en una red de flujo con costes. El arco
      creado tien valor de flujo cero. 

      @param[in] src_node nodo origen
      @param[in] tgt_node nodo destino
      @param[in] cap valor de capacidad del arco
      @param[in] cost valor de coste por unidad de flujo del arco
      @return puntero al nuevo arco
      @throw bad_alloc si no hay suficiente memoria
   */
  virtual Arc * insert_arc(Node * src_node, Node * tgt_node,
			   const Flow_Type & cap, const Flow_Type & cost)
  {
    Arc * a = Net::insert_arc(src_node, tgt_node, cap, 0, Arc_Type());
    a->cost = cost;
    return a;
  }

  template <typename ... Args>
  Arc * emplace_arc(Node * src_node, Node * tgt_node,
		    const Flow_Type & cap, const Flow_Type & cost, 
		    Args && ... args)
  {
    auto a = Net::insert_arc(src_node, tgt_node, cap, cost, Arc_Type(args...));
    a->cost = cost;
    return a;
  }

   /// Usado por algorítmica interna. NO UTILIZAR
  virtual Arc * insert_arc(Node * src_node, Node * tgt_node)
  {
    Arc * a = Net::insert_arc(src_node, tgt_node, Arc_Type());
    a->cost = 0;
    return a;
  }

   /// Usado por algorítmica interna. NO UTILIZAR
  // virtual Arc * insert_arc(Node * src_node, Node * tgt_node, 
  // 			   const Arc_Type & arc_info)
  // {
  //   Arc * a = Net::insert_arc(src_node, tgt_node, arc_info);
  //   a->cost = 0;
  //   return a;
  // }

  /// Calcula el coste total del flujo circulante por la red
  Flow_Type flow_cost() const
  {
    Flow_Type total = 0;
    for (Arc_Iterator<Net_MFMC> it(*this); it.has_curr(); it.next_ne())
      {
	Arc * a = it.get_curr();
	total += a->flow_cost();
      }

    return total;
  }

  /// retorna una tripleta de capacidad, flujo y costo saliente
  auto out_pars(Node * p)
  {
    Flow_Type cap_sum = 0, flow_sum = 0, cost_sum = 0;
    for (__Out_Iterator<Net> it(p); it.has_curr(); it.next_ne())
      {
	Arc * a = it.get_curr();
	cap_sum += a->cap;
	flow_sum += a->flow;
	cost_sum += a->cost;
      }
    return make_tuple(cap_sum, flow_sum, cost_sum);
  }

    /// retorna una tripleta de capacidad, flujo y costo saliente
  auto in_pars(Node * p)
  {
    Flow_Type cap_sum = 0, flow_sum = 0, cost_sum = 0;
    for (__In_Iterator<Net> it(p); it.has_curr(); it.next_ne())
      {
	Arc * a = it.get_curr();
	cap_sum += a->cap;
	flow_sum += a->flow;
	cost_sum += a->cost;
      }
    return make_tuple(cap_sum, flow_sum, cost_sum);
  }
};


// Filtro de arcos para una red residual
    template <class Net>
struct Res_Filt
{
  Res_Filt(typename Net::Node *) noexcept {}

  Res_Filt() noexcept {}

  bool operator () (typename Net::Arc * a) const noexcept
  {
    return a->cap > a->flow;
  }
};

    template <class Net> 
struct Rcost
{
  Rcost () noexcept {}

  using Distance_Type = typename Net::Flow_Type;
  
  // Este es el operador que se llamaría cuando se ejecuta Distance()(a)
  typename Net::Flow_Type operator () (typename Net::Arc * a) const noexcept
  {
    return a->cost;
  }

  static void set_zero(typename Net::Arc * a) noexcept
  { 
    a->cap = numeric_limits<Distance_Type>::max();
    a->flow = 0;
    a->cost = 0; 
  }
};

    
    template <typename Ftype>
struct Res_Arc : public Net_Cost_Arc<Empty_Class, Ftype>
{
  using Base = Net_Cost_Arc<Empty_Class, Ftype>;
  using Base::Base;

  bool is_residual = false;
  Res_Arc * img = nullptr;
};


template <typename Ftype>
using Residual_Net = Net_Cost_Graph<Net_Node<Empty_Class>, Res_Arc<Ftype>>;

// Inserta en la red residual residual_net los arcos residuales
// correspondientes al arco a. src y tgt son los nodos origen y destino
// respectivamente de la red residual. Retorna el arco residual reflejo
// de a
template <class Res_Net> typename Res_Net::Arc * 
create_residual_arc(Res_Net & residual_net, 
		    typename Res_Net::Node * src,
		    typename Res_Net::Node * tgt,
		    const typename Res_Net::Flow_Type cap,
		    const typename Res_Net::Flow_Type flow,
		    const typename Res_Net::Flow_Type cost)
{
  assert(flow <= cap and cost >= 0);

  auto arc = residual_net.insert_arc(src, tgt, cap, cost);
  auto rarc = residual_net.insert_arc(tgt, src, cap, -cost);

  arc->is_residual = false;
  arc->flow = flow;
  arc->img = rarc;

  rarc->is_residual = true;
  rarc->img = arc;
  rarc->flow = arc->cap - arc->flow;

  assert(arc->cap == cap and arc->flow == flow and arc->cost == cost);
  assert(rarc->cap == cap and rarc->flow == cap - flow and rarc->cost == -cost);

  return arc;
}

// Construye una red residual para la red net. Retorna una tupla de dos
// elementos. El primer elemento es la red residual. El segundo es un
// mapeo desde los arcos de net hacia los arcos de los arcos de la red
// residual. Retorna el nodo mapeo del fuente en la red residual
template <class Net>
typename Residual_Net<typename Net::Flow_Type>::Node * 
build_residual_net(const Net & net, 
		   Residual_Net<typename Net::Flow_Type> & rnet,
		   DynMapTree<void*, void*> & arcs)
{
  if (not (net.is_single_source() and net.is_single_sink()))
    throw std::domain_error("Network is not single source and single sink");

  using Rnet = Residual_Net<typename Net::Flow_Type>;

      // recorre los nodos de net e inserta copias en la red residual
  for (typename Net::Node_Iterator it(net); it.has_curr(); it.next_ne())
    {
      auto p = it.get_curr();
      auto q = rnet.insert_node(); // TODO: borrar get_info
      map_nodes<Net, Rnet>(p, q); // mapeo temporal de nodos
    }

    // Recorre arcos de net y construye mapeo arcs
  for (typename Net::Arc_Iterator it(net); it.has_curr(); it.next_ne())
    {
      auto ga = it.get_curr();
      auto gsrc = (typename Net::Node*) ga->src_node;
      auto gtgt = (typename Net::Node*) ga->tgt_node;

      auto rsrc = mapped_node<Net, Rnet>(gsrc);
      auto rtgt = mapped_node<Net, Rnet>(gtgt);
      auto ra = 
	create_residual_arc(rnet, rsrc, rtgt, ga->cap, ga->flow, ga->cost);
      arcs.insert(ga, ra);
    }

  assert(check_residual_net(rnet));

  return (typename Rnet::Node*) NODE_COOKIE(net.get_source());
} 

template <class Res_Net>
bool check_residual_net(const Res_Net & net)
{
  return
    net.all_arcs([] (typename Res_Net::Arc * a) { return a->img->img == a; });
}

template <class Res_Net>
void cancel_cycle(const Res_Net &, const Path<Res_Net> & path)
{
  assert(not path.is_empty() and path.is_cycle());
  using Ftype = typename Res_Net::Flow_Type;

  // Determina el slack del ciclo
  Ftype slack = std::numeric_limits<Ftype>::max();
  path.for_each_arc([&slack] (typename Res_Net::Arc * a)
    {
      assert(a->cap -a->flow > 0);
      slack = std::min(slack, a->cap - a->flow);
    });

  // cancela el ciclo
  path.for_each_arc([slack] (typename Res_Net::Arc * a)
    {
      auto img = a->img;
      assert(img->img == a);
      assert(a->cap == img->cap);
      a->flow += slack;
      img->flow -= slack;
    });
}

// recibe un mapeo de arcos desde la red residual hacia la red
// original. Recorre los arcos y actualiza los valores de flujo en la
// red original
template <class Net> static 
void residual_to_net(const DynMapTree<void*, void*> & arcs)
{
  using Rnet = Residual_Net<typename Net::Flow_Type>;
  arcs.for_each([] (std::pair<void*, void*> p)
		 {
		   auto a = (typename Rnet::Arc *) p.first;
		   auto ra = (typename Net::Arc *) p.second;
		   a->flow = ra->flow;
		 });
}

    /** Calcula el flujo máximo a coste mínimo mediante la técnica de
        cancelación de ciclos. 

        Esta primitiva emplea el típico algoritmo de cancelación de
        ciclos para calcular el flujo máximo a coste mínimo de una red
        capacitada de flujo.

        Primero, la rutina calcula un flujo máximo mediante un algoritmo
        de maximización de flujo. El algoritmo es un parámetro tipo,
        llamado Max_Flow_Algo, instanciado sobre una red de tipo
        genérico Net. Una vez calculado un flujo máximo inicial, el
        algoritmo busca ciclos negativos sobre la red residual mediante
        el algoritmo de Bellman-Ford. Por cada ciclo negativo encontrado, se
        cancela el ciclo por aumento del flujo por el ciclo y así se elimina. 

        El procedimiento anterior se repite hasta que ya no se
        encuentren más ciclos negativos. Momento en el cual el flujo es
        máximo y el coste mínimo.

        @param[in] net la red a maximizar flujo y minimizar coste.
	@param[in] it_factor factor de iteraciones en el cual el
	algoritmo de Bellman-Ford comenzará a buscar ciclos negativos
        @throw bad_alloc si no hay suficiente memoria.
	@return Una dupla con la cantidad de ciclos encontrados y el
	factor final de iteraciones 

       @ingroup Redes
     */
template 
<class Net, 
 template <class> class Max_Flow_Algo = Ford_Fulkerson_Maximum_Flow>
tuple<size_t, double>
max_flow_min_cost_by_cycle_canceling(Net & net, 
				     double it_factor = 0.4,
				     size_t step = 10)
{ 
  Max_Flow_Algo <Net> () (net); // compute max flow

  using Rnet = Residual_Net<typename Net::Flow_Type>;
  using BF = 
    Bellman_Ford<Rnet, Rcost<Rnet>, Arc_Iterator, Out_Iterator, Res_Filt<Rnet>>;

  Rnet rnet;
  DynMapTree<void*, void*> arcs_map;
  typename Rnet::Node * source = build_residual_net(net, rnet, arcs_map);

  size_t count = 0;

  while (true)
    { search: 
      tuple<Path<Rnet>, size_t> cycle =
	BF(rnet).search_negative_cycle(source, it_factor, step); 
      if (get<0>(cycle).is_empty())
	break;

          // augment it_factor according to last break counter
      it_factor = ((double) get<1>(cycle))/net.vsize();
      cancel_cycle(rnet, get<0>(cycle));
    }
  
  tuple<Path<Rnet>, size_t> cycle = 
    BF(rnet).search_negative_cycle(it_factor, step); 
  if (not get<0>(cycle).is_empty()) // negative cycle found?
    {
      cancel_cycle(rnet, get<0>(cycle));    
      goto search;
    }

  residual_to_net<Net>(arcs_map);

  return make_tuple(count, it_factor);
}


template <class Net, 
	  template <class> class Max_Flow_Algo = Ford_Fulkerson_Maximum_Flow>
struct Max_Flow_Min_Cost_By_Cycle_Canceling
{
  tuple<size_t, double> operator () (Net & net, 
				     double it_factor = 0.4, 
				     size_t step = 10)
  {
    return max_flow_min_cost_by_cycle_canceling<Net, Max_Flow_Algo>
      (net, it_factor, step);
  }
};

template <class Net>
void print(const Net & net, ostream & out)
{
  long i = 0;
  net.nodes().for_each([&i] (typename Net::Node* p) { NODE_COUNTER(p) = i++; });

  struct Show_Node
  {
    void operator () (const Net &, typename Net::Node * p, ostream & out)
    {
      out << "label = \"(" << p->get_info() << "," << NODE_COUNTER(p) << ")\"";
    }
  };

  struct Show_Arc
  {
    void operator () (const Net &, typename Net::Arc * a, ostream & out)
    {
      out << "label = \"" << a->flow << "/" << a->cap << "/" << a->cost << "\"";
    }
  };

  To_Graphviz<Net, Show_Node, Show_Arc> ().digraph (net, out);
}

template <class Net>
void print(const Residual_Net<typename Net::Flow_Type> & net, ostream & out)
{
  long i = 0;
  net.nodes().for_each([&i] (typename Net::Node* p) { NODE_COUNTER(p) = i++; });

  struct Show_Node
  {
    void operator () (const Net &, typename Net::Node * p, ostream & out)
    {
      out << "label = \"(" << p->get_info() << "," << NODE_COUNTER(p) << ")\"";
    }
  };

  struct Show_Arc
  {
    void operator () (const Net &, typename Net::Arc * a, ostream & out)
    {
      out << "label = \"" << a->flow << "/" << a->cap << "/" << a->cost << "\"";
      if (a->is_residual)
	out << " color = red";
    }
  };

  To_Graphviz<Net, Show_Node, Show_Arc, Dft_Show_Node<Net>, Res_Filt<Net>>().
    digraph (net, out);
}
  

    template <class Net>
struct SimplexInfo
{
  typename Net::Flow_Type potential;
  long valid = 0;
};

template <class Net> static inline
void init_simplex_info(Net & /* net */)
{

}


// 1er campo arcos vacíos, 2do arcos llenos, 3ro arcos parciales
  template <class Net>
using Feasible_Tree = tuple<DynList<typename Net::Arc*>, 
			    DynList<typename Net::Arc*>, 
			    DynList<typename Net::Arc*>>;
								

template <class Net> static inline
Feasible_Tree<Net> build_feasible_spanning_tree(const Net & net)
{
  using Arc = typename Net::Arc;
  DynList<Arc*> empty, full, partial;
  for (typename Net::Arc_Iterator it(net); it.has_curr(); it.next_ne())
    {
      auto a = it.get_curr();
      if (a->flow == 0) // arco vacío?
	empty.append(a);
      else if (a->flow == a->cap) // arco lleno?
	full.append(a);
      else
	partial.append(a);
    }
  return make_tuple(std::move(empty), std::move(full), std::move(partial));
}


template <class Net> static inline 
DynList<typename Net::Arc*> get_partial_arcs(const Net & net)
{
  DynList<typename Net::Arc*> ret;
  for (typename Net::Arc_Iterator it(net); it.has_curr(); it.next_ne())
    {
      auto a = it.get_curr();
      if (a->flow > 0 and a->flow < a->cap)
	ret.append(a);
    }
  return ret;
}


# define POTENTIAL(p) (NODE_COUNTER(p))


template <class Net>
typename Net::Flow_Type rcost(typename Net::Node * src, 
			      typename Net::Node * tgt)
{
  for (Out_Iterator<Net> it(src); it.has_curr(); it.next_ne())
    {
      auto a = it.get_curr();
      if (a->tgt_node == tgt)
	return a->cost - (POTENTIAL(src) - POTENTIAL(tgt));
    }
  AH_ERROR("Arc not found");
}


template <class Net>
typename Net::Flow_Type rcost(typename Net::Arc * a)
{
  using Node = typename Net::Node*;
  return a->cost - 
  (POTENTIAL((Node*)a->src_node) - POTENTIAL((Node*)a->tgt_node));
}




# undef POTENTIAL

} // end namespace Aleph

# endif // TPL_MAXFLOW_MINCOST_H
