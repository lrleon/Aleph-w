
# ifndef FILTER_ITERATOR_H
# define FILTER_ITERATOR_H

# include <stdexcept>

namespace Aleph {


    /** Iterador filtro sobre otro iterador.

        Un iterador filtro es un iterador que recibe como tipos
        parametrizados los siguientes tipos:
        -# Container: un contenedor de elementos entre la mayor parte de
        los definidos en Aleph. 
        -# It: una clase iterador que debe pertenecer al iterador para
        la clase Container. Puesto que pudieran existir varias clases de
        iterador asociados (Container::It1, Container::It2, ...).
        -# Show_Item: una clase que determina si un elemento del
        conjunto debe o no ser mostrado por el iterador. La
        determinación anterior se realiza mediante la llamada lógica (bool)
        Show_Item::operator()(Container&, It, T) la cual debe retornar
        true si el elemento debe mostrarse o false de lo contrario.
        .

        El fin de Filter_Iterator es exportar un iterador genérico que
        filtre algunos elementos del conjunto según algún criterio
        definido por la clase Show_Item. Esto permite que algoritmos
        genéricos que usen Filter_Iterator tengan distintos
        comportamientos según el filtro Show_Item.

        Los contenedores de Aleph exportan dos tipos muy importantes en
        el contexto de Filter_Iterator: Set_Type e Item_Type, los
        cuales se corresponden al tipo de conjunto sobre el cual se
        itera y al tipo de elemento que retorna el iterador It.

        @note Esta clase está diseñada para Aleph, no funciona para la
        biblioteca estándar std.
        @ingroup Secuencias
     */
    template <class Container, class It, class Show_Item>
class Filter_Iterator : public It
{
  Show_Item show_item; 

  Container * container_ptr = nullptr;

  void * cookie = nullptr;

  void goto_first_valid_item() 
  {
    try
      {     // colocarse en el primer elemento que acepte show_item
        for (this->It::reset_first(); true; this->It::next())
          if (not this->It::has_curr() or show_item(this->It::get_curr()))
            return;
      }
    catch (std::overflow_error) { /* seguir en overflow; no propagar */ }
  }

  void forward()
  {
    this->It::next(); 
    try 
      {     // avanzar hasta el siguiente item que acepte show_item
        for (;true; this->It::next()) 
	  {
            if (not this->It::has_curr() or show_item(this->It::get_curr()))
	      return ;
	  }
      }
    catch (std::overflow_error) { /* seguir en overflow; no propagar */ }
  }

  void goto_last_valid_item()
  {
    for (this->It::reset_last(); true; this->It::prev())
      if (not this->It::has_curr() or show_item(this->It::get_curr()))
        return;
  }

  void backward()
  {
    this->It::prev();
    try 
      {     
        for (;true; this->It::prev()) 
          if (not this->It::has_curr() or show_item(this->It::get_curr()))
            return;
      }
    catch (std::underflow_error) { /* seguir en underflow sin propagar */ }
  }
 
public:

      /// Tipo de elemento que retorna get_current()
  typedef typename It::Item_Type Item_Type;

     /// El tipo de iterador.
  typedef It Iterator_Type;

  const Container & get_container() const
  {
    return *const_cast<Container*>(container_ptr);
  }

      /// Retorna el iterador de fondo.
  It & get_iterator() { return *this; }

      /// Retorna una referencia a la clase filtro. Úsese esta función
      /// si se requiere obtener el estado del filtro (recuérdese que se
      /// perderá cuando se invoque al destructor ~Filter_Iterator()
  Show_Item & get_filter() { return show_item; }

  void set_filter(Show_Item si)
  {
    show_item = si;
  }

  Filter_Iterator(Show_Item si = Show_Item()) : show_item(si)
  {
    // empty
  }

  /// Iterador sobre conjunto cont_ptr con filtro si
  Filter_Iterator(const Container & c, Show_Item si = Show_Item()) 
    : It(c), show_item(si), container_ptr(&const_cast<Container&>(c))
  {
    goto_first_valid_item();
  }

  /// Iterador sobre conjunto cont_ptr con filtro si
  Filter_Iterator(const Container & c, void * __cookie,
		  Show_Item si = Show_Item()) 
    : Filter_Iterator(c, si)
  {
    cookie = __cookie;
  }

  void set_cookie(void * __cookie) { cookie = __cookie; }

      /// Adelanta el iterador una posición.
  void next() { forward(); }

      /// Retrocede el iterador una posición.
  void prev() { backward(); }

      /// Coloca el iterador sobre el primer elemento de la secuencia.
  void reset_first() { goto_first_valid_item(); }

      /// Coloca el iterador sobre el último  elemento de la secuencia.
  void reset_last() { goto_last_valid_item(); }
};


} // end namespace Aleph
# endif // FILTER_ITERATOR_H

