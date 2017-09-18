# ifndef TPL_SINGLELIST_H
# define TPL_SINGLELIST_H

# include <ahDefs.H>

    template <class T>
class Single_List 
{
private:

  struct Node
  {
    T      data;
    Node * next; 

    Node * get_by_pos(int pos)
    {
      int i;
      Node * node = this;
      for (i = 0; i < pos; ++i)
        {
          if (node->next == nullptr)
            throw std::overflow_error("overflow in access by position");
          node = node->next;
        }
      
      return node;
    }
    Node() : next (nullptr) { /* empty */ }

    Node(const T& _data) : data(_data), next (nullptr) { /* empty */ }
  };
  int    num_items;
  Node   head_node;
  Node * head;

public:

  unsigned int get_num_items() const { return num_items; }
  T& operator [] (unsigned int pos) Exception_Prototypes(std::overflow_error) 
  { 
    if (pos > num_items) 
      throw std::overflow_error("List overflow");
    return head->get_by_pos(pos + 1)->data;
  }
  void insert(unsigned int pos, const T& t) 
    Exception_Prototypes(std::bad_alloc, std::overflow_error)  
  {
    if (pos > num_items) 
      throw std::overflow_error("List overflow");
    Node * node = new Node (t);
    Node * prev = head->get_by_pos(pos - 1); 
    node->next = prev->next;
    prev->next = node;
    ++num_items; 
  }
  void remove(unsigned int pos) Exception_Prototypes(std::overflow_error)
  { 
    if (pos > num_items) 
      throw std::overflow_error("List overflow");

    Node * prev = head->get_by_pos(pos - 1);
    Node * node_to_delete = prev->next;
    prev->next = node_to_delete->next;
    num_items--;
    delete node_to_delete;
  }
};

# endif
