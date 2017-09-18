# include <iostream>
# include <fstream>
# include <xml_graph.H>

struct Node_Info
{
  size_t level;
  std::string name;
};

typedef Graph_Node <Node_Info> Node;
typedef Graph_Arc <double> Arc;
typedef List_Graph <Node, Arc> Graph;

struct Node_Reader
{
  void operator () (Graph &, Graph::Node * p, DynArray <Attr> & attrs)
  {
    Node_Info & info = p->get_info();
    info.level = std::atol(attrs.access(0).value.c_str());
    info.name = attrs.access(1).value;
  }
};

struct Arc_Reader
{
  void operator () (Graph &, Graph::Arc * a, DynArray <Attr> & attrs)
  {
    a->get_info() = std::atof(attrs.access(0).value.c_str());
  }
};

struct Node_Writer
{
  void operator () (Graph &, Graph::Node * p, DynArray <Attr> & attrs)
  {
    Attr & level = attrs.append();
    level.name = "level";
    std::string value = gnu::autosprintf("%ld", p->get_info().level); 
    level.value = value;
    Attr & name = attrs.append();
    name.name = "name";
    name.value = p->get_info().name;
  }
};

struct Arc_Writer
{
  void operator () (Graph &, Graph::Arc * a, DynArray <Attr> & attrs)
  {
    Attr & quantity = attrs.append();
    quantity.name = "quantity";
    std::string value = gnu::autosprintf("%g", a->get_info());
    quantity.value = value;
  }
};

void generate_dot(Graph & g)
{
  DynMapTreap <Graph::Node *, size_t> map;
  ofstream file("graph.dot");

  file << "digraph\n"
       << "{\n"
       << "  rankdir = LR;\n\n";

  size_t i = 0;

  for (Graph::Node_Iterator it(g); it.has_current(); it.next(), ++i)
    {
      Graph::Node * p = it.get_current();
      Node_Info & info = p->get_info();

      std::string color = info.level == 0 ? "green" : "yellow";

      file << "  " << i << "[label = \"" << info.name << "\""
           << "style = \"filled\" color = \"" << color << "\"];\n";
      map.insert(p, i);
    }

  for (Graph::Arc_Iterator it(g); it.has_current(); it.next())
    {
      Graph::Arc * a = it.get_current();
      const size_t & s = map.find(g.get_src_node(a));
      const size_t & t = map.find(g.get_tgt_node(a));

      file << "  " << s << "->" << t
           << "[label = \"" << a->get_info() << "\"];\n";
    }

  file << "}";

  file.close();
}

int main(int argc, char const * argv[])
{
  std::string input_file_name = argc > 1 ? argv[1] : "test.xml";
  std::string output_file_name = argc > 2 ? argv[2] : "output_test.xml";

  Xml_Graph<Graph, Node_Reader, Arc_Reader, Node_Writer, Arc_Writer> xml_graph;

  Graph g = xml_graph(input_file_name);
  generate_dot(g);
  xml_graph(g, output_file_name);

  return 0;
}
