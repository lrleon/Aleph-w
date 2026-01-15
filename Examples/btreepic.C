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

/**
 * @file btreepic.C
 * @brief Generate LaTeX/eepic pictures of binary trees
 *
 * This utility program reads a binary tree specification from a file and
 * generates high-quality LaTeX code for visualizing binary trees using the
 * eepic package. It's designed for creating publication-quality tree diagrams
 * for papers, presentations, and educational materials.
 *
 * ## Purpose
 *
 * `btreepic` is essential for:
 * - **Academic papers**: Professional tree visualizations
 * - **Educational materials**: Clear, readable tree diagrams
 * - **Documentation**: Illustrating tree algorithms and structures
 * - **Presentations**: High-quality slides with tree diagrams
 *
 * ## Key Features
 *
 * ### Tree Structure
 * - **Binary trees**: Standard left/right child structure
 * - **Node positioning**: Automatic layout with configurable spacing
 * - **Edge drawing**: Lines connecting parents to children
 * - **Thread visualization**: Support for threaded binary trees
 *
 * ### Customization
 * - **Node shapes**: Circles, ellipses, rectangles
 * - **Colors**: Customizable node and edge colors
 * - **Labels**: Text labels for nodes and edges
 * - **Spacing**: Configurable horizontal and vertical spacing
 * - **Sizing**: Adjustable node sizes and picture dimensions
 *
### Output Format
 * - **LaTeX/eepic**: High-quality vector graphics
 * - **Scalable**: Vector format, scales without quality loss
 * - **Publication-ready**: Professional appearance
 *
 * ## Input Format (DSL)
 *
 * The input file uses a domain-specific language (DSL) with commands:
 *
 * ### Basic Commands
 * - `NODE <id> <key>`: Create a node with identifier and key value
 * - `EDGE <parent> <child>`: Connect parent to child
 * - `LEFT <parent> <child>`: Set left child
 * - `RIGHT <parent> <child>`: Set right child
 *
 * ### Styling Commands
 * - `COLOR <id> <color>`: Set node color
 * - `SHAPE <id> <shape>`: Set node shape (circle, ellipse, rectangle)
 * - `HRADIO <id> <value>`: Set horizontal radius for specific node
 * - `VRADIO <id> <value>`: Set vertical radius for specific node
 *
 * ### Advanced Features
 * - `THREAD <from> <to>`: Draw thread connection (for threaded trees)
 * - `TAG <id> <position> <text>`: Add text tag to node
 * - `SHADOW_NODE <id>`: Draw node with shadow effect
 * - `DASHED_ARC <from> <to>`: Draw dashed edge
 *
 * ## Usage
 *
 * ```bash
 * # Generate LaTeX from tree specification
 * btreepic input.tree > output.tex
 *
 * # Compile LaTeX to PDF
 * pdflatex output.tex
 * ```
 *
 * ## Example Input File
 *
 * ```
 * NODE 1 10
 * NODE 2 5
 * NODE 3 15
 * LEFT 1 2
 * RIGHT 1 3
 * COLOR 1 blue
 * ```
 *
 * ## Output
 *
 * Generates LaTeX code that can be compiled to PDF. The output uses the
 * eepic package for drawing, ensuring compatibility with standard LaTeX
 * distributions.
 *
 * ## Related Tools
 *
 * - **write_tree.C**: Generates `.Tree` files for btreepic
 * - **writeBalance.C**: Creates balanced tree visualizations
 * - **writeHeap.C**: Visualizes heap structures
 * - **ntreepic.C**: For n-ary (general) trees
 *
 * @see ntreepic.C N-ary tree visualization tool
 * @see graphpic.C Graph visualization tool
 * @see treepic_utils.H Shared utilities for tree visualization
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */
    
# include <stdio.h>
# include <math.h>
# include <stdlib.h>
# include <cstring>
# include <fstream>
# include <iostream>
# include <aleph.H>
# include <tpl_dynDlist.H>
# include <tpl_binNode.H>
# include <tpl_binNodeUtils.H>
# include <tpl_dynArray.H> 
# include <tpl_sort_utils.H>

# include <argp.h>

# include "parse_utils.H"
# include "treepic_utils.H"

using namespace std;

/* TODO:: opcion THREAD para dibujar una arco threads parábola entre 2
   nodos (esta cas lista)

   TODO: opcion AUX-KEY 20 "cadena" para "cadena" en nodo 20

   TODO: opcion ARROW para dibujar flechas -interfza aun no definida

   TODO: opcion dick y luka para arboles de lukasiewicks y dick, sus
   codigos y dibujado de caminos de catalan

   TODO: opcion para proyeccion infija

   TODO: opcion para tipos de letra

   TODO: opcion que reenicie archivo .btreepic
*/

extern bool tiny_keys;

/* Estos son los valores de distancias */
long double hr = 10;  // radio horizontal de la elipse
long double vr = 10;  // radio vertical de la elipse
long double hd = 2*hr;  // diametro horizontal de la elipse
long double vd = 2*vr;  // diametro vertical de la elipse
long double w = 20; // separacion horizontal entre centros
long double h = 35; // separacion vertical entre centros
long double h_size = 110; /* longitud horizontal de picture */
double v_size = 190; /* longitud vertical de picture */
long double x_offset = 0; // offset horizontal para etiqueta principal
long double y_offset = 0; // offset vertical etiqueta principal
long double x_aux_offset = 0; // offset horizontal para etiqueta auxiliar
long double y_aux_offset = 0; // offset vertical para etiqueta auxiliar
long double x_picture_offset = 0; // offset horizontal para dibujo
long double y_picture_offset = 0; // offset vertical para dibujo


enum Token_Type { NUMBER, STRING, START_PREFIX, START_INFIX, START_KEY, 
		  START_AUX, SHADOW_NODE, WITHOUT_NODE, TAG, ARC, DASHED_ARC,
		  SCRATCH, SPLIT, KEY, XOFFSET, YOFFSET, TRIANGLE, 
		  TRIANGLEH, WITHOUT_ARC, COMMENT, RECTANGLE, PARRECTANGLE,
		  DISTANCE, LEFT, RIGHT, THREAD,
		  //Tag options
		  NORTH,     // N
		  SOUTH,     // S
		  EAST,      // E
		  WEST,      // W
		  NORTH_EAST, // NE
		  NORTH_WEST, // NW
		  SOUTH_EAST, // SE
		  SOUTH_WEST, // SW
		  END_FILE, INVALID };


typedef Token_Type Tag_Option;


/* Una tag es una etiqueta que se puede colocar externamente a un
   nodo. Esta struct describe tal etiqueta */
struct Tag_Data
{
  string      tag; // el string a escribir
  Tag_Option  tag_option; // orientacion de tag en sentido horario
  long double x_offset;
  long double y_offset;

  Tag_Data() { /* empty */ }

  Tag_Data(const string & str, Tag_Option option) 
    : tag(str), tag_option(option)
  {
    // empty
  }
};


template <class Key> class EepicNode; /* Necesaria por punteros extras
					 dentro EepicNode_Data */ 

struct Arc_Desc
{
  EepicNode<long> * target_node;
  bool              is_dashed;

  Arc_Desc() : target_node(NULL), is_dashed(false) { /* empty */ }
};


struct Arc_Data
{
  long target_node;
  bool is_dashed;

  Arc_Data() : is_dashed(false) { /* empty */ }
};


struct Thread_Desc
{
  EepicNode<long> * target_node;
  bool              is_dashed;

  Thread_Desc() : target_node(NULL), is_dashed(false) { /* empty */ }
};


struct Thread_Data
{
  long target_node;
  bool is_dashed;

  Thread_Data() : is_dashed(false) { /* empty */ }
};


struct Split_Data
{
  string upper_string;
  string lower_string;
};


struct Line_Distance_Data
{
  string str;
  Token_Type orientation;
  long double xoffset;
  long double yoffset;

  bool is_left() const { return orientation == LEFT; }
};


class EepicNode_Data
{
private:

  int count;
  int level;

  /* coordenadas x,y en pixels */
  long double x;
  long double y;

  long double xoffset;
  long double yoffset;

  long double triangle_height;

  long double rectangle_height;

  bool external_node;

  string key_string;
  string aux_string;
  bool shadow;
  bool without_node;
  bool scratch;
  bool with_arc;

  bool dash_llink;
  bool dash_rlink;

  EepicNode<long>* prev;
  EepicNode<long>* succ;

  Line_Distance_Data line_distance_data;

  DynDlist<Tag_Data> tag_list;

  DynDlist<Arc_Desc> arc_list;

  DynDlist<Thread_Desc> thread_list;

public:

  EepicNode_Data() : 
    xoffset(0.0), yoffset(0.0), 
    triangle_height(0.0), rectangle_height(0.0),
    external_node(false), shadow(false), without_node(false), 
    scratch(false), with_arc(true),
    dash_llink(false), dash_rlink(false),
    prev(NULL), succ(NULL)
 { /* empty */ }

  void reset() { /* empty */ }

  int& getCount() { return count; }

  int& get_level() { return level; }

  long double& get_x() { return x; }

  long double& get_y() { return y; }

  long double& get_triangle_height() { return triangle_height; }

  long double& get_rectangle_height() { return rectangle_height; }

  long double& get_xoffset() { return xoffset; }

  long double& get_yoffset() { return yoffset; }

  bool& get_external_node() { return external_node; }

  bool is_external() const { return external_node; }

  bool is_triangle() const { return triangle_height != 0.0; }

  bool is_rectangle() const { return rectangle_height != 0.0; }

  string & get_key_string() { return key_string; }

  string & get_aux_string() { return aux_string; }

  Line_Distance_Data& get_line_distance_data() { return line_distance_data; }

  bool & get_shadow() { return shadow; }

  bool & get_without_node() { return without_node; }

  bool & get_scratch() { return scratch; }

  bool & get_with_arc() { return with_arc; }

  bool & get_dash_llink() { return dash_llink; }

  bool & get_dash_rlink() { return dash_rlink; }

  EepicNode<long>*& get_prev() { return prev; };

  EepicNode<long>*& get_succ() { return succ; }

  DynDlist<Tag_Data> & get_tag_list() { return tag_list; }

  DynDlist<Arc_Desc> & get_arc_list() { return arc_list; }

  DynDlist<Thread_Desc> & get_thread_list() { return thread_list; }
};


# define INFIXPOS(p)    ((p)->getCount())
# define LEVEL(p)       ((p)->get_level())
# define X(p)           ((p)->get_x())
# define Y(p)           ((p)->get_y())
# define XOFFSET(p)     ((p)->get_xoffset())
# define YOFFSET(p)     ((p)->get_yoffset())
# define TRIANGLEH(p)   ((p)->get_triangle_height())
# define RECTANGLEH(p)  ((p)->get_rectangle_height())
# define EXTERNAL(p)    ((p)->get_external_node())
# define ISEXTERNAL(p)  ((p)->is_external())
# define ISTRIANGLE(p)  ((p)->is_triangle())
# define DSTRING(p)     ((p)->get_distance_string())
# define LDISTANCE(p)   ((p)->get_line_distance_data())
# define ISDISTANCE(p)  ((p)->get_line_distance_data().str.size() != 0)
# define ISRECTANGLE(p) ((p)->is_rectangle())
# define STRING(p)      ((p)->get_key_string())
# define AUX(p)         ((p)->get_aux_string())
# define SHADOW(p)      ((p)->get_shadow())
# define WITHOUT(p)     ((p)->get_without_node())
# define SCRATCH(p)     ((p)->get_scratch())
# define WITHARC(p)     ((p)->get_with_arc())
# define DASHLLINK(p)   ((p)->get_dash_llink())
# define DASHRLINK(p)   ((p)->get_dash_rlink())
# define TAGLIST(p)     ((p)->get_tag_list())
# define ARCLIST(p)     ((p)->get_arc_list())
# define THREADLIST(p)  ((p)->get_thread_list())
# define SUCC(p)        ((p)->get_succ())
# define PREV(p)        ((p)->get_prev())


DECLARE_BINNODE(EepicNode, 5000, EepicNode_Data);



string command_line;


string input_file_name;
string output_file_name;


/* Estados globales */
int num_nodes = 0;


/* opciones logicas y estados de lectura del archivo de entrada */
bool verbose_mode      = true;  // Imprimir acciones reconocidas 
bool silent_mode       = true;  // No imprimir tokens vistos de archivo entrada
bool latex_header      = false; // envolver salida con un latex header
bool landscape         = false; // latex header en landscape
bool fit_mode          = false; // modo calculo automatico de tama~os */
bool draw_node_mode    = true;  // dibujar elipses
bool printing_key_mode = false; // imprimir valor de clave dentro de elipse
bool with_string_key   = false; // Hay un string en archivo como clave
bool with_string_aux   = false; // Hay un string auxiliar aparte clave
bool threaded_trees    = false; // Dibujar arbol hilado
bool dash_threaded_trees = false; // Dibujar arbol hilado
bool with_external_nodes = false; // Dibujar nodos externos
bool draw_nodes          = true;  // Dibujar elipses de nodos 

const char * parameters_file_name = "./.btreepic";


    inline
void save_parameters()
{
  ofstream output(parameters_file_name, ios::trunc);

  output << hr << " " << vr << " " << hd << " " << vd << " " << w << " " 
	 << h << " " << resolution << " " << h_size << " " 
	 << v_size << " " << x_offset << " " << y_offset << " " 
	 << x_aux_offset << " " << y_aux_offset << " " 
	 << x_picture_offset << " " << y_picture_offset; 
}


    inline
void read_parameters()
{
  ifstream input(parameters_file_name, ios::in);

  input >> hr >> vr >> hd >> vd >> w >> h >> resolution >> h_size 
	>> v_size >> x_offset >> y_offset >> x_aux_offset 
	>> y_aux_offset >> x_picture_offset >> y_picture_offset;
}


DynArray<long>   prefix_dynarray;    // recorrido prefijo
DynArray<long>   infix_dynarray;     // recorrido infijo
DynArray<string> key_print_dynarray; // claves en string en orden infijo
DynArray<string> aux_print_dynarray; // strings auxiliares en orden infijo
DynArray<long>   shadow_dynarray;    /* lista secuencial de posiciones infijas
					a ensombrecer */
DynArray<long>   without_node_dynarray; /* lista secuencial de nodos
					   que no se dibujan */  
DynArray<Tag_Data> tag_data_dynarray; /* Se colocan tags */
DynArray<long>   tag_pos_dynarray; /* posiciones infijas de tags */
DynArray<long>   source_arc_dynarray; /* posiciones infijas de origen
					 de arcos adicionales */
DynArray<Arc_Data> target_arc_dynarray; /* posiciones infijas de destino
					   de arcos adicionales */
DynArray<long> source_thread_dynarray; /* posiciones infijas de origen
					  de threads adicionales */
DynArray<Thread_Data> target_thread_dynarray; /* posiciones infijas de destino
						 de threads adicionales */
DynArray<long>   scratch_dynarray; /* posiciones infijas de nodos a
				      tachar */
DynArray<long>   split_dynarray; /* posiciones infijas de lineas de
				    particion */
DynArray<Split_Data> split_string_dynarray; /* etiquetas de lines de
					       particion */ 
DynArray<long> key_pos_dynarray; /* posiciones infijas de claves */
DynArray<string> key_string_dynarray; /* claves */

/* offsets horizontales particulares */
DynArray<long> pos_xoffset_dynarray; /* posicion infija */
DynArray<long double> xoffset_dynarray; /* valor del offset */

/* offsets horizontales particulares */
DynArray<long> pos_yoffset_dynarray; /* posicion infija */
DynArray<long double> yoffset_dynarray; /* valor del offset */

/* triangulos */
DynArray<long> pos_triangle_dynarray; /* posicion infija */
DynArray<long double> height_triangle_dynarray; /* altura del triangulo */

/* rectangulos */
DynArray<long double> height_rectangle_dynarray; /* posicion infija */
DynArray<long> pos_rectangle_dynarray; /* altura del triangulo */

/* rectangulos con tachaduras al final */
DynArray<long double> height_parrectangle_dynarray; /* posicion infija */
DynArray<long> pos_parrectangle_dynarray; /* altura del triangulo */

/* lineas de distancia */
DynArray<long> pos_distance_dynarray; /* altura del triangulo */
DynArray<Line_Distance_Data> distance_dynarray; /* posicion infija */

DynArray<long> without_arc_dynarray; // lista de nodos sin arco


# define TERMINATE(n) (save_parameters(), exit(n))


# define DYNARRAY_APPEND(array, item) (array[array.size()] = item)


using namespace Aleph;



Token_Type get_token(ifstream& input_stream);


void load_tag_option(ifstream& input_stream)
{
  DYNARRAY_APPEND(tag_pos_dynarray, load_number(input_stream));

  Tag_Data tag_data;
  tag_data.tag = load_string(input_stream); // string de tag

  Token_Type token_type = get_token(input_stream);

  if (token_type < NORTH or token_type > SOUTH_WEST)
    print_parse_error_and_exit("Invalid tag option found");

  tag_data.tag_option = token_type;

  tag_data.x_offset = load_number(input_stream); // xoffset
  tag_data.y_offset = load_number(input_stream); // yoffset

  DYNARRAY_APPEND(tag_data_dynarray, tag_data);
}


void load_arc_option(ifstream& input_stream, const Token_Type & token_type)
{
  DYNARRAY_APPEND(source_arc_dynarray, load_number(input_stream));

  Arc_Data arc_data;
  arc_data.target_node = load_number(input_stream);
  arc_data.is_dashed = token_type == ARC ? false : true; 

  DYNARRAY_APPEND(target_arc_dynarray, arc_data);
}


void load_thread_option(ifstream& input_stream)
{
  DYNARRAY_APPEND(source_thread_dynarray, load_number(input_stream));

  Thread_Data thread_data;
  thread_data.target_node = load_number(input_stream);
  thread_data.is_dashed = true; 

  DYNARRAY_APPEND(target_thread_dynarray, thread_data);
}


void load_split_option(ifstream& input_stream)
{
  DYNARRAY_APPEND(split_dynarray, load_number(input_stream));

  Split_Data split_data;

  split_data.upper_string = load_string(input_stream);
  split_data.lower_string = load_string(input_stream);

  DYNARRAY_APPEND(split_string_dynarray, split_data);
}


void load_key_option(ifstream& input_stream)
{
  long position = DYNARRAY_APPEND(key_pos_dynarray, load_number(input_stream));

  if (position >= prefix_dynarray.size())
    PRINT_ERROR("Node position greater than number of nodes in KEY option");

  DYNARRAY_APPEND(key_string_dynarray, load_string(input_stream));
}


void load_offset_option(ifstream& input_stream, 
			DynArray<long> & positions, 
			DynArray<long double>& offsets)
{
  DYNARRAY_APPEND(positions, load_number(input_stream));
  DYNARRAY_APPEND(offsets, load_number(input_stream));
}


void load_triangleh_option(ifstream& input_stream)
{
  DYNARRAY_APPEND(pos_triangle_dynarray, load_number(input_stream));
  DYNARRAY_APPEND(height_triangle_dynarray, load_number(input_stream));
}


void load_triangle_option(ifstream& input_stream)
{
  DYNARRAY_APPEND(pos_triangle_dynarray, load_number(input_stream));
  DYNARRAY_APPEND(height_triangle_dynarray, h);
}

/* esta variable memoriza la maxima altura en nodos de un
   rectangulo. El proposito es reajustar la longitud vertical del
   ambiente picture */
int max_num_nodes_rectangle = 0;

void load_rectangle_option(ifstream& input_stream)
{
  DYNARRAY_APPEND(pos_rectangle_dynarray, load_number(input_stream)); 

  long height = load_number(input_stream);

  if (height > max_num_nodes_rectangle)
    max_num_nodes_rectangle = height;

  if (height == 0)
    PRINT_ERROR("Heigth in nodes cannot be zero");

  DYNARRAY_APPEND(height_rectangle_dynarray, height*vd); 
}


void load_parrectangle_option(ifstream& input_stream)
{
  DYNARRAY_APPEND(pos_parrectangle_dynarray, load_number(input_stream)); 

  long height = load_number(input_stream);

  if (height > max_num_nodes_rectangle)
    max_num_nodes_rectangle = height;

  if (height == 0)
    PRINT_ERROR("Heigth in nodes cannot be zero");

  DYNARRAY_APPEND(height_parrectangle_dynarray, height*vd); 
}

void load_distance_option(ifstream& input_stream)
{
  DYNARRAY_APPEND(pos_distance_dynarray, load_number(input_stream));

  Line_Distance_Data line_distance_data;
  
  line_distance_data.str = load_string(input_stream);

  Token_Type token_type = get_token(input_stream);

  if (token_type != LEFT and token_type != RIGHT)
    PRINT_ERROR("Invalid orientation in DISTANCE option");

  line_distance_data.orientation = token_type;
  line_distance_data.xoffset = load_number(input_stream);
  line_distance_data.yoffset = load_number(input_stream);

  DYNARRAY_APPEND(distance_dynarray, line_distance_data); 
}


Token_Type get_token(ifstream& input_stream)  
{
  int current_char;

  char buffer[Buffer_Size];
  char * start_address = buffer;
  char * end_address   = buffer + Buffer_Size;

  init_token_scanning();
  skip_white_spaces(input_stream);

  try { current_char = read_char_from_stream(input_stream); }
  catch (const std::out_of_range&) { return END_FILE; }

  if (current_char == EOF)
    return END_FILE;

  if (not isprint(current_char))
    return INVALID;

  if (isdigit(current_char) or current_char == '-')
    {
      if (current_char == '-')
	{
	  put_char_in_buffer(start_address, end_address, current_char);
	  current_char = read_char_from_stream(input_stream);
	  if (not isdigit(current_char))
	    return INVALID;
	}
      do 
	{
	  put_char_in_buffer(start_address, end_address, current_char);
	  current_char = read_char_from_stream(input_stream);
	}
      while (current_char != EOF and isdigit(current_char));

      close_token_scanning(buffer, start_address, end_address);
      return NUMBER;
    }

  if (current_char == '\"') /* string delimitado entre comillas */
    while (true)
      {
	current_char = read_char_from_stream(input_stream);
	if (current_char == '\"')
	  {
	    close_token_scanning(buffer, start_address, end_address);
	    return STRING;
	  }
	if (current_char == EOF or current_char == '\n')
	  return INVALID;

	put_char_in_buffer(start_address, end_address, current_char);
      } 

  if (current_char == '%')
    {     /* comentario */
      while (read_char_from_stream(input_stream) != '\n') { /* nothing */ }

      return COMMENT;
    }

  do /* delimita cadena caracteres separada por blancos */
    {
      put_char_in_buffer(start_address, end_address, current_char);
      current_char = read_char_from_stream(input_stream);
    }
  while (isgraph(current_char));

  close_token_scanning(buffer, start_address, end_address);

  if (strcasecmp(buffer, "START-PREFIX") == 0 or
      strcasecmp(buffer, "PREFIX") == 0 or
      strcasecmp(buffer, "START-PREORDER") == 0 or
      strcasecmp(buffer, "PREORDER") == 0) 
    return START_PREFIX;

  if (strcasecmp(buffer, "START-INFIX") == 0 or
      strcasecmp(buffer, "START-INORDER") == 0 or
      strcasecmp(buffer, "INORDER") == 0 or
      strcasecmp(buffer, "INFIX") == 0
      )
    return START_INFIX;

  if (strcasecmp(buffer, "START-KEY") == 0)
    return START_KEY;

  if (strcasecmp(buffer, "START-AUX") == 0)
    return START_AUX;

  if (strcasecmp(buffer, "START-SHADOW") == 0 or
      strcasecmp(buffer, "SHADOW") == 0)
    return SHADOW_NODE;

  if (strcasecmp(buffer, "WITHOUT-NODE") == 0)
    return WITHOUT_NODE;

  if (strcasecmp(buffer, "TAG") == 0)
    return TAG;

  if (strcasecmp(buffer, "ARC") == 0)
    return ARC;

  if (strcasecmp(buffer, "DASHED-ARC") == 0)
    return DASHED_ARC;

  if (strcasecmp(buffer, "SCRATCH") == 0)
    return SCRATCH;

  if (strcasecmp(buffer, "SPLIT") == 0)
    return SPLIT;

  if (strcasecmp(buffer, "KEY") == 0)
    return Token_Type::KEY;

  if (strcasecmp(buffer, "XOFFSET") == 0)
    return XOFFSET;

  if (strcasecmp(buffer, "YOFFSET") == 0)
    return YOFFSET;

  if (strcasecmp(buffer, "TRIANGLE") == 0)
    return TRIANGLE;

  if (strcasecmp(buffer, "TRIANGLEH") == 0)
    return TRIANGLEH;

  if (strcasecmp(buffer, "WITHOUT-ARC") == 0)
    return WITHOUT_ARC;

  if (strcasecmp(buffer, "RECTANGLE") == 0)
    return RECTANGLE;

  if (strcasecmp(buffer, "PARRECTANGLE") == 0)
    return PARRECTANGLE;

  if (strcasecmp(buffer, "DISTANCE") == 0)
    return DISTANCE;

  if (strcasecmp(buffer, "THREAD") == 0)
    return THREAD;

  if (strcasecmp(buffer, "L") == 0)
    return LEFT;

  if (strcasecmp(buffer, "R") == 0)
    return RIGHT;

  if (strcasecmp(buffer, "N") == 0)
    return NORTH;

  if (strcasecmp(buffer, "S") == 0)
    return SOUTH;

  if (strcasecmp(buffer, "E") == 0)
    return EAST;

  if (strcasecmp(buffer, "W") == 0)
    return WEST;

  if (strcasecmp(buffer, "NE") == 0)
    return NORTH_EAST;

  if (strcasecmp(buffer, "NW") == 0)
    return NORTH_WEST;

  if (strcasecmp(buffer, "SE") == 0)
    return SOUTH_EAST;

  if (strcasecmp(buffer, "SW") == 0)
    return SOUTH_WEST;

  return STRING;
}


enum Parsing_State { PREFIX, /* Fase obligatoria */
		     INFIX,  /* El resto de las fases son opcionales */
		     KEYS,    /* Lectura de valores de claves principales */
		     AUX,    /* Lectura de valores de claves auxiliares */
		     SHADOW, /* Ensombrecer nodo */
		     WITHOUT /* No dibujar elipse */ 
};


void assign_key(EepicNode<long> * p, int, int position)
{
  STRING(p) = key_print_dynarray[position];
}


void assign_aux(EepicNode<long> * p, int, int position)
{
  AUX(p) = aux_print_dynarray[position];
}


void assign_shadow(EepicNode<long> * p, int, int position) 
{             
  if (sequential_search<long>(shadow_dynarray, position,
			      0, shadow_dynarray.size() - 1) >= 0)
    {
      SHADOW(p) = true;
      WITHOUT(p) = false;
    }
}

void reassign_key(EepicNode<long> * p, int, int position) 
{       
  int found_index = 
    sequential_search<long>(key_pos_dynarray, position,
			    0, key_pos_dynarray.size() - 1);
  if (found_index >= 0)
    STRING(p) = key_string_dynarray[found_index];
}


void assign_without_node(EepicNode<long> * p, int, int position)
{
  if (sequential_search<long>(without_node_dynarray, position,
			      0, without_node_dynarray.size() - 1) >= 0)
    WITHOUT(p) = true;
}


void assign_tag(EepicNode<long> * p, int, int position)
{
  int low_index = 0;
  int found_index;

  do /* busca todas las ocurrencias del valor node_index en
	tag_pos_dynarray */ 
    {
      found_index = 
	sequential_search<long>(tag_pos_dynarray, position, low_index, 
				tag_pos_dynarray.size() - 1);
      if (found_index >= 0)
	TAGLIST(p).append(tag_data_dynarray[found_index]);

      low_index = found_index + 1;
    } 
  while (found_index >= 0);
}


/* 
   busca el i-esimo nodo infijo asumiendo que cada nodo posee un campo
   INFIX_POS que almacena su posicion infija. Asume que el arbol vacio
   es NULL -no hay nulo centinela-
*/
    template <class Node> inline
Node * select(Node * root, const int & i)
{
  while (root != NULL)
    {
      if (INFIXPOS(root) == i)
	return root;

      if (i < INFIXPOS(root))
	root = LLINK(root);
      else
	root = RLINK(root);
    }
  return NULL;
}


/* recorre el arbol en prefijo y asigna los arcos almacenados en los
   arreglos source_arc_dynarray y target_arc_dynarray */
    inline
void assign_arcs(EepicNode<long> * root, EepicNode<long> * p)
{
  if (p == NULL)
    return;

  int low_index = 0;
  int found_index;

  do /* busca todas las ocurrencias del valor node_index en
	source_arc_dynarray */ 
    {
      found_index = 
	sequential_search<long>(source_arc_dynarray, INFIXPOS(p), low_index, 
				source_arc_dynarray.size() - 1);
      if (found_index >= 0)
	{
	  Arc_Data arc_data = target_arc_dynarray[found_index];
	  Arc_Desc arc_desc;
	  arc_desc.is_dashed = arc_data.is_dashed;
	  arc_desc.target_node = select(root, arc_data.target_node);
	  ARCLIST(p).append(arc_desc);
	}

      low_index = found_index + 1;
    } 
  while (found_index >= 0);

  do /* busca todas las ocurrencias del valor node_index en
	source_thread_dynarray */ 
    {
      found_index = 
	sequential_search<long>(source_thread_dynarray, INFIXPOS(p), 
				low_index, source_thread_dynarray.size() - 1);
      if (found_index >= 0)
	{
	  Thread_Data thread_data = target_thread_dynarray[found_index];
	  Thread_Desc thread_desc;
	  thread_desc.is_dashed = thread_data.is_dashed;
	  thread_desc.target_node = select(root, thread_data.target_node);
	  THREADLIST(p).append(thread_desc);
	}

      low_index = found_index + 1;
    } 
  while (found_index >= 0);

  assign_arcs(root, LLINK(p));
  assign_arcs(root, RLINK(p));
}


void assign_external_nodes(EepicNode<long> * p)
{ 
  {
    EepicNode<long> *& l = LLINK(p);

    if (l == NULL)
      {                              
	l =  new EepicNode<long>;
	EXTERNAL(l) = true;
      }
    else
      assign_external_nodes(l);
  }
  {
    EepicNode<long> *& r = RLINK(p);

    if (r == NULL)
      {
	r = new EepicNode<long>;
	EXTERNAL(r) = true;
      }
    else
      assign_external_nodes(r);
  }
}


void assign_scratch(EepicNode<long> * p, int, int position)
{
  if (sequential_search<long>(scratch_dynarray, position,
			      0, scratch_dynarray.size() - 1) >= 0)
    SCRATCH(p) = true;
}


void assign_xoffset(EepicNode<long> * p, int, int position)
{
  int found_index = 
    sequential_search<long>(pos_xoffset_dynarray, position,
			    0, pos_xoffset_dynarray.size() - 1);
  if (found_index >= 0)
    XOFFSET(p) = xoffset_dynarray[found_index];
}


void assign_yoffset(EepicNode<long> * p, int, int position)
{
  int found_index = 
    sequential_search<long>(pos_yoffset_dynarray, position,
			    0, pos_yoffset_dynarray.size() - 1);
  if (found_index >= 0)
    YOFFSET(p) = yoffset_dynarray[found_index];
}


void assign_triangle(EepicNode<long> * p, int, int position)
{
  int found_index = 
    sequential_search<long>(pos_triangle_dynarray, position,
			    0, pos_triangle_dynarray.size() - 1);
  if (found_index >= 0)
    {
      if (not (LLINK(p) == NULL and RLINK(p) == NULL))
	PRINT_ERROR("Triagle on %d th node is not a leaf", INFIXPOS(p));

      TRIANGLEH(p) = height_triangle_dynarray[found_index];
    }
}


void assign_rectangle(EepicNode<long> * p, int, int position)
{
  int found_index = 
    sequential_search<long>(pos_rectangle_dynarray, position,
			    0, pos_rectangle_dynarray.size() - 1);
  if (found_index >= 0)
    {
      if (not (LLINK(p) == NULL and RLINK(p) == NULL))
	PRINT_ERROR("Rectangle on %d th node is not a leaf", INFIXPOS(p));

      RECTANGLEH(p) = height_rectangle_dynarray[found_index];
    }
}


void assign_parrectangle(EepicNode<long> * p, int, int position)
{
  int found_index = 
    sequential_search<long>(pos_parrectangle_dynarray, position,
			    0, pos_parrectangle_dynarray.size() - 1);
  if (found_index >= 0)
    {
      if (not (LLINK(p) == NULL and RLINK(p) == NULL))
	PRINT_ERROR("Rectangle on %d th node is not a leaf", INFIXPOS(p));

      RECTANGLEH(p) = height_parrectangle_dynarray[found_index];
      SCRATCH(p) = true;
    }
}



void assign_distance(EepicNode<long> * p, int, int position)
{
  int found_index = 
    sequential_search<long>(pos_distance_dynarray, position,
			    0, pos_distance_dynarray.size() - 1);
  if (found_index >= 0)
    {
      Line_Distance_Data line_distance_data = distance_dynarray[found_index];
      if (line_distance_data.orientation == LEFT and LLINK(p) != NULL)
	PRINT_ERROR("Distance line on %d th node has a left branch",
		    INFIXPOS(p));
      if (line_distance_data.orientation == RIGHT and RLINK(p) != NULL)
	PRINT_ERROR("Distance line on %d th node has a right branch",
		    INFIXPOS(p));

      LDISTANCE(p) = line_distance_data;
    }
}


void assign_without_arc(EepicNode<long> * p, int, int position)
{
  int found_index = 
    sequential_search<long>(without_arc_dynarray, position,
			    0, without_arc_dynarray.size() - 1);
  if (found_index >= 0)
    WITHARC(p) = false;
}


/*
  Asigna posicion infija a cada nodo. Se usa con con rutina inOrderRec
  y debe asegurarse que node_index este iniciado en cero antes de
  llamar a inOrderRec
*/
void assign_pos_and_level(EepicNode<long> * p, int level, int position)
{ 
  INFIXPOS(p) = position;
  X(p) = hr + position*w;
  LEVEL(p) = level;
  Y(p)     = level*h + vr;
}


    inline
void thread_tree(EepicNode<long> * p)
{
  {
    EepicNode<long> * prev = LLINK(p);

    if (prev != NULL)
      {
	while (RLINK(prev) != NULL)
	  prev = RLINK(prev);
      
	SUCC(prev) = p;

	thread_tree(LLINK(p));
      }
  }
  {
    EepicNode<long> * succ = RLINK(p);

    if (succ != NULL)
      {
	while (LLINK(succ) != NULL)
	  succ = LLINK(succ);
      
	PREV(succ) = p;

	thread_tree(RLINK(p));
      }
  }
}


void file_to_dynarrays(const char * file_name)
{
  try
    {
      ifstream input_stream(file_name, ios::in);

      if (not input_stream)
	AH_ERROR("%s file does not exist", input_file_name.c_str());

      Token_Type token_type;
      Parsing_State parsing_state = PREFIX; 

      while (true)
	{
	  token_type = get_token(input_stream);

	  if (not silent_mode and token_type != END_FILE)
	    cout << token_instance << " ";

	  switch (token_type)
	    {
	    case INVALID: PRINT_ERROR("Found an invalid token");
	    case END_FILE: 
	      return;

	    case COMMENT: break;

	    case STRING:
	      switch (parsing_state)
		{
		case KEYS:
		  DYNARRAY_APPEND(key_print_dynarray, token_instance);
		  break;
		case AUX:
		  DYNARRAY_APPEND(aux_print_dynarray, token_instance);
		  break;
		default: PRINT_ERROR("Found an string in invalid mode (%ld)", 
				     parsing_state);
		} 
	      break;

	    case NUMBER:
	      switch (parsing_state)
		{
		case PREFIX:
		  DYNARRAY_APPEND(key_print_dynarray, token_instance);
		  DYNARRAY_APPEND(prefix_dynarray, 
				  atoi(token_instance.c_str()));
		  break;
		case INFIX:
		  DYNARRAY_APPEND(infix_dynarray, 
				  atoi(token_instance.c_str()));
		  break;
		case KEYS:
		  DYNARRAY_APPEND(key_print_dynarray, token_instance);
		  break;
		case AUX:
		  DYNARRAY_APPEND(aux_print_dynarray, token_instance);
		  break;
		case SHADOW:
		  DYNARRAY_APPEND(shadow_dynarray,
				  atoi(token_instance.c_str()));
		  break;
		case WITHOUT:
		  DYNARRAY_APPEND(without_node_dynarray, 
				  atoi(token_instance.c_str()));
		  break;
		default: AH_ERROR("Found an integer in invalid mode (%ld)", 
			       parsing_state);
		}
	      break;

	    case START_PREFIX:
	      parsing_state = PREFIX;
	      break;

	    case START_INFIX:
	      parsing_state = INFIX;
	      break;

	    case START_KEY: 
	      parsing_state = KEYS;
	      if (key_print_dynarray.size() > 0)
		key_print_dynarray.cut();
	      with_string_key = true;
	      printing_key_mode = true;
	      break;

	    case START_AUX:
	      parsing_state = AUX;
	      with_string_aux = true;
	      break;

	    case SHADOW_NODE:
	      parsing_state = SHADOW;
	      break;

	    case WITHOUT_NODE:
	      parsing_state = WITHOUT;
	      break;

	    case TAG:
	      load_tag_option(input_stream);
	      break;

	    case ARC:
	    case DASHED_ARC:
	      load_arc_option(input_stream, token_type);
	      break; 
	      
	    case THREAD:
	      load_thread_option(input_stream);
	      break;

	    case SCRATCH:
	      DYNARRAY_APPEND(scratch_dynarray, load_number(input_stream));
	      break;

	    case SPLIT:
	      load_split_option(input_stream);
	      break;

	    case Token_Type::KEY:
	      load_key_option(input_stream);
	      break;

	    case XOFFSET:
	      load_offset_option(input_stream, pos_xoffset_dynarray,
				 xoffset_dynarray);
	      break;

	    case YOFFSET:
	      load_offset_option(input_stream, pos_yoffset_dynarray,
				 yoffset_dynarray);
	      break;

	    case TRIANGLE:
	      load_triangle_option(input_stream);
	      break;

	    case TRIANGLEH:
	      load_triangleh_option(input_stream);
	      break;

	    case RECTANGLE:
	      load_rectangle_option(input_stream);
	      break;

	    case PARRECTANGLE:
	      load_parrectangle_option(input_stream);
	      break;

	    case DISTANCE:
	      load_distance_option(input_stream);
	      break;

	    case WITHOUT_ARC:
	      DYNARRAY_APPEND(without_arc_dynarray, load_number(input_stream));
	      break;

	    default:
	      PRINT_ERROR("Internal error: invalid token type (%ld)", 
			  token_type);
	    }
	}
    }
  catch (exception& e)
    {
      PRINT_ERROR(e.what());
    }
}



inline bool north_offset(EepicNode<long> * p)
{
  if (TAGLIST(p).is_empty())
    return false;

  for (DynDlist<Tag_Data>::Iterator tag_itor(TAGLIST(p)); 
       tag_itor.has_curr(); tag_itor.next())
    {
      Tag_Data & tag_data = tag_itor.get_curr();

      if (tag_data.tag_option == NORTH or 
	  tag_data.tag_option == NORTH_EAST or
	  tag_data.tag_option == NORTH_WEST)
	return true;
    }

  return false;
}


inline bool east_offset(EepicNode<long> * root)
{
  EepicNode<long> * p = find_max(root);

  if (TAGLIST(p).is_empty())
    return false;

  for (DynDlist<Tag_Data>::Iterator tag_itor(TAGLIST(p)); 
       tag_itor.has_curr(); tag_itor.next())
    {
      Tag_Data & tag_data = tag_itor.get_curr();

      if (tag_data.tag_option == EAST or 
	  tag_data.tag_option == NORTH_EAST or
	  tag_data.tag_option == SOUTH_EAST)
	return true;
    }

  return false;
}


inline bool west_offset(EepicNode<long> * root)
{
  EepicNode<long> * p = find_min(root);

  if (TAGLIST(p).is_empty())
    return false;

  for (DynDlist<Tag_Data>::Iterator tag_itor(TAGLIST(p)); 
       tag_itor.has_curr(); tag_itor.next())
    {
      Tag_Data & tag_data = tag_itor.get_curr();

      if (tag_data.tag_option == WEST or 
	  tag_data.tag_option == NORTH_WEST or
	  tag_data.tag_option == SOUTH_WEST)
	return true;
    }

  return false;
}
	
inline bool south_offset(EepicNode<long> * root, 
			       const size_t &    level)
{
  DynDlist<EepicNode<long>*> deepest_nodes = 
    compute_nodes_in_level(root, level);

  for (DynDlist<EepicNode<long>*>::Iterator it(deepest_nodes); 
       it.has_curr(); it.next())
    {
      EepicNode<long> * p = it.get_curr();

      if (TAGLIST(p).is_empty())
	continue;

      for (DynDlist<Tag_Data>::Iterator tag_itor(TAGLIST(p)); 
	   tag_itor.has_curr(); tag_itor.next())
	{
	  Tag_Data & tag_data = tag_itor.get_curr();

	  if (tag_data.tag_option == SOUTH or 
	      tag_data.tag_option == SOUTH_EAST or
	      tag_data.tag_option == SOUTH_WEST)
	    return true;
	}
    }

  return false;
}

void adjust_size_by_tags(EepicNode<long> * root, const size_t & height)
{
  long double r = std::max(hr, vr) + 2.0/resolution; // 2mm

  if (north_offset(root))
    v_size += r;

  if (south_offset(root, height - 1))
    v_size += r;

  if (east_offset(root))
    h_size += r;

  if (west_offset(root))
    h_size += r;
}

    inline
void set_picture_size(EepicNode<long> * p)
{
  int height = computeHeightRec(p);

  h_size = (num_nodes - 1)*w + hd;
  v_size = (height -1)*h + vd;

  if (max_num_nodes_rectangle > 0)
    v_size += (max_num_nodes_rectangle - 1)*vd;

  if (height_triangle_dynarray.size() > 0)
    {
      float max_triangle_height = 
	height_triangle_dynarray[search_max(height_triangle_dynarray, 0, 
					    height_triangle_dynarray.size())];
      v_size += max_triangle_height;
    }
  
  //  if (split_string_dynarray.size() > 0)
  //    v_size += 4.0/resolution; 

  adjust_size_by_tags(p, height);
}


    inline
void compute_picture_size(EepicNode<long> * p)
{
  int height = computeHeightRec(p);

  resolution = 1;

  w = h_size / (num_nodes + 1);
  h = v_size / (height + 1);
  hr = w / 4; hd = 2*hr;
  vr = h / 4; vd = 2*vr;     
}


EepicNode<long> * build_tree()
{
  try
    {
      num_nodes = prefix_dynarray.size();

      EepicNode<long> * root = NULL;
  
          /* construccion inicial del arbol segun recorridos de entrada */
      if (infix_dynarray.size() == 0)
	root = preorder_to_bst<EepicNode<long> >
	  (prefix_dynarray, 0, num_nodes - 1); /* solo recorrido prefijo */
      else
	{     /* entrada con dos recorridos */
	  if (infix_dynarray.size() != num_nodes)
	    AH_ERROR("Sizes of traversals differ");
	  
	  root = 
	    build_tree<EepicNode, long>(prefix_dynarray, 0, num_nodes - 1,
					infix_dynarray, 0, num_nodes - 1);
	  infix_dynarray.cut();
	}

      if (with_string_key)
	{
	  if (key_print_dynarray.size() != num_nodes)
	    AH_ERROR("Number of keys is different from tree size");
    
	  inOrderRec(root, assign_key);
	}
      else
	preOrderRec(root, assign_key);

      if (with_string_aux) 
	{
	  if (aux_print_dynarray.size() != num_nodes)
	    AH_ERROR("Number of auxiliary keys is different from tree size");

	  inOrderRec(root, assign_aux);
	}

      if (with_external_nodes) 
	assign_external_nodes(root);

          /* asignar posiciones infija a cada nodo */
      inOrderRec(root, assign_pos_and_level);

      if (with_external_nodes) 
	{
	  if (not with_string_key)
	    fill_type = "black";

	  set_picture_size(root);

	  return root; /* con nodos externos solo se dibuja forma */
	}

      key_print_dynarray.cut();
      prefix_dynarray.cut();

      if (without_node_dynarray.size() > 0)
	{
	  inOrderRec(root, assign_without_node);
	  without_node_dynarray.cut();
	}

      if (shadow_dynarray.size() > 0)
	{
	  inOrderRec(root, assign_shadow); 
	  shadow_dynarray.cut();
	}

      if (tag_pos_dynarray.size() > 0)
	{
	  inOrderRec(root, assign_tag);
	  tag_pos_dynarray.cut();
	  tag_data_dynarray.cut();
	}

      if (not fit_mode)
	set_picture_size(root);
      else
	compute_picture_size(root);

      if (source_arc_dynarray.size() > 0)
	{
	  assign_arcs(root, root);
	  source_arc_dynarray.cut();
	  target_arc_dynarray.cut();
	}

      if (scratch_dynarray.size() > 0)
	{
	  inOrderRec(root, assign_scratch);
	  scratch_dynarray.cut();
	}

      if (key_pos_dynarray.size() > 0)
	{ 
	  inOrderRec(root, reassign_key);
	  key_pos_dynarray.cut();
	  key_string_dynarray.cut();
	}

      if (pos_xoffset_dynarray.size() > 0)
	{ 
	  inOrderRec(root, assign_xoffset);
	  pos_xoffset_dynarray.cut();
	  xoffset_dynarray.cut();
	}

      if (pos_yoffset_dynarray.size() > 0)
	{ 
	  inOrderRec(root, assign_yoffset);
	  pos_yoffset_dynarray.cut();
	  yoffset_dynarray.cut();
	}
 
      if (pos_triangle_dynarray.size() > 0)
	{ 
	  inOrderRec(root, assign_triangle);
	  pos_triangle_dynarray.cut();
	  height_triangle_dynarray.cut();
	}

      if (pos_rectangle_dynarray.size() > 0)
	{ 
	  inOrderRec(root, assign_rectangle);
	  pos_rectangle_dynarray.cut();
	  height_rectangle_dynarray.cut();
	}

      if (pos_parrectangle_dynarray.size() > 0)
	{ 
	  inOrderRec(root, assign_parrectangle);
	  pos_parrectangle_dynarray.cut();
	  height_parrectangle_dynarray.cut();
	}

      if (pos_distance_dynarray.size() > 0)
	{ 
	  inOrderRec(root, assign_distance);
	  pos_distance_dynarray.cut();
	  distance_dynarray.cut();
	}

      if (without_arc_dynarray.size() > 0)
	{
	  inOrderRec(root, assign_without_arc);
	  without_arc_dynarray.cut();
	}
 
      if (threaded_trees or dash_threaded_trees)
	thread_tree(root);

      return root;
    }
  catch (exception& e)
    {
      AH_ERROR("%s", e.what());
    }

  return NULL; /* never reached */
}


void generate_prologue(ofstream & output)
{
  time_t t;
  time(&t);
  output << endl
	 << "%      This LaTeX picture is a binary tree automatically" << endl
	 << "%      generated by btreepic program" << endl
	 << endl
	 << "% Copyright (C) 2007, 2006, 2005, 2004, 2003, 2002" << endl
	 << "% UNIVERSITY of LOS ANDES (ULA)" << endl
	 << "% Merida - REPUBLICAf1727 BOLIVARIANA DE VENEZUELA" << endl
	 << "% Center of Studies in Microelectronics & Distributed Systems"
	 << " (CEMISID)" << endl
	 << "% ULA Computer Science Department" << endl
	 << endl
	 << "% Leandro Leon - lrleon@ula.ve" << endl
	 << endl
	 << "% You must use curves, epic and eepic latex packages" << endl
	 << "% in your LaTeX application" << endl
	 << endl
	 << "% curves Copyright by I.L. Maclaine-cross" << endl
	 << "% epic Copyright by Sunil Podar" << endl
	 << "% eepic Copyright by Conrad Kwok" << endl
	 << "% LaTeX is a collection of TeX macros created by Leslie Lamport" 
	 << endl
	 << "% TeX was created by Donald Knuth" << endl
	 << endl
	 << "% command line: " << endl
	 << "% " << command_line << endl 
	 << endl
	 << "% input file: " << input_file_name << endl 
	 << "% output file: " << output_file_name << endl 
	 << endl
	 << "% Creation date: " << ctime(&t) << endl 
	 << endl;

  if (latex_header) 
    output << "%%%%%%%%%%%%%%%% LATEX Header generated with -a option" << endl
	   << "\\documentclass[11pt]{article}" << endl
	   << (landscape ? "\\usepackage[landscape]{geometry}" : "") << endl
	   << endl
	   << (dash_threaded_trees ? "\\usepackage{curves}" : "" ) << endl
	   << "\\usepackage{epic}" << endl
	   << "\\usepackage{eepic}" << endl
	   << endl
	   << "\\begin{document}" << endl
	   << "\\begin{center}" << endl;

   output << endl
	  << "%    Resolution is " << resolution << "mm" << endl
	  << "%    Change resolution with -l option" << endl
	  << "\\setlength{\\unitlength}{" << resolution << "mm}" << endl
	  << "\\filltype{" << fill_type << "}" << endl
	  << (dash_threaded_trees ? "\\curvedashes[0.17mm]{1,5,3}\n" : "")
	  << endl
	  << "\\begin{picture}(" << h_size << "," << v_size << ")" 
	  << "(" << x_picture_offset << "," << y_picture_offset << ")" << endl;
}


void generate_epilogue(ofstream & output)
{
  output << endl 
	 << endl
	 << "\\end{picture}" << endl;

  if (latex_header)
    output << endl
	   << "\\end{center}" << endl
	   << "\\end{document}" << endl;
}


void generate_tree(ofstream & output, EepicNode<long> * p)
{
  if (p == NULL)
    return;

  long double x = X(p);
  long double y = Y(p);

      /* Imprimir comentario cabecera de nodo */
  output << endl 
	 << endl
	 << "% Node at infix position " << INFIXPOS(p) 
	 << " with key " << STRING(p); 

      /* dibujar nodo */
  if (ISEXTERNAL(p)) /* recta correspondiente a nodo externo */
    output << endl 
           << "%   External node" << endl
	   << "\\path(" << x - hr << "," << YPIC(y) << ")(" 
	   << x + hr << "," << YPIC(y) << ")" << endl;
  else if (ISTRIANGLE(p)) /* dibujar triangulo */
    output << endl
	   << "%   Triangle" << endl
	   << "\\path(" << x << "," << YPIC(y) << ")("
	   << x - hd << "," << YPIC(y + TRIANGLEH(p)) << ")("
	   << x + hd << "," << YPIC(y + TRIANGLEH(p)) << ")("
	   << x << "," << YPIC(y) << ")";
  else if (ISRECTANGLE(p)) /* dibujar rectangulo */
    {
      long double x1 = X(p) - hr;
      long double x2 = X(p) + hr;
      long double y1, y2;

      if (SCRATCH(p))
	{
	  y1 = Y(p);
	  y2 = Y(p) + RECTANGLEH(p) - vd;
      
	  output << endl
		 << "%   Partial Rectangle" << endl
		 << "\\path(" << x1 << "," << YPIC(y1) << ")("
		 << x2 << "," << YPIC(y1) << ")("
		 << x2 << "," << YPIC(y2) << ")("
		 << x1 << "," << YPIC(y2) << ")("
		 << x1 << "," << YPIC(y1) << ")";
	  
	  y1 = Y(p) + RECTANGLEH(p) - vd;
	  y2 = Y(p) + RECTANGLEH(p);

	  output << endl
		 << "\\dashline{" << dash_len << "}("
		 << x1 << "," << YPIC(y1) << ")("
		 << x1 << "," << YPIC(y2) << ")("
		 << x2 << "," << YPIC(y2) << ")("
		 << x2 << "," << YPIC(y1) << ")" << endl
		 << "\\path(" << x1 << "," << YPIC(y1) << ")("
		 << x2 << "," << YPIC(y2) << ")" << endl
		 << "\\path(" << x2 << "," << YPIC(y1) << ")("
		 << x1 << "," << YPIC(y2) << ")";	    
	}
      else
	{
	  y1 = Y(p);
	  y2 = y1 + RECTANGLEH(p);
      
	  output << endl
		 << "%   Rectangle" << endl
		 << "\\path(" << x1 << "," << YPIC(y1) << ")("
		 << x2 << "," << YPIC(y1) << ")("
		 << x2 << "," << YPIC(y2) << ")("
		 << x1 << "," << YPIC(y2) << ")("
		 << x1 << "," << YPIC(y1) << ")";
	}
    }
  else if (draw_nodes and not WITHOUT(p)) /* elipse de nodo interno */ 
    output << endl
	   << "%   Elippse" << endl
	   << "\\put(" << x << "," << YPIC(y) << ")" 
	   << (SHADOW(p) ? "{\\ellipse*{" : "{\\ellipse{") << hd 
	   << "}{" << vd << "}}";
  else if (SHADOW(p))
    output << endl
	   << "%   Elippse" << endl
	   << "\\put(" << x << "," << YPIC(y) << "){\\ellipse*{" << hd 
	   << "}{" << vd << "}}";

  /* imprimir linea de distancia si es el caso */
  if (ISDISTANCE(p))
    {
      output << endl
	     << "%   Distance line";
      long double xof = 2.0/resolution; /* 2 mm de separacion de nodo */
      long double yplus = 1.0/resolution; /* sobrelargo de linea */
      long double yof = 3.5/resolution; /* 3.0 mm de espacio para letras */
      long double line_len = 0;
      Line_Distance_Data ldd = LDISTANCE(p);
      long double xd = 0;

      if (ISRECTANGLE(p))
	{
	  xd = ldd.is_left() ? - (hr + xof) : (hr +xof);
	  line_len = (RECTANGLEH(p) - yof) / 2;
	}
      else if (ISTRIANGLE(p))
	{
	  xd = ldd.is_left() ? - (w/2 + xof): (w/2 + xof);
	  line_len = (TRIANGLEH(p) - yof) / 2;
	}
      else
	  PRINT_ERROR("Distance line on %d th complete node", INFIXPOS(p));

      long double xl = x + xd;
      long double gap_len = 2.0/resolution;
      long double dy = sin_45*gap_len;
      long double yf1 = y + line_len;
      long double yf2 = y + yof + 2*line_len; 
      output << endl
	     << "\\path(" << xl << "," << YPIC(y - yplus) << ")(" 
	     << xl << "," << YPIC(y + line_len) << ")" << endl
	     << "\\path(" << xl << "," << YPIC(yf1 + yof) << ")("
	     << xl << "," << YPIC(yf2 + yplus) << ")" << endl
	     << "\\path(" << xl - gap_len/2 << "," << YPIC(y) << ")("
	     << xl + gap_len/2 << "," << YPIC(y) << ")" << endl
	     << "\\path(" << xl - gap_len/2 << "," << YPIC(yf2 + yplus) 
	     << ")" << endl
	     << "\\path(" << xl - gap_len/2 << "," << YPIC(yf2) << ")("
	     << xl + gap_len/2 << "," << YPIC(yf2) << ")" << endl
	     << "\\path(" << xl - gap_len/2 << "," << YPIC(y + dy) << ")("
	     << xl + gap_len/2 << "," << YPIC(y - dy) << ")" << endl
	     << "\\path(" << xl - gap_len/2 << "," << YPIC(yf2 + dy) << ")("
	     << xl + gap_len/2 << "," << YPIC(yf2 - dy) << ")";
      
      long double string_gap = 2.0/resolution;
      long double str_offset = ldd.is_left() ? 
	string_width(ldd.str) + string_gap : - string_gap;
      put_string(output, xl - str_offset + ldd.xoffset, 
		 y + line_len + yof + ldd.yoffset,
		 "String of line distance", ldd.str);
    }

      /* imprimir el contenido del nodo */
  if (printing_key_mode and not EXTERNAL(p)) 
    {
      long double dx = center_string(STRING(p), hd);
      long double dy = font_height()/2.0;
      long double dy_triangle = not ISTRIANGLE(p) ? 0 : TRIANGLEH(p)/4;
      long double dy_rectangle = 0;

      if (ISRECTANGLE(p))
	dy_rectangle = 
	  (SCRATCH(p) ? (RECTANGLEH(p) - vd) : RECTANGLEH(p))/2 + vr; 

      if (not with_string_aux) /* imprimir solo la clave */
	put_string(output, x + x_offset + XOFFSET(p) - dx, 
		   y + dy + y_offset + YOFFSET(p) + 
		   RECTANGLEH(p) - dy_rectangle + TRIANGLEH(p) - dy_triangle, 
		   "Key text= " + STRING(p), STRING(p));
      else
	{     /* nodos contienen dos campos: clave y cadena auxiliar */
	  long double dxa = center_string(AUX(p), hd);
	  long double dyk = 1.2/resolution; // 1.2 mm above line
	  long double dya = 2/resolution; // 3 mm below line

	      /* colocar clave */
	  put_string(output, x + x_offset + XOFFSET(p) - dx,
		     y + dy + y_offset + YOFFSET(p) + TRIANGLEH(p) - 
		         dy_triangle +
		     RECTANGLEH(p) - dy_rectangle - dyk,
		     "Key text= " + STRING(p), STRING(p));

	      /* linea divisoria de nodo entre cadena principal y auxiliar */
	  if (not (ISTRIANGLE(p) or ISRECTANGLE(p)))
	    output << endl
		   << "\\path(" << x - hr << "," << YPIC(y) << ")(" 
		   << x + hr << "," << YPIC(y) << ")";

	      /* colocar clave auxiliar */
	  put_string(output, x + x_aux_offset - dxa,
		     y + dy + y_aux_offset + dya + TRIANGLEH(p) - dy_triangle +
		     RECTANGLEH(p) - dy_rectangle,
		     "Auxiliar string= " + AUX(p), AUX(p));
	}
    }

      /* imprimir las etiquetas -TAGs-del nodo */
  if (not TAGLIST(p).is_empty())
    {
      long double tx, ty; /* coordenadas de la tag */
      Tag_Data tag_data;
      long double r = std::max(hr, vr) + 2.0/resolution; // 2mm
      long double dr = sin_45*r; /* radius r at 45 degrees */
      long double x_font_size = 2.0/resolution;
      long double y_font_size = 2.5/resolution;
      string comment;

      for (DynDlist<Tag_Data>::Iterator itor(TAGLIST(p)); 
	   itor.has_curr(); itor.next())
	{
	  tag_data = itor.get_curr();

	  switch (tag_data.tag_option)
	    {
	    case NORTH: 
	      comment = "North tag: ";
	      tx = x - x_font_size/2 + tag_data.x_offset; 
	      ty = y - r + tag_data.y_offset; 

	      break;
	    case SOUTH: 
	      comment = "South tag: ";
	      tx = x - x_font_size/2 + tag_data.x_offset; 
	      ty = y + r + y_font_size/2 + tag_data.y_offset; 
	      break;
	    case EAST: 
	      comment = "East tag: ";
	      tx = x + r + tag_data.x_offset; 
	      ty = y + y_font_size/2 + tag_data.y_offset; 
	      break;
	    case WEST: 
	      comment = "West tag: ";
	      tx = x - r - x_font_size + tag_data.x_offset -
		string_width(tag_data.tag); 
	      ty = y + y_font_size/2 + tag_data.y_offset; 
	      break;
	    case NORTH_EAST: 
	      comment = "Northeast tag: ";
	      tx = x + dr + x_font_size/2 + tag_data.x_offset; 
	      ty = y - dr + y_font_size/2 + tag_data.y_offset; 
	      break;
	    case NORTH_WEST: 
	      comment = "Northwest tag: ";
	      tx = x - dr - x_font_size/2 + tag_data.x_offset -
		string_width(tag_data.tag); 
	      ty = y - dr + y_font_size/2 + tag_data.y_offset; 
	      break; 
	    case SOUTH_EAST:
	      comment = "Southeast tag: ";
	      tx = x + dr + x_font_size/2 + tag_data.x_offset; 
	      ty = y + dr + y_font_size/2 + tag_data.y_offset; 
	      break;
	    case SOUTH_WEST: 
	      comment = "Southwest tag: ";
	      tx = x - dr - x_font_size/2 + tag_data.x_offset -
		string_width(tag_data.tag); 
	      ty = y + dr + y_font_size/2 + tag_data.y_offset; 
	      break;
	    default:
	      PRINT_ERROR("Internal error: unexpected tag option");
	    }
	  put_string(output, tx, ty, comment, tag_data.tag);
	}
    }

  if (SCRATCH(p) and not ISRECTANGLE(p))
    {
      long double r = std::max(hr, vr) + 2.0/resolution; // 2mm
      long double dr = sin_45*r; /* radius r at 45 degrees */
      output << endl
	     << "%   Scratching" << endl
	     << "\\path(" << x - dr << "," << YPIC(y - dr) << ")"
	     << "(" << x + dr << "," << YPIC(y + dr) << ")"
	     << "\\path(" << x + dr << "," << YPIC(y - dr) << ")"
	     << "(" << x - dr << "," << YPIC(y + dr) << ")";
    }

  EepicNode<long> * l = LLINK(p);
  EepicNode<long> * r = RLINK(p);

      /* dibujar arcos adicionales del nodo */
  for (DynDlist<Arc_Desc>::Iterator itor(ARCLIST(p)); itor.has_curr(); 
       itor.next())
    {
      Arc_Desc arc_desc = itor.get_curr();

      if (arc_desc.target_node == l)
	{ /* el arco sera dibujado cuando se procese lazo izq */
	  DASHLLINK(p) = arc_desc.is_dashed ? true : false;
	  continue; 
	}

      if (arc_desc.target_node == r)
	{ /* el arco sera dibujado cuando se procese lazo der */
	  DASHRLINK(p) = arc_desc.is_dashed ? true : false;
	  continue;
	}

      long double tx = X(arc_desc.target_node);
      long double ty = Y(arc_desc.target_node);
      long double dx, dy;
      intersection_ellipse_line(x, y, tx, ty, hr, vr, dx, dy);
      long double src_x, src_y, tgt_x, tgt_y;

          /* determina ptos x arco segun posiciones de nodos */
      if (x > tx)
	{
	  src_x = x - dx;
	  tgt_x = tx + dx;
	} 
      else
	{
	  src_x = x + dx;
	  tgt_x = tx - dx;
	} 

          /* determina ptos y arco segun posiciones de nodos */
      if (y > ty)
	{
	  src_y = y - dy;
	  tgt_y = ty + dy;
	} 
      else
	{
	  src_y = y + dy;
	  tgt_y = ty - dy;
	} 
      
      if (ISTRIANGLE(arc_desc.target_node) or 
	  ISRECTANGLE(arc_desc.target_node))
	{ // si hay triangulo o rectangulo arc va hasta la punta de triangulo
	  tgt_x = tx;
	  tgt_y = ty;
	}
	  
      output << endl 
	     << "%   Additional arc to infix node " 
	     << INFIXPOS(arc_desc.target_node)
	     << " with key " << STRING(arc_desc.target_node)
	     << endl;

      draw_arc(output, src_x, src_y, tgt_x, tgt_y, 
	       arc_desc.is_dashed, with_arrow);
    }

      /* procesado de arco o hilo del hijo izquierdo */
  if (l != NULL and WITHARC(l))
    {    /* dibujar arco hacia hijo izquierdo */
      long double lx = X(l);
      long double ly = Y(l);
      long double dx, dy;
      intersection_ellipse_line(x, y, lx, ly, hr, vr, dx, dy);
      output << endl
	     << "%   Arc to left infix node " << INFIXPOS(l) 
	     << " with key " << STRING(l) << endl;

      if (ISEXTERNAL(l) or ISTRIANGLE(l) or ISRECTANGLE(l))
	draw_arc(output, x - dx, y + dy, lx, ly, DASHLLINK(p), with_arrow);
      else
	draw_arc(output, x - dx, y + dy, lx + dx, ly - dy, 
		 DASHLLINK(p), with_arrow);
    }
  else if (PREV(p) != NULL and not ISTRIANGLE(p) and not ISRECTANGLE(p))
    {    /* dibujar hilo hacia predecesor */
      EepicNode<long> * prev = PREV(p);
      long double px = X(prev);
      long double py = Y(prev);
      long double dx, dy;
      intersection_ellipse_line(x, y, px, py, hr, vr, dx, dy);
      output << endl
	     << "%   Thread to predecessor infix node " << INFIXPOS(prev) 
	     << " with key " << STRING(prev) << endl;
      if (dash_threaded_trees)
	output << "\\curve(" << x - dx << "," << YPIC(y + dy) << "," 
	       << px + dx << "," << YPIC(y + h) << ","
	       << px + dx << "," << YPIC(py + dy) << ")";
      else
	output << "\\spline(" << x - dx << "," << YPIC(y + dy) << ")(" 
	       << px << "," << YPIC(y + h) << ")("
	       << px + dx << "," << YPIC(py + dy) << ")";
    }

      /* procesacimiento de arco o hilo del hijo derecho */
  if (r != NULL and WITHARC(r))
    {    /* dibujar arco hacia hijo derecho */
      long double rx = X(r);
      long double ry = Y(r);
      long double dx, dy;
      intersection_ellipse_line(x, y, rx, ry, hr, vr, dx, dy);
      output << endl
	     << "%   Arc to right infix node " << INFIXPOS(r) 
	     << " with key " << STRING(r) << endl;

      if (ISEXTERNAL(r) or ISTRIANGLE(r) or ISRECTANGLE(r))
	draw_arc(output, x + dx, y + dy, rx, ry, DASHRLINK(p), with_arrow);
      else
	draw_arc(output, x + dx, y + dy, rx - dx, ry - dy, 
		 DASHRLINK(p), with_arrow);
    } 
  else if (SUCC(p) != NULL and not ISTRIANGLE(p) and not ISRECTANGLE(p))
    {    /* dibujar hilo hacia sucesor */
      EepicNode<long> * succ = SUCC(p);
      long double px = X(succ);
      long double py = Y(succ);
      long double dx, dy;
      intersection_ellipse_line(x, y, px, py, hr, vr, dx, dy);
      output << endl
	     << "%   Thread to successor infix node " << INFIXPOS(succ) 
	     << " with key " << STRING(succ) << endl;
      if (dash_threaded_trees)
	output << "\\curve(" << x + dx << "," << YPIC(y + dy) << "," 
	       << px - dx << "," << YPIC(y + h) << ","
	       << px - dx << "," << YPIC(py + dy) << ")";
      else
	output << "\\spline(" << x + dx << "," << YPIC(y + dy) << ")(" 
	       << px << "," << YPIC(y + h) << ")("
	       << px - dx << "," << YPIC(py + dy) << ")";
    }

  generate_tree(output, l); /* procesar rama arbol izquierdo */
  generate_tree(output, r); /* procesar rama arbol derecho */
}


    inline
void generate_split_lines(ofstream & output, EepicNode<long> * root)
{
  if (split_dynarray.size() > num_nodes)
    PRINT_ERROR("Number of split points is greater than total of nodes");

  for (int i = 0; i < split_dynarray.size(); i++)
    {
      int pos = split_dynarray[i];
      if (pos >= num_nodes - 1)
	PRINT_ERROR("Split position (%d) out of range", pos);

      EepicNode<long> * src = select(root, pos);

      long double x = X(src);  
      long double line_space = 1.0/resolution; // 1mm
      long double line_len = v_size - 4*line_space + vd/2.0;
      long double dash_len = 1/resolution; // 1mm

      output << endl
	     << "%   Split line at node " << INFIXPOS(src) 
	     << " with key " << STRING(src) << endl
	     << "\\dashline{" << dash_len << "}(" << x << "," 
	     << YPIC(line_space) << ")"
	     << "(" << x << "," << YPIC(line_space + line_len) << ")";

      Split_Data split_data = split_string_dynarray[i];

      int upper_len = split_data.upper_string.size();

      if (upper_len > 0) 
	{
	  long double upper_size = 
	    upper_len*(2.0/resolution); // 2.0mm por letra
	  long double dx = upper_size/2.0; 
	  output << endl
		 << "\\put(" << x - dx << "," << YPIC(line_space) << ")"
		 << "{" << font_wrapper << split_data.upper_string << "}}"
		 << endl;
	}

      int lower_len = split_data.lower_string.size();

      if (lower_len > 0)
	{
	  long double lower_size = 
	    lower_len*(2.0/resolution); // 2.0mm por letra
	  long double dx = lower_size/2.0; 
	  long double font_height = 3.0/resolution;
	  output << endl
		 << "\\put(" << x - dx << "," 
		 << YPIC(line_space + line_len + font_height) << ")"
		 << "{" << font_wrapper << split_data.lower_string << "}}"
		 << endl;
	}
    }
}

const char *argp_program_version = 
"btreepic 1.9.6\n"
"ALEPH drawer for binary trees\n"
"Copyright (C) 2007 UNIVERSITY of LOS ANDES (ULA)\n"
"Merida - REPUBLICA BOLIVARIANA DE VENEZUELA\n"
"Center of Studies in Microelectronics & Distributed Systems (CEMISID)\n"
"ULA Computer Science Department\n"
"This is free software; There is NO warranty; not even for MERCHANTABILITY\n"
"or FITNESS FOR A PARTICULAR PURPOSE\n"
"\n";

const char *argp_program_bug_address = "lrleon@ula.ve";

static char doc[] = "btreepic -- Aleph drawer for binary trees";

static char argDoc[] = "-f input-file\n";

static char * hello = 
"\n"
"ALEPH drawer for binary trees\n"
"Copyright (C) 2007, 2006, 2005, 2004, 2003, 2002 University of Los Andes (ULA)\n"
"Merida - REPUBLICA BOLIVARIANA DE VENEZUELA\n"
"Center of Studies in Microelectronics & Distributed Systems (CEMISID)\n"
"ULA Computer Science Department\n"
"This is free software; There is NO warranty; not even for MERCHANTABILITY\n"
"or FITNESS FOR A PARTICULAR PURPOSE\n"
"\n";

static const char license_text [] = 
"Aleph drawer for binary trees. License & Copyright Note\n"
"Copyright (C) 2007, 2006, 2005, 2004, 2003, 2002\n"
"UNIVERSITY of LOS ANDES (ULA)\n"
"Merida - VENEZUELA\n"
"Center of Studies in Microelectronics & Distributed Systems (CEMISID)\n"
"ULA Computer Science Department\n"
"This is free software; There is NO warranty; not even for MERCHANTABILITY\n"
"or FITNESS FOR A PARTICULAR PURPOSE\n"
"\n"
"  PERMISSION TO USE, COPY, MODIFY AND DISTRIBUTE THIS SOFTWARE AND ITS \n"
"  DOCUMENTATION IS HEREBY GRANTED, PROVIDED THAT BOTH THE COPYRIGHT \n"
"  NOTICE AND THIS PERMISSION NOTICE APPEAR IN ALL COPIES OF THE \n"
"  SOFTWARE, DERIVATIVE WORKS OR MODIFIED VERSIONS, AND ANY PORTIONS \n"
"  THEREOF, AND THAT BOTH NOTICES APPEAR IN SUPPORTING DOCUMENTATION. \n"
"\n"
"  This program is distributed in the hope that it will be useful,\n"
"  but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. \n"
"\n"
"  ULA requests users of this software to return to \n"
"      Proyecto Aleph - CEMISID Software\n"
"      Nucleo Universitario La Hechicera. Ed Ingenieria\n"
"      3er piso, ala Este \n"
"      Universidad de Los Andes \n"
"      Merida 5101 - REPUBLICA BOLIVARIANA DE VENEZUELA \n"
"\n"
"  or to 	lrleon@ula.ve \n"
"\n"
"  any improvements or extensions that they make and grant Universidad \n"
"  de Los Andes (ULA) the full rights to redistribute these changes. \n"
"\n"
" This program was granted by: \n"
" - Consejo de Desarrollo Cientifico, Humanistico, Tecnico de la ULA\n"
"  (CDCHT)\n";


static struct argp_option options [] = {
  {"radius", 'r', "radius", OPTION_ARG_OPTIONAL, 
   "fit radius for circles" , 0},
  {"width", 'w', "width", OPTION_ARG_OPTIONAL, "horizontal separation" , 0},
  {"height", 'h', "height", OPTION_ARG_OPTIONAL, "vertical separation" , 0},
  {"h-radius", 'x', "horizontal-radius", OPTION_ARG_OPTIONAL, 
   "horizontal radius (ellipse)" , 0},
  {"v-radius", 'y', "vertical-radius", OPTION_ARG_OPTIONAL, 
   "vertical  radius (ellipse)" , 0},
  {"resol", 'l', "resolution", OPTION_ARG_OPTIONAL, "resolution in mm" , 0},
  {"latex", 'a', 0, OPTION_ARG_OPTIONAL, "add latex header" , 0},
  {"landscape", 'p', 0, OPTION_ARG_OPTIONAL, "latex landscape mode" , 0},
  {"with-key", 'k', 0, OPTION_ARG_OPTIONAL, "printing keys" , 0},
  {"tiny-keys", 'K', 0, OPTION_ARG_OPTIONAL, "printing keys with tiny font" , 0},
  {"fit", 't', 0, OPTION_ARG_OPTIONAL, "fit in rectangle" , 0},
  {"horizontal", 'z', "horizontal-size", OPTION_ARG_OPTIONAL, 
   "specify horizontal width for fitting", 0 },
  {"vertical", 'u', "vertical-size", OPTION_ARG_OPTIONAL, 
   "specify vertical height for fitting", 0},
  {"min radio", 'n', 0, OPTION_ARG_OPTIONAL, "radious is minimum", 0},
  {"without-node", 'N', 0, OPTION_ARG_OPTIONAL, "no draw node; only arcs", 0},
  {"key-x-offset", 'X', "offset", OPTION_ARG_OPTIONAL, 
   "horizontal offset for keys", 0},
  {"key-y-offset", 'Y', "offset", OPTION_ARG_OPTIONAL, 
   "vertical offset for keys", 0},
  {"aux-y-offset", 'H', "offset", OPTION_ARG_OPTIONAL, 
   "vertical offset for auxiliary string", 0},
  {"aux-y-offset", 'W', "offset", OPTION_ARG_OPTIONAL, 
   "horizontal offset for auxiliary string", 0},
  {"input-file", 'i', "input-file", 0, "input file", 0},
  {"input-file", 'f', "input-file", OPTION_ALIAS, 0, 0},
  {"output", 'o', "output-file", 0, "output file", 0},
  {"license", 'C', 0, OPTION_ARG_OPTIONAL, "print license", 0},
  {"x-picture-offset", 'O', "horizontal-picture-offset", 
   OPTION_ARG_OPTIONAL, "horizontal global picture offset", 0},
  {"y-picture-offset", 'P', "vertical-picture-offset", 
   OPTION_ARG_OPTIONAL, "vertical global picture offset", 0},
  {"print", 'R', 0, OPTION_ARG_OPTIONAL, "print current parameters", 0},
  {"verbose", 'v', 0, OPTION_ARG_OPTIONAL, "verbose mode", 0},
  {"unsilent", 's', 0, OPTION_ARG_OPTIONAL, "unsilent mode - print tokens", 0},
  {"version", 'V', 0, OPTION_ARG_OPTIONAL, 
   "Print version information and then exit", 0},
  {"black-fill", 'B', 0, OPTION_ARG_OPTIONAL, "Fill black ellipses", 0},
  {"shade-fill", 'S', 0, OPTION_ARG_OPTIONAL, "Fill shade ellipses", 0},
  {"threaded", 'D', 0, OPTION_ARG_OPTIONAL, 
   "draw dotted threads instead NULL pointers", 0},
  {"threaded-no-dash", 'T', 0, OPTION_ARG_OPTIONAL, 
   "draw contiguous threads instead NULL pointers", 0},
  {"external-nodes", 'e', 0, OPTION_ARG_OPTIONAL, "draw external nodes", 0},
  {"arrow-len", 'L', "arrow lenght in points", OPTION_ARG_OPTIONAL, 
   "arrow lenght", 0},
  {"arrow-len", 'I', "arrow width in points", OPTION_ARG_OPTIONAL, 
   "arrow width", 0},
  {"arrows", 'A', 0, OPTION_ARG_OPTIONAL, "draw arcs with arrows", 0},
  {"vertical-flip", 'F', 0, OPTION_ARG_OPTIONAL, "Flip tree respect y axe", 0},
  { 0, 0, 0, 0, 0, 0 } 
}; 


    inline
void print_parameters()
{
  cout
    << "Horizontal radius             -x   = " << hr << endl
    << "Vertical radius               -y   = " << vr << endl
    << "Horizontal diameter                = " << hd << endl
    << "Vertical diameter                  = " << vd << endl
    << "Horizontal separation         -w   = " << w << endl
    << "Vertical separation           -h   = " << h << endl
    << "Resolution in mm              -l   = " << resolution << endl
    << "Horizontal size               -z   = " << h_size << endl
    << "Vertical size                 -u   = " << v_size << endl
    << "Horizontal offset for key     -X   = " << x_offset << endl
    << "Vertical offset for key       -Y   = " << y_offset << endl
    << "Horizontal offset for aux tag -W   = " << x_aux_offset << endl
    << "Vertical offset for aux tag   -H   = " << y_aux_offset << endl
    << "Horizontal offset for picture -O   = " << x_picture_offset << endl
    << "Vertical offset for picture   -P   = " << y_picture_offset << endl;
}


static error_t parser_opt(int key, char *arg, struct argp_state *)
{
  switch (key)
    {
    case 'r': /* Especificacion de radio */
      if (arg == NULL)
	AH_ERROR("Waiting for radius in command line");
      hr = vr = atof(arg); hd = vd = 2*hr;
      break;
    case 'w': /* Separacion horizontal entre nodos */
      if (arg == NULL)
	AH_ERROR("Waiting for width in command line");
      w = atof(arg);
      break;
    case 'h': /* Separacion vertical entre nodos */
      if (arg == NULL)
	AH_ERROR("Waiting for height in command line");
      h = atof(arg);
      break;
    case 'x': /* radio horizontal de elipse */
      if (arg == NULL)
	AH_ERROR("Waiting for horizontal radius in command line");
      hr = atof(arg); hd = 2*hr;
      break;
    case 'y': /* radio vertical de elipse */
      if (arg == NULL)
	AH_ERROR("Waiting for vertical radius in command line");
      vr = atof(arg); vd = 2*vr;
      break;
    case 'l': /* resolucion en milimetros */
      if (arg == NULL)
	AH_ERROR("Waiting for resolution in command line");
      resolution =  atof(arg);
      if (resolution > 10)
	cout << "Warning: resolution too big" << endl;
      break;
    case 'a': /* colocar un encabezado latex */
      latex_header = true;
      break;
    case 'p': // encabezado latex en modo landsacpe
      latex_header = true;
      landscape = true;
      break;
    case 'k': /* imprimir claves */
      printing_key_mode = true;
      break;
    case 'K': /* tiny keys */
      tiny_keys = true;
      break;
    case 't': /* coloca modo automatico de calculo de parametros */
      fit_mode = true;
      break;
    case 'z':
      if (arg == NULL)
	AH_ERROR("Waiting for horizontal size in command line");
      h_size = atof(arg);
      break;
    case 'u':
      if (arg == NULL)
	AH_ERROR("Waiting for vertical size in command line");
      v_size = atof(arg);
      break;
    case 'n': /* coloca el radio minimo */
      hr = vr = resolution/2; hd = vd = resolution;  
      break;
    case 'N': /* no dibuja nodos, solo arcos */ 
      draw_nodes = false;
      break;
    case 'X': /* offset horizontal para letras */ 
      if (arg == NULL)
	AH_ERROR("Waiting for horizontal offset in command line");
      x_offset = atof(arg);
      break;
    case 'Y': /* offset vertical para letras */ 
      if (arg == NULL)
	AH_ERROR("Waiting for vertical offset in command line");
      y_offset = atof(arg);
      break;
    case 'O': /* offset horizontal para dibujo */ 
      if (arg == NULL)
	AH_ERROR("Waiting for horizontal offset in command line");
      x_picture_offset = atof(arg);
      break;
    case 'P': /* offset horizontal para dibujo */ 
      if (arg == NULL)
	AH_ERROR("Waiting for vertical offset in command line");
      y_picture_offset = atof(arg);
      break;
    case 'W': /* offset horizontal para claves auxiliares */ 
      if (arg == NULL)
	AH_ERROR("Waiting for horizontal offset in command line");
      x_aux_offset = atof(arg);
      break;
    case 'H': /* offset vertical para letras */ 
      if (arg == NULL)
	AH_ERROR("Waiting for vertical offset in command line");
      y_aux_offset = atof(arg);
      break;
    case 'v': /* verbose mode (no implementado) */ 
      break;
    case 'i': /* archivo de entrada */
    case 'f':
      {
	if (arg == NULL)
	  AH_ERROR("Waiting for input file name");
	input_file_name  = arg;
	break;
      }
    case 'o': /* archivo de salida */
      if (arg == NULL)
	AH_ERROR("Waiting for output file name");
      output_file_name = arg;
      break;
    case 'C': /* imprime licencia */ 
      cout << license_text;
      TERMINATE(0);
      break;
    case 'R': /* imprime parametros */ 
      print_parameters();
      save_parameters();
      TERMINATE(0);
      break;
    case 's': /* elimina modo silencioso */ 
      silent_mode = false;
      break;
    case 'V': /* imprimir version */ 
      cout << argp_program_version;
      TERMINATE(0);
      break;
    case 'B': /* filltype == black */
      fill_type = "black";
      break;
    case 'S': /* filltype == shade */
      fill_type = "shade";
      break;
    case 'T': /* draw dotted threads */
      threaded_trees = true; 
      break;
    case 'D':
      dash_threaded_trees = true; 
      break;
    case 'e':
      with_external_nodes = true; 
      break;
    case 'A':
      with_arrow = true;
      break;
    case 'L':
      with_arrow = true;
      if (arg == NULL)
	AH_ERROR("Waiting for arrow lenght in command line");
      arrow_lenght = atof(arg);
      break;
    case 'I':
      with_arrow = true;
      if (arg == NULL)
	AH_ERROR("Waiting for arrow width in command line");
      arrow_width = atof(arg);
      break;
    case 'F':
      flip_y = true;
      break;    default: return ARGP_ERR_UNKNOWN;
    }
  return 0;
}


static struct argp argDefs = { options, parser_opt, argDoc, doc, 0, 0, 0 }; 


int main(int argc, char *argv[]) 
{
  command_line = command_line_to_string(argc, argv);

  read_parameters();

  argp_parse(&argDefs, argc, argv, ARGP_IN_ORDER, 0, NULL);

  if (input_file_name.size() == 0)
    AH_ERROR("Input file not given");

  cout << hello;

  file_to_dynarrays(input_file_name.c_str());

  EepicNode<long> * root = build_tree();

  if (output_file_name.size() == 0)
    {
      output_file_name = input_file_name;
      const int pos = input_file_name.rfind(".");
      if (pos != string::npos)
	output_file_name = 
	  string(input_file_name).replace(pos, input_file_name.size() - pos, 
					  string(""));
      output_file_name = output_file_name + (tiny_keys ? ".eepicaux" : ".eepic");
    }
  
  cout << "input from " << input_file_name << " file " << endl
       << "output sent to " << output_file_name << " file " << endl << endl;

  ofstream output(output_file_name.c_str(), ios::out);
  generate_prologue(output);
  generate_tree(output, root);
  generate_split_lines(output, root);
  generate_epilogue(output);
  output.close();
  
  save_parameters();
}
