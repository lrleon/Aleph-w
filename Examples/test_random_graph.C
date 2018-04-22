
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

# include <tpl_indexNode.H>
# include <random_graph.H>
# include <eulerian.H>
# include <hamiltonian.H>
# include <single_graph.H>

typedef List_Graph<Graph_Node<int>, Graph_Arc<int> > Grafo;

typedef List_Digraph<Graph_Node<int>, Graph_Arc<int> > Digrafo;

    template <class GT>
GT crear_grafo(const size_t & num_nodes, const size_t & num_arcs)
{
  GT g = Random_Graph<GT> () (num_nodes, num_arcs);

  {
    typename GT::Node_Iterator it(g);
    for (int i = 0; it.has_current(); it.next(), ++i)
      it.get_current()->get_info() = i;
  }

  {
    typename GT::Arc_Iterator it(g);
    for (int i = 0; it.has_current(); it.next(), ++i)
      it.get_current()->get_info() = i;
  }

  return std::move(g);
}

void index_verify(Grafo & g)
{
  {
    IndexNode<Grafo> idx_node(g); 

    for (Grafo::Node_Iterator it(g); it.has_current(); it.next())
      assert(idx_node.search(it.get_current()) == it.get_current());
  }

{
    IndexArc<Grafo> idx_arc(g); 

    for (Grafo::Arc_Iterator it(g); it.has_current(); it.next())
      assert(idx_arc.search(it.get_current()) == it.get_current());
  }
}

int main(int argn, char ** argc)
{
  size_t num_nodes = argc[1] ? atoi(argc[1]) : 1000;
  size_t num_arcs = num_nodes;
  if (argn > 2)
    num_arcs = atoi(argc[2]);

  double p = 0.1;

  if (argn > 3)
    p = atof(argc[3]);

  unsigned int t = time(0);
  if (argn > 4)
    t = atoi(argc[4]);

  cout << argc[0] << " " << num_nodes << " " << num_arcs 
       << " " << p << " " << t << endl;

  {
    cout << "Building random graph by selecting random nodes ..." << endl;

    Grafo g = Random_Graph<Grafo> (t) (num_nodes, num_arcs);

    cout << "Created graph of " << g.get_num_nodes() << " nodes and " 
	 << g.get_num_arcs() << " arcs " << endl << endl;

    if (not test_connectivity (g))
      AH_ERROR("Grafo no es conexo");
    
    assert(Test_Single_Graph<Grafo>()(g));
  }

  {
    cout << "Building random graph by arc probability ..." << endl;

    Grafo g = Random_Graph<Grafo> (t) (num_nodes, p);

    cout << "Created graph of " << g.get_num_nodes() << " nodes and " 
	 << g.get_num_arcs() << " arcs " << endl << endl;

    if (not test_connectivity (g))
      AH_ERROR("Grafo no es conexo");

    assert(Test_Single_Graph<Grafo>()(g));
  }

  {
    cout << "Building random digraph by selecting random nodes ..." << endl;

    Digrafo g = Random_Digraph<Digrafo> (t) (num_nodes, num_arcs);

    cout << "Created digraph of " << g.get_num_nodes() << " nodes and " 
	 << g.get_num_arcs() << " arcs " << endl << endl;

    Digrafo g1 = Random_Digraph<Digrafo> (t) (num_nodes, num_arcs);

    cout << "Created digraph of " << g1.get_num_nodes() << " nodes and " 
     	 << g1.get_num_arcs() << " arcs " << endl << endl;

    {
      DynDlist<DynDlist<Digrafo::Node*> > blocks;
      Tarjan_Connected_Components <Digrafo> () (g, blocks);
      if (blocks.size() > 1)
	AH_ERROR("Digrafo no es conexo");
    }
    
    assert(Test_Single_Graph<Digrafo>()(g));
  }
 
  {
    cout << "Building random digraph by arc probability ..." << endl;

    Digrafo g = Random_Digraph<Digrafo> (t) (num_nodes, p);

    cout << "Created digraph of " << g.get_num_nodes() << " nodes and " 
	 << g.get_num_arcs() << " arcs " << endl << endl;

    {
      DynDlist<DynDlist<Digrafo::Node*> > blocks;
      Tarjan_Connected_Components <Digrafo> () (g, blocks);
      if (blocks.size() > 1)
	AH_ERROR("Digrafo no es conexo");
    
      assert(Test_Single_Graph<Digrafo>()(g));
    }
  }

  {
    cout << "Building random eulerian graph by selecting random nodes ..." << endl;

    Grafo g =  Random_Graph <Grafo> () . eulerian (num_nodes, num_arcs);

    cout << "Created eulerian graph of " << g.get_num_nodes() << " nodes and " 
	 << g.get_num_arcs() << " arcs " << endl << endl;

    if (not test_connectivity (g))
      AH_ERROR("Grafo no es conexo");

    if (Test_Eulerian <Grafo> () (g))
      cout << "el grafo es euleriano" << endl;
    else
      cout << "el grafo no es euleriano" << endl;
    
    cout << endl;
    
    assert(Test_Single_Graph<Grafo>()(g));
  }

  {
    cout << "Building random eulerian graph by arc probability ..." << endl;

    Grafo g =  Random_Graph <Grafo> (t) . eulerian (num_nodes, p);

    cout << "Created eulerian graph of " << g.get_num_nodes() << " nodes and " 
	 << g.get_num_arcs() << " arcs " << endl << endl;

    if (not test_connectivity (g))
      AH_ERROR("Grafo no es conexo");

    if (Test_Eulerian <Grafo> () (g))
      cout << "el grafo es euleriano" << endl;
    else
      cout << "el grafo no es euleriano" << endl;

    assert(Test_Single_Graph<Grafo>()(g));

    cout << endl;
  }

  {
    cout << "Building random eulerian digraph by selecting random nodes ..." << endl;

    Digrafo g =  Random_Digraph <Digrafo> () . eulerian (num_nodes, num_arcs);

    cout << "Created eulerian digraph of " << g.get_num_nodes() 
	 << " nodes and " << g.get_num_arcs() << " arcs " << endl << endl;

    {
      DynDlist<DynDlist<Digrafo::Node*>> blocks;
      Tarjan_Connected_Components <Digrafo> () (g, blocks);
      if (blocks.size() > 1)
	AH_ERROR("Digrafo no es conexo");
    }

    if (Test_Eulerian <Digrafo> () (g))
      cout << "el grafo es euleriano" << endl;
    else
      cout << "el grafo no es euleriano" << endl;
    
    assert(Test_Single_Graph<Digrafo>()(g));

    cout << endl;
  }

  {
    cout << "Building random eulerian digraph by arc probability ..." << endl;

    Digrafo g = Random_Digraph <Digrafo> (t) . eulerian (num_nodes, p);

    cout << "Created eulerian graph of " << g.get_num_nodes() << " nodes and " 
	 << g.get_num_arcs() << " arcs " << endl << endl;

    {
      DynDlist<DynDlist<Digrafo::Node*> > blocks;
      Tarjan_Connected_Components <Digrafo> () (g, blocks);
      if (blocks.size() > 1)
	AH_ERROR("Digrafo no es conexo");
    }

    if (Test_Eulerian <Digrafo> () (g))
      cout << "el grafo es euleriano" << endl;
    else
      cout << "el grafo no es euleriano" << endl;

    assert(Test_Single_Graph<Digrafo>()(g));

    cout << endl;
  }

  {
    cout << "Building random hamiltonian graph by arc probability ..." 
	 << endl;

    Grafo g = Random_Graph <Grafo> () . sufficient_hamiltonian (num_nodes);

    cout << "Created hamiltonian graph of " << g.get_num_nodes() 
	 << " nodes and " 
	 << g.get_num_arcs() << " arcs " << endl << endl;

    if (not test_connectivity (g))
      AH_ERROR("Grafo no es conexo");

    if (Test_Hamiltonian_Sufficiency <Grafo> () (g))
      cout << "el grafo es hamiltonian" << endl;
    else
      cout << "el grafo no es hamiltonian" << endl;
    
    assert(Test_Single_Graph<Grafo>()(g));

    cout << endl;
  }
 
  {
    cout << "Building random hamiltonian digraph by arc probability ..." 
	 << endl;

    Digrafo g = 
      Random_Digraph <Digrafo> () . sufficient_hamiltonian (num_nodes);

    cout << "Created hamiltonian digraph of " << g.get_num_nodes() 
	 << " nodes and " 
	 << g.get_num_arcs() << " arcs " << endl << endl;

    if (not (Tarjan_Connected_Components <Digrafo> ()) . test_connectivity(g))
      AH_ERROR("Digrafo no es conexo");

    if (Test_Hamiltonian_Sufficiency <Digrafo> () (g))
      cout << "el digrafo es hamiltonian" << endl;
    else
      cout << "el digrafo no es hamiltonian" << endl;

    assert(Test_Single_Graph<Digrafo>()(g));
    
    cout << endl;
  }
}
