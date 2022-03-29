

/*
                          Aleph_w

  Data structures & Algorithms
  version 1.9d
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2022 Leandro Rabindranath Leon

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

typedef std::pair<string, DynDlist<string>> Info_Nodo;
typedef Graph_Node<Info_Nodo> Nodo;
typedef Graph_Arc<string> Arco;

typedef List_Digraph<Nodo, Arco> Digrafo;

// Esta rutina fue tomada y modificada de Rogue Wave Software.
// http://www2.roguewave.com/support/docs/sourcepro/edition9-update1/html/stdlibug/13-3.html
void split (const string &     text, 
	    const string &     separators,
            DynDlist<string> & words) 
{
  const size_t n     = text.length ();
  size_t start = text.find_first_not_of (separators);

  while (start < n) 
    {
      size_t stop = text.find_first_of (separators, start);
      if (stop > n) 
	stop = n;
      words.append (text.substr (start, stop-start));
      start = text.find_first_not_of (separators, stop+1);
    }
}

struct Equal_Node
{
  bool operator () (const Info_Nodo & o1, const Info_Nodo & o2) const
  {
    return o1.first == o2.first;
  }
};

Digrafo::Node * search_node(Digrafo & g, const string & s)
{
  Digrafo::Node * p = g.search_node <Equal_Node> (s);
  return p == nullptr ? g.insert_node(s) : p;
}


void load_digraph(Digrafo & g, ifstream & nodes_input, ifstream & arcs_input)
{
  string line;
  getline(nodes_input, line); // ignorar primera linea
  while (getline(nodes_input, line))
    {
       DynDlist<string> fields;
       split(line, "|", fields);
       if (fields.size() < 7)
	 continue;
       g.insert_node(Info_Nodo(fields[0], fields));
    }

  while (getline(arcs_input, line))
    {
      DynDlist<string> nums;
      split(line, " ,", nums);
      if (nums.size() < 2)
	continue;
      g.insert_arc(search_node(g, nums[0]), search_node(g, nums[1]));
    }
}

bool with_power = false;
bool with_nes = false;
bool only_num = false;
bool with_class = false;
size_t font_size = 6;
bool vertical = true;

void generate_dot_file(Digrafo & g, ostream & output)
{
  output << "digraph G {" << endl
	 << "fontsize = " << font_size << endl;

  //	 << "shape=box" << endl
    //	 << "rankdir = LR" << endl
    ;

  for (Node_Iterator<Digrafo> it(g); it.has_curr(); it.next_ne())
    {
      Digrafo::Node * p = it.get_curr_ne();
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

  Path<Digrafo> p;
  const bool with_cycle = Compute_Cycle_In_Digraph <Digrafo> () (g, p);
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
      DynDlist<DynDlist<Digrafo::Node*>*> ranks;

      Q_Topological_Sort <Digrafo> () (g, ranks);

      for (DynDlist<DynDlist<Digrafo::Node*>*>::Iterator it(ranks); 
	   it.has_curr(); it.next_ne())
	{
	  DynDlist<Digrafo::Node*> * l = it.get_curr_ne();
	  output << "{ rank = same; " << endl;
	  for (DynDlist<Digrafo::Node*>::Iterator i(*l); 
	       i.has_curr(); i.next_ne())
	    output << i.get_curr_ne()->get_info().first << " ";
	  output << "}" << endl;
	}
    }
  
  for (Arc_Iterator<Digrafo> it(g); it.has_curr(); it.next_ne())
    {
      Digrafo::Arc * a = it.get_curr_ne();
      output << g.get_src_node(a)->get_info().first << " -> " 
	     << g.get_tgt_node(a)->get_info().first << endl;
    }

  output << "}" << endl;
}
