
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
# ifndef TPL_BIPARTITE_H
# define TPL_BIPARTITE_H

# include <tpl_dynDlist.H>
# include <tpl_net.H>

namespace Aleph {

enum Bipartite_Color { Bp_White, Bp_Red, Bp_Blue };

    template <class GT> 
static long & color(typename GT::Node * p)
{
  return NODE_COUNTER(p);
}

    template <class GT> 
static long & color(typename GT::Arc * a)
{
  return ARC_COUNTER(a);
}

    /** Toma un grafo bipartido y calcula los conjuntos de partición.
	
	Un grafo es bipartido si puede dividirse en dos subconjuntos l y
	r tal que todo nodo de l sólo tiene arcos hacia nodos de r y
	viceversa. 
       
	compute_bipartite(g,l,r) toma un grafo bipartido g y calcula los
	conjuntos de bipartición nombrados por los parámetros l y r,
	respectivamente. 

	@param[in] g el grafo bipartido.
	@param[out] l un conjunto partición.
	@param[out] r un conjunto partición.
	@exception domain_error si durante el cálculo se determina que
	el grafo no es bipartido.
	@exception bad_alloc si no hay suficiente memoria.
	@ingroup Grafos 
     */
     template <class GT, class SA = Dft_Show_Arc<GT> > 
void compute_bipartite(const GT &                      g, 
		       DynDlist<typename GT::Node *> & l,
		       DynDlist<typename GT::Node *> & r)
{
  g.reset_nodes(); // inicializa contadores en Blanco
  g.reset_arcs();

  DynDlist<typename GT::Node *> red, blue;
  typename GT::Node * p = g.get_first_node();
  color<GT>(p) = Bp_Red;
  red.put(p); l.put(p);  

  while (true)
    if (not red.is_empty()) // ¿Hay rojo con arcos no mirados?
      {
	typename GT::Node * p = red.get();
	for (Node_Arc_Iterator<GT, SA> it(p); it.has_curr(); it.next_ne())
	  {
 	    typename GT::Arc * a = it.get_curr();
 	    if (color<GT>(a) == Bp_Red)
 	      throw std::domain_error("Graph is not bipartite");
 	    else if (color<GT>(a) == Bp_Blue)
 	      continue;

 	    color<GT>(a) = Bp_Red; 
	    typename GT::Node * q = it.get_tgt_node();
	    if (color<GT>(q) == Bp_Red)
	      throw std::domain_error("Graph is not bipartite");	      
	    else if (color<GT>(q) == Bp_Blue)
	      continue;

	    color<GT>(q) = Bp_Blue;
	    blue.put(q); r.put(q);
	  }
      }
    else if (not blue.is_empty()) // ¿Hay azul con arcos no mirados?
      {
	typename GT::Node * p = blue.get();
	for (Node_Arc_Iterator<GT, SA> it(p); it.has_curr(); it.next_ne())
	  {
 	    typename GT::Arc * a = it.get_curr();
 	    if (color<GT>(a) == Bp_Blue)
 	      throw std::domain_error("Graph is not bipartite");
 	    else if (color<GT>(a) == Bp_Red)
 	      continue;

 	    color<GT>(a) = Bp_Blue;

	    typename GT::Node * q = it.get_tgt_node();
	    if (color<GT>(q) == Bp_Blue)
	      throw std::domain_error("Graph is not bipartite");
	    else if (color<GT>(q) == Bp_Red)
	      continue;

	    color<GT>(q) = Bp_Red;
	    red.put(q); l.put(q);
	  }
      }
    else
      break;
}

   /** Clase que toma un grafo bipartido y calcula los conjuntos de partición.
	
	Un grafo es bipartido si puede dividirse en dos subconjuntos l y
	r tal que todo nodo de l sólo tiene arcos hacia nodos de r y
	viceversa. 
       
	@ingroup Grafos 
     */
    template <class GT, class SA = Dft_Show_Arc<GT> > 
class Compute_Bipartite
{
public:
  
     /** Calcula los conjuntos de partición de un grafo bipartido.
	
	@param[in] g el grafo bipartido.
	@param[out] l un conjunto partición.
	@param[out] r un conjunto partición.
	@exception domain_error si durante el cálculo se determina que
	el grafo no es bipartido.
	@exception bad_alloc si no hay suficiente memoria.
     */
  void operator () (const GT &                      g, 
		    DynDlist<typename GT::Node *> & l,
		    DynDlist<typename GT::Node *> & r)
  {
    compute_bipartite <GT, SA> (g, l, r);
  }
};

     /** Calcula el emparejamiento de cardinalidad máxima de un grafo
	 bipartido. 

	 compute_maximum_cardinality_bipartite_matching(g,matching)
	 recibe un grafo bipartido g y calcula el máximo emparejamiento
	 bipartido en la lista matching.

	 El procedimiento calcula los conjuntos de bipartición, luego
	 construye una red capacitada unitaria equivalente y sobre ella
	 invoca a un algoritmo de maximización de flujo.

	 La rutina maneja dos parámetros tipo:
	 -# GT el tipo de grafo bipartido
	 -# Max_Flow el algoritmo de maximización de flujo a emplear
	 para realizar el cálculo
	 .

	 @param[in] g el grafo bipartido.
	 @param[out] matching lista de arcos componentes del
	 emparejamiento.
	 @exception bad_alloc si no hay suficiente memoria.

	 @ingroup Grafos 
      */
     template <class GT, 
	       template <class> class Max_Flow = Ford_Fulkerson_Maximum_Flow, 
	       class SA = Dft_Show_Arc<GT>> 
void compute_maximum_cardinality_bipartite_matching
  (const GT & g, DynDlist<typename GT::Arc *> & matching)
{
  DynDlist<typename GT::Node *> l, r;

  compute_bipartite(g, l, r);

  typedef Net_Graph<Net_Node<Empty_Class>, Net_Arc<Empty_Class> > AN;
  AN net;

      // recorrer nodos de g e insertar imagen en net
  for (Node_Iterator<GT> it(g); it.has_curr(); it.next_ne())
  {
    typename GT::Node * p = it.get_curr();
    NODE_COOKIE(p) = net.insert_node();
    NODE_COOKIE((typename GT::Node *)NODE_COOKIE(p)) = p;
  }

  typename AN::Node * source = net.insert_node();

      // recorrer nodos de l, atarlos a fuente e insertar sus arcos
  for (typename DynDlist<typename GT::Node*>::Iterator i(l); 
       i.has_curr(); i.next_ne())
    {
      typename GT::Node * p = i.get_curr();
      typename AN::Node * src = mapped_node<GT, AN> (p);
      net.insert_arc(source, src, 1);

      for (Node_Arc_Iterator<GT, SA> j(p); j.has_curr(); j.next_ne())
	{
	  typename GT::Arc * arc = j.get_current_arc_ne();
	  typename AN::Node * tgt = mapped_node <GT, AN> (g.get_tgt_node(arc));
	  typename AN::Arc * a = net.insert_arc(src, tgt, 1);
	  ARC_COOKIE(arc) = a;
	  ARC_COOKIE(a) = arc;
	}
    }

  typename AN::Node * sink = net.insert_node();

    // recorrer nodos de r y atarlos al sumidero
  for (typename DynDlist<typename GT::Node*>::Iterator it(r); 
       it.has_curr(); it.next_ne())
    {
      typename GT::Node * p = it.get_curr();
      net.insert_arc(mapped_node<GT, AN> (p), sink, 1);
    }

  Max_Flow <AN> () (net); 

  for (Arc_Iterator<AN> it(net); it.has_curr(); it.next_ne())
    {
      typename AN::Arc * a = it.get_curr();
      if (a->flow == 0) 
	continue;

      typename GT::Arc * arc = mapped_arc <AN, GT> (a);
      if (arc == nullptr)
	continue;

      matching.append(arc);
    }
}

     /** Clase de cálculo del emparejamiento de cardinalidad máxima de un grafo
	 bipartido. 

	 La clase maneja dos parámetros tipo:
	 -# GT el tipo de grafo bipartido
	 -# Max_Flow el algoritmo de maximización de flujo a emplear
	 para realizar el cálculo. Por omisión se emplea el algoritmo de
	 Ford_Fulkerson
	 .

	 @ingroup Grafos 
      */
     template <class GT, 
	       template <class> class Max_Flow = Ford_Fulkerson_Maximum_Flow,
	       class SA = Dft_Show_Arc<GT> > 
class Compute_Maximum_Cardinality_Bipartite_Matching
{
public:

  /** Calcula el máximo emparejamiento bipartido de un grafo.

      compute_maximum_cardinality_bipartite_matching(g,matching)
      recibe un grafo bipartido g y calcula el máximo emparejamiento
      bipartido en la lista matching.

      El procedimiento calcula los conjuntos de bipartición, luego
      construye una red capacitada unitaria equivalente y sobre ella
      invoca a un algoritmo de maximización de flujo.

      @param[in] g el grafo bipartido.
      @param[out] matching lista de arcos componentes del
      emparejamiento.
      @exception bad_alloc si no hay suficiente memoria.
   */
  void operator () (const GT & g, DynDlist<typename GT::Arc *> & matching)
  {
    compute_maximum_cardinality_bipartite_matching <GT, Max_Flow, SA>
      (g, matching);
  }
};


} // end namespace Aleph

# endif //  TPL_BIPARTITE_H
