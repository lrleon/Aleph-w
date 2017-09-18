# ifndef AH_STACK_H
# define AH_STACK_H

# include <tpl_dynListStack.H>


namespace Aleph {

    /** Implantación Aleph del contenedor estándar stack<T>.

	En Aleph este contenedor estándar es implantado mediante una
	lista circular simplemente enlazada. Consecuentemente, esta
	versión de stack<T> no emplea un contenedor genérico como
	parámetro.

	@ingroup stl
     */
    template <typename T>
class stack : public Aleph::DynListStack<T>
{
public:

      /// El tipo de valor que guarda la pila.
  typedef T value_type;

      /// Tipo numérico para tamaño de pila.
  typedef size_t size_type;

      /// Instancia una pila vacía. 
  stack() { /* empty */ }

      /// Instancia una pila con los elementos del contenedor cont.
      template <class Container>
  stack(Container & cont)
  {
    const typename Container::iterator end = cont.end();

    for (typename Container::iterator it(cont.begin()); it != end; push(it++));
  }
  
      /// Retorna true si la pila está vacía.
  bool empty() const { return this->is_empty(); }
};

} // end namespace Aleph

# endif // AH_STACK_H
