
# ifndef TPL_ARRAYHEAP_H
# define TPL_ARRAYHEAP_H

# include <ahFunction.H>
# include <ahUtils.H>
# include <ahDefs.H>
# include <ahAssert.H>
# include <array_it.H>
# include <htlist.H>
# include <tpl_dynDlist.H>
# include <ah-args-ctor.H>
# include <ahDry.H>
# include <ah-dry.H>

using namespace Aleph;

namespace Aleph {


    template <typename T, class Compare> inline
T & sift_up(T * ptr, const size_t l, const size_t r, Compare & cmp)
{
  size_t i = r;
  for (size_t p; i > l; i = p) 
    {
      p = u_index(i); // índice del padre (c = i/2)
      if (cmp(ptr[p], ptr[i])) // ¿cumple propiedad orden?
        return ptr[i]; // si, todo el arreglo es un heap

      std::swap(ptr[p], ptr[i]); // intercambie y restaure nivel p
    }

  return ptr[i];
}

    template <typename T, class Compare> inline
void sift_down(T * ptr, const size_t l, const size_t r, Compare & cmp)
{
  size_t i = l, c;
  while (true)
    {
      c = l_index(i); // índice del hijo izquierdo (c = i/2)
      if (c > r) // ¿hay hijo izquierdo?
        return; // no ==> termine

      if (c + 1 <= r) // ¿hay hijo derecho?
        if (cmp (ptr[c + 1], ptr[c])) // sí ==> escoja menor
          c++;

      if (cmp (ptr[i], ptr[c])) // ¿cumple propiedad orden?
        return;  // sí ==> termine

      std::swap(ptr[c], ptr[i]); 
      i = c;
    }
}

    template <typename T, class Compare> inline
void sift_down_up(T * ptr, const size_t l, const size_t i, const size_t r,
		  Compare & cmp)
{
  sift_down <T, Compare> (ptr, i, r, cmp);
  sift_up <T, Compare> (ptr, l, i, cmp);
}

    /** Ordena un arreglo por el método heapsort.

        heapsort(array,n) emplea el método de heapsort para ordenar
        el arreglo a de n elementos.

        El heapsort tiene un desempeño garantizado de \f$O(n \; \lg n)\f$
        y es estable.
        
        El método emplea dos parámetros tipo:
        -# T: el tipo de elementos que tiene el arreglo.
        -# Compare: clase de comparación.

        Una versión sobrecargada heapsort() especializa la 
        comparación al operador "menor que" sobre el tipo T.

        @param[in,out] array el arreglo a ordenar.
        @param[in] n la dimensión del arreglo.
        @see insertion_sort() quicksort_rec() mergesort() 
        @see selection_sort() faster_heapsort()
        @ingroup Algos
     */
    template <typename T, class Compare = Aleph::less<T>> inline
void heapsort(T * array, const size_t n, Compare cmp = Compare()) 
{
  Aleph::Inversed_Compare<T, Compare> inv_cmp(cmp);

  --array; //desplazar posición hacia atrás ==> array[1] == primero
  for (size_t i = 2; i <= n; ++i)
    sift_up <T, Aleph::Inversed_Compare<T, Compare>> (array, 1, i, inv_cmp);
  for (size_t i = n; i > 1; --i)
    {
      std::swap(array[1], array[i]); // poner en raíz i-ésimo item 
      sift_down<T,Aleph::Inversed_Compare<T,Compare>>(array, 1, i - 1, inv_cmp);
    }
}

    /** Ordena un arreglo por el método heapsort mejorado.

        faster_heapsort(array,n) emplea una versión substancialmente 
        mejorada del heapsort para ordenar el arreglo a de n elementos.

        El heapsort tiene un desempeño garantizado de \f$O(n \; \lg n)\f$
        y es estable.
        
        El método emplea dos parámetros tipo:
        -# T: el tipo de elementos que tiene el arreglo.
        -# Compare: clase de comparación.

        Una versión sobrecargada faster_heapsort() especializa la 
        comparación al operador "menor que" sobre el tipo T.

        @param[in,out] array el arreglo a ordenar.
        @param[in] n la dimensión del arreglo.
        @see insertion_sort() quicksort_rec() mergesort() heapsort() 
        @see selection_sort() heapsort()
        @ingroup Algos
     */
    template <typename T, class Compare = Aleph::less<T>> inline
void faster_heapsort(T * array, const size_t n, Compare cmp = Compare())
{
  Aleph::Inversed_Compare<T, Compare> inv_cmp(cmp);

  --array; // desplazar posición hacia atrás ==> array[1] == primero
  for (size_t i = n/2; i >= 1; --i)
    sift_down(array, i, n, inv_cmp);
  for (size_t i = n; i > 1; --i)
    {
      std::swap(array[1], array[i]); // poner en raíz i-ésimo item 
      sift_down(array, 1, i - 1, inv_cmp);
    }
}

   /// Retorna true si array entre l y r es un heap con criterio de
   /// comparación Compare()()
      template <typename T, class Compare = Aleph::less<T>>
bool valid_heap(T * array, const size_t l, const size_t r,
		Compare cmp = Compare()) 
{
  size_t i;
  for (i = l_index(l) /* i = 2*l */; i <= r; i++)
    if (cmp(array[i], array[u_index (i)]))
      break;
  return i > r;
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
class ArrayHeap :
	public LocateFunctions<ArrayHeap<T, Compare>, T>,
	public FunctionalMethods<ArrayHeap<T, Compare>, T>,
	public GenericItems<ArrayHeap<T, Compare>, T>,
	public EqualToMethod<ArrayHeap<T, Compare>>,
	public StlAlephIterator<ArrayHeap<T, Compare>>
{
  T *            array = nullptr;
  mutable size_t dim;
  size_t         num_items = 0;

  mutable bool array_allocated = true;

  Compare cmp;

  static size_t r_index(const size_t & i)
  {
    return (i << 1) + 1; // multiplica i por 2 y suma 1
  }

public:

  void swap(ArrayHeap & h)
  {
    std::swap(array, h.array);
    std::swap(dim, h.dim);
    std::swap(num_items, h.num_items);
    std::swap(array_allocated, h.array_allocated);
    std::swap(cmp, h.cmp);
  }

  using Item_Type = T;

  using Key_Type = T;

  Special_Ctors(ArrayHeap, T);

  Args_Ctor(ArrayHeap, T);

      /// Constructor con dimensión por omisión.
  ArrayHeap(const size_t d = 1024, Compare __cmp = Compare()) 
    : array(nullptr), dim(d), num_items(0), array_allocated(false), cmp(__cmp)
  { 
    array = new T [d + 1];
    array_allocated = true;
  }

      /// Constructor con arreglo y dimensión.
  ArrayHeap(T * ptr, const size_t & d, Compare __cmp = Compare()) 
    : array(ptr), dim(d), num_items(0), array_allocated(false), cmp(__cmp)
  { 
    // empty
  }

  ArrayHeap(const ArrayHeap & h)
    : dim(h.dim), num_items(h.num_items), cmp(h.cmp)
  {
    array = new T [dim + 1];
    for (size_t i = 1; i <= num_items; ++i)
      array[i] = h.array[i];
  }

  ArrayHeap(ArrayHeap && h) noexcept
    : array_allocated(false), cmp(h.cmp)
    {
      swap(h);
    }

  ArrayHeap & operator = (const ArrayHeap & h)
  {
    if (this == &h)
      return *this;

    if (dim < h.dim)
      {
	T * ptr = new T [h.dim + 1];
	if (array_allocated)
	  delete [] array;
	array = ptr;
	array_allocated = true;
	dim = h.dim;
      }

    for (size_t i = 1; i <= h.num_items; ++i)
      array[i] = h.array[i];
    num_items = h.num_items;
    cmp = h.cmp;

    return *this;
  }

  ArrayHeap & operator = (ArrayHeap && h)
  {
    swap(h);
    return *this;
  }

      /// Destructor.
  virtual ~ArrayHeap() 
  {
    if (array_allocated and array != nullptr)
      delete [] array;
  }

      /// Retorna el menor elemento del heap.
  T & top() throw(std::exception, std::underflow_error)
  {
    if (num_items == 0)
      throw std::underflow_error("Heap is empty");

    return array[1];
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
    if (num_items >= dim)
      throw std::overflow_error("Heap out of capacity"); 

    array[++num_items] = key;  // colocar nuevo elemento
    return sift_up(array, 1, num_items, cmp); 
  }

  T & insert(T && key) throw(std::exception, std::overflow_error)
  {
    if (num_items >= dim)
      throw std::overflow_error("Heap out of capacity"); 

    array[++num_items] = std::move(key);  // colocar nuevo elemento
    return sift_up(array, 1, num_items, cmp); 
  }

  T & put(const T & key) { return insert(key); }

  T & append(const T & key) { return insert(key); }

  T & put(T && key) { return insert(std::forward<T>(key)); }

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

    T ret_val = array[1];
    array[1] = array[num_items--]; 
    sift_down(array, 1, num_items, cmp); 
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
  const size_t & size() const { return num_items; }
  
  /// Retorna true si el heap está vacío
  bool is_empty() const { return num_items == 0; }

      /** Actualiza prioridad de una referencia contenida en el heap.

          update(data) toma una referencia a un elemento dentro del heap,
          presumiblemente modificada, y actualiza su prioridad dentro del
          heap. La idea es que si por alguna razón una prioridad debe ser
          modificada, entonces ésta pueda actualizarse.

          La referencia debe haberse obtenido mediante una llamada previa
          a insert().

          @param[in] data referencia a la entrada dentro del heap que se
          desea actualizar
          @see insert()
          @note Es esencial que data sea una referencia válida al heap. 
          Resultados son impredecibles (y probablemente fatales) si este
          no es el caso.
       */
  void update(T & data) 
  {
    assert(&data >= &array[0] and &data <= &array[dim]);

    const size_t i = &data - array;
    sift_down_up(array, 1, i, num_items, cmp);
    sift_up(array, i, num_items, cmp);
  }

  void remove(T & item)
  {
    item = array[num_items--];
    update(item);
  }

      /// Retorna la i-ésima entrada del heap.
  T & operator [] (const size_t i)
  {
    return array[i];
  }

  struct Iterator : public Array_Iterator<T>
  {
    // Iterator() {}

    Iterator(const ArrayHeap & h) noexcept
    : Array_Iterator<T>(h.array + 1, h.dim, h.num_items) { /* empty */ }
  };

private:

    // super fast array traversal
  template <class Operation>
  bool __traverse(Operation & operation)
  {
    for (size_t i = 1; i <= num_items; ++i)
      if (not operation(array[i]))
	return false;

    return true;
  }

public:

  template <class Operation>
  bool traverse(Operation & operation) const
  {
    return const_cast<ArrayHeap&>(*this).__traverse(operation);
  }

  template <class Operation>
  bool traverse(Operation & operation) 
  {
    return __traverse(operation); 
  }

  template <class Operation>
  bool traverse(Operation && operation = Operation()) const
  {
    return const_cast<ArrayHeap&>(*this).__traverse(operation);
  }

  template <class Operation>
  bool traverse(Operation && operation = Operation()) 
  {
    return traverse(operation);
  }
};

} // end namespace Aleph
# endif /* TPL_ARRAYHEAP_H */

