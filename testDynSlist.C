
# include <iostream>
# include <aleph.H>
# include <tpl_dynSlist.H>

# define Num_Items 10

using namespace std;

int main()
{
  DynSlist<int> list;
  DynSlist<int*> list_ptr;

  for (int i = 0; i < Num_Items; i++)
    {
      list.insert(i, i);
      list_ptr.insert(i, &i);
    }

  DynSlist<int>::Iterator itor(list);

  DynSlist<int>::Iterator itor_aux(list);

  itor_aux = itor;

  for (/* nothing */; itor.has_current(); itor.next())
    cout << itor.get_current() << " ";

  cout << endl;

  for (int i = Num_Items; i < 2*Num_Items; i++)
    list.insert(i - Num_Items, i);

  for (itor.reset_first(); itor.has_current(); itor.next())
    cout << itor.get_current() << " ";

  cout << endl;

  for (int i = 2*Num_Items; i < 3*Num_Items; i++)
    list.insert(0, i);

  for (itor.reset_first(); itor.has_current(); itor.next())
    cout << itor.get_current() << " ";

  cout << endl;

  for (int i = 3*Num_Items; i < 4*Num_Items; i++)
    list.insert(list.size(), i);

  for (int i = 0; i < list.size(); i++)
    cout << list[i] << " ";

  cout << endl;

  for (int i = 0; i < list.size(); i++)
    cout << list[list.size() - i - 1] << " ";

  cout << endl;

  list.remove(list.size() - 1);

  list.remove(0);

  for (itor.reset_first(); itor.has_current(); itor.next())
    cout << itor.get_current() << " ";

  cout << endl;

}
