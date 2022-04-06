
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

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
# ifndef TPL_DYNMAPTREE_H
# define TPL_DYNMAPTREE_H

# include <tpl_dynSetTree.H>

using namespace Aleph;

namespace Aleph {

  /** Mapeo genérico instrumentado con árboles binarios de búsqueda.

      DynMapTree define un mapeo entre un conjunto de claves, que
      no se pueden repetir, y un conjunto rango.  El mapeo se implanta
      mediante alguna clase de árbol binario de búsqueda definido como
      parámetros tipo.

      Los parámetros tipo de esta clase se definen del siguiente modo:
      -# Tree: la clase de árbol binario de búsqueda con que se desea
      instrumentar el mapeo. Los posibles tipos son BinTree Avl_Tree
      Splay_Tree Rb_Tree Treap y Rand_Tree.
      -# Key: tipo de clave de indización. También se le dice "dominio"
      del mapeo.
      -# Data: tipo del rango.
      -# Compare: criterio de comparación entre las claves.

      Los elementos del mapeo pueden accederse mediante el operador []
      colocando como parámetro la clave de indización.

      @see BinTree Avl_Tree Splay_Tree Rb_Tree Treap Rand_Tree
      @ingroup Arboles
  */
  template <
    typename Key, typename Data,
    template <typename, class> class Tree = Avl_Tree,
    class Compare                         = Aleph::less<Key>>
  class DynMapTree :
    public DynSetTree<std::pair<Key, Data>, Tree,
                      Dft_Pair_Cmp<Key, Data, Compare>>
  {
    using Pair = std::pair<Key, Data>;

    using Base =
      DynSetTree<std::pair<Key, Data>, Tree, Dft_Pair_Cmp<Key, Data, Compare>>;

  public:

    using Base::Base;

    DynMapTree(const DynList<Key> & keys)
    {
      keys.for_each([this] (auto & k) { this->insert(make_pair(k, Data())); });
    }

    DynMapTree() {}

    using Key_Type = Key;

    using Item_Type = Key;

    using Value_Type = Data ;

    // using Base::Base;  // no more need. But I don't remember why I put it
    using Base::insert;

    static Data & get_data(const Key & key) noexcept
    {
      return key_to_pair<Key, Data>(&const_cast<Key&>(key))->second;
    }

    static const Key & get_key(Data * data_ptr) noexcept
    {
      return data_to_pair<Key, Data>(data_ptr)->first;
    }

    /** Inserta un par en el mapeo.

        insert(key,data) inserta en el mapeo un nuevo par (key,data)
        indizado por el valor de key. La operación no realiza inserción si
        el valor de key ya está presente en el mapeo.

        @param[in] key clave a insertar.
        @param[in] data valor a indizar por la clave key.
        @return puntero al dato mapeado.
        @throw bad_alloc si no hay suficiente memoria.
    */
    Pair * insert(const Key & key, const Data & data)
    {
      return this->Base::insert(Pair(key, data));
    }

    Pair * insert(const Key & key, Data && data = Data())
    {
      return this->Base::insert(Pair(key, std::forward<Data>(data)));
    }

    Pair * insert(Key && key, const Data & data)
    {
      return this->Base::insert(Pair(std::forward<Key>(key), data));
    }

    Pair * insert(Key && key, Data && data = Data())
    {
      return this->Base::insert(Pair(std::forward<Key>(key),
                                     std::forward<Data>(data)));
    }

    Pair * append(const Key & key, const Data & data)
    {
      return this->Base::insert(Pair(key, data));
    }

    Pair * append(const Key & key, Data && data = Data())
    {
      return this->Base::insert(Pair(key, std::forward<Data>(data)));
    }

    Pair * append(Key && key, const Data & data)
    {
      return this->Base::insert(Pair(std::forward<Key>(key), data));
    }

    Pair * append(Key && key, Data && data = Data())
    {
      return this->Base::insert(Pair(std::forward<Key>(key),
                                     std::forward<Data>(data)));
    }

    /** Inserta un par en el mapeo.

        put(key,data) inserta en el mapeo un nuevo par (key,data)
        indizado por el valor de key. La operación no realiza inserción si
        el valor de key ya está presente en el mapeo.

        @param[in] key clave a insertar.
        @param[in] data valor a indizar por la clave key.
        @return número de elementos que contiene el mapeo.
        @throw bad_alloc si no hay suficiente memoria.
    */
    Pair * put(const Key & key, const Data & data)
    {
      return insert(key, data);
    }

    Pair * put(const Key & key, Data && data)
    {
      return insert(key, std::forward<Data>(data));
    }

    Pair * put(Key && key, const Data & data)
    {
      return insert(std::forward<Key>(key), data);
    }

    Pair * put(Key && key, Data && data)
    {
      return insert(std::forward<Key>(key), std::forward<Data>(data));
    }

    /** Deletes the pair `key,data`

        `remove(key)` deletes from the mapping the pair associated to `key`

        @param[in] key to delete
        @return the data associated to the removed key.
        @throw domain_error if key is not in the mapping
    */
    Data remove(const Key & key)
    {
      Pair p(key, Data());
      return this->del(p).second;
    }

    Data remove(Key && key)
    {
      return this->del(forward<Key>(key), Data()).second;
    }

    void remove_key(const Key & key)
    {
      Base::remove(Pair(key, Data()));
    }

    /** Retorna dato asociado a una clave.

        search(key) retorna un puntero al valor del dato asociado a la
        clave key.

        @param[in] key clave a buscar.
        @return puntero al dato asociado a key si la clave existe en
        mapeo; nullptr de lo contrario.
        @see test
    */
    Pair * search(const Key & key) const noexcept
    {
      return this->Base::search(Pair(key, Data()));
    }

    Pair * search(Key && key) const noexcept
    {
      return this->Base::search(Pair(move(key), Data()));
    }

    bool has(const Key & key) const noexcept { return search(key) != nullptr; }

    bool has(Key && key) const noexcept { return search(move(key)) != nullptr; }

    bool contains(const Key & key) const noexcept { return has(key); }

    bool contains(Key && key) const noexcept { return has(move(key)); }

    /** Retorna dato asociado a una clave.

        search(key) retorna un puntero al valor del dato asociado a la
        clave key.

        @param[in] key clave a buscar.
        @return puntero al dato asociado a key si la clave existe en
        mapeo; nullptr de lo contrario.
        @throw domain_error si la clave no está dentro del mapeo
    */
    Data & find(const Key & key)
    {
      return this->Base::find(Pair(key, Data())).second;
    }

    const Data & find(const Key & key) const
    {
      return this->Base::find(Pair(key, Data())).second;
    }

    Data & operator [] (const Key & key)
    {
      return this->search_or_insert(Pair(key, Data()))->second;
    }

    const Data & operator [] (const Key & key) const
    {
      return this->find(key);
    }

    Data & operator [] (Key && key)
    {
      return this->search_or_insert(Pair(move(key), Data()))->second;
    }

    const Data & operator [] (Key && key) const
    {
      return this->find(move(key));
    }

    using Iterator = typename Base::Iterator;

    DynList<Key> keys() const
    {
      return this->template maps<Key>([] (auto p) { return p.first; });
    }

    DynList<Data> values() const
    {
      return this->template maps<Data>([] (auto p) { return p.second; });
    }

    DynList<Data*> values_ptr()
    {
      DynList<Data*> ret;
      for (Iterator it(*this); it.has_curr(); it.next_ne())
        ret.append(&it.get_curr_ne().second);
      return ret;
    }

    DynList<Pair*> items_ptr()
    {
      DynList<Pair*> ret;
      for (Iterator it(*this); it.has_curr(); it.next_ne())
        ret.append(&it.get_curr_ne());
      return ret;
    }
  };
} // end namespace Aleph


# include <tpl_binTree.H>
# include <tpl_avl.H>
# include <tpl_rb_tree.H>
# include <tpl_rand_tree.H>
# include <tpl_treap.H>
# include <tpl_treapRk.H>
# include <tpl_splay_tree.H>

namespace Aleph {
  /**
     Mapeo dinámico implementado con árboles binario de búsqueda clásicos.

     @see BinTree DynMapTree
     @ingroup Arboles
  */
  template <typename Key, typename Type, class Compare = Aleph::less<Key> >
  class DynMapBinTree : public DynMapTree<Key, Type, BinTree, Compare> {};

  /**
     Mapeo dinámico implementado con árboles binario de búsqueda AVL.

     @see Avl_Tree DynMapTree
     @ingroup Arboles
  */
  template <typename Key, typename Type, class Compare = Aleph::less<Key> >
  class DynMapAvlTree : public DynMapTree<Key, Type, Avl_Tree, Compare> {};
  /**
     Mapeo dinámico implementado con árboles binario de búsqueda rojo-negro.

     @see Rb_Tree DynMapTree
     @ingroup Arboles
  */
  template <typename Key, typename Type, class Compare = Aleph::less<Key> >
  class DynMapRbTree : public DynMapTree<Key, Type, Rb_Tree, Compare>
  { /* empty */ };

  /**
     Mapeo dinámico implementado con árboles binario de búsqueda
     aleatorizados.

     @see Rand_Tree DynMapTree
     @ingroup Arboles
  */
  template <typename Key, typename Type, class Compare = Aleph::less<Key> >
  class DynMapRandTree : public DynMapTree<Key, Type, Rand_Tree, Compare>
  { /* empty */ };

  /**
     Mapeo dinámico implementado con árboles binario de búsqueda treaps.

     @see Treap DynMapTree
     @ingroup Arboles
  */
  template <typename Key, typename Type, class Compare = Aleph::less<Key> >
  class DynMapTreap : public DynMapTree<Key, Type, Treap, Compare>
  { /* empty */ };

  /**
     Mapeo dinámico implementado con árboles binario de búsqueda
     treaps y con rango. Ofrece la operación position.

     @see Treap_Rk DynMapTree
     @ingroup Arboles
  */
  template <typename Key, typename Type, class Compare = Aleph::less<Key> >
  class DynMapTreapRk : public DynMapTree<Key, Type, Treap_Rk, Compare>
  { /* empty */ };

  /**
     Mapeo dinámico implementado con árboles binario de búsqueda splay.

     @see Splay_Tree DynMapTree
     @ingroup Arboles
  */
  template <typename Key, typename Type, class Compare = Aleph::less<Key> >
  class DynMapSplayTree : public DynMapTree<Key, Type, Splay_Tree, Compare>
  { /* empty */ };

  template <typename T, class Op, class C>
  DynMapTree<typename C::Item_Type, T> map_unify(const C & c, Op op)
  {
    DynMapTree<typename C::Item_Type, T> ret;
    for (auto it = c.get_it(); it.has_curr(); it.next_ne())
      {
        const auto & curr = it.get_curr_ne();
        ret.insert(curr, op(curr));
      }
    return ret;
  }

} // end namespace Aleph

# endif /* TPL_DYNMAPTREE_H */

