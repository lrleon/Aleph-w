
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
  @file treepic_utils.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TREEPIC_UTILS_H
# define TREEPIC_UTILS_H

# include <iostream>
# include <fstream>
# include <aleph.H>

# include <argp.h>
# include <string.h>
# include <stdio.h>
# include <math.h>
# include <stdlib.h>

using namespace std;

extern const long double sin_45;
extern const long double cos_45;

extern string input_file_name;
extern string output_file_name;


extern const char * const font_wrapper; 
extern const char * fill_type;

extern long double resolution;
extern long double font_width_in_mm; 
extern long double font_height_in_mm; 
extern long double font_width_in_points;

extern long double resolution; /* Resolution in millimeters */
extern double v_size;

    /* Extern bool tiny_keys; */

long double font_width();
long double font_height();

/** 
  @brief Calculates the points of intersection between a line and an ellipse

  @details The line passes through the center point of the ellipse.
  
  lx0, ly0 : Ellipse center point and the 1st point on the line
  lx1, ly1 : 2nd Line point
  a,b ellipse parameters (x - lx0)^2/a^2 + (y - ly0)^2/b^2

  dx, dy are the differences in x and y of the points of intersection
*/
    void 
intersection_ellipse_line(long double lx0, /* Line points */
			  long double ly0, 
			  long double lx1, 
			  long double ly1, 
			  long double a, /* Ellipse parameters (centro lx0,ly0) */
			  long double b, 
			  long double & dx, 
			  long double & dy); /* Differences from the center of the ellipse */ 


    void 
intersection_line_line(long double lx1, /* 1st line points */
		       long double ly1, 
		       long double lx2, 
		       long double ly2, 
		       long double rx1, /* 2nd line points */
		       long double ry1, 
		       long double rx2, 
		       long double ry2,
		       long double & x,
		       long double & y);


long double distance_between_points(long double x1, 
				    long double y1, 
				    long double x2,
				    long double y2);

/** 
  @brief Calculates the points of intersection between a line and a rectangle.

  @details The line passes through the center point of the rectangle.

  lx0, ly0 : Rectangle center point and the 1st point on the line
  lx1, ly1 : 2nd point on the line
  a,b horizontal and vertical radii of rectangle

  dx, dy are the differences in x and y of the points of intersection
*/
    void 
intersection_rectangle_line(long double lx0, /* Line points */
			    long double ly0, 
			    long double lx1, 
			    long double ly1, 
			    long double a, /* Horizontal radius (center lx0,ly0) */
			    long double b, /* Vertical radius */
			    long double & dx, 
			    long double & dy); /* Differences from the rectangle center */ 

extern bool flip_y;

inline long double YPIC(long double y) 
{ 
  return flip_y ? y : v_size - y; 
}


size_t compute_true_len(const string & str);


void put_string(ofstream& output,
		const long double& x, const long double& y,
		const string & comment, const string & str);

void put_string_tkiz(ofstream& output,
		     const long double& x, const long double& y,
		     const string & comment, const string & str);

extern long double arrow_width;
extern long double arrow_lenght;
extern bool with_arrow;
extern long double dash_len;

void draw_arc(ofstream& output,
	      long double src_x, long double src_y, 
	      long double tgt_x, long double tgt_y,
	      bool is_dashed, bool with_arrow, bool thick = true);

void draw_arc_tikz(ofstream& output,
		   long double src_x, long double src_y, 
		   long double tgt_x, long double tgt_y,
		   bool is_dashed, bool with_arrow, bool thick = true);


/** 
  @brief This routine estimates the offset x where you should place the string str
  window_size a window width. 
*/
long double center_string(const string& str, long double window_size);


/** 
  @brief Calculate the length of the string in points according to the resolution.
*/
long double string_width(const string& str);


int compute_section(long double x1, long double y1, 
		    long double x2, long double y2);


/** 
  @brief Given the points (sx,sy) y (tx,ty) calculates the points (mx,my) 
  located perpendicular to the line. 
*/
void compute_mid_point_line(const long double & sx, const long double & sy, 
			    const long double & tx, const long double & ty,
			    const long double & d, 
			    const bool &        left, 
			    long double & mx, long double & my); 


# endif // TREEPIC_UTILS_H
