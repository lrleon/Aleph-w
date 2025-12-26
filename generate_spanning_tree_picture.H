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



    template <typename GT> 
struct Shade_Span_Node
{
  string operator () (typename GT::Node * p) const
  {
    return NODE_COOKIE(p) == nullptr ? "" : "SHADOW-NODE";
  }
};


    template <typename GT> 
struct Shade_Span_Arc
{
  string operator () (typename GT::Arc * a) const
  {
    return ARC_COOKIE(a) == nullptr ? "ARC" : "SHADOW-ARC"; 
  }
};


    template <typename GT, class Write_Node, class Write_Arc, class SA>
void generate_cross_spanning_tree(GT &            g, 
				  const size_t &  nodes_by_level, 
				  const double &  xdist,
				  const double &  ydist,
				  std::ofstream & output)
{
  generate_cross_graph
    <GT, Write_Node, Write_Arc, Shade_Span_Node<GT>, Shade_Span_Arc<GT>, SA>
      (g, nodes_by_level, xdist, ydist, output);
}


    template <typename GT, class Write_Node, class Write_Arc>
void generate_cross_spanning_tree(GT &            g, 
				  const size_t &  nodes_by_level, 
				  const double &  xdist,
				  const double &  ydist,
				  std::ofstream & output)
{
  typedef Dft_Show_Arc<GT> DSA;
  generate_cross_spanning_tree<GT, Write_Node, Write_Arc, DSA>
    (g, nodes_by_level, xdist, ydist, output);
}

    template <typename GT, class Write_Node, class Write_Arc, class SA>
void generate_net_spanning_tree(GT &            g, 
				const size_t &  nodes_by_level, 
				const double &  xdist,
				const double &  ydist,
				std::ofstream & output)
{
  generate_net_graph
    <GT, SA, Write_Node, Write_Arc, Shade_Span_Node<GT>, Shade_Span_Arc<GT> >
      (g, nodes_by_level, xdist, ydist, output);
}


    template <typename GT, class Write_Node, class Write_Arc>
void generate_net_spanning_tree(GT &            g, 
				const size_t &  nodes_by_level, 
				const double &  xdist,
				const double &  ydist,
				std::ofstream & output)
{
  typedef Dft_Show_Arc<GT> DSA;
  generate_net_spanning_tree<GT, Write_Node, Write_Arc, DSA>
    (g, nodes_by_level, xdist, ydist, output);
}

