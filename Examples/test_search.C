
# include <iostream>
# include <tpl_sort_utils.H>

using namespace std;

int main()
{
  {
    Dnode<int> list;

    for (int i = 0; i < 100; i++)
      list.insert( new Dnode<int> (i) );

    sequential_search<int, Aleph::less<int> >(list, 10);
  }

  {
    DynDlist<int> list;

    for (int i = 0; i < 100; i++)
      list.insert(i);

    int * ptr = sequential_search (list, 10);

    cout << *ptr << endl;

    ptr = search_extreme(list);

    cout << *ptr << endl;

    ptr = search_max(list);

    cout << *ptr << endl;
  }

}
