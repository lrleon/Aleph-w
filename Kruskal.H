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
# ifndef KRUSKAL_H
# define KRUSKAL_H

# include <ahFunction.H>
# include <tpl_agraph.H>
# include <tpl_graph_utils.H>
# include <tpl_test_acyclique.H>
# include <tpl_union.H>

using namespace Aleph;

namespace Aleph {

  
  /** Calcula el árbol abarcador mínimo de un grafo según el
      algoritmo de Kruskal. 

      El algoritmo de Kruskal es el recomendado para grafos
      esparcidos. 

      El procedimiento es parametrizado con las siguientes
      especificaciones:
      -# GT: el tipo de grafo basado en List_Graph.
      -# Distance<GT>: La clase de lectura del peso del arco  que debe
      exportar los siguientes atributos:
        -# typedef Distance<GT>::Distance_Type: el tipo de dato que
	representa un peso en un arco.
	-# Distance<GT>::Distance_Type operator()(typename GT::Arc *a):
	que retorna el valor del peso en el arco a.
	-# Distance<GT>::Max_Distance: constante estática
	correspondiente al valor de distancia máximo que un algoritmo
	consideraría como valor infinito. 
	-# typename Distance<GT>::Zero_Distance: constante estática
	correspondiente al elemento neutro de la suma. Tradicionalmente,
	en la inmensa mayoría de casos, este será el cero.
	.
      -# Compare<GT>: clase que realiza la comparación entre dos
      pesos y cuyo prototipo es:
      \code
      - bool operator () (const typename Distance<GT>::Distance_Type & op1,
                          const typename Distance<GT>::Distance_Type & op2) 
      \endcode
      . Por omisión, esta clase implanta el operador relacional
      menor que. 
      -# SA: filtro de arcos.

      @see Prim_Min_Spanning_Tree
      @ingroup Grafos
  */
 template <class GT, 
	   class Distance = Dft_Dist<GT>, 
	   class SA       = Dft_Show_Arc<GT>> 
class Kruskal_Min_Spanning_Tree 
{
  Distance & dist;
  SA         sa;
  bool       painted;
  
public:

  /** Constructor.

      \param[inout] __dist acceso a la distancia de cada arco
      \param[inout] __cmp comparación entre distancias de arco
      \param[inout] __sa filtro de iterador de arcos
  */
  Kruskal_Min_Spanning_Tree(Distance && __dist = Distance(),  
			    SA             __sa   = SA()) 
  noexcept(std::is_nothrow_move_assignable<Distance>::value and
	   std::is_nothrow_move_assignable<SA>::value)
    : dist(__dist), sa(__sa), painted(false)
  { 
    /* empty */ 
  }

  Kruskal_Min_Spanning_Tree(Distance & __dist, SA __sa = SA()) 
  noexcept(std::is_nothrow_copy_assignable<Distance>::value and
	   std::is_nothrow_copy_assignable<SA>::value)
    : dist(__dist), sa(__sa), painted(false)
  { 
    /* empty */ 
  }

public:

  /// Filtro de arcos pintados por el algoritmo de Kruskal
      template <class G, class GT_SA>
  struct Paint_Filt
  {
    GT_SA & sa;

    Paint_Filt(GT_SA & __sa) 
    noexcept(std::is_nothrow_copy_assignable<SA>::value)
    : sa(__sa) { /* empty */ }

    bool operator () (typename G::Arc * a) const noexcept
    {
      if (not sa(a))
	return false;

      return IS_ARC_VISITED(a, Aleph::Spanning_Tree);
    }
  };
 
private:

  struct Init_Node
  {
    long count;

    Init_Node() noexcept : count(0) { /* empty */ }

    void operator () (const GT &, typename GT::Node * p) noexcept
    {
      NODE_COUNTER(p) = count++;
      NODE_BITS(p).set_bit(Aleph::Spanning_Tree, false);
    }
  };

  static bool arc_is_in_tree(Fixed_Relation & tree, long i, long j) noexcept
  {
    return tree.are_connected(i, j);
  }

public:

  void paint_min_spanning_tree(const GT & g)
  {
    if (g.is_digraph())
      throw std::domain_error("g is a digraph");

    g.reset_bit_arcs(Aleph::Spanning_Tree); // limpiar bits de marcado de arcos
    Operate_On_Nodes<GT, Init_Node> () (g, Init_Node());
    
    typedef Distance_Compare<GT, Distance> DCMP;
    DCMP comp(dist);
    const_cast<GT&>(g).template sort_arcs<DCMP>(comp);
    const size_t V = g.get_num_nodes();

    Fixed_Relation tree(V);

    // Recorrer arcos ordenados de g hasta que numero de arcos de
    // tree sea igual al numero de nodos de g 
    for (Arc_Iterator <GT, SA> it(g, sa); tree.get_num_blocks() > 1 and
	   it.has_curr(); it.next()) 
      {     // siguiente menor arco
	auto arc = it.get_current_arc();
	long i = NODE_COUNTER(g.get_src_node(arc));
	long j = NODE_COUNTER(g.get_tgt_node(arc));
	if (arc_is_in_tree(tree, i, j))
	  continue;
	
	tree.join(i, j);
	ARC_BITS(arc).set_bit(Aleph::Spanning_Tree, true);
      }

    painted = true;
  }

  void paint_min_spanning_tree(const GT & g, GT & tree)
  {
    paint_min_spanning_tree(g);
    clear_graph(tree); // limpia grafo destino 

    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next())
      {
	auto gp = it.get_curr();
	auto tp = tree.insert_node(gp->get_info());
	GT::map_nodes(gp, tp);
      }

    typedef Paint_Filt<GT, SA> F;
    for (Arc_Iterator<GT, F> it(g, F(sa)); it.has_curr(); it.next()) 
      {
	auto ga    = it.get_current_arc();
	auto tsrc = mapped_node<GT>(g.get_src_node(ga));
	auto ttgt = mapped_node<GT>(g.get_tgt_node(ga));
	auto ta   = tree.insert_arc(tsrc, ttgt, ga->get_info());
	GT::map_arcs(ga, ta);
      }
  }

public:

  /** Invoca el cálculo del árbol abarcador según Kruskal.

      @param[in] g el grafo al cual se desea calcular el árbol
      abarcador mínimo.
      @param[out] tree el grafo donde se desea guardar el árbol
      abarcador mínimo resultado. Este grafo es limpiado antes del
      comienzo del algoritmo.
      @throw bad_alloc si no hay suficiente memoria para construir
      tree. En este caso el valor de tree es indeterminado y no está
      limpio.  
   */
  void operator () (const GT & g, GT & tree) 
  {
    paint_min_spanning_tree(g, tree);
  }

  /** Pinta en el grafo el árbol abarcador según Kruskal.

      Al terminar el algoritmo, los arcos de g que pertenecen al árbol
      abarcador están pintados con el bit Kruskal.

      @param[in] g el grafo al cual se desea calcular el árbol
      abarcador mínimo.
      @throw bad_alloc si no hay suficiente memoria para construir
      tree. En este caso el valor de tree es indeterminado y no está
      limpio.  
  */
  void operator () (const GT & g) 
  {
    paint_min_spanning_tree(g);
  }
};
      

} // end namespace Aleph

# endif // KRUSKAL_H
