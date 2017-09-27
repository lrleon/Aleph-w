# ifndef TPL_DYNARRAYHEAP_H
# define TPL_DYNARRAYHEAP_H

# include <tpl_dynArray.H>

namespace Aleph {


    template <typename T, class Compare> inline
void sift_up(DynArray<T> & a, size_t l, size_t r, Compare & cmp) noexcept
{
  for (size_t p, i = r; i > l; i = p) 
    {
      p = u_index(i);
      T & ap = a.access(p);
      T & ai = a.access(i);
      if (cmp (ap, ai)) // ¿cumple propiedad orden?
        return; // si, todo el arreglo es un heap

      std::swap(ap, ai); // intercambie y restaure nivel p
    }
}

    template <typename T, class Compare> inline
void sift_down(DynArray<T> & a, size_t l, size_t r, Compare & cmp) noexcept
{
  size_t i = l, c;
  while (true)
    {
      c = l_index(i); // índice del hijo izquierdo (c = i/2)
      if (c > r) // ¿hay hijo izquierdo?
        return; // no ==> termine

      T * ac = & a.access(c);
      if (c + 1 <= r) // ¿hay hijo derecho?
	{
	  T * ac1 =  & a.access(c + 1);
	  if (cmp(*ac1, *ac)) // sí ==> escoja menor
	    {
	      c++;
	      ac = ac1;
	    }
	}

      T & ai = a.access(i);
      if (cmp(ai, *ac)) // ¿cumple propiedad orden?
        return;  // sí ==> termine

      std::swap(*ac, ai); 
      i = c;
    }
}


     /** Heap o cola de prioridad implementada con arreglos.

         ArrayHeap define un heap instrumentado mediante un arreglo 
         estático cuya dimensión es especificada en tiempo de 
         construcción.

         @param T el tipo de elementos que tiene el heap.
         @param Compare el criterio de comparación entre los 
         elementos el cual determina el tipo de orden y prioridad.

         @see BinHeap DynBinHeap
         @ingroup Arboles
      */
    template <typename T, class Compare = Aleph::less<T>> 
class DynArrayHeap :
    public LocateFunctions<DynArrayHeap<T, Compare>, T>,
    public FunctionalMethods<DynArrayHeap<T, Compare>, T>,
    public GenericKeys<DynArrayHeap<T, Compare>, T>,
    public EqualToMethod<DynArrayHeap<T, Compare>>,
    public StlAlephIterator<DynArrayHeap<T, Compare>>
{
  DynArray<T> array;
  size_t      num_items = 0;

  Compare cmp;

  static size_t r_index(const size_t & i) noexcept
  {
    return (i << 1) + 1; // multiplica i por 2 y suma 1
  }

public:

  using Item_Type = T;

      /// Constructor con dimensión por omisión.
  DynArrayHeap(Compare __cmp = Compare()) : num_items(0), cmp(__cmp)
  { 
    // empty
  }

  Special_Ctors(DynArrayHeap, T);

  Args_Ctor(DynArrayHeap, T);

      /// Retorna el menor elemento del heap.
  T & top() throw(std::exception, std::underflow_error)
  {
    if (num_items == 0)
      throw std::underflow_error("Heap is empty");

    return array.access(1);
  }

      /** Inserta un elemento en el heap.

          insert(key) inserta en el heap la clave una copia de la 
          clave key.

          @param[in] key clave a insertar.
          @return una referencia modificable al elemento insertado.
          @throw overflow_error si el arreglo interno está lleno.
       */
  T & insert(const T & key) throw(std::exception, std::overflow_error)
  {
    array.touch(++num_items) = key;  // colocar nuevo elemento
    sift_up(array, 1, num_items, cmp); // restaurar propiedad orden
    return array.access(num_items);
  }

  T & insert(T && key) throw(std::exception, std::overflow_error)
  {
    array.touch(++num_items) = move(key);  // colocar nuevo elemento
    sift_up(array, 1, num_items, cmp); // restaurar propiedad orden
    return array.access(num_items);
  }

  void reserve(size_t n) 
  {
    if (num_items > n)
      throw out_of_range("DynArrayHeap: n is greater than current heap size");
    array.reserve(n); 
  }

  T & insert_direct(const T & key) noexcept
  {
    array(++num_items) = key;  // colocar nuevo elemento
    sift_up(array, 1, num_items, cmp); // restaurar propiedad orden
    return array.access(num_items);
  }

  T & insert_direct(T && key) noexcept
  {
    array(++num_items) = move(key);  // colocar nuevo elemento
    sift_up(array, 1, num_items, cmp); // restaurar propiedad orden
    return array.access(num_items);
  }

  T & put(const T & key) { return insert(key); }

  T & put(T && key) { return insert(std::forward<T>(key)); }

  T & append(const T & key) { return insert(key); }

  T & append(T && key) { return insert(std::forward<T>(key)); }

      /** Elimina el menor elemento del heap y retorna una copia del valor
          eliminado.

          getMin() extrae del heap el elemento con menor valor según sea 
          el criterio de comparación especificado.

          @throw underflow_error si el heap está vacío.
          @return una copia del valor eliminado.
       */
  T getMin() throw(std::exception, std::underflow_error)
  {
    if (num_items == 0)
      throw std::underflow_error("Heap is empty");

    T & a1 = array(1);
    T ret_val = move(a1);
    a1 = move(array(num_items--)); 
    sift_down(array, 1, num_items, cmp); // propiedad orden

    array.cut(num_items + 1);

    return ret_val;
  }

  /// \overload getMin() 
  T get() throw(std::exception, std::underflow_error)
  {
    return getMin();
  }

  /// \overload getMin() para usar si se invierte el orden de prioridad
  T getMax() throw(std::exception, std::underflow_error)
  {
    return getMin();
  }

  /// Retorna la cantidad de elementos
  const size_t & size() const noexcept { return num_items; }
  
  /// Retorna true si el heap está vacío
  bool is_empty() const noexcept { return num_items == 0; }

  struct Iterator : DynArray<T>::Iterator
  {
    using Base = typename DynArray<T>::Iterator;

    Iterator(const DynArrayHeap & h) noexcept : Base(h.array) 
    {
      if (h.num_items != 0)
	this->next();
    }

    Iterator() noexcept { /* empty */ }

    bool has_curr() const noexcept
    {
      return this->curr_idx != 0 and this->curr_idx != this->array_ptr->size();
    }

    long get_pos() const noexcept { return this->Base::get_pos() - 1; }
  };

  template <class Operation>
  bool traverse(Operation & operation) noexcept(noexcept(operation))
  {
    for (Iterator it(*this); it.has_curr(); it.next())
      if (not operation(it.get_curr()))
	return false;
    return true;
  }

  template <class Operation>
  bool traverse(Operation & operation) const noexcept(noexcept(operation))
  {
    return const_cast<DynArrayHeap&>(*this).traverse<Operation>(operation);
  }

  template <class Operation>
  bool traverse(Operation && operation = Operation()) const
    noexcept(noexcept(operation))
  {

    return traverse<Operation>(operation);
  }

  template <class Operation>
  bool traverse(Operation && operation = Operation()) 
    noexcept(noexcept(operation))
  {
    return traverse<Operation>(operation);
  }
};



} // end namespace Aleph

# endif // TPL_DYNARRAYHEAP_H
