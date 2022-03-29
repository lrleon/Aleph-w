

/*
                          Aleph_w

  Data structures & Algorithms
  version 1.9d
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2022 Leandro Rabindranath Leon

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

    template <template <class> class RankList = DynList, 
              template <class> class List = DynList>
    RankList<List<typename GT::Node*>> ranks(const GT & g)
    {
      g.reset_counter_nodes();

      // recorra todos los nodos para contabilizar grado de entrada
      for (typename GT::Node_Iterator i(g); i.has_curr(); i.next_ne()) 
        for (Itor<GT, SA> j(i.get_current_node_ne(), sa);
             j.has_curr(); j.next_ne())
          NODE_COUNTER(j.get_tgt_node())++;

      // revisar nodos con grado de entrada 0 y meterlos en cola
      DynListQueue<typename GT::Node*> q; // cola de fuentes
      for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne())
        {
          auto p = it.get_current_node_ne();
          if (NODE_COUNTER(p) == 0) // ¿es un nodo fuente?
            q.put(p); // sí ==> colocarlo en la cola
        }
    
      RankList<List<typename GT::Node*>> ranks; // valor de retorno
      while (not q.is_empty()) 
        {
          List<typename GT::Node*> rank;
          DynListQueue<typename GT::Node*> aq;

          while (not q.is_empty()) // saca todos los nodos del nivel i
            {
              auto p = q.get(); // saque último fuente
              rank.append(p); // insértelo en el rango topológico

              // decrementar grado entrada de cada nodo adyacente a p
              for (Itor<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
                {
                  auto tgt = it.get_tgt_node_ne();
                  if (--NODE_COUNTER(tgt) == 0) // ¿tgt deviene fuente?
                    aq.put(tgt); // sí ==> colóquelo en cola auxiliar
                }
            }

          ranks.append(std::move(rank));
          q.swap(aq);
          assert(aq.is_empty());
        }

      return ranks; 
    }

    void operator () (const GT & g, DynDlist<DynList<typename GT::Node *>> & list)
    {
      this->ranks<>(g).swap(list);
    }

    void operator () (const GT & g, DynList<DynList<typename GT::Node *>> & list)
    {
      this->ranks<DynList>(g).swap(list);
    }

    /// Sobrecarga de () por razones compatibilidad
    void operator () (const GT & g, DynDlist<typename GT::Node*> & list) 
    {
      this->perform<DynDlist>(g).swap(list);
    }
  };

} // end namespace Aleph

# endif // TOPOLOGICAL_SORT_H
