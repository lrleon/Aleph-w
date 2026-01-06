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
 * @author Leandro Rabindranath León
 * @ingroup Ejemplos
*/

/**
 * @file timeAllTree.C
 * @brief Benchmark for all BST implementations in Aleph-w
 * 
 * This program measures insertion and deletion times for various binary
 * search tree implementations at different sizes (powers of 2).
 * 
 * Supported tree types:
 * - BinTree: Pure (unbalanced) binary search tree
 * - Avl_Tree / Avl_Tree_Rk: AVL tree (with optional rank)
 * - Splay_Tree / Splay_Tree_Rk: Splay tree (with optional rank)
 * - Treap / Treap_Rk: Treap (with optional rank)
 * - Rb_Tree / Rb_Tree_Rk: Red-Black tree (with optional rank)
 * - TdRbTree / TdRbTreeRk: Top-down Red-Black tree (with optional rank)
 * - Rand_Tree: Randomized BST
 * 
 * For each power of 2, it measures:
 * - Insertion time (min, avg, median, sigma, max)
 * - Deletion time (min, avg, median, sigma, max)
 * - Tree height
 * - Internal path length (IPL)
 * 
 * Usage: timeAllTree -n <nodes> -m <seed> [tree options]
 *        -l, --all     : Benchmark all tree types
 *        -a, --avl     : AVL tree
 *        -b, --bin     : Pure binary tree
 *        -r, --redblack: Red-Black tree
 *        -s, --splay   : Splay tree
 *        -p, --treap   : Treap
 *        -d, --rand    : Randomized tree
 * 
 * @note Requires GNU Scientific Library (GSL) for random number generation.
 */

# include <gsl/gsl_rng.h>

# include <cmath>
# include <cstring>

# include <algorithm>
# include <array>
# include <chrono>
# include <iostream>
# include <limits>
# include <tuple>
# include <vector>

# include <tpl_binNodeUtils.H>
# include <tpl_sort_utils.H>
# include <tpl_binTree.H>
# include <tpl_avl.H>
# include <tpl_treap.H>
# include <tpl_treapRk.H>
# include <tpl_avlRk.H>
# include <tpl_splay_tree.H>
# include <tpl_splay_treeRk.H>
# include <tpl_rb_tree.H>
# include <tpl_rbRk.H>

# include <tpl_tdRbTree.H>
# include <tpl_tdRbTreeRk.H>
# include <tpl_rand_tree.H>
# include <tpl_dynMapTree.H>

# include <argp.h>


using namespace std;

template <class Node>
static void printNode(Node *node, int, int)
{
  cout << "(" << node->get_key() << "," << node->get_data() << ")";
}


constexpr int Num_Samples = 37;

inline bool is_two_power(const unsigned int x)
{
  return x != 0 and not (x & (x - 1));
}

typedef tuple<double, double, double, double, double> Stat;

typedef tuple<int, double> Sample;

template <typename Key>
struct Cmp_Sample
{
  bool operator ()(const Sample & t1, const Sample & t2) const
  {
    return get<1>(t1) < get<1>(t2);
  }
};

template <template <typename, class> class TreeType,
          typename Key,
          class Compare = Aleph::less<Key>>
tuple<Stat, Stat, int, int> sample_tree(TreeType<Key, Compare> & tree,
                                        gsl_rng *r, int n, int k)
{
  cout << "Sampling at 2^" << k << " = " << n << " ..." << endl;
  typedef TreeType<Key, Compare> Tree;
  auto *p = new typename Tree::Node;

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
  double rem_time;
  double ins_avg = 0, rem_avg = 0;

  // take Num_Samples distinct values and perform 5 insertions and
  // deletion for each one
  for (int i = 0; i < Num_Samples; ++i)
    {
      int value = gsl_rng_get(r); // select a random sample not in tree
      while (tree.search(value) != nullptr)
        value = gsl_rng_get(r);
      KEY(p) = value;

      static constexpr int Num_Measures = 100;
      // take 20 mesures for the same value and average them
      double ins_time = rem_time = 0;
      for (int k = 0; k < Num_Measures; ++k)
        {
          auto sample_time = Chrono::now();
          (void) tree.insert(p);  // Ignore nodiscard - benchmarking
          auto dd = Chrono::now() - sample_time;
          ins_time += dd.count();

          sample_time = Chrono::now();
          (void) tree.remove(KEY(p));  // Ignore nodiscard - benchmarking
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
      ins_med = get<1>(ins_sample[Num_Samples / 2]),
      rem_min = get<1>(rem_sample[0]),
      rem_max = get<1>(rem_sample[Num_Samples - 1]),
      rem_med = get<1>(rem_sample[Num_Samples / 2]);

  for (int i = 0; i < Num_Samples; ++i)
    {
      double s = ins_avg - get<1>(ins_sample[i]);
      ins_desv += s * s;
      s = rem_avg - get<1>(rem_sample[i]);
      rem_desv += s * s;
    }
  ins_desv /= (Num_Samples - 1);
  rem_desv /= (Num_Samples - 1);

  ins_avg /= Num_Samples;
  rem_avg /= Num_Samples;

  ins_desv = (ins_desv - Num_Samples * ins_avg) / (Num_Samples - 1);
  rem_desv = (rem_desv - Num_Samples * rem_avg) / (Num_Samples - 1);

  delete p;

  cout << "   min Ins time = " << 1e6 * ins_min << endl
      << "    avg ins time = " << 1e6 * ins_avg << endl
      << "    med ins time = " << 1e6 * ins_med << endl
      << "    sig ins time = " << 1e6 * sqrt(ins_desv) << endl
      << "    max ins time = " << 1e6 * ins_max << endl
      << "    min Rem time = " << 1e6 * rem_min << endl
      << "    avg rem time = " << 1e6 * rem_avg << endl
      << "    med rem time = " << 1e6 * rem_med << endl
      << "    sig rem time = " << 1e6 * sqrt(rem_desv) << endl
      << "    max rem time = " << 1e6 * rem_max << endl
      << "    height = " << height << endl
      << "    ipl = " << ipl << endl
      << "done!" << endl << endl;

  return tuple<Stat, Stat, int, int>
      (Stat(ins_min, ins_avg, ins_med, ins_desv, ins_max),
       Stat(rem_min, rem_avg, rem_med, rem_desv, rem_max),
       height, ipl);
}

template <template <typename /* key */, class /* Compare */> class TreeType>
void test(unsigned long n, gsl_rng *r)
{
  typedef TreeType<int, Aleph::less<int>> Tree;
  Tree tree;

  static const size_t N = 64;
  Stat ins_stats[N], rem_stats[N];
  int height[N], ipl[N];
  size_t num_pow = 0;

  for (unsigned int i = 0; i < n; i++)
    {
      while (true)
        if (int value = gsl_rng_get(r); tree.search(value) == nullptr)
          {
            (void) tree.insert(new typename Tree::Node(value));  // Ignore nodiscard
            break;
          }

      if (is_two_power(i))
        {
          tuple<Stat, Stat, int, int> stats =
              sample_tree(tree, r, i, log(i) / log(2));
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
             1e6 * get<0>(si), 1e6 * get<1>(si), 1e6 * get<2>(si),
             1e6 * get<3>(si), 1e6 * get<4>(si),
             1e6 * get<0>(sr), 1e6 * get<1>(sr), 1e6 * get<2>(sr),
             1e6 * get<3>(sr), 1e6 * get<4>(sr));
    }
}

enum class TreeType
{
  BIN,
  AVL,
  AVL_RK,
  SPLAY,
  SPLAY_RK,
  TREAP,
  TREAP_RK,
  RB,
  RB_RK,
  TD_RB,
  TD_RB_RK,
  RAND
};

template <template <typename, class> class Tree>
void run_tree(unsigned long n, gsl_rng *r)
{
  test<Tree>(n, r);
}

struct TreeBenchmark
{
  TreeType type;
  const char *label;

  void (*runner)(unsigned long, gsl_rng *);
};

static const std::array<TreeBenchmark, 12> kBenchmarks = {
      {
        {TreeType::BIN, "BinTree", run_tree<BinTree>},
        {TreeType::AVL, "Avl_Tree", run_tree<Avl_Tree>},
        {TreeType::AVL_RK, "Avl_Tree_Rk", run_tree<Avl_Tree_Rk>},
        {TreeType::SPLAY, "Splay_Tree", run_tree<Splay_Tree>},
        {TreeType::SPLAY_RK, "Splay_Tree_Rk", run_tree<Splay_Tree_Rk>},
        {TreeType::TREAP, "Treap", run_tree<Treap>},
        {TreeType::TREAP_RK, "Treap_Rk", run_tree<Treap_Rk>},
        {TreeType::RB, "Rb_Tree", run_tree<Rb_Tree>},
        {TreeType::RB_RK, "Rb_Tree_Rk", run_tree<Rb_Tree_Rk>},
        {TreeType::TD_RB, "TdRbTree", run_tree<TdRbTree>},
        {TreeType::TD_RB_RK, "TdRbTreeRk", run_tree<TdRbTreeRk>},
        {TreeType::RAND, "Rand_Tree", run_tree<Rand_Tree>}
      }
    };

static const TreeBenchmark * find_benchmark(TreeType type)
{
  const auto it = ranges::find_if(kBenchmarks,
                                  [type](const TreeBenchmark & bench)
                                    {
                                      return bench.type == type;
                                    });
  return it == kBenchmarks.end() ? nullptr : &(*it);
}

struct Parameters
{
  long n;
  int seed;
  bool run_all = false;
  std::vector<TreeType> selected;

  Parameters(int _n, int _seed)
    : n(_n), seed(_seed)
  {
    // Empty
  }
};


const char * argp_program_version = "timeAllTree 0.0";
const char * argp_program_bug_address = "lrleon@lavabit.com";

static char doc[] = "timeAllTree -- Benchmark Aleph tree implementations";
static char argDoc[] = "-n num_nodes -m seed_for_random [tree options]\n";

enum OptionKey
{
  OPT_ALL = 'l',
  OPT_AVL_RK = 1000,
  OPT_SPLAY_RK,
  OPT_TREAP_RK,
  OPT_RB_RK,
  OPT_TD_RB,
  OPT_TD_RB_RK
};

static struct argp_option options[] = {
      {"bin", 'b', 0, OPTION_ARG_OPTIONAL, "Pure binary tree", 0},
      {"avl", 'a', 0, OPTION_ARG_OPTIONAL, "AVL tree", 0},
      {"avlrk", OPT_AVL_RK, 0, OPTION_ARG_OPTIONAL, "AVL tree (rank)", 0},
      {"splay", 's', 0, OPTION_ARG_OPTIONAL, "Splay tree", 0},
      {"splayrk", OPT_SPLAY_RK, 0, OPTION_ARG_OPTIONAL, "Splay tree (rank)", 0},
      {"redblack", 'r', 0, OPTION_ARG_OPTIONAL, "Red-black tree", 0},
      {"redblackrk", OPT_RB_RK, 0, OPTION_ARG_OPTIONAL, "Red-black tree (rank)", 0},
      {"tdrb", OPT_TD_RB, 0, OPTION_ARG_OPTIONAL, "Top-down red-black tree", 0},
      {"tdrbrk", OPT_TD_RB_RK, 0, OPTION_ARG_OPTIONAL, "Top-down red-black tree (rank)", 0},
      {"rand", 'd', 0, OPTION_ARG_OPTIONAL, "Randomized tree", 0},
      {"treap", 'p', 0, OPTION_ARG_OPTIONAL, "Treap tree", 0},
      {"treaprk", OPT_TREAP_RK, 0, OPTION_ARG_OPTIONAL, "Treap tree (rank)", 0},
      {"all", OPT_ALL, 0, OPTION_ARG_OPTIONAL, "Benchmark all tree types", 0},
      {
        "nodes", 'n', "num_nodes", OPTION_ARG_OPTIONAL,
        "Specify the number of nodes to be generated", 0
      },
      {
        "seed", 'm', "seed_for_random", OPTION_ARG_OPTIONAL,
        "Specify the seed for randon number generator", 0
      },
      {0, 0, 0, 0, 0, 0}
    };

static error_t parser_opt(int key, char *, struct argp_state *state)
{
  auto *parsPtr = static_cast<Parameters *>(state->input);

  auto add_tree = [parsPtr](TreeType type)
    {
      parsPtr->selected.push_back(type);
    };

  switch (key)
    {
    case ARGP_KEY_END:
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
      add_tree(TreeType::BIN);
      break;
    case 'a':
      add_tree(TreeType::AVL);
      break;
    case OPT_AVL_RK:
      add_tree(TreeType::AVL_RK);
      break;
    case 'r':
      add_tree(TreeType::RB);
      break;
    case OPT_RB_RK:
      add_tree(TreeType::RB_RK);
      break;
    case 's':
      add_tree(TreeType::SPLAY);
      break;
    case OPT_SPLAY_RK:
      add_tree(TreeType::SPLAY_RK);
      break;
    case 'p':
      add_tree(TreeType::TREAP);
      break;
    case OPT_TREAP_RK:
      add_tree(TreeType::TREAP_RK);
      break;
    case OPT_TD_RB:
      add_tree(TreeType::TD_RB);
      break;
    case OPT_TD_RB_RK:
      add_tree(TreeType::TD_RB_RK);
      break;
    case 'd':
      add_tree(TreeType::RAND);
      break;
    case OPT_ALL:
      parsPtr->run_all = true;
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

static struct argp argDefs = {options, parser_opt, argDoc, doc, 0, 0, 0};

int main(int argc, char *argv[])
{
  Parameters pars(1000, time(0));

  if (error_t status = argp_parse(&argDefs, argc, argv, 0, 0, &pars); status != 0)
    AH_ERROR(("Internal error"));

  unsigned long n = pars.n;

  gsl_rng *r = gsl_rng_alloc(gsl_rng_mt19937);
  gsl_rng_set(r, pars.seed % gsl_rng_max(r));

  std::vector<const TreeBenchmark *> benches;
  if (pars.run_all)
    for (const auto & bench: kBenchmarks)
      benches.push_back(&bench);
  else if (! pars.selected.empty())
    for (const TreeType type: pars.selected)
      if (const TreeBenchmark *bench = find_benchmark(type))
        benches.push_back(bench);
      else
        benches.push_back(find_benchmark(TreeType::BIN));

  benches.erase(ranges::remove(benches, nullptr).begin(),
                benches.end());

  if (benches.empty())
    AH_ERROR(("No valid tree types selected"));

  try
    {
      for (const TreeBenchmark *bench: benches)
        {
          cout << "timeAllTree<" << bench->label << "> "
              << n << " " << pars.seed << endl;
          gsl_rng_set(r, pars.seed % gsl_rng_max(r));
          bench->runner(n, r);
          cout << "timeAllTree<" << bench->label << "> "
              << n << " " << pars.seed << endl;
        }
    }
  catch (exception & e)
    {
      cout << "**** Exception: " << e.what() << endl;
    }

  gsl_rng_free(r);
}
