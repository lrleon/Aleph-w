
# include <iostream>
# include <tpl_sort_utils.H>

using namespace std;
using namespace Aleph;

# ifdef NADA

void insert_sorted(Dnode<T> & list, Dlink * p)
{
  typename Dlink::Iterator it(list); 
  while (it.has_current() and it.get_current()->get_data() < p->get_data()) 
    it.next();

  if (it.has_current())
    it.get_current()->append(p); // insertar a la derecha de current
  else
    list.append(p); 
}
# endif


    template <typename T> 
void ordena_trios_insercion(Dnode<T> * lista)
{
  Dnode<T> trios; // guarda ordenadamente elementos pos mod 3
  Dnode<T> aux; // guarda el resto de los elementos
  
  while (not lista->is_empty())
    {
          // saca de lista pos mod 3 e inserta ordenadamente en trios
      insert_sorted(trios, lista->remove_first()); 

          // Mete los siguientes 2 en aux
      if (not lista->is_empty())
	aux.append(lista->remove_first());
      if (not lista->is_empty())
	aux.append(lista->remove_first());
    }

  // aquí aux está originalmente desordenada y trios ordenada

      // construir lista con los pos mod 3 ordenados (que están en trios)
  while (not aux.is_empty() and not trios.is_empty())
    {
      lista->append(trios.remove_first());

      if (not aux.is_empty())
	lista->append(aux.remove_first());
      if (not aux.is_empty())
	lista->append(aux.remove_first());
    }
  if (aux.is_empty())
    lista->concat_list(&trios);
  else
    lista->concat_list(&aux);
}


    template <typename T>
void bubble_sort(Dnode<T> & list)
{
  Dnode<T> s;
  Dnode<T> g;
  
  while (not list.is_empty())
    {

      Dnode<T> * p = list.remove_next();

      while (not list.is_empty())
	{
	  Dnode<T> * q = list.remove_next();

	  if (p->get_data() < q->get_data())
	    std::swap(p, q);

	  s.append(q);
	}

      g.insert(p);

      list.swap(s); // intercambia las secuencias list y s en O(1)
    }

  list.swap(g); // intercambia las secuencias list y s en O(1)
}


int main(int argn, char *argc[])
{
  int n = argc[1] ? atoi(argc[1]) : 10;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  {
    Dnode<int> list;

    int value;

    for (int i = 0; i < n; ++i)
      {
	value = (int) (10.0*n*rand()/(RAND_MAX+1.0));

	list.append(new Dnode<int>(value));
      }

    for (Dnode<int>::Iterator it(list); it.has_current(); it.next())
      cout << it.get_current()->get_data() << " ";

    cout << endl << endl;

    bubble_sort(list);  

    for (Dnode<int>::Iterator it(list); it.has_current(); it.next())
      cout << it.get_current()->get_data() << " ";

    cout << endl << endl;

    Dnode<int> aux, r;

    list.split_list(aux, r);

    list.swap(r);

    if (list.is_empty())
      cout << "l1 esta vacia" << endl;
    else
      {
	cout << "l1: ";
	for (Dnode<int>::Iterator it(list); it.has_current(); it.next())
	  cout << it.get_current()->get_data() << " ";
	cout << endl << endl;
      }

    if (aux.is_empty())
      cout << "l2 esta vacia" << endl;
    else
      {
	cout << "l2: ";
	for (Dnode<int>::Iterator it(aux); it.has_current(); it.next())
	  cout << it.get_current()->get_data() << " ";
	cout << endl << endl;
      }

    mergesort(list);

    for (Dnode<int>::Iterator it(list); it.has_current(); it.next())
      cout << it.get_current()->get_data() << " ";

    cout << endl << endl;

    while (not list.is_empty())
      delete list.remove_first();
  
    while (not aux.is_empty())
      delete aux.remove_first();
  
    DynDlist<int> ll;

    for (int i = 0; i < n; ++i)
      {
	value = (int) (10.0*n*rand()/(RAND_MAX+1.0));

	ll.append(value);

	cout << value << " " ;
      }
   
    cout << endl;

    int * ptr = random_search(ll, value);

    assert(ptr != NULL and *ptr == value);

    ptr = random_select<int>(ll, n/2);

    cout << "ll[" << n/2 << "] = " << *ptr << endl;

    quicksort(ll);

    for (DynDlist<int>::Iterator it(ll); it.has_current(); it.next())
      cout << it.get_current() << " ";

    cout << endl << endl;
  }

  cout << "Probando listas simples" << endl;

  {
    DynList<int> list;

    for (int i = 0; i < n; ++i)
      {
	int value = (int) (10.0*n*rand()/(RAND_MAX+1.0));

	list.append(value);
      }
   
    cout << endl
	 << endl
	 << "ordenando por mergesort" << endl;

    mergesort(list);

    cout << "listo!" << endl;

    for (DynList<int>::Iterator it(list); it.has_current(); it.next())
      cout << it.get_curr() << " ";

    cout << endl << endl;

  }
}
