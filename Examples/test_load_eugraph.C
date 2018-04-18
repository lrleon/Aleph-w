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
# include <euclidian-graph-common.H>
# include <io_graph.H>

using namespace Aleph;

bool verbose = false;

typedef List_SGraph<Graph_Snode<My_P>, Graph_Sarc<int>> Graph;


void usage(int argn, char * argc[])
{
  if (argn > 1)
    return;

  printf("usage: %s filename\n", argc[0]);
  exit(1);
}
 

int main(int argn, char * argc[])
{
  usage(argn, argc);

  ifstream in(argc[1]);

  if (not in)
    {
      cerr << "cannot open " << argc[1] << " file" << endl;
      exit(1);
    }

  Graph g;

  IO_Graph<Graph, Rnode<Graph>, Wnode<Graph>, Rarc<Graph>, Warc<Graph>> (g).
    load_in_text_mode(in);

  IO_Graph<Graph, Rnode<Graph>, Wnode<Graph>, Rarc<Graph>, Warc<Graph>> 
      (g).save_in_text_mode(cout);  
}
