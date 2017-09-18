# ifndef TPL_STD_ITRO_H
# define TPL_STD_ITRO_H

namespace Aleph {


    /** Iterador sobre un un conjunto de la biblioteca estándar C++
	implantado con Aleph.   

	@ingroup Arboles Contenedores
    */
  template <class Tree_Type>
class Tree_Iterator
{
private:

  friend class Set_Type;
    
  typedef typename Tree_Type::Iterator Iterator;
    
  Iterator itor;
  bool underflow;
  bool overflow;

  void init_flags ()
  {
    if (itor.has_current () )
      underflow = overflow = false;
    else
      underflow = overflow = true;
  }

  void goto_begin ()
  {
    itor.reset_first ();
    init_flags ();
  }

  void goto_last ()
  {
    itor.reset_last ();
    init_flags ();
  }

  void goto_end ()
  {
    itor.reset_last ();
    init_flags ();
    overflow = true;
    if (not size() != 0)
      itor.next ();
  }

  void forward ()
  {
    if (underflow)
      {
	goto_begin ();
	return;
      }
      
    itor.next ();

    if (not itor.has_current () )
      overflow = true;
  }

  void backward ()
  {
    if (overflow)
      {
	goto_last ();
	return;
      }
      
    itor.prev ();

    if (not itor.has_current () )
      underflow = true;
  }

  iterator (Tree_Type & _tree, Node * node) 
    : itor (_tree, node), underflow (false), overflow (false)
  {
    /* empty */
  }

public:

  typedef typename map::value_type    value_type;
  typedef typename map::size_type     difference_type;
  typedef typename map::value_type *   pointer;
  typedef typename map::reference     reference;

  /// Constructor vacío; no tiene validez si no se asocia un conjunto
  iterator () { /* empty */ }

  iterator (Tree_Type & tree) : itor (tree)
  {
    init_flags ();
  }

  /// Proporciona una referencia al elemento actual.
  Pair & operator * ()
  {
    return KEY (itor.get_current () );
  }

  /// "Dereferencia" un puntero al elemento actual.
  Pair * operator -> ()
  {
    return &KEY (itor.get_current () );
  }

  /// Avanza el iterador una posición hacia adelante, luego carga
  /// el elemento actual.
  Pair & operator ++ ()
  {
    forward ();

    return KEY (itor.get_current () );
  }

  Pair & operator ++ (int)
  {
    Pair & retPair = KEY (itor.get_current () );
    forward ();

    return retPair;
  }

  Pair & operator -- ()
  {
    backward ();

    return KEY (itor.get_current () );
  }

  Pair & operator -- (int)
  {
    Pair & retPair = KEY (itor.get_current () );
    backward ();

    return retPair;
  }
    
  Pair & operator += (const size_type & n)
  {
    itor.reset_to_pos (itor.get_current_position () + n);

    return KEY (itor.get_current () );
  } 

  Pair & operator -= (const size_type & n)
  {
    itor.reset_to_pos (itor.get_current_position () - n);

    return KEY (itor.get_current () );
  } 

  bool operator == (const iterator & _itor) const
  {
    return itor == _itor.itor;
  }

  bool operator != (const iterator & _itor) const
  {
    return not (itor == _itor.itor);
  }

  bool verify (const map & _map) const
  {
    return itor.verify ( (Tree_Type*) &_map.tree); 
  }

  bool verify (const iterator & it) const
  {
    return itor.verify (it.itor);
  }
};



} // end namespace Aleph

# endif // TPL_STD_ITRO_H
