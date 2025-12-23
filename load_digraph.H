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
# include <fstream>
# include <string>
# include <tpl_graph.H>
# include <topological_sort.H>
# include <Tarjan.H>

using std::istream;
using std::ostream;
using std::string;

/// Node payload: (id, fields) where fields come from the input row.
using Info_Nodo = std::pair<string, DynDlist<string>>;
using Nodo      = Graph_Node<Info_Nodo>;
using Arco      = Graph_Arc<string>;
using Digrafo   = List_Digraph<Nodo, Arco>;

/** Split a text into tokens using a separator set (no trimming beyond separators). */
inline void split(const string & text,
                  const string & separators,
                  DynDlist<string> & words)
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

struct Equal_Node
{
  bool operator ()(const Info_Nodo & o1, const Info_Nodo & o2) const
  {
    return o1.first == o2.first;
  }
};

/** Find a node by id, inserting it if not present. */
inline Digrafo::Node * search_node(Digrafo & g, const string & s)
{
  // Linear search by id
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    {
      auto *n = it.get_curr();
      if (n->get_info().first == s)
        return n;
    }
  // Not found -> insert a node with empty fields list
  DynDlist<string> empty_fields;
  return g.insert_node(Info_Nodo(s, empty_fields));
}

/** Minimum expected columns for a node record. */
constexpr size_t MIN_NODE_FIELDS = 7;

/** Load nodes and arcs from streams into the directed graph.
 *  - nodes_input: pipe-separated rows, first line is a header and ignored.
 *  - arcs_input: lines with two ids separated by space or comma.
 *  Lines with insufficient fields are skipped.
 */
inline void load_digraph(Digrafo & g, istream & nodes_input, istream & arcs_input)
{
  string line;

  // Skip header line if present
  std::getline(nodes_input, line);

  while (std::getline(nodes_input, line))
    {
      DynDlist<string> fields;
      split(line, "|", fields);
      if (fields.size() < MIN_NODE_FIELDS)
        continue;
      g.insert_node(Info_Nodo(fields[0], fields));
    }

  while (std::getline(arcs_input, line))
    {
      DynDlist<string> nums;
      split(line, " ,", nums);
      if (nums.size() < 2)
        continue;
      g.insert_arc(search_node(g, nums[0]), search_node(g, nums[1]));
    }
}

// Formatting flags for DOT export
inline bool with_power = false;
inline bool with_nes = false;
inline bool only_num = false;
inline bool with_class = false;
inline size_t font_size = 6;
inline bool vertical = true;

/** Generate a DOT representation of the given digraph.
 *  Colors and labels are driven by Info_Nodo fields:
 *  - color by fields[1] (cp/mp/lp)
 *  - labels optionally include fields[6], P=fields[5], NES=fields[4]
 *  - shapes optionally determined by fields[2] (it/ef/pp)
 */
inline void generate_dot_file(Digrafo & g, ostream & output)
{
  output << "digraph G {" << endl
      << "fontsize = " << font_size << endl;

  //  << "shape=box" << endl
  //  << "rankdir = LR" << endl
  ;

  for (Node_Iterator<Digrafo> it(g); it.has_curr(); it.next_ne())
    {
      Digrafo::Node *p = it.get_curr();
      Info_Nodo & info = p->get_info();
      output << info.first << " [color=";
      DynDlist<string> & fields = info.second;
      const string & plazo = fields[1];
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
        output << "\\n" << fields[4] << "\%";

      output << "\" ";

      if (with_class)
        {
          const string & clase = fields[2];
          if (clase == "it")
            output << " shape = box ";
          else if (clase == "ef")
            output << " shape = ellipse ";
          else if (clase == "pp")
            output << " shape = hexagon ";
        }

      output << "]" << endl;
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
      output << endl;
    }
  else
    {
      DynList<DynList<Digrafo::Node *>> ranks;

      Q_Topological_Sort<Digrafo>()(g, ranks);

      for (auto it = ranks.get_it(); it.has_curr(); it.next_ne())
        {
          DynList<Digrafo::Node *> & level = it.get_curr();
          output << "{ rank = same; " << endl;
          for (auto ni = level.get_it(); ni.has_curr(); ni.next_ne())
            output << ni.get_curr()->get_info().first << " ";
          output << "}" << endl;
        }
    }

  for (Arc_Iterator<Digrafo> it(g); it.has_curr(); it.next_ne())
    {
      Digrafo::Arc *a = it.get_curr();
      output << g.get_src_node(a)->get_info().first << " -> "
          << g.get_tgt_node(a)->get_info().first << endl;
    }

  output << "}" << endl;
}
