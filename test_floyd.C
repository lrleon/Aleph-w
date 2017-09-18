
# include <limits>
# include <iostream>
# include <string>
# include <tpl_graph_utils.H>
# include <Floyd.H>
# include <mat_path.H>

using namespace std;

using namespace Aleph;

typedef List_Digraph<Graph_Node<string>, Graph_Arc<int>> Grafo;

void insertar_arco(Grafo &       grafo, 
		   const string & src_name, 
		   const string & tgt_name,
		   const int      distancia)
{
  Grafo::Node * n1 = grafo.find_node(src_name);

  if (n1 == NULL)
    n1 = grafo.insert_node(src_name);

  Grafo::Node * n2 = grafo.find_node(tgt_name);

  if (n2 == NULL)
    n2 = grafo.insert_node(tgt_name);

  grafo.insert_arc(n1, n2, distancia);
}



void build_test_graph(Grafo & g)
{
  g.insert_node("A");
  g.insert_node("B");
  g.insert_node("C");
  g.insert_node("D");
  g.insert_node("E");
  g.insert_node("F");
  g.insert_node("G");
  g.insert_node("H");
  g.insert_node("I");

  insertar_arco(g, "A", "B", 2);
  insertar_arco(g, "A", "F", 5);
  insertar_arco(g, "B", "F", 1);
  insertar_arco(g, "B", "D", 3);
  insertar_arco(g, "C", "A", 1);  
  insertar_arco(g, "C", "E", 4);  
  insertar_arco(g, "F", "D", -2);
  insertar_arco(g, "F", "C", -1);
  insertar_arco(g, "F", "E", 2);
  insertar_arco(g, "D", "F", 2);
  insertar_arco(g, "D", "H", 4); 
  insertar_arco(g, "E", "G", 2);
  insertar_arco(g, "E", "I", -2);
  insertar_arco(g, "G", "D", 3);
  insertar_arco(g, "G", "F", -1);
  insertar_arco(g, "G", "H", 2);
  insertar_arco(g, "H", "D", -2);
  insertar_arco(g, "H", "G", -1);
  insertar_arco(g, "I", "G", 4);
  insertar_arco(g, "I", "H", 3);

}

int main() 
{
  Grafo g;

  build_test_graph(g);

  const size_t & N = g.get_num_nodes();

  Floyd_All_Shortest_Paths <Grafo> floyd(g);

  if (floyd.has_negative_cycle())
    cout << "All ok" << endl;
  else
    cout << "Negative cyles" << endl;

  const DynMatrix<int> & cost_mat = floyd.get_dist_mat();

  const DynMatrix<long> & path_mat = floyd.get_path_mat();

  for (int i = 0; i < N; ++i)
    {
      for (int j = 0; j < N; ++j)
	cout << cost_mat.access(i,j) << " "; 
      cout << endl;
    }
}
