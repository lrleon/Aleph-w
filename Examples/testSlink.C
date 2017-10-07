
# include <aleph.H>
# include <slink.H>

# define NumItems 10

struct Record1 : public Slink
{
  unsigned n;

  Record1* get_next() { return static_cast<Record1*>(Slink::get_next()); }
};

struct Record2 
{
  unsigned n;

  Slink link;

  SLINK_TO_TYPE(Record2, link);
};
# include <iostream>


void* foo();

int main()
{
  {
    Record1 head;
    unsigned i;
    Record1 *node; 
    Slink *aux;

    for (i = 0; i < NumItems; i++)
      {
	node = new Record1;

	node->n = i;
	head.insert_next(node);
      }

    for (node = head.get_next(); node not_eq &head; node = node->get_next())
      {
	printf("%u ", node->n);
      }

    while (not head.is_empty())
      {
	aux = head.remove_next();
	delete aux;
      }
  }

  {
    Slink head;
    unsigned i;
    Record2 *node; 
    Slink *link;

    for (i = 0; i < NumItems; i++)
      {
	node = new Record2;

	node->n = i;
	head.insert_next(&node->link);
      }

    for (link = head.get_next(); link not_eq &head; link = link->get_next())
      {
	node = Record2::slink_to_type(link);
	printf("%u ", node->n);
      }

    while (not head.is_empty())
      {
	node = Record2::slink_to_type(head.remove_next());
	delete node;
      }
  }

  printf("Ended\n");
}
