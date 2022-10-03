
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

# ifndef TPL_AVL_H
# define TPL_AVL_H

# include <algorithm>
# include <ahFunction.H>
# include <tpl_arrayStack.H>
# include <avlNode.H>
# include <tpl_binNodeUtils.H>

using namespace Aleph;

namespace Aleph {

   /** AVL balanced binary search tree.

       An AVL tree is an binary search tree deterministically
       balanced. The maximum height of AVL tree of \f$n\f$ nodes is
       \f$ 1.4404 \lg{(n + 2)} \ - \ 0.3277\f$. So this number bounds
       the maximum number of visited nodes in a unsuccessful search,
       what bounds the traditional operations (insertion, search and
       removal) to \f$O(\lg n)\$f complexity.

       @see Avl_Tree Avl_Tree_Vtl
       @ingroup Arboles
    */
  template <template <typename> class NodeType, typename Key, class Compare>
class Gen_Avl_Tree
{
public:

  using Node = NodeType<Key>; /// The type of node

private:

  FixedStack<Node *> avl_stack;
  Node               head_node;
  Node *             head_ptr;
  Node *&            root;
  Compare            cmp;

  bool avl_stack_empty() noexcept { return avl_stack.top() == head_ptr; }

  void clean_avl_stack() noexcept { avl_stack.popn (avl_stack.size() - 1); }

  Node * search_and_stack_avl(const Key &  key) noexcept
  {
    assert(avl_stack_empty());

    Node * p = root;
    do // desciende en búsqueda de key y empila camino de búsqueda
      {
        avl_stack.push(p);
        if (cmp(key, KEY(p))) // ¿key < KEY(p)?
          p = LLINK(p);
        else if (cmp(KEY(p), key)) // ¿key > KEY(p)?
          p = RLINK(p);
        else
          return p; // clave duplicada
      }
    while (p != Node::NullPtr);

    return avl_stack.top();
  }

  Node * search_dup_and_stack_avl(const Key &  key) noexcept
  {
    assert(avl_stack_empty());

    Node * p = root;
    do // desciende en búsqueda de key y empila camino de búsqueda
      {
        avl_stack.push(p);
        if (cmp(key, KEY(p))) // ¿key < KEY(p)?
          p = LLINK(p);
        else // key >= KEY(p)
          p = RLINK(p);
      }
    while (p != Node::NullPtr);

    return avl_stack.top();
  }

  static Node * rotateLeft(Node * p) noexcept
  {
    assert(DIFF(p) == 2);
    assert(RLINK(p) != Node::NullPtr);

    Node * q = RLINK(p);
    RLINK(p) = LLINK(q);
    LLINK(q) = p;

    if (DIFF(q) == 0)      // ajuste de los factores de equilibrio
      { // este caso ocurre durante la eliminación
        DIFF(q) = -1;
        DIFF(p) = 1;
      }
    else
      DIFF(q) = DIFF(p) = 0;

    return q;
  }

  static Node * rotateRight(Node * p) noexcept
  {
    assert(DIFF(p) == -2);
    assert(LLINK(p) != Node::NullPtr);

    Node * q = LLINK(p);
    LLINK(p) = RLINK(q);
    RLINK(q) = p;

    if (DIFF(q) == 0)       // ajuste de los factores de equilibrio
      { // este caso ocurre durante la eliminación
        DIFF(q) = 1;
        DIFF(p) = -1;
      }
    else
      DIFF(q) = DIFF(p) = 0;

    return q;
  }

  static Node * doubleRotateLeft(Node * p) noexcept
  {
    assert(DIFF(p) == 2 or DIFF(p) == -2);
    assert(RLINK(p) != Node::NullPtr and LLINK(RLINK(p)) != Node::NullPtr);

    Node * q = RLINK(p);
    Node * r = LLINK(q);
    RLINK(p) = LLINK(r);
    LLINK(q) = RLINK(r);
    LLINK(r) = p;
    RLINK(r) = q;

    unsigned char b; // altura lógica de hijo izq de r
    unsigned char c; // altura lógica de hijo der de r

        // Determinación de alturas lógicas de p, q y r
    if (DIFF(r) == 1) // ==> c > b ==> c-b == 1
      {
        c = 1;
        b = 0;
      }
    else if (DIFF(r) == -1) // ==> c < b ==> c-b = -1
      {
        c = 0;
        b = 1;
      }
    else
      c = b = 1;

        // ajuste de factores de equilibrio
    DIFF(r) = 0;
    DIFF(p) = b - 1; // altura lógica de hijo izq de p es 1
    DIFF(q) = 1 - c; // altura lógica de hijo der de q es 1

    return r;
  }

  static Node * doubleRotateRight(Node * p) noexcept
  {
    assert(DIFF(p) == 2 or DIFF(p) == -2);
    assert(LLINK(p) != Node::NullPtr and RLINK(LLINK(p)) != Node::NullPtr);

    Node * q  = LLINK(p);
    Node * r  = RLINK(q);
    LLINK(p) = RLINK(r);
    RLINK(q) = LLINK(r);
    RLINK(r) = p;
    LLINK(r) = q;

    unsigned char b; // altura lógica de hijo izq de r
    unsigned char c; // altura lógica de hijo der de r

        // determinación de alturas lógicas de hijos de p, q y r
    if (DIFF(r) == 1) // ==> c > b ==> c-b == 1
      {
        c = 1;
        b = 0;
      }
    else if (DIFF(r) == -1) // ==> c < b ==> c-b == -1
      {
        c = 0;
        b = 1;
      }
    else
      c = b = 1;

        // ajuste de factores de equilibrio
    DIFF(r) = 0;
    DIFF(p) = 1 - c; // altura lógica de hijo der de p es 1
    DIFF(q) = b - 1; // altura lógica de hijo izq de p es 1

    return r;
  }

  enum Rotation_Type
    { ROTATE_LEFT, ROTATE_RIGHT, DOUBLE_ROTATE_LEFT, DOUBLE_ROTATE_RIGHT };

  static Rotation_Type rotation_type(Node * p) noexcept
  {
    assert(DIFF(p) == 2 or DIFF(p) == -2);

    Node * pc; // guarda hijo de p
    if (DIFF(p) == 2) // hacia la izquierda
      {
        pc = RLINK(p);
        if (DIFF(pc) == 1 or DIFF(pc) == 0)
          return ROTATE_LEFT;

        return DOUBLE_ROTATE_LEFT;
      }

    pc = LLINK(p);
    if (DIFF(pc) == -1 or DIFF(pc) == 0)
      return ROTATE_RIGHT;

    return DOUBLE_ROTATE_RIGHT;
  }

  static Node * restore_avl(Node * p, Node * pp) noexcept
  {
    assert(LLINK(pp) == p or RLINK(pp) == p);
    assert(DIFF(p) == -2 or DIFF(p) == 2);

    Node ** link = LLINK(pp) == p ? &LLINK(pp) : &RLINK(pp);
    switch (rotation_type(p))
      {
      case ROTATE_LEFT:         return *link = rotateLeft(p);
      case ROTATE_RIGHT:        return *link = rotateRight(p);
      case DOUBLE_ROTATE_LEFT:  return *link = doubleRotateLeft(p);
      case DOUBLE_ROTATE_RIGHT: return *link = doubleRotateRight(p);

      default:
        AH_ERROR("Invalid rotation type");
        break;
      }

    return nullptr;
  }

  void restore_avl_after_insertion(Node * p) noexcept
  {
    Node * pp  = avl_stack.pop(); // padre del nodo insertado
    if (LLINK(pp) == p) // ajuste el factor del padre del nodo insertado OJO
      DIFF(pp)--;
    else
      DIFF(pp)++;

    if (DIFF(pp) == 0)
      {    // en este caso, altura del ascendiente de pp no aumenta
        clean_avl_stack();
        return;
      }

    if (avl_stack_empty())
      return; // pp es raíz

    Node *gpp; // padre de pp
    do     // buscar nodo con factor igual a 0
      {
        gpp = avl_stack.pop();
            // actualizar factores de equilibrio
        if (LLINK(gpp) == pp) // AH_ERROR if (Compare () (key, KEY(gpp)))
          DIFF(gpp)--;
        else
          DIFF(gpp)++;

        if (DIFF(gpp) == 0)
          break; // no se necesita reajuste
        else if (DIFF(gpp) == -2 or DIFF(gpp) == 2)// ¿es AVL?
          {      // sí ==> se requiere reajuste
            Node *ggpp = avl_stack.pop();
            restore_avl(gpp, ggpp);
            break;
          }

        pp = gpp; // AH_ERROR; añadir
      }
    while (not avl_stack_empty());

    clean_avl_stack();
  }

  Node * swapWithSuccessor(Node * p, Node *& pp) noexcept
  {   // Referencia al tope de la pila, pues p será intercambiado con
      // sucesor y la posición en la pila la ocupará el sucesor de p
    Node *& ref_to_stack_top = avl_stack.top();

    Node *fSucc = p;        // padre del sucesor
    Node *succ  = RLINK(p); // búsqueda comienza desde RLINK(p)

    avl_stack.push(succ);

        // encuentre el sucesor a la vez que actualiza la pila
    while (LLINK(succ) != Node::NullPtr) // descender lo más a la izq
      {
        fSucc = succ;
        succ  = LLINK(succ);
        avl_stack.push(succ);
      }

      // actualice antigua entrada de pila ocupada por p. Equivale
      // a intercambiar antiguo tope (antes de buscar suc) con actual
    ref_to_stack_top = succ;
    avl_stack.top()  = p;
    if (LLINK(pp) == p) // actualice el nuevo hijo de pp (sucesor)
      LLINK(pp) = succ;
    else
      RLINK(pp) = succ;

    LLINK(succ) = LLINK(p); // intercambie las ramas izquierdas
    LLINK(p)    = Node::NullPtr;
    if (RLINK(p) == succ) // actualice ramas derechas
      { // sucesor es exactamente el hijo derecho de p
        RLINK(p)    = RLINK(succ);
        RLINK(succ) = p;
        pp          = succ;
      }
    else
      { // sucesor es el descendiente más a la izquierda de RLINK(p)
        Node *succr  = RLINK(succ);
        RLINK(succ)  = RLINK(p);
        LLINK(fSucc) = p;
        RLINK(p)     = succr;
        pp           = fSucc;
      }

    DIFF(succ) = DIFF(p); // intercambie factores de equilibrio

    return succ;
  }

  void restore_avl_after_deletion(bool left_deficit) noexcept
  {
    Node * pp  = avl_stack.top(1);  // padre de p
    Node * ppp = avl_stack.popn(3); // elimina de pila p, padre y abuelo
    while (true)
      {    // actualice factores de equilibrio
        if (left_deficit) // AH_ERROR Compare () (key, KEY(pp)))
          DIFF(pp)++;
        else
          DIFF(pp)--;

        if (DIFF(pp) == -2 or DIFF(pp) == 2) // ¿es válido?
          pp = restore_avl(pp, ppp); // no!

        if (DIFF(pp) != 0 or pp == root)
          break; // altura global de árbol no ha cambiado ==> terminar

        left_deficit = LLINK(ppp) == pp;
        pp  = ppp; // avance al próximo ascendiente
        ppp = avl_stack.pop();
      }

      clean_avl_stack();
    }

public:

  using key_type = Key; /// The key type

      /// Return a reference to teh comparison criteria
  Compare & key_comp() noexcept { return cmp; }

      /// \overload key_comp()
  Compare & get_compare() noexcept { return key_comp(); }

  Gen_Avl_Tree(Compare __cmp = Compare()) noexcept
    : avl_stack(Node::MaxHeight), head_ptr(&head_node),
      root(RLINK (head_ptr)), cmp(__cmp)
  {
    avl_stack.push(head_ptr);
  }

  /** Swap in constant time all the items of `this` with the items of
      `tree`.

      @param[in] tree el treap a intercambiar con this
  */
  void swap(Gen_Avl_Tree & tree) noexcept
  {
    std::swap(root, tree.root);
    std::swap(cmp, tree.cmp);
  }

  virtual ~Gen_Avl_Tree() noexcept { assert(avl_stack_empty()); }

      /// Return a modifiable reference to tree's root
  Node *& getRoot() noexcept { return root; }

     /// Search a node containing `key`; if found, then a pointer to the
     /// node containing it is returned; otherwise `nullptr` is returned
  Node * search(const Key & key) const noexcept
  {
    return searchInBinTree <Node, Compare> (root, key, cmp);
  }

  /** Insert the node pointed by `p` in the tree.

      `insert(p)' inserts the node `p` inte AVL tree. If `KEY(p)` is
      found in the tree, then no insertion is done and `nullptr` is
      returned. Otherwise, `p` is inserted and this same value is
      returned.

      @param[in] p the node to be inserted
      @return a the pointer `p` if `KEY(p)` is not found in the tree,
      `nullptr` otherwise.
  */
  Node * insert(Node * p) noexcept
  {
    if (root == Node::NullPtr)
      return root = p;

    Node *pp = search_and_stack_avl(KEY(p));
    if (cmp (KEY(p), KEY(pp)))
      LLINK (pp) = p;
    else if (cmp (KEY(pp), KEY(p)))
      RLINK(pp) = p;
    else
      { // clave duplicada
        clean_avl_stack();
        return nullptr;
      }

    restore_avl_after_insertion(p);

    return p;
  }

      /** Search or insert a key.

          `search_or_insert(p)` searches in the tree a node containing
          `KEY(p)`. If this is found, then a pointer to the node in the
          tree is returned. Otherwise, `p` is inserted and its value
          returned,

          This method is very useful when it is required to search and
          eventually to insert,

          @param[in] p the node whose key must be searched and
          eventually inserted,
          @return if `p` is inserted, the this value is returned;
          otherwise, a pointer to the node containing `KEY(p)` is returned.

       */
  Node * search_or_insert(Node * p) noexcept
  {
    if (root == Node::NullPtr)
      return root = p;

    Node * pp = search_and_stack_avl(KEY(p));
    if (cmp(KEY(p), KEY(pp)))
      LLINK(pp) = p;
    else if (cmp(KEY(pp), KEY(p)))
      RLINK(pp) = p;
    else
      { // clave duplicada
        clean_avl_stack();
        return pp;
      }

    restore_avl_after_insertion(p);

    return p;
  }

      /// Insert the node `p` without testing for key duplicity
  Node * insert_dup(Node * p) noexcept
  {
    if (root == Node::NullPtr)
      return root = p;

    Node *pp = search_dup_and_stack_avl(KEY(p));
    if (cmp (KEY(p), KEY(pp)))
      LLINK (pp) = p;
    else
      RLINK(pp) = p;

    restore_avl_after_insertion(p);

    return p;
  }

      /// Elimina de un árbol AVL el nodo que contiene la clave
      /// key. Retorna el nodo eliminado si se encuentra la clave; nullptr de
      /// lo contrario.
  Node * remove(const Key & key) noexcept
  {
    if (root == Node::NullPtr)
      return nullptr;

    Node * p = search_and_stack_avl(key);
    if (no_equals<Key, Compare> (KEY(p), key, cmp))
      {     // clave no fue encontrada
        clean_avl_stack();
        return nullptr;
      }

    Node * pp = avl_stack.top(1); // obtener padre de p
    bool left_deficit; // AH_ERROR Key removed_key = KEY(p);
    while (true)
      {
        left_deficit = LLINK(pp) == p;
        if (LLINK(p) == Node::NullPtr) // ¿incompleto por la izquierda?
          {      // Sí, ate a pp el hijo de p
            if (LLINK(pp) == p)
              LLINK(pp) = RLINK(p);
            else
              RLINK(pp) = RLINK(p);
            break;
          }

        if (RLINK(p) == Node::NullPtr) // ¿incompleto por la izquierda?
          {     // Sí, ate a pp el hijo de p
            if (LLINK(pp) == p)
              LLINK(pp) = LLINK(p);
            else
              RLINK(pp) = LLINK(p);
            break;
          }

            // aquí p es un nodo completo ==> intercambiar por sucesor 
        swapWithSuccessor(p, pp);
        //    removed_key = KEY(succ); // AH_ERROR eliminar
      }

     p->reset();

    if (pp == head_ptr) // verifique si se eliminó la raíz
      { // factores quedan inalterados ==> no se viola condición AVL   
        clean_avl_stack();
        return p;
      }

    restore_avl_after_deletion(left_deficit); // ERROR

    return p;
  }

  bool verify() const
  {
    return is_avl(root);
  }

  /** Iterador sobre los nodos.

      Esta clase permite recorrer los nodos del árbol en forma
      ordenada según el criterio de comparación especificado en el
      momento de instanciación del árbol.

     @ingroup Arboles
  */
  struct Iterator : public BinNodeInfixIterator<Node>
  {
    Iterator(Gen_Avl_Tree & t) : BinNodeInfixIterator<Node>(t.getRoot()) {}
  };
};


   /** Árbol binario de búsqueda AVL con nodos sin destructor virtual.

       Un árbol binario de búsqueda AVL es un árbol binario de búsqueda
       cuya altura está acotada a \f$O(\lg n)\f$ y sus operaciones de
       modificación acotadas en tiempo por \f$O(\lg n)\f$ nodos
       inspeccionados. 

       Esta clase maneja nodos sin destructor virtual.

       @param Key el tipo de clave que albergan los nodos del árbol.
       @param Compare clase de comparación entre las claves; por
       omisión, esta clase es el operador relacional menor que para el
       tipo Key.
       @see Avl_Tree_Vtl
       @ingroup Arboles
    */
    template <typename Key, class Compare = Aleph::less<Key>>
struct Avl_Tree : public Gen_Avl_Tree<AvlNode, Key, Compare>
{
  using Base = Gen_Avl_Tree<AvlNode, Key, Compare>;
  using Base::Base;
};


   /** Árbol binario de búsqueda AVL con destructor virtual en sus nodos.

       Un árbol binario de búsqueda AVL es un árbol binario de búsqueda
       cuya altura está acotada a \f$O(\lg n)\f$ y sus operaciones de
       modificación acotadas en tiempo por \f$O(\lg n)\f$ nodos
       inspeccionados. 

       Esta clase maneja nodos con destructor virtual.

       @param Key el tipo de clave que albergan los nodos del árbol.
       @param Compare clase de comparación entre las claves; por
       omisión, esta clase es el operador relacional menor que para el
       tipo Key.
       @see Avl_Tree
       @ingroup Arboles
    */
    template <typename Key, class Compare = Aleph::less<Key>>
struct Avl_Tree_Vtl : public Gen_Avl_Tree<AvlNodeVtl, Key, Compare>
{
  using Base = Gen_Avl_Tree<AvlNodeVtl, Key, Compare>;
  using Base::Base;
};


} // end namespace Aleph
# endif // TPL_AVL_H

