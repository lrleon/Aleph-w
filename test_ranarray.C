
# include <ran_array.h>
# include <iostream>
# include <aleph.H>

using namespace std;

void print_syntax_and_exit(char * argc[])
{
  cout << "Syntax is:" << endl
       << argc[0] << " number-of-randoms " << "limit " << "[seed]" << endl;
  exit(0);
}

int main(int argn, char * argc[])
{
  if (argn < 2)
    print_syntax_and_exit(argc);
    
  int n = atoi(argc[1]);

  if (argn < 3)
    print_syntax_and_exit(argc);
    
  long double limite = atoi(argc[2]);

  int seed = argn < 4 ? time(0) : atoi(argc[3]);

  cout << argc[0] << " " << n << " " << limite << " " << seed << endl;

  ran_start(seed);

  for (int i = 0; i < n; i++)
    {
      long value = 1 + (long) ((2.0*limite*ran_arr_next())/(RAND_MAX+1.0));
      cout << value << " ";
    }

  cout << endl;
}
