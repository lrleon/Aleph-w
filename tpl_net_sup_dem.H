
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
#line 620 "reducciones.nw"
# ifndef TPL_NET_SUP_DEM_H
# define TPL_NET_SUP_DEM_H

# include <tpl_netgraph.H>

namespace Aleph {

#line 586 "reducciones.nw"
      /** Nodo con provisión de flujo.

	  @ingroup Grafos Redes
       */
    template <typename Node_Info, typename F_Type = long>
class Net_Sup_Dem_Node : public Net_Node<Node_Info, F_Type>
{
public:
         /// Tipo que representa el flujo 
  typedef F_Type Flow_Type;

  Flow_Type supply_flow; /// Valor de provisión (positivo) o demanda (negativo).
      /// Retorna el valor de provisión del nodo
  Flow_Type & get_supply_flow() { return supply_flow; }

  Net_Sup_Dem_Node() : supply_flow(0) { /* empty */ }

      /// Constructor que recibe información de nodo a guardar. Los valores
      /// de atributos de capacidad y flujo son iniciados en cero.
  Net_Sup_Dem_Node(const Node_Info & node_info) 
    : Net_Node<Node_Info, F_Type>(node_info), supply_flow(0) { /* empty */ }

       /// Constructor copia de nodo. Los valores
       /// de atributos de capacidad y flujo son iniciados en cero.
  Net_Sup_Dem_Node(Net_Sup_Dem_Node * node) 
    : Net_Node<Node_Info, F_Type>(*node), supply_flow(node->supply_flow) { /* empty */ }
};

#line 629 "reducciones.nw"
  /** Red con provisiones de flujo.

      Una red con provisiones de flujo es una en la cual algunos nodos
      tienen un valor de provisión de flujo. Si el valor es positivo,
      significa que el nodo provee un flujo máximo correspondiente al
      valor dado. Si el valor es negativo, significa que el nodo
      requiere recibir por sus arcos de entrada un flujo correspondiente
      al valor absoluto del valor dado.

      En una red de provisiones, se dice que el flujo es factible si
      es posible satisfacer las demandas a partir de las
      provisiones. Para ello, se debe calcular una red capacitada
      equivalente y calcular el flujo máximo.

      @ingroup Grafos Redes
   */
    template <class NodeT = Net_Sup_Dem_Node<Empty_Class, double>, 
	      class ArcT  = Net_Arc<Empty_Class, double> >
class Net_Sup_Dem_Graph : public Net_Graph<NodeT, ArcT>
{
public:
      /// La clase de red tradicional
  typedef Net_Graph<NodeT, ArcT> Net_Class;
      /// Tipo de arco.
  typedef ArcT Arc;
      /// Tipo de node.
  typedef NodeT Node;
      /// Tipo que representa la capacidad y el flujo
  typedef typename Arc::Flow_Type Flow_Type;
      /// Tipo de atributo que almacena un nodo
  typedef typename Node::Node_Type Node_Type;
      /// Tipo de atributo que almacena un arco
  typedef typename Arc::Arc_Type Arc_Type;

  
#line 675 "reducciones.nw"
  /** Crea un nodo con provisión y lo inserta en una red.

      @param[in] node_info valor de atributo del nodo.
      @param[in] supply valor de provisión de flujo. Si el valor es
      positivo, entonces el nodo es proveedor de flujo por el valor
      dado; de lo contrario, el nodo demanda la cantidad dada. Por
      omisión, este parámetro es cero.
      @return puntero a nodo creado e insertado.
      @exception bad_alloc si no hay suficiente memoria.
   */
Node * insert_node(const Node_Type & node_info, const Flow_Type & supply = 0)
{
  Node * p = Net_Class::insert_node(node_info);
  p->supply_flow = supply;
  return p;
}
   /** Crea un nodo con provisión y lo inserta en una red.

       @param[in] supply valor de provisión de flujo. Si el valor es
       positivo, entonces el nodo es proveedor de flujo por el valor
       dado; de lo contrario, el nodo demanda la cantidad dada. Por
       omisión, este parámetro es cero.
       @return puntero a nodo creado e insertado.
       @exception bad_alloc si no hay suficiente memoria.
   */
Node * insert_node(const Flow_Type & supply = 0)
{
  return insert_node(Node_Type(), supply);
}
#line 719 "reducciones.nw"
private:

  Node * super_source;
  Node * super_sink;

public:
  Net_Sup_Dem_Graph() : super_source(nullptr), super_sink(nullptr) { /* empty */ }
      /// Retorna true si la red auxiliar ya ha sido creada.
  bool exist_aux_net() const 
  { 
    return super_source != nullptr or super_sink != nullptr; 
  }

#line 739 "reducciones.nw"
    /// El tipo de red capacitada auxiliar equivalente.
typedef Net_Sup_Dem_Graph Aux_Net;

    /** Calcula la red capacitada equivalente.

	Dada una red con provisiones en los nodos (representada en this),
	compute_aux_net() calcula la red capacitada equivalente sobre
	la cual se pueden realizar cálculos de maximización de flujo,
	corte equivalente y, en particular, verificación de factibilidad
	de flujo.

	La red capacitada equivalente se construye sobre la propia red
	de provisiones. Por esa razón, cualquier cálculo sobre la red
	equivalente es inmediatamente reflejado sobre la red con
	provisiones. 

	@return un puntero a la red equivalente.
	@exception bad_alloc si no hay suficiente memoria para crear la
	red equivalente.
	@exception domain_error si la red equivalente ya ha sido
	calculada.
	@exception domain_error si la red residual de la equivalente ya
	ha sido calculada. 
     */
Net_Sup_Dem_Graph * compute_aux_net()
{
  if (exist_aux_net())
    throw std::domain_error("Auxiliar net is already computed");
  if (this->residual_net)
    throw std::domain_error("Residual net has been already computed");

  super_source = insert_node();   // fuente auxiliar
  super_sink   = insert_node();   // sumidero auxiliar
      // recorrer nodos y atar arcos a fuente o sumidero según signo provisión
  for (Node_Iterator<Net_Sup_Dem_Graph> it(*this); it.has_curr(); it.next_ne())
    {
      Node * p = it.get_curr();
      if (p->supply_flow > 0) // ¿nodo proveedor?
	{
 	  if (p->out_cap < p->supply_flow)
 	    throw std::range_error(gnu::autosprintf("Supply flow in node at %p is "
						    "greater than out capacity", p));
        insert_arc(super_source, p, p->supply_flow);
	}
      else if (p->supply_flow < 0) // ¿nodo demandante?
	{
 	  if (p->in_cap < -p->supply_flow)
 	    throw std::range_error(gnu::autosprintf("Supply flow in node at %p is "
						    "smaller than in capacity", p));
        insert_arc(p, super_sink, -p->supply_flow);
	}
    }
  if (this->get_out_degree(super_source) == 0)
    {
      remove_node(super_source);
      super_source = nullptr;
    }
   if (this->get_in_degree(super_sink) == 0)
    {
      remove_node(super_sink);
      super_sink = nullptr;
    }
  return this;
}
#line 825 "reducciones.nw"
  Net_Sup_Dem_Graph * get_aux_net() { return exist_aux_net() ? this : nullptr; }
#line 833 "reducciones.nw"
    /** Recorre la red de provisiones y determina si el flujo es
	factible. 

	@return true si el flujo es factible; false de lo contrario.
	@exception domain_error si la red capacitada equivalente no ha
	sido previamente calculada.
     */
bool is_feasible() const
{
  if (not exist_aux_net())
    throw std::domain_error("Auxiliar net has not been computed");
  for (Node_Iterator<Net_Sup_Dem_Graph> it(*this); it.has_curr(); it.next_ne())
    {
      Node * p = it.get_curr();
      Flow_Type & supply_flow = p->supply_flow;
      if (supply_flow == 0)
	continue;
      if (supply_flow > 0 and p->out_flow < supply_flow)
	return false;
      else if (supply_flow < 0 and p->in_flow < -supply_flow)
	return true;
    }
  return true;
}

   /** Calcula los nodos cuyo flujo no es factible.

       non_feasible_nodes() recorre los nodos de la red. Por cada nodo,
       según su valor de provisión, se verifica si ésta se satisface. Si
       no es posible satisfacer la provisión, entonces la dirección del
       nodo se guarda en una lista según que el nodo se de demanda o de
       provisión.

       @param[out] supply_list lista de nodos cuyo flujo de salida es
       menor que lo que pueden proveer.
       @param[out] demand_list lista de nodos cuyo flujo de entrada es
       menor que lo que reciben.
       @exception bad_alloc si no hay suficiente memoria para insertar
       en alguna de las listas parámetros.
    */
void non_feasible_nodes(DynDlist<Node *> & supply_list, DynDlist<Node *> & demand_list)
{
  for (Node_Iterator<Net_Sup_Dem_Graph> it(*this); it.has_curr(); it.next_ne())
    {
      Node * p = it.get_curr();
      Flow_Type & supply_flow = p->supply_flow;
      if (supply_flow == 0)
	continue;
      if (supply_flow > 0 and p->out_flow < supply_flow)
	supply_list.append(p);
      else if (supply_flow < 0 and p->in_flow < -supply_flow)
	demand_list.append(p);
    }
}
#line 895 "reducciones.nw"
    /** Ajusta el valor de provisión de un nodo.

	set_supply_flow(p, supply) coloca el valor de provisión supply
	sobre el nodo apuntado por el puntero p.

	@param[in] p puntero al nodo.
	@param[in] supply valor de provisión a colocar sobre el nodo.
	@note una vez realizado cualquier ajuste sobre la provisión, es
	necesario recalcular el flujo máximo si se desea conocer la
	factibilidad del flujo.
	@exception range_error si el valor de provisión no es factible
	a la capacidad entrante o saliente del nodo.
     */
void set_supply_flow(Node * p, const Flow_Type & supply)
{
  if (supply > 0 and p->out_cap < supply)
    throw std::range_error(gnu::autosprintf("Supply flow in node at %p is "
					    "greater than out capacity", p));
  else if (supply < 0 and p->in_cap < -supply)
    throw std::range_error(gnu::autosprintf("Supply flow in node at %p is "
					    "smaller than in capacity", p));
  p->supply_flow = supply;
}
#line 922 "reducciones.nw"
    /// Libera memoria de la red extendida.
void free_aux_net()
{
  if (not exist_aux_net())
    throw std::domain_error("Auxiliar net has not been computed");

  if (this->residual_net)
    {
      this->unmake_residual_net();
      this->unmake_super_nodes();
    } 
  if (super_source != nullptr)
    {
      remove_node(super_source);
      super_source = nullptr;
    }
  if (super_sink != nullptr)
    {
      remove_node(super_sink);
      super_sink = nullptr;
    }
}
#line 664 "reducciones.nw"
};

} // end namespace Aleph

# endif // TPL_NET_SUP_DEM_H
