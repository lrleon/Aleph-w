
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
# ifndef TPL_SORT_UTILS_H
# define TPL_SORT_UTILS_H

# include <ahUtils.H>
# include <ahFunctional.H>
# include <tpl_arrayStack.H>
# include <tpl_array.H>
# include <tpl_dynArray.H>
# include <tpl_dynDlist.H>
# include <htlist.H>

namespace Aleph 
{
  extern size_t Insertion_Threshold;

  extern size_t Quicksort_Threshold;

  extern const int Not_Found;

  /** Return true if the sequence is sorted

     \ingroup Algos
   */
  template <template <typename> class Container, typename T,
	    class Compare = Aleph::less<T>>
bool is_sorted(const Container<T> & cont, Compare cmp = Compare())
{
  if (cont.is_empty())
    return true;

  T item = cont.get_first();
  typename Container<T>::Iterator it(cont); it.next_ne();
  for (; it.has_curr(); it.next_ne())
    {
      const T & curr = it.get_curr_ne();
      if (not cmp(item, curr))
	return false;

      item = curr;
    }
  return true;
}


  /** Check if container is sorted.

      @return a pair whose first element says whether the array is or
      not sorted. If the array is not sorted, then second contains the
      index where the inversion was detected. Otherwise, second
      contains the number of elements that the container has.

      \ingroup Algos
   */
template <template <typename> class Container, typename T,
	    class Compare = Aleph::less<T>>
pair<bool, size_t> search_inversion(const Container<T> & cont,
				    Compare cmp = Compare())
{
  if (cont.is_empty())
    return make_pair(true, 0);

  T item = cont.get_first();
  typename Container<T>::Iterator it(cont); it.next_ne();
  size_t n = 0;
  for (; it.has_curr(); it.next_ne(), ++n)
    {
      const T & curr = it.get_curr_ne();
      if (not cmp(item, curr))
	return make_pair(false, n);

      item = curr;
    }
  return make_pair(true, n);
}
  


    /** Return true if the sequence is inversely sorted

	\ingroup Algos
   */
  template <template <typename> class Container, typename T,
	    class Compare = Aleph::less<T>>
bool is_inversely_sorted(const Container<T> & cont, Compare cmp = Compare())
{
  if (cont.is_empty())
    return true;

  T item = cont.get_first();
  typename Container<T>::Iterator it(cont); it.next_ne();
  for (; it.has_curr(); it.next_ne())
    {
      const T & curr = it.get_curr_ne();
      if (not cmp(curr, item))
	return false;

      item = curr;
    }
  return true;
}

 /** Return a pair where first is `true` if the sequence is sorted. If the
     sequence is not sorted, then first is `false` and second contains
     the index where the test was failed

     \ingroup Algos
   */
  template <template <typename> class Container, typename T,
	    class Compare = Aleph::less<T>>
  pair<bool, size_t>
test_sorted(const Container<T> & cont, Compare cmp = Compare()) noexcept
{
  if (cont.is_empty())
    return pair<bool, size_t>(true, 0);

  T item = cont.get_first();
  typename Container<T>::Iterator it(cont); it.next_ne();
  for (size_t i = 1; it.has_curr(); it.next_ne(), ++i)
    {
      const T & curr = it.get_curr_ne();
      if (cmp(curr, item))
	return pair<bool, size_t>(false, i);

      item = curr;
    }
  return pair<bool, size_t>(true, 0);
}


    /** Ordena un arreglo por el método de selección.

	selection_sort(a,n) emplea el método de selección para ordenar
	el arreglo a de n elementos.

	El método de selección tiene un desempeño de \f$O(n^2)\f$. 
	Debido a su simplicidad de implantación, su coste constante
	es bajo, por lo que es	un buen método para arreglos de
	dimensión muy pequeña.

	El método emplea dos parámetros tipo:
	-# T: el tipo de elementos que tiene el arreglo.
	-# Compare: clase de comparación.

	@param[in,out] a el arreglo a ordenar.
	@param[in] n la dimensión del arreglo.
	@see insertion_sort() quicksort_rec() mergesort() heapsort()
	\ingroup Algos
     */
    template <typename T, class Compare  = Aleph::less<T>> inline
void selection_sort(T * a, const size_t n, Compare cmp = Compare()) noexcept
{
  for (size_t i = 0, min, j; i < n - 1; ++i)
    {
      for (min = i, j = i + 1; j < n; ++j)
	if (cmp(a[j], a[min]))
	  min = j;

      if (cmp(a[min], a[i]))
        std::swap(a[min], a[i]);
    }
}


    /** Búsqueda genérica de un elemento extremo en una lista
	de nodos Dlink.
		
	search_extreme(list) busca secuencialmente en la lista de nodos
	list el elemento extremo, mínimo o
	máximo según el criterio de comparación Compare.

	@param[in] list la lista sobre la cual realizar la búsqueda.
	@return puntero al nodo contentivo del elemento extremo.
	@note No se verifica si la lista está vacía.
	\ingroup Algos
     */
  template <class Link, class Compare> inline 
Link * search_extreme(const Link & list, Compare & cmp)
{
  typename Link::Iterator it(const_cast<Link&>(list));
  Link * extreme = it.get_curr();

  for (it.next(); it.has_curr(); it.next_ne())
    {
      Link * curr = it.get_curr_ne();
      if (cmp(curr, extreme))
	extreme = curr;
    } 

  return extreme;
}

    template <class Compare> inline 
Dlink * search_extreme(const Dlink & list, Compare cmp = Compare())
{
  return search_extreme<Dlink, Compare>(list, cmp);
}

    template <class Compare> inline 
Slinknc * search_extreme(const Slinknc & list, Compare cmp = Compare())
{
  return search_extreme<Slinknc, Compare>(list, cmp);
}

    /** Ordena por el método de selección una lista doblemente enlazada.

	selection_sort(list) sobre una lista derivada de Dlink ordena
	mediante el método de selección la lista list.

	La rutina requiere el parámetro tipo Compare que instrumenta la 
	comparación entre los miembros de la lista. Compare se invoca como 
	Compare()(l1,l2), donde l1 y l2 son dos enlaces de tipo Dlink. El
	usuario es responsable de instrumentar adecuadamente Compare()() 
	de modo que se accedan los campos de interés y se efectúe la 
	comparación.

	@param[in,out] list lista a ser ordenada.
	\ingroup Algos
     */
    template <class Compare> inline 
void selection_sort(Dlink & list, Compare cmp) noexcept
{
  Dlink aux;
  while (not list.is_empty())
    {
      Dlink * extreme = search_extreme <Dlink, Compare> (list, cmp); 
      extreme->del(); // saque extremo de list
      aux.append(extreme); // insértelo ordenado en aux;
    }

  list.swap(&aux);
}

template <typename Tlink, 
	  template <class> class Tnode, 
	  typename T, class Compare> 
class Compare_Tnode
{
  Compare cmp;

public:

  Compare_Tnode(Compare __cmp = Compare()) 
  noexcept(std::is_nothrow_move_assignable<Compare>::value)
    : cmp(__cmp) { /* empty */ }

  bool operator () (Tlink * l1, Tlink * l2) const noexcept
  {
    Tnode<T> * n1 = static_cast<Tnode<T>*>(l1);
    Tnode<T> * n2 = static_cast<Tnode<T>*>(l2);

    assert(n1 == l1 and n2 == l2);

    return cmp(n1->get_data(), n2->get_data());
  }

  // Para generalizar en el sequential_search
  bool operator () (Tlink * l, const T & x) const noexcept
  {
    Tnode<T> * n = static_cast<Tnode<T>*>(l);

    assert(n == l);

    return cmp(n->get_data(), x);
  }
};

    template <typename T, class Compare> 
struct Compare_Dnode : public Compare_Tnode<Dlink, Dnode, T, Compare>
{
  Compare_Dnode(Compare cmp = Compare()) 
  noexcept(std::is_nothrow_move_assignable<Compare>::value)
    : Compare_Tnode<Dlink, Dnode, T, Compare> (std::forward<Compare>(cmp)) 
  { /* empty */ }
};

template <typename T, class Compare>
using Compare_Snodenc = Compare_Tnode<Slinknc, Snodenc, T, Compare>;


    /** Ordena una lista enlazada de tipo Dnode<T> mediante 
	el método de selección.

	Ordena la lista list, con nodos de tipo Dnode<T> 
	según el método de selección. 
	
	El método de selección tiene un desempeño de \f$O(n^2)\f$. 
	Debido a su simplicidad de implantación su coste constante
	es bajo, por lo que es	un buen método para arreglos de
	dimensión muy pequeña.

	El método emplea dos parámetros tipo:
	-# T: el tipo de elementos que tiene el arreglo.
	-# Compare: clase de comparación.

	@param[in,out] list ka lista a ser ordenada.
	\ingroup Algos
     */
    template <typename T, class Compare = Aleph::less<T>> 
inline void selection_sort(Dnode<T> & list, Compare cmp) noexcept
{
  selection_sort<Compare_Dnode<T, Compare>>
    (list, Compare_Dnode<T, Compare>(cmp));
}

      /** Búsqueda secuencial sobre un arreglo.

	  sequential_search(a,x,l,r) busca secuencialmente, 
	  en el arreglo a, entre los índices l y r, respectivamente,
	  la primera ocurrencia de x.

	  La función maneja dos parámetros tipo:
	  -# T: el tipo de datos que alberga el arreglo.
	  -# Equal: clase comparación entre los elementos.

	  @param[in] a el arreglo sobre el cual se realiza la búsqueda.
	  @param[in] x el elemento a buscar.
	  @param[in] l índice izquierdo donde comienza la búsqueda.
	  @param[in] r índice derecho donde termina la búsqueda.
	  @return índice de la primera posición en arreglo que contiene
	  el elemento x; -1 de lo contrario.
	  \ingroup Algos
       */
      template <typename T, class Equal = Aleph::equal_to<T>> inline 
int sequential_search(T * a, T && x, const int l, const int r,
		      Equal eq = Equal()) noexcept
{
  for (int i = l; i <= r; i++)
    if (eq(a[i], x)) return i;

  return Not_Found;
}

     /** Búsqueda secuencial sobre un arreglo dinámico.

	  sequential_search(a,x,l,r) busca secuencialmente, 
	  en el arreglo a, entre los índices l y r, respectivamente,
	  la primera ocurrencia de x.

	  La función maneja dos parámetros tipo:
	  -# T: el tipo de datos que alberga el arreglo.
	  -# Equal: clase comparación entre los elementos.

	  Esta versión sobre arreglos dinámicos salta las posiciones 
	  que con certeza no han sido escritas. Sin embargo, tómese
	  en consideración que, según el tamaño del bloque, pueden 
	  haber entradas no escritas pero circunscritas en direcciones
	  de memoria válidas. En este sentido, es posible que dentro de
	  aquella clase de entradas se encuentre un elemento con valor x.

	  @param[in] a el arreglo dinámico sobre el cual se realiza 
	  la búsqueda.
	  @param[in] x el elemento a buscar.
	  @param[in] l índice izquierdo donde comienza la búsqueda.
	  @param[in] r índice derecho donde termina la búsqueda.
	  @return índice de la primera posición en arreglo que contiene
	  el elemento x; -1 de lo contrario.
	  @note La búsqueda no puede distinguir si una entrada válida ha
	  sido escrita o no. La programación de la clase Equal puede 
	  considerar un valor especial que represente que la entrada no
	  ha sido escrita. Se garantiza que entrada que no haya sido 
	  mapeada en un bloque no será accedida.
 	  @see DynArray
	\ingroup Algos
       */
    template <typename T, class Equal = Aleph::equal_to<T>> inline
int sequential_search(const DynArray<T> & a, const T & x, 
		      const int l, const int r, Equal eq = Equal()) noexcept
{
  for (int i = l; i <= r; i++)
    if (a.exist(i))
      if (eq(a(i), x))
	return i;

  return -1;
}

    template <class Link, typename T, class Equal>
Link * sequential_search(const Link & list, const T & x, Equal & eq) noexcept
{
  for (typename Link::Iterator it(const_cast<Link&>(list)); 
       it.has_curr(); it.next_ne())
    {
      Link * curr = it.get_curr_ne();
      if (eq(curr, x)) 
	return curr;
    }

  return nullptr;
}

    template <typename T, class Equal>
Dlink * sequential_search(const Dlink & list, const T & x,
			  Equal eq = Equal()) noexcept
{
  return sequential_search <Dlink, T, Equal>(list, x, eq);
}

    template <typename T, class Equal>
Slinknc * sequential_search(const Slinknc & list, const T & x,
			    Equal eq = Equal()) noexcept
{
  return sequential_search <Slinknc, T, Equal>(list, x, eq);
}

    /** Búsqueda secuencial sobre una lista de nodos.

	sequential_search(list,x) busca secuencialmente, 
	en la lista de nodos de tipo Dnode<T> list la primera
	ocurrencia de x.

	La función maneja dos parámetros tipo:
	-# T: el tipo de datos que alberga el arreglo.
	-# Equal: clase comparación entre los elementos.

	@param[in] list la lista sobre el cual se realiza 
	la búsqueda.
	@param[in] x el elemento a buscar.
	@return puntero al primer nodo que contiene
	el elemento x; nullptr de lo contrario.
	@see Dnode
	\ingroup Algos
    */
    template <typename T, class Equal = Aleph::equal_to<T>> inline
Dnode<T> * sequential_search(const Dnode<T> & list, const T & x, 
			     Equal & eq) noexcept
{
  Dlink * ret =
   sequential_search<T, Compare_Dnode <T, Equal>>
      ((Dlink &) list, x, Compare_Dnode<T, Equal>(eq));

  return ret == nullptr ? nullptr : static_cast <Dnode<T> *>(ret);
}

    template <typename T, class Equal = Aleph::equal_to<T>> inline
Dnode<T> * sequential_search(const Dnode<T> & list, const T & x, 
			     Equal && eq = Equal()) noexcept
{
  return sequential_search<T, Equal>(list, x, eq);
}

    /** Búsqueda secuencial sobre una lista dinámica DynDlist.

	sequential_search(list,x) busca secuencialmente, 
	en la lista dinámica list (DynDlist<T>) la primera
	ocurrencia de x.

	La función maneja dos parámetros tipo:
	-# T: el tipo de datos que alberga el arreglo.
	-# Equal: clase comparación entre los elementos.

	@param[in] list la lista dinámica sobre el cual se realiza 
	la búsqueda.
	@param[in] x el elemento a buscar.
	@return puntero al primer elemento con valor igual a x;
	nullptr de lo contrario.
	@see DynDlist
	\ingroup Algos
    */
    template <typename T, class Equal = Aleph::equal_to<T>> inline
T * sequential_search(const DynDlist<T> & list, const T & x,
		      Equal eq = Equal()) noexcept
{
  Dnode<T> * ret = sequential_search<T, Equal>((Dnode<T>&) list, x, eq);
  return ret != nullptr ? &ret->get_data() : nullptr;
}

    template <typename T, class Equal = Aleph::equal_to<T>> inline
T * sequential_search(const DynList<T> & list, const T & x,
		      Equal & eq = Equal())
      noexcept
{
  Slinknc * ret =
    sequential_search<T, Compare_Snodenc<T, Equal>>
     (*list.get_head(), x, Compare_Snodenc<T, Equal>(eq));

  return 
    ret != nullptr ? &((static_cast<Snodenc<T>*>(ret))->get_data()) : nullptr;
}


    /** Búsqueda genérica en un arreglo de un elemento extremo.
	
	search_extreme(a, l, r) busca secuencialmente en el arreglo
	a, entre los índices l y r, el elemento extremo, mínimo o
	máximo según el criterio de comparación Compare.

	@param[in] a el arreglo sobre el cual realizar la búsqueda.
	@param[in] l índice de comienzo de la búsqueda.
	@param[in] r índice de término de la búsqueda.
	@return índice contentivo del extremo.
	@note No se verifica si la secuencia está vacía.
	\ingroup Algos
     */
    template <typename T, class Compare = Aleph::less<T>> inline
int search_extreme(T * a, const int l, const int r, Compare cmp = Compare())
{
  T extreme_index = l;
  for (int i = l + 1; i <= r; i++)
    if (cmp(a[i], a[extreme_index])) // ¿se ve un nuevo menor?
      extreme_index = i; // sí

  return extreme_index;
}

 /** Retorna el mínimo elemento del arreglo a entre l y r.

     \ingroup Algos
  */
    template <typename T, class Compare = Aleph::less<T>> inline
int search_min(T * a, const int l, const int r, Compare cmp = Compare())
{
  return search_extreme<T, Compare> (a, l, r, cmp);
}

 /** Retorna el máximo elemento del arreglo a entre l y r.

     \ingroup Algos
  */
    template <typename T, class Compare = Aleph::greater<T>> inline
int search_max(T * a, const int l, const int r, Compare cmp = Compare())
{
  return search_extreme<T, Compare> (a, l, r, cmp);
}

    /** \overload search_extreme
	\ingroup Algos
    */
    template <typename T, class Compare = Aleph::less<T>> inline 
Dnode<T> * search_extreme(const Dnode<T> & list, Compare cmp = Compare())
{
  Compare_Dnode<T, Compare> cmp_dnode(cmp);
  Dlink * ret = 
    search_extreme<Dlink, Compare_Dnode<T, Compare>>((Dlink&) list, cmp_dnode);

  return static_cast <Dnode<T>*> (ret);
}

   /** Búsqueda genérica de un elemento extremo en una lista
	de nodos (Dlist<T>).
		
	search_extreme(list) busca secuencialmente en la lista de nodos
	list el elemento extremo, mínimo o
	máximo según el criterio de comparación Compare.

	@param[in] list la lista sobre la cual realizar la búsqueda.
	@return puntero al nodo contentivo del elemento extremo.
	@note No se verifica si la lista está vacía.
	\ingroup Algos
     */
    template <typename T, class Compare = Aleph::less<T>> inline
T * search_extreme(const DynDlist<T> & list, Compare cmp = Compare())
{
  Dnode<T> * ret = search_extreme <T, Compare> ((Dnode<T>&) list, cmp); 

  return ret != nullptr ? &(ret->get_data()) : nullptr;
}

    template <typename T, class Compare = Aleph::less<T>> inline 
T * search_extreme(const DynList<T> & list, Compare cmp = Compare())
{
  Slinknc * ret =
    search_extreme <Compare_Snodenc<T, Compare>>
      (*list.get_head(), Compare_Snodenc<T, Compare>(cmp)); 

  return ret != nullptr ? &(((Snodenc<T>*)ret)->get_data()) : nullptr;
}

    /** Retorna el mínimo elemento de la lista list.

	\ingroup Algos
    */
    template <typename T, class Compare = Aleph::less<T>> inline
T * search_min(const DynDlist<T> & list, Compare cmp = Compare())
{
  return search_extreme<T, Compare> (list, cmp);
}

    /** Retorna el máximo elemento de la lista list.

	\ingroup Algos
    */
    template <typename T, class Compare = Aleph::less<T>> inline
T * search_max(const DynDlist<T> & list, Compare cmp = Compare())
{
  return search_extreme<T, Compare> (list, cmp);
}

    /** Ordena un arreglo por el método de inserción.

	insertion_sort(a,l,r) emplea el método de selección para ordenar
	el arreglo a entre los índices l y r.

	El método de inserción tiene un desempeño de \f$O(n^2)\f$. 
	Es un método simple, por lo que consume poco tiempo constante.
	En promedio realiza la mitad de los intercambios que el método
	de selección. Su tiempo tiende a ser lineal si el arreglo está 
	semi-ordenado. Es un buen método para arreglos pequeños y para 
	particiones realizadas por métodos superiores pero con 
	mayores tiempos constantes.

	El método emplea dos parámetros tipo:
	-# T: el tipo de elementos que tiene el arreglo.
	-# Compare: clase de comparación.

	@param[in,out] a el arreglo a ordenar.
	@param[in] l índice izquierdo del arreglo.
	@param[in] r índice derecho del arreglo.
	@see selection_sort() quicksort_rec() mergesort() heapsort()
	\ingroup Algos
     */
    template <typename T, class Compare = Aleph::less<T>> inline
void insertion_sort(T * a, const size_t l, const size_t r,
		    Compare cmp = Compare()) noexcept
{
  for (size_t i = l, j; i <= r; ++i)
    {
      T tmp = a[i]; // memorice a[i], pues será sobre escrito
      
      for (j = i; j > l and cmp(tmp, a[j - 1]); --j)
	a[j] = a[j - 1]; // desplazar hacia la derecha

      a[j] = tmp; // inserte tmp en la brecha
    }
}

    /** Inserta un nodo ordenadamente en una lista doblemente enlazada.

	insert_sorted(list,p) inserta en una lista basada sobre Dlink el
	nodo apuntado por p.

	El criterio de orden está dado por la clase de comparación Compare.

	@param[in,out] list la lista a la cual se le insertará p.
	@param[in] p el nodo a ser insertado.
	@see Dlink 
	\ingroup Algos
     */
    template <class Compare> inline 
void insert_sorted(Dlink & list, Dlink * p, Compare & cmp) noexcept
{
  typename Dlink::Iterator it(list); 
  while (it.has_curr() and cmp(it.get_curr_ne(), p)) 
    it.next_ne();

  if (it.has_curr())
    it.get_curr_ne()->append(p); // insertar antes de current
  else
    list.append(p); 
}

template <class Compare> inline
void insert_sorted(HTList & list, Slinknc * p, Compare & cmp) noexcept
{
  Slinknc * first = list.get_first();
  if (cmp(p, first) or not cmp(first, p)) // p <= first?
    {
      list.insert(p);
      return;
    }

  Slinknc * last = list.get_last();
  if (cmp(last, p) or not cmp(p, last)) // p >= last?
    {
      list.append(p);
      return;
    }

  Slinknc * prev = list.get_first();
  typename HTList::Iterator it(list); 
  for (it.next(); it.has_curr(); it.next_ne())
    {
      Slinknc * curr = it.get_curr_ne();
      if (cmp(p, curr)) // p < curr
	{
	  prev->insert(p);
	  return;
	}
      prev = curr;
    }
  assert(false); // it should be unreachable
}

    /** Ordena según el método de inserción una lista de nodos simples

	insertion_sort(list) ordena según el método de inserción la
	lista enlazada list 

	El método de inserción tiene un desempeño de \f$O(n^2)\f$. 
	
	@param[in,out] list la lista a ordenar.
	\ingroup Algos
     */
template <class ListType, class Compare> inline 
void list_insertion_sort(ListType & list, Compare & cmp) noexcept
{
  if (list.is_empty())
    return;

  ListType aux; aux.append(list.remove_first());
  while (not list.is_empty())
    insert_sorted<Compare>(aux, list.remove_first(), cmp);

  list.swap(aux);
}

template <typename T, class Compare = Aleph::less<T>> inline 
void insertion_sort(DynList<T> & l, Compare cmp = Compare()) noexcept
{
  using Cmp = Compare_Snodenc<T, Compare>;
  Cmp c(cmp);  
  list_insertion_sort<HTList, Cmp>(l, c);
}

template <typename T, class Compare = Aleph::less<T>> inline 
DynList<T> insertion_sort(DynList<T> && l, Compare cmp = Compare()) noexcept
{
  using Cmp = Compare_Snodenc<T, Compare>;
  Cmp c(cmp);  
  list_insertion_sort<HTList, Cmp>(l, c);
  return move(l);
}

    /** Ordena la lista list por el método de inserción.

	Ordena la lista list basada en Dnode<T> por el método de 
	inserción según criterio de comparación Compare.

	El método de inserción tiene un desempeño de \f$O(n^2)\f$. 

	@param[in,out] list la lista a ordenar.
	@see Dnode selection_sort(Dlink * link)
     */
    template <typename T, class Compare = Aleph::less<T>> 
inline void insertion_sort(Dnode<T> & list, Compare cmp = Compare()) noexcept
{
  using Cmp = Compare_Dnode<T, Compare>;
  Cmp c(cmp);
  list_insertion_sort <Dlink, Cmp> (list, c);
}

/** Mezcla dos particiones ordenadas almacenadas en un arreglo.

    merge() asume como entrada dos particiones ordenadas entre los
    rangos [l..m] y [m+1..r] y las mezcla en un ordenamiento entre
    [l..r].

    @param[in] a el arreglo
    @param[in] l índice izquierdo del arreglo
    @param[in] m centro del arreglo, de la partición
    @param[in] r índice derecho del arreglo

    \ingroup Algos
*/
    template <typename T, class Compare = Aleph::less<T>> inline
void merge(T * a, const int l, const int m, const int r,
	   Compare cmp = Compare()) noexcept
{
  int i, j, k, s;
  s = r - l + 1;
  T b[s];
 
  for (i = l, k = 0; i <= m; ++i, ++k)
    b[k] = a[i];

  for (j = r; j > m; ++k, --j)
    b[k] = a[j];

  for (k = l, i = 0, j = s - 1; k <= r; ++k)
    if (cmp(b[i], b[j]))
      a[k] = b[i++];
    else
      a[k] = b[j--];
}

     /** Ordena un arreglo por el método de mezcla.

	 mergesort(a,l,r) ordena el arreglo a entre los índices l y r
	 según el método de mezcla según criterio de comparación Compare.

	 El método emplea dos parámetros tipo:
	-# T: el tipo de elementos que tiene el arreglo.
	-# Compare: clase de comparación.

	 Este método tiene un desempeño de \f$O(n \; \lg n)\f$, pero 
	 un consumo de espacio de \f$O(n)\f$.

	 @param[in,out] a el arreglo a ordenar.
	 @param[in] l índice inferior.
	 @param[in] r índice superior.

	 @see selection_sort() insertion_sort() quicksort_rec() heapsort()

	 \ingroup Algos
      */
    template <typename T, class Compare = Aleph::less<T>> inline
void mergesort(T * a, const int l, const int r, Compare cmp = Compare())
      noexcept
{
  if (l >= r) 
    return;

  const int m = (l + r)/2;

  mergesort<T, Compare>(a, l, m, cmp);
  mergesort<T, Compare>(a, m + 1, r, cmp);

  if (cmp(a[m], a[m + 1]))
    return;

  merge<T, Compare>(a, l, m, r, cmp);
}

    /** Mezcla dos listas ordenadas en una sola.
 
	merge_lists(l1,l2,result) toma dos listas ordenadas l1 y l2 
	y las fusiona en una sola lista ordenada result según 
	criterio de comparación Compare.

	No se verifica si las listas l1 y l2 están ordenadas. 
	Resultados serán incorrectos si no se cumple esta premisa.

	@param[in,out] l1 una lista ordenada a fusionar.
	@param[in,out] l2 una lista ordenada a fusionar.
	@param[out] result la lista donde se colocará el resultado.
	
	\ingroup Algos
     */
    template <typename Tlist, class Compare> inline
void merge_lists(Tlist & l1, Tlist & l2, Tlist & result,
		 Compare cmp = Compare()) noexcept
{
  assert(result.is_empty());

  while (not l1.is_empty() and not l2.is_empty())
    if (cmp(l1.get_first_ne(), l2.get_first_ne()))
      result.append(l1.remove_first_ne());
    else
      result.append(l2.remove_first_ne());

  if (l1.is_empty())
    result.concat_list(l2);
  else
    result.concat_list(l1);

  assert(l1.is_empty() and l2.is_empty());
}

    /** Mezcla dos listas ordenadas de tipo Dnode en una sola.
 
	merge_lists(l1,l2,result) toma dos listas ordenadas l1 y l2 
	y las fusiona en una sola lista ordenada result según 
	criterio de comparación Compare.

	No se verifica si las listas l1 y l2 están ordenadas. 
	Resultados serán incorrectos si no se cumple esta premisa.

	@param[in,out] l1 una lista ordenada a fusionar.
	@param[in,out] l2 una lista ordenada a fusionar.
	@param[out] result la lista donde se colocará el resultado.
	
	\ingroup Algos
     */
     template <typename T, class Compare = Aleph::less<T>> inline
void merge_lists(Dnode<T> & l1, Dnode<T> & l2, Dnode<T> & result,
		 Compare cmp = Compare()) noexcept
{
  merge_lists<Dnode<T>, Compare_Dnode<T, Compare>>(l1, l2, result, cmp);
}

     /** Ordena una lista según el método de mezcla (mergesort).

	 Ordena mediante el método de mezcla (mergesort) una lista
	 basada en Dlink, según criterio de comparación Compare.

	 Este método tiene un desempeño determinista de \f$O(n \; \lg n)\f$
	 y un consumo de espacio de \f$O(1)\f$. Es un muy buen método
	 para listas.

	 @param[in,out] list la lista a ser ordenada.

	 @see mergesort(Dnode<T> & list) 

	 \ingroup Algos
     */
    template <typename Tlist, class Compare> inline 
void mergesort(Tlist & list, Compare cmp = Compare()) noexcept
{
  if (list.is_unitarian_or_empty()) 
    return;

  Tlist l, r;
  list.split_list_ne(l, r);         // dividir en dos listas

  mergesort <Tlist, Compare> (l, cmp);
  mergesort <Tlist, Compare> (r, cmp);  
  
  merge_lists <Tlist, Compare> (l, r, list, cmp); // mezclarlas 
}

     /** Ordena una lista según el método de mezcla (mergesort)
	 combinado con el método de inserción.

	 Ordena mediante el método de mezcla (mergesort) una lista
	 dinámica (DynList o DynDlist) según criterio de comparación Compare.

	 Este método tiene un desempeño determinista de \f$O(n \; \lg n)\f$
	 y un consumo de espacio de \f$O(1)\f$. Es un muy buen método
	 para listas.

	 @param[in,out] list la lista a ser ordenada.

	 @see mergesort(Dnode<T> & list) 

	 \ingroup Algos
     */
template <template <typename> class Tlist, typename T,
	  class Compare = Aleph::less<T>> inline 
void mergeinsertsort(Tlist<T> & list, Compare cmp = Compare(), 
		     const size_t lsz = Aleph::Insertion_Threshold) noexcept
{
  if (lsz < Aleph::Insertion_Threshold)
    {
      insertion_sort<T, Compare>(list, cmp);
      return;
    }

  if (list.is_unitarian_or_empty()) 
    return;

  Tlist<T> l, r;
  list.split_list(l, r);         // dividir en dos listas

  mergeinsertsort <Tlist, T, Compare> (l, cmp);
  mergeinsertsort <Tlist, T, Compare> (r, cmp);  
  
  merge_lists <Tlist<T>, Compare> (l, r, list, cmp); // mezclarlas 
}

template <template <typename> class Tlist, typename T,
	  class Compare = Aleph::less<T>> inline 
void mergesort(Tlist<T> & list, Compare cmp = Compare()) noexcept
{
  if (list.is_unitarian_or_empty()) 
    return;

  Tlist<T> l, r;
  list.split_list(l, r);         // dividir en dos listas

  mergesort <Tlist, T, Compare> (l, cmp);
  mergesort <Tlist, T, Compare> (r, cmp);  
  
  merge_lists <Tlist<T>, Compare> (l, r, list, cmp); // mezclarlas 
}

     /** Ordena una lista según el método de mezcla (mergesort).

	 Ordena mediante el método de mezcla (mergesort) una lista
	 basada en Dnode<T>, según criterio de comparación Compare.

	 El método emplea dos parámetros tipo:
	-# T: el tipo de elementos que tiene el arreglo.
	-# Compare: clase de comparación.

	Este método tiene un desempeño determinista de \f$O(n \; \lg n)\f$
	y un consumo de espacio de \f$O(1)\f$. Es un muy buen método
	para listas.

	@param[in,out] list la lista a ser ordenada.

	@see mergesort(Dlink & list) quicksort(Dnode<T>)

	\ingroup Algos
     */
    template <typename T, class Compare = Aleph::less<T>> inline
  void mergesort(Dnode<T> & list, Compare cmp = Compare()) noexcept
{
  mergesort <Dnode<T>, Compare_Dnode<T, Compare>> (list, cmp);
}

    /** Ordena una lista dinámica según el método de mezcla (mergesort).

	Ordena mediante el método de mezcla (mergesort) una lista
	dinámica según criterio de comparación Compare.

	El método emplea dos parámetros tipo:
	-# T: el tipo de elementos que tiene el arreglo.
	-# Compare: clase de comparación.

	Este método tiene un desempeño determinista de \f$O(n \; \lg n)\f$
	y un consumo de espacio de \f$O(1)\f$. Es un muy buen método
	para listas.

	@param[in,out] list la lista a ser ordenada.

	@see quicksort(DynDlist<T> & list)

	\ingroup Algos
     */
    template <typename T, class Compare = Aleph::less<T>> inline
void mergesort(DynDlist<T> & list, Compare cmp = Compare()) noexcept
{
  mergesort <Dnode<T>, Compare_Dnode<T, Compare>> (list, cmp);
}

    template <typename T, class Compare = Aleph::less<T>> inline
      void mergesort(DynList<T> & list, Compare cmp = Compare()) noexcept
{
  mergesort<DynList<T>, Compare>(list, cmp);
}

    template <typename T, class Compare = Aleph::less<T>> inline
int select_pivot(T * a, const int l, const int r, Compare cmp = Compare())
      noexcept; 


    template <typename T, class Compare = Aleph::less<T>> inline
int partition(T * a, const int l, const int r, Compare cmp = Compare()) noexcept
{
  const int p = select_pivot <T, Compare> (a, l, r, cmp);
  std::swap(a[p], a[r]);

  T pivot = a[r]; // elemento pivot
  int i = l - 1,  // índice primer elemento a la izquierda > que pivot
      j = r;      // índice primer elemento a la derecha < que pivot
  while (true)
    {
          // avance mientras a[i] < a[pivot]
      while (cmp(a[++i], pivot)) { /* no hay cuerpo */ }

      while (cmp(pivot, a[--j])) // avance mientras a[pivot]< a[j] 
	if (j == l) // ¿se alcanzó el borde izquierdo?
	  break; // sí ==> hay que terminar la iteración

      if (i >= j) 
        break;

        // En este punto hay una inversión a[i] > a[pivot] > a[j] 
      std::swap(a[i], a[j]); // Eliminar la inversión
    }

  std::swap(a[i], a[r]);

  return i;
}

    /** Ordena un arreglo por el método quicksort.

	 quicksort_rec(a,l,r) ordena el arreglo a entre los índices l y r
	 según el método quicksort según criterio de comparación Compare.

	 El método emplea dos parámetros tipo:
	-# T: el tipo de elementos que tiene el arreglo.
	-# Compare: clase de comparación.

	Este método tiene un desempeño esperado de \f$O(n \; \lg n)\f$ 
	y es considerado el método de ordenamiento más veloz. 

	Esta versión de quicksort puede ocupar espacio \f$O(n)\f$.
	Utilice quicksort_rec_min() si se desea garantizar un consumo 
	máximo de espacio de \f$O(\lg n)\f$ en detrimento de un poco más
	de tiempo constante.

	El quicksort es un método probabilístico. En un muy mal caso 
	-muy mala suerte- puede degradarse a 
	\f$O(n^2)\f$. Para paliar, en la medida de la suerte, los malos
	casos, use quicksort_insertion() que ejecuta heurísticas para
	paliar los malos casos e invoca al método de inserción para
	particiones del arreglo pequeñas.

	@param[in,out] a el arreglo a ordenar.
	@param[in] l índice inferior.
	@param[in] r índice superior.

	@see selection_sort() insertion_sort() mergesort() heapsort()
	@see quicksort_rec_min() quicksort_insertion() quicksort()

	\ingroup Algos
      */
    template <typename T, class Compare = Aleph::less<T>> inline
void quicksort_rec(T * a, const int l, const int r, Compare cmp = Compare())
      noexcept
{
  if (l >= r) 
    return;

  const int pivot = partition <T, Compare> (a, l, r, cmp);

  quicksort_rec <T, Compare> (a, l, pivot - 1, cmp);
  quicksort_rec <T, Compare> (a, pivot + 1, r, cmp);
}

    template <typename T, class Compare = Aleph::less<T>> inline
void quicksort_no_tail(T * a, int l, int r, Compare cmp = Compare()) noexcept
{
 start:
  if (l >= r) 
    return;

  const int pivot = partition <T, Compare> (a, l, r, cmp);

  quicksort_no_tail <T, Compare> (a, l, pivot - 1, cmp);

  l = pivot + 1;
  goto start;
}

    /** Ordena un arreglo según el método quicksort con mínimo
	consumo de espacio.

	El quicksort puramente recursivo puede consumir pila proporcional
	al tamaño del arreglo a ordenar. Para evitar este problema, 
	en detrimento de un ligero coste de ejecución, quicksort_rec_min()
	siempre ordena de primero la partición más pequeña. Por los demás,
	las mismas consideraciones que para quicksort_rec() aplican.

	@param[in,out] a el arreglo a ordenar.
	@param[in] l índice inicial por donde se ordena.
	@param[in] r índice final por donde se ordena.
	@see quicksort_rec() quicksort() quicksort_insertion()
	\ingroup Algos
     */
    template <typename T, class Compare = Aleph::less<T>> inline
void quicksort_rec_min(T * a, const int l, const int r,
		       Compare cmp = Compare()) noexcept
{
  if (r <= l) 
    return;  

  const int pivot = partition<T, Compare>(a, l, r, cmp);
  if (pivot - l < r - pivot) // ¿cual es la partición más pequeña?
    {     // partición izquierda más pequeña
      quicksort_rec_min<T, Compare>(a, l, pivot - 1, cmp);
      quicksort_rec_min<T, Compare>(a, pivot + 1, r, cmp);
    }
  else
    {     // partición derecha más pequeña
      quicksort_rec_min<T, Compare>(a, pivot + 1, r, cmp);
      quicksort_rec_min<T, Compare>(a, l, pivot - 1, cmp);
    }
} 

    template <typename T, class Compare> inline
int select_pivot(T * a, const int l, const int r, Compare cmp)
      noexcept
{
  assert(l <= r);

  if (r - l <= 2) 
    return r;

  const int m = (r + l) / 2; // índice del centro
  const int p = cmp(a[l], a[m]) ? m : l; // p=max(a[l],a[m])

  return cmp (a[r], a[m]) ? r : p; // retornar min(a[r], a[m])
}

    /** Ordena un arreglo por el método quicksort sin recursión.

	 quicksort_rec(a,l,r) ordena el arreglo a entre los índices l y r
	 según el método quicksort según criterio de comparación Compare.

	 El método emplea dos parámetros tipo:
	-# T: el tipo de elementos que tiene el arreglo.
	-# Compare: clase de comparación.

	Este método tiene un desempeño esperado de \f$O(n \; \lg n)\f$ 
	y es considerado el método de ordenamiento más veloz. 

	Esta versión de quicksort puede ocupar espacio \f$O(n)\f$.
	Utilice quicksort_rec_min() si se desea garantizar un consumo 
	máximo de espacio de \f$O(\lg n)\f$ en detrimento de un poco más
	de tiempo constante.

	El quicksort es un método probabilístico. En un muy mal caso 
	-muy mala suerte- puede degradarse a 
	\f$O(n^2)\f$. Para paliar, en la medida de la suerte, los malos
	casos, use quicksort_insertion() que ejecuta heurísticas para
	paliar los malos casos e invoca al método de inserción para
	particiones del arreglo pequeñas.

	@param[in,out] a el arreglo a ordenar.
	@param[in] l índice inferior.
	@param[in] r índice superior.

	@see selection_sort() insertion_sort() mergesort() heapsort()
	@see quicksort_rec_min() quicksort_insertion() quicksort_rec()

	\ingroup Algos
      */
    template <typename T, class Compare = Aleph::less<T>> inline
void quicksort(T * a, const int l, const int r, Compare cmp = Compare())
      noexcept
{
  if (r - l < Quicksort_Threshold) 
    {
      insertion_sort(a, l, r, cmp);
      return;
    }

  typedef typename std::pair<int, int> Partition; 
  FixedStack<Partition> stack(64);
  stack.push(Partition(l, r)); // todo el arreglo como partición inicial

  while (stack.size() > 0)
    {
      const Partition p = stack.pop();
      const long diff = p.second - p.first;
      
      if (diff <= 1)
	continue;

      if (diff < Quicksort_Threshold)
	{
	  insertion_sort(a, p.first, p.second, cmp);
	  continue;
	}

      const int pivot = partition <T, Compare>(a, p.first, p.second, cmp);

      if (pivot - p.first < p.second - pivot) // ¿cuál más pequeña?
	{     // partición izquierda más pequeña
	  stack.push(Partition(pivot + 1, p.second));
	  stack.push(Partition(p.first, pivot - 1));
	}
      else
	{     // partición derecha más pequeña
	  stack.push(Partition(p.first, pivot - 1));
	  stack.push(Partition(pivot + 1, p.second));
	}
    }
}

    /** Ordena una lista enlazada por el método quicksort.

	quicksort(list) ordena una lista
	según el método quicksort según criterio de comparación Compare.

	Este método tiene un desempeño esperado de \f$O(n \; \lg n)\f$ 
	y es considerado el método de ordenamiento más veloz. 

	@param[in,out] list la lista a ordenar.

	\ingroup Algos
      */
    template <class Compare> 
void quicksort(Dlink & list, Compare cmp = Compare()) noexcept
{
  if (list.is_unitarian_or_empty()) 
    return;

  Dlink * pivot = list.remove_next();
  Dlink smaller, bigger; // listas de menores y mayores que pivot

  while (not list.is_empty()) 
    {
      Dlink * p = list.remove_next();
      if (cmp(p, pivot))
	smaller.append(p);
      else
	bigger.append(p);
    }

  quicksort <Compare> (bigger, cmp);  
  quicksort <Compare> (smaller, cmp);

  list.concat_list(&smaller); // restaurar listas ordenadas en list
  list.append(pivot);
  list.concat_list(&bigger);
} 

    /** Ordena una lista por el método quicksort.

	 quicksort(list) ordena  una lista mediante el método
	 quicksort según criterio de comparación Compare.

	 El método emplea dos parámetros tipo:
	-# T: el tipo de elementos que tiene el arreglo.
	-# Compare: clase de comparación.

	Este método tiene un desempeño esperado de \f$O(n \; \lg n)\f$ 
	y es considerado el método de ordenamiento más veloz. 

	Esta primitiva puede usarse para listas de tipo Dlist y DynDlist.

	@param[in,out] list la lista a ordenar.

	@see Dnode Dlist DynDlist

	\ingroup Algos
      */
    template <typename T, class Compare = Aleph::less<T>>
void quicksort(Dnode<T> & list, Compare cmp = Compare()) noexcept
{
  quicksort <Compare_Dnode<T, Compare>> (list, cmp);
}

template <typename T, class Compare> 
void quicksort(HTList & list, Compare cmp = Compare()) noexcept
{
  if (list.is_unitarian_or_empty()) 
    return;

  Snodenc<T> * pivot = (Snodenc<T> *) list.remove_first_ne();
  HTList smaller, bigger; // listas de menores y mayores que pivot

  while (not list.is_empty()) 
    {
      Snodenc<T> * p = (Snodenc<T> *) list.remove_first_ne();
      if (cmp(p->get_data(), pivot->get_data()))
	smaller.append(p);
      else
	bigger.append(p);
    }

  quicksort <T, Compare> (bigger, cmp);  
  quicksort <T, Compare> (smaller, cmp);

  list.concat_list(smaller); // restaurar listas ordenadas en list
  list.append(pivot);
  list.concat_list(bigger);
} 

    template <typename T, class Compare = Aleph::less<T>>
void quicksort(DynList<T> & list, Compare cmp = Compare()) noexcept
{
  quicksort<T, Compare>((HTList&) list, cmp);
}

    /** Ordena un arreglo por el método quicksort mejorado.

	quicksort_insertion(a,l,r) ordena el arreglo a entre los 
	índices l y r según el método quicksort según criterio de
	comparación Compare. 

	El procedimiento combina diversas técnicas para acelerar el
	ordenamiento y a la vez evitar degradación por malos casos. 

	Los malos casos son tratados mediante una selección del pivote 
	consistente de la mediana entre l, r y el centro de la partición.

	Para asegurar un consumo máximo de pila de \f$O(\lg n)\f$, 
	el método siempre procede recursivamente a ordenar la partición
	más pequeña.

	Cuando la partición alcanza un tamaño menor o igual a la
	constante global Aleph::Insertion_Threshold, entonces 
	la partición es ordenada por el método de inserción, el cual
	es más rápido que el quicksort para tamaños pequeños.

	 El método emplea dos parámetros tipo:
	-# T: el tipo de elementos que tiene el arreglo.
	-# Compare: clase de comparación.

	Este método tiene un desempeño esperado de \f$O(n \; \lg n)\f$ 
	y es considerado el método de ordenamiento más veloz. 

	@param[in,out] a el arreglo a ordenar.
	@param[in] l índice inferior.
	@param[in] r índice superior.

	@see insertion_sort() mergesort() heapsort()
	@see quicksort_rec_min() quicksort_insertion() quicksort()

	\ingroup Algos
      */
     template <typename T, class Compare = Aleph::less<T>> inline
void quicksort_insertion(T * a, const int l, const int r,
			 Compare cmp = Compare()) noexcept
{
  if (r <= l) 
    return;

  const int pivot = partition<T, Compare>(a, l, r, cmp);

  const int l_size = pivot - l; // tamaño partición izquierda
  const int r_size = r - pivot; // tamaño partición derecha
  bool left_done  = false; // true si partición izq está ordenada
  bool right_done = false; // true si partición der está ordenada

  if (l_size <= Aleph::Insertion_Threshold) 
    {     // ordene partición izq por inserción
      insertion_sort<T, Compare>(a, l, pivot - 1, cmp);  
      left_done = true;
    }

  if (r_size <= Aleph::Insertion_Threshold) 
    {     // ordene partición der  por inserción
      insertion_sort<T, Compare>(a, pivot + 1, r, cmp);
      right_done = true;
    }

  if (left_done and right_done) 
    return; // ambas particiones ordenadas por inserción

  if (left_done) // ¿partición izq ordenada por inserción?
    { // sí; sólo resta ordenar recursivamente partición der
      quicksort_insertion<T, Compare>(a, pivot + 1, r, cmp);
      return;
    }

  if (right_done) // ¿partición der ordenada por inserción?
    { // sí; sólo resta ordenar recursivamente partición izq
      quicksort_insertion<T, Compare>(a, l, pivot - 1, cmp);
      return;
    }

      // aquí, ambas particiones no fueron ordenadas por inserción
  if (l_size < r_size) // ordenar primero partición más pequeña 
    {     // partición izquierda más pequeña
      quicksort_insertion <T, Compare> (a, l, pivot - 1, cmp);
      quicksort_insertion <T, Compare> (a, pivot + 1, r, cmp);
    }
  else
    {     // partición derecha más pequeña
      quicksort_insertion <T, Compare> (a, pivot + 1, r, cmp);
      quicksort_insertion <T, Compare> (a, l, pivot - 1, cmp);
    }
}    

     /** Búsqueda aleatoria de un elemento en un arreglo.

	 random_search(a,x,l,r) se vale del algoritmo de partición del
	 quicksort para buscar el elemento x en el arreglo a comprendido 
	 entre los límites l y r.

	 El método emplea dos parámetros tipo:
	-# T: el tipo de elementos que tiene el arreglo.
	-# Compare: clase de comparación.

	 El procedimiento tiene una complejidad esperada de 
	 \f$O(n \; \lg n)\f$, que es mayor que la mera búsqueda 
	 secuencial, pero con el añadido de que luego de la
	 búsqueda el arreglo queda parcialmente ordenado y, puesto que
	 la selección del pivote es la mediana entre l, r y el centro 
	 del arreglo, la búsqueda tiende a ser lineal a medida que
	 se hacen más búsquedas aleatorias.

	 @param[in,out] a arreglo a buscar; es parcialmente modificado
	 luego de la búsqueda.
	 @param[in] x elemento a buscar.
	 @param[in] l índice inferior de comienzo de la búsqueda.
	 @param[in] r índice superior de término de la búsqueda.
	 @return índice de una entrada del arreglo contentiva del
	 valor x si ésta se encuentra dentro del arreglo; 
	 Not_Found (por general -1) de lo contrario.

	 \ingroup Algos
	 @see sequential_search() binary_search_rec()
      */
     template <typename T, class Compare = Aleph::less<T>> inline
int random_search(T * a, const T & x, const long l, const long r, 
		  Compare cmp = Compare()) noexcept
{
  if (l > r)
    return Not_Found;

  const long pivot = partition<T, Compare>(a, l, r, cmp);

  if (cmp(x, a[pivot]))
    return random_search<T, Compare>(a, x, l, pivot - 1, cmp);
  else if (cmp(a[pivot], x))
    return random_search<T, Compare>(a, x, pivot + 1, r, cmp);

  return pivot; // elemento encontrado en el índice x
}

    template <typename T, class Compare = Aleph::less<T>> inline
int random_search(DynArray<T> & a, const T & x, const long l, const long r,
		  Compare cmp = Compare()) noexcept
{
  if (l > r)
    return Not_Found;

  const long pivot = partition<T, Compare>(a, l, r, cmp);

  if (cmp(x, a(pivot)))
    return random_search<T, Compare>(a, x, l, pivot - 1, cmp);
  else if (cmp(a(pivot), x))
    return random_search<T, Compare>(a, x, pivot + 1, r, cmp);

  return pivot; // elemento encontrado en el índice x
}

    /** Búsqueda aleatoria de un elemento en una lista dlink.

	dlink_random_search(list,x) se vale del algoritmo 
	de partición del quicksort para buscar el elemento x 
	en la lista list.

	El método emplea dos parámetros tipo:
	-# T: el tipo de elementos que tiene el arreglo.
	-# Compare: clase de comparación.

	El procedimiento tiene una complejidad esperada de 
	\f$O(n \; \lg n)\f$, que es mayor que la mera búsqueda 
	secuencial, pero con el añadido de que luego de la
	búsqueda la lista queda parcialmente ordenada.

	@param[in,out] list lista en la cual se realiza la búsqueda; 
	es parcialmente modificada luego de la búsqueda.
	@param[in] x elemento a buscar.
	@return puntero al nodo contentivo del valor x si ésta
	se encuentra dentro en la lista; nullptr de lo contrario.

	@see random_search(Dlink&list,const T&x)

	\ingroup Algos
      */
    template <typename T, class Compare> inline
Dnode<T> * dlink_random_search(Dlink & list, const T & x,
			       Compare cmp = Compare()) noexcept
{
  if (list.is_empty()) 
    return nullptr;

  Dnode<T>   item(x);
  Dnode<T> * item_ptr = &item; // puntero a celda que contiene a x

  Dlink smaller; // lista de los menores que pivot
  Dlink bigger;  // lista de los mayores que pivot

  Dnode<T> * pivot = static_cast<Dnode<T>*>(list.remove_next());
  
  while (not list.is_empty()) 
    {
      Dlink * p = list.remove_next();
      if (cmp(p, pivot))
	smaller.append(p);
      else
	bigger.append(p);
    }

  Dnode<T> * ret_val = nullptr;
  if (cmp(item_ptr, pivot))
    ret_val = dlink_random_search <T, Compare> (smaller, x, cmp);
  else if (cmp(pivot, item_ptr))
    ret_val = dlink_random_search <T, Compare> (bigger, x, cmp);
  else
    ret_val = pivot;

  assert(list.is_empty());

  list.swap(&smaller); 
  list.append(pivot);  
  list.concat_list(&bigger);

  return ret_val;
}

    /** Búsqueda aleatoria de un elemento en una lista Dnode<T>.

	random_search(list,x) se vale del algoritmo de partición del
	quicksort para buscar el elemento x en la lista list.

	El método emplea dos parámetros tipo:
	-# T: el tipo de elementos que tiene el arreglo.
	-# Compare: clase de comparación.

	El procedimiento tiene una complejidad esperada de 
	\f$O(n \; \lg n)\f$, que es mayor que la mera búsqueda 
	secuencial, pero con el añadido de que luego de la
	búsqueda la lista queda parcialmente ordenada.

	Una especialización usa la relación "menor que" como 
	criterio de comparación y ahorra la escritura de esa
	clase.

	@param[in,out] list lista en la cual se realiza la búsqueda; 
	es parcialmente modificada luego de la búsqueda.
	@param[in] x elemento a buscar.
	@return puntero al nodo contentivo del valor x si ésta
	se encuentra dentro en la lista; nullptr de lo contrario.

	@see dlink_random_search

	\ingroup Algos
      */
    template <typename T, class Compare = Aleph::less<T>>
Dnode<T> * random_search(Dlink & list, const T & x, Compare cmp = Compare())
      noexcept
{
  return dlink_random_search <T, Compare_Dnode<T, Compare>> (list, x, cmp);
}

    /** Búsqueda aleatoria de un elemento en una lista dinámica.

	random_search(list,x) se vale del algoritmo de partición del
	quicksort para buscar el elemento x en la lista dinámica list.

	El método emplea dos parámetros tipo:
	-# T: el tipo de elementos que tiene el arreglo.
	-# Compare: clase de comparación.

	El procedimiento tiene una complejidad esperada de 
	\f$O(n \; \lg n)\f$, que es mayor que la mera búsqueda 
	secuencial, pero con el añadido de que luego de la
	búsqueda la lista queda parcialmente ordenada.

	Una especialización usa la relación "menor que" como 
	criterio de comparación y ahorra la escritura de esa
	clase.

	@param[in,out] list lista dinámica en la cual se realiza 
	la búsqueda; es parcialmente modificada luego de la búsqueda.
	@param[in] x elemento a buscar.
	@return puntero x dentro de la lista si ésta
	se encuentra dentro de ella; nullptr de lo contrario.

	@see DynDlist 

	\ingroup Algos
      */
    template <typename T, class Compare = Aleph::less<T>> inline
T * random_search(DynDlist<T> & list, const T & x, Compare cmp = Compare())
      noexcept
{
  Dnode<T> * p = 
    dlink_random_search <T, Compare_Dnode<T, Compare>> (list, x, cmp);

  return p == nullptr ? nullptr : &(p->get_data());
}

    template <typename T, class Compare> static inline
const T & __random_select(T * a, const long i, const long l, const long r,
			  Compare & cmp) noexcept
{
  const long pivot = partition<T, Compare>(a, l, r, cmp);
  if (i == pivot) 
    return a[i];

  if (i < pivot) // ¿está en partición izquierda?
    return __random_select<T, Compare>(a, i, l, pivot - 1, cmp);
  else
    return __random_select<T, Compare>(a, i, pivot + 1, r, cmp);
}

/** Selecciona un elemento pivote de un DynArray como la mediana entre
    los extremos y el centro.

    @ingrouop Algos
 */
    template <typename T, class Compare = Aleph::less<T>> inline
long select_pivot_op(const DynArray<T> & a, const long l, const long r,
		     Compare cmp = Compare()) noexcept
{
  assert(l <= r);

  if (r - l <= 5)
    return r;

  const long m = (l + r)/2; // elemento central

      // Accede las entradas del arreglo una sola vez
  T & la = a(l); 
  T & ra = a(r);
  T & ma = a(m);
  
  const T * med_ptr = Aleph::median<T, Compare>(la, ma, ra, cmp);
  
  if (med_ptr == &la)
    return l;

  if (med_ptr == &ma)
    return m;

  assert(med_ptr == &ra);

  return r;
}

/** Selecciona un elemento pivote de un DynArray como la mediana entre
    los extremos y el centro.

    @ingrouop Algos
 */
    template <typename T, class Compare = Aleph::less<T>> inline
long select_pivot_op(const Array<T> & a, const long l, const long r,
		     Compare cmp = Compare()) noexcept
{
  assert(l <= r);

  if (r - l <= 5)
    return r;

  const long m = (l + r)/2; // elemento central

      // Accede las entradas del arreglo una sola vez
  T & la = a(l); 
  T & ra = a(r);
  T & ma = a(m);
  
  const T * med_ptr = Aleph::median<T, Compare>(la, ma, ra, cmp);
  
  if (med_ptr == &la)
    return l;

  if (med_ptr == &ma)
    return m;

  assert(med_ptr == &ra);

  return r;
}

    template <typename T, class Compare = Aleph::less<T>> inline
int partition_op(const DynArray<T> & a, long l, long r,
		 Compare cmp = Compare()) noexcept
{
  if (l == r)
    return l;

  long i = l - 1, 
       j = r;

  T & pivot       = a(r);
  long pivot_idx  = select_pivot_op<T, Compare>(a, l, r, cmp);
  T & pivot_value = a(pivot_idx);
  std::swap(pivot, pivot_value);

  while (true)
    {
      while (cmp(a(++i), pivot)) ; 

      while (cmp(pivot, a(--j)))
	if (j == l)
	  break;

      if (i >= j)
	break;

      std::swap(a(i), a(j));
    }

  std::swap(a(i), a(r));

  return i;
}

    template <typename T, class Compare = Aleph::less<T>> inline
int partition_op(const Array<T> & a, long l, long r,
		 Compare cmp = Compare()) noexcept
{
  if (l == r)
    return l;

  long i = l - 1, 
       j = r;

  T & pivot       = a(r);
  long pivot_idx  = select_pivot_op<T, Compare>(a, l, r, cmp);
  T & pivot_value = a(pivot_idx);
  std::swap(pivot, pivot_value);

  while (true)
    {
      while (cmp(a(++i), pivot)) ; 

      while (cmp(pivot, a(--j)))
	if (j == l)
	  break;

      if (i >= j)
	break;

      std::swap(a(i), a(j));
    }

  std::swap(a(i), a(r));

  return i;
}

    template <typename T, class Compare> static inline
const T & __random_select(const DynArray<T> & a, const long i, 
			  const long l, const long r,
			  Compare cmp = Compare()) noexcept
{
  assert(i >= l and i <= r);

  const long pivot = partition_op<T, Compare>(a, l, r, cmp);
  if (i == pivot) 
    return a(i);

  if (i < pivot) // ¿está en partición izquierda?
    return __random_select<T, Compare>(a, i, l, pivot - 1, cmp);
  else
    return __random_select<T, Compare>(a, i, pivot + 1, r, cmp);
}

    template <typename T, class Compare> static inline
const T & __random_select(const Array<T> & a, const long i, 
			  const long l, const long r,
			  Compare cmp = Compare()) noexcept
{
  assert(i >= l and i <= r);

  const long pivot = partition_op<T, Compare>(a, l, r, cmp);
  if (i == pivot) 
    return a(i);

  if (i < pivot) // ¿está en partición izquierda?
    return __random_select<T, Compare>(a, i, l, pivot - 1, cmp);
  else
    return __random_select<T, Compare>(a, i, pivot + 1, r, cmp);
}

template <typename T, class Compare = Aleph::less<T>> 
const T & random_select(DynArray<T> & a, const long i, Compare cmp = Compare())
{
  const int n = a.size() - 1;
  if (i > n)
    throw std::out_of_range("index out of range");

  return __random_select<T, Compare>(a, i, 0, n, cmp);
}

template <typename T, class Compare = Aleph::less<T>> 
const T & random_select(Array<T> & a, const long i, Compare cmp = Compare())
{
  const int n = a.size() - 1;
  if (i > n)
    throw std::out_of_range("index out of range");

  return __random_select<T, Compare>(a, i, 0, n, cmp);
}

    /** Selección aleatoria del i-ésimo elemento de un arreglo.

	random_select(a,i,n) retorna el i-ésimo menor elemento 
	contenido en el arreglo a de dimensión n.

	La rutina se sirve de la partición del quicksort para
	buscar la posición i en tiempo \f$O(n \; \lg n)\f$, lo cual
	es un tiempo substancialmente mejor que el de la búsqueda
	secuencial (\f$O(n^2)\f$).

	El método emplea dos parámetros tipo:
	-# T: el tipo de elementos que tiene el arreglo.
	-# Compare: clase de comparación.

	@param[in,out] a arreglo donde se buscará el i-ésimo elemento. 
	El arreglo queda semi-ordenado a través de las sucesivas
	particiones que se hayan realizado.
	@param[in] i posición que se desea acceder.
	@param[in] n dimensión del arreglo
	@throw out_of_range si i es mayor o igual a n.

	\ingroup Algos
     */
  template <typename T, class Compare = Aleph::less<T>> 
const T & random_select(T * a, const long i, const long n,
			Compare cmp = Compare())
{
  if (i >= n)
    throw std::out_of_range("index out of range");

  return __random_select<T, Compare>(a, i, 0, n - 1, cmp);
}

  template <typename T, class Compare = Aleph::less<T>> 
const T & random_select(T * a, const long i, const long n, 
			Compare && cmp = Compare()) noexcept
{
  return random_select<T, Compare>(a, i, n, cmp);
}

    /** Selección aleatoria del i-ésimo elemento de una lista basada 
	sobre Dlink.

	random_select(list,i) retorna el i-ésimo menor elemento 
	contenido en la lista list según criterio de orden 
	determinado por la clase Compare.

	La rutina se sirve de la partición del quicksort para
	buscar la posición i en tiempo \f$O(n \; \lg n)\f$, lo cual
	es un tiempo substancialmente mejor que el de la búsqueda
	secuencial (\f$O(n^2)\f$).

	@param[in,out] list lista donde se buscará el i-ésimo elemento. 
	La lista queda semi-ordenado a través de las sucesivas
	particiones que se hayan realizado.
	@param[in] i posición que se desea acceder.
	@return puntero a Dlink correspondiente a la posición i dentro
	de la lista; 

	\ingroup Algos
     */
    template <class Compare>
Dlink * dlink_random_select(Dlink & list, const size_t i,
			    Compare cmp = Compare())
{
  if (list.is_empty()) 
    return nullptr;

  Dlink smaller; // lista de los menores que pivot
  Dlink bigger;  // lista de los mayores que pivot

  size_t smaller_count = 0, // cantidad de elementos de smaller
         bigger_count  = 0; // cantidad de elementos de bigger

  Dlink * pivot = list.remove_next();
  
  while (not list.is_empty()) 
    {
      Dlink * p = list.remove_next();
      if (cmp(p, pivot)) // ¿p < pivot?
	{  smaller.append(p); ++smaller_count;  }
      else
	{  bigger.append(p); ++bigger_count;  }
    }

  if (i >= smaller_count + bigger_count + 1) 
    throw std::out_of_range("index of selection greater than list's size");

  Dlink * ret_val = nullptr;
  if (i == smaller_count)
    ret_val = pivot;
  else if (i < smaller_count)
    ret_val = dlink_random_select<Compare>(smaller, i, cmp);
  else
    ret_val = dlink_random_select<Compare>(bigger, i - (smaller_count+1), cmp);

  list.concat_list(&smaller);
  list.append(pivot);
  list.concat_list(&bigger);

  return ret_val;
}

    /** Selección aleatoria del i-ésimo elemento de una lista basada 
	sobre Dlink.

	random_select(list,i) retorna el i-ésimo menor elemento 
	contenido en la lista list.

	La rutina se sirve de la partición del quicksort para
	buscar la posición i en tiempo \f$O(n \; \lg n)\f$, lo cual
	es un tiempo substancialmente mejor que el de la búsqueda
	secuencial (\f$O(n^2)\f$).

	El método emplea dos parámetros tipo:
	-# T: el tipo de elementos que tiene el arreglo.
	-# Compare: clase de comparación.

	@param[in,out] list lista donde se buscará el i-ésimo elemento. 
	La lista queda semi-ordenado a través de las sucesivas
	particiones que se hayan realizado.
	@param[in] i posición que se desea acceder.
	@return puntero a Dlink correspondiente a la posición i dentro
	de la lista; nullptr si i está fuera de rango.

	\ingroup Algos
     */
    template <typename T, class Compare = Aleph::less<T>>
Dnode<T> * random_select(Dlink & list, const size_t i, Compare cmp = Compare())
{
  return (Dnode<T>*) 
    dlink_random_select<Compare_Dnode<T, Compare>> (list, i, cmp);
}

    /** Selección aleatoria del i-ésimo elemento de una lista dinámica.

	random_select(list,i) retorna el i-ésimo menor elemento 
	contenido en la lista dinámica list.

	La rutina se sirve de la partición del quicksort para
	buscar la posición i en tiempo \f$O(n \; \lg n)\f$, lo cual
	es un tiempo substancialmente mejor que el de la búsqueda
	secuencial (\f$O(n^2)\f$).

	El método emplea dos parámetros tipo:
	-# T: el tipo de elementos que tiene el arreglo.
	-# Compare: clase de comparación.

	@param[in,out] list lista dinámica donde se buscará el 
	i-ésimo elemento. La lista queda semi-ordenado a través 
	de las sucesivas
	particiones que se hayan realizado.
	@param[in] i posición que se desea acceder.
	@return puntero al elemento de la lista dinámica correspondiente
	a la posición i dentro de la lista; nullptr si i está fuera de rango.

	\ingroup Algos
     */
    template <typename T, class Compare = Aleph::less<T>>
T * random_select(DynDlist<T> & list, const size_t i, Compare cmp = Compare())
{
  Dlink * link = dlink_random_select <Compare_Dnode<T, Compare>> (list, i, cmp);

  Dnode<T> * p = static_cast<Dnode<T>*>(link);

  return p != nullptr ? &(p->get_data()) : nullptr;
}
  
  /** Ordena un arreglo dinámico por el método de selección.

      selection_sort(a) emplea el método de selección para ordenar
      el arreglo dinámico de n elementos.

      El método de selección tiene un desempeño de \f$O(n^2)\f$. 
      Debido a su simplicidad de implantación, su coste constante
      es bajo, por lo que es	un buen método para arreglos de
      dimensión muy pequeña.

      El método emplea dos parámetros tipo:
      -# T: el tipo de elementos que tiene el arreglo.
      -# Compare: clase de comparación.

      @param[in,out] a el arreglo a ordenar.
      @see insertion_sort() quicksort_rec() mergesort() heapsort()
      \ingroup Algos
  */
    template <typename T, class Compare = Aleph::less<T>> inline
void selection_sort(DynArray<T> & a, Compare cmp = Compare()) noexcept
{
  const int n = a.size();

  for (int i = 0; i < n - 1; i++)
    {
      int min = i;

      for (int j = i + 1; j < n; j++)
	if (cmp(a(j), a(min)))
	  min = j;

      if (cmp(a(min), a(i)))
	std::swap(a(min), a(i));
    }
}

  /** Ordena un arreglo dinámico por el método de la burbuja.

      bubble_sort(a) emplea el método de la burbuja para ordenar
      el arreglo dinámico de n elementos.

      El método de la burbuja tiene un desempeño de
      \f$O(n^2)\f$. Posiblemente es el método más ineficiente de todos
      los existentes.

      El método emplea dos parámetros tipo:
      -# T: el tipo de elementos que tiene el arreglo.
      -# Compare: clase de comparación.

      @param[in,out] a el arreglo a ordenar.
      @see insertion_sort() quicksort_rec() mergesort() heapsort()
      \ingroup Algos
  */
    template <typename T, class Compare = Aleph::less<T>> inline
void bubble_sort(DynArray<T> & a, Compare cmp = Compare()) noexcept
{
  const int n = a.size();

  for (int i = 0; i < n - 1; i++)
    for (int j = n - 1; j > i; j--)
      if (cmp(a(j), a(j - 1)))
	std::swap(a(j - 1), a(j));
}

    /** Ordena un arreglo dinámico por el método de inserción.

	insertion_sort(a) emplea el método de selección para ordenar
	el arreglo dinámico a.

	El método de inserción tiene un desempeño de \f$O(n^2)\f$. 
	Es un método simple, por lo que consume poco tiempo constante.
	En promedio realiza la mitad de los intercambios que el método
	de selección. Su tiempo tiende a ser lineal si el arreglo está 
	semi-ordenado. Es un buen método para arreglos pequeños y para 
	particiones realizadas por métodos superiores pero con 
	mayores tiempos constantes.

	El método emplea dos parámetros tipo:
	-# T: el tipo de elementos que tiene el arreglo.
	-# Compare: clase de comparación.

	@param[in,out] a el arreglo a ordenar.
	@param[in] l índice izquierdo de la árte del arreglo que se
	desea ordenar 
	@param[in] r índice derecho de la parte del arreglo que se desea
	ordenar. 
	@see selection_sort() quicksort_rec() mergesort() heapsort()
	\ingroup Algos
     */
template <template <typename> class C, typename T, 
	  class Compare = Aleph::less<T>> inline
void insertion_sort(C<T> & a, long l, long r, Compare cmp = Compare()) noexcept
{
  for (long i = l + 1; i <= r; i++)
    {
      T tmp = a(i);
      long j = i;
      for (/* nothing */; j > 0 and cmp(tmp, a(j - 1)); --j)
	a(j) = a(j - 1);

      a(j) = tmp;
    }
}
  
    /**
	\overload insertion_sort()
	\ingroup Algos
     */
    template <template <typename> class C, typename T, 
	      class Compare = Aleph::less<T>> inline
void insertion_sort(C<T> & a, Compare cmp = Compare()) noexcept
{
  insertion_sort(a, 0, a.size() - 1, cmp);
}

    /** Ordena un arreglo dinámico por el método de shell.

	shellsort(a) emplea el método de selección para ordenar
	el arreglo dinámico a.

	El método shell tiende a  un desempeño de \f$O(n^2)\f$, pero en
	la práctica es considerablemente menor.

	El método emplea dos parámetros tipo:
	-# T: el tipo de elementos que tiene el arreglo.
	-# Compare: clase de comparación.

	@param[in,out] a el arreglo a ordenar.
	@see selection_sort() quicksort_rec() mergesort() heapsort()
	\ingroup Algos
     */
    template <typename T, class Compare = Aleph::less<T>> inline
void shellsort(DynArray<T> & a, Compare cmp = Compare()) noexcept
{
  const long n = a.size();
  int incs[16] = { 1391376, 463792, 198768, 86961, 33936, 13776, 
		   4592, 1968, 861, 336, 112, 48, 21, 7, 3, 1 }; 

  for (long k = 0; k < 16; k++) 
    { 
      const long h = incs[k]; 
      for (long i = h; i < n; i++) 
      { 
	T tmp = a(i); 
	long j = i;

	while (j >= h and cmp(tmp, a(j - h)))
	  {
	    a(j) = a(j - h);
	    j -= h;
	  } 

	a(j) = tmp;
      } 
    } 
}


  inline static long back_index(const long i) noexcept { return i - 1; }


    template <typename T, class Compare> inline
void sift_up(DynArray<T> & table, const size_t n, Compare & cmp) noexcept
{ 
  long p;
  for (long i = n; i > 1; i = p) 
    {
      p = i >> 1;     // c = i/2 

      if (cmp(table(back_index(p)), table(back_index(i))))
	return;

      std::swap(table(back_index(p)), table(back_index(i)));
    }
}

    template <typename T, class Compare> inline
void sift_down(DynArray<T> & table, const size_t n, Compare & cmp) noexcept
{
  long i = 1;

  while (true)
    {
      size_t c = i << 1; // c = 2*i

      if (c > n) 
	return;

      if (c + 1 <= n)
	if (cmp(table(back_index(c + 1)), table(back_index(c))))
	  c++;

      if (cmp(table(back_index(i)), table(back_index(c))))
	return; 

      std::swap(table(back_index(c)), table(back_index(i)));
      i = c;
    }
}

    template <typename T, class Compare>
class Negate_Compare
{
  Compare cmp;

public:

  Negate_Compare(Compare __cmp = Compare()) 
  noexcept(std::is_nothrow_copy_assignable<Compare>::value)
    : cmp(__cmp) { /* Empty */ }

  bool operator () (const T & e1, const T & e2) noexcept
  {
    return not cmp(e1, e2);
  }
};

    /** Ordena un arreglo dinámico por el método heapsort.

	heapsort(a) emplea el método de heapsort para ordenar
	el arreglo a de n elementos.

	El heapsort tiene un desempeño garantizado de \f$O(n \; \lg n)\f$
	y es estable.
	
	El método emplea dos parámetros tipo:
	-# T: el tipo de elementos que tiene el arreglo.
	-# Compare: clase de comparación.

	@param[in,out] a el arreglo a ordenar.
	@see insertion_sort() quicksort_rec() mergesort() 
	@see selection_sort() faster_heapsort()
	\ingroup Algos
     */
    template <typename T, class Compare = Aleph::less<T>> inline
void heapsort(DynArray<T> & a, Compare cmp = Compare()) noexcept
{
  const long n = a.size();

  long i;
  for (i = 2; i <= n; i++)
    sift_up<T, Negate_Compare<T, Compare>>(a, i, 
                                Negate_Compare<T, Compare>(cmp));

  for (i = n; i >= 2; i--)
    {
      std::swap(a(0), a(i - 1));

      sift_down<T, Negate_Compare<T, Compare>>(a, i - 1,
                                 Negate_Compare<T, Compare>(cmp));
    }
}

    template <template <typename> class C, typename T, 
	      class Compare = Aleph::less<T>> inline
long partition(C<T> & a, long l, long r, Compare cmp = Compare()) noexcept
{
  if (l == r)
    return l;

  long i = l - 1, 
       j = r;
  const T & pivot = a(r);

  while (true)
    {
      while (cmp(a(++i), pivot)) { /* Más nada */ }

      while (cmp(pivot, a(--j)))
	if (j == l)
	  break;

      if (i >= j)
	break;
      
      std::swap(a(i), a(j));
    }

    std::swap(a(i), a(r));

  return i;
}

    template <typename T, class Compare = Aleph::less<T>> inline 
void quicksort_rec(DynArray<T> & a, const long l, const long r,
		   Compare cmp = Compare()) noexcept
{
  if (r <= l) 
    return;

  long i = partition<T, Compare>(a, l, r, cmp);
  if (i - l < r - i)
    {
      quicksort_rec<T, Compare>(a, l, i - 1, cmp);
      quicksort_rec<T, Compare>(a, i + 1, r, cmp);
    }
  else
    {
      quicksort_rec<T, Compare>(a, i + 1, r, cmp);
      quicksort_rec<T, Compare>(a, l, i - 1, cmp);
    }
}

# define push2(stack, a, b)  stack.push(b); stack.push(a);

    /** Ordena un arreglo dinámico por el método quicksort sin recursión.

	 quicksort(a) ordena el arreglo dinámico según el método
	 quicksort según criterio de comparación Compare.

	 El método emplea dos parámetros tipo:
	-# T: el tipo de elementos que tiene el arreglo.
	-# Compare: clase de comparación.

	Este método tiene un desempeño esperado de \f$O(n \; \lg n)\f$ 
	y es considerado el método de ordenamiento más veloz. 

	Esta versión de quicksort ocupa espacio máximo de \f$O(\lg n)\f$.

	El quicksort es un método probabilístico. En un muy mal caso 
	-muy mala suerte- puede degradarse a 
	\f$O(n^2)\f$. Para paliar, en la medida de la suerte, los malos
	casos, use quicksort_insertion() que ejecuta heurísticas para
	paliar los malos casos e invoca al método de inserción para
	particiones del arreglo pequeñas.

	@param[in,out] a el arreglo a ordenar.

	@see selection_sort() insertion_sort() heapsort()
	@see quicksort_rec_min() quicksort_insertion() quicksort_rec()

	\ingroup Algos
      */
    template <typename T, class Compare = Aleph::less<T>> inline
void quicksort(DynArray<T> & a, Compare cmp = Compare()) noexcept
{
  long i, l = 0, r = a.size() -1;

  FixedStack<long> stack(40);

  push2(stack, l, r);

  while (not stack.is_empty())
    {
      l = stack.pop(); r = stack.pop();

      if (r <= l)
	continue;

      i = partition(a, l, r, cmp);

      if (i - l > r - i)
	{
	  push2(stack, l, i - 1); push2(stack, i + 1, r);
	}
      else 
	{
	  push2(stack, i + 1, r); push2(stack, l, i - 1); 
	} 
    }
}

    /** Búsqueda genérica en un arreglo dinámico de un elemento extremo.
	
	search_extreme(a, l, r) busca secuencialmente en el arreglo
	a, entre los índices l y r, el elemento extremo, mínimo o
	máximo según el criterio de comparación Compare.

	@param[in] a el arreglo sobre el cual realizar la búsqueda.
	@param[in] l índice de comienzo de la búsqueda.
	@param[in] r índice de término de la búsqueda.
	@return índice contentivo del extremo.
	@note No se verifica si la secuencia está vacía.
	\ingroup Algos
     */
    template <typename T, class Compare = Aleph::less<T>> inline
long search_extreme(const DynArray<T>& a, const long l, const long r, 
		    Compare cmp = Compare())
{
  long extreme_index = l;

  for (long i = l + 1; i <= r; i++)
    if (cmp(a(i), a(extreme_index)))
      extreme_index = i;

  return extreme_index;
}

  /** Retorna el máximo elemento del arreglo a entre l y r.

      \ingroup Algos
  */
    template <typename T, class Compare = Aleph::less<T>> inline
long search_max(const DynArray<T>& a, const long l, const long r,
		Compare cmp = Compare())
{
  return search_extreme<T, Compare>(a, l, r, cmp);
}  

    /** Búsqueda binaria sobre un arreglo dinámico ordenado.

	 binary_search<T,Compare>(a,x,l,r) realiza la búsqueda de x
	 en el arreglo a comprendida entre los límites inferior l y
	 superior r. 

	 La rutina es genérica y utiliza dos parámetros tipo:
	 -# El tipo de dato que alberga el arreglo.
	 -# La clase de comparación.

	 La rutina usa el algoritmo de la búsqueda binaria, lo que exige
	 que el arreglo esté ordenado. Esta condición no se verifica en
	 el algoritmo.

	 El método siempre retorna un entero comprendido entre
	 [l..r]. Si el elemento es encontrado, entonces el valor de retorno
	 es índice donde éste se encuentra; de lo contrario, se retorna
	 el índice donde se insertaría x para que el arreglo estuviese
	 ordenado.  

	 @warning La rutina no falla si el arreglo está vacío, pero
	 tenga cuidado con accederlo en esta situación

	 @param[in] a el arreglo dinámico sobre el cual realizar la búsqueda.
	 @param[in] x el elemento a buscar.
	 @param[in] l índice izquierdo de búsqueda.
	 @param[in] r índice derecho de búsqueda.
	 @return índice del elemento donde se encuentra x o el índice de
	 la posición donde se insertaría x.
	 @see sequential_search()
	 \ingroup Algos
      */
template <template <typename> class C, typename T, 
	  class Compare = Aleph::less<T>> inline
long binary_search(const C<T> & a, const T & x, long l, long r,
		   Compare cmp = Compare()) noexcept
{
  if (l > r) 
    return l;

  long m;
  while (l <= r)
    {
      m = (l + r)/2;
      if (cmp(x, a(m)))
	r = m -1;
      else if (cmp(a(m), x))
	l = m + 1;
      else
	return m; // key found
    }

  return m;
} 

    template <template <typename> class C, typename T, 
	      class Compare = Aleph::less<T>> inline
long binary_search(const C<T*> & a, const T & x, long l, long r,
		   Compare cmp = Compare()) noexcept
{
  if (l > r) 
    return l;

  long m;
  while (l <= r)
    {
      m = (l + r)/2;
      if (cmp(x, *a(m)))
	r = m - 1;
      else if (cmp(*a(m), x))
	l = m + 1;
      else
	return m; // clave encontrada
    }

  return m;
} 

    template <template <typename> class C, typename T, 
	      class Compare = Aleph::less<T>> inline
long binary_search(const C<T*> & a, const T & x, Compare && cmp = Compare()) 
      noexcept
{
  return binary_search(a, x, 0, a.size() - 1, cmp);
}

    /** Búsqueda binaria sobre un arreglo dinámico ordenado.

	 binary_search(a,x) realiza la búsqueda de x
	 en el arreglo a comprendida entre 0 y <code>size()</code>. 

	 La rutina usa el algoritmo de la búsqueda binaria, lo que exige
	 que el arreglo esté ordenado. Esta condición no se verifica en
	 el algoritmo.

	 El método siempre retorna un entero comprendido entre
	 [0..size()). Si el elemento es encontrado, entonces el valor de
	 retorno es el índice donde éste se encuentra; de lo contrario,
	 se retorna el índice donde se insertaría x para que el arreglo
	 estuviese ordenado.  

	 @warning La rutina no falla si el arreglo está vacío, pero
	 tenga cuidado con accederlo en esta situación

	 @param[in] a el arreglo dinámico sobre el cual realizar la búsqueda.
	 @param[in] x el elemento a buscar.
	 @return índice del elemento donde se encuentra x o el índice de
	 la posición donde se insertaría x.
	 @see sequential_search()
	 \ingroup Algos
      */
    template <template <typename> class C, typename T, 
	      class Compare = Aleph::less<T>> inline
long binary_search(const C<T> & a, const T & x, Compare cmp = Compare()) noexcept
{
  return binary_search(a, x, 0, a.size() - 1, cmp);
}

    template <template <typename> class C, typename T, 
	      class Compare = Aleph::less<T>> inline
DynList<size_t> binary_search_dup(const C<T> & a, const T & x,
				  Compare cmp = Compare()) noexcept
{
  DynList<size_t> ret;
  long idx = binary_search(a, x, 0, a.size() - 1, cmp);
  if (idx < 0)
    return ret;

  if (not are_equals(a(idx), x, cmp))
    return ret;;

  ret.append(idx);
  for (long i = idx - 1; i >= 0; --i)
    {
      if (not are_equals(a(i), x, cmp))
	break;
      ret.insert(i);
    }

  for (long i = idx + 1, n = a.size(); i < n; ++i)
    {
      if (not are_equals(a(i), x, cmp))
	break;
      ret.append(i);
    }

  return ret;
}

    template <template <typename> class C, typename T, 
	      class Compare = Aleph::less<T>> inline
T * bsearch(const C<T> & a, const T & x, Compare cmp = Compare()) noexcept
{
  long i = binary_search(a, x, cmp);
  if (i < 0)
    return nullptr;
  T * ptr = &a(i);
  return are_equals(*ptr, x, cmp) ? ptr : nullptr;
}

    template <template <typename> class C, typename T, 
	      class Compare = Aleph::less<T>> inline
T * bsearch(const C<T*> & a, const T & x, Compare cmp = Compare()) noexcept
{
  long i = binary_search(a, x, cmp);
  if (i < 0)
    return nullptr;
  T * ptr = a(i);
  return are_equals(*ptr, x, cmp) ? ptr : nullptr;
}

template <template <typename> class C, typename T,
	  class Compare = Aleph::less<T>> inline
DynList<T*> bsearch_dup(const C<T> & a, const T & x, Compare cmp = Compare())
  noexcept
{
  DynList<T*> ret;
  long idx = binary_search(a, x, cmp);
  if (idx < 0)
    return ret;

  T * found_ptr = const_cast<T*>(&a(idx));
  if (not are_equals(*found_ptr, x, cmp))
    return ret;;

  for (long i = idx - 1; i >= 0; --i)
    {
      T * ptr = const_cast<T*>(&a(i));
      if (not are_equals(*ptr, x, cmp))
	break;
      ret.insert(ptr);
    }

  ret.append(found_ptr);

  for (long i = idx + 1, n = a.size(); i < n; ++i)
    {
      T * ptr = const_cast<T*>(&a(i));
      if (not are_equals(*ptr, x, cmp))
	break;
      ret.append(ptr);
    }

  return ret;
}

template <template <typename> class C, typename T,
	  class Compare = Aleph::less<T>> inline
DynList<T*> bsearch_dup(const C<T*> & a, const T & x, Compare cmp = Compare())
  noexcept
{
  DynList<T*> ret;
  long idx = binary_search(a, x, cmp);
  if (idx < 0)
    return ret;

  T * found_ptr = a(idx);
  if (not are_equals(*found_ptr, x, cmp))
    return ret;;

  for (long i = idx - 1; i >= 0; --i)
    {
      T * ptr = a(i);
      if (not are_equals(*ptr, x, cmp))
	break;
      ret.insert(ptr);
    }

  ret.append(found_ptr);

  for (long i = idx + 1, n = a.size(); i < n; ++i)
    {
      T * ptr = a(i);
      if (not are_equals(*ptr, x, cmp))
	break;
      ret.append(ptr);
    }

  return ret;
}

    template <template <typename> class C, typename T, 
	      class Compare = Aleph::less<T>> inline
long binindex(const C<T> & a, const T & x, Compare cmp = Compare()) noexcept
{
  return binary_search(a, x, cmp);
}

    template <template <typename> class C, typename T,
	      class Compare = Aleph::less<T>> inline
DynList<long> binindex_dup(const C<T> & a, const T & x, Compare cmp = Compare())
  noexcept
{
  DynList<long> ret;
  long idx = binary_search(a, x, cmp);
  if (idx < 0)
    return ret;

  T * ptr = const_cast<T*>(&a(idx));
  if (not are_equals(*ptr, x, cmp))
    return ret;;

  long mid = idx;

  for (long i = idx - 1; i >= 0; --i)
    {
      ptr = const_cast<T*>(&a(i));
      if (not are_equals(*ptr, x, cmp))
	break;
      ret.insert(i);
    }

  ret.append(mid);

  for (long i = idx + 1, n = a.size(); i < n; ++i)
    {
      ptr = const_cast<T*>(&a(i));
      if (not are_equals(*ptr, x, cmp))
	break;
      ret.append(i);
    }

  return ret;
}

template <template <typename> class C, typename T,
	  class Compare = Aleph::less<T>> inline
DynList<long> binindex_dup(const C<T*> & a, const T & x, Compare cmp = Compare())
  noexcept
{
  DynList<long> ret;
  long idx = binary_search(a, x, cmp);
  if (idx < 0)
    return ret;

  T * ptr = a(idx);
  if (not are_equals(*ptr, x, cmp))
    return ret;;

  for (long i = idx - 1; i >= 0; --i)
    {
      ptr = a(i);
      if (not are_equals(*ptr, x, cmp))
	break;
      ret.insert(i);
    }

  ret.append(ptr);

  for (long i = idx + 1, n = a.size(); i < n; ++i)
    {
      T * ptr = a(i);
      if (not are_equals(*ptr, x, cmp))
	break;
      ret.append(i);
    }

  return ret;
}

    template <template <typename> class C, typename T, 
	      class Compare = Aleph::less<T>> inline
DynArray<size_t> build_index(const C<T> & a, Compare cmp = Compare())
{
  const size_t & n = a.size();
  DynArray<size_t> ret; ret.reserve(a.size());
  for (size_t i = 0; i < n; ++i)
    ret(i) = i;
  
  quicksort_op(ret,
	       [&a, &cmp] (size_t i, size_t j) { return cmp(a(i), a(j)); });

  return ret;
}

    template <template <typename> class C, typename T, 
	      class Compare = Aleph::less<T>> inline
DynArray<T*> build_index_ptr(const C<T> & a, Compare cmp = Compare())
{
  const size_t & n = a.size();
  DynArray<T*> ret; ret.reserve(a.size());
  for (size_t i = 0; i < n; ++i)
    ret(i) = &a(i);
  
  quicksort_op(ret, [&cmp] (const T * ptr1, const T * ptr2) 
	       { return cmp(*ptr1, *ptr2); });

  return ret;
}

template <template <typename> class C, typename T, 
	  class Compare = Aleph::less<T>> inline
void quicksort_op(C<T> & a, Compare cmp = Compare(),
		  size_t threshold = Quicksort_Threshold) noexcept
{
  size_t i, l = 0, r = a.size() -1;

  FixedStack<int> stack(40);

  push2(stack, l, r);

  while (not stack.is_empty())
    {
      l = stack.pop(); r = stack.pop();

      size_t partition_size = r - l + 1;
      if (partition_size <= 1)  
	continue;

      if (partition_size <= threshold)
	{
	  insertion_sort(a, l, r, cmp);
	  continue;
	}

      i = partition_op<T, Compare>(a, l, r, cmp);

      if (i - l > r - i)
	{
	  push2(stack, l, i - 1); push2(stack, i + 1, r);
	}
      else 
	{
	  push2(stack, i + 1, r); push2(stack, l, i - 1); 
	} 
    }
}

# undef push2

      /** Búsqueda binaria recursiva sobre un arreglo ordenado.

	 binary_search_rec<T,Compare>(a,x,l,r) realiza la búsqueda de x
	 en el arreglo a comprendida entre los límites inferior l y
	 superior r. 

	 La rutina es genérica y utiliza dos parámetros tipo:
	 -# El tipo de dato que alberga el arreglo.
	 -# La clase de comparación.

	 La rutina usa el algoritmo de la búsqueda binaria, lo que exige
	 que el arreglo esté ordenado. Esta condición no se verifica en
	 el algoritmo.

	 El método siempre retorna un entero comprendido entre
	 [l..r]. Si el elemento es encontrado, entonces el valor de retorno
	 es índice donde éste se encuentra; de lo contrario, se retorna
	 el índice donde se insertaría x para que el arreglo estuviese
	 ordenado.  

	 @param[in] a el arreglo sobre el cual realizar la búsqueda.
	 @param[in] x el elemento a buscar.
	 @param[in] l índice izquierdo de búsqueda.
	 @param[in] r índice derecho de búsqueda.
	 @return índice del elemento donde se encuentra x o el índice de
	 la posición donde se insertaría x.
	 @see sequential_search()
	 \ingroup Algos
      */
    template <typename T, class Compare = Aleph::less<T>> inline 
int binary_search_rec(T * a, const T & x, const long l, const long r,
		      Compare cmp = Compare()) noexcept
{
  const long m = (l + r) / 2;
  if (l > r) 
    return m;

  if (cmp(x, a[m]))
    return binary_search_rec<T, Compare>(a, l, m - 1, cmp);
  else if (cmp(a[m], x))
    return binary_search_rec<T, Compare>(a, m + 1, r, cmp);

  return m; // encontrado
}

    /** Búsqueda binaria iterativa sobre un arreglo ordenado.

	 binary_search<T,Compare>(a,x,l,r) realiza la búsqueda de x
	 en el arreglo a comprendida entre los límites inferior l y
	 superior r. 

	 La rutina es genérica y utiliza dos parámetros tipo:
	 -# El tipo de dato que alberga el arreglo.
	 -# La clase de comparación.

	 La rutina usa el algoritmo de la búsqueda binaria, lo que exige
	 que el arreglo esté ordenado. Esta condición no se verifica en
	 el algoritmo.

	 El método siempre retorna un entero comprendido entre
	 [l..r]. Si el elemento es encontrado, entonces el valor de retorno
	 es índice donde éste se encuentra; de lo contrario, se retorna
	 el índice donde se insertaría x para que el arreglo estuviese
	 ordenado.  

	 @param[in] a el arreglo sobre el cual realizar la búsqueda.
	 @param[in] x el elemento a buscar.
	 @param[in] l índice izquierdo de búsqueda.
	 @param[in] r índice derecho de búsqueda.
	 @return índice del elemento donde se encuentra x o el índice de
	 la posición donde se insertaría x.
	 @see sequential_search()
	 \ingroup Algos
      */
    template <typename T, class Compare = Aleph::less<T>> inline 
long binary_search(T * a, const T & x, long l, long r,
		   Compare cmp = Compare()) noexcept
{
  long m;
  while (l <= r)
    {
      m = (l + r) / 2;
      if (cmp(x, a[m]))
	r = m - 1;
      else if (cmp(a[m], x))
	l = m + 1;
      else 
	break;
    }
  return m; 
}

}

# endif // TPL_SORT_UTILS_H
