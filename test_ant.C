
# include <gsl/gsl_rng.h>
# include <gsl/gsl_randist.h>

# include <argp.h>

# include <limits>
# include <iostream>
# include <fstream>
# include <parse_utils.H>
# include <tpl_dynArray.H>
# include <tpl_graph_utils.H>
# include <Dijkstra.H>
# include <Kruskal.H>
# include <tpl_ant.H>
# include <random_graph.H>
# include <io_graph.H>

/* Variable parámetros; pasadas por línea de comandos */

double initial_food = 1000;

bool verbose = false;

bool print_pars = false;

size_t num_nodes = 5000;

size_t num_threads = 5;

size_t num_mutex = 10;

size_t num_ants = 30000;

double arc_prob = 0.1;

string load_file_name;

string save_file_name;

bool distribute_ants_randomly = true;


/* Otras variable de control para manejar estado según la línea de comandos */

bool generate_graph = false;

bool save_graph = false;

bool load_graph = false;



/*****************************************************************/

void print_parameters()
{
  cout << "Ant test configuration:" << endl
       << "    num_nodes    = " << num_nodes << endl
       << "    arc prob     = " << arc_prob << endl
       << "    num_ants     = " << num_ants << endl
       << "    num_treads   = " << num_threads << endl
       << "    num_mutex    = " << num_mutex << endl
       << "    min arc dist = " << min_dist << endl
       << "    max arc dist = " << max_dist << endl
       << "    ant randomly = " << distribute_ants_randomly << endl << endl;

  if (generate_graph)
    cout << "    A random graph would be generated" << endl << endl;

  if (save_graph)
    cout << "    processed graph would be saved in " << save_file_name 
	 << endl << endl;

}

struct Init_Ant_Node
{
  static int counter;

  void operator () (Ant_Graph &, Ant_Graph::Node * p)
  {
    p->num = counter++;

    if (verbose)
      cout << " " << p->num;
  }
};

int Init_Ant_Node::counter = 0;


struct Init_Ant_Arc
{
  static gsl_rng * r;

  static double range;

  void operator () (Ant_Graph & g, Ant_Graph::Arc * a)
  {
    a->feromone_level = 0;
    a->distance = gsl_rng_uniform(r) * Init_Ant_Arc::range + min_dist;

    if (verbose)
      cout << "(" << g.get_src_node(a)->num << " " << a->distance << " "
	   << g.get_tgt_node(a)->num << ")";
  }
};

gsl_rng * Init_Ant_Arc::r;
double Init_Ant_Arc::range;


Ant_Graph * create_random_ant_graph(const size_t & num_nodes, const double & p)
{
  Init_Ant_Arc::r = gsl_rng_alloc (gsl_rng_mt19937);
  Init_Ant_Arc::range = max_dist - min_dist + 1;

  Ant_Graph * g = 
    Random_Graph <Ant_Graph, Init_Ant_Node, Init_Ant_Arc> () (num_nodes, p);

  gsl_rng_free (Init_Ant_Arc::r);

  return g;
}


const char *argp_program_version = 
"ant 0.1\n"
"ALEPH ant based graph explorer\n"
"Copyright (C) 2007 UNIVERSITY of LOS ANDES (ULA)\n"
"Merida - REPÚBLICA BOLIVARIANA DE VENEZUELA\n"
"Center of Studies in Microelectronics & Distributed Systems (CEMISID)\n"
"ULA Computer Science Department\n"
"This is free software; There is NO warranty; not even for MERCHANTABILITY\n"
"or FITNESS FOR A PARTICULAR PURPOSE\n"
"\n";

const char *argp_program_bug_address = "leandro.r.leon@gmail.com";

static char doc[] = "ant -- Aleph ant based graph explorer\n";

static char argDoc[] = "-f input-file\n";

static char * hello = 
"\n"
"ALEPH ant based graph explorer\n"
"Copyright (C) 2007 University of Los Andes (ULA)\n"
"Merida - REPUBLICA BOLIVARIANA DE VENEZUELA\n"
"Center of Studies in Microelectronics & Distributed Systems (CEMISID)\n"
"This is free software; There is NO warranty; not even for MERCHANTABILITY\n"
"or FITNESS FOR A PARTICULAR PURPOSE\n"
"\n";

static const char license_text [] = 
"ALEPH ant based graph explorer. License & Copyright Note\n"
"Copyright (C) 2007\n"
"UNIVERSITY of LOS ANDES (ULA)\n"
"Merida - VENEZUELA\n"
"Center of Studies in Microelectronics & Distributed Systems (CEMISID)\n"
"This is free software; There is NO warranty; not even for MERCHANTABILITY\n"
"or FITNESS FOR A PARTICULAR PURPOSE\n"
"\n"
"  PERMISSION TO USE, COPY, MODIFY AND DISTRIBUTE THIS SOFTWARE AND ITS \n"
"  DOCUMENTATION IS HEREBY GRANTED, PROVIDED THAT BOTH THE COPYRIGHT \n"
"  NOTICE AND THIS PERMISSION NOTICE APPEAR IN ALL COPIES OF THE \n"
"  SOFTWARE, DERIVATIVE WORKS OR MODIFIED VERSIONS, AND ANY PORTIONS \n"
"  THEREOF, AND THAT BOTH NOTICES APPEAR IN SUPPORTING DOCUMENTATION. \n"
"\n"
"  This program is distributed in the hope that it will be useful,\n"
"  but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. \n"
"\n"
"  ULA requests users of this software to return to \n"
"      Proyecto Aleph - CEMISID Software\n"
"      Nucleo Universitario La Hechicera. Ed Ingenieria\n"
"      3er piso, ala Este \n"
"      Universidad de Los Andes \n"
"      Merida 5101 - REPÚBLICA BOLIVARIANA DE VENEZUELA \n"
"\n"
"  or to 	lrleon@ula.ve \n"
"\n"
"  any improvements or extensions that they make and grant Universidad \n"
"  de Los Andes (ULA) the full rights to redistribute these changes. \n"
"\n";

static struct argp_option options [] = {
  {"load-file", 'f', "file-name", OPTION_ARG_OPTIONAL, "load file", 0}, 
  {"save-file", 's', "file-name", OPTION_ARG_OPTIONAL, "save file", 0}, 
  {"num-ant", 'n', "value", OPTION_ARG_OPTIONAL, "number of ants", 0}, 
  {"num-mutex", 'm', "value", OPTION_ARG_OPTIONAL, "number of mutexes", 0}, 
  {"num-nodes", 'V', "value", OPTION_ARG_OPTIONAL, "number of vertex", 0}, 
  {"probability", 'P', "value", OPTION_ARG_OPTIONAL, "arc probability", 0}, 
  {"distance", 'd', "min max", OPTION_ARG_OPTIONAL, 
   "min and maximum distances", 0}, 
  {"num-threads", 'h', "value", OPTION_ARG_OPTIONAL, "number of threads", 0}, 
  {"verbose", 'v', NULL, OPTION_ARG_OPTIONAL, "verbose mode", 0}, 
  {"print-parameters", 'p', NULL, OPTION_ARG_OPTIONAL, "print parameters", 0}, 
  { 0, 0, 0, 0, 0, 0 } 
}; 

static error_t parser_opt(int key, char * arg, struct argp_state * state)
{
  switch (key)
    {
    case 'v': 
      verbose = true;
      break;

    case 'p': 
      print_pars = true;
      break;

    case 'P': 
      if (arg == NULL)
	AH_ERROR("Waiting probability in command line");
      arc_prob = atof(arg);
      break;

    case 'h': 
      if (arg == NULL)
	AH_ERROR("Waiting num threads in command line");
      num_threads = atoi(arg);
      break;

    case 'f': 
      if (arg == NULL)
	AH_ERROR("Waiting for input file name");
      load_file_name = arg;
      generate_graph = false;
      load_graph = true;
      break;

    case 's': 
      if (arg == NULL)
	AH_ERROR("Waiting file name");
      save_file_name = arg;
      save_graph = true;
      break;

    case 'r': 
      distribute_ants_randomly = true;
      break;

    case 'n': 
      if (arg == NULL)
	AH_ERROR("Waiting for num ants value in command line");
      num_ants = atoi(arg);
      break;

    case 'm': 
      if (arg == NULL)
	AH_ERROR("Waiting for num mutexes value in command line");
      num_mutex = atoi(arg);
      break;

    case 't': 
      if (arg == NULL)
	AH_ERROR("Waiting value in command line");
      initial_food = atoi(arg);
      break;

    case 'V': 
      if (arg == NULL)
	AH_ERROR("Waiting value in command line");
      num_nodes = atoi(arg);
      generate_graph = true;
      load_graph = false;
      break;

    case 'd': 
      if (arg == NULL)
	AH_ERROR("Waiting for min distance in command line");

      min_dist = atoi(arg);

      if (state->argv[state->next] == NULL)
	AH_ERROR("Waiting for max distance in command line");

      max_dist = atoi(state->argv[state->next]);

      if (min_dist >= max_dist)
	AH_ERROR("Min distance %f is greater than max %f", min_dist, max_dist);
      break;
    }
  
  return 0;
}

static struct argp argDefs = { options, parser_opt, argDoc, doc, 0, 0, 0 }; 


int main(int argn, char *argv[])
{
 argp_parse(&argDefs, argn, argv, ARGP_IN_ORDER, 0, NULL);

  cout << hello;

  if (print_pars)
    {
      print_parameters();
      return 0;
    }

  Ant_Graph * g = NULL;

  if (generate_graph)
    {
      cout << "Generating random graph ...";

      g = create_random_ant_graph(num_nodes, arc_prob);

      

      cout << endl << endl
	   << "Created an Ant random graph of " << g->get_num_nodes() 
	   << " nodes and " << g->get_num_arcs() << " arcs " << endl << endl;
    }
  else if (load_graph)
    {
      cout << "Loading graph from" << load_file_name.c_str() 
	   << " file ..." << endl;

      g = new Ant_Graph;

      ifstream input(load_file_name.c_str());

      load(*g, input);
    }


  // TODO las partes de procesamiento

  if (save_graph)
    {
      if (g == NULL)
	AH_ERROR("There is no graph to save");

      cout << "Saving graph in " << save_file_name << " ...";

      ofstream out(save_file_name.c_str(), ios::trunc);

      save(*g, out);

      return 0;
    }

  if (g != NULL)
    delete g;
}
