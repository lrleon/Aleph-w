# ifndef SLINK_NC_H
# define SLINK_NC_H

# include <aleph.H>

namespace Aleph
{
  /** Enlace simple a lista de nodos no circular.

      Slink_Nc modeliza un puntero a un nodo dentro de una lista
      simplemente enlazada no circular. 

      @ingroup Secuencias
      @see Snode Slist DynSlist Dlink 
   */
  class Slink_Nc
  {
  protected:
    Slink_Nc * next;
  
  public:
    /// Constructor vacío.
    Slink_Nc() : next(nullptr) { /* empty */ }

    /// Constructor copia; coloca enlace a que apunte a nullptr.
    Slink_Nc(const Slink_Nc &) : next(nullptr) { /* empty */ }

    ~Slink_Nc() { /* empty */ }

    /// Asignación; coloca enlace a que apunte a nullptr.
    Slink_Nc & operator = (const Slink_Nc & link)
    {
      if (&link == this)
        return *this;

      if (not is_empty())
        throw std::invalid_argument("link is not empty");

      next = nullptr; 
    }

    /// Reinicia enlace a que apunte a nullptr
    void reset()
    {
      assert(this not_eq nullptr);
      next = nullptr;
    }

    /// Retorna true si this está vacío (apunta a nullptr)
    bool is_empty() const
    {
      assert(this not_eq nullptr);
      return next == nullptr;
    }

    /// Retorna el siguiente enlace.
    Slink_Nc *& get_next()
    {
      assert(this not_eq nullptr);
      return next;
    }

    /** Inserta en enlace p después de this.

          insert(p) inserta un nodo simple cuyo enlace es p después
          del nodo cuyo enlace es this.

          @param[in] p el enlace simple a insertar.
       */
    void insert(Slink_Nc * p)
    {
      assert(this not_eq nullptr);
      assert(p not_eq nullptr);
      assert(p->is_empty());
      p->next = next;
      next = p;
    }

    /** Elimina el enlace siguiente a this.

          remove_next() elimina el nodo simple situado después
          del nodo cuyo enlace es this y retorna la dirección del enlace 
          eliminado.

          @return  el enlace eliminado.
       */
    Slink_Nc * remove_next()
    {
      assert(this not_eq nullptr);
      Slink_Nc * ret_val = next;
      next = ret_val->next;
      ret_val->reset();
      return ret_val;
    }

    class Iterator
    {
      Slink_Nc * head;
      Slink_Nc * curr;
    
    public:
      Iterator() : head(nullptr), curr(nullptr) { /* Empty */ }

      Iterator(Slink_Nc * head_ptr) : head(head_ptr), curr(head->get_next())
      { 
        // empty
      }

      Iterator(Slink_Nc & _head) : head(&_head), curr(head->get_next())
      { 
        // empty
      }

      Iterator(Slink_Nc * head_ptr, Slink_Nc * curr_ptr)
        : head(head_ptr), curr(curr_ptr)
      {
        // empty
      }

      void reset_first() /// Reinicia iterador a primer nodo de la lista
      {
        assert(curr != nullptr and head != nullptr);
        curr = head->get_next();
      }

      void set(Slink_Nc * new_curr) 
      {
        assert(curr != nullptr and head != nullptr);
        curr = new_curr;
      }

      void reset(Slink_Nc * new_head)
      {
        head = new_head;
        curr = head->get_next();;
      }

      bool has_current() const 
      {
        assert(head != nullptr);
        return curr != nullptr;
      }

      // sinónimo de has_current()
      bool has_curr() const { return has_current(); }

      /// Retorna dirección de nodo actual
      Slink_Nc * get_current()  
      {
        assert(curr != nullptr and head != nullptr);

        if (not has_current())
          throw std::overflow_error("Not element in list");

        return curr;
      }

      // sinónimo de get_current()
      Slink_Nc * get_curr() { return get_current(); }

      /// Retorna true si iterador está sobre primer elemento
      bool is_in_first() const { return curr == head->next; }

      /// Avanza iterador en una posición 
      void next() throw(std::exception, std::overflow_error)
      {
        if (not has_current())
          throw std::overflow_error("Not next element in list");

        curr = curr->get_next();
      }

      /// Retorna true si iteradores están sobre el mismo elemento.
      bool operator == (const Iterator & it) const { return curr == it.curr; }

      /// retorna true si iteradores tienen estados diferentes.
      bool operator != (const Iterator & it) const { return curr != it.curr; }
    
      bool verify(Slink_Nc * l) const { return head == l; }

      bool verify(const Iterator & it) const { return head == it.head; }
    };
    
  };
}

# endif

