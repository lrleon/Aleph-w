

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

void set_supply_flow(Node * p, const Flow_Type & supply)
{
  if (supply > 0 and p->out_cap < supply)
    throw std::range_error(gnu::autosprintf("Supply flow in node at %p is "
					    "greater than out capacity", p));
  else if (supply < 0 and p->in_cap < -supply)
    throw std::range_error(gnu::autosprintf("Supply flow in node at %p is "
					    "smaller than in capacity", p));
  p->supply_flow = supply;
}
#line 922 "reducciones.nw"
    /// Libera memoria de la red extendida.
void free_aux_net()
{
  if (not exist_aux_net())
    throw std::domain_error("Auxiliar net has not been computed");

  if (this->residual_net)
    {
      this->unmake_residual_net();
      this->unmake_super_nodes();
    } 
  if (super_source != nullptr)
    {
      remove_node(super_source);
      super_source = nullptr;
    }
  if (super_sink != nullptr)
    {
      remove_node(super_sink);
      super_sink = nullptr;
    }
}
#line 664 "reducciones.nw"
};

} // end namespace Aleph

# endif // TPL_NET_SUP_DEM_H
