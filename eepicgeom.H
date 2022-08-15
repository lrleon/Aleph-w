
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
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
# ifndef EEPICGEOM_H
# define EEPICGEOM_H


# include <fstream>
# include <tpl_dynDlist.H>
# include <tpl_sort_utils.H>
# include <point.H>
# include <polygon.H>

extern bool tiny_keys;

// TODO: curvas de poligonos punteadas

class Eepic_Plane;


// Objeto geométrico abstracto para dibujarse en un plano Eeepic
class Eepic_Geom_Object
{
  Eepic_Geom_Object();

  Eepic_Geom_Object(const Eepic_Geom_Object&);

protected:

  // copia del objeto geométrico ==> es responsabilidad de esta
  // clase de manejar su memoria 
  const Geom_Object * const geom_object_ptr; 

  bool to_delete; // esta bandera en falso indica que no debe llamarse a
		  // delete geom_object_ptr; en el destructor

public:

  Eepic_Geom_Object(Geom_Object * ptr, const bool & __to_delete = false) 
    : geom_object_ptr(ptr), to_delete(__to_delete)
  {
    // empty
  }

  virtual ~Eepic_Geom_Object() 
  { 
    if (to_delete)
      delete geom_object_ptr;
  }

  virtual void draw(Eepic_Plane *, ostream &) const
  {
    if (geom_object_ptr == nullptr)
      throw std::domain_error("Eepic_Geom_Object is null");

    throw std::logic_error("Call to draw() method from abstract class");
  }

  // debe retornar un objeto de la clase derivada
  virtual Eepic_Geom_Object * clone() const = 0;

  virtual Point highest_point() const = 0;

  virtual Point lowest_point() const = 0;

  virtual Point leftmost_point() const = 0;

  virtual Point rightmost_point() const = 0;
};


class Eepic_Plane
{
  // marco de ambiente picture definido en tiempo de construcción
  double wide;   // ancho en puntos de la resolución
  double height; // altura en puntos de la resolución

  // marco final resultante según que el usuario decida o no escalar
  // la figura  
  double real_wide = 0;   // ancho en puntos de la resolución
  double real_height = 0; // altura en puntos de la resolución

  double xoffset;   // margen adicional de ancho
  double yoffset; // margen adicional de altura

  Geom_Number geom_wide;   // ancho desde el punto más a la izquierda
			   // hasta el más a la derecha
  Geom_Number geom_height; // ancho desde el punto más bajo hasta el
                           // más alto

  DynDlist<Eepic_Geom_Object*> list;

  Geom_Number geom_x_min; // mínimo valor de x en coordenadas reales
  Geom_Number geom_y_min; // mínimo valor de y en coordenadas reales

  // ATENCIÓN: estos valores deben inicializarse en cero porque son
  // usados para calcularse a sí mismos
  double x_min; // mínimo valor de x en el plano
  double y_min; // mínimo valor de y en el plano

  // puntos extremos con los cuales calcular el ancho y la altura de los
  // objetos (geom_x_min y geom_y_min)
  Point __leftmost;
  Point __rightmost;
  Point __highest;
  Point __lowest;

  bool with_cartesian_axis; // indica que se deben dibujar dos segmentos
			    // correspondientes a los ejes cartesianos 

  // Radio por omisión en resolución de un punto en el plano. Se utiliza
  // también para la altura de las letras
  static const double default_r;

  double r; // el radio en puntos reales con que se dibuja un punto

  // parámetros que ajustan la manera en que se define el ambiente latex
  // picture  
  double resolution; // resolución en milímetros (0.05mm por omisión)

  string fill_type; // tipo de relleno (black)

  string shade_type; // tipo de sombreado para segmentos 

  static const double normal_thickness;

  static const double default_shade_thickness;

  double shade_thickness;

  static const double dotgap; 

  /*
    Realiza una cambio de escala de un punto p en un plano a escala
    geom_max a un plano de escala max.

    p * max
    result = ----------
    geom_max
  */
  double geom_number_to_plane(const Geom_Number & p,
			      const Geom_Number & geom_max,
			      const double &      max) const
  { 
    const Geom_Number ratio = p / geom_max;

    double result = geom_number_to_double(ratio);

    result = max * result;
    
    return result;
  }

  /* Los puntos deben normalizarse al tamaño del plano en pixels. Estas
     rutinas toman la coordenada y la normalizan al eje cero (x o y)
  */

  // normaliza al eje x
  double xpic(const double & x) const
  {
    return x - x_min;
  }

  // normaliza al eje x
  double ypic(const double & y) const
  {
    return y - y_min;
  }

public:

  // calcula una proporción Geom_Number de x a su equivalente en en el
  // plano según wide
  double h_geom_number_to_eepic(const Geom_Number & x) const
  {
    return geom_number_to_plane(x, geom_wide, wide);
  }

  // calcula una proporción Geom_Number de y a su equivalente en en el
  // plano según height
  double v_geom_number_to_eepic(const Geom_Number & y) const
  {
    return geom_number_to_plane(y, geom_height, height);
  }

  // calcula una coordenada x de tipo Geom_Number a su equivalente
  // en en el plano 
  double x_geom_number_to_eepic(const Geom_Number & x) const
  {
    return xpic(geom_number_to_plane(x, geom_wide, wide));
  }

  // calcula una coordenada y de tipo Geom_Number a su equivalente
  // en en el plano 
  double y_geom_number_to_eepic(const Geom_Number & y) const
  {
    const double result = geom_number_to_plane(y, geom_height, height);

    return ypic(result);
  }

  // Realiza un barrido lineal de todos los objetos geométricos
  // presentes y calcula los punto extremos
  void compute_extreme_points()
  {
    // recorrer todos lo objetos en búsqueda de las coordenadas extremas
    // (más a la izquierad, a la derecha, alto y bajo
    DynDlist<Eepic_Geom_Object*>::Iterator it(list);

    Eepic_Geom_Object * curr = it.get_curr();

    __lowest    = curr->lowest_point();
    __highest   = curr->highest_point();
    __leftmost  = curr->leftmost_point();
    __rightmost = curr->rightmost_point();

    for (it.next(); it.has_curr(); it.next_ne())
      {
	curr = it.get_curr();

	if (curr->lowest_point().get_y() < __lowest.get_y())
	  __lowest = curr->lowest_point();

	if (curr->highest_point().get_y() > __highest.get_y())
	  __highest = curr->highest_point();

	if (curr->leftmost_point().get_x() < __leftmost.get_x())
	  __leftmost = curr->leftmost_point();

	if (curr->rightmost_point().get_x() > __rightmost.get_x())
	  __rightmost = curr->rightmost_point();
      }
  }

  // Aumenta y disminuye el plano eepic en puntos. Este es el método a
  // emplear para aumentar o disminuir el objeto eepic picture
  void zoom(const double & factor)
  {
    if (factor <= 0)
      throw std::domain_error("zoom factor must be greater than zero");

    height *= factor;
    wide *= factor;
  }

private:

  // calcula las coordenadas extremas y con ella el ancho y la altura de
  // los objetos geométricos colocados en el plano 
  // El parámetro square == true hace que la rescala del dibujo se
  // ajuste para que los objetos no se distorsionen 
  void compute_geom_plane(const bool & squarize)
  {
    compute_extreme_points();
    
    // calcular los márgenes mínimos según las figuras contenidas
    geom_height = __highest.get_y()   - __lowest.get_y();
    geom_wide   = __rightmost.get_x() - __leftmost.get_x();

    // Calcular las coordenadas mínimas x e y
    geom_x_min = geom_number_to_double(__leftmost.get_x()); 
    geom_y_min = geom_number_to_double(__lowest.get_y());

    // según los puntos mínimos, ajustamos al plano (0,0)
    x_min = h_geom_number_to_eepic(geom_x_min);
    y_min = v_geom_number_to_eepic(geom_y_min);

    if (squarize) // esta opción indica que el plano no debe deformar a
		  // las figuras
      { 
	if (geom_wide > geom_height)
	  {
	    geom_height = geom_wide;
	    real_height = y_geom_number_to_eepic(__highest.get_y());
	    real_wide = wide;
	  }
	else
	  { 
	    geom_wide = geom_height;
	    real_wide = x_geom_number_to_eepic(__rightmost.get_x());
	    real_height = height;
	  }
      }
    else
      {
	real_wide = wide;
	real_height = height;
      }

    assert(geom_height > 0 and geom_wide > 0);
  }

  void draw_cartesian_axis(ostream & output);

public:

  const Point & leftmost() const { return __leftmost; }

  const Point & rightmost() const { return __rightmost; }

  const Point & highest() const { return __highest; }

  const Point & lowest() const { return __lowest; }

  // dado el punto p, genera un string de la forma "(x, y)" con
  // coordenadas normalizadas al plano. Este string se usa para la
  // propia generación eepic
  string point_string(const Point & p)
  {
    const double x = x_geom_number_to_eepic(p.get_x());
    const double y = y_geom_number_to_eepic(p.get_y());
    return "(" + to_string(x) + "," + to_string(y) + ")";
  }

  /* genera en el stream output un ambiente
     \begin{picture}...\end{picture} contentivo de las figuras
     geométricas presentes.

     El parámetro squarize == false implica que la escala de los objetos
     son transformadas para caber exactamente en el plano definido en
     tiempo de construcción. squarize == true garantiza que las figuras
     se dibujen sin deformación pero en detrimento del espacio aprovechado
  */
  void draw(ostream & output, const bool & squarize = true)
  {
    compute_geom_plane(squarize);
    
    // fija el formato de salida punto flotante
    output << fixed << setprecision(6);

    // genera medidas y abre encabezado
    output << "\\setlength{\\unitlength}{" << resolution << "mm}" << endl
	   << endl
	   << "\\filltype{" << fill_type << "}" << endl
	   << endl
	   << "% leftmost point is " << __leftmost.to_string() << " --> "
	   << point_string(__leftmost) << endl
	   << "% rightmost point is " << __rightmost.to_string()  << " --> "
	   << point_string(__rightmost) << endl
	   << "% highest point is " << __highest.to_string()  << " --> "
	   << point_string(__highest) << endl
	   << "% lowest point is " << __lowest.to_string()  << " --> "
	   << point_string(__lowest) << endl
	   << "% " << list.size() << " geometric objects were put in the plane"
	   << endl
	   << "\\begin{picture}(" << real_wide << "," << real_height << ")" 
	   << "(" << xoffset << "," << yoffset << ")" << endl
	   << endl;

    draw_cartesian_axis(output); // en caso de que la bandera sea cierta

    // recorre cada objeto y lo dibuja
    for (DynDlist<Eepic_Geom_Object*>::Iterator it(list); 
	 it.has_curr(); it.next_ne())
      {
	Eepic_Geom_Object * curr = it.get_curr();
	
        // reinicia cada vez al grosor por omisión de forma
	output << "\\thinlines" << std::endl 
	       << std::endl;

	curr->draw(this, output); 
      }

    // cierra encabezado
    output << "\\end{picture}" << std::endl
	   << std::endl;
  }

  /*
    observadores del plano
  */
  const double & get_wide() const { return wide; }

  const double & get_height() const { return height; }

  const double & get_yoffset() const { return yoffset; }

  const double & get_xoffset() const { return xoffset; }

  const double & get_r() const { return r; }

  Eepic_Plane(const double & __wide,
	      const double & __height,
	      const double & __xoffset = 0.0,
	      const double & __yoffset = 0.0)
    : wide(__wide), height(__height), 
      xoffset(__xoffset), yoffset(__yoffset), 
      x_min(0), y_min(0), with_cartesian_axis(false),
      r(default_r), resolution(0.05), shade_thickness(default_shade_thickness)
  {
    // empty
  }

  ~Eepic_Plane()
  {
    while (not list.is_empty())
      delete list.remove_first_ne();
  }

  void put_cartesian_axis()
  {
    with_cartesian_axis = true;
  }

  /* Eepic_Plane maneja objetos de tipo Eepic_Geom_Object. Sin embargo,
     el fin es manejar directamente objetos derivados de
     Geom_Object. Esto seria posible con una clase Geom_Object altamente
     polimórfica que permita manejar los métodos clone, draw y puntos
     extremos virtualmente. Pero este enfoque conlleva dos
     problemas. (1) Geom_Object deviene de "grano grueso", pues, por
     ejemplo, Point es altamente usado. Esto implicaría un coste de al
     menos 7 punteros en la tabla virtual. (2) El carácter polimórfico
     varía según el plano; en la ocurrencia, dibujar en eepic no es
     igual a dibujar en un lienzo gtk+. El polimorfismo en Geom_Object
     no garantiza generalidad para el dibujado, pues éste depende del
     tipo de plano o fondo.

     Por la razón anterior, definimos, más adelante, un método externo,
     llamado put_in_plane(plane, geom_obj) el cual hace:
     
     1-. Clona geom_obj a su correspondiente eepic_geom_obj

     2-. Inserta el nuevo eepic_geom_obj en el plano

     Por simplicidad de implantación, esta funcion es pública, pero su
     uso es privado (de todos modos) no es simple lograr su invocación
  */
  void put(const Eepic_Geom_Object & __eepic_geom_obj)
  {
    Eepic_Geom_Object * eepic_geom_obj = __eepic_geom_obj.clone();

    list.append(eepic_geom_obj);
  }

  void set_resolution(const double & res)
  {
    resolution = res;
  }

  const double & get_resolution() const { return resolution; }

  void set_fill_type(const string & ftype)
  {
    fill_type = ftype;
  }

  void set_shade_thickness(const double & __thickness)
  {
    shade_thickness = __thickness;
  }

  double get_shade_thickness() const { return shade_thickness; }

  double get_dotgap() const { return dotgap; }

  /***************************************************************** 

     Definición de "métodos primitivos" de dibujado eepic de objetos
     geométricos. Los métodos draw() de cada clase pueden invocar
     alguno(s) de estos métodos. También, los métodos pueden ser
     invocados por el usuario (a su riesgo, por supuesto)

     Esta familia de "métodos primitivos" asume que los puntos extremos
     necesarios para mapear un punto al plano eepic ya han sido
     calculados mediante el método compute_geom_plane();

  *****************************************************************/

  // escribe en el stream output el código eepic para dibujar el
  // punto p
  void draw_point(const Point & p, ostream & output)
  {
    const double x = x_geom_number_to_eepic(p.get_x());

    const double y = y_geom_number_to_eepic(p.get_y());

    output << "    %% Point " << p.to_string()
	   << "    %% mapped in this plane to (" << x << "," << y << ")"
	   << std::endl
	   << "\\put(" << x << "," << y << "){\\ellipse{" << r
	   << "}{" << r << "}}" << std::endl
	   << std::endl;
  }

  // cálculo general de coordenadas origen y destino de un segmento. La
  // colocamos como macro para evitarnos líneas demás so por
  // declaraciones e invocación de método
# define COMPUTE_SEGMENT_EEPIC_COORDENATES()                            \
  const double src_x = x_geom_number_to_eepic(sg.get_src_point().get_x()); \
                                                                        \
  const double src_y = y_geom_number_to_eepic(sg.get_src_point().get_y()); \
                                                                        \
  const double tgt_x = x_geom_number_to_eepic(sg.get_tgt_point().get_x()); \
                                                                        \
  const double tgt_y = y_geom_number_to_eepic(sg.get_tgt_point().get_y()); 

  // En el mismo espíritu del macro anterior (y además porque refiere a
  // variables locales allí definidas), este macro dibuja el segmento sg
  // en el stream output
# define DRAW_SEGMENT(sg, output)                                       \
  COMPUTE_SEGMENT_EEPIC_COORDENATES();                                  \
  output << "    %% Segment from " << sg.get_src_point().to_string()    \
  << " to " << sg.get_tgt_point().to_string() << std::endl              \
  << "    %% mapped in this plane to (" << src_x << "," << src_y        \
  << ") to (" << tgt_x << "," << tgt_y << ")" << std::endl              \
  << "\\path(" << src_x << "," << src_y                                 \
  << ")(" << tgt_x << "," << tgt_y << ")" << std::endl                  \
  << std::endl;                                                         \

  // escribe en el stream output el código eepic para dibujar el
  // segmento sg
  void draw_segment(const Segment & sg, ostream & output)
  {
    DRAW_SEGMENT(sg, output);
  }

  static const double arrow_width_in_mm;

  static const double arrow_lenght_in_mm;

  // cálculo general de las coordenadas de dos rectas que pictorizan los
  // filos de las puntas de una flecha y su dibujado en el según las
  // coordenadas origen y destino usadas en el macro
  // COMPUTE_SEGMENT_EEPIC_COORDENATES. Usamos un macro por las mismas
  // razones anteriores. La longitud y amplitus de los filos están
  // determinados por las constantes anteriores arrow_width_in_mm y
  // arrow_lenght_in_mm 
# define PUT_ARROW()                                                    \
  const double arrow_width  = arrow_width_in_mm/resolution;             \
  const double arrow_lenght = arrow_lenght_in_mm/resolution;            \
                                                                        \
  const double l = sqrt(arrow_width*arrow_width + arrow_lenght*arrow_lenght); \
  const double tetha = atan2(arrow_width, arrow_lenght);                \
  const double phi = atan( fabs( (tgt_y - src_y)/(tgt_x - src_x) ) );   \
                                                                        \
  double dx1 = l*cos(phi - tetha);                                      \
  double dy1 = l*sin(phi - tetha);                                      \
                                                                        \
  double dx2 = l*sin(M_PI_2 - (phi + tetha));                           \
  double dy2 = l*cos(M_PI_2 - (phi + tetha));                           \
                                                                        \
  if (tgt_x > src_x)                                                    \
    {                                                                   \
      dx1 = -dx1;                                                       \
      dx2 = -dx2;                                                       \
    }                                                                   \
                                                                        \
  if (tgt_y > src_y)                                                    \
    {                                                                   \
      dy1 = -dy1;                                                       \
      dy2 = -dy2;                                                       \
    }                                                                   \
                                                                        \
  output << endl                                                        \
  << "    % Drawing of arrow's edges at (" << tgt_x                     \
  << "," << tgt_y << ")" << endl                                        \
  << "\\path(" << tgt_x << "," << tgt_y << ")("                         \
  << tgt_x + dx1 << "," << tgt_y + dy1 << ")" << endl                   \
  << "\\path(" << tgt_x << "," << tgt_y << ")("                         \
  << tgt_x + dx2 << "," << tgt_y + dy2 << ")" << endl << endl; 

  // genera en el stream output código eepic para dibujar segmento
  // sg con puntas de flecha
# define DRAW_ARROW(sg, output)                 \
  DRAW_SEGMENT(sg, output);                     \
  PUT_ARROW();

  // escribe en el stream output el código eepic para dibujar el
  // segmento sg con filos de flecha
  void draw_arrow(const Segment & sg, ostream & output)
  {
    DRAW_ARROW(sg, output);
  }

  // macro para dibujar un segmento puntillado
# define DRAW_DOTTED_SEGMENT(sg, output)                                \
  COMPUTE_SEGMENT_EEPIC_COORDENATES();                                  \
                                                                        \
  output << "    %% Dotted Segment from " << sg.get_src_point().to_string() \
  << " to " << sg.get_tgt_point().to_string() << std::endl              \
  << "    %% mapped in this plane to (" << src_x << "," << src_y        \
  << ") to (" << tgt_x << "," << tgt_y << ")" << std::endl              \
  << "\\dottedline{" << dotgap << "}(" << src_x                         \
  << "," << src_y << ")(" << tgt_x << "," << tgt_y << ")" << std::endl  \
  << std::endl; 

  // escribe en el stream output el código eepic para dibujar el
  // segmento puntillado sg
  void draw_dotted_segment(const Segment & sg, ostream & output)
  {
    DRAW_DOTTED_SEGMENT(sg, output);
  }

  // macro para generar código eepic en el setream output del
  // segmento sg en forma de línea partida
# define DRAW_DASH_SEGMENT(sg, output)                                  \
  COMPUTE_SEGMENT_EEPIC_COORDENATES();                                  \
                                                                        \
  output << "    %% Dash Segment from " << sg.get_src_point().to_string() \
  << " to " << sg.get_tgt_point().to_string() << std::endl              \
  << "    %% mapped in this plane to (" << src_x << "," << src_y        \
  << ") to (" << tgt_x << "," << tgt_y << ")" << std::endl              \
  << "\\dashline{" << dotgap << "}(" << src_x                           \
  << "," << src_y << ")(" << tgt_x << "," << tgt_y << ")" << std::endl  \
  << std::endl; 

  // escribe en el stream output el código eepic para dibujar el
  // segmento sg con línea partida
  void draw_dash_segment(const Segment & sg, ostream & output)
  {
    DRAW_DASH_SEGMENT(sg, output);
  }

  // dibuja segmento puntillado sg con puntas de flecha
# define DRAW_ARROW_DOTTED_SEGMENT(sg, output)                          \
  COMPUTE_SEGMENT_EEPIC_COORDENATES();                                  \
                                                                        \
  output << "    %% Dotted arrow Segment from " << sg.get_src_point().to_string() \
  << " to " << sg.get_tgt_point().to_string() << std::endl              \
  << "    %% mapped in this plane to (" << src_x << "," << src_y        \
  << ") to (" << tgt_x << "," << tgt_y << ")" << std::endl              \
  << "\\dottedline{" << dotgap << "}(" << src_x                         \
  << "," << src_y << ")(" << tgt_x << "," << tgt_y << ")" << std::endl  \
  << std::endl;                                                         \
  PUT_ARROW();

  // escribe en el stream output el código eepic para dibujar el
  // segmento puntillado sg con puntas de flecha
  void draw_arrow_dotted_segment(const Segment & sg, ostream & output)
  {
    DRAW_ARROW_DOTTED_SEGMENT(sg, output);
  }

  // dibuja un segmento con línea partida y punta de flecha
# define DRAW_ARROW_DASH_SEGMENT(sg, output)                            \
  COMPUTE_SEGMENT_EEPIC_COORDENATES();                                  \
                                                                        \
  output << "    %% Dash arrow Segment from " << sg.get_src_point().to_string() \
  << " to " << sg.get_tgt_point().to_string() << std::endl              \
  << "    %% mapped in this plane to (" << src_x << "," << src_y        \
  << ") to (" << tgt_x << "," << tgt_y << ")" << std::endl              \
  << "\\dashline{" << dotgap << "}(" << src_x                           \
  << "," << src_y << ")(" << tgt_x << "," << tgt_y << ")" << std::endl  \
  << std::endl;                                                         \
  PUT_ARROW();

  // escribe en el stream output el código eepic para dibujar el
  // segmento sg con línea partida y punta de flecha
  void draw_arrow_dash_segment(const Segment & sg, ostream & output)
  {
    DRAW_ARROW_DASH_SEGMENT(sg, output);
  }

  // escribe en el stream output el código eepic para dibujar la
  // elipse e
  void draw_ellipse(const Ellipse & e, ostream & output)
  {
    const Point & center = e.get_center();

    const double x = x_geom_number_to_eepic(center.get_x());
    
    const double y = y_geom_number_to_eepic(center.get_y());

    const double hd = h_geom_number_to_eepic(2*e.get_hradius());
    
    const double vd = v_geom_number_to_eepic(2*e.get_vradius());

    output << "    %% Ellipse with center in " << center.to_string()
	   << " with horizontal radius of " << e.get_hradius()
	   << " and vertical radius of " << e.get_vradius() << std::endl
	   << "    %% mapped in the plane at center (" << x
	   << "," << y << ") with horizontal radius of " << hd
	   << " and vertical radius of " << vd << std::endl
	   << "\\put(" << x << "," << y << "){\\ellipse{" << hd
	   << "}{" << vd << "}}" << std::endl
	   << std::endl;
  }

  // definición de puntero a función para dibujar segmento. Este
  // puntero permite usar una sola función para dibujar polígonos;
  // vistos sus lados como segmentos
  typedef void (Eepic_Plane::*Draw_Segment_Fct)(const Segment & poly, 
						ostream &     output);


  // escribe en el stream output el código eepic para dibujar el
  // polígono según los lados que lo conforman. draw_sg_fct es un
  // puntero a la función encargada de dibujar un segmento, la cual
  // se invoca cada vez que se requiera dibujar un lado.
  // Nótese que la función es genérica; en efecto, se usa tambien
  // para dibujar polígonos regulares
  template <class Poly>
  void draw_polygon(Poly &           poly, 
		    ostream &       output,
		    Draw_Segment_Fct draw_sg_fct)
  {
    for (typename Poly::Segment_Iterator it(poly); it.has_curr(); it.next_ne())
      (this->*draw_sg_fct)(it.get_current_segment(), output);
  }

  // escribe en el stream output el código eepic para dibujar el
  // polígono poly con su interior sombreado. Es independiente de
  // que esté o no cerrado. La diferencia con la versión
  // sobrecargada de tres parámetros es que este genera un camino de
  // vértices en lugar de una serie de segmentos. Esta version es
  // mucho más rápida que la versión de tres parámetros; aunque más
  // menos versátil (nada es sin costes; así es la vida)
  template <class Poly>
  void draw_closed_polygon(Poly & poly, ostream & output)
  {
    output << "    % Drawing of polygon of " << poly.size() 
	   << " sides" << endl
	   << "\\shade\\path";

    for (typename Poly::Vertex_Iterator it(poly); it.has_curr(); it.next_ne())
      output << point_string(it.get_current_vertex());

    if (poly.is_closed())
      output << point_string(poly.get_first_vertex()) << endl;
  }

  // escribe en el stream output el código eepic para dibujar el
  // polígono con su interior sombreado y las líneas de sus lados
  // punteadas. Es independiente de que esté o no cerrado
  template <class Poly>
  void draw_dotted_closed_polygon(Poly &  poly, ostream & output)
  {
    output << "    % Drawing of dotted line Polygon" << endl
	   << "\\shade\\dottedline{" << dotgap << "}";

    for (typename Poly::Vertex_Iterator it(poly); it.has_curr(); it.next_ne())
      output << point_string(it.get_current_vertex());

    if (poly.is_closed())
      output << point_string(poly.get_first_vertex()) << endl;
  }

  // escribe en el stream output el código eepic para dibujar los
  // puntos correspondientes los puntos vértices del polígono
  // poly. ATENCIÓN: NO DIBUJA LOS LADOS; SÓLO DIBUJA LOS
  // VÉRTICES 
  template <class Poly>
  void draw_dots_from_polygon(Poly & poly, ostream & output)
  {
    for (typename Poly::Vertex_Iterator it(poly); it.has_curr(); it.next_ne())
      {
	Vertex & vertex = it.get_current_vertex();
      
	draw_point(vertex, output);
      }
  }

  // genera código eepic en el stream output para el filo de flecha
  // del segmento sg
  void draw_arrow_extreme(const Segment & sg, ostream & output)
  {    // ¡tal cual!
    COMPUTE_SEGMENT_EEPIC_COORDENATES();
    PUT_ARROW();
  }

  // escribe en el stream output el código eepic para dibujar las
  // puntas de flecha correspondientes los puntos vértices del
  // polígono poly. ATENCIÓN: NO DIBUJA LOS LADOS; SÓLO DIBUJA LOS
  // LOS FILOS DE FLECHA DE LO QUE SON LOS LADOS DEL POLÍGONO
  // VISTOS COMO SEGMENTOS
  template <class Poly>
  void draw_arrows_in_polygon(Poly & poly, ostream & output)
  {
    for (typename Poly::Segment_Iterator it(poly); it.has_curr(); it.next_ne())
      {
	const Segment & sg = it.get_current_segment();
      
	draw_arrow_extreme(sg, output);
      }
  }

  template <class Poly>
  void draw_closed_polygon_with_arrows(Poly & poly, ostream & output)
  {
    draw_closed_polygon(poly, output);
    draw_arrows_in_polygon(poly, output);
  }

  // escribe en el stream output el código eepic para dibujar el
  // punto texto correspondiente al punto p. El texto es de la forma
  // "(x,y)". La coordenada y se desplaza ligeramente hacia arriba
  // en r de modo tal de dejar espacio para escribir el número de
  // vértice por debajo
  void draw_point_text(const Point & p, ostream & output)
  {
    const double x = x_geom_number_to_eepic(p.get_x());

    const double y = y_geom_number_to_eepic(p.get_y()) + r;

    const string str = p.to_string();

    output << "    %% Text Point " << str
	   << "    %% mapped in this plane to (" << x << "," << y << ")"
	   << std::endl
	   << "\\put(" << x << "," << y << "){\\texttt{" << str
	   << "}}" << std::endl
	   << std::endl;
  }

  // escribe en el stream output el código eepic para dibujar los
  // textos de los vértices del polígono poly. ATENCIÓN: NO DIBUJA
  // LOS LADOS; SÓLO DIBUJA LOS TEXTOS DE LOS VÉRTICES
  // TODO: Escribir el texto fuera o dentro del polígono sin que
  // cruce un lado ==> que se requiere el contexto de los lados ==>
  // que no se podría usar draw_point_text
  template <class Poly>
  void draw_points_text_in_polygon(Poly & poly, ostream & output)
  {
    for (typename Poly::Vertex_Iterator it(poly); it.has_curr(); it.next_ne())
      draw_point_text(it.get_current_vertex(), output);
  }

  // escribe en el stream output el código eepic para dibujar los
  // números de vértice del polígono poly. ATENCIÓN: NO DIBUJA LOS
  // LADOS; SÓLO DIBUJA LOS NÚMEROS DE LOS VÉRTICES TODO: Escribir el
  // texto fuera o dentro del polígono sin que cruce un lado ==> que
  // se requiere el contexto de los lados ==> que no se podría usar
  // draw_point_text
  template <class Poly>
  void draw_vertex_numbers_in_polygon(Poly & poly, ostream & output)
  {
    int n = 0;

    for (typename Poly::Vertex_Iterator it(poly); 
	 it.has_curr(); it.next_ne(), ++n)
      {
	Vertex & vertex = it.get_current_vertex();
      
	const double x = x_geom_number_to_eepic(vertex.get_x());

	const double y = y_geom_number_to_eepic(vertex.get_y()) - r;

	output << "    %% Poligon Vertex Number " << n
               << "    %% mapped in this plane to (" << x << "," << y << ")"
               << std::endl
               << "\\put(" << x << "," << y << "){$" << n << "$}" << std::endl
               << std::endl;
      }
  }

  // escribe en el stream output el código eepic para dibujar la
  // curva spline caracterizada por los puntos del polígono y que
  // pasa por sus puntos extremos
  template <class Poly>
  void draw_spline(Poly & poly, ostream & output)
  {
    output << "    % Drawing spline of " << poly.size() 
	   << " points" << endl
	   << "\\spline";

    for (typename Poly::Vertex_Iterator it(poly); it.has_curr(); it.next_ne())
      output << point_string(it.get_current_vertex());

    if (poly.is_closed())
      output << point_string(poly.get_first_vertex()) << endl;
  }

  // escribe en el stream output el código eepic para dibujar la
  // curva a línea partida caracterizada por los puntos del polígono
  // y que pasa por sus puntos extremos
  template <class Poly>
  void draw_dash_spline(Poly & poly, ostream & output)
  {
    output << "    % Drawing dash curve of " << poly.size() 
	   << " points" << endl
	   << "\\closecurve";

    for (typename Poly::Vertex_Iterator it(poly); it.has_curr(); it.next_ne())
      output << point_string(it.get_current_vertex());

    if (poly.is_closed())
      output << point_string(poly.get_first_vertex()) << endl;
  }

  // escribe en el stream output el código eepic para dibujar la
  // curva spline caracterizada por los puntos del polígono y que
  // pasa por sus puntos extremos y cuyo último vértice tiene punta
  // de flecha
  template <class Poly>
  void draw_spline_arrow(Poly & poly, ostream & output)
  {
    draw_spline(poly, output); // dibujamos primero la curva

    // ahora dibujamos los filos de las flechas según la pendiente del
    // último segmento del polígono
    
    draw_arrow_extreme(poly.get_last_segment(), output);
  }

  // escribe en el stream output el código eepic para dibujar la
  // curva spline a línea partida caracterizada por los puntos del
  // polígono y que pasa por sus puntos extremos y cuyo último
  // vértice tiene punta de flecha
  template <class Poly>
  void draw_dash_spline_arrow(Poly & poly, ostream & output)
  {
    draw_dash_spline(poly, output); // dibujamos primero la curva

    // ahora dibujamos los filos de las flechas según la pendiente del
    // último segmento del polígono

    draw_arrow_extreme(poly.get_last_segment(), output);
  }

  void draw_text(const Text & t, ostream & output, 
		 const double & hfactor, const double & vfactor)
  {
    const Point & p = t.get_point();

    const double str_size_in_points = t.len()*Text::font_width_in_points;

    const double x = 
      x_geom_number_to_eepic(p.get_x()) - hfactor*str_size_in_points; 

    const double y = (y_geom_number_to_eepic(p.get_y()) - 
		      vfactor*Text::font_height_in_points); 

    output << "%    string at " << p.to_string() << endl 
	   << "%    mapped to the plane at (" << x << "," << y << ")" << endl 
	   << "\\put(" << x << "," << y << "){\\texttt{" 
	   << (tiny_keys ? "\\tiny " : "") << t.get_str() 
	   << "}}" << endl 
	   << endl;
  }
};


/*****************************************************************

   Definición del generador de objetos derivados de Eepic_Geom_Object

*****************************************************************/

# define DEFINE_EEPIC_COPY_CTOR(Type)                                   \
  Eepic_##Type(const Type & o) : Eepic_Geom_Object(new Type(o), true) { } 


// métodos en común que siempre tiene una clase Eepic_Type
# define DEFINE_EEPIC_COMMON_METHODS(Type)                              \
  friend void put_in_plane(Eepic_Plane & plane, const Type & geom_obj); \
                                                                        \
  ~Eepic_##Type() { }                                                   \
                                                                        \
  virtual Point highest_point() const                                   \
  {                                                                     \
    return static_cast<const Type*>(geom_object_ptr)->highest_point();  \
  }                                                                     \
                                                                        \
  virtual Point lowest_point() const                                    \
  {                                                                     \
    return static_cast<const Type*>(geom_object_ptr)->lowest_point();   \
  }                                                                     \
                                                                        \
  virtual Point leftmost_point() const                                  \
  {                                                                     \
    return static_cast<const Type*>(geom_object_ptr)->leftmost_point(); \
  }                                                                     \
                                                                        \
  virtual Point rightmost_point() const                                 \
  {                                                                     \
    return static_cast<const Type*>(geom_object_ptr)->rightmost_point(); \
  }                                                                     \
                                                                        \
  virtual Eepic_Geom_Object * clone() const                             \
  {                                                                     \
    return new Eepic_##Type(static_cast<const Type&>(*geom_object_ptr)); \
  }                                                                     \
                                                                        \
  virtual void draw(Eepic_Plane *, ostream &) const;


// Métodos comunes de Eepic_Type más el constructor copia
# define DEFINE_EEPIC_COMMON_WITH_COPY_CTOR(Type)       \
  DEFINE_EEPIC_COPY_CTOR(Type)                          \
  DEFINE_EEPIC_COMMON_METHODS(Type)


// Métodos comunes de Eepic_Type pero el constructor copia sólo está
// declarado ==> debe escribirse después 
# define DEFINE_EEPIC_COMMON_WITHOUT_COPY_CTOR(Type)    \
  DEFINE_EEPIC_COMMON_METHODS(Type)                     \
  Eepic_##Type(const Type &);


// Método de insertar un objeto Eepic_Type en un Eepic_Plane
# define DEFINE_PUT_IN_PLANE(Type)                                      \
  extern void put_in_plane(Eepic_Plane & plane, const Type & geom_obj)


# define IMPL_PUT_IN_PLANE(Type)                                \
  void put_in_plane(Eepic_Plane & plane, const Type & geom_obj)	\
  {                                                             \
    Eepic_##Type eepic_obj(geom_obj);                           \
                                                                \
    plane.put(eepic_obj);                                       \
  } 

/*
  Este es el macro fundamental para declarar una clase derivada de
  Eepic_Geom_Object 
*/

// definición típica de una clase Eepic_Type. Sólo se debe implantar
// la función virtual Eepic_Plane::put(const Eepic_Type &)
# define DEFINE_EEPIC_CLASS(Type)               \
  class Eepic_##Type : public Eepic_Geom_Object \
  {                                             \
    DEFINE_EEPIC_COMMON_WITH_COPY_CTOR(Type)    \
  };                                            \
                                                \
  DEFINE_PUT_IN_PLANE(Type) 


// definición típica de una clase Eepic_Type con otros
// atributos. Esto implica que hay que definir por separado el
// constructor copia que realice la inicialización de los atributos
// adicionales 
# define DEFINE_EEPIC_CLASS_AND_MEMBERS(Type, ...)	\
  class Eepic_##Type : public Eepic_Geom_Object         \
  {                                                     \
    __VA_ARGS__ ;                                       \
    DEFINE_EEPIC_COMMON_WITHOUT_COPY_CTOR(Type)         \
  };                                                    \
                                                        \
  DEFINE_PUT_IN_PLANE(Type) 


/*****************************************************************

  A partir de aquí el asunto sólo consiste en definir nuevas clases
  Eepic según el objeto y la funcionalidad de dibujado que se desee
  implantar 

*****************************************************************/

DEFINE_EEPIC_CLASS(Point);


DEFINE_EEPIC_CLASS(Segment);


/*****************************************************************
  
  Requerimos definir varias maneras de dibujar segmentos. Este macro
  define una nueva clase derivada de Segment. Su único propósito es
  tener un nuevo nombre de objeto geométrico que permita definir
  comportamientos de dibujado distintos
*/
# define DEFINE_CLASS_DERIVATED_FROM_SEGMENT(Name)      \
  struct Name : public Segment                          \
  {                                                     \
    Name() { }                                          \
                                                        \
    Name(const Segment & s) : Segment(s)                \
    {                                                   \
                                                        \
    }                                                   \
    Name(const Point & __src, const Point & __tgt)      \
      : Segment(__src, __tgt)                           \
    {                                                   \
                                                        \
    }                                                   \
                                                        \
    Name(const Point &       __src,                     \
         const Geom_Number & m,                         \
         const Geom_Number & d)                         \
      : Segment(__src, m, d)                            \
    {                                                   \
                                                        \
    }                                                   \
  };

// define una clase derivada de segmento con nombre Name - El propósito
// es nombramiento para distiguirlo de Segment 
# define DEFINE_EEPIC_SEGMENT_CLASS(Name)       \
  DEFINE_CLASS_DERIVATED_FROM_SEGMENT(Name)     \
  DEFINE_EEPIC_CLASS(Name);


// Igual que la anterior pero permite incorpora otro miembros en el
// parámetro macro Members
# define DEFINE_EEPIC_SEGMENT_CLASS_AND_MEMBERS(Name, ...)	\
  DEFINE_CLASS_DERIVATED_FROM_SEGMENT(Name)                     \
  DEFINE_EEPIC_CLASS_AND_MEMBERS(Name, __VA_ARGS__);


/*****************************************************************
  Familia de clases de maneras para dibujar un segmento
*/

DEFINE_EEPIC_SEGMENT_CLASS(Arrow)

DEFINE_EEPIC_SEGMENT_CLASS(Thick_Segment);

DEFINE_EEPIC_SEGMENT_CLASS(Thick_Dash_Segment);

DEFINE_EEPIC_SEGMENT_CLASS(Thick_Arrow);

DEFINE_EEPIC_SEGMENT_CLASS(Dotted_Segment);

DEFINE_EEPIC_SEGMENT_CLASS(Dash_Segment);

DEFINE_EEPIC_SEGMENT_CLASS(Dotted_Arrow);

DEFINE_EEPIC_SEGMENT_CLASS(Dash_Arrow);

DEFINE_EEPIC_SEGMENT_CLASS(Thick_Dash_Arrow);


/*****************************************************************

   Triangulo

*/
DEFINE_EEPIC_CLASS(Triangle);


/*****************************************************************

  Elipse
*/
DEFINE_EEPIC_CLASS(Ellipse);


// define una nueva clase derivada de elipse que permita definir
// conductas de dibujado distintas
# define DEFINE_CLASS_DERIVATED_FROM_ELLIPSE(Name)      \
  struct Name : public Ellipse                          \
  {                                                     \
    Name() { }                                          \
                                                        \
    Name(const Ellipse & p) : Ellipse(p)                \
    {                                                   \
                                                        \
    }                                                   \
                                                        \
    Name(const Point &       center,                    \
         const Geom_Number & hr,                        \
         const Geom_Number & vr)                        \
      : Ellipse(center, hr, vr)                         \
    {                                                   \
                                                        \
    }                                                   \
  };


DEFINE_CLASS_DERIVATED_FROM_ELLIPSE(Shade_Ellipse);
DEFINE_CLASS_DERIVATED_FROM_ELLIPSE(Thick_Ellipse);
DEFINE_EEPIC_CLASS(Shade_Ellipse);
DEFINE_EEPIC_CLASS(Thick_Ellipse);


/*****************************************************************

  Texto
*/

# define DEFINE_CLASS_DERIVATED_FROM_TEXT(Name)         \
  struct Name : public Text                             \
  {                                                     \
    Name() { }                                          \
                                                        \
    Name(const Text & t) : Text(t)                      \
    {                                                   \
                                                        \
    }                                                   \
                                                        \
    Name(const Point & p, const std::string & str)      \
      : Text(p, str)                                    \
    {                                                   \
                                                        \
    }                                                   \
  };


DEFINE_EEPIC_CLASS(Text);

DEFINE_CLASS_DERIVATED_FROM_TEXT(Left_Text);
DEFINE_CLASS_DERIVATED_FROM_TEXT(Right_Text);
DEFINE_CLASS_DERIVATED_FROM_TEXT(Center_Text);

DEFINE_EEPIC_CLASS(Left_Text);
DEFINE_EEPIC_CLASS(Right_Text);
DEFINE_EEPIC_CLASS(Center_Text);


/*****************************************************************

   Ahora requerimos varias maneras de dibujar polígonos generales
*/

// La más simple forma de dibujar un polígono
DEFINE_EEPIC_CLASS(Polygon);



// define una nueva clase derivada de poligono que permita definir
// conductas de dibujado distintas
# define DEFINE_CLASS_DERIVATED_FROM_POLYGON(Name)      \
  struct Name : public Polygon                          \
  {                                                     \
    Name() { }                                          \
                                                        \
    Name(const Polygon & p) : Polygon(p)                \
    {                                                   \
                                                        \
    }                                                   \
  };


# define DEFINE_EEPIC_POLYGON(Name)             \
  DEFINE_CLASS_DERIVATED_FROM_POLYGON(Name);    \
  DEFINE_EEPIC_CLASS(Name);


// Familia de polígonos

// con círculos punto en los vértices
DEFINE_EEPIC_POLYGON(Polygon_With_Points); 

// con círculos punto en los vértices
DEFINE_EEPIC_POLYGON(Polygon_With_Arrows); 

// lados puntillados
DEFINE_EEPIC_POLYGON(Dotted_Polygon);

// lados línea partida
DEFINE_EEPIC_POLYGON(Dash_Polygon);

// lados puntillados y círculos en los vértices
DEFINE_EEPIC_POLYGON(Dotted_Polygon_With_Points); 

// lados líneas partidas con círculos en los vértices
DEFINE_EEPIC_POLYGON(Dash_Polygon_With_Points);

// lados líneas partidas con punta d eflecha en último segmento
DEFINE_EEPIC_POLYGON(Dash_Polygon_With_Arrow);

// el interior del polígono es sobreado (gris)
DEFINE_EEPIC_POLYGON(Shade_Polygon); 

// el interior del polígono es sobreado (gris) con círculos en los vértices
DEFINE_EEPIC_POLYGON(Shade_Polygon_With_Points);

// el interior del polígono es sobreado (gris) y lados son flechas
DEFINE_EEPIC_POLYGON(Shade_Polygon_With_Arrows);

// interior sobreado (gris) y con coordenadas de los vértices
DEFINE_EEPIC_POLYGON(Shade_Polygon_With_Text_Points);

// interior sobreado (gris) y con números de los vértices
DEFINE_EEPIC_POLYGON(Shade_Polygon_With_Vertex_Numbers);

// Curva spline construida según los puntos del polígono
DEFINE_EEPIC_POLYGON(Spline);

// Curva spline a línea partida construida según los puntos del polígono
DEFINE_EEPIC_POLYGON(Dash_Spline);

// Curva spline construida según los puntos del polígono con flecha
DEFINE_EEPIC_POLYGON(Spline_Arrow);

// Curva spline a línea partida construida según los puntos del
// polígono con flecha
DEFINE_EEPIC_POLYGON(Dash_Spline_Arrow);

// con círculos punto en los vértices
DEFINE_EEPIC_POLYGON(Thick_Polygon_With_Points); 

// con círculos punto en los vértices
DEFINE_EEPIC_POLYGON(Thick_Polygon_With_Arrows); 

// lados puntillados
DEFINE_EEPIC_POLYGON(Thick_Dotted_Polygon);

// lados línea partida
DEFINE_EEPIC_POLYGON(Thick_Dash_Polygon);

// lados puntillados y círculos en los vértices
DEFINE_EEPIC_POLYGON(Thick_Dotted_Polygon_With_Points); 

// lados líneas partidas con círculos en los vértices
DEFINE_EEPIC_POLYGON(Thick_Dash_Polygon_With_Points);

// lados líneas partidas con puntas de flechas
DEFINE_EEPIC_POLYGON(Thick_Dash_Polygon_With_Arrow);

// el interior del polígono es sobreado (gris)
DEFINE_EEPIC_POLYGON(Thick_Shade_Polygon); 

// el interior del polígono es sobreado (gris) con círculos en los vértices
DEFINE_EEPIC_POLYGON(Thick_Shade_Polygon_With_Points);

// el interior del polígono es sobreado (gris) y lados son flechas
DEFINE_EEPIC_POLYGON(Thick_Shade_Polygon_With_Arrows);

// interior sobreado (gris) y con coordenadas de los vértices
DEFINE_EEPIC_POLYGON(Thick_Shade_Polygon_With_Text_Points);

// interior sobreado (gris) y con números de los vértices
DEFINE_EEPIC_POLYGON(Thick_Shade_Polygon_With_Vertex_Numbers);

// Curva spline construida según los puntos del polígono
DEFINE_EEPIC_POLYGON(Thick_Spline);

// Curva spline a línea partida construida según los puntos del polígono
DEFINE_EEPIC_POLYGON(Thick_Dash_Spline);

// Curva spline construida según los puntos del polígono con flecha
DEFINE_EEPIC_POLYGON(Thick_Spline_Arrow);

// Curva spline a línea partida construida según los puntos del
// polígono con flecha
DEFINE_EEPIC_POLYGON(Thick_Dash_Spline_Arrow);

/*****************************************************************

 Ahora haremos casi lo mismo para los polígonos regulares.

 NOTA: en realidad, con la definición del constructor de
 Polygon(Regular_Polygon) y del operador de conversión Regular_Polygon
 hacia Polygon se puede dibujar polígonos regulares mediante las clases
 para dibujar polígonos irregulares. El asunto es, pues, sólo de un poco
 de eficiencia: se puede dibujar un polígono regular en tiempo constante
 más rápido que uno irregular. En todo caso, la complejidad de tiempo es
 la misma O(n) para las dos clases
*/

// define una nueva clase derivada de poligono regular que permita
// definir conductas de dibujado distintas. Hay que definir una
// nueva porque el constructor es distinto
# define DEFINE_CLASS_DERIVATED_FROM_REGULAR_POLYGON(Name)      \
  struct Name : public Regular_Polygon                          \
  {                                                             \
    Name() { }                                                  \
                                                                \
    Name(const Regular_Polygon & p) : Regular_Polygon(p)        \
    {                                                           \
                                                                \
    }                                                           \
                                                                \
    Name(const Point &  c,                                      \
         const double & side_sz,                                \
         const size_t & n,                                      \
         const double & ang = 0)                                \
      : Regular_Polygon(c, side_sz, n, ang)                     \
    {                                                           \
                                                                \
    }                                                           \
  };


# define DEFINE_EEPIC_REGULAR_POLYGON(Name)             \
  DEFINE_CLASS_DERIVATED_FROM_REGULAR_POLYGON(Name);    \
  DEFINE_EEPIC_CLASS(Name);


DEFINE_EEPIC_CLASS(Regular_Polygon);

// Definiciones para los polígonos regulares
DEFINE_EEPIC_REGULAR_POLYGON(Regular_Polygon_With_Points);
DEFINE_EEPIC_REGULAR_POLYGON(Dotted_Regular_Polygon);
DEFINE_EEPIC_REGULAR_POLYGON(Dash_Regular_Polygon);
DEFINE_EEPIC_REGULAR_POLYGON(Dotted_Regular_Polygon_With_Points);
DEFINE_EEPIC_REGULAR_POLYGON(Dash_Regular_Polygon_With_Points);
DEFINE_EEPIC_REGULAR_POLYGON(Shade_Regular_Polygon);
DEFINE_EEPIC_REGULAR_POLYGON(Shade_Regular_Polygon_With_Points);
DEFINE_EEPIC_REGULAR_POLYGON(Shade_Regular_Polygon_With_Arrows);
DEFINE_EEPIC_REGULAR_POLYGON(Shade_Regular_Polygon_With_Text_Points);
DEFINE_EEPIC_REGULAR_POLYGON(Shade_Regular_Polygon_With_Vertex_Numbers);

  

# endif // EEPICGEOM_H
