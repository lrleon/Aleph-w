

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

  void update()
  {
    if (aux_net == nullptr)
      throw std::domain_error("Auxiliar net has not been generated");

    for (Arc_Iterator<Aux_Net, No_Res_Arc<Aux_Net> > it(*aux_net); 
         it.has_curr(); it.next_ne())
      {
        typename Aux_Net::Arc * arc = it.get_curr_ne();
        if (arc->get_info())
          {
            Node * p = (Node*) ARC_COOKIE(arc);
            p->in_flow = p->out_flow = arc->flow;
          }
        else
          {
            Arc * a = (Arc*) ARC_COOKIE(arc);
            a->flow = arc->flow;
          }
      }
  }
      /// Libera la memoria de la red equivalente. Dispara excepción
      /// domain_error si la red equivalente no ha sido creada.
  void free_aux_net()
  {
    if (aux_net == nullptr)
      throw std::domain_error("Auxiliar net has not been generated");

    clear_graph(*aux_net);
    delete aux_net;
    aux_net = nullptr;
  }

  ~Net_Cap_Graph()
  {
    if (aux_net != nullptr)
      free_aux_net();
  }
};

} // end namespace Aleph

# endif // endif
