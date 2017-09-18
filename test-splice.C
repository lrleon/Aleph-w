
# include <iostream>
# include <tpl_dnode.H>

using namespace std;

using Node = Dnode<long>;

static long counter = 0;

Node * create_list_with_header(size_t n)
{
  Node * head = new Node;
  for (auto i = 0; i < n; ++i)
    head->append(new Node(counter++));

  return head;
}

Node * create_list_without_header(size_t n)
{
  Node * p = new Node(1000 + counter++);
  for (auto i = 1; i < n; ++i)
    p->append(new Node(1000 + counter++));
  return p;
}

Node * access(Node * list, size_t n)
{
  Node::Iterator it(list); 
  for (auto i = 0; i < n; ++i)
    it.next();
  return it.get_curr();
}

ostream & operator << (ostream & out, Node * p)
{
  for (Node::Iterator it(p); it.has_curr(); it.next())
    out << it.get_curr()->get_data() << " ";
  return out;
}


int main(int argc, char *argv[])
{
  if (argc != 4)
    {
      cout << "usage: " << argv[0] << " n m i" << endl
	   << endl
	   << "Where n: number of items of big list" << endl
	   << "      m: number of items of sublist to be inserted" << endl
	   << "      i: position where the sublist will be inserted" << endl;
      exit(0);
    }

  size_t n = atoi(argv[1]);
  size_t m = atoi(argv[2]);
  size_t i = atoi(argv[3]);

  Node * list = create_list_with_header(n);

  access(list, i)->splice(create_list_without_header(m));

  cout << list << endl;

  list->remove_all_and_delete();
  delete list;
}
