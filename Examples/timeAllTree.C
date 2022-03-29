

/*
                          Aleph_w

  Data structures & Algorithms
  version 1.9d
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2022 Leandro Rabindranath Leon

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
> class TreeType>
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
