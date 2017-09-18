# include <tpl_list_graph.H>



int main()
{
  List_Graph<int, int> multigraph;

  List_Graph<int, int>::Node * node1;// = multigraph.insert_node(0);
  List_Graph<int, int>::Node * node2;// = multigraph.insert_node(1);

  List_Graph<int, int>::Arc * arc = multigraph.insert_arc(node1, node2, 0);
}
