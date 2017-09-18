
# include <iostream>
# include <tpl_dnode.H>


using namespace std;

# define DATA(p) ((p)->get_data())


void build_list(Dnode<int> * l, int n)
{
  for (int i = 0; i < n; i++)
    l->append(new Dnode<int> (rand()));
}


int select(Dnode<int> * l, int i)
{
  int lcount = 0, rcount = 0;
  Dnode<int> smaller, bigger;
  Dnode<int> * p = l->remove_next();

  /* 
     particiona la lista según pivote p en smaller, p, bigger y cuenta
     los elementos de manera que el total es lcount + 1 + rcount
     correspondientes a las longitudes de smaller, p y bigger 
  */
  while (not l->is_empty()) 
    {
      Dnode<int> *node = l->remove_next();
      if (DATA(node) < DATA(p))
	{
	  lcount++;
	  smaller.append(node);
	}
      else
	{
	  rcount++;
	  bigger.append(node);
	}
    } 

  /* En este punto lcount es la posicion del pivote */

  int result;
  if (i == lcount) 
    result = DATA(p); /* el pivote corresponde al i menor elemento */
  else if (i < lcount)
    result = select(&smaller, i); /* el i menor está en la lista smaller */
  else
    result = select(&bigger, i - (lcount + 1)); /* el i menor está en
						   la lista bigger */

      /* reconstruir lista */
  l->concat_list(&smaller);
  l->append(p);
  l->concat_list(&bigger);

  return result;
}

int acceder(Dnode<int> * l, int i)
{
  Dnode<int>::Iterator itor(l);

  for (int k = 0; k < i; k++)
    itor.next();

  return DATA(itor.get_current());
}

int main(int argn, char *argc[])
{
  int n = argc[1] ? atoi(argc[1]) : 1000;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  Dnode<int> list;

  cout << "Construyendo una lista de numeros aleatorios ...";

  build_list(&list, n);

  cout << " terminado" << endl << endl
       << "Buscando el " << n/4 << " menor elemento ...";

  int result = select(&list, n/4);

  cout << " terminado. El resultado es " << result << endl
       << "Ordenando arreglo para verificacion de correctitud ...";

  //  list.quicksort();

  cout << " terminado" << endl;

  if (result == acceder(&list, n/4))
    cout << "El resultado es " << result << endl;
  else
    cout << "Error" << endl;
}
