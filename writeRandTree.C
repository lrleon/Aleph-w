
# include <stdlib.h>
# include <iostream>
# include <fstream>
# include <time.h>
# include <tpl_dynArray.H>
# include <tpl_rand_tree.H>
# include <tpl_binNodeUtils.H>

using namespace Aleph;

gsl_rng * r = nullptr;

using Tree = Rand_Tree<unsigned long>;
using Node = Tree::Node;

Tree create_tree(unsigned long n)
{
  Tree tree;

  for (long i = 0; i < n; ++i)
    tree.insert_dup(new Node(gsl_rng_get(r)));

  assert(tree.size() == n);
  assert(tree.verify());
  assert(check_bst(tree.getRoot()));

  return tree;
}

void usage()
{
  cout << "usage: " << endl
       << "    ./test n seed" << endl
       << endl;
  exit(0);
}

int main(int argn, char *argc[])
{
  if (argn != 3)
    usage();

  unsigned long n = atoi(argc[1]);
  unsigned long seed = atoi(argc[2]);
  r = gsl_rng_alloc (gsl_rng_mt19937);
  gsl_rng_set(r, seed % gsl_rng_max(r));

  cout << argc[0] << " " << n << " " << seed << endl;

  Tree t1 = create_tree(n);
  Tree t2 = create_tree(n);

  t1.join_dup(t2);

  assert(t1.size() == 2*n and t2.size() == 0);

  destroyRec(t1.getRoot());
  gsl_rng_free(r);
}
