
# include <iostream>
# include <tpl_dnode.H>

using namespace std;

void escribir(Dnode<int> & list)
{
  for (Dnode<int>::Iterator it(&list); it.has_current(); it.next())
    cout << it.get_current()->get_data() << " ";
  
  cout << endl;
}


void invertir(Dlink & l)
{
  if (l.is_empty())
    return;

  Dlink tmp_head;

  while (not l.is_empty())
    {
      Dlink * current = l.get_next();

      current->del();
      tmp_head.insert(current);
    }  

  tmp_head.swap(&l);
}



int main(int argn, char *argc[])
{
  int i, n = argc[1] ? atoi(argc[1]) : 1000;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  Dnode<int> list;
  Dnode<int> * p;
  for (i = 0; i < n; i++)
    {
      p = new Dnode<int> (i);
      list.append(p);
    }

  escribir(list);

  invertir(list);

  escribir(list);

  list.remove_all_and_delete();

  cout << argc[0] << " " << n << " " << t << endl;
}
