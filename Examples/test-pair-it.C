
# include <iostream>
# include <ahFunctional.H>
# include <tpl_dynArray.H>

using namespace std;

int main()
{
  DynList<int> l = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  DynArray<int> a = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  using It1 = DynList<int>::Iterator;
  using It2 = DynArray<int>::Iterator;

  It2 it2(a);

  for (Pair_Iterator<It1, It2> it{It1(l), It2(a)}; it.has_curr(); it.next())
    ;

  // for (Pair_Iterator<It1, It2> it(It1(l), it2); it.has_curr(); it.next())
    // cout << it.get_curr().first << " " << it.get_curr().second << endl;
}
