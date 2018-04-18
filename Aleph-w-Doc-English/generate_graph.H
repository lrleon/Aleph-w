
/*
  This file is part of Aleph-w system

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014
  Leandro Rabindranath León
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met: 

  1. Redistributions of source code must retain the above copyright 
     notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright 
     notice, this list of conditions and the following disclaimer in the 
     documentation and/or other materials provided with the distribution.

  3. All advertising materials mentioning features or use of this software
     must display the following acknowledgement:

     Copyright (c) 2002-2014 Leandro Rabindranath León. See details of 
     licence.     

     This product includes software developed by the Hewlett-Packard
     Company, Free Software Foundation and Silicon Graphics Computer
     Systems, Inc. 

  4. Neither the name of the ULA nor the names of its contributors may
     be used to endorse or promote products derived from this software
     without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY Leandro Rabindranath León ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Aleph-w is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.

  I request users of this software to return to 

  Leandro Rabindranath Leon
  CEMISID 
  Ed La Hechicera 
  3er piso, ala sur
  Facultad de Ingenieria 
  Universidad de Los Andes 
  Merida - REPÚBLICA BOLIVARIANA DE VENEZUELA    or

  leandro.r.leon@gmail.com

  any improvements or extensions that they make and grant me the rights
  to redistribute these changes.  
*/

/**
  @file generate_graph.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef GENERATE_GRAPH_H
# define GENERATE_GRAPH_H

# include <fstream>
# include <tpl_dynArray.H>
# include <tpl_sort_utils.H>
# include <tpl_graph.H>

using namespace std;
using namespace Aleph;

namespace Aleph {


    template <class GT, class SA> inline static 
bool is_there_a_double_arc(const GT *          g, 
                           typename GT::Node * src, 
                           typename GT::Node * tgt)
{
  if (not g->is_digraph())
    return false;

  return search_arc<GT,SA>(*g, src, tgt) != NULL and 
    search_arc<GT,SA>(*g, tgt, src) != NULL;
}


    template <class GT>
static int search_node(DynArray<typename GT::Node *> & nodes, 
		       typename GT::Node * p)
{
  return sequential_search(nodes, p, 0, nodes.size() - 1);
}


/** 
  @brief generates a specification for the program drawn graph
  graphpic cross graph type.

  @details In a cross graph drawing are distributed nodes_by_level - 1
  num_nodes_by_level peers and nodes on odd levels.

  generate_cross_graph (g, nodes_by_level, xdist, ydist, output) generates
  graphpic an entrance to a cross nodes_by_level graph,
  apart horizontally and vertically xdist
  ydist. The output is output to the output file associated with.

  To deal with the contents of nodes and arcs and that
  want to generate, type the following parameters are used:
  - # GT: type of graph or digraph.
  - # SA: Class arches filter.
  - # Write_Node: Converts the contents of a node to a string
  for the text to be issued.
  - # Write_Arc: Converts the contents of an arc to a string
  for the text to be issued.
  - # Shade_Node: Command to be issued for a dark node. If I dont know
  want dark, then you must generate the empty string; than
  otherwise, generating the command graphpic (usually
  Shadow-Node).
  - # Shade_Arc: Command to be issued for a dark bow. If I dont know
  want dark, then you must generate the empty string; than
  otherwise, generating the command graphpic (usually
  Shadow-Arc.
  - # SA: Filter arches iterator

  @param [in] g Network or the digraph to be generated drawing.
  @param [in] xdist Horizontal separation between nodes.
  @param [out] output Where the output file to be issued.

  @see Filter_Iterator

  @ingroup Graphs
*/
template <class GT, 
	  class Write_Node, class Write_Arc,
	  class Shade_Node, class Shade_Arc, class SA> 
void generate_graphpic(const GT &      g, 
		       const double &  xdist,
		       //const double &  ydist,
		       std::ostream & output)
{
  DynArray<typename GT::Node *> nodes;

  typename GT::Node_Iterator it(g);
  for (int i = 0; it.has_current(); it.next(), ++i)
    {
      typename GT::Node * p = it.get_current_node();

      nodes[i] = p;

      if (Shade_Node() (p).size() != 0)
	output << Shade_Node() (p) << " " << i << endl;
      
      const string text_node = Write_Node () (p);

      if (text_node.size() == 0)
	continue;

      output << "NODE-TEXT " << i << " \"" << text_node << "\" 0 0" << endl;
    }

  for (Arc_Iterator<GT, SA> it(g); it.has_current(); it.next())
    {
      typename GT::Arc * a = it.get_current_arc();

      typename GT::Node * src = g.get_src_node(a);
      typename GT::Node * tgt = g.get_tgt_node(a);
      
      const int src_idx = search_node <GT> (nodes, src);
      const int tgt_idx = search_node <GT> (nodes, tgt);

      if (is_there_a_double_arc <GT, SA> (&g, src, tgt))
	output << "CURVE-ARC " << src_idx << " " << tgt_idx << " " 
	       << xdist/5 << " L" << endl;
      else
	output << "ARC " << src_idx << " " << tgt_idx << endl;
      
      if ( Shade_Arc()(a).size() != 0)
	output << Shade_Arc()(a) << " " 
	       << src_idx << " " << tgt_idx << " " << endl;

      const string text_arc = Write_Arc() (a);

      if (text_arc.size() == 0)
	continue;

      output << "ARC-TEXT " <<  src_idx << " " << tgt_idx << " \""
             << text_arc << "\" 0 0 " << endl;
    }  
}

/** 
  @brief generates a specification for the program drawn graph
  graphpic cross graph type.

  @details In a cross graph drawing are distributed nodes_by_level - 1
  num_nodes_by_level peers and nodes on odd levels.

  generate_cross_graph (g, nodes_by_level, xdist, ydist, output) Generates
  graphpic an entrance to a cross nodes_by_level graph,
  apart horizontally and vertically xdist
  ydist. The output is output to the output file associated with.

  To deal with the contents of nodes and arcs and that
  want to generate, type the following parameters are used:
  - # GT: type of graph or digraph.
  - # Write_Node: Converts the contents of a node to a string
  for the text to be issued.
  - # Write_Arc: Converts the contents of an arc to a string
  for the text to be issued.
  - # Shade_Node: Command to be issued for a dark node. If I dont know
  want dark, then you must generate the empty string.
  - # Shade_Arc: Command to be issued for a dark bow. If I dont know
  want dark, then you must generate the empty string.
  - # Dashed_Node: Command for generating dotted line node.
  - # Dashed_Arc: Command line to generate dotted bow.
  - # SA: Filter iterator arches.
  - # SN: Filter node iterator.

  @param [in] g Network or the digraph to be generated drawing.
  @param [out] output Where the output file to be issued.
  @param [in] rankdir Address drawn between values
  "TB", "BT", "LR" and "RL"
  @param [in] ranksep Topological spacing ranges (where applicable)
  @param [in] nodesep Separation between nodes

  @see Filter_Iterator

  @ingroup Graphs
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
		       const string & rankdir = "TB",
		       float ranksep          = 0.2,
		       float nodesep          = 0.2)
{
  output << "// Generated by generate_graphviz() from Aleph library. See at:" << endl
	 << "// http://webdelprofesor.ula.ve/ingenieria/lrleon/aleph/html/index.html" << endl
	 << "// for documentation" << endl
	 << "// Copyleft Leandro Rabindranath Leon lrleon@ula.ve" << endl
	 << "// for using of graphviz system. See at http://graphviz.org/" 
	 << endl << endl;
  string arc_str;
  if (g.is_digraph())
    {
      arc_str = " -> ";
      output << "digraph {" << endl;
    }
  else
    {
      arc_str = " -- ";
      output << "graph {" << endl;
    }
  output << endl
	 << "rankdir = " << rankdir << endl
	 << "style = none" << endl
	 << "truecolor=false" << endl
	 << "ranksep = " << ranksep << endl
	 << "nodesep = " << nodesep << endl << endl;

  DynArray<typename GT::Node *> nodes;

  Node_Iterator<GT, SN> it(g);
  for (int i = 0; it.has_current(); it.next(), ++i)
    {
      output << i << " [ ";

      typename GT::Node * p = it.get_current_node();
      nodes[i] = p;

      if (Shade_Node () (p))
	output << "style = bold ";
      
      const string text_node = Write_Node () (p);

      if (text_node.size() != 0)
	output << "label = \"" << text_node << "\"";
      output << "]" << endl;
    }

  output << endl;

  for (Arc_Iterator<GT, SA> it(g); it.has_current(); it.next())
    {
      typename GT::Arc * a = it.get_current_arc();
      
      typename GT::Node * src = g.get_src_node(a);
      typename GT::Node * tgt = g.get_tgt_node(a);
      
      int src_idx = search_node <GT> (nodes, src);
      int tgt_idx = search_node <GT> (nodes, tgt);

      output << src_idx << arc_str << tgt_idx << " [";
      
      if (Shade_Arc () (a))
	output << "style = bold ";

      const string text_arc = Write_Arc() (a);

      if (text_arc.size() != 0)
	output << "label = \"" << text_arc << "\"";
      output <<"]" << endl;
    }  

  output << "}" << endl;
}


/** 
  @brief generates a specification for the program drawn graph
  graphpic cross graph type.

  @details In a cross graph drawing are distributed nodes_by_level - 1
  num_nodes_by_level peers and nodes on odd levels.

  generate_cross_graph (g, nodes_by_level, xdist, ydist, output) Generates
  graphpic an entrance to a cross nodes_by_level graph,
  apart horizontally and vertically xdist
  ydist. The output is output to the output file associated with.

  To deal with the contents of nodes and arcs and that
  want to generate, type the following parameters are used:
  - # GT: type of graph or digraph
  - # Node_Attr: Generates a node dot specifcation
  - # Arc_attr: Generates an arc dot specifcation
  for the text to be issued.
  - # SN: Filter node iterator
  - # SA: Filter arches iterator
  .

  @param [in] g Network or the digraph to be generated drawing.
  @param [out] out to where the output file is output.
  @param [in] rankdir Address drawn between values
  "TB", "BT", "LR" and "RL"

  @see Filter_Iterator
    
  @ingroup Graphs
*/
template <class GT, 
	  class Node_Attr, 
	  class Arc_Attr,
	  class SN,
	  class SA> 
void generate_graphviz(const GT & g, std::ostream & out,
		       Node_Attr  node_attr = Node_Attr(),
		       Arc_Attr   arc_attr  = Arc_Attr(),
		       const string & rankdir     = "TB")
{
  out << "// Generated by generate_graphviz() from Aleph library" << endl
      << "// See at:" 
      << "// http://webdelprofesor.ula.ve/ingenieria/lrleon/aleph/html/index.html" << endl
      << "// for documentation of Aleph library" << endl
      << "// Copyleft Leandro Rabindranath Leon lrleon@ula.ve" << endl
      << "// for using of graphviz system. See at http://graphviz.org/" 
      << endl << endl
      << (g.is_digraph() ? "digraph {" : "graph {") << endl 
      << endl
      << "rankdir = " << rankdir << endl
      << endl 
      << "// Node list" << endl 
      << endl;

  DynMapTreap<typename GT::Node *, int> nodes_table;

  Node_Iterator<GT, SN> it(g);
  for (int i = 0; it.has_current(); it.next(), ++i)
    {
      typename GT::Node * p = it.get_current_node();

      nodes_table.insert(p, i);

      out << i << " [ ";
	     
      node_attr (g, p, out);

      out << "]" << endl;
    }

  out << endl
      << endl
      << "// Arc list" << endl
      << endl;

  const string arrow = g.is_digraph() ? "->" : "--";

  for (Arc_Iterator<GT, SA> it(g); it.has_current(); it.next())
    {
      typename GT::Arc * a = it.get_current_arc();

      typename GT::Node * src = g.get_src_node(a);
      typename GT::Node * tgt = g.get_tgt_node(a);
      
      const int src_idx = nodes_table.find(src);
      const int tgt_idx = nodes_table.find(tgt);

      out << src_idx << arrow << tgt_idx << " [";
      arc_attr (g, a, out) ;
      out << "]" << endl;
    }  

  out << "}" << endl;
}

/** 
  @brief generates a specification for the program drawn graph
  graphpic cross graph type.

  @details In a cross graph drawing are distributed nodes_by_level - 1
  num_nodes_by_level peers and nodes on odd levels.

  generate_cross_graph (g, nodes_by_level, xdist, ydist, output) Generates
  graphpic an entrance to a cross nodes_by_level graph,
  apart horizontally and vertically xdist
  ydist. The output is output to the output file associated with.

  To deal with the contents of nodes and arcs and that
  want to generate, type the following parameters are used:
  - # GT: type of graph or digraph
  - # Node_Attr: Generates a node dot specifcation
  - # Arc_attr: Generates an arc dot specifcation for the text to be issued.
  - # SN: Filter node iterator
  - # SA: Filter arches iterator
  .

  @param [in] g Network or the digraph to be generated drawing.
  @param [out] out to where the output file is output.
  @param [in] rankdir Address drawn between values
  "TB", "BT", "LR" and "RL"

  @see Filter_Iterator

  @ingroup Graphs
*/
template <class GT, 
	  class Node_Attr, 
	  class Arc_Attr,
	  class SN,
	  class SA> 
void digraph_graphviz(const GT & g, std::ostream & out,
		      Node_Attr  node_attr = Node_Attr(),
		      Arc_Attr   arc_attr  = Arc_Attr(),
		      const string & rankdir     = "TB")
{
  out << "// Generated by generate_graphviz() from Aleph library" << endl
      << "// See at:" 
      << "// http://webdelprofesor.ula.ve/ingenieria/lrleon/aleph/html/index.html" << endl
      << "// for documentation of Aleph library" << endl
      << "// Copyleft Leandro Rabindranath Leon lrleon@ula.ve" << endl
      << "// for using of graphviz system. See at http://graphviz.org/" 
      << endl << endl
      << "digraph {" << endl 
      << endl
      << "rankdir = " << rankdir << endl
      << endl 
      << "// Node list" << endl 
      << endl;

  DynMapTreap<typename GT::Node *, int> nodes_table;

  Node_Iterator<GT, SN> it(g);
  for (int i = 0; it.has_current(); it.next(), ++i)
    {
      typename GT::Node * p = it.get_current_node();
      nodes_table.insert(p, i);

      out << i << " [ ";
	     
      node_attr (g, p, out);

      out << "]" << endl;
    }

  out << endl
      << endl
      << "// Arc list" << endl
      << endl;

  const string arrow = "->";

  for (Arc_Iterator<GT, SA> it(g); it.has_current(); it.next())
    {
      typename GT::Arc * a = it.get_current_arc();

      typename GT::Node * src = g.get_src_node(a);
      typename GT::Node * tgt = g.get_tgt_node(a);
      
      const int src_idx = nodes_table.find(src);
      const int tgt_idx = nodes_table.find(tgt);

      out << src_idx << arrow << tgt_idx << " [";
      arc_attr (g, a, out) ;
      out << "]" << endl;
    }  

  out << "}" << endl;
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

/** 
  @brief generates a specification of a graph for the program
  graphviz drawn.

  @details TO_Graphviz is a kind of mid-level walking
  entirely the graph g generates a DOT specification for
  graphviz program drawn. Responsibility for To_Graphviz
  and their associated functions is to generate the topological structure
  In terms of the dot language. The contents of the noso and
  arches and different attributes they can asociárseles
  Delegates are special classes.

  To_Graphviz the type parameters are:

  - # GT: graph type derived from the base class List_Graph
  - # Node_Attr: Class commissioned to write to the output stream
  all relevant information and node attributes. For each
  node in the graph, write the dot online:
  @code
  i [result-of-Node_Attr]
  @endcode
  It is the index of the node designated by the class. Class
  - # Is invoked as follows:
  @code
  Node_Attr () (g, p, out);
  @endcode
  where g is the graph, p is the node and out is the output stream
  - # Arc_Attr: Class commissioned to write to the output stream
  all relevant information and attributes the arc. For each
  node in the graph, write the dot online:
  @code
  i [result-of-Node_Attr]
       @endcode
        The class is invoked as follows
  @code
  Node_Attr () (g, p, out);
  @endcode
  where g is the graph, and p is the node is out of the stream
  output
  - # SN: Filter nodes. Only nodes are examined showing the
  - # Iterator filter instantiated this class
  - # SA: Filter arches. Only arcs showing examines the
  - # Iterator filter instantiated this class
  .

  @see Filter_Iterator

  @ingroup Graphs
*/
template <class GT, 
	  class Node_Attr = Dft_Node_Attr<GT>, 
	  class Arc_Attr  = Dft_Arc_Attr<GT>,
	  class SN        = Dft_Show_Node<GT>,
	  class SA        = Dft_Show_Arc<GT>>
struct To_Graphviz
{

/** 
  @brief generation invokes the graph dot specification.

  @param [in] g Graph
  @param [in] stream out to which you want to write the specification.
  @param [in] node_attr Operation attribute for node generation.
  @param [in] arc_attr Operation attribute for generating arc.
  @param [in] rankdir Drawn address.
*/
  void operator () (const GT & g, std::ostream & out,
		    const Node_Attr & node_attr = Node_Attr(),
		    const Arc_Attr & arc_attr   = Arc_Attr(),
		    const string & rankdir      = "TB")
  {
    generate_graphviz <GT, Node_Attr, Arc_Attr, SN, SA> 
      (g, out, node_attr, arc_attr, rankdir);
  }

  void digraph(const GT & g, std::ostream & out,
	       const Node_Attr & node_attr = Node_Attr(),
	       const Arc_Attr & arc_attr   = Arc_Attr(),
	       const string & rankdir      = "TB")
  {
    digraph_graphviz <GT, Node_Attr, Arc_Attr, SN, SA> 
      (g, out, node_attr, arc_attr, rankdir);
  }
};



    template <class GT>
struct Dummy_Attr
{
  bool operator () (typename GT::Node *) const { return false; }

  bool operator () (typename GT::Arc *) const { return false; }
};


/** 
  @brief generates a specification for the program drawn graph
  graphpic cross graph type.

  @details In a cross graph drawing are distributed nodes_by_level - 1
  num_nodes_by_level peers and nodes on odd levels.

  generate_cross_graph (g, nodes_by_level, xdist, ydist, out) Generates
  graphpic an entrance to a cross nodes_by_level graph,
  apart horizontally and vertically xdist
  ydist. The output is output to the file associated with out.

  To deal with the contents of nodes and arcs and that
  want to generate, type the following parameters are used:
  - # GT: type of graph or digraph.
  - # Write_Node: Converts the contents of a node to a string
  for the text to be issued.
  - # Write_Arc: Converts the contents of an arc to a string
  for the text to be issued.
  - # Shade_Node: Command to be issued for a dark node. If I dont know
  want dark, then you must generate the empty string.
  - # Shade_Arc: Command to be issued for a dark bow. If I dont know
  want dark, then you must generate the empty string.
  - # Dashed_Node: Command for generating dotted line node.
  - # Dashed_Arc: Command line to generate dotted bow.
  - # SA: Filter iterator arches.
  - # SN: Filter node iterator.

  @see Filter_Iterator

  @ingroup Graphs
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
/** 
  @brief Summons generation dot specification.

  @param [in] g network or the digraph to be generated drawing.
  @param [out] out to where the output file is output.
  @param [in] rankdir address drawn between values
     "TB", "BT", "LR" and "RL"
  @param [in] ranksep topological spacing ranges (where applicable)
  @param [in] nodesep separation between nodes
*/

  void operator () (GT & g, std::ostream & out,
		    const string & rankdir = "TB",
		    float ranksep = 0.4, float nodesep = 0.4)
  {
    generate_graphviz <GT, Write_Node, Write_Arc, Shade_Node, 
      Shade_Arc, Dashed_Node, Dashed_Arc, SA, SN> (g, out, rankdir, 
						   ranksep, nodesep);
  }
};

/** 
  @brief generates a specification for the program drawn graph
  graphpic cross graph type.

  @details In a cross graph drawing are distributed nodes_by_level - 1
  num_nodes_by_level peers and nodes on odd levels.

  generate_cross_graph (g, nodes_by_level, xdist, ydist, out) Generates
  graphpic an entrance to a cross nodes_by_level graph,
  apart horizontally and vertically xdist
  ydist. The output is output to the file associated with out.

  To deal with the contents of nodes and arcs and that
  want to generate, type the following parameters are used:
  - # GT: type of graph or digraph.
  - # SA: Class arches filter.
  - # Write_Node: Converts the contents of a node to a string
  for the text to be issued.
  - # Write_Arc: Converts the contents of an arc to a string
  for the text to be issued.
  - # Shade_Node: Command to be issued for a dark node. If I dont know
  want dark, then you must generate the empty string; than
  otherwise, generating the command graphpic (usually
  Shadow-Node).
  - # Shade_Arc: Command to be issued for a dark bow. If I dont know
  want dark, then you must generate the empty string; than
  otherwise, generating the command graphpic (usually
  Shadow-Arc.

  @param [in] g Network or the digraph to be generated drawing.
  @param [in] nodes_by_level Number of nodes per level.
  The number of nodes of the last level will depend on the
  number of nodes containing the graph.
  @param [in] xdist Horizontal separation between nodes.
  @param [in] ydist Vertical separation between nodes.
  @param [out] out To where the output file is output.

  @ingroup Graphs
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
         << xdist << " " << ydist << endl 
         << endl;
  
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

/** 
  @brief generates a specification for the program drawn graph
  graphpic network type.

  @details In a drawing network, always distributed
  nodes_by_level nodes per level.

  generate_net_graph (g, nodes_by_level, xdist, ydist, out) Generates
  graphpic an entrance to a network graph nodes_by_level,
  apart horizontally and vertically xdist
  ydist. The output is output to the file associated with out.

  To deal with the contents of nodes and arcs and that
  want to generate, type the following parameters are used:
  - # GT: type of graph or digraph.
  - # SA: Class arches filter.
  - # Write_Node: Converts the contents of a node to a string
  for the text to be issued.
  - # Write_Arc: Converts the contents of an arc to a string
  for the text to be issued.
  - # Shade_Node: command to be issued to a dark node. If I dont know
  want dark, then you must generate the empty string; than
  otherwise, generating the command graphpic (usually
  Shadow-Node.
  - # Shade_Arc: Command to be issued for a dark bow. If I dont know
  want dark, then you must generate the empty string; than
  otherwise, generating the command graphpic (usually
  Shadow-Arc.

  @param [in] g Network or the digraph to be generated drawing.
  @param [in] nodes_by_level Number of nodes per level.
  The number of nodes of the last level will depend on the
  number of nodes containing the graph.
  @param [in] xdist Horizontal separation between nodes.
  @param [in] ydist Vertical separation between nodes.
  @param [out] out To where the output file is output.

  @ingroup Graphs
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
         << xdist << " " << ydist << endl 
         << endl;
  
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
  string operator () (typename GT::Node *) const
  {
    return "";
  }
};


template <class GT> struct __Shade_Arc
{
  string operator () (typename GT::Arc *) const
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


} /*End namespace Aleph */


# endif // GENERATE_GRAPH_H
