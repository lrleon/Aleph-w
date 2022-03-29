

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

  template <class GT, 
            class Write_Node, class Write_Arc,
            class Shade_Node, class Shade_Arc, class SA>
  void generate_net_graph(GT &            g, 
                          const size_t &  nodes_by_level, 
                          const double &  xdist,
                          const double &  ydist,
                          std::ostream & out)
  {
    if (g.is_digraph())
      out << "net-digraph ";  
    else 
      out << "net-graph "; 

    out << g.get_num_nodes() << " " << nodes_by_level << " " 
        << xdist << " " << ydist << endl 
        << endl;
  
    generate_graphpic<GT, Write_Node, Write_Arc, Shade_Node, Shade_Arc, SA>
      (g, xdist, ydist, out);
  }

  template <class GT, 
            class Write_Node, class Write_Arc,
            class Shade_Node, class Shade_Arc>
  void generate_net_graph(GT &            g, 
                          const size_t &  nodes_by_level, 
                          const double &  xdist,
                          const double &  ydist,
                          std::ostream & out)
  {
    typedef Dft_Show_Arc<GT> DSA;
    generate_net_graph<GT, Write_Node, Write_Arc, Shade_Node, Shade_Arc, DSA>
      (g, nodes_by_level, xdist, ydist, out);
    
  }

  template <class GT> struct __Shade_Node
  {
    string operator () (typename GT::Node *) const
    {
      return "";
    }
  };


  template <class GT> struct __Shade_Arc
  {
    string operator () (typename GT::Arc *) const
    {
      return "";
    }
  };


  template <class GT, class Write_Node, class Write_Arc, class SA>
  void generate_cross_graph(GT &            g, 
                            const size_t &  nodes_by_level, 
                            const double &  xdist,
                            const double &  ydist,
                            std::ostream & out)
  {
    generate_cross_graph
      <GT, Write_Node, Write_Arc, __Shade_Node<GT>, __Shade_Arc<GT>, SA>
      (g, nodes_by_level, xdist, ydist, out);
  }

  template <class GT, class Write_Node, class Write_Arc, class SA>
  void generate_net_graph(GT &            g, 
                          const size_t &  nodes_by_level, 
                          const double &  xdist,
                          const double &  ydist,
                          std::ostream & out)
  {
    generate_net_graph
      <GT, Write_Node, Write_Arc, __Shade_Node<GT>, __Shade_Arc<GT>, SA>
      (g, nodes_by_level, xdist, ydist, out);
  }


  template <class GT, class Write_Node, class Write_Arc>
  void generate_cross_graph(GT &            g, 
                            const size_t &  nodes_by_level, 
                            const double &  xdist,
                            const double &  ydist,
                            std::ostream & out)
  {
    typedef Dft_Show_Arc<GT> DSA;
    generate_cross_graph
      <GT, Write_Node, Write_Arc, __Shade_Node<GT>, __Shade_Arc<GT>, DSA>
      (g, nodes_by_level, xdist, ydist, out);
  }

  template <class GT, class Write_Node, class Write_Arc>
  void generate_net_graph(GT &            g, 
                          const size_t &  nodes_by_level, 
                          const double &  xdist,
                          const double &  ydist,
                          std::ostream & out)
  {
    typedef Dft_Show_Arc<GT> DSA;
    generate_net_graph 
      <GT, Write_Node, Write_Arc, __Shade_Node<GT>, __Shade_Arc<GT>, DSA>
      (g, nodes_by_level, xdist, ydist, out);
  }


} // end namespace Aleph    


# endif // GENERATE_GRAPH_H
