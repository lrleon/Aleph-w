
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


/**
 * @file gen_rand_graph.C
 * @brief Generate random Euclidean graphs for testing and visualization
 *
 * This utility program creates random graphs where nodes are placed at
 * random 2D coordinates within a specified rectangular region. Edges are
 * randomly generated to connect nodes, creating realistic spatial graphs
 * useful for testing graph algorithms, visualization, and benchmarking.
 *
 * ## What is a Euclidean Graph?
 *
 * A **Euclidean graph** is a graph where:
 * - Each node has **2D coordinates** (x, y) in a plane
 * - Nodes are **randomly distributed** within a rectangular region
 * - Edges connect nodes (randomly or based on distance)
 * - Useful for modeling **spatial networks**
 *
 * ### Applications of Euclidean Graphs
 *
 * - **Road networks**: Cities connected by roads
 * - **Wireless networks**: Devices with spatial positions
 * - **Geographic data**: Locations on a map
 * - **Spatial algorithms**: Algorithms that use coordinates
 *
 * ## Key Features
 *
 * ### Random Node Placement
 * - **Uniform distribution**: Nodes distributed uniformly in W×H rectangle
 * - **Spatial coordinates**: Each node has (x, y) position
 * - **Configurable region**: Control width and height
 *
 * ### Edge Generation
 * - **Random edges**: Edges randomly connect nodes
 * - **Configurable count**: Control number of edges
 * - **Spatial awareness**: Can use distance-based connection (if implemented)
 *
 * ### Reproducibility
 * - **Random seed**: Optional seed for deterministic generation
 * - **Consistent output**: Same seed produces same graph
 * - **Testing**: Useful for reproducible test cases
 *
 * ### Standard Format
 * - **Aleph-w format**: Outputs in Aleph-w text format
 * - **Compatibility**: Can be loaded with IO_Graph
 * - **Visualization**: Compatible with visualization tools
 *
 * ## Applications
 *
 * ### Algorithm Testing
 * - **Test cases**: Generate test cases for graph algorithms
 * - **Edge cases**: Create graphs with specific properties
 * - **Scalability**: Test algorithms on various sizes
 *
 * ### Visualization
 * - **Visual analysis**: Create graphs for visual inspection
 * - **Demonstrations**: Generate graphs for presentations
 * - **Debugging**: Visualize algorithm behavior
 *
 * ### Benchmarking
 * - **Performance tests**: Generate graphs of various sizes
 * - **Scalability analysis**: Test algorithm scalability
 * - **Comparison**: Compare algorithms on same graphs
 *
 * ### Research
 * - **Graph theory**: Create random instances for experiments
 * - **Spatial analysis**: Study spatial graph properties
 * - **Network modeling**: Model real-world networks
 *
 * ## Output Format
 *
 * The graph is saved in Aleph-w text format, which includes:
 * - **Node information**: Coordinates (x, y) for each vertex
 * - **Edge information**: Connections between nodes
 * - **Metadata**: Graph properties and statistics
 *
 * Can be loaded with `IO_Graph` for visualization or further processing.
 *
 * ## Graph Properties
 *
 * ### Node Distribution
 * - Nodes uniformly distributed in rectangle
 * - No clustering (unless specified)
 * - Random spatial positions
 *
 * ### Edge Properties
 * - Random edge connections
 * - Configurable edge count
 * - May create disconnected components
 *
 * ## Usage Examples
 *
 * ```bash
 * # Generate a small graph (50 nodes, 200 edges) in 500x500 region
 * gen_rand_graph -n 50 -m 200 -W 500 -H 500 graph.txt
 *
 * # Generate with specific seed for reproducibility
 * gen_rand_graph -n 100 -m 500 -W 1000 -H 1000 -s 12345 test.gra
 *
 * # Output to stdout (pipe to another program)
 * gen_rand_graph -n 20 -m 30 -W 200 -H 200 | visualize_graph
 *
 * # Generate large graph for benchmarking
 * gen_rand_graph -n 10000 -m 50000 -W 10000 -H 10000 large.gra
 * ```
 *
 * ## Parameters
 *
 * | Parameter | Short | Description | Default |
 * |-----------|-------|-------------|---------|
 * | `--nodes` | `-n` | Number of nodes | 100 |
 * | `--edges` | `-m` | Number of edges | 1000 |
 * | `--width` | `-W` | Width of coordinate space | 1000 |
 * | `--height` | `-H` | Height of coordinate space | 1000 |
 * | `--seed` | `-s` | Random seed (0 = current time) | 0 |
 * | `[file]` | | Output file (stdout if omitted) | stdout |
 *
 * ## Graph Characteristics
 *
 * ### Connectivity
 * - May be **connected** or **disconnected**
 * - Depends on number of edges relative to nodes
 * - More edges → higher connectivity probability
 *
 * ### Density
 * - **Sparse**: Few edges (E ≈ V)
 * - **Dense**: Many edges (E ≈ V²)
 * - Configurable via edge count parameter
 *
 * ## Integration with Other Tools
 *
 * - **Visualization**: Use with `graphpic.C` or GraphViz
 * - **Algorithms**: Load with `IO_Graph` for algorithm testing
 * - **Analysis**: Process with graph analysis tools
 *
 * @see Random_Graph Random graph generation functions
 * @see euclidian-graph-common.H Euclidean graph node types
 * @see io_graph.H Graph I/O operations
 * @see random_graph_example.C Random graph models (Erdős–Rényi, etc.)
 * @author Leandro Rabindranath León
 * @ingroup Examples
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
