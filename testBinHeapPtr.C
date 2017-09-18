
# include <iostream>
# include <aleph.H>
# include <tpl_binHeap.H>
# include <tpl_binNodeUtils.H>

using namespace std;
using namespace Aleph;

struct Foo
{
  int number;

  Foo(int i) : number(i) { /* empty */ }

  bool operator < (const Foo& r)
  {
    return number < r.number;
  }

  bool operator <= (const Foo& r)
  {
    return number <= r.number;
  }
};


int main(int argn, char *argc[])
{
  int n = 1000;
  unsigned int t = time(0);

  if (argn > 1)
    n = atoi(argc[1]);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << "testBinHeap " << n << " " << t << endl;

  BinHeap<Foo*> heap;
  BinHeap<Foo*>::Node *node;
  Foo *ptr;
  int i;

  for (i = n - 1; i >= 0; i--)
    {
      ptr = new Foo (i);
      node = new BinHeap<Foo*>::Node;
      node->get_key() = ptr;
      heap.insert(node);
    }

  //  assert(heap.verify_heap());

  for (i = 0; i < n; i++)
    {
      node = heap.getMin();
      delete node->get_key();
      delete node;
    }

  //  assert(heap.verify_heap());  

  int value;

  for (i = n - 1; i >= 0; i--)
    {
      value = (int) (n*100.0*rand()/(RAND_MAX+1.0));
      ptr = new Foo (value);
      node = new BinHeap<Foo*>::Node ;
      node->get_key() = ptr;
      heap.insert(node);
    }

  //  assert(heap.verify_heap());

  for (i = 0; i < n; i++)
    {
      node = heap.getMin();
      delete node->get_key();
      delete node;
    }

  //  assert(heap.verify_heap());  

  for (i = n - 1; i >= 0; i--)
    {
      value = (int) (n*100.0*rand()/(RAND_MAX+1.0));
      ptr = new Foo (value);
      node = new BinHeap<Foo*>::Node;
      node->get_key() = ptr;
      heap.insert(node);
    }

  //  assert(heap.verify_heap());

  for (i = 0; i < n/2; i++)
    {
      node = heap.getMin();
      delete node->get_key();
      delete node;
    }
  //  assert(heap.verify_heap());  

  for (i = n - 1; i >= 0; i--)
    {
      value = (int) (n*100.0*rand()/(RAND_MAX+1.0));
      ptr = new Foo (value);
      node = new BinHeap<Foo*>::Node;
      node->get_key() = ptr;
      heap.insert(node);
    }
  //  assert(heap.verify_heap());  

  for (i = 0; i < n + n/2; i++)
    {
      node = heap.getMin();
      delete node->get_key();
      delete node;
    }
  //  assert(heap.verify_heap());  

}
