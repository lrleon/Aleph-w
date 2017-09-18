
# include <iostream>
# include <tpl_arrayQueue.H>

# define NumItems 10

using namespace std;

int main()
{
  FixedQueue<unsigned> q;
  unsigned i;

  for (i = 0; i < NumItems; i++)
    {
      q.putn(1);
      q.rear() = i;
      printf("%u ", i);
    }

  cout << endl
       << "queue = ";
  q.for_each([] (unsigned i) { cout << " " << i; });
  cout << endl;

  cout << "rear de 0: " << q.rear() << endl
       << "rear de 1: " << q.rear(1) << endl
       << "rear de 2: " << q.rear(2)<< endl
       << "rear de 3: " << q.rear(3)<< endl
       << "la cola deberia ser: " << endl;
  while (not q.is_empty())
    cout << q.get() << " ";

  cout << endl;
}
