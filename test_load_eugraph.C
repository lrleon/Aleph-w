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
