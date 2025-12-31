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
# ifndef IO_GRAPH_H
# define IO_GRAPH_H

# include <fstream>
# include <iostream>
# include <memory>
# include <tpl_graph.H>
# include <ah-errors.H>

extern bool verbose; // comunmente definida en main

namespace Aleph {

  /** Default node storage functor for binary and text modes.

      Stores node information by writing the raw bytes of get_info()
      to binary streams, or using operator<< for text streams.

      @tparam GT Graph type
      @ingroup Grafos
   */
  template <class GT>
  struct Dft_Store_Node
  {
    /// Stores node to binary stream
    /// @param output Binary output stream
    /// @param p Node pointer to store
    void operator () (std::ofstream & output, GT &, typename GT::Node * p)
    {
      output.write(static_cast<char *>(&p->get_info()), sizeof(typename GT::Node_Type));
    }

    /// Stores node to text stream
    /// @param output Text output stream
    /// @param p Node pointer to store
    void operator () (std::ostream & output, GT &, typename GT::Node * p)
    {
      output << p->get_info() << std::endl;
    }
  };

  /** Default arc storage functor for binary and text modes.

      Stores arc information by writing the raw bytes of get_info()
      to binary streams, or using operator<< for text streams.

      @tparam GT Graph type
      @ingroup Grafos
   */
  template <class GT>
  struct Dft_Store_Arc
  {
    /// Stores arc to binary stream
    /// @param output Binary output stream
    /// @param a Arc pointer to store
    void operator () (std::ofstream & output, GT &, typename GT::Arc * a)
    {
      output.write(static_cast<char *>(&a->get_info()), sizeof(typename GT::Arc_Type));
    }

    /// Stores arc to text stream
    /// @param output Text output stream
    /// @param a Arc pointer to store
    void operator () (std::ostream & output, GT &, typename GT::Arc * a)
    {
      output << a->get_info() << std::endl;
    }
  };

  /** Default node loading functor for binary and text modes.

      Loads node information by reading raw bytes from binary streams,
      or using operator>> for text streams.

      @tparam GT Graph type
      @ingroup Grafos
   */
  template <class GT>
  struct Dft_Load_Node
  {
    /// Loads node from binary stream
    /// @param input Binary input stream
    /// @param p Node pointer to load into
    void operator () (std::ifstream & input, GT &, typename GT::Node * p)
    {
      input.read(static_cast<char *>(&p->get_info()), sizeof(typename GT::Node_Type));
    }

    /// Loads node from text stream
    /// @param input Text input stream
    /// @param p Node pointer to load into
    void operator () (std::istream & input, GT &, typename GT::Node * p)
    {
      input >> p->get_info();
    }
  };

  /** Default arc loading functor for binary and text modes.

      Loads arc information by reading raw bytes from binary streams,
      or using operator>> for text streams.

      @tparam GT Graph type
      @ingroup Grafos
   */
  template <class GT>
  struct Dft_Load_Arc
  {
    /// Loads arc from binary stream
    /// @param input Binary input stream
    /// @param a Arc pointer to load into
    void operator () (std::ifstream & input, GT &, typename GT::Arc * a)
    {
      input.read(static_cast<char *>(&a->get_info()), sizeof(typename GT::Arc_Type));
    }

    /// Loads arc from text stream
    /// @param input Text input stream
    /// @param a Arc pointer to load into
    void operator () (std::istream & input, GT &, typename GT::Arc * a)
    {
      input >> a->get_info();
    }
  };

  /** Clase para guardar y recuperar grafos en y de memoria secundaria.

      IO_Graph está diseñada para guardar y leer grafos en y desde
      memoria secundaria, a través de objetos derivados de fstream.

      La clase maneja 7 parámetros tipo
      - GT: el grafo a guardar o leer.
      - Load_Node: clase encargada de leer la información relacionada a
      un nodo y cargarla a su contenido.
      - Store_Node:  clase encargada de guardar la información relacionada a
      un nodo y cargarla a su contenido. Por omisión se
      escribe el valor arrojado por get_info() sobre el nodo.
      - Load_Arc: clase encargada de leer la información relacionada a
      un arco y cargarla a su contenido. Por omisión se lee en bruto
      del stream y se carga a get_info() sobre el arco.
      - Store_Arc:  clase encargada de guardar la información relacionada a
      un arco y cargarla a su contenido. Por omisión se
      escribe el valor arrojado por get_info() sobre el arco.
      - NF: clase filtro de nodos; sólo se invoca para guardar el
      grafo y sólo se guardan los nodos que pasen por el filtro.
      - AF: clase filtro de arcos; sólo se invoca para guardar el
      grafo y sólo se guardan los arcos que pasen por el filtro.

      EL formato para guardar es:
      -# Cantidad de nodos.
      -# Contenido de los nodos escrito por Store_Node.
      -# Cantidad de arcos.
      -# Finalmente se escriben los arcos con el siguiente formato:
      - Índice del nodo origen e índice del nodo destino según el
      orden de aparición cuando se escribieron los nodos.
      - Contenido del arco escrito por Store_Arc.
      .

      En el caso más común en el cual sólo se requiera guardar en
      bruto los contenidos ofrecidos por get_info() en los nodos y
      arcos, entonces, a través de las plantillas por omisión, es
      suficiente con invocar a la clase IO_Graph<GT> sin el resto de
      los parámetros tipo.

      Según el compilador (en el caso de este autor gnu), los errores
      de compilación si hay alguna falla de correspondencia con los
      parámetros plantilla o con los parámetros de las funciones
      pueden ser muy crípticos. Por esa razón, se recomienda
      ampliamente que las clases de lectura y escritura sean hechas a
      partir de las de omisión que se incluyen en el archivo.

      @see Filter_Iterator 
      @ingroup Grafos
  */
  template <class GT, 
            class Load_Node  = Dft_Load_Node<GT>,
            class Store_Node = Dft_Store_Node<GT>,
            class Load_Arc   = Dft_Load_Arc<GT>,
            class Store_Arc  = Dft_Store_Arc<GT>,
            class NF         = Aleph::Dft_Show_Node<GT>, // Filtro de nodos
            class AF         = Aleph::Dft_Show_Arc<GT> > // Filtro de arcos
  class IO_Graph
  {
    GT & g;

    Load_Node load_node   = Load_Node();
    Store_Node store_node = Store_Node();
    Load_Arc load_arc     = Load_Arc();
    Store_Arc store_arc   = Store_Arc();

    NF node_filter = NF();
    AF arc_filter  = AF();

    bool verbose_mode = false;  // Replaces global verbose variable

  public:

    /// Sets verbose mode for progress output
    /// @param v True to enable verbose output, false to disable
    void set_verbose(bool v) { verbose_mode = v; }

    /// Gets current verbose mode setting
    /// @return True if verbose mode is enabled, false otherwise
    bool is_verbose() const { return verbose_mode; }

    /// Sets the node loading functor
    /// @param ln Node loading functor to use
    void set_load_node(const Load_Node & ln) { load_node = ln; }

    /// Sets the node storage functor
    /// @param sn Node storage functor to use
    void set_store_node(const Store_Node & sn) { store_node = sn; }

    /// Sets the arc loading functor
    /// @param la Arc loading functor to use
    void set_load_arc(const Load_Arc & la) { load_arc = la; }

    /// Sets the arc storage functor
    /// @param sa Arc storage functor to use
    void set_store_arc(const Store_Arc & sa) { store_arc = sa; }

    /// Sets the node filter for save operations
    /// @param nf Node filter functor to use
    void set_node_filter(const NF & nf) { node_filter = nf; }

    /// Sets the arc filter for save operations
    /// @param af Arc filter functor to use
    void set_arc_filter(const AF & af) { arc_filter = af; }

    /// Constructor con referencia a grafo
    IO_Graph(GT & __g) noexcept : g(__g) { /* empty */ }

    /// Constructor con puntero a grafo
    IO_Graph(GT * gptr) noexcept : g(*gptr) { /* empty */ }

    /// Guarda el grafo en el stream output
    void save(std::ofstream & output)
    {
      // guarda cantidad de nodos
      const size_t num_nodes = g.get_num_nodes();

      if (verbose_mode)
        std::cout << "Storing " << num_nodes << " nodes ... ";

      output.write(reinterpret_cast<const char *>(&num_nodes), sizeof(num_nodes));

      int i = 0; // contador de nodos. Lleva los índices en la tabla

      // tabla de pares <ptr nodo, orden de isnerción>
      DynMapTreap<typename GT::Node *, int> nodes_table;

      for (Node_Iterator<GT,NF> it(g, node_filter); it.has_curr();
           it.next_ne(), ++i)
        {
          auto p = it.get_curr();

          if (verbose_mode)
            std::cout << i << " ";

          store_node(output, g, p);

          nodes_table.insert(p, i);
        }

      const size_t num_arcs = g.get_num_arcs();

      if (verbose_mode)
        std::cout << " done " << std::endl 
             << "Storing " << num_arcs << " arcs ... " << std::endl;

      output.write(reinterpret_cast<const char *>(&num_arcs), sizeof(num_arcs));

      for (Arc_Iterator<GT, AF> it(g, arc_filter); it.has_curr(); it.next_ne())
        {
          auto a = it.get_curr();

          auto src = g.get_src_node(a);
          auto tgt = g.get_tgt_node(a);

          const auto & src_idx = nodes_table.find(src);
          const auto & tgt_idx = nodes_table.find(tgt);

          // guarda índices de nodos origen y destino respectivamente
          output.write(static_cast<const char *>(&src_idx), sizeof(int));
          output.write(static_cast<const char *>(&tgt_idx), sizeof(int));

          if (verbose_mode)
            std::cout << " " << src_idx << "--" << tgt_idx << " ";

          store_arc(output, g, a);

          if (verbose_mode)
            std::cout << std::endl;
        }

      if (verbose_mode)
        std::cout << " done " << std::endl << std::endl;
    }

    /// Carga el grafo desde el stream output
    void load(std::ifstream & input)
    {
      // lee cantidad de nodos
      size_t num_nodes;
      input.read(reinterpret_cast<char *>(&num_nodes), sizeof(num_nodes));
      ah_runtime_error_if(!input.good())
        << "Failed to read node count from binary stream";

      if (verbose_mode)
        std::cout << "Loading " << num_nodes << " nodes ...";

      DynArray<typename GT::Node*> nodes_table(num_nodes);
      nodes_table.reserve(0, num_nodes - 1);

      for (size_t i = 0; i < num_nodes; ++i)
        {
          std::unique_ptr<typename GT::Node> p(new typename GT::Node);

          if (verbose_mode)
            std::cout << " " << i;

          load_node(input, g, p.get());
          ah_runtime_error_if(!input.good())
            << "Failed to load node " << i << " from binary stream";

          typename GT::Node * inserted = g.insert_node(p.release());
          nodes_table.access(i) = inserted;
        }

      size_t num_arcs;
      input.read(reinterpret_cast<char *>(&num_arcs), sizeof(num_arcs));
      ah_runtime_error_if(!input.good())
        << "Failed to read arc count from binary stream";

      if (verbose_mode)
        std::cout << " done " << std::endl
             << "Loading " << num_arcs << " arcs ... " << std::endl;

      for (size_t i = 0; i < num_arcs; ++i)
        {
          int src_idx;
          input.read(reinterpret_cast<char *>(&src_idx), sizeof(int));
          ah_runtime_error_if(!input.good())
            << "Failed to read source index for arc " << i;

          auto src = nodes_table.access(src_idx);

          int tgt_idx;
          input.read(reinterpret_cast<char *>(&tgt_idx), sizeof(int));
          ah_runtime_error_if(!input.good())
            << "Failed to read target index for arc " << i;

          auto tgt = nodes_table.access(tgt_idx);

          auto a = g.insert_arc(src, tgt);

          if (verbose_mode)
            std::cout << " " << src_idx << "--" << tgt_idx << " ";

          load_arc(input, g, a);
          ah_runtime_error_if(!input.good())
            << "Failed to load arc " << i << " data";

          if (verbose_mode)
            std::cout << std::endl;
        }

      if (verbose_mode)
        std::cout << " done " << std::endl << std::endl;
    }

    /// Guarda el grafo en el stream output
    void save_in_text_mode(std::ostream & output)
    {        // guarda cantidad de nodos
      const size_t num_nodes = g.get_num_nodes();
      const size_t num_arcs  = g.get_num_arcs();
      output << num_nodes << std::endl
             << num_arcs << std::endl;

      if (verbose_mode)
        std::cout << "Storing " << num_nodes << " nodes ... ";

      int i = 0; // contador de nodos. Lleva los índices en la tabla

      // tabla de pares <ptr nodo, orden de inserción>
      DynMapTreap<typename GT::Node *, int> nodes_table;

      for (Node_Iterator<GT,NF> it(g, node_filter); it.has_curr();
           it.next_ne(), ++i)
        {
          typename GT::Node * p = it.get_curr();

          if (verbose_mode)
            std::cout << i << " ";

          store_node(output, g, p);

          nodes_table.insert(p, i);
        }

      if (verbose_mode)
        std::cout << " done " << std::endl 
             << "Storing " << num_arcs << " arcs ... " << std::endl;

      for (Arc_Iterator<GT, AF> it(g, arc_filter); it.has_curr(); it.next_ne())
        {
          auto a = it.get_curr();

          auto src = g.get_src_node(a);
          auto tgt = g.get_tgt_node(a);

          const auto & src_idx = nodes_table.find(src);
          const auto & tgt_idx = nodes_table.find(tgt);

          // guarda índices de nodos origen y destino respectivamente
          output << src_idx << " " << tgt_idx << " ";

          if (verbose_mode)
            std::cout << " " << src_idx << "--" << tgt_idx << " ";

          store_arc(output, g, a);

          if (verbose_mode)
            std::cout << std::endl;
        }

      if (verbose_mode)
        std::cout << " done " << std::endl << std::endl;
    }

    /// Carga el grafo desde el stream intput
    void load_in_text_mode(std::istream & input)
    {
      // lee cantidad de nodos
      size_t num_nodes;
      size_t num_arcs;

      input >> num_nodes >> num_arcs;
      ah_runtime_error_if(!input.good())
        << "Failed to read node/arc count from text stream";

      input.ignore();

      if (verbose_mode)
        std::cout << "Loading " << num_nodes << " nodes ...";

      DynArray<typename GT::Node*> nodes_table(num_nodes);
      nodes_table.reserve(0, num_nodes - size_t(1));

      for (size_t i = 0; i < num_nodes; ++i)
        {
          std::unique_ptr<typename GT::Node> p(new typename GT::Node);

          if (verbose_mode)
            std::cout << " " << i;

          load_node(input, g, p.get());
          ah_runtime_error_if(!input.good())
            << "Failed to load node " << i << " from text stream";

          typename GT::Node * inserted = g.insert_node(p.release());
          nodes_table.access(i) = inserted;
        }

      if (verbose_mode)
        std::cout << " done " << std::endl
             << "Loading " << num_arcs << " arcs ... " << std::endl;

      for (size_t i = 0; i < num_arcs; ++i)
        {
          int src_idx;
          int tgt_idx;

          input >> src_idx >> tgt_idx;
          ah_runtime_error_if(!input.good())
            << "Failed to read arc " << i << " indices from text stream";

          auto src = nodes_table.access(src_idx);
          auto tgt = nodes_table.access(tgt_idx);
          auto a = g.insert_arc(src, tgt);

          if (verbose_mode)
            std::cout << " " << src_idx << "--" << tgt_idx << " ";

          load_arc(input, g, a);
          ah_runtime_error_if(!input.good())
            << "Failed to load arc " << i << " data from text stream";

          if (verbose_mode)
            std::cout << " " << std::endl;
        }

      if (verbose_mode)
        std::cout << " done " << std::endl << std::endl;
    }
  };

} // end namespace Aleph

# endif // IO_GRAPH_H
