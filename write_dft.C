# include <iostream>
# include <autosprintf.h>
# include <tpl_graph.H>
# include <tpl_graph_utils.H>
# include <graph_to_tree.H>
# include <generate_tree.H>

struct Nodo : public Graph_Node<char>
{
  Nodo(const char & c) : Graph_Node<char>(c) {}

  Nodo() {}

  Nodo(Node * p) :  Graph_Node<char>(p) {} 
};

struct Arco : public Graph_Arc<Empty_Class> {};


typedef List_Graph<Nodo, Arco> Grafo;



void build_graph(Grafo & g)
{
  Grafo::Node * A = g.insert_node('A');
  Grafo::Node * B = g.insert_node('B');
  Grafo::Node * C = g.insert_node('C');
  Grafo::Node * D = g.insert_node('D');
  Grafo::Node * E = g.insert_node('E');
  Grafo::Node * F = g.insert_node('F');
  Grafo::Node * G = g.insert_node('G');
  Grafo::Node * H = g.insert_node('H');
  Grafo::Node * I = g.insert_node('I');
  Grafo::Node * J = g.insert_node('J');
  Grafo::Node * K = g.insert_node('K');
  Grafo::Node * L = g.insert_node('L');
  Grafo::Node * M = g.insert_node('M');
  Grafo::Node * N = g.insert_node('N');

  g.insert_arc(A, B, Empty_Class());
  g.insert_arc(A, C, Empty_Class());
  g.insert_arc(A, D, Empty_Class());
  g.insert_arc(A, F, Empty_Class());
  g.insert_arc(B, J, Empty_Class());
  g.insert_arc(B, D, Empty_Class());
  g.insert_arc(C, D, Empty_Class());
  g.insert_arc(C, F, Empty_Class());
  g.insert_arc(D, J, Empty_Class());
  g.insert_arc(D, H, Empty_Class());
  g.insert_arc(D, E, Empty_Class());
  g.insert_arc(D, F, Empty_Class());
  g.insert_arc(E, J, Empty_Class());
  g.insert_arc(E, F, Empty_Class());
  g.insert_arc(E, G, Empty_Class());
  g.insert_arc(E, N, Empty_Class());
  g.insert_arc(E, H, Empty_Class());
  g.insert_arc(J, H, Empty_Class());
  g.insert_arc(H, M, Empty_Class());
  g.insert_arc(H, N, Empty_Class());
  g.insert_arc(H, G, Empty_Class());
  g.insert_arc(F, G, Empty_Class());
  g.insert_arc(G, N, Empty_Class());
  g.insert_arc(G, L, Empty_Class());
  g.insert_arc(G, K, Empty_Class());
  g.insert_arc(N, M, Empty_Class());
  g.insert_arc(N, L, Empty_Class());
  g.insert_arc(N, I, Empty_Class());
  g.insert_arc(N, K, Empty_Class());
  g.insert_arc(M, L, Empty_Class());
  g.insert_arc(M, K, Empty_Class());
  g.insert_arc(I, L, Empty_Class());
  g.insert_arc(I, K, Empty_Class());
}

struct Convert
{
  void operator () (Grafo::Node * gnode, Tree_Node<char> * tnode)
  {
    tnode->get_key() = gnode->get_info();
  }
};


struct Write
{
  string operator () (Tree_Node<char> * p)
  {
    return gnu::autosprintf("%c", p->get_key());
  }
};


int main()
{
  Grafo g;

  build_graph(g);

  Grafo t1, t2, t3, t4;

  find_depth_first_spanning_tree(g, g.search_node('A'), t1);

  find_depth_first_spanning_tree(g, g.search_node('N'), t2);

  find_breadth_first_spanning_tree(g, g.search_node('A'), t3);

  find_breadth_first_spanning_tree(g, g.search_node('N'), t4);

  Tree_Node<char> * tree1 = 
    Graph_To_Tree_Node <Grafo, char, Convert> () (t1, t1.search_node('A'));

  Tree_Node<char> * tree2 = 
    Graph_To_Tree_Node <Grafo, char, Convert> () (t2, t2.search_node('N'));

  Tree_Node<char> * tree3 = 
    Graph_To_Tree_Node <Grafo, char, Convert> () (t3, t3.search_node('A'));

  Tree_Node<char> * tree4 = 
    Graph_To_Tree_Node <Grafo, char, Convert> () (t4, t4.search_node('N'));

  ofstream f1("dft-1.Tree", ios::trunc), f2("dft-2.Tree", ios::trunc),
           f3("bft-1.Tree", ios::trunc), f4("bft-2.Tree", ios::trunc);

  generate_tree<Tree_Node<char>, Write>(tree1, f1);

  generate_tree<Tree_Node<char>, Write>(tree2, f2);

  generate_tree<Tree_Node<char>, Write>(tree3, f3);

  generate_tree<Tree_Node<char>, Write>(tree4, f4);

  destroy_tree(tree1);
  destroy_tree(tree2);
  destroy_tree(tree3);
  destroy_tree(tree4);
}


