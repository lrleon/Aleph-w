
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

# include <generate_graph.H>
# include <tpl_netcost.H>

# include <autosprintf.h>

typedef Net_Cost_Node<string> Nodo;

typedef Net_Cost_Arc<Empty_Class> Tubo;


typedef Net_Cost_Graph<Nodo, Tubo> Red;


void crear_tubo(Red & red, const string & src_name, const string & tgt_name, 
		const Red::Flow_Type & cap, const Red::Flow_Type & cost = 0)
{
  Red::Node * src = red.find_node(src_name);
  if (src == NULL)
    src = red.insert_node(src_name);
 
  Red::Node * tgt = red.find_node(tgt_name);
  if (tgt == NULL)
    tgt = red.insert_node(tgt_name);

  red.insert_arc(src, tgt, cap, cost);
}


Red crear_red()
{
  Red red;
  crear_tubo(red, "S", "A", 70, 20); 
  //  crear_tubo(red, "S", "B", 70, 120);
  crear_tubo(red, "S", "C", 50, 50); 

  //  crear_tubo(red, "A", "E", 20, 20);
  crear_tubo(red, "A", "F", 60, 10);
  crear_tubo(red, "A", "B", 30, 20);
  crear_tubo(red, "C", "B", 30, 5);
  crear_tubo(red, "B", "E", 40, 20);
  crear_tubo(red, "B", "D", 10, 1);
  crear_tubo(red, "C", "D", 50, 40);
  crear_tubo(red, "C", "E", 30, 10);
  crear_tubo(red, "E", "G", 20, 8);
  crear_tubo(red, "E", "H", 50, 30);
  crear_tubo(red, "E", "I", 20, 30);
  crear_tubo(red, "D", "I", 30, 15);
  crear_tubo(red, "D", "E", 80, 30);
  crear_tubo(red, "I", "F", 30, 5);
  crear_tubo(red, "I", "G", 40, 90);
  crear_tubo(red, "I", "H", 50, 20);
  crear_tubo(red, "K", "J", 30, 2);
  crear_tubo(red, "F", "J", 60, 50);
  crear_tubo(red, "D", "H", 40, 35);
  crear_tubo(red, "H", "G", 15, 4);
  crear_tubo(red, "H", "K", 50, 25);
  //  crear_tubo(red, "I", "J", 30, 25);
  crear_tubo(red, "J", "T", 70, 20);
  crear_tubo(red, "G", "K", 50, 30);
  crear_tubo(red, "K", "T", 60, 15);
  return red;
}


int main()
{
  Red red1 = crear_red();

  max_flow_min_cost_by_cycle_canceling<Red, Edmonds_Karp_Maximum_Flow>(red1);

  // Residual_Net<Red::Flow_Type> res_net = build_residual_net(red1);

  // print(res_net, cout);

  Red red2 = red1;
  Red red3 = red1;
  Red red4 = red1;
  Red red5 = red1;


}
