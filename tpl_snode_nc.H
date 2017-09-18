# ifndef TPL_SNODE_NC_H
# define TPL_SNODE_NC_H

# include <slink_nc.H>

namespace Aleph
{
  template <typename T> class Snode_Nc : public Slink_Nc
  {
    T data;

  public:
    /// Constructor vacío.
    Snode_Nc() : Slink_Nc() { /* empty*/ }

    /// Constructor que copia dato.
    Snode_Nc(const T & _data) : Slink_Nc(), data(_data) { /* empty */ }

    ~Snode_Nc() { /* empty */ }

    /// Retorna una referencia al dato contenido en el nodo.
    T & get_data() { return data; }
  
    /** Elimina el nodo siguiente a this.

        remove_next() elimina el nodo enlazado después de this y retorna
        su dirección de memoria.
          
        @return el nodo eliminado.
    */
    Snode_Nc * remove_next()
    { 
      return (Snode_Nc *) Slink_Nc::remove_next();
    }
    
    /// Retorna el nodo siguiente a this
    Snode_Nc *& get_next()
    {
      return (Snode_Nc *&) Slink_Nc::get_next();
    }

    class Iterator : public Slink_Nc::Iterator
    {
    public:
      Iterator() { /* empty */ }

      /// Instancia iterador en lista cuyo nodo cabecera es head_ptr 
      Iterator(Snode_Nc<T> * head_ptr)
        : Slink_Nc::Iterator(head_ptr)
      {
        // empty
      }

      /// Instancia iterador en lista cuyo nodo cabecera es head_ptr */
      Iterator(Snode_Nc<T> & head) : Slink_Nc::Iterator(&head) { /* empty */ }

      // Instancia iterador en lista cuyo nodo cabecera es head_ptr y
      // coloca el elemento actual en el apuntado por curr_ptr 
      Iterator(Snode_Nc<T> * head_ptr, Snode_Nc<T> * curr_ptr) 
        : Slink_Nc::Iterator(head_ptr, curr_ptr)
      {
        // Empty
      }

      Snode_Nc * get_current() /// retorna puntero al nodo actual 
      {
        return (Snode_Nc *) this->Slink_Nc::Iterator::get_current(); 
      }

      Snode_Nc * get_curr() /// retorna puntero al nodo actual 
      {
        return Snode_Nc::Iterator::get_current(); 
      }
    };
  };
}

# endif

