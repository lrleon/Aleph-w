
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

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

# include <random_graph.H>
# include <io_graph.H>


struct Data_Node
{
  int i, j;
};

typedef List_Graph<Graph_Node<int>, Graph_Arc<string> > Grafo;

bool verbose = true; 

struct Write_Arc
{
  void operator () (ofstream & output, Grafo &, Grafo::Arc * a)
  {
    size_t sz = a->get_info().size() + 1;
    output.write((char*) &sz, sizeof(sz));
    output.write(a->get_info().c_str(), a->get_info().size() + 1);
  }
};

struct Read_Arc
{
  static const size_t max_sz = 255;

  static char buffer[max_sz];

  void operator () (ifstream & input, Grafo &, Grafo::Arc * a)
  {
    size_t sz;
    input.read((char*) &sz, sizeof(sz));

    if (sz > max_sz)
      std::range_error("string too long; possibly bad");

    input.read(buffer, sz);

    a->get_info() = string(buffer);
  }
};

  char Read_Arc::buffer[max_sz];

Grafo build_graph()
{
  Grafo g = Random_Graph <Grafo> () (10000, 0.2);

  int i = 0;

  for (Grafo::Node_Iterator it(g); it.has_curr(); it.next())
    it.get_curr()->get_info() = i++;

  for (Grafo::Arc_Iterator it(g); it.has_curr(); it.next())
    {
      Grafo::Arc * a = it.get_curr();
      Grafo::Node * src = g.get_src_node(a);
      Grafo::Node * tgt = g.get_tgt_node(a);

      a->get_info() = 
	to_string(src->get_info()) + "--" + to_string(tgt->get_info());
    }

  return g;
}


void print_graph(Grafo & g)
{
  cout << "Nodes' content ";
  for (Grafo::Node_Iterator it(g); it.has_curr(); it.next())
    cout << it.get_curr()->get_info() << " ";

  cout << " done" << endl << endl
       << "Arcs' content ";
  for (Grafo::Arc_Iterator it(g); it.has_curr(); it.next())
    {
      Grafo::Arc * a = it.get_curr();

      cout << a->get_info() << " ";
    }
  cout << " done" << endl << endl;
}


int main()
{
  {
    Grafo g = build_graph();

    ofstream out("test-random-graph.gra", ios::trunc);

    cout << "saving graph ...";

    IO_Graph<Grafo,Dft_Load_Node<Grafo>,Dft_Store_Node<Grafo>,Read_Arc,Write_Arc> 
      (g).save (out); 

    cout << " done" << endl << endl;

    print_graph(g);
  }
  
  {
    ifstream in("test-random-graph.gra");

    Grafo  g;

    cout << "Enter a key in order to star loading ...";
    cin.get();

    cout << "loading graph ...";

    IO_Graph<Grafo,Dft_Load_Node<Grafo>,Dft_Store_Node<Grafo>,Read_Arc,Write_Arc> 
      (g).load (in); 

    cout << " done" << endl << endl;

    print_graph(g);
  }
}
