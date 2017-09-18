# ifndef SEGMENT_H
# define SEGMENT_H

# include "point.H"
//# include "points_utils.H"

class Segment
{

  const Point * a;
  const Point * b;

public:

  Segment() : a(nullptr), b(nullptr)
  { 
    /* empty */  
  }
  
  Segment(const Point * __a, const Point * __b) : a(__a), b(__b) 
  { 
    /* empty */ 
  }

  Segment(const Point & __a, const Point & __b) : a(&__a), b(&__b) 
  { 
    /* empty */ 
  }
  
  ~Segment()
  {
    /* empty */
  }
  
  const bool operator == (const Segment & segment) const 
  {
    return ((*a == *segment.a and *b == *segment.b) or
	    (*a == *segment.b and *b == *segment.a));
  }
  
  /* Verifica si el segmento intersecta otro segmento  */
  const bool intersect(const Segment & segment) const
  {
    return intersectp(*a, *b, *segment.a, *segment.b);
  }

  const Point * get_a() { return a ;} //Retorna el punto 'a' del segmento

  const Point * get_b() { return b; } //Retorna el punto 'b' del segmento


  /* Recibe la coordenada 'y' de una linea horizontal y devuelve */
  /*     el punto de interseccion con el segmento                */
  Point  horiz_line_inter(const Geom_Number & __y)
  {
    
    Geom_Number intersection_x;
    float m;

    m = (float)(a->get_y() - b->get_y()) / (float)(a->get_x() - b->get_x());

    intersection_x = (Geom_Number)((__y - b->get_y())/m + b->get_x());
    
    Point  intersection_point(intersection_x,__y);

    return intersection_point;
  }

  /* Retorna el punto mas alto entre a y b  */  
  const Point * highest_point()
  {

    if (a->get_y() > b->get_y())
      return a;
    else
      return b;
  }

  /* Retorna el punto mas bajo entre a y b  */  
  const Point * lowest_point()
  {
    if (a->get_y() < b->get_y())
      return a;
    else
      return b;
  }
};

# endif // SEGMENT_H
