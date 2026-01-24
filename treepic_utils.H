
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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


/** @file treepic_utils.H
 *  @brief Tree picture generation utilities.
 *
 *  @warning This file uses static global variables for drawing parameters
 *           and is NOT thread-safe. Do not use these functions concurrently
 *           from multiple threads.
 *
 *  @ingroup Utilities
 *  @author Leandro Rabindranath León
 */

# ifndef TREEPIC_UTILS_H
# define TREEPIC_UTILS_H

# include <cstdio>
# include <cmath>

# include <cstdlib>
# include <cstring>
# include <iostream>
# include <fstream>

# include <aleph.H>

const long double sin_45 = sin(M_PI_4);
const long double cos_45 = cos(M_PI_4);

const char * const font_wrapper = "\\texttt{"; 
const char * fill_type = "shade";

static const long double points_per_inch = 72.27;
static const long double mm_per_inch = 25.4;

long double font_width_in_points = 5; 

long double font_width_in_mm =
    font_width_in_points*mm_per_inch/points_per_inch; 
long double font_height_in_mm = 2.0;

long double resolution = 0.05; /* resolution en milimetros */

bool flip_y = false;

bool tiny_keys = false;


extern const long double sin_45;
extern const long double cos_45;

extern std::string input_file_name;
extern std::string output_file_name;


extern const char * const font_wrapper; 
extern const char * fill_type;

extern long double resolution;
extern long double font_width_in_mm; 
extern long double font_height_in_mm; 
extern long double font_width_in_points;

extern long double resolution; /* resolution en milimetros */
extern double v_size;

//extern bool tiny_keys;

/* ancho de letra en puntos segun resolucion */
inline long double font_width()
{
  return font_width_in_mm/resolution;
}


inline long double font_height()
{
  return font_height_in_mm/resolution;
}



/* 
   calcula los puntos de interseccion entre una recta y una elipse. La
   recta pasa por el punto central de la elipse.

   lx0, ly0 : punto centro de la elipse y primer punto de la recta
   lx1, ly1 : segundo punto de la recta
   a,b parametros de elipse (x - lx0)^2/a^2 + y - ly0)^2/b^2

   dx, dy son las diferencias en x e y de los puntos de interseccion 
*/
inline void 
intersection_ellipse_line(long double lx0, // puntos recta
			  long double ly0, 
			  long double lx1, 
			  long double ly1, 
			  long double a, // parametros elipse (centro lx0,ly0)
			  long double b, 
			  long double & dx, 
			  long double & dy) /* Diferencias respecto al
						centro de la elipse */
{
  long double m = (1.0*(ly1 - ly0)) / (1.0*(lx1 - lx0));
  long double m2 = m*m;
  long double a2 = a*a;
  long double b2 = b*b;
  long double R = m2 + b2/a2;
  long double L = lx0*lx0 - b2/R;
  long double S = 1.0/m2 + a2/b2;
  long double M = ly0*ly0 - a2/S; 

  long double x0_2 = 2.0*lx0;
  long double xdisc = sqrt(4.0*lx0*lx0 - 4.0*L);

  long double ix0 = (x0_2 + xdisc) / 2.0;

  dx = ix0 - lx0;
  assert(dx >= 0);

  long double y0_2 = 2.0*ly0;
  long double ydisc = sqrt(4.0*ly0*ly0 - 4.0*M);

  long double iy0 = (y0_2 - ydisc) / 2.0;

  dy = ly0 - iy0;
  assert(dy >= 0);
}


inline void 
intersection_line_line(long double lx1, // puntos recta 1
		       long double ly1, 
		       long double lx2, 
		       long double ly2, 
		       long double rx1, // puntos recta 2
		       long double ry1, 
		       long double rx2, 
		       long double ry2,
		       long double & x,
		       long double & y)
{
  long double lm = (ly2 - ly1)/(lx2 - lx1); // pendiente recta l
  long double rm = (ry2 - ry1)/(rx2 - rx1); // pendiente recta r

  x = (lm*lx1 - rm*rx1 + ry1 - ly1)/(lm - rm);

  long double lm_inv = 1/lm; // inversa pendiente recta l
  long double rm_inv = 1/rm; // inversa pendiente recta r

  y = (lm_inv*ly1 - rm_inv*ry1 + rx1 - lx1)/(lm_inv - rm_inv);
}


inline long double distance_between_points(long double x1, 
                                           long double y1, 
                                           long double x2,
                                           long double y2)
{
  long double a = x2 - x1;
  long double b = y2 - y1;

  return sqrt(a*a + b*b);
}

/* 
   calcula los puntos de interseccion entre una recta y un
   rectangulo. La recta pasa por el punto central del rectangulo.

   lx0, ly0 : punto centro del rectangulo y primer punto de la recta
   lx1, ly1 : segundo punto de la recta
   a,b radios horizontal y vertical del rectangulo

   dx, dy son las diferencias en x e y de los puntos de interseccion 
*/
inline void 
intersection_rectangle_line(long double lx0, // puntos recta
			    long double ly0, 
			    long double lx1, 
			    long double ly1, 
			    long double a, // radio hor (centro lx0,ly0)
			    long double b, // radio ver
			    long double & dx, 
			    long double & dy) /* Diferencias respecto al
						 centro rectangulo */
{
  assert(a >= 0);
  assert(b >= 0);

  /* La intersección requiere considerar intersección con dos rectas
     del rectangulo: la horizontal y la vertical */

    // diferencia con punto de interseccion con recta horizontal
  long double dxh = b * ((lx1 - lx0)/(ly1 - ly0));
  long double dyh = b;
  
      // diferencia con punto de interseccion con recta vertical
  long double dxv = a;
  long double dyv = a * ((ly1 - ly0)/(lx1 - lx0));

  /* calcular distancias desde centro rectangulo hasta los puntos de
     interseccion. El punto será el correspondiente a la menor
     distancia. Para ello calculamos cuadrados de hipotenusas según el
     milenario Pitágoras y comparamos */
  if (dxh*dxh + dyh*dyh < dxv*dxv + dyv*dyv)
    {
      dx = std::fabs(dxh);
      dy = b;
    }
  else
    {
      dx = a;
      dy = std::fabs(dyv);
    }
}

extern bool flip_y;

inline long double YPIC(long double y) 
{ 
  return flip_y ? y : v_size - y; 
}


inline size_t compute_true_len(const std::string & str)
{
  const char * ptr = str.c_str();
 
  for (int i = 0, counter = 0; true; /* empty */)
    {
      switch (ptr[i])
	{
	case '\\':
	  for (i++; isalnum(ptr[i]) and ptr[i] not_eq '\0'; /* nothing */)  
	    i++;
	  counter++; 
	  break;
	case '$': case '{': case '}': case '\n': 
	  i++;
	  break;
	case '\0':
	  return counter;
	default:
	  counter++; i++;
	  break;	  
	}
    }
}


inline void put_string(std::ofstream& output,
                       const long double& x, const long double& y,
                       const std::string & comment, const std::string & str)
{
  if (str.size() == 0)
    return;

  output << std::endl
	 << "%    " << comment << std::endl
	 << "\\put(" << x << "," 
            /* Se ajusta la posicion y del caracter segun si la
	       impresion es flip o no */   
	 << (flip_y ? YPIC(y) - font_height() : YPIC(y))
	 << "){" ;
  if (tiny_keys)
    output << font_wrapper << "{\\tiny " << str << "}}}" << std::endl << std::endl;
  else
    output << font_wrapper << str << "}}" << std::endl << std::endl;
}

inline void put_string_tkiz(std::ofstream& output,
                            const long double& x, const long double& y,
                            const std::string & comment, const std::string & str)
{
   if (str.size() == 0)
    return;

  output << std::endl
	 << "%    " << comment << std::endl
	 << "\\draw (" << x << "mm," 
            /* Se ajusta la posicion y del caracter segun si la
	       impresion es flip o no */   
	 << (flip_y ? YPIC(y) - font_height() : YPIC(y))
	 << "mm) node { " << str << " }" << std::endl << std::endl;
}

/// @name Arrow drawing parameters (in mm)
/// @warning These global variables are NOT thread-safe
/// @{
static long double arrow_width_in_mm = 0.5;
static long double arrow_lenght_in_mm = 1.5;
/// @}
bool with_arrow = false;
long double dash_len = 1/resolution; // 1m
long double arrow_width  = arrow_width_in_mm/resolution;
long double arrow_lenght = arrow_lenght_in_mm/resolution;

inline void draw_arc(std::ofstream& output,
                     long double src_x, long double src_y, 
                     long double tgt_x, long double tgt_y,
                     bool is_dashed, bool with_arrow, [[maybe_unused]] bool thick = true)
{
  if (is_dashed)
    output << "\\dashline{" << dash_len << "}(";
  else
    output << "\\drawline(";
  output << src_x << "," << YPIC(src_y) << ")("
	 << tgt_x << "," << YPIC(tgt_y) << ")";

  if (not with_arrow)
    return;

  long double l = sqrt(arrow_width*arrow_width + arrow_lenght*arrow_lenght);
  long double tetha = atan2(arrow_width, arrow_lenght);
  long double phi = atan( std::fabs( (tgt_y - src_y)/(tgt_x - src_x) ) );

  long double dx1 = l*cos(phi - tetha);
  long double dy1 = l*sin(phi - tetha);

  long double dx2 = l*sin(M_PI_2 - (phi + tetha));
  long double dy2 = l*cos(M_PI_2 - (phi + tetha));

  if (tgt_x > src_x)
    {
      dx1 = -dx1;
      dx2 = -dx2;
    }

  if (tgt_y > src_y)
    {
      dy1 = -dy1;
      dy2 = -dy2;
    }

  output << std::endl
	 << "\\path(" << tgt_x << "," << YPIC(tgt_y) << ")(" 
	 << tgt_x + dx1 << "," << YPIC(tgt_y + dy1) << ")" << std::endl
     	 << "\\path(" << tgt_x << "," << YPIC(tgt_y) << ")(" 
 	 << tgt_x + dx2 << "," << YPIC(tgt_y + dy2) << ")" << std::endl << std::endl;
}

inline void draw_arc_tikz(std::ofstream& output,
                          long double src_x, long double src_y, 
                          long double tgt_x, long double tgt_y,
                          bool is_dashed, bool with_arrow, [[maybe_unused]] bool thick = true)
{
  if (is_dashed)
    output << "\\draw[dashed] (";
  else
    output << "\\draw (";
  output << src_x << "mm," << YPIC(src_y) << "mm) -- ("
	 << tgt_x << "mm," << YPIC(tgt_y) << "mm) ;";

  if (not with_arrow)
    return;

  long double l = sqrt(arrow_width*arrow_width + arrow_lenght*arrow_lenght);
  long double tetha = atan2(arrow_width, arrow_lenght);
  long double phi = atan( std::fabs( (tgt_y - src_y)/(tgt_x - src_x) ) );

  long double dx1 = l*cos(phi - tetha);
  long double dy1 = l*sin(phi - tetha);

  long double dx2 = l*sin(M_PI_2 - (phi + tetha));
  long double dy2 = l*cos(M_PI_2 - (phi + tetha));

  if (tgt_x > src_x)
    {
      dx1 = -dx1;
      dx2 = -dx2;
    }

  if (tgt_y > src_y)
    {
      dy1 = -dy1;
      dy2 = -dy2;
    }

  output << std::endl
	 << "\\draw (" << tgt_x << "," << YPIC(tgt_y) << ") -- (" 
	 << tgt_x + dx1 << "mm," << YPIC(tgt_y + dy1) << "mm) ; " << std::endl
     	 << "\\draw (" << tgt_x << "mm," << YPIC(tgt_y) << "mm) -- (" 
 	 << tgt_x + dx2 << "mm," << YPIC(tgt_y + dy2) << "mm) ; " << std::endl << std::endl;
}

/*
  Esta rutina estima el offset x donde se debe colocar la cadena str
  en una ventana de ancho window_size */
inline long double center_string(const std::string& str, [[maybe_unused]] long double window_size)
{
  return compute_true_len(str)*font_width();
}


/* calcula la longitud de la cadena en puntos segun la resolucion */
inline long double string_width(const std::string& str)
{
  return compute_true_len(str)*font_width();
}


inline int compute_section(long double x1, long double y1, 
                           long double x2, long double y2)
{
  if (x2 > x1 and y2 > y1)
    return 0;

  if (x2 < x1 and y2 > y1)
    return 1;

  // en este punto y2 < y1 

  return x2 > x1 ? 3 : 2;
}


/* Dados dos puntos (sx,sy) y (tx,ty) calcula el punto (mx,my) situado
   perpendicularmente sobre la recta */
inline void
compute_mid_point_line(const long double & sx, const long double & sy, 
                       const long double & tx, const long double & ty,
                       const long double & d, 
                       [[maybe_unused]] const bool & left, 
                       long double & mx, long double & my)
{
  const long double lx = fabsl(tx - sx);
  const long double ly = fabsl(ty - sy);

  const long double h = sqrtl(lx*lx + ly*ly); // longitud del segmento
					      // (sx,sy) --> (tx,ty) 

  const long double h2 = h/2; // mitad del segmento

  const long double alpha = atanl(ly/lx); // ángulo del segmento
						 // respecto al eje x

  const long double beta = atanl(d/h2); // ángulo entre el segmento
                                           // y la recta que estaría
					   // entre (sx,sy) y (mx,my)

  const long double hp = sqrtl(d*d + h2*h2); // longitud del segmento
					     // (sx,sy) --> (mx,my) 

  const long double ab = alpha + beta; // ángulo del segmento
				       // (sx,sy)-->(mx,my) respecto al
				       // eje x

  const long double dx = hp*cosl(ab);
  const long double dy = hp*sinl(ab);

      // coordenadas del punto normalizadas a un segmento apuntando
      // hacia el noroeste
  mx = tx - dx;
  my = dy;
}


# endif // TREEPIC_UTILS_H
