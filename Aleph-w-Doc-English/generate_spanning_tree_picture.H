
/*
  This file is part of Aleph-w system

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014
  Leandro Rabindranath León
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met: 

  1. Redistributions of source code must retain the above copyright 
     notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright 
     notice, this list of conditions and the following disclaimer in the 
     documentation and/or other materials provided with the distribution.

  3. All advertising materials mentioning features or use of this software
     must display the following acknowledgement:

     Copyright (c) 2002-2014 Leandro Rabindranath León. See details of 
     licence.     

     This product includes software developed by the Hewlett-Packard
     Company, Free Software Foundation and Silicon Graphics Computer
     Systems, Inc. 

  4. Neither the name of the ULA nor the names of its contributors may
     be used to endorse or promote products derived from this software
     without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY Leandro Rabindranath León ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Aleph-w is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.

  I request users of this software to return to 

  Leandro Rabindranath Leon
  CEMISID 
  Ed La Hechicera 
  3er piso, ala sur
  Facultad de Ingenieria 
  Universidad de Los Andes 
  Merida - REPÚBLICA BOLIVARIANA DE VENEZUELA    or

  leandro.r.leon@gmail.com

  any improvements or extensions that they make and grant me the rights
  to redistribute these changes.  
*/

/**
  @file generate_spanning_tree_picture.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# include <generate_graph.H>



    template <typename GT> 
struct Shade_Span_Node
{
  string operator () (typename GT::Node * p) const
  {
    return NODE_COOKIE(p) == NULL ? "" : "SHADOW-NODE";
  }
};


    template <typename GT> 
struct Shade_Span_Arc
{
  string operator () (typename GT::Arc * a) const
  {
    return ARC_COOKIE(a) == NULL ? "ARC" : "SHADOW-ARC"; 
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
