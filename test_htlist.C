
# include <iostream>
# include <htlist.H>
# include <filter_iterator.H>

using namespace std;

using namespace Aleph;

void print(HTList & l)
{
  cout << "l: ";
  for (HTList::Iterator it(l); it.has_curr(); it.next())
    {
      Snodenc<int> * p = (Snodenc<int>*) it.get_curr();
      cout << p->get_data() << " ";
    }
  cout << endl;
}

struct Par
{
  bool operator () (int i) const
  {
    return i % 2 == 0;
  }
};

typedef Filter_Iterator<DynList<int>, DynList<int>::Iterator, Par> It;


int main(int argn, char * argc[])
{
  size_t n = 10;
  if (argn > 1)
    n = atoi(argc[1]);

  HTList list;

  for (int i = 0; i < n; ++i)
    list.append(new Snodenc<int>(i));

  while (not list.is_empty())
    delete list.remove_first();


  for (int i = 0; i < n; ++i)
    list.append(new Snodenc<int>(i));

  HTList l1, l2;

  size_t sz = list.split_list(l1, l2);

  cout << "Lista de " << sz << " elementos partida en dos" << endl;
  print(l1);
  cout << endl;

  print(l2);
  cout << endl;

  list.append(l2);
  print(list);
  list.insert(l1);

  print(list);

  list.reverse();
  print(list);

  HTList::Iterator it(list);
  for (int i = 0; it.has_curr() and i < n/10; it.next(), ++i)
    ;

  list.cut(it.get_curr(), l1);

  print(list);
  cout << endl;

  print(l1);
  cout << endl;

  list.remove_all_and_delete();
  l1.remove_all_and_delete();

  cout << "****************" << endl;

  DynList<int> l = {7};

  for (int i = 0; i < n; ++i)
    l.append(i);

  DynList<int> ll = {2};

  ll = l ;

  cout << "Mostrando todos los elementos .. " << endl;
  for (DynList<int>::Iterator it(ll); it.has_curr(); it.next())
    cout << it.get_curr() << " " ;
  cout << endl;

  for (It it(ll); it.has_curr(); it.next())
     cout << it.get_curr() << " " ;
  cout << endl;

  DynList<int> lll = l;

  lll.reverse();

  l.append(l);

  l.append(ll);

  l.append(DynList<int>(l));

  l.insert(DynList<int>(lll));

  print(l);
}
