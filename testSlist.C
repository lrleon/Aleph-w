
# include <iostream>
# include <tpl_slist.H>

using namespace std;

int main()
{
  Slist<int> list;
  Slist<int>::Node *p;

  int i;

  for (i = 0; i < 20; i++)
    {
      p = new Slist<int>::Node (i);
      list.insert_first(p);
    }

  p = list.get_first(); 
  do
    {
      cout << p->get_data() << " ";
      p = p->get_next();
    } while (p not_eq &list);

  cout << endl;

  for (Slist<int>::Iterator itor(list); itor.has_current(); itor.next())
    {
      p = itor.get_current();
      cout << p->get_data() << " ";
    }

  try
    {
      while (1)
	{
	  p = list.remove_first();
	  delete p;
	}
    }
  catch (std::underflow_error)
    {
      cout << "List has been emptied"<< endl;
    }
}

  
    
