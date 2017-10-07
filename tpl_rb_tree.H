
# ifndef TPL_RB_TREE_H
# define TPL_RB_TREE_H

# include <ahFunction.H>
# include <tpl_arrayStack.H>
# include <tpl_binNodeUtils.H>
# include <rbNode.H>

using namespace Aleph;

namespace Aleph {

   /** Árbol binario de búsqueda rojo-negro.

       Un árbol binario de búsqueda rojo negro es un árbol binario de búsqueda
       cuya altura está acotada a \f$O(\lg n)\f$ y sus operaciones de
       modificación acotadas en tiempo por \f$O(\lg n)\f$ nodos
       inspeccionados. 

       Esta clase es genérica en el sentido de que maneja nodos con o
       sin destructor virtual. No está destinada a usarse
       normalmente. En su lugar, use las clases derivadas Rb_Tree o
       Rb_Tree_Vtl. 

       @param NodeType el tipo de nodo entre RbNode y RbNodeVtl (no
       están documentados). 
       @param Key el tipo de clave que albergan los nodos del árbol.
       @param Compare clase de comparación entre las claves.
       @see Rb_Tree Rb_Tree_Vtl
       @ingroup Arboles
    */
  template <template <typename> class NodeType, typename Key, class Compare>
class Gen_Rb_Tree 
{
public:

  typedef NodeType<Key> Node;

private:

  Node              head_node; // cabecera centinela
  Node *            head;      // puntero a centinela
  Node *&           root; 
  FixedStack<Node*> rb_stack; 
  Compare           cmp;

  Node * search_and_stack_rb(const Key & key)             
  {
    Node * p = root; 
    rb_stack.push(head);
    do 
      {
        rb_stack.push(p); 
        if (cmp(key, KEY(p)))
          p = LLINK(p);
        else if (cmp(KEY(p), key))
          p = RLINK(p);
        else
          return p;
      }
    while (p != Node::NullPtr);

    return rb_stack.top();
  }

  Node * search_dup_and_stack_rb(const Key & key)                 
  {
    Node * p = root; 
    rb_stack.push(head);
    do 
      {
        rb_stack.push(p); 
        if (cmp(key, KEY(p)))
          p = LLINK(p);
        else 
          p = RLINK(p);
      }
    while (p != Node::NullPtr);

    return rb_stack.top();
  }

  void fix_red_condition(Node * p)
  {
    assert(COLOR(p) == RED);

    while (p != root) 
      {
        Node * pp = rb_stack.pop(); // padre de p
        if (COLOR(pp) == BLACK) // ¿padre de p negro?
          break; // sí ==> no hay rojos consecutivos ==> terminar

        if (root == pp) // ¿p es hijo directo de la raíz?
          {    // sí ==> colorear raíz de negro y terminar
            COLOR(root) = BLACK; 
            break; 
          }

        Node * ppp = rb_stack.pop(); // abuelo de p
        Node * spp = LLINK(ppp) == pp ? RLINK(ppp) : LLINK(ppp); // tío
        if (COLOR(spp) == RED) // ¿tío de p rojo?
          {     // intercambiar colores entre los niveles
            COLOR(ppp) = RED;
            COLOR(pp)  = BLACK;
            COLOR(spp) = BLACK;
            p = ppp;
            continue; // ir próximo ancestro, verificar violaciones
          }

        Node * pppp = rb_stack.pop(); // bisabuelo de p
        if (LLINK(pp) == p and LLINK(ppp) == pp)
          {
            rotate_to_right(ppp, pppp);
            COLOR(pp) = BLACK;
          }
        else if (RLINK(pp) == p and RLINK(ppp) == pp)
          {
            rotate_to_left(ppp, pppp);
            COLOR(pp) = BLACK;
          }
        else 
          {
            if (RLINK(pp) == p)
              {
                rotate_to_left(pp, ppp);
                rotate_to_right(ppp, pppp);
              }
            else
              {
                rotate_to_right(pp, ppp);
                rotate_to_left(ppp, pppp);
              }
            COLOR(p) = BLACK;
          }

        COLOR(ppp) = RED;
        break; // árbol es rojo-negro ==> terminar
      }

     rb_stack.empty();
  }

  void find_succ_and_swap(Node * p, Node *& pp)
  {
    Node *& ref_rb_stack = rb_stack.top(); 

    /* Find successor while updating rb_stack */
    Node * fSucc = p;        // successor's parent
    Node * succ  = RLINK(p); // Searching starts from p's right child 
    rb_stack.push(succ);

    while (LLINK(succ) != Node::NullPtr)  // go down to leftmost
      {
        fSucc = succ;
        succ  = LLINK(succ);
        rb_stack.push(succ);
      }

    ref_rb_stack   = succ; /* swap old top with current top */
    rb_stack.top() = p; 

    if (LLINK(pp) == p) /* Setting of parent of p to new child(succ) */ 
      LLINK(pp) = succ;
    else
      RLINK(pp) = succ;

    LLINK(succ) = LLINK(p); /* Swaps left branches */
    LLINK(p)    = Node::NullPtr; 

    if (RLINK(p) == succ) /* For right branches there are two cases */
      { /* successor is just right child of p */   
	RLINK(p)    = RLINK(succ); 
	RLINK(succ) = p;
	pp          = succ;
      }
    else
      { /* Successor is leftmost node descending from right child of p */ 
	Node *succr  = RLINK(succ); 
	RLINK(succ)  = RLINK(p);
	LLINK(fSucc) = p;
	RLINK(p)     = succr;
	pp           = fSucc;
      }

    std::swap(COLOR(succ), COLOR(p));
  }

  void fix_black_condition(Node * p)
  {
    if (COLOR(p) == RED) // ¿p es rojo?
      {     // sí ==> lo pintamos de negro y terminamos
        COLOR(p) = BLACK; // esto compensa el déficit 
        return;
      }

    Node * pp = rb_stack.popn(2); // padre de p
    while (p != root)
      {
        assert(LLINK(pp) == p or RLINK(pp) == p);
        assert(LLINK(rb_stack.top()) == pp or RLINK(rb_stack.top()) == pp);

        Node * sp = LLINK(pp) == p ? RLINK(pp) : LLINK(pp); // hermano p
        if (COLOR(sp) == RED) // ¿hermano de p es rojo?
          {
            Node *& ppp = rb_stack.top(); // abuelo de p

            if (LLINK(pp) == p)
              {
                sp  = LLINK(sp);
                ppp = rotate_to_left(pp, ppp);
              }
            else
              {
                sp  = RLINK(sp);
                ppp = rotate_to_right(pp, ppp);
              }

            COLOR(ppp) = BLACK;
            COLOR(pp)  = RED;
          }

        Node * np, * snp; // sobrinos de nodo p
        if (LLINK(pp) == p) // ¿p es hijo izquierdo?
          {     // sí ==> que sp es hijo derechos
            np  = RLINK(sp);
            snp = LLINK(sp);
          }
        else
          {
            np  = LLINK(sp);
            snp = RLINK(sp);
          }

        if (COLOR(np) == RED) // ¿np es rojo?
          {
            Node * ppp = rb_stack.top();

            if (RLINK(sp) == np)
              rotate_to_left(pp, ppp);
             else
              rotate_to_right(pp, ppp);

            COLOR(sp) = COLOR(pp);
            COLOR(pp) = BLACK;
            COLOR(np) = BLACK;

            return;
          }

        if (COLOR(snp) == RED) // ¿snp es rojo?
          {
            Node * ppp = rb_stack.top();

            if (LLINK(sp) == snp)
              {
                rotate_to_right(sp, pp);
                rotate_to_left(pp, ppp);
              }
            else
              {
                rotate_to_left(sp, pp);
                rotate_to_right(pp, ppp);
              }

            COLOR(snp) = COLOR(pp);
            COLOR(pp)  = BLACK;;

            return;
          }

        if (COLOR(pp) == RED) // ¿pp es rojo?
          {
            COLOR(pp) = BLACK;
            COLOR(sp) = RED;;
            return;
          }

          // no hay nodo rojo en las adyacencias de p ==> desplazar el
          // déficit hacia pp y repetir la iteración
        COLOR(sp) = RED;
        p         = pp;
        pp        = rb_stack.pop(); 
      }
  }

public:

     /// El tipo de clave que contiene el nodo
  typedef Key key_type;

      /// Retorna una referencia al criterio de comparación
  Compare & key_comp() { return cmp; }

      /// \overload key_comp()
  Compare & get_compare() { return key_comp(); }

      /// Instancia un árbol rojo-negro.
  Gen_Rb_Tree(Compare __cmp = Compare()) 
    : head(&head_node), root(head_node.getR()), 
      rb_stack(Node::MaxHeight), cmp(__cmp)
  {
    /* empty */ 
  }

  /** Intercambia todos los elementos del treap this con el treap tree
      en tiempo contante (y extremadamente rápido).

      @param[in] tree el treap a intercambiar con this
  */
  void swap (Gen_Rb_Tree & tree)
  {
    std::swap(root, tree.root);
    std::swap(cmp, tree.cmp);
  }

      /// Destruye un árbol rojo-negro.
  virtual ~Gen_Rb_Tree() { /* empty */ }

      /// Busca un nodo con clave key. Retorna un puntero al nodo
      /// contentivo de key si éste se encuentra en el árbol; nullptr de lo
      /// contrario. 
  Node * search(const Key & key)
  {
    Node * retVal = Aleph::searchInBinTree <Node, Compare> (root, key, cmp);
    return retVal == Node::NullPtr ? nullptr : retVal;
  }

      /// Obtiene un puntero a la raíz del árbol.
  Node *& getRoot() { return root; }
      /// Inserta el nodo p en el árbol rojo-negro. Si la clave de p no
      /// está contenida en el árbol, entonces el nodo es insertado y se
      /// retorna el puntero a p. De lo contrario no ocurre la inserción y
      /// se retorna nullptr.
  Node * insert(Node * p)
  {
    assert(COLOR(p) == RED);

    if (root == Node::NullPtr) 
      return root = p; // inserción en árbol vacío

    Node * q = search_and_stack_rb(KEY(p));
    if (cmp(KEY(p), KEY(q))) 
      LLINK(q) = p;
    else if (cmp(KEY(q), KEY(p))) 
      RLINK(q) = p;
    else
      {
        rb_stack.empty();
        return nullptr;  // clave duplicada
      }
    fix_red_condition(p);

    return p;
  }

       /** Busca la clave de p en el árbol rojo negro o lo inserta
           en caso de no encontrarse.

           search_or_insert(p) busca en el árbol rojo-negro un nodo cuya clave
           sea <code>KEY(p)</code>. Si la clave se encuentra, entonces se
           rerona un puntero al nodo que la alberga. De lo contrario se
           inserta p en el árbol binario de búsqueda this. 

           @param[in] p el nodo a buscar o insertar.
           @return puntero al nodo insertado si la clave de p no está
           contenida dentro del árbol; De lo contrario, se retorna un
           puntero al nodo dentro del árbol que contiene a
           <code>KEY(p)</code>. 
       */
  Node * search_or_insert(Node * p)
  {
    assert(COLOR(p) == RED);

    if (root == Node::NullPtr) 
      return root = p; // inserción en árbol vacío

    Node * q = search_and_stack_rb(KEY(p));
    if (cmp(KEY(p), KEY(q))) 
      LLINK(q) = p;
    else if (cmp(KEY(q), KEY(p))) 
      RLINK(q) = p;
    else
      {
        rb_stack.empty();
        return q;  // clave duplicada
      }
    fix_red_condition(p);

    return p;
  }

      /// Inserta el nodo p en el árbol rojo-negro. Si la clave de p no
      /// está contenida en el árbol, entonces el nodo es insertado y se
      /// retorna el puntero a p. De lo contrario no ocurre la inserción y
      /// se retorna nullptr.
  Node * insert_dup(Node * p)
  {
    assert(COLOR(p) == RED);

    if (root == Node::NullPtr) 
      return root = p; // inserción en árbol vacío

    Node * q = search_dup_and_stack_rb(KEY(p));
    if (cmp(KEY(p), KEY(q))) 
      LLINK(q) = p;
    else 
      RLINK(q) = p;

    fix_red_condition(p);

    return p;
  }

  bool verify() const { return is_red_black_tree(root) ; }

      /// Elimina el nodo contentivo de la clave key. Si la clave se
      /// encuentra en el árbol, entonces se elimina el nodo que la
      /// contiene y se retorna su dirección. De lo contrario se retorna
      /// nullptr. 
  Node* remove(const Key & key)
  {
    if (root == Node::NullPtr) 
      return nullptr;

    Node * q = search_and_stack_rb(key);
    if (no_equals<Key, Compare> (KEY(q), key, cmp)) // ¿clave no encontrada?
      {    
        rb_stack.empty();
        return nullptr;
      }

    Node * pq = rb_stack.top(1); // padre de 1
    Node * p; // hijo de q luego de que éste ha sido eliminado
    while (true) // eliminación clásica árbol binario de búsqueda
      {
        if (LLINK(q) == Node::NullPtr) 
          {
            if (LLINK(pq) == q)
              p = LLINK(pq) = RLINK(q);
            else
              p = RLINK(pq) = RLINK(q);

            break; // goto end;
          }

        if (RLINK(q) == Node::NullPtr) 
          {
            if (LLINK(pq) == q)
              p = LLINK(pq) = LLINK(q);
            else
              p = RLINK(pq) = LLINK(q);

            break; // goto end;
          }

        find_succ_and_swap(q, pq);
      }

    if (COLOR(q) == BLACK) // ¿se eliminó un nodo negro?
      fix_black_condition(p);

    q->reset();
    rb_stack.empty();

    return q;
  }

  /** Iterador sobre los nodos.

      Esta clase permite recorrer los nodos del árbol en forma
      ordenada según el criterio de comparación especificado en el
      momento de instanciación del árbol.

     @ingroup Arboles
  */
  struct Iterator : public BinNodeInfixIterator<Node>
  {
    Iterator(Gen_Rb_Tree & t) : BinNodeInfixIterator<Node>(t.getRoot()) {}
  };
};
    /** Árbol binario de búsqueda rojo-negro con nodos sin destructor virtual.

       Un árbol binario de búsqueda rojo negro es un árbol binario de búsqueda
       cuya altura está acotada a \f$O(\lg n)\f$ y sus operaciones de
       modificación acotadas en tiempo por \f$O(\lg n)\f$ nodos
       inspeccionados. 

       Esta clase maneja nodos sin destructor virtual.

       @param Key el tipo de clave que albergan los nodos del árbol.
       @param Compare clase de comparación entre las claves.
       @see Rb_Tree_Vtl
       @ingroup Arboles
    */
    template <typename Key, class Compare = Aleph::less<Key>>
struct Rb_Tree : public Gen_Rb_Tree<RbNode, Key, Compare> 
{
  using Base = Gen_Rb_Tree<RbNode, Key, Compare>;
  using Base::Base;
};

    /** Árbol binario de búsqueda rojo-negro con destructor virtual en
        sus nodos.  

       Un árbol binario de búsqueda rojo negro es un árbol binario de búsqueda
       cuya altura está acotada a \f$O(\lg n)\f$ y sus operaciones de
       modificación acotadas en tiempo por \f$O(\lg n)\f$ nodos
       inspeccionados. 

       Esta clase maneja nodos con destructor virtual.

       @param Key el tipo de clave que albergan los nodos del árbol.
       @param Compare clase de comparación entre las claves.
       @see Rb_Tree_Vtl
       @ingroup Arboles
    */
    template <typename Key, class Compare = Aleph::less<Key>>
struct Rb_Tree_Vtl : public Gen_Rb_Tree<RbNodeVtl, Key, Compare> 
{
  using Base = Gen_Rb_Tree<RbNodeVtl, Key, Compare>;
  using Base::Base;
};

} // end namespace Aleph

# endif // TPL_RB_TREE_H

