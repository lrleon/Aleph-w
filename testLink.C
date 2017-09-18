
# include "slink.H"

# define N 10

struct Record : public Slink
{
  unsigned n;

  Record* get_next() { return static_cast<Record*>(Slink::get_next()); }
};

# include <iostream>



void* foo();

int main()
{
    DebugAlloc myAlloc;
    heapAlloc = &myAlloc;
    Record head;
    unsigned i;
    Record*  node;

    for (i = 0; i < N; i++)
      {
	node = new Record;

	node->n = i;
	head.insert(node);
      }

    for (node = head.get_next(); /* empty */; node = node->get_next())
      {
	printf("%u ", node->n);
      }

    printf("Ended\n");
}
