# include <iostream>
# include <tpl_dynArray.H>


int main(int argn, char *argc[])
{
  unsigned long n = argn > 1 ? atoi(argc[1]) : 1000;
  unsigned int  t = argn > 2 ? atoi(argc[2]) : time(NULL);
  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  DynArray<size_t> array;

}
