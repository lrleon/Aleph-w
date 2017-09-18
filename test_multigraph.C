# include <tpl_list_graph.H>


struct Int
{
  int i;

  Int(const int __i) : i(__i) {}
};

int main()
{
  List_Graph<Int, Int> multigraph;

  List_Graph<Int, Int>::Node * node1 = multigraph.insert_node((Int) 0);
  List_Graph<Int, Int>::Node * node2 = multigraph.insert_node(1);

  List_Graph<Int, Int>::Arc * arc1 = multigraph.insert_arc(node1, node2, 0);
  List_Graph<Int, Int>::Arc * arc2 = multigraph.insert_arc(node1, node2, *arc1);
}
