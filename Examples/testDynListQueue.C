
# include <iostream>
# include <aleph.H>
# include <tpl_dynListQueue.H>

using namespace std;

using namespace Aleph;

# define NumItems 1000

int main(int argn, char *argc[])
{
  int i, n = argc[1] ? atoi(argc[1]) : NumItems;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  DynListQueue<int> q;
  
  for (i = 0; i < n; i++)
    {
      q.put(i);
      cout << q.rear() << " ";
    }

  cout << endl;

  while (not q.is_empty())
    {
      cout << q.front() << " ";
      q.get();
    }

  cout << endl;
}
