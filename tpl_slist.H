
# ifndef TPL_SLIST_H
# define TPL_SLIST_H

# include <ahDefs.H>
# include <tpl_snode.H>

using namespace Aleph;

namespace Aleph {

  /** Lista simplemente enlazada de nodos conteniendo datos de tipo T.

      Slist<T> es una lista cuyos elementos son nodos. Cada nodo
      contiene un dato de tipo genérico T.

      @param T el tipo de dato que alberga cada nodo.
      @ingroup Secuencias
   */
template <typename T> class Slist : public Snode<T>
{

public:

  typedef Snode<T> Node;

    /// Constructor vacío.
  Slist() { /* empty */ }

    /** Inserta un nodo después de this.

        insert_first(node) inserta el nodo justamente después del nodo
        this. 

        @param[in] node el nodo a insertar.
     */
  void insert_first(Node * node)
  {
    assert(node not_eq nullptr);
    assert(node->is_empty());

    this->insert_next(node);
  }

    /** Elimina el primer nodo de la lista y retorna su dirección.

        Elimina el primer nodo de la lista.

        @throw underflow_error si la lista está vacía.
        @return un puntero al nodo eliminado.
     */
  Node * remove_first() 
    throw(std::exception, std::underflow_error)
  {
    if (this->is_empty())
      throw std::underflow_error ("list is empty");

    return this->remove_next();
  }

    /// Retorna el primer nodo de la lista.
  Node * get_first() 
    Exception_Prototypes(std::underflow_error)
  {
    if (this->is_empty())
      throw std::underflow_error ("list is empty");

    return this->get_next(); 
  }

      /** Iterador sobre lista de nodos simples.

          Esta clase define un iterador sobre una lista simplemente enlazada 
          de nodos.

          @ingroup Secuencias
          @see Slist
       */
  class Iterator
  {

  private:

    Slist * list;
    Node  * current;

  public:
        /// El tipo de conjunto sobre el cual se itera.
    typedef Snode<T> Set_Type;
        /// El tipo de elemento que retorna get_current().
    typedef Snode<T> * Item_Type;

      /** Constructor.

          Construye un iterador sobre la lista de nodos list.

          @param[in] _list la lista sobre la cual se desea iterar.
      */
    Iterator(Slist & _list) : list(&_list), current(list->get_first()) {}

        /// Retorna true si el iterador tiene nodo actual.
    bool has_current() const { return current != list; }

        /** Retorna el nodo actual sobre el cual está posicionado el iterador.

            @return un apuntador al elemento actual.
            @throw overflow_error si el iterador no está posicionado sobre 
            un elemento. 
         */
    Node * get_current() 

      throw(std::exception, std::overflow_error)

    {

      if (not this->has_current())
        throw std::overflow_error ("");

      return current;
    }
        /** Avanza el iterador hacia adelante.

            Avanza el iterador hacia el elemento siguiente del actual.

            Si el iterador se encuentra en el último elemento de la secuencia,
            entonces éste queda en un estado en que no hay elemento actual.

            @throw overflow_error si el iterador no se encuentra sobre un elemento 
            actual.
         */
    void next()

      throw(std::exception, std::overflow_error)

    {

      if (not this->has_current())
        throw std::overflow_error ("");

      current = current->get_next();
    }
        /// Coloca el iterador en el primer elemento de la lista.
    void reset_first() { current = list->get_next(); }
    Iterator & operator = (Node * node)
    {
      if (this == node) 
        return *this;

      current = node;
      return *this;
    }
  };
};

} // end namespace Aleph

# endif /* TPL_SLIST_H */

