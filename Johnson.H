# ifndef JOHNSON_H
# define JOHNSON_H

# include <Dijkstra.H>
# include <Bellman_Ford.H>


 template <class GT, 
	      class Distance = Dft_Dist<GT>, 
	      class SA       = Dft_Show_Arc<GT>> 
class Jhonson
{
  

  void 
  compute_ratio_and_diameter(Distance_Type & ratio, Node * rsrc, Node * rtgt,
			     Distance_Type & diameter, Node * dsrc, Node * dtgt)
  {
    
  }
 

public:

};


# endif // JOHNSON_H
