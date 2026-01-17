/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \|  __/ |_) | | | |_____\ V  V /  version 1.9c
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

#ifndef LOAD_DIGRAPH_H
#define LOAD_DIGRAPH_H

/** @file load_digraph.H
    @brief Utilities for loading directed graphs from pipe-separated files.

    Provides functions to load a directed graph from two input streams:
    one for nodes (pipe-separated fields) and one for arcs (space/comma
    separated pairs of node IDs). Also provides DOT format export with
    optional styling based on node attributes.

    @par Example: Loading graph from files
    @code
    #include <load_digraph.H>
    #include <fstream>
    
    // Create node file (pipe-separated: id|name|value)
    // nodes.txt:
    // 1|Node_A|100
    // 2|Node_B|200
    // 3|Node_C|300
    
    // Create arc file (space-separated: src dst)
    // arcs.txt:
    // 1 2
    // 2 3
    // 1 3
    
    ifstream nodes_file("nodes.txt");
    ifstream arcs_file("arcs.txt");
    
    Digrafo g;
    load_digraph(g, nodes_file, arcs_file);
    
    // Graph now loaded with 3 nodes and 3 arcs
    cout << "Nodes: " << g.get_num_nodes() << endl;
    cout << "Arcs: " << g.get_num_arcs() << endl;
    @endcode

    @par Example: Exporting to DOT format
    @code
    ofstream dot_file("graph.dot");
    save_in_dot(g, dot_file);
    // Creates DOT file for visualization with Graphviz
    @endcode

    @ingroup Graphs
 *  @author Leandro Rabindranath León
*/

#include <fstream>
#include <string>
#include <tpl_graph.H>
#include <topological_sort.H>
#include <Tarjan.H>

namespace Aleph
{

/// Node payload: (id, fields) where fields come from the input row.
using Info_Nodo = std::pair<std::string, DynDlist<std::string>>;

/// Node type for the digraph.
using Nodo = Graph_Node<Info_Nodo>;

/// Arc type for the digraph (edge info is a string).
using Arco = Graph_Arc<std::string>;

/// Digraph type used by the loader.
using Digrafo = List_Digraph<Nodo, Arco>;

/** @brief Split a text into tokens using a separator set.

    Tokenizes the input text by splitting on any character in the
    separators set. Empty tokens are not included.

    @param text The text to split.
    @param separators String containing separator characters.
    @param words Output list to receive the tokens.

    @ingroup Graphs
*/
inline void split(const std::string & text,
                  const std::string & separators,
                  DynDlist<std::string> & words)
{
  const size_t n = text.length();
  size_t start = text.find_first_not_of(separators);

  while (start < n)
    {
      size_t stop = text.find_first_of(separators, start);
      if (stop > n)
        stop = n;
      words.append(text.substr(start, stop - start));
      start = text.find_first_not_of(separators, stop + 1);
    }
}

/** @brief Functor to compare nodes by their ID.
    @ingroup Graphs
*/
struct Equal_Node
{
  /** @brief Compare two node info objects by ID.
      @param o1 First node info.
      @param o2 Second node info.
      @return true if IDs are equal.
  */
  bool operator()(const Info_Nodo & o1, const Info_Nodo & o2) const
  {
    return o1.first == o2.first;
  }
};

/** @brief Find or create a node by ID.

    Searches for a node with the given ID in the graph. If not found,
    creates a new node with empty fields.

    @param g The digraph to search.
    @param s The node ID to find.
    @return Pointer to the found or created node.

    @note This performs a linear search. For large graphs, consider
          using a hash index.

    @ingroup Graphs
*/
inline Digrafo::Node * search_node(Digrafo & g, const std::string & s)
{
  // Linear search by id
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    {
      auto *n = it.get_curr();
      if (n->get_info().first == s)
        return n;
    }
  // Not found -> insert a node with empty fields list
  DynDlist<std::string> empty_fields;
  return g.insert_node(Info_Nodo(s, empty_fields));
}

/// Minimum expected columns for a node record.
constexpr size_t MIN_NODE_FIELDS = 7;

/** @brief Load nodes and arcs from streams into the directed graph.

    Reads nodes from a pipe-separated file and arcs from a space/comma
    separated file, populating the graph.

    The nodes file format:
    - First line is a header (ignored)
    - Subsequent lines: field1|field2|...|fieldN
    - Lines with fewer than MIN_NODE_FIELDS are skipped
    - field[0] is used as the node ID

    The arcs file format:
    - Each line: source_id target_id (or source_id,target_id)
    - Lines with fewer than 2 IDs are skipped
    - If a referenced node doesn't exist, it's created with empty fields

    @param g The digraph to populate.
    @param nodes_input Input stream for node data.
    @param arcs_input Input stream for arc data.

    @ingroup Graphs
*/
inline void load_digraph(Digrafo & g,
                         std::istream & nodes_input,
                         std::istream & arcs_input)
{
  std::string line;

  // Skip header line if present
  std::getline(nodes_input, line);

  while (std::getline(nodes_input, line))
    {
      DynDlist<std::string> fields;
      split(line, "|", fields);
      if (fields.size() < MIN_NODE_FIELDS)
        continue;
      g.insert_node(Info_Nodo(fields[0], fields));
    }

  while (std::getline(arcs_input, line))
    {
      DynDlist<std::string> nums;
      split(line, " ,", nums);
      if (nums.size() < 2)
        continue;
      g.insert_arc(search_node(g, nums[0]), search_node(g, nums[1]));
    }
}

/** @brief Configuration options for DOT file generation.

    These variables control the appearance of the generated DOT output.
    They are inline variables to avoid ODR violations while maintaining
    the original API.

    @note Consider using a configuration struct instead of global variables
          in new code.

    @ingroup Graphs
*/
/// @{

/// If true, include power value in node labels.
inline bool with_power = false;

/// If true, include NES percentage in node labels.
inline bool with_nes = false;

/// If true, show only the node number, not the full label.
inline bool only_num = false;

/// If true, set node shapes based on class field.
inline bool with_class = false;

/// Font size for the DOT output.
inline size_t font_size = 6;

/// If true, use vertical layout (default).
inline bool vertical = true;

/// @}

/** @brief Generate a DOT representation of the digraph.

    Creates a DOT file suitable for rendering with Graphviz. The output
    includes:
    - Node colors based on the "plazo" field (cp=Green, mp=Yellow, lp=Red)
    - Optional labels showing power and NES values
    - Optional shapes based on the "class" field
    - Cycle detection with warning comments
    - Topological ranking for acyclic graphs

    The appearance is controlled by the global configuration variables:
    with_power, with_nes, only_num, with_class, font_size.

    @param g The digraph to export.
    @param output Output stream for the DOT data.

    @ingroup Graphs
*/
inline void generate_dot_file(Digrafo & g, std::ostream & output)
{
  output << "digraph G {\n"
         << "fontsize = " << font_size << "\n";

  for (Node_Iterator<Digrafo> it(g); it.has_curr(); it.next_ne())
    {
      Digrafo::Node *p = it.get_curr();
      Info_Nodo & info = p->get_info();
      output << info.first << " [color=";
      DynDlist<std::string> & fields = info.second;
      const std::string & plazo = fields[1];
      if (plazo == "cp")
        output << "Green";
      else if (plazo == "mp")
        output << "Yellow";
      else if (plazo == "lp")
        output << "Red";

      output << " label=\"" << info.first;

      if (not only_num)
        output << "\\n" << fields[6];

      if (with_power)
        output << "\\nP=" << fields[5];

      if (with_nes)
        output << "\\n" << fields[4] << "%";

      output << "\" ";

      if (with_class)
        {
          const std::string & clase = fields[2];
          if (clase == "it")
            output << " shape = box ";
          else if (clase == "ef")
            output << " shape = ellipse ";
          else if (clase == "pp")
            output << " shape = hexagon ";
        }

      output << "]\n";
    }

  Path<Digrafo> p(g);
  const bool with_cycle = Compute_Cycle_In_Digraph<Digrafo>()(g, p);
  if (with_cycle)
    {
      output << "// WARNING: Cycle detected in ";
      for (Path<Digrafo>::Iterator it(p); it.has_curr(); it.next_ne())
        {
          Info_Nodo & info = it.get_current_node_ne()->get_info();
          output << "(" << info.first << ")" << info.second[6];
          if (not it.is_in_last())
            output << "->";
        }
      output << "\n";
    }
  else
    {
      DynList<DynList<Digrafo::Node *>> ranks;

      Q_Topological_Sort<Digrafo>()(g, ranks);

      for (auto it = ranks.get_it(); it.has_curr(); it.next_ne())
        {
          DynList<Digrafo::Node *> & level = it.get_curr();
          output << "{ rank = same; \n";
          for (auto ni = level.get_it(); ni.has_curr(); ni.next_ne())
            output << ni.get_curr()->get_info().first << " ";
          output << "}\n";
        }
    }

  for (Arc_Iterator<Digrafo> it(g); it.has_curr(); it.next_ne())
    {
      Digrafo::Arc *a = it.get_curr();
      output << g.get_src_node(a)->get_info().first << " -> "
             << g.get_tgt_node(a)->get_info().first << "\n";
    }

  output << "}\n";
}

} // end namespace Aleph

// Backward compatibility: expose types and functions at global scope
using Aleph::Info_Nodo;
using Aleph::Nodo;
using Aleph::Arco;
using Aleph::Digrafo;
using Aleph::split;
using Aleph::Equal_Node;
using Aleph::search_node;
using Aleph::load_digraph;
using Aleph::generate_dot_file;
using Aleph::with_power;
using Aleph::with_nes;
using Aleph::only_num;
using Aleph::with_class;
using Aleph::font_size;
using Aleph::vertical;
using Aleph::MIN_NODE_FIELDS;

#endif // LOAD_DIGRAPH_H
