
# include <aleph.H>
# include <tpl_snode.H>


# define NumItems 10


# include <iostream>

using namespace std;

int main()
{
  Snode<int> head;
  unsigned i;
  Snode<int> *node; 
  Snode<int> *aux;

  for (i = 0; i < NumItems; i++)
    {
      node = new Snode<int> (i);
      head.insert_next(node);
    }

  for (node = head.get_next(); node != &head; node = node->get_next())
    {
      cout << node->get_data() << " ";
    }

  while (not head.is_empty())
    {
      aux = head.remove_next();
      delete aux;
    }

  printf("Ended\n");
}
