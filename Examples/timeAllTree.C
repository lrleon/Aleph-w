
# include <gsl/gsl_rng.h>

# include <cmath>
# include <cstring>

# include <limits>
# include <iostream>
# include <tuple>
# include <chrono>

# include <tpl_binNodeUtils.H>
# include <tpl_sort_utils.H>
# include <tpl_binTree.H>
# include <tpl_avl.H>
# include <tpl_treap.H>
# include <tpl_splay_tree.H>
# include <tpl_rb_tree.H>
# include <tpl_rand_tree.H>
# include <tpl_dynMapTree.H>

# include <argp.h>


using namespace std;

    template <class Node>
static void printNode(Node* node, int, int)
{
  cout << "(" << node->get_key() << "," << node->get_data() << ")";
}


const int Num_Samples = 37;

inline bool is_two_power(unsigned int x)
{
  return ((x != 0) && !(x & (x - 1)));
}

typedef tuple<double, double, double, double, double> Stat;

typedef tuple<int, double> Sample;

template <typename Key>
struct Cmp_Sample
{
  bool operator () (const Sample & t1, const Sample & t2) const
  {
    return get<1>(t1) < get<1>(t2);
  }
};

template <template <typename, class> class TreeType,
	  typename Key,
	  class Compare = Aleph::less<Key>>
tuple<Stat, Stat, int, int> sample_tree(TreeType<Key, Compare> & tree, 
					gsl_rng * r, int n, int k)
{
  cout << "Sampling at 2^" << k << " = " << n << " ..." << endl;
  typedef TreeType<Key, Compare> Tree;
  typename Tree::Node * p = new typename Tree::Node;

  cout << "    Computing height ..." << endl;
  size_t height = computeHeightRec(tree.getRoot());
  cout << "    done = " << height << endl
       << endl
       << "    Computing IPL ..." << endl;
  size_t ipl = internal_path_length(tree.getRoot());
  cout << "    done = " << ipl << endl
       << endl;

  // The Num_Samples times are stored in a array
  using Chrono = std::chrono::high_resolution_clock;
  Sample ins_sample[Num_Samples], rem_sample[Num_Samples];
  double ins_time, rem_time;
  double ins_avg = 0, rem_avg = 0;

  // take Num_Samples distint values and perform 5 insertions and
  // deletion for each one
  for (int i = 0; i < Num_Samples; ++i)
    {
      int value = gsl_rng_get(r);         // select a random sample not in tree
      while (tree.search(value) != NULL)
	value = gsl_rng_get(r);
      KEY(p) = value;

      static const int Num_Measures = 100;
      // take 20 mesures for the same value and average them
      ins_time = rem_time = 0;
      for (int k = 0; k <Num_Measures; ++k)
	{
	  auto sample_time = Chrono::now();
	  tree.insert(p);
	  auto dd = Chrono::now() - sample_time;
	  ins_time += dd.count();

	  sample_time = Chrono::now();
	  tree.remove(KEY(p));
	  rem_time += (Chrono::now() - sample_time).count();
	}

      ins_time /= Num_Measures;
      rem_time /= Num_Measures;

      ins_sample[i] = Sample(value, ins_time);
      rem_sample[i] = Sample(value, rem_time);
      ins_avg += ins_time;
      rem_avg += rem_time;
    }

  Aleph::insertion_sort<Sample, Cmp_Sample<int>>
    (ins_sample, 0, Num_Samples - 1);
  Aleph::insertion_sort<Sample, Cmp_Sample<int>>
    (rem_sample, 0, Num_Samples - 1);

  double ins_desv = 0, rem_desv = 0,
    ins_min = get<1>(ins_sample[0]), 
    ins_max = get<1>(ins_sample[Num_Samples - 1]),
    ins_med = get<1>(ins_sample[Num_Samples/2]),
    rem_min = get<1>(rem_sample[0]), 
    rem_max = get<1>(rem_sample[Num_Samples - 1]),
    rem_med = get<1>(rem_sample[Num_Samples/2]);

  for (int i = 0; i < Num_Samples; ++i)
    {
      double s = ins_avg - get<1>(ins_sample[i]);
      ins_desv += s*s;
      s = rem_avg - get<1>(rem_sample[i]);
      rem_desv += s*s;
    }
  ins_desv /= (Num_Samples - 1);
  rem_desv /= (Num_Samples - 1);

  ins_avg /= Num_Samples;
  rem_avg /= Num_Samples;
    
  ins_desv = (ins_desv - Num_Samples*ins_avg)/(Num_Samples - 1);
  rem_desv = (rem_desv - Num_Samples*rem_avg)/(Num_Samples - 1);
  
  delete p;

  cout << "    min Ins time = " << 1e6*ins_min << endl
  << "    avg ins time = " << 1e6*ins_avg << endl
  << "    med ins time = " << 1e6*ins_med << endl
  << "    sig ins time = " << 1e6*sqrt(ins_desv) << endl
  << "    max ins time = " << 1e6*ins_max << endl
  << "    min Rem time = " << 1e6*rem_min << endl
  << "    avg rem time = " << 1e6*rem_avg << endl
  << "    med rem time = " << 1e6*rem_med << endl
  << "    sig rem time = " << 1e6*sqrt(rem_desv) << endl
  << "    max rem time = " << 1e6*rem_max << endl
  << "    height = " << height << endl
  << "    ipl = " << ipl << endl
  << "done!" << endl << endl;

  return tuple<Stat, Stat, int, int>
    (Stat(ins_min, ins_avg, ins_med, ins_desv, ins_max),
     Stat(rem_min, rem_avg, rem_med, rem_desv, rem_max),
     height, ipl);
}

template < template <typename /* key */, class /* Compare */> class TreeType>
void test(unsigned long n, gsl_rng * r)
{
  unsigned int i;
  int value;
  typedef TreeType<int, Aleph::less<int>> Tree;
  Tree tree;

  static const size_t N = 64;
    Stat ins_stats[N], rem_stats[N];
    int height[N], ipl[N];
  size_t num_pow = 0;

  for (i = 0; i < n; i++)
    {
      while (true)
	{
	  value = gsl_rng_get(r);
	  if (tree.search(value) == NULL)
	    {
	      tree.insert(new typename Tree::Node(value));
	      break;
	    }
	}

      if (is_two_power(i))
	{
	  tuple<Stat, Stat,int,int> stats = 
	    sample_tree(tree, r, i, log(i)/log(2)); 
	  ins_stats[num_pow] = get<0>(stats);
	  rem_stats[num_pow] = get<1>(stats);
	  height[num_pow] = get<2>(stats);
	  ipl[num_pow] = get<3>(stats);
	  ++num_pow;
	}
    }

  destroyRec(tree.getRoot());

  printf("#2^k      n   h   ipc     [min  ins  med  sigma  max]    [min  ins  med  sigma  max]\n");
  for (int i = 0; i < num_pow; ++i)
    {
      Stat & si = ins_stats[i];
      Stat & sr = rem_stats[i];
      printf("%02d %08d  %02d  %08d  %02.2f %02.2f %02.2f %02.2f %02.2f"
	     "        %02.2f %02.2f %02.2f %02.2f %02.2f\n",
	     i, (int) pow(2, i), height[i], ipl[i],
	     1e6*get<0>(si), 1e6*get<1>(si), 1e6*get<2>(si), 
	     1e6*get<3>(si), 1e6*get<4>(si),
	     1e6*get<0>(sr), 1e6*get<1>(sr), 1e6*get<2>(sr), 
	     1e6*get<3>(sr), 1e6*get<4>(sr));
    }
}

enum TreeType 
{ 
  INVALID, BIN, AVL, SPLAY, TREAP, RB, RAND 
};

struct Parameters
{
  long      n;
  int      seed;
  TreeType type;
  char     *str;

  Parameters(int _n, int _seed)
    : n(_n), seed(_seed), type(INVALID), str(NULL)
    {
      // Empty 
    }    
};
  

const char *argp_program_version = "timeAllTree 0.0";
const char *argp_program_bug_address = "lrleon@lavabit.com";

static char doc[] = "timeAllTree -- A tester for all binary trees";
static char argDoc[] = "-n num_nodes -m seed_for_random -<tree type>\n"
;

static struct argp_option options [] = {
  { "bin",  'b', 0, OPTION_ARG_OPTIONAL, "pure binary tree" , 0},
  { "avl",  'a', 0, OPTION_ARG_OPTIONAL, "avl tree" , 0},
  { "splay",  's', 0, OPTION_ARG_OPTIONAL, "splay tree" , 0},
  { "redblack",  'r', 0, OPTION_ARG_OPTIONAL, "red black tree" , 0},
  { "rand",  'd', 0, OPTION_ARG_OPTIONAL, "randomized tree" , 0},
  { "treap",  'p', 0, OPTION_ARG_OPTIONAL, "treap tree" , 0},
  { "nodes", 'n', "num_nodes", OPTION_ARG_OPTIONAL, 
    "Specify the number of nodes to be generated", 0 },
  { "seed",  'm', "seed_for_random", OPTION_ARG_OPTIONAL, 
    "Specify the seed for randon number generator", 0},
  { 0, 0, 0, 0, 0, 0 } 
}; 

static error_t parser_opt(int key, char *, struct argp_state *state)
{
  Parameters *parsPtr = static_cast<Parameters*>(state->input);

  switch (key)
    {
    case ARGP_KEY_END:
      if (parsPtr->type == INVALID)
	argp_usage(state);
      break;
    case 'n': 
      char *end;
      if (state->argv[state->next] == NULL)
	argp_usage(state);
      parsPtr->n = strtol(state->argv[state->next], &end, 10);
      if (*end != '\0' && *end != '\n')
	argp_usage(state);
      state->next++;
      break;
    case 'b':
      parsPtr->type = BIN;
      parsPtr->str = "BinTree";
      break;
    case 'a':
      parsPtr->type = AVL;
      parsPtr->str = "AvlTree";
      break;
    case 'r':
      parsPtr->type = RB;
      parsPtr->str = "RbTree";
      break;
    case 's':
      parsPtr->type = SPLAY;
      parsPtr->str = "SplayTree";
      break;
    case 'p': 
      parsPtr->type = TREAP;
      parsPtr->str = "Treap";
      break;
    case 'd': 
      parsPtr->type = RAND;
      parsPtr->str = "Randomized";
      break;
    case 'm': 
      if (state->argv[state->next] == NULL)
	argp_usage(state);
      parsPtr->seed = strtol(state->argv[state->next], &end, 10);
      if (*end != '\0' && *end != '\n')
	argp_usage(state);
      state->next++;
      break;
    case ARGP_KEY_ARG:
    default: return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argDefs = { options, parser_opt, argDoc, doc, 0, 0, 0 };

int main(int argc, char *argv[])
{ 
  Parameters pars(1000, time(0));
  
  error_t status = argp_parse(&argDefs, argc, argv, 0, 0, &pars);

  if (status != 0)
    AH_ERROR( ("Internal error") );

  if (pars.type == INVALID)
    AH_ERROR( ("Invalid tree type" ) );

  unsigned long n = pars.n;

  gsl_rng * r = gsl_rng_alloc (gsl_rng_mt19937);
  gsl_rng_set(r, pars.seed % gsl_rng_max(r));

  cout << "timeAllTree<" << pars.str << "> " << n << " " << pars.seed
       << endl; 

  try
    {
      switch (pars.type)
	{
	case BIN:      
	  test<BinTree>(n, r); 
	  break;
	case AVL:      
	  test<Avl_Tree>(n, r); 
	  break;
	case TREAP:    
	  test<Treap>(n, r); 
	  break;
	case RAND:    
	  test<Rand_Tree>(n, r); 
	  break;
	case SPLAY:    
	  test<Splay_Tree>(n, r); 
	  break;
	case RB:  
	  test<Rb_Tree>(n, r); 
	  break; 
	case INVALID:
	default: AH_ERROR("Invalid tree type %d", pars.type);
	}

      cout << "timeAllTree<" << pars.str << "> " << n << " " << pars.seed
	   << endl; 
    }
  catch (exception & e)
    {
      cout << "**** Exception: " << e.what() << endl;
    }
  
  gsl_rng_free(r);
}
