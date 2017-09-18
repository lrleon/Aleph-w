
# include <stdlib.h>
# include <iostream>
# include <tpl_arrayStack.H>
# include <tpl_avl.H>

using namespace std;

int fib_it(int n)
{
  if (n <= 1)
    return 1;

  int fi_1 = 1, fi_2 = 1, fi = 0, i;

  for (i = 2; i <= n; i++, fi_2 = fi_1, fi_1 = fi)
    fi = fi_1 + fi_2;

  return fi;
}

int fib_rec(int n)
{
  if (n <= 1)
    return 1;

  int f1 = fib_rec(n - 1);
  int f2 = fib_rec(n - 2);
  return f1 + f2;
}

# include <fib_stack.C>

int main(int, char * argv[])
{
  int n = atoi(argv[1]);

  cout << "fib(" << n << ") = " << fib_rec(n) << " = " << fib_it(n) 
       << " = " << fib_st(n) << endl;
}
