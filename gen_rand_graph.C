# include <sys/resource.h>
# include <tpl_sgraph.H>
# include <tpl_agraph.H>
# include <io_graph.H>
# include <random_graph.H>
# include <euclidian-graph-common.H>

bool verbose = false;

using namespace Aleph;


typedef Array_Graph<Graph_Anode<My_P>, Graph_Aarc<int>> Graph;


void usage(int argn, char * argc[])
{
  if (argn >= 5)
    return;

  printf("usage: %s n m w h seed [file-name]\n", argc[0]);
  exit(1);
}
 
int main(int argn, char * argc[])
{
  usage(argn, argc);
  
  size_t n = atoi(argc[1]);
  size_t m = atoi(argc[2]);
  int w = atoi(argc[3]);
  int h = atoi(argc[4]);
  unsigned int seed = atoi(argc[5]);
  
  cout << argc[0] << " " << n << " " << m << " " << w << " " << h << " " 
       << seed << endl;

  cout << "Preparing system stack size to 256 Mb ... " << endl
       << endl;

  const rlim_t kStackSize = 256 * 1024 * 1024;   // min stack size = 16 MB
  struct rlimit rl;
  int result;

  result = getrlimit(RLIMIT_STACK, &rl);
  if (result == 0)
      if (rl.rlim_cur < kStackSize)
        {
	  rl.rlim_cur = kStackSize;
	  result = setrlimit(RLIMIT_STACK, &rl);
	  if (result != 0)
	    fprintf(stderr, "setrlimit returned result = %d\n", result);
	  else
	    cout << "OK. done!" << endl
		 << endl;
        }

  cout << "Generating graph ... " << endl;

  Graph g = gen_random_euclidian_graph<Graph>(n, m, w, h, seed);

  if (argn == 7)
    {
      ofstream out(argc[6]);
      IO_Graph<Graph, Rnode<Graph>, Wnode<Graph>, Rarc<Graph>, Warc<Graph>> 
	(g).save_in_text_mode(out);
    }
  else
    IO_Graph<Graph, Rnode<Graph>, Wnode<Graph>, Rarc<Graph>, Warc<Graph>> 
      (g).save_in_text_mode(cout);
}


