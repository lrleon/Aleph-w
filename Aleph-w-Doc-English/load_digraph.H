
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
  @file load_digraph.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
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

/** @brief This routine was taken and modified Rogue Wave Software.
<http://www2.roguewave.com/support/docs/sourcepro/edition9-update1/html/stdlibug/13-3.html> */
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
  return p == NULL ? g.insert_node(s) : p;
}


void load_digraph(Digrafo & g, ifstream & nodes_input, ifstream & arcs_input)
{
  string line;
  getline(nodes_input, line); /* Ignore first line */
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

  for (Node_Iterator<Digrafo> it(g); it.has_current(); it.next())
    {
      Digrafo::Node * p = it.get_current();
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
      for (Path<Digrafo>::Iterator it(p); it.has_current(); it.next())
	{
	  Info_Nodo & info = it.get_current_node()->get_info();
	  output << "(" << info.first << ")" << info.second[6];
	  if (not it.is_in_last())
	    ooutput << "->";
	}
      output << endl;
    }
  else
    {
      DynDlist<DynDlist<Digrafo::Node*>*> ranks;

      Q_Topological_Sort <Digrafo> () (g, ranks);

      for (DynDlist<DynDlist<Digrafo::Node*>*>::Iterator it(ranks); 
	   it.has_current(); it.next())
	{
	  DynDlist<Digrafo::Node*> * l = it.get_current();
	  output << "{ rank = same; " << endl;
	  for (DynDlist<Digrafo::Node*>::Iterator i(*l); 
	       i.has_current(); i.next())
	    output << i.get_current()->get_info().first << " ";
	  output << "}" << endl;
	}
    }
  
  for (Arc_Iterator<Digrafo> it(g); it.has_current(); it.next())
    {
      Digrafo::Arc * a = it.get_current();
      output << g.get_src_node(a)->get_info().first << " -> " 
	     << g.get_tgt_node(a)->get_info().first << endl;
    }

  output << "}" << endl;
}
