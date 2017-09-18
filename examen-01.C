
# include <iostream>
# include <tpl_dnode.H>

using namespace std;


bool similar_ordenado(Dnode<int> & l1, Dnode<int> & l2)
{
  Dnode<int>::Iterator it1(l1);
  Dnode<int>::Iterator it2(l2);

  while (it1.has_current() and it2.has_current())
    {
      if (it1.get_current()->get_data() != it2.get_current()->get_data())
	return false;

      it1.next();
      it2.next();
    }

  return (not it1.has_current()) and  (not it2.has_current());
}


Dnode<int> * buscar(Dnode<int> & l, const int & item)
{
  for (Dnode<int>::Iterator it(l); it.has_current(); it.next())
    if (it.get_current()->get_data() == item)
      return it.get_current();

  return NULL;
}

bool similar_no_ordenado(Dnode<int> & l1, Dnode<int> & l2)
{
  Dnode<int> l2_aux; //lista auxiliar para salvar nodos de l2

      // para cada nodo de l1, verificar si se encuentra en l2
  for (Dnode<int>::Iterator it(l1); it.has_current(); it.next())
    {
      Dnode<int> * p_l2 = buscar(l2, it.get_current()->get_data());

      if (p_l2 == NULL)
	return false; // no se encontró ==> listas son diferentes

      p_l2->del(); // borrar nodo encontrado de l2
      l2_aux.append(p_l2);; // insertarlo en lista auxiliar
    }

      // si las listas son iguales ==> l2 debe estar vacía
  const bool ret_val = l2.is_empty(); // valor de retorno de la función

      // reconstruir lista l2
  l2.concat_list(&l2_aux);

  return ret_val;
}


void invertir(int A[], int l, int r)
{
  while (l < r)
    std::swap(A[l++], A[r--]);
}


void rotar(int S[], const int & n, const int & m)
{
  invertir(S, 0, n - m - 1);
  invertir(S, n - m, n - 1);
  invertir(S, 0, n - 1);
}

int alea()
{
  return 1+ (int) (1000.0*rand()/(RAND_MAX+1.0));
}


Dnode<int> * hacer_lista(int n)
{
  Dnode<int> * l = new Dnode<int>;

  for (int i = 0; i < n; ++i)
    l->insert(new Dnode<int>(alea()));

  return l;
}

    template <typename T> 
void deshacer_lista(Dnode<T> *& l)
{
  l->remove_all_and_delete();

  delete l;

  l = NULL;
}

    template <typename T> 
Dnode<T> * copiar(Dnode<T> * l)
{
  Dnode<int> * r = new Dnode<int>;

  for (typename Dnode<T>::Iterator it(*l); it.has_current(); it.next())
    r->append(new Dnode<int>(it.get_current()->get_data()));

  return r;
}

    template <typename T> 
void print_list(Dnode<T> * l)
{
  for (typename Dnode<T>::Iterator it(*l); it.has_current(); it.next())
    cout << it.get_current()->get_data() << " ";
  cout << endl;
}



    template <typename T> 
bool contienen_los_mismos(Dnode<T> * l1, Dnode<T> * l2)
{
  Dnode<T> aux; // lista vacía auxiliar donde se colocan elementos de l1

  bool iguales = true;

 mirar_proximo_de_l1:
  while (iguales and (not l1->is_empty()))
    {
      Dnode<T> * p = l1->remove_next(); // nodo a examinar si está en l2
      T & data = p->get_data();         // salvar dato
      aux.append(p);                    // salvar nodo en lista auxiliar

          // buscar p->get_data() en l2
      for (typename Dnode<T>::Iterator it(l2); it.has_current(); it.next())
	if (it.get_current()->get_data() == data)
	  goto mirar_proximo_de_l1;

      iguales = false;
    }

  aux.concat_list(l1); // restarurar l1 a su estado original
  aux.swap(l1);

  return iguales;
}

    template <typename T> 
bool hay_repetidos(Dnode<T> * l)
{
  Dnode<T> aux; // lista vacía auxiliar donde se colocan elementos de l1

  bool repetidos = false;

  while (not repetidos and not l->is_empty())
    {
      Dnode<T> * p = l->remove_next(); // nodo a examinar si está repetido
      T & data = p->get_data();         // salvar dato
      aux.append(p);                    // salvar nodo en lista auxiliar

          // buscar p->get_data() en los nodos restantes
      for (typename Dnode<T>::Iterator it(l); it.has_current(); it.next())
	if (it.get_current()->get_data() == data)
	  {
	    repetidos = true;
	    break;
	  }      
    }

  aux.concat_list(l); // restarurar l a su estado original
  aux.swap(l);

  return repetidos;
}

int main()
{
  srand(time(0));

  Dnode<int> * l1 = hacer_lista(10);
  print_list(l1);

  if (hay_repetidos(l1))
    cout << "hay repetidos" << endl;
  else
    cout << "no hay repetidos" << endl;

  Dnode<int> * l2 = hacer_lista(10);
  // Dnode<int> * l2 = copiar(l1);
  print_list(l2);

  if (contienen_los_mismos(l1, l2))
    cout << "Las listas son iguales " << endl;
  else
    cout << "Las listas no son iguales " << endl;

  print_list(l1);
  print_list(l2);
  deshacer_lista(l1);
  deshacer_lista(l2);
}
