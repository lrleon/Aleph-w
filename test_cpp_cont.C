
# include <iostream>
# include <Set.H>
# include <Stack.H>
# include <Queue.H>
# include <Priority_Queue.H>

using namespace std;
using namespace Aleph;

int main(int argn, char *argc[])
{
  int n = argc[1] ? atoi(argc[1]) : 10;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  set<int> s;

  for (int i = 0; i < n; ++i)
    s.insert(i);

  //  stack<int> st(s);

  //  queue<int> q(s);

  priority_queue<int> pq(s);
}
