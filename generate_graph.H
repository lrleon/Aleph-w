
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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


/** @file generate_graph.H
 *  @brief Graph visualization and output generation utilities.
 *
 *  This file provides functions for generating graph representations in
 *  various formats suitable for visualization tools and external processing.
 *
 *  ## Supported Formats
 *
 *  - **DOT (Graphviz)**: For generating PNG, SVG, PDF visualizations
 *  - **Custom text formats**: For debugging and logging
 *
 *  ## Key Functions
 *
 *  - `generate_dot()` - Generate DOT format output
 *  - `generate_cross_graph()` - Create cross product of graphs
 *  - `generate_path()` - Create a path graph
 *  - `generate_cycle()` - Create a cycle graph
 *  - `generate_complete()` - Create complete graph K_n
 *
 *  ## DOT Format Example
 *
 *  The DOT format is used by Graphviz tools (dot, neato, etc.):
 *
 *  ```dot
 *  digraph G {
 *    A -> B [label="5"];
 *    B -> C [label="3"];
 *    A -> C [label="10"];
 *  }
 *  ```
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  List_Graph<Node, Arc> g;
 *  // ... build graph ...
 *
 *  // Generate DOT file
 *  std::ofstream out("graph.dot");
 *  generate_dot(g, out);
 *  out.close();
 *
 *  // Then run: dot -Tpng graph.dot -o graph.png
 *  ```
 *
 *  ## Special Graph Generators
 *
 *  ```cpp
 *  // Create path P_5: 0-1-2-3-4
 *  auto path = generate_path<Graph>(5);
 *
 *  // Create cycle C_6: 0-1-2-3-4-5-0
 *  auto cycle = generate_cycle<Graph>(6);
 *
 *  // Create complete graph K_4
 *  auto complete = generate_complete<Graph>(4);
 *  ```
 *
 *  @see random_graph.H For random graph generation
 *  @see tpl_graph.H Graph data structures
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
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


/** @brief Generate a graphpic specification for graph visualization.
   *
   *  Creates a text specification for the graphpic drawing program.
   *  Handles both graphs and digraphs, with support for curved arcs
   *  when bidirectional edges exist.
   *
   *  ## Template Parameters
   *
   *  - **GT**: Graph or digraph type
   *  - **Write_Node**: Functor returning node label as string
   *  - **Write_Arc**: Functor returning arc label as string  
   *  - **Shade_Node**: Functor returning shading command (e.g., "SHADOW-NODE")
   *  - **Shade_Arc**: Functor returning shading command (e.g., "SHADOW-ARC")
   *  - **SA**: Arc filter class
   *
   *  @param g The graph or digraph to visualize
   *  @param xdist Horizontal distance between nodes
   *  @param ydist Vertical distance between levels (currently unused)
   *  @param output Output stream for the graphpic specification
   *
   *  @see Filter_Iterator For arc filtering
   *  @ingroup Graphs
   */
  template <class GT,
            class Write_Node, class Write_Arc,
            class Shade_Node, class Shade_Arc, class SA>
  void generate_graphpic(const GT &      g,
                         const double &  xdist,
                         const double &  ydist,
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

/** @brief Generate a Graphviz DOT specification for graph visualization.
   *
   *  Creates a DOT format specification for the Graphviz system (dot, neato, etc.).
   *  Supports both graphs and digraphs with configurable layout direction.
   *
   *  ## Template Parameters
   *
   *  - **GT**: Graph or digraph type
   *  - **Write_Node**: Functor returning node label
   *  - **Write_Arc**: Functor returning arc label
   *  - **Shade_Node**: Returns true to apply bold style to node
   *  - **Shade_Arc**: Returns true to apply bold style to arc
   *  - **Dashed_Node**: (unused) For dashed node borders
   *  - **Dashed_Arc**: (unused) For dashed arc lines
   *  - **SA**: Arc filter class
   *  - **SN**: Node filter class
   *
   *  @param g The graph or digraph to visualize
   *  @param output Output stream for the DOT specification
   *  @param rankdir Layout direction: "TB" (top-bottom), "BT", "LR", "RL"
   *  @param ranksep Separation between ranks (topological levels)
   *  @param nodesep Separation between nodes in the same rank
   *
   *  @see Filter_Iterator For node/arc filtering
   *  @ingroup Graphs
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


/** @brief Generate Graphviz DOT output with custom attribute functors.
   *
   *  Creates a DOT specification using custom functors to generate
   *  node and arc attributes. This is the most flexible version,
   *  allowing complete control over the DOT output.
   *
   *  ## Attribute Functors
   *
   *  - **Node_Attr**: Called as `node_attr(g, node, out)` to write attributes
   *  - **Arc_Attr**: Called as `arc_attr(g, arc, out)` to write attributes
   *
   *  Example attribute functor:
   *  ```cpp
   *  struct MyNodeAttr {
   *    void operator()(const Graph& g, Graph::Node* n, std::ostream& out) {
   *      out << "label=\"" << n->get_info() << "\" color=blue";
   *    }
   *  };
   *  ```
   *
   *  @tparam GT Graph type
   *  @tparam Node_Attr Node attribute generator functor
   *  @tparam Arc_Attr Arc attribute generator functor
   *  @tparam SN Node filter class
   *  @tparam SA Arc filter class
   *
   *  @param g Graph to visualize
   *  @param out Output stream for DOT specification
   *  @param node_attr Functor for node attributes
   *  @param arc_attr Functor for arc attributes
   *  @param rankdir Layout direction: "TB", "BT", "LR", "RL"
   *
   *  @see Filter_Iterator For filtering
   *  @ingroup Graphs
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

/** @brief Generate Graphviz DOT output specifically for digraphs.
   *
   *  Similar to generate_graphviz() but always outputs "digraph {}"
   *  regardless of the graph type. Useful when you want to force
   *  directed edge notation.
   *
   *  @tparam GT Graph type
   *  @tparam Node_Attr Node attribute generator
   *  @tparam Arc_Attr Arc attribute generator
   *  @tparam SN Node filter
   *  @tparam SA Arc filter
   *
   *  @param g Graph to visualize
   *  @param out Output stream
   *  @param node_attr Node attribute functor
   *  @param arc_attr Arc attribute functor
   *  @param rankdir Layout direction (default: "LR" for left-to-right)
   *
   *  @see generate_graphviz() For automatic graph/digraph detection
   *  @ingroup Graphs
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

/** @brief Generate Graphviz DOT output with topological ranking.
   *
   *  Creates a DOT specification that groups nodes by their topological
   *  rank (level in DAG). Each rank is placed in a separate subgraph
   *  for visual grouping.
   *
   *  This is useful for visualizing DAGs where the vertical position
   *  should reflect dependencies.
   *
   *  @tparam GT Graph type (must be a DAG)
   *  @tparam Node_Attr Node attribute generator
   *  @tparam Arc_Attr Arc attribute generator
   *  @tparam SN Node filter
   *  @tparam SA Arc filter
   *
   *  @param g Graph to visualize (should be a DAG)
   *  @param out Output stream
   *  @param node_attr Node attribute functor
   *  @param arc_attr Arc attribute functor
   *  @param rankdir Layout direction (default: "LR")
   *  @return Number of topological ranks
   *
   *  @see Q_Topological_Sort For rank computation
   *  @ingroup Graphs
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

/** @brief Functor class for generating Graphviz DOT specifications.
   *
   *  A high-level class that traverses a graph and generates a DOT
   *  specification for the Graphviz visualization system. The class
   *  handles the topological structure while delegating content
   *  generation to customizable attribute classes.
   *
   *  ## Template Parameters
   *
   *  - **GT**: Graph type (derived from List_Graph)
   *  - **Node_Attr**: Functor for node attributes. Invoked as:
   *    ```cpp
   *    Node_Attr()(g, node, out);  // writes to out stream
   *    ```
   *  - **Arc_Attr**: Functor for arc attributes. Invoked as:
   *    ```cpp
   *    Arc_Attr()(g, arc, out);    // writes to out stream
   *    ```
   *  - **SN**: Node filter - only filtered nodes are included
   *  - **SA**: Arc filter - only filtered arcs are included
   *
   *  ## Generated Output
   *
   *  For each node:
   *  ```dot
   *  i [ <Node_Attr output> ]
   *  ```
   *
   *  For each arc:
   *  ```dot
   *  src -> tgt [ <Arc_Attr output> ]   // for digraphs
   *  src -- tgt [ <Arc_Attr output> ]   // for graphs
   *  ```
   *
   *  ## Usage Example
   *
   *  ```cpp
   *  To_Graphviz<MyGraph> to_dot;
   *  std::ofstream out("graph.dot");
   *  to_dot(my_graph, out);
   *  ```
   *
   *  @see Filter_Iterator For node/arc filtering
   *  @see Dft_Node_Attr Default node attribute generator
   *  @see Dft_Arc_Attr Default arc attribute generator
   *
   *  @ingroup Graphs
   */
  template <class GT,
            class Node_Attr = Dft_Node_Attr<GT>,
            class Arc_Attr  = Dft_Arc_Attr<GT>,
            class SN        = Dft_Show_Node<GT>,
            class SA        = Dft_Show_Arc<GT>>
  struct To_Graphviz
  {
/** @brief Generate DOT specification for a graph.
     *
     *  @param g The graph to visualize
     *  @param out Output stream for DOT specification
     *  @param node_attr Node attribute generator functor
     *  @param arc_attr Arc attribute generator functor
     *  @param rankdir Layout direction ("TB", "BT", "LR", "RL")
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


/** @brief Functor for generating Graphviz specifications.
   *
   *  A convenience functor class that wraps generate_graphviz() with
   *  full template parameter control.
   *
   *  @tparam GT Graph type
   *  @tparam Write_Node Functor returning node label string
   *  @tparam Write_Arc Functor returning arc label string
   *  @tparam Shade_Node Functor for node shading (returns string command)
   *  @tparam Shade_Arc Functor for arc shading (returns string command)
   *  @tparam Dashed_Node (unused) For dashed node styling
   *  @tparam Dashed_Arc (unused) For dashed arc styling
   *  @tparam SA Arc filter
   *  @tparam SN Node filter
   *
   *  @see generate_graphviz() The underlying function
   *  @ingroup Graphs
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
    /** @brief Generate DOT specification for the graph.
     *
     *  @param g Graph to visualize
     *  @param out Output stream
     *  @param rankdir Layout direction: "TB", "BT", "LR", "RL"
     *  @param ranksep Separation between ranks
     *  @param nodesep Separation between nodes
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

/** @brief Generate a cross-graph layout specification for graphpic.
   *
   *  Creates a graphpic specification with nodes arranged in a cross pattern.
   *  In this layout, even levels have (nodes_by_level - 1) nodes and odd
   *  levels have nodes_by_level nodes, creating a staggered pattern.
   *
   *  @tparam GT Graph type
   *  @tparam Write_Node Functor returning node label string
   *  @tparam Write_Arc Functor returning arc label string
   *  @tparam Shade_Node Functor returning shading command (e.g., "SHADOW-NODE")
   *  @tparam Shade_Arc Functor returning shading command (e.g., "SHADOW-ARC")
   *  @tparam SA Arc filter
   *
   *  @param g Graph to visualize
   *  @param nodes_by_level Nodes per level (last level may have fewer)
   *  @param xdist Horizontal distance between nodes
   *  @param ydist Vertical distance between levels
   *  @param out Output stream for graphpic specification
   *
   *  @ingroup Graphs
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

/** @brief Generate a net-graph layout specification for graphpic.
   *
   *  Creates a graphpic specification with nodes arranged in a regular
   *  grid (net) pattern. Each level has exactly nodes_by_level nodes.
   *
   *  @tparam GT Graph type
   *  @tparam Write_Node Functor returning node label string
   *  @tparam Write_Arc Functor returning arc label string
   *  @tparam Shade_Node Functor returning shading command (e.g., "SHADOW-NODE")
   *  @tparam Shade_Arc Functor returning shading command (e.g., "SHADOW-ARC")
   *  @tparam SA Arc filter
   *
   *  @param g Graph to visualize
   *  @param nodes_by_level Nodes per level (last level may have fewer)
   *  @param xdist Horizontal distance between nodes
   *  @param ydist Vertical distance between levels
   *  @param out Output stream for graphpic specification
   *
   *  @ingroup Graphs
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
