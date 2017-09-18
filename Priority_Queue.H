# ifndef AH_PRIORITY_QUEUE_H
# define AH_PRIORITY_QUEUE_H

# include <tpl_dynBinHeap.H>

namespace Aleph {

    /** Implantación Aleph del contenedor estándar prority_queue<T>.

	En Aleph este contenedor estándar es implantado mediante una
	la clase DynBinHeap, la cual, a su vez está basada en árboles
	binarios. Esta implantación so se sustenta sobre un arreglo y es
	altamente dinámica. Otras estructuras o algoritmos pueden
	mantener sin riesgo punteros hacia los datos que contiene la cola de
	prioridad.

	Esta versión de Priority_Queue<T> no emplea un contenedor
	genérico como parámetro tipo. Consecuentemente, los
	constructores estándar están ligeramente modificados.

	@see DynBinHeap
	@ingroup stl
     */
    template <typename T, class Compare = Aleph::less<T> >
class priority_queue : private Aleph::DynBinHeap<T, Compare>
{
public:

      /// Tipo de valor que almacena la cola de prioridad.
  typedef T value_type;

      /// Instancia una cola de prioridad vacía
  priority_queue() { /* empty */ }

      /// Instancia una cola con todos los elementos del contenedor cont.
      template <class Container>
  priority_queue(Container & cont)
  {
    const typename Container::iterator end = cont.end();

    for (typename Container::iterator it(cont.begin()); 
	 it != end; this->insert(*it++));
  }

      /// Instancia una cola con los elementos comprendidos entre el
      /// rango [beg..end) del contenedor asociado a los iteradores.
      template <class Itor>
  priority_queue(Itor beg, const Itor & end)
  {
    Aleph::verify_iterators (beg, end);

    while (beg != end)
      this->insert(*beg++);
  }

      /// Inserta una copia de value en la cola de prioridad; retorna
      /// una referencia modificable a la copia insertada (esto no es
      /// parte del estándar).
  T & push(const T & value) { return this->insert(value); }

      /// Elimina el menor elemento de la cola de prioridad según el
      /// criterio de comparación especificado en la declaración.
  void pop() { this->getMin(); }

      /// Retorna el menor elemento de la cola de prioridad según el
      /// criterio de comparación especificado en la declaración.
  const T & top() { return Aleph::DynBinHeap<T, Compare>::top(); }

      /// Retorna true si la cola de prioridad está vacía.
  bool empty() const { return this->is_empty(); }

      /// Retorna la cantidad de elementos de la cola.
  size_t size() const { return Aleph::DynBinHeap<T, Compare>::size(); }
};

} // end namespace Aleph

# endif // AH_PRIORITY_QUEUE_H
