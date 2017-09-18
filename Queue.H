# ifndef AH_QUEUE_H
# define AH_QUEUE_H

# include <tpl_dynListQueue.H>

namespace Aleph {

    /** Implantación Aleph del contenedor estándar queue<T>.

	En Aleph este contenedor estándar es implantado mediante una
	lista circular simplemente enlazada. Consecuentemente, esta
	versión de queue<T> no emplea un contenedor genérico como
	parámetro.

	@ingroup stl
     */
    template <class T>
class queue : public Aleph::DynListQueue<T>
{
public:

      /// El tipo numérico para representar el tamaño
  typedef size_t size_type;

      /// Instancia una cola vacía
  queue() { /* empty */ }

       /// Instancia una cola con los elementos del contenedor cont.
      template <class Container>
  queue(Container & cont)
  {
    const typename Container::iterator end = cont.end();

    for (typename Container::iterator it(cont.begin()); it != end; put(it++));
  }
  
      /// Inserta value en la cola.
  void push(const T & value) { put(value); }

      /// Elimina un elemento del frente de la cola.
  void pop() { this->get(); }

      /// Consulta el elemento trasero de la cola.
  T & back() { return this->rear(); }

      /// Retorna true la cola está vacía.
  bool empty() const { return this->is_empty(); }
};

} // end namespace Aleph

# endif // AH_QUEUE_H

