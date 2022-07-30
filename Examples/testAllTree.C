
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
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
# include <iostream>
# include <aleph.H>
# include <tpl_binTree.H>
# include <tpl_avl.H>
# include <tpl_treap.H>
# include <tpl_splay_tree.H>
# include <tpl_rb_tree.H>
# include <tpl_rand_tree.H>
# include <tpl_dynMapTree.H>
# include <ran_array.h>
# include <time.h>
# include <gsl/gsl_rng.h>
# include <argp.h>

using namespace std;

    template <class Node>
static void printNode(Node* node, int, int)
{
  cout << "(" << node->get_key() << "," << node->get_data() << ")";
}


    template <
      template <typename /* key */, class /* Compare */>
    class TreeType>
void test(unsigned long n, gsl_rng * r)
{
  unsigned long max = 100*n;
  size_t i;
  int value;
  DynMapTree<int, int, TreeType> tree;

  for (i = 0; i < n; i++)
    {
      value = gsl_rng_uniform_int(r, max);
      tree.insert(value, i);
    }

  cout << "Reading test ... " << endl;

  int content;

  for (i = 0; i < n; i++)
    {
      value = gsl_rng_uniform_int(r, max);
      try
	{
	  content = tree.find(value);
	  //	  cout << "(" << value << "," << content << ")";
	}
      catch (...)
	{
	  ;//cout << i << ".";  
	}
    }
  
  cout << endl;

  cout << "Writing test ... " << endl;

  for (i = 0; i < n; i++)
    {
      value = gsl_rng_uniform_int(r, max);
      auto val = tree.insert(value, i);
      cout << "(" << val << "," << i << ")";
    }
  
  cout << endl;

  cout << "The path length is " << tree.internal_path_length() << endl;

  cout << "The height is " << tree.height() << endl;

  unsigned int insCount = tree.size();
  cout << insCount << " Items inserted" << endl;

  for (i = 0; i < n; i++)
    {
      value = gsl_rng_uniform_int(r, max);
      tree.remove(value);
    }

  cout << insCount - tree.size() << " Items removed" << endl;
}

enum TreeType 
{ 
  INVALID, BIN, AVL, SPLAY, TREAP, RB, RAND 
};

struct Parameters
{
  int      n;
  int      seed;
  TreeType type;
  char     *str;

  Parameters(int _n, int _seed)
    : n(_n), seed(_seed), type(INVALID), str(NULL)
    {
      // Empty 
    }    
};
  

const char *argp_program_version = "testAllTree 0.0";
const char *argp_program_bug_address = "aleph-bugs@aleph.ula.ve";

static char doc[] = "testAllTree -- A tester for all binary trees";
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

  cout << "testAllTree<" << pars.str << "> " << n << " " << pars.seed
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

      cout << "testAllTree<" << pars.str << "> " << n << " " << pars.seed
	   << endl; 
    }
  catch (exception & e)
    {
      cout << "**** Exception: " << e.what() << endl;
    }
  
  gsl_rng_free(r);
}
