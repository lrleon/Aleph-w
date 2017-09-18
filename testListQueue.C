
# include "tpl_listQueue.H"

# define NumNodes 50

int main()
{
  ListQueue<unsigned> q;
  ListQueue<unsigned>::Node* node;
  unsigned i;
  
  for (i = 0; i < NumNodes; i++)
    {
      node = new ListQueue<unsigned>::Node (i);
      q.put(node);
      printf("%u ", q.rear()->get_data());
    }

  printf("\n");

  while (not q.is_empty())
    {
      printf("%u ", q.front()->get_data());
      node = q.get();
      delete node;
    }

  printf("Ended\n");
}
