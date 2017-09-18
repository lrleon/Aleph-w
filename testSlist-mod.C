# include "tpl_slist_modified.H"
# include <iostream>

using namespace std;
using namespace Aleph;

int main()
{
  SlistVtl<int> l1;
  int x;
  for (int i = 0; i < 10; i++)
    {
      cin >> x;
      SlistVtl<int>::Node * n = new SlistVtl<int>::Node(x);
      l1.insert_first(n);
    }
  for (SlistVtl<int>::Iterator it(l1); it.has_current(); it.next())
    cout << "Item " << it.get_current()->get_data() << endl;
  while(not l1.is_empty())
    {
      cout << "Liberando " << l1.get_first()->get_data() << endl;
      delete l1.remove_first();
    }
  return 0;
}

