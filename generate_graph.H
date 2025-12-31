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
# ifndef GENERATE_GRAPH_H
# define GENERATE_GRAPH_H

# include <fstream>
# include <tpl_dynArray.H>
# include <tpl_sort_utils.H>
# include <tpl_graph.H>
# include <topological_sort.H>

using namespace Aleph;

namespace Aleph {


  template <class GT, class SA> inline static
  bool is_there_a_double_arc(const GT *          g,
                             typename GT::Node * src,
                             typename GT::Node * tgt) noexcept
  {
    if (not g->is_digraph())
      return false;

    return search_arc<GT,SA>(*g, src, tgt) != nullptr and
      search_arc<GT,SA>(*g, tgt, src) != nullptr;
  }


  template <class GT>
  static int search_node(DynArray<typename GT::Node *> & nodes,
                         typename GT::Node * p) noexcept
  {
    return sequential_search(nodes, p, 0, nodes.size() - 1);
  }


  /** Genera una especificación para el programa de dibujado de grafos
      graphpic de tipo grafo cruzado.

      En un dibujo de grafo cruzado, se distribuyen nodes_by_level - 1
      nodos los niveles pares y num_nodes_by_level en los impares.

      generate_cross_graph(g,nodes_by_level,xdist,ydist,output) genera
      una entrada a graphpic de un grafo cruzado de nodes_by_level,
      separados horizontalmente por xdist y verticalmente por
      ydist. La salida se emite al archivo asociado a output.

      Para tratar con los contenidos de los nodos y arcos y lo que se
      desee generar, se emplean los siguientes parámetros tipo:
      -# GT: el tipo de grafo o digrafo.
      -# SA: clase filtro de arcos.
      -# Write_Node: convierte el contenido de un nodo a un std::string
      correspondiente al texto que se desea emitir.
      -# Write_Arc: convierte el contenido de un arco a un std::string
      correspondiente al texto que se desea emitir.
      -# Shade_Node: comando a emitir para un nodo oscuro. Si no se
      desea oscurecer, entonces se debe generar la cadena vacía; de lo
      contrario, generar el comando graphpic (por lo general
      Shadow-Node).
      -# Shade_Arc: comando a emitir para un arco oscuro. Si no se
      desea oscurecer, entonces se debe generar la cadena vacía; de lo
      contrario, generar el comando graphpic (por lo general
      Shadow-Arc.
      -# SA: filtro para el iterador de arcos

      @param[in] g grafo o digrafo del que se desea generar el dibujo.
      @param[in] xdist separación horizontal entre los nodos.
      @param[out] output archivo hacia donde se emitirá la salida.

      @see Filter_Iterator
      @ingroup Grafos
  */
  template <class GT,
            class Write_Node, class Write_Arc,
            class Shade_Node, class Shade_Arc, class SA>
  void generate_graphpic(const GT &      g,
                         const double &  xdist,
                         const double &  /* ydist no usado por ahora */,
                         std::ostream & output)
  {
    DynArray<typename GT::Node *> nodes;
    typename GT::Node_Iterator it(g);
    for (int i = 0; it.has_curr(); it.next_ne(), ++i)
      {
        auto p = it.get_current_node_ne();

        nodes[i] = p;

        if (Shade_Node() (p).size() != 0)
          output << Shade_Node() (p) << " " << i << std::endl;

        const std::string text_node = Write_Node () (p);

        if (text_node.size() == 0)
          continue;

        output << "NODE-TEXT " << i << " \"" << text_node << "\" 0 0" << std::endl;
      }

    for (Arc_Iterator<GT, SA> it(g); it.has_curr(); it.next_ne())
      {
        auto a = it.get_current_arc_ne();
        auto src = g.get_src_node(a);
        auto tgt = g.get_tgt_node(a);

        const auto src_idx = search_node <GT> (nodes, src);
        const auto tgt_idx = search_node <GT> (nodes, tgt);

        if (is_there_a_double_arc <GT, SA> (&g, src, tgt))
          output << "CURVE-ARC " << src_idx << " " << tgt_idx << " "
                 << xdist/5 << " L" << std::endl;
        else
          output << "ARC " << src_idx << " " << tgt_idx << std::endl;

        if ( Shade_Arc()(a).size() != 0)
          output << Shade_Arc()(a) << " "
                 << src_idx << " " << tgt_idx << " " << std::endl;

        const std::string text_arc = Write_Arc() (a);

        if (text_arc.size() == 0)
          continue;

        output << "ARC-TEXT " <<  src_idx << " " << tgt_idx << " \""
               << text_arc << "\" 0 0 " << std::endl;
      }
  }

  /** Genera una especificación para el programa de dibujado de grafos
      graphpic de tipo grafo cruzado.

      En un dibujo de grafo cruzado, se distribuyen nodes_by_level - 1
      nodos los niveles pares y num_nodes_by_level en los impares.

      generate_cross_graph(g,nodes_by_level,xdist,ydist,output) genera
      una entrada a graphpic de un grafo cruzado de nodes_by_level,
      separados horizontalmente por xdist y verticalmente por
      ydist. La salida se emite al archivo asociado a output.

      Para tratar con los contenidos de los nodos y arcos y lo que se
      desee generar, se emplean los siguientes parámetros tipo:
      -# GT: el tipo de grafo o digrafo.
      -# Write_Node: convierte el contenido de un nodo a un std::string
      correspondiente al texto que se desea emitir.
      -# Write_Arc: convierte el contenido de un arco a un std::string
      correspondiente al texto que se desea emitir.
      -# Shade_Node: comando a emitir para un nodo oscuro. Si no se
      desea oscurecer, entonces se debe generar la cadena vacía.
      -# Shade_Arc: comando a emitir para un arco oscuro. Si no se
      desea oscurecer, entonces se debe generar la cadena vacía.
      -# Dashed_Node: comando para generar línea del nodo punteada.
      -# Dashed_Arc: comando para generar línea del arco punteada.
      -# SA: filtro para el iterador de arcos.
      -# SN: filtro para el iterador de nodos.

      @param[in] g grafo o digrafo del que se desea generar el dibujo.
      @param[out] output archivo hacia donde se emitirá la salida.
      @param[in] rankdir dirección del dibujado entre los valores
      "TB", "BT", "LR" y "RL"
      @param[in] ranksep separación entre rangos topológicos (cuando aplique)
      @param[in] nodesep separación entre los nodos

      @see Filter_Iterator
      @ingroup Grafos
  */
  template <class GT,
            class Write_Node,
            class Write_Arc,
            class Shade_Node,
            class Shade_Arc,
            class Dashed_Node,
            class Dashed_Arc,
            class SA,
            class SN>
  void generate_graphviz(const GT & g, std::ostream & output,
                         const std::string & rankdir = "TB",
                         float ranksep          = 0.2,
                         float nodesep          = 0.2)
  {
    output << "// Generated by generate_graphviz() from Aleph-w library. See at:" << std::endl
           << "// http://webdelprofesor.ula.ve/ingenieria/lrleon/aleph/html/index.html" << std::endl
           << "// for documentation" << std::endl
           << "// Copyleft Leandro Rabindranath Leon lrleon@ula.ve" << std::endl
           << "// for using of graphviz system. See at http://graphviz.org/" 
           << std::endl << std::endl;
    std::string arc_str;
    if (g.is_digraph())
      {
        arc_str = " -> ";
        output << "digraph {" << std::endl;
      }
    else
      {
        arc_str = " -- ";
        output << "graph {" << std::endl;
      }
    output << std::endl
           << "rankdir = " << rankdir << std::endl
           << "style = none" << std::endl
           << "truecolor=false" << std::endl
           << "ranksep = " << ranksep << std::endl
           << "nodesep = " << nodesep << std::endl << std::endl;

    DynArray<typename GT::Node *> nodes;

    Node_Iterator<GT, SN> it(g);
    for (int i = 0; it.has_curr(); it.next_ne(), ++i)
      {
        output << i << " [ ";

        auto p = it.get_current_node_ne();
        nodes[i] = p;

        if (Shade_Node () (p))
          output << "style = bold ";

        const std::string text_node = Write_Node () (p);

        if (text_node.size() != 0)
          output << "label = \"" << text_node << "\"";
        output << "]" << std::endl;
      }

    output << std::endl;

    for (Arc_Iterator<GT, SA> it(g); it.has_curr(); it.next_ne())
      {
        auto a = it.get_current_arc_ne();
        auto src = g.get_src_node(a);
        auto tgt = g.get_tgt_node(a);

        auto src_idx = search_node <GT> (nodes, src);
        auto tgt_idx = search_node <GT> (nodes, tgt);

        output << src_idx << arc_str << tgt_idx << " [";

        if (Shade_Arc () (a))
          output << "style = bold ";

        const std::string text_arc = Write_Arc() (a);

        if (text_arc.size() != 0)
          output << "label = \"" << text_arc << "\"";
        output <<"]" << std::endl;
      }

    output << "}" << std::endl;
  }


  /** Genera una especificación para el programa de dibujado de grafos
      graphpic de tipo grafo cruzado.

      En un dibujo de grafo cruzado, se distribuyen nodes_by_level - 1
      nodos los niveles pares y num_nodes_by_level en los impares.

      generate_cross_graph(g,nodes_by_level,xdist,ydist,output) genera
      una entrada a graphpic de un grafo cruzado de nodes_by_level,
      separados horizontalmente por xdist y verticalmente por
      ydist. La salida se emite al archivo asociado a output.

      Para tratar con los contenidos de los nodos y arcos y lo que se
      desee generar, se emplean los siguientes parámetros tipo:
      -# GT: el tipo de grafo o digrafo
      -# Node_Attr: genera la especifcación dot de un nodo
      -# Arc_attr:  genera la especifcación dot de un arco
      correspondiente al texto que se desea emitir.
      -# SN: filtro para el iterador de nodos
      -# SA: filtro para el iterador de arcos
      .

      @param[in] g grafo o digrafo del que se desea generar el dibujo.
      @param[out] out archivo hacia donde se emitirá la salida.
      @param[in] rankdir dirección del dibujado entre los valores
      "TB", "BT", "LR" y "RL"

      @see Filter_Iterator
      @ingroup Grafos
  */
  template <class GT,
            class Node_Attr,
            class Arc_Attr,
            class SN,
            class SA>
  void generate_graphviz(const GT & g, std::ostream & out,
                         Node_Attr  node_attr = Node_Attr(),
                         Arc_Attr   arc_attr  = Arc_Attr(),
                         const std::string & rankdir     = "TB")
  {
    out << "// Generated by generate_graphviz() from Aleph-w library" << std::endl
        << "// See at:"
        << "// http://webdelprofesor.ula.ve/ingenieria/lrleon/aleph/html/index.html" << std::endl
        << "// for documentation of Aleph-w library" << std::endl
        << "// Copyleft Leandro Rabindranath Leon lrleon@ula.ve" << std::endl
        << "// for using of graphviz system. See at http://graphviz.org/"
        << std::endl << std::endl
        << (g.is_digraph() ? "digraph {" : "graph {") << std::endl
        << std::endl
        << "rankdir = " << rankdir << std::endl
        << std::endl
        << "// Node list" << std::endl
        << std::endl;

    DynMapTreap<typename GT::Node *, int> nodes_table;

    Node_Iterator<GT, SN> it(g);
    for (int i = 0; it.has_curr(); it.next_ne(), ++i)
      {
        auto p = it.get_current_node_ne();

        nodes_table.insert(p, i);

        out << i << " [ ";

        node_attr (g, p, out);

        out << "]" << std::endl;
      }

    out << std::endl
        << std::endl
        << "// Arc list" << std::endl
        << std::endl;

    const std::string arrow = g.is_digraph() ? "->" : "--";

    for (Arc_Iterator<GT, SA> it(g); it.has_curr(); it.next_ne())
      {
        auto a = it.get_current_arc_ne();
        auto src = g.get_src_node(a);
        auto tgt = g.get_tgt_node(a);

        const auto src_idx = nodes_table.find(src);
        const auto tgt_idx = nodes_table.find(tgt);

        out << src_idx << arrow << tgt_idx << " [";
        arc_attr (g, a, out) ;
        out << "]" << std::endl;
      }

    out << "}" << std::endl;
  }

  /** Genera una especificación para el programa de dibujado de grafos
      graphpic de tipo grafo cruzado.

      En un dibujo de grafo cruzado, se distribuyen nodes_by_level - 1
      nodos los niveles pares y num_nodes_by_level en los impares.

      generate_cross_graph(g,nodes_by_level,xdist,ydist,output) genera
      una entrada a graphpic de un grafo cruzado de nodes_by_level,
      separados horizontalmente por xdist y verticalmente por
      ydist. La salida se emite al archivo asociado a output.

      Para tratar con los contenidos de los nodos y arcos y lo que se
      desee generar, se emplean los siguientes parámetros tipo:
      -# GT: el tipo de grafo o digrafo
      -# Node_Attr: genera la especifcación dot de un nodo
      -# Arc_attr:  genera la especifcación dot de un arco
      correspondiente al texto que se desea emitir.
      -# SN: filtro para el iterador de nodos
      -# SA: filtro para el iterador de arcos
      .

      @param[in] g grafo o digrafo del que se desea generar el dibujo.
      @param[out] out archivo hacia donde se emitirá la salida.
      @param[in] rankdir dirección del dibujado entre los valores
      "TB", "BT", "LR" y "RL"

      @see Filter_Iterator
      @ingroup Grafos
  */
  template <class GT,
            class Node_Attr,
            class Arc_Attr,
            class SN,
            class SA>
  void digraph_graphviz(const GT & g, std::ostream & out,
                        Node_Attr  node_attr = Node_Attr(),
                        Arc_Attr   arc_attr  = Arc_Attr(),
                        const std::string & rankdir     = "LR")
  {
    out << "// Generated by generate_graphviz() from Aleph-w library" << std::endl
        << "// See at:"
        << "// http://webdelprofesor.ula.ve/ingenieria/lrleon/aleph/html/index.html" << std::endl
        << "// for documentation of Aleph-w library" << std::endl
        << "// Copyleft Leandro Rabindranath Leon lrleon@ula.ve" << std::endl
        << "// for using of graphviz system. See at http://graphviz.org/"
        << std::endl << std::endl
        << "digraph {" << std::endl
        << std::endl
        << "rankdir = " << rankdir << std::endl
        << std::endl
        << "// Node list" << std::endl
        << std::endl;

    DynMapTreap<typename GT::Node *, int> nodes_table;

    Node_Iterator<GT, SN> it(g);
    for (int i = 0; it.has_curr(); it.next_ne(), ++i)
      {
        auto p = it.get_current_node_ne();
        nodes_table.insert(p, i);

        out << i << " [ ";

        node_attr (g, p, out);

        out << "]" << std::endl;
      }

    out << std::endl
        << std::endl
        << "// Arc list" << std::endl
        << std::endl;

    const std::string arrow = "->";

    for (Arc_Iterator<GT, SA> it(g); it.has_curr(); it.next_ne())
      {
        auto a = it.get_current_arc_ne();
        auto src = g.get_src_node(a);
        auto tgt = g.get_tgt_node(a);

        const auto src_idx = nodes_table.find(src);
        const auto tgt_idx = nodes_table.find(tgt);

        out << src_idx << arrow << tgt_idx << " [";
        arc_attr (g, a, out) ;
        out << "]" << std::endl;
      }

    out << "}" << std::endl;
  }

  /** Genera una especificación para el programa de dibujado de grafos
      graphpic de tipo grafo cruzado.

      En un dibujo de grafo cruzado, se distribuyen nodes_by_level - 1
      nodos los niveles pares y num_nodes_by_level en los impares.

      generate_cross_graph(g,nodes_by_level,xdist,ydist,output) genera
      una entrada a graphpic de un grafo cruzado de nodes_by_level,
      separados horizontalmente por xdist y verticalmente por
      ydist. La salida se emite al archivo asociado a output.

      Para tratar con los contenidos de los nodos y arcos y lo que se
      desee generar, se emplean los siguientes parámetros tipo:
      -# GT: el tipo de grafo o digrafo
      -# Node_Attr: genera la especifcación dot de un nodo
      -# Arc_attr:  genera la especifcación dot de un arco
      correspondiente al texto que se desea emitir.
      -# SN: filtro para el iterador de nodos
      -# SA: filtro para el iterador de arcos
      .

      @param[in] g grafo o digrafo del que se desea generar el dibujo.
      @param[out] out archivo hacia donde se emitirá la salida.
      @param[in] rankdir dirección del dibujado entre los valores
      "TB", "BT", "LR" y "RL"

      @see Filter_Iterator
      @ingroup Grafos
  */
  template <class GT,
            class Node_Attr,
            class Arc_Attr,
            class SN,
            class SA>
  size_t rank_graphviz(const GT & g, std::ostream & out,
                       Node_Attr  node_attr = Node_Attr(),
                       Arc_Attr   arc_attr  = Arc_Attr(),
                       const std::string & rankdir     = "LR")
  {
    out << "// Generated by generate_graphviz() from Aleph-w library" << std::endl
        << "// See at:"
        << "// http://webdelprofesor.ula.ve/ingenieria/lrleon/aleph/html/index.html" << std::endl
        << "// for documentation of Aleph-w library" << std::endl
        << "// Copyleft Leandro Rabindranath Leon lrleon@ula.ve" << std::endl
        << "// for using of graphviz system. See at http://graphviz.org/"
        << std::endl << std::endl
        << "digraph {" << std::endl
        << std::endl
        << "rankdir = " << rankdir << std::endl
        << "rank = same" << std::endl
        << std::endl
        << "// Node list" << std::endl
        << std::endl;

    DynMapTreap<typename GT::Node*, size_t> nodes_table;
    DynList<DynList<typename GT::Node*>> ranks = Q_Topological_Sort<GT>().ranks(g);
    size_t rank = 0, i = 0;
    for (auto rank_it = ranks.get_it(); rank_it.has_curr();
         rank_it.next_ne(), ++rank)
      {
        out << "subgraph rank_" << rank << std::endl
            << "{" << std::endl
            <<  "label = \"rank " << rank << "\"" << std::endl;
        for (auto it = rank_it.get_curr().get_it(); it.has_curr();
             it.next_ne(), ++i)
          {
            auto p = it.get_curr();
            nodes_table.insert(p, i);
            out << i << " [ ";
            node_attr(g, p, out);
            out << "]" << std::endl;
          }
        out << "}" << std::endl;
      }

    out << std::endl
        << std::endl
        << "// Arc list" << std::endl
        << std::endl;

    const std::string arrow = "->";
    for (Arc_Iterator<GT, SA> it(g); it.has_curr(); it.next_ne())
      {
        auto a = it.get_current_arc_ne();
        auto src = g.get_src_node(a);
        auto tgt = g.get_tgt_node(a);

        const auto src_idx = nodes_table.find(src);
        const auto tgt_idx = nodes_table.find(tgt);

        out << src_idx << arrow << tgt_idx << " [";
        arc_attr (g, a, out) ;
        out << "]" << std::endl;
      }
    out << "}" << std::endl;

    return rank;
  }

  template <class GT>
  struct Dft_Node_Attr
  {
    void operator () (const GT&, typename GT::Node * p, std::ostream & out)
    {
      out << "label = \"" << p->get_info() << "\"";
    }
  };

  template <class GT>
  struct Dft_Arc_Attr
  {
    void operator () (const GT&, typename GT::Arc * a, std::ostream & out)
    {
      out << "label = \"" << a->get_info() << "\"";
    }
  };

  /** Genera una especificaciòn de un grafo para el programa de
      dibujado graphviz.

      TO_Graphviz es una clase de mediano nivel que recorre
      enteramente el grafo g y genera una especificaciòn dot para el
      programa de dibujado graphviz. La responsabilidad de To_Graphviz
      y de sus funciones asociadas es generar la estructura topològica
      en tèrminos del lenguaje dot. Los contenidos de los noso y
      arcos, asì como los distintos atributos que puedan asociárseles
      son delegados a clases especiales.

      Los parámetros tipo de To_Graphviz son los siguientes:

      - GT: el tipo de grafo derivado de la clase base List_Graph
      - Node_Attr: clase encargada de escribir en el stream de salida
      toda información y atributos pertinentes al nodo. Por cada
      nodo del grafo, se escribe la línea dot:
      \code
      i [ resultado-de-Node_Attr ]
      \endcode
      es el índice del nodo designado por la la clase. La clase
      - es invocada del siguiente modo:
      \code
      Node_Attr () (g, p, out);
      \endcode
      donde g es el grafo, p es el nodo y out es el stream de
      salida
      - Arc_Attr:  clase encargada de escribir en el stream de salida
      toda información y atributos pertinentes al arc. Por cada
      nodo del grafo, se escribe la línea dot:
      \code
      i [ resultado-de-Node_Attr ]
      \endcode
      La clase es invocada del siguiente modo
      \code
      Node_Attr () (g, p, out);
      \endcode
      donde g es el grafo, p es el nodo y out es el stream de
      salida
      - SN: filtro de nodos. sólo se examinan los nodos que muestra el
      - iterador filtro instanciado con esta clase
      - SA: filtro de arcos. Sólo se examinan los arcos que muestra el
      - iterador filtro instanciado con esta clase
      .

      @see Filter_Iterator
      @ingroup Grafos
  */
  template <class GT,
            class Node_Attr = Dft_Node_Attr<GT>,
            class Arc_Attr  = Dft_Arc_Attr<GT>,
            class SN        = Dft_Show_Node<GT>,
            class SA        = Dft_Show_Arc<GT>>
  struct To_Graphviz
  {
    /** Invoca a la generación de la especificación dot del grafo.

        @param[in] g el grafo
        @param[in] out el stream hacia el cual se desea escribir la
        especificación.
        \param[in] node_attr operación de generación de atributo para nodo.
        \param[in] arc_attr operación de generación de atributo para arco.
        @param[in] rankdir la dirección del dibujado.
    */
    void operator () (const GT & g, std::ostream & out,
                      const Node_Attr & node_attr = Node_Attr(),
                      const Arc_Attr & arc_attr   = Arc_Attr(),
                      const std::string & rankdir      = "LR")
    {
      generate_graphviz <GT, Node_Attr, Arc_Attr, SN, SA>
        (g, out, node_attr, arc_attr, rankdir);
    }

    void digraph(const GT & g, std::ostream & out,
                 const Node_Attr & node_attr = Node_Attr(),
                 const Arc_Attr & arc_attr   = Arc_Attr(),
                 const std::string & rankdir      = "LR")
    {
      digraph_graphviz <GT, Node_Attr, Arc_Attr, SN, SA>
        (g, out, node_attr, arc_attr, rankdir);
    }

    void ranks(const GT & g, std::ostream & out,
               const Node_Attr & node_attr = Node_Attr(),
               const Arc_Attr & arc_attr   = Arc_Attr(),
               const std::string & rankdir      = "LR")
    {
      rank_graphviz <GT, Node_Attr, Arc_Attr, SN, SA>
        (g, out, node_attr, arc_attr, rankdir);
    }
  };



  template <class GT>
  struct Dummy_Attr
  {
    bool operator () (typename GT::Node *) const { return false; }

    bool operator () (typename GT::Arc *) const { return false; }
  };


  /** Genera una especificación para el programa de dibujado de grafos
      graphpic de tipo grafo cruzado. 

      En un dibujo de grafo cruzado, se distribuyen nodes_by_level - 1
      nodos los niveles pares y num_nodes_by_level en los impares.

      generate_cross_graph(g,nodes_by_level,xdist,ydist,out) genera
      una entrada a graphpic de un grafo cruzado de nodes_by_level,
      separados horizontalmente por xdist y verticalmente por
      ydist. La salida se emite al archivo asociado a out.

      Para tratar con los contenidos de los nodos y arcos y lo que se
      desee generar, se emplean los siguientes parámetros tipo:
      -# GT: el tipo de grafo o digrafo.
      -# Write_Node: convierte el contenido de un nodo a un std::string
      correspondiente al texto que se desea emitir.
      -# Write_Arc: convierte el contenido de un arco a un std::string
      correspondiente al texto que se desea emitir.
      -# Shade_Node: comando a emitir para un nodo oscuro. Si no se
      desea oscurecer, entonces se debe generar la cadena vacía.
      -# Shade_Arc: comando a emitir para un arco oscuro. Si no se
      desea oscurecer, entonces se debe generar la cadena vacía.
      -# Dashed_Node: comando para generar línea del nodo punteada.
      -# Dashed_Arc: comando para generar línea del arco punteada.
      -# SA: filtro para el iterador de arcos.
      -# SN: filtro para el iterador de nodos.

      @see Filter_Iterator
      @ingroup Grafos
  */
  template <class GT, 
            class Write_Node, 
            class Write_Arc,
            class Shade_Node = Dummy_Attr<GT>, 
            class Shade_Arc = Dummy_Attr<GT>,  
            class Dashed_Node = Dummy_Attr<GT>, 
            class Dashed_Arc = Dummy_Attr<GT>, 
            class SA = Dft_Show_Arc<GT>,
            class SN = Dft_Show_Node<GT> > 
  struct Generate_Graphviz
  {
    /** Invoca la generación de la especificación dot.

        @param[in] g grafo o digrafo del que se desea generar el dibujo.
        @param[out] out archivo hacia donde se emitirá la salida.
        @param[in] rankdir dirección del dibujado entre los valores
        "TB", "BT", "LR" y "RL"
        @param[in] ranksep separación entre rangos topológicos (cuando aplique)
        @param[in] nodesep separación entre los nodos
    */
    void operator () (GT & g, std::ostream & out,
                      const std::string & rankdir = "TB",
                      float ranksep = 0.4, float nodesep = 0.4)
    {
      generate_graphviz <GT, Write_Node, Write_Arc, Shade_Node, 
                         Shade_Arc, Dashed_Node, Dashed_Arc, SA, SN> 
        (g, out, rankdir, ranksep, nodesep);
    }
  };

  /** Genera una especificación para el programa de dibujado de grafos
      graphpic de tipo grafo cruzado. 

      En un dibujo de grafo cruzado, se distribuyen nodes_by_level - 1
      nodos los niveles pares y num_nodes_by_level en los impares.

      generate_cross_graph(g,nodes_by_level,xdist,ydist,out) genera
      una entrada a graphpic de un grafo cruzado de nodes_by_level,
      separados horizontalmente por xdist y verticalmente por
      ydist. La salida se emite al archivo asociado a out.

      Para tratar con los contenidos de los nodos y arcos y lo que se
      desee generar, se emplean los siguientes parámetros tipo:
      -# GT: el tipo de grafo o digrafo.
      -# SA: clase filtro de arcos.
      -# Write_Node: convierte el contenido de un nodo a un std::string
      correspondiente al texto que se desea emitir.
      -# Write_Arc: convierte el contenido de un arco a un std::string
      correspondiente al texto que se desea emitir.
      -# Shade_Node: comando a emitir para un nodo oscuro. Si no se
      desea oscurecer, entonces se debe generar la cadena vacía; de lo
      contrario, generar el comando graphpic (por lo general
      Shadow-Node). 
      -# Shade_Arc: comando a emitir para un arco oscuro. Si no se
      desea oscurecer, entonces se debe generar la cadena vacía; de lo
      contrario, generar el comando graphpic (por lo general
      Shadow-Arc.

      @param[in] g grafo o digrafo del que se desea generar el dibujo.
      @param[in] nodes_by_level cantidad de nodos por nivel. El número
      de nodos del último nivel dependerá de la cantidad de nodos que
      contenga el grafo.
      @param[in] xdist separación horizontal entre los nodos.
      @param[in] ydist separación vertical entre los nodos.
      @param[out] out archivo hacia donde se emitirá la salida.

      @ingroup Grafos
  */
  template <class GT,
            class Write_Node, class Write_Arc,
            class Shade_Node, class Shade_Arc, class SA>
  void generate_cross_graph(GT &            g, 
                            const size_t &  nodes_by_level, 
                            const double &  xdist,
                            const double &  ydist,
                            std::ostream & out)
  {
    if (g.is_digraph())
      out << "cross-net-digraph ";  
    else 
      out << "cross-net-graph "; 

    out << g.get_num_nodes() << " " << nodes_by_level << " " 
        << xdist << " " << ydist << std::endl 
        << std::endl;
  
    generate_graphpic<GT, Write_Node, Write_Arc, Shade_Node, Shade_Arc, SA>
      (g, xdist, ydist, out);
  }

  template <class GT,
            class Write_Node, class Write_Arc,
            class Shade_Node, class Shade_Arc>
  void generate_cross_graph(GT &            g, 
                            const size_t &  nodes_by_level, 
                            const double &  xdist,
                            const double &  ydist,
                            std::ostream & out)
  {
    typedef Dft_Show_Arc<GT> DSA;
    generate_cross_graph<GT, Write_Node, Write_Arc, Shade_Node, Shade_Arc, DSA>
      (g, nodes_by_level, xdist, ydist, out);
  }

  /** Genera una especificación para el programa de dibujado de grafos
      graphpic de tipo red. 

      En un dibujo de red, siempre se distribuyen nodes_by_level nodos
      por nivel.

      generate_net_graph(g,nodes_by_level,xdist,ydist,out) genera
      una entrada a graphpic de un grafo red de nodes_by_level,
      separados horizontalmente por xdist y verticalmente por
      ydist. La salida se emite al archivo asociado a out.

      Para tratar con los contenidos de los nodos y arcos y lo que se
      desee generar, se emplean los siguientes parámetros tipo:
      -# GT: el tipo de grafo o digrafo.
      -# SA: clase filtro de arcos.
      -# Write_Node: convierte el contenido de un nodo a un std::string
      correspondiente al texto que se desea emitir.
      -# Write_Arc: convierte el contenido de un arco a un std::string
      correspondiente al texto que se desea emitir.
      -# Shade_Node: comando a emitir para un nodo oscuro. Si no se
      desea oscurecer, entonces se debe generar la cadena vacía; de lo
      contrario, generar el comando graphpic (por lo general
      Shadow-Node. 
      -# Shade_Arc: comando a emitir para un arco oscuro. Si no se
      desea oscurecer, entonces se debe generar la cadena vacía; de lo
      contrario, generar el comando graphpic (por lo general
      Shadow-Arc.

      @param[in] g grafo o digrafo del que se desea generar el dibujo.
      @param[in] nodes_by_level cantidad de nodos por nivel. El número
      de nodos del último nivel dependerá de la cantidad de nodos que
      contenga el grafo.
      @param[in] xdist separación horizontal entre los nodos.
      @param[in] ydist separación vertical entre los nodos.
      @param[out] out archivo hacia donde se emitirá la salida.

      @ingroup Grafos
  */
  template <class GT, 
            class Write_Node, class Write_Arc,
            class Shade_Node, class Shade_Arc, class SA>
  void generate_net_graph(GT &            g, 
                          const size_t &  nodes_by_level, 
                          const double &  xdist,
                          const double &  ydist,
                          std::ostream & out)
  {
    if (g.is_digraph())
      out << "net-digraph ";  
    else 
      out << "net-graph "; 

    out << g.get_num_nodes() << " " << nodes_by_level << " " 
        << xdist << " " << ydist << std::endl 
        << std::endl;
  
    generate_graphpic<GT, Write_Node, Write_Arc, Shade_Node, Shade_Arc, SA>
      (g, xdist, ydist, out);
  }

  template <class GT, 
            class Write_Node, class Write_Arc,
            class Shade_Node, class Shade_Arc>
  void generate_net_graph(GT &            g, 
                          const size_t &  nodes_by_level, 
                          const double &  xdist,
                          const double &  ydist,
                          std::ostream & out)
  {
    typedef Dft_Show_Arc<GT> DSA;
    generate_net_graph<GT, Write_Node, Write_Arc, Shade_Node, Shade_Arc, DSA>
      (g, nodes_by_level, xdist, ydist, out);
    
  }

  template <class GT> struct __Shade_Node
  {
    std::string operator () (typename GT::Node *) const
    {
      return "";
    }
  };


  template <class GT> struct __Shade_Arc
  {
    std::string operator () (typename GT::Arc *) const
    {
      return "";
    }
  };


  template <class GT, class Write_Node, class Write_Arc, class SA>
  void generate_cross_graph(GT &            g, 
                            const size_t &  nodes_by_level, 
                            const double &  xdist,
                            const double &  ydist,
                            std::ostream & out)
  {
    generate_cross_graph
      <GT, Write_Node, Write_Arc, __Shade_Node<GT>, __Shade_Arc<GT>, SA>
      (g, nodes_by_level, xdist, ydist, out);
  }

  template <class GT, class Write_Node, class Write_Arc, class SA>
  void generate_net_graph(GT &            g, 
                          const size_t &  nodes_by_level, 
                          const double &  xdist,
                          const double &  ydist,
                          std::ostream & out)
  {
    generate_net_graph
      <GT, Write_Node, Write_Arc, __Shade_Node<GT>, __Shade_Arc<GT>, SA>
      (g, nodes_by_level, xdist, ydist, out);
  }


  template <class GT, class Write_Node, class Write_Arc>
  void generate_cross_graph(GT &            g, 
                            const size_t &  nodes_by_level, 
                            const double &  xdist,
                            const double &  ydist,
                            std::ostream & out)
  {
    typedef Dft_Show_Arc<GT> DSA;
    generate_cross_graph
      <GT, Write_Node, Write_Arc, __Shade_Node<GT>, __Shade_Arc<GT>, DSA>
      (g, nodes_by_level, xdist, ydist, out);
  }

  template <class GT, class Write_Node, class Write_Arc>
  void generate_net_graph(GT &            g, 
                          const size_t &  nodes_by_level, 
                          const double &  xdist,
                          const double &  ydist,
                          std::ostream & out)
  {
    typedef Dft_Show_Arc<GT> DSA;
    generate_net_graph 
      <GT, Write_Node, Write_Arc, __Shade_Node<GT>, __Shade_Arc<GT>, DSA>
      (g, nodes_by_level, xdist, ydist, out);
  }


} // end namespace Aleph    


# endif // GENERATE_GRAPH_H
