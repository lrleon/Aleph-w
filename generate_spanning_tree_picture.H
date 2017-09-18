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

