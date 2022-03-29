

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


# include <random_net.H>

typedef Net_Arc<Empty_Class> Arc;
typedef Net_Node<Empty_Class> Node;

typedef Net_Graph<> Net;

int main(int argn, char * argc[])
{
  size_t num_nodes = argn > 1 ? atoi(argc[1]) : 1000;
  size_t num_ranks = argn > 2 ? atoi(argc[2]) : 100;
  unsigned int seed = argn > 3 ? atoi(argc[3]) : 0;

  cout << argc[0] << " " << num_nodes << " " << num_ranks << " " 
       << seed << endl;

  Net * net_ptr = Random_Network_Flow<Net> (seed) (num_ranks, num_nodes);

  delete net_ptr;
}
