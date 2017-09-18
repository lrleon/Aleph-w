
# ifndef TPL_DYNSLIST_H
# define TPL_DYNSLIST_H

# include <tpl_slist.H>

using namespace Aleph;

namespace Aleph {

  /** Lista dinámica de elementos de tipo T instrumentada mediante una lista
      simplemente enlazada.

      
      @param T el tipo de dato de los elementos de la lista.

      @see Slist Snode Slink 
      @ingroup Secuencias
  */
    template <typename T>
class DynSlist : public Slist<T>
{

private:

  size_t num_items;
  int        current_pos;
  Snode<T> * current_node;
  typename Slist<T>::Node * get_previous_to_pos(const int & pos)
  {

    if (pos > num_items)
      throw std::out_of_range ("position out of range");

    if (pos < current_pos) // hay que retroceder?
      { // Si, reinicie posición actual
        current_pos  = 0;
        current_node = this;
      }
    while (current_pos < pos)  // avanzar hasta nodo predecesor a pos
      {
        current_node = current_node->get_next();
        ++current_pos;
      }
    return current_node;
  }

public:

    /// Constructor.
  DynSlist() : num_items(0), current_pos(0), current_node(this)
  { 
    // Empty 
  } 
    /** Acceso al i-ésimo elemento de la lista.

        El operador [i] sobre una lista accede al elemento cuya posición
        es i.

        El acceso a la posición i requiere recorrer secuencialmente los i - 1 
        elementos previos. 

        @param[in] i posición del elemento que se desea acceder.
        @throw out_of_range_error si i es mayor que la cantidad de
        elementos de la lista.
        @return una referencia constante al elemento i dentro de la lista.
        @note En términos de desempeño, este operador no es el mismo que
        el acceso sobre un arreglo.
     */
  T & operator [] (const size_t & i) 

    throw(std::exception, std::out_of_range)

  {
    return get_previous_to_pos(i)->get_next()->get_data();
  }    

      /// Retorna la cantidad de elementos que tiene la lista.
  size_t size() const { return num_items; }

      /** Inserta un elemento en la posición pos dentro de una lista
          dinámica.

          insert(pos, data) inserta en la posición pos de la lista this el
          elemento cuyo valor es data.

          @param[in] pos posición de inserción.
          @param[in] data valor del elemento a insertar.
          @throw bad_alloc si no hay suficiente memoria.
          @throw out_of_range si pos está fuera de rango.
       */
  void insert(const int & pos, const T & data) 

    throw(std::exception, std::bad_alloc, std::out_of_range)

  {      // apartar nodo para nuevo elemento
    typename Slist<T>::Node * node = new typename Slist<T>::Node (data);
    typename Slist<T>::Node * prev = get_previous_to_pos(pos); 
    prev->insert_next(node);
    ++num_items;
  }
      /** Elimina de la lista el elemento es la posición pos.

          @param[in] pos posición del elemento a eliminar.
          @throw out_of_range si pos está fuera de rango.
       */
  void remove(const int & pos) 

    throw(std::exception, std::range_error)

  {      // obtener nodo predecesor al nuevo elemento
    typename Slist<T>::Node * prev = get_previous_to_pos(pos);
    typename Slist<T>::Node * node_to_delete = prev->remove_next();
    delete node_to_delete;
    --num_items;
  }
      /// Destructor.
  ~DynSlist() 
  {      // eliminar nodo por nodo hasta que la lista devenga vacía
    while (not this->is_empty())
      delete this->remove_first(); // remove_first de clase Slink
  }
    /** Iterador sobre listas dinámicas implementadas con listas
        simplemente enlazadas. 

       @ingroup Secuencias
     */
  class Iterator : public Slist<T>::Iterator
  {
  public:
        /// El tipo de conjunto sobre el cual se itera.
    typedef Slist<T> Set_Type;
        /// El tipo de elemento que retorna get_current().
    typedef T Item_Type;

        /// Constructor.
    Iterator(DynSlist & list) : Slist<T>::Iterator(list) { /* Empty */ } 

        /// retorna una referencia al elemento actual.
    T & get_current() { return Slist<T>::Iterator::get_current()->get_data(); }
  };

};

} // end namespace Aleph

# endif // TPL_DYNSLIST_H

