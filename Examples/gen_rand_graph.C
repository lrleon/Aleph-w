
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

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
# include <sys/resource.h>
# include <tpl_sgraph.H>
# include <tpl_agraph.H>
# include <io_graph.H>
# include <random_graph.H>
# include <euclidian-graph-common.H>
# include <tclap/CmdLine.h>

bool verbose = false;

using namespace Aleph;
using namespace std;


typedef Array_Graph<Graph_Anode<My_P>, Graph_Aarc<int>> Graph;


int main(int argc, char * argv[])
{
  TCLAP::CmdLine cmd("Generate random euclidian graph", ' ', "1.0");

  TCLAP::ValueArg<size_t> nArg("n", "nodes", "Number of nodes", false, 100, "size_t");
  TCLAP::ValueArg<size_t> mArg("m", "edges", "Number of edges", false, 1000, "size_t");
  TCLAP::ValueArg<int> wArg("W", "width", "Width", false, 1000, "int");
  TCLAP::ValueArg<int> hArg("H", "height", "Height", false, 1000, "int");
  TCLAP::ValueArg<unsigned int> seedArg("s", "seed", "Random seed", false, 0, "unsigned int");
  TCLAP::UnlabeledValueArg<string> fileArg("file", "Output file name", false, "", "string");

  cmd.add(nArg);
  cmd.add(mArg);
  cmd.add(wArg);
  cmd.add(hArg);
  cmd.add(seedArg);
  cmd.add(fileArg);

  try {
      cmd.parse(argc, argv);
  } catch (TCLAP::ArgException &e) {
      cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
      return 1;
  }

  size_t n = nArg.getValue();
  size_t m = mArg.getValue();
  int w = wArg.getValue();
  int h = hArg.getValue();
  unsigned int seed = seedArg.getValue();
  string fileName = fileArg.getValue();

  if (seed == 0)
    seed = time(nullptr);
  
  cout << argv[0] << " " << n << " " << m << " " << w << " " << h << " "
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

  if (fileName != "")
    {
      ofstream out(fileName);
      IO_Graph<Graph, Rnode<Graph>, Wnode<Graph>, Rarc<Graph>, Warc<Graph>> 
	(g).save_in_text_mode(out);
    }
  else
    IO_Graph<Graph, Rnode<Graph>, Wnode<Graph>, Rarc<Graph>, Warc<Graph>> 
      (g).save_in_text_mode(cout);
}
