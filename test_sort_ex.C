
# include <iostream>

# include <tpl_dnode.H>

# include <tpl_sort_utils.H>

using namespace std;
using namespace Aleph;

    template <typename T>
void insert_sorted(Dnode<T> & list, Dnode<T> * p)
{
  typename Dnode<T>::Iterator it(list); 
  while (it.has_current() and it.get_current()->get_data() < p->get_data()) 
    it.next();

  if (it.has_current())
    it.get_current()->append(p); // insertar a la derecha de current
  else
    list.append(p); 
}


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
void merge_lists(Dnode<T> & l1, Dnode<T> & l2,
		 Dnode<T> & result, Dnode<T> & dups)
{
      // mirar los extremos de cada lista e ir insertando ordenadamente en
      // result con el cuidado de que los repetidos se isnerten en dups
      // (ordenadamente también)
  while (not l1.is_empty() and not l2.is_empty())
    if (l1.get_first()->get_data() < l2.get_first()->get_data()) // l1 < l2
      result.append(l1.remove_first());
    else if (l2.get_first()->get_data() < l1.get_first()->get_data()) // l2 < l1
      result.append(l2.remove_first());
    else     // l1 == l2
      {
	dups.append(l2.remove_first());
	result.append(l1.remove_first());
      }

  // aquí result está completamemnte ordenada, dups contiene los
  // repetidos, ordenados también, y una de las listas l1 o l2, aún
  // contiene elementos que son todos mayores que los contenidos en
  // result. Como entre los restantes puede haber también también
  // repetidos, entonces  

  if (l1.is_empty())
    result.concat_list(&l2);
  else
    result.concat_list(&l1);
}


 // if (l1.is_empty())
 //    while (not l2.is_empty())
 //      if (result.get_last()->get_data() == l2.get_first()->get_data())
 // 	dups.append(l2.remove_last());
 //      else
 // 	result.append(l2.remove_last());
 //  else
 //    while (not l1.is_empty())
 //      if (result.get_last()->get_data() == l1.get_first()->get_data())
 // 	dups.append(l1.remove_last());
 //      else
 // 	result.append(l1.remove_last());


    template <typename T>
void __mezcla_dups(Dnode<T> & list, Dnode<T> & dups) 
{
  if (list.is_unitarian_or_empty()) 
    return;

  Dnode<T> l, r;

  list.split_list(l, r); // dividir en dos listas

  __mezcla_dups (l, dups); // ordenar la primera
  __mezcla_dups (r, dups); // ordenar la segunda

  merge_lists (l, r, list, dups); // mezclarlas 
}


    template <typename T> 
void mezcla_dups(Dnode<T> & list, Dnode<T> & dups)
{
  __mezcla_dups(list, dups);
  mergesort(dups); 
}

int main(int argn, char *argc[])
{
  int n = argc[1] ? atoi(argc[1]) : 10;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  Dnode<int> list;
  Dnode<int> dups;

  int value;

  for (int i = 0; i < n; ++i)
    {
      value = (int) (1.0*n*rand()/(RAND_MAX+1.0));

      list.append(new Dnode<int>(value));
    }

  for (Dnode<int>::Iterator it(list); it.has_current(); it.next())
    cout << it.get_current()->get_data() << " ";

  cout << endl << endl;

  //  ordena_trios_insercion(&list);  
  mezcla_dups(list, dups);
  
  cout << "lista: ";

  for (Dnode<int>::Iterator it(list); it.has_current(); it.next())
    cout << it.get_current()->get_data() << " ";

  cout << endl << endl;

  cout << "duplicados: ";
  for (Dnode<int>::Iterator it(dups); it.has_current(); it.next())
    cout << it.get_current()->get_data() << " ";

  cout << endl << endl;

  return 0;
}
