# ifndef ALEPH_IT_H
# define ALEPH_IT_H


# define DEFINE_ITERATOR(Name, Base_Name, Set_Name, Item_Type)	\
class Name : public Base_Name
  {
  public:

    /// Tipo de elemento que retorna get_current()
    typedef Node * Item_Type;

    /// El tipo de conjunto sobre el cual se itera.
    typedef List_AGraph Set_Type;

    Node_Iterator() { /* empty */ }

    Node_Iterator(List_AGraph & g) : Base_Name::Iterator(g.node_list)
    {
      // empty
    }

    /// retorna el nodo actual.
    Node * get_current_node()
    {
      return DynArray<Node*>::Iterator::get_curr();
    }

    /// \overload get_current_node()
    Node * get_current() { return get_current_node(); }

    /// \overload get_current_node()
    Node * get_curr() { return get_current_node(); }
  };



# endif // ALEPH_IT_H
