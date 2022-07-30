
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
# ifndef TARJAN_H
# define TARJAN_H

# include <tpl_dynListStack.H>
# include <tpl_dynSetTree.H>
# include <htlist.H>
# include <tpl_graph_utils.H>
# include <tpl_find_path.H>

namespace Aleph {


  /** Operaciones sobre componentes fuertemente conexos de un digrafo
      empleando el algoritmo de Tarjan para el cálculo de los
      componentes fuertemente conexos de un grafo.

      La clase emplea dos parámetros tipo:
      -# GT: la clase digrafo.
      -# SA: el filtro de arcos que emplea el iterador interno.

      @ingroup Grafos
  */
  template <class GT,
            template <typename, class> class Itor = Out_Iterator,
            class SA = Dft_Show_Arc<GT>>
  class Tarjan_Connected_Components
  {
    SA sa;

    GT * g_ptr = nullptr;

    DynListStack<typename GT::Node*> stack;

    long df_count = 0;
    mutable size_t n = 0; // número de nodos del grafo

    // lista de listas; cada lista almacena los nodos de los bloques
    DynList<DynList<typename GT::Node*>> * list_list_ptr = nullptr;

    DynList <GT> * block_list_ptr = nullptr; // lista de bloques fuertemente conexos

    DynList<size_t> * list_len_ptr = nullptr; // lista de tamaños de componentes

    Path<GT> * path_ptr = nullptr;

  public:

    Tarjan_Connected_Components(SA  __sa = SA())
      : sa(__sa) { /* empty */ }

  private:

    struct Init_Tarjan_Node
    {
      void operator () (const GT & g, typename GT::Node * p) const noexcept
      {
        g.reset_bits(p);
        g.reset_counter(p); // inicializa df
        low <GT> (p) = -1;  // inicializa low
      }
    };

    bool is_node_in_stack(typename GT::Node * p) const noexcept
    {
      return IS_NODE_VISITED(p, Aleph::Min);
    }

    void init_node_and_push_in_stack(typename GT::Node * p)
    {
      assert(not is_node_in_stack(p));

      stack.push(p);
      NODE_BITS(p).set_bit(Aleph::Min, true);
      NODE_BITS(p).set_bit(Aleph::Depth_First, true);
      df<GT>(p) = low<GT>(p) = df_count++;
    }

    typename GT::Node * pop_from_stack()
    {
      auto ret = stack.pop();
      NODE_BITS(ret).set_bit(Aleph::Min, false);

      return ret;
    }

    void scc_by_blocks(typename GT::Node * v)
    {
      init_node_and_push_in_stack(v);

      // recorrer en profundidad todos los nodos conectados a v
      for (Itor<GT, SA> it(v, sa); it.has_curr(); it.next_ne())
        {
          auto w = g_ptr->get_tgt_node(it.get_curr());
          if (not IS_NODE_VISITED(w, Aleph::Depth_First))
            {
              scc_by_blocks(w);
              low <GT> (v) = std::min(low <GT> (v), low <GT> (w));
            }
          else if (is_node_in_stack(w))
            // si está en pila ==> v fue visitado antes que p
            low <GT> (v) = std::min(low <GT> (v), df <GT> (w));
        }

      if (low <GT> (v) == df <GT> (v)) // ¿primer nodo visitado del bloque?
        {	    // sí ==> saque los nodos del bloque que están en pila
          const auto & blk_idx = block_list_ptr->size();
          GT & blk = block_list_ptr->append(GT());

          while (true) // sacar el bloque de la pila hasta sacar a v
            {
              auto p = pop_from_stack();
              auto q = blk.insert_node(p->get_info());
              *q = *q; // copia contenido del nodo
              NODE_COOKIE(p) = NODE_COOKIE(q) = nullptr;
              GT::map_nodes(p, q);
              NODE_COUNTER(p) = NODE_COUNTER(q) = blk_idx;
              if (p == v)
                break;
            }
        }
    }

    void scc_by_lists(typename GT::Node * v)
    {
      init_node_and_push_in_stack(v);

      // recorrer en profundidad todos los nodos conectados a v
      for (Itor<GT, SA> it(v, sa); it.has_curr(); it.next_ne())
        {
          auto w = g_ptr->get_tgt_node(it.get_curr());
          if (not IS_NODE_VISITED(w, Aleph::Depth_First))
            {
              scc_by_lists(w);
              low <GT> (v) = std::min(low <GT> (v), low <GT> (w));
            }
          else if (is_node_in_stack(w))
            // si está en pila ==> v fue visitado antes que p
            low <GT> (v) = std::min(low <GT> (v), df <GT> (w));
        }

      if (low<GT>(v) == df<GT>(v)) // ¿primer nodo visitado del bloque?
        {     // sí ==> saque los nodos del bloque que están en pila
          auto & l = list_list_ptr->append(DynList<typename GT::Node*>());
          while (true) // sacar bloque de pila hasta llegar a v
            {
              auto p = pop_from_stack();
              l.append(p);
              if (p == v)
                break;
            }
        }
    }

    void scc_by_len(typename GT::Node * v)
    {
      init_node_and_push_in_stack(v);

      // recorrer en profundidad todos los nodos conectados a v
      for (Itor<GT, SA> it(v, sa); it.has_curr(); it.next_ne())
        {
          auto w = g_ptr->get_tgt_node(it.get_curr());
          if (not IS_NODE_VISITED(w, Aleph::Depth_First))
            {
              scc_by_len(w);
              low <GT> (v) = std::min(low <GT> (v), low <GT> (w));
            }
          else if (is_node_in_stack(w))
            // si está en pila ==> v fue visitado antes que p
            low <GT> (v) = std::min(low <GT> (v), df <GT> (w));
        }

      if (low<GT>(v) == df<GT>(v)) // ¿primer nodo visitado del bloque?
        {     // sí ==> saque los nodos del bloque que están en pila
          size_t count = 0;
          while (true) // sacar bloque de pila hasta llegar a v
            {
              auto p = pop_from_stack();
              ++count;

              if (p == v)
                break;
            }
          list_len_ptr->append(count);
        }
    }

    void init_tarjan(const GT & g)
    {
      Operate_On_Nodes <GT, Init_Tarjan_Node> () (g); // inicia bits, df y low
      df_count = 0; // contador de visitas
      stack.empty();
      n = g.get_num_nodes();
      g_ptr = &const_cast<GT&>(g);
    }

    // retorna true si se encuentra un ciclo desde v
    bool has_cycle(typename GT::Node * v)
    {
      init_node_and_push_in_stack(v);

      // recorrer en profundidad todos los nodos conectados a v
      for (Itor <GT, SA> it(v, sa); it.has_curr(); it.next_ne())
        {
          auto w = g_ptr->get_tgt_node(it.get_curr());
          if (not IS_NODE_VISITED(w, Aleph::Depth_First))
            {
              if (has_cycle(w))
                return true;

              low<GT>(v) = std::min(low<GT>(v), low<GT>(w));
            }
          else if (is_node_in_stack(w))
            // si está en pila ==> v fue visitado antes que p
            low<GT>(v) = std::min(low<GT>(v), df<GT>(w));
        }

      if (low<GT>(v) == df<GT>(v)) // ¿primer nodo visitado de bloque?
        {     // sí ==> verifique si tiene dos o más nodos
          int i = 0;
          for (; i < 2; ++i) // sacar bloque de la pila hasta sacar a v
            if (pop_from_stack() == v)
              break;

          return i >= 2; // si i>= 2 ==> hay un ciclo
        }

      return false; // se recorrió todo v sin encontrar ciclo
    }

    // Toma el bloque block, que está mapeado del grafo original, que es
    // fuertemente conexo, y construye en path_ptr el ciclo
    void
    build_path(const GT &                                             block, 
               DynMapAvlTree<typename GT::Node*, typename GT::Node*> & table)
    {
      // buscar ciclo en blk.
      auto a     = block.get_first_arc();
      auto start = block.get_tgt_node(a);
      auto end   = block.get_src_node(a);
      assert(start != end);

      auto aux_path = Directed_Find_Path<GT, Itor, SA>(block, sa).dfs(start, end);
      assert(not aux_path.is_empty()); // como es conexo debe encontrarse

      // aux_path es sobre el bloque mapeado. Pero nosotros necesitamos
      // que el camino sea sobre el grafo original. Los nodos del grafo
      // original se recuperan del mapeo table
      path_ptr->empty();
      for (typename Path<GT>::Iterator i(aux_path); i.has_curr(); i.next_ne())
        path_ptr->append_directed(table.find(i.get_current_node_ne()));

      path_ptr->append_directed(path_ptr->get_first_node());
    }

    // retorna true si encuentra un ciclo, en cuyo caso lo pone en
    // path. De lo contrario, retorna false y path queda intacto
    bool build_cycle(typename GT::Node * v)
    {
      init_node_and_push_in_stack(v);

      // recorrer en profundidad todos los nodos conectados a v
      for (Itor <GT, SA> it(v, sa); it.has_curr(); it.next_ne())
        {
          auto w = g_ptr->get_tgt_node(it.get_curr());
          if (not IS_NODE_VISITED(w, Aleph::Depth_First))
            {
              if (build_cycle(w))
                return true;

              low<GT>(v) = std::min(low<GT>(v), low<GT>(w));
            }
          else if (is_node_in_stack(w))
            // si está en pila ==> v fue visitado antes que p
            low<GT>(v) = std::min(low<GT>(v), df<GT>(w));
        }

      if (low<GT>(v) == df<GT>(v)) // ¿primer nodo visitado del bloque?
        {
          GT blk; // grafo auxiliar

          // mapeo de nodos de g con blk (los cookies están ocupados)
          DynMapAvlTree<typename GT::Node*, typename GT::Node*> table;

          // saque nodos de pila e insertelos en bloque auxiliar
          while (true) // saca el componente y lo inserta en blk
            {
              auto p = pop_from_stack();
              auto q = blk.insert_node();
              *q = *p; // copia contenido del nodo
              table.insert(q, p);
              table.insert(p, q);
              if (p == v)
                break;
            }

          if (blk.get_num_nodes() == 1)
            return false; // si blk sólo tiene un nodo no hay ciclo

    	    // terminanos de construir el bloque con los arcos
          for (typename GT::Node_Iterator j(blk); j.has_curr(); j.next_ne())
            {
              auto bsrc = j.get_curr();
              auto gsrc = table.find(bsrc);

    	        // recorrer los arcos de gsrc
              for (Itor <GT, SA> k(gsrc, sa); k.has_curr(); k.next_ne())
                {
                  auto ga   = k.get_curr();
                  auto gtgt = g_ptr->get_tgt_node(ga);
                  auto ptr  = table.search(gtgt);
                  if (ptr == nullptr) // ¿arco del bloque?
                    continue;

                  auto ta = blk.insert_arc(bsrc, ptr->second);
                  *ta = *ga; // copia contenido del arco
                }
            }

          build_path(blk, table);

          return true;
        }

      assert(path_ptr->is_empty());

      return false;
    }

    bool is_connected(typename GT::Node * v)
    {
      init_node_and_push_in_stack(v);

      // recorrer en profundidad todos los nodos conectados a v
      for (Itor <GT, SA> it(v, sa); it.has_curr(); it.next_ne())
        {
          auto w = g_ptr->get_tgt_node(it.get_curr());
          if (not IS_NODE_VISITED(w, Aleph::Depth_First))
            {
              if (not is_connected(w))
                return false;

              low <GT> (v) = std::min(low <GT> (v), low <GT> (w));
            }
          else if (is_node_in_stack(w))
            low <GT> (v) = std::min(low <GT> (v), df <GT> (w));
        }

      if (low<GT>(v) == df<GT>(v)) // ¿primer nodo visitado de bloque?
        {     // saque nodos de pila hasta encontrar a v
          while (pop_from_stack() != v);

          return stack.is_empty();
        }

      return true;
    }

  public:

    /** Calcula los componentes fuertemente conexos de un digrafo.

        connected_components() toma un digrafo g,
        "supuestamente débilmente conexo", calcula sus subgrafos o componentes
        inconexos y los almacena en una lista dinámica blk_list de subdigrafos
        mapeados, tanto en los nodos como en los arcos. El énfasis en el
        supuesto carácter conexo del grafo se debe a que si el grafo no
        fuese conexo, entonces la lista resultante contendría un solo
        elemento correspondiente al digrafo copia mapeado de g. Si este
        fuese el caso, entonces es preferible hacerlo con la función
        copy_graph().

        La función se basa en el algoritmo de Tarjan.

        La función se vale del bit build_subtree para marcar nodos y
        arcos visitados.

        El algoritmo internamente descubre los componentes mediante
        búsquedas en profundidad.

        @param[in] g el grafo sobre el cual se desea calcular sus bloques.
        @param[out] blk_list lista de subgrafos mapeados a g con los
        subgrafos o componentes inconexos de g.
        @param[out] arc_list lista de arcos que conectan a los bloques.
        @throw bad_alloc si no hay memoria para construir algún bloque o
        insertar en la lista.
        @see copy_graph()
    */
    void connected_components(const GT & g, DynList<GT> & blk_list,
                              DynList<typename GT::Arc*> & arc_list)
    {
      init_tarjan(g);

      block_list_ptr = &blk_list;

      for (typename GT::Node_Iterator it(g); df_count < n; it.next_ne())
        {
          auto v = it.get_curr();
          if (not IS_NODE_VISITED(v, Aleph::Depth_First))
            scc_by_blocks(v);
        }

      assert(stack.is_empty());

      // recorrer cada uno de subgrafos parciales y añadir sus arcos
      for (typename DynList<GT>::Iterator i(blk_list); i.has_curr(); i.next_ne())
        {     // recorrer todos los nodos del bloque
          GT & blk = i.get_curr();
          for (typename GT::Node_Iterator j(blk); j.has_curr(); j.next_ne())
            {
              auto bsrc = j.get_curr();
              auto gsrc = mapped_node<GT>(bsrc);

              // recorrer arcos de gsrc
              for (Itor<GT, SA> k(gsrc, sa); k.has_curr(); k.next_ne())
                {
                  auto ga   = k.get_curr();
                  auto gtgt = g_ptr->get_tgt_node(ga);
                  if (NODE_COUNTER(gsrc) != NODE_COUNTER(gtgt))
                    {   // arco inter-bloque ==> añádalo a arc_list
                      arc_list.append(ga);
                      continue;
                    }

                  // insertar y mapear el arco en el sub-bloque
                  auto btgt = mapped_node<GT>(gtgt);
                  auto ba = blk.insert_arc(bsrc, btgt);
                  *ba = *ga; // copia contenido del arco
                  GT::map_arcs(ga, ba);
                }
            }
        }
    }

    /** Calcula los componentes fuertemente conexos de un digrafo.

        Esta versión sobrecargada connected_components() toma un digrafo g,
        "supuestamente débilmente conexo", calcula sus subgrafos o componentes
        inconexos y los almacena en una lista dinámica list de listas de nodos.

        Cada lista contiene los nodos que pertenecen al componente
        fuertemente conexo.

        Úsese esta función si no es necesario obtener copias mapeadas de
        los componentes fuertemente conexos.

        La función se basa en el algoritmo de Tarjan.

        La función se vale del bit build_subtree para marcar nodos y
        arcos visitados.

        El algoritmo internamente descubre los componentes mediante
        búsquedas en profundidad.

        Puesto que se ahorra el mapeo, esta rutina es más rápida y
        menos onerosa en memoria que la versión anterior. La
        desventaja quizá es que no calcula los posibles arcos que
        podrían interconectar un bloque en un solo sentido.

        @param[in] g el grafo sobre el cual se desea calcular sus bloques.
        @param[out] blks lista de listas de nodos. Cada lista
        contiene los nodos que conforman un componente conexo.

        @throw bad_alloc si no hay memoria para insertar en la lista.

        @see copy_graph()
    */
    void connected_components(const GT & g,
                              DynList<DynList<typename GT::Node*>> & blks)
    {
      init_tarjan(g);
      list_list_ptr = &blks;
      for (typename GT::Node_Iterator it(g); df_count < n; it.next_ne())
        {
          auto v = it.get_curr();
          if (not IS_NODE_VISITED(v, Aleph::Depth_First))
            scc_by_lists(v);
        }
    }

    DynList<DynList<typename GT::Node*>> connected_components(const GT & g)
    {
      DynList<DynList<typename GT::Node*>> blks;
      connected_components(g, blks);
      return blks;
    }

    /** Calcula los componentes fuertemente conexos de un digrafo.

        Esta versión sobrecargada connected_components() toma un digrafo g,
        "supuestamente débilmente conexo" y cuenta sus subgrafos o componentes
        inconexos junto con sus tamaños.

        La función se basa en el algoritmo de Tarjan.

        La función se vale del bit build_subtree para marcar nodos y
        arcos visitados.

        El algoritmo internamente descubre los componentes mediante
        búsquedas en profundidad.

        @param[in] g el grafo sobre el cual se desea calcular sus bloques.
        @param[out] blks lista de enteros positivos. Cada lista
        contiene el tamaño de un componente conexo distinto.

        @throw bad_alloc si no hay memoria para insertar en la lista.

        @see copy_graph()
    */
    void connected_components(const GT & g, DynList<size_t> & blks)
    {
      init_tarjan(g);
      list_len_ptr = &blks;
      for (typename GT::Node_Iterator it(g); df_count < n; it.next_ne())
        {
          auto v = it.get_curr();
          if (not IS_NODE_VISITED(v, Aleph::Depth_First))
            scc_by_len(v);
        }
    }

    /// \overload connected_components
    void operator () (const GT &                   g,
                      DynList <GT> &               blk_list,
                      DynList<typename GT::Arc*> & arc_list)
    {
      connected_components(g, blk_list, arc_list);
    }

    /// \overload connected_components
    void operator () (const GT &                             g,
                      DynList<DynList<typename GT::Node*>> & blks)
    {
      connected_components(g, blks);
    }

    DynDlist<DynDlist<typename GT::Node*>> operator () (const GT & g)
    {
      return connected_components(g);
    }

    /// \overload connected_components
    void operator () (const GT &                    g,
                      DynDlist <GT> &               blk_list,
                      DynDlist<typename GT::Arc*> & arc_list)
    {
      DynList<GT> blist;
      DynList<typename GT::Arc*> alist;
      connected_components(g, blist, alist);

      for (typename DynList<GT>::Iterator it(blist); it.has_curr(); it.next_ne())
        {
          GT & curr = it.get_curr();
          GT & block = blk_list.append(GT());
          curr.swap(block);
        }

      for (typename DynList<typename GT::Arc*>::Iterator it(alist);
           it.has_curr(); it.next_ne())
        arc_list.append(it.get_curr());
    }

    /// \overload connected_components
    void operator () (const GT &                               g,
                      DynDlist<DynDlist<typename GT::Node*>> & blks)
    {
      DynList<DynList<typename GT::Node*>> b;
      connected_components(g, b);

      for (typename DynList<DynList<typename GT::Node*>>::Iterator it(b);
           it.has_curr(); it.next_ne())
        {
          auto & tgt_list = blks.append(DynDlist<typename GT::Node*>());

          auto & blk = it.get_curr();
          while (not blk.is_empty())
            tgt_list.append(blk.remove_first());
        }
    }

    /// Retorna true si el digrafo g contiene al menos un ciclo
    bool has_cycle(const GT & g)
    {
      init_tarjan(g);
      for (typename GT::Node_Iterator it(g); df_count < n; it.next_ne())
        {
          auto v = it.get_curr();
          if (not IS_NODE_VISITED(v, Aleph::Depth_First))
            if (has_cycle(v))
              return true;
        }

      return false;
    }

    /// Retorna true si el grafo dirigido es acíclico
    bool is_dag(const GT & g)
    {
      return not has_cycle(g);
    }

    bool compute_cycle(const GT & g, Path<GT> & path)
    {
      init_tarjan(g);
      path_ptr = &path;
      path_ptr->set_graph(g);

      for (typename GT::Node_Iterator it(g); df_count < n; it.next_ne())
        {
          auto v = it.get_curr();
          if (not IS_NODE_VISITED(v, Aleph::Depth_First)) // ¿p visitado?
            if (build_cycle(v))
              return true;
        }

      path.empty();
      return false;
    }

    bool compute_cycle(const GT & g, typename GT::Node * src, Path<GT> & path)
    {
      init_tarjan(g);
      path_ptr = &path;
      path_ptr->set_graph(g);
      return build_cycle(src);
    }

    bool test_connectivity(const GT & g)
    {
      init_tarjan(g);
      for (typename GT::Node_Iterator it(g); df_count < n; it.next_ne())
        {
          auto v = it.get_curr();
          if (not IS_NODE_VISITED(v, Aleph::Depth_First))
            if (not is_connected(v))
              return false;
        }

      assert(stack.is_empty());

      return true;
    }
  };



  /** Determina si un digrafo contiene un ciclo y lo construye.

      Compute_Cycle_In_Digraph() toma un digrafo g, determina si
      contiene un ciclo y, en caso afirmativo, construye un camino
      contentivo del ciclo en cuestión.

      La clase se basa en el algoritmo de Tarjan.

      La función emplea dos parámetros tipo:
      -# GT: la clase digrafo.
      -# SA: el filtro de arcos que emplea el iterador interno.

      La clase se vale del bit build_subtree para marcar nodos y
      arcos visitados.

      @ingroup Grafos
      @throw domain_error si g es un grafo (no un digrafo)
  */
  template <class GT,
            template <typename, class> class Itor = Out_Iterator,
            class SA = Dft_Show_Arc<GT>>
  class Compute_Cycle_In_Digraph
  {
    SA & sa;

  public:

    Compute_Cycle_In_Digraph(SA && __sa = SA())
      : sa(__sa) { /* empty */ }

    Compute_Cycle_In_Digraph(SA & __sa)
      : sa(__sa) { /* empty */ }

    /** Invoca al cálculo de un ciclo en un digrafo.

        @param[in] g el grafo sobre el cual se desea calcular sus bloques.
        @param[out] path camino que define el ciclo.
        @return true si el grafo contiene un ciclo; false de lo
        contrario. En este último caso el valor de path es indeterminado.
        @throw bad_alloc si no hay suficiente memoria.
    */
    bool operator () (const GT & g, Path<GT> & path) const
    {
      Tarjan_Connected_Components<GT, Itor, SA> tarjan(sa);

      return tarjan.compute_cycle(g, path);
    }

    Path<GT> operator () (const GT & g) const
    {
      Path<GT> ret(g);
      Tarjan_Connected_Components<GT, Itor, SA> (sa).compute_cycle(g, ret);
      return ret;
    }

    Path<GT> operator () (const GT & g, typename GT::Node * src) const
    {
      Path<GT> ret(g);
      Tarjan_Connected_Components<GT, Itor, SA>(sa).compute_cycle(g, src, ret);
      return ret;
    }
  };

} // end namespace Aleph

# endif // TARJAN_H
