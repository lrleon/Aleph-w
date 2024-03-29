
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
 
# include <iostream>
# include <fstream>
# include <string>
# include <tpl_dynTreap.H>
# include <tpl_dynArray.H>
# include <tpl_sort_utils.H>
# include <tpl_graph.H>
# include <treepic_utils.H>
# include <parse_utils.H>
# include <eepicgeom.H>

# include <argp.h>

# define TERMINATE(n) (save_parameters(), exit(n))

// TODO: corregir tag con los angulos de 45 al punto de intersección con
// la elipse  

enum Token_Type { 
  COMMENT,
  DIGRAPH, GRAPH,   // Indica si es grafo o digrafo
  POLY_DIGRAPH, POLY_GRAPH,
  NET_GRAPH, NET_DIGRAPH,
  CROSS_NET_GRAPH, CROSS_NET_DIGRAPH,
  NODE,             // Nodo
  ARC,              // Arco
  SHADOW_NODE,      // Nodo sombreado
  SHADOW_ARC,       // Arco sombreado
  DASHED_ARC,       // Arco punteado
  DASHED_CURVE_ARC, // Arco curvo punteado
  CURVE_ARC,        // Arco curvo
  SHADOW_CURVE_ARC, // Arco curvo sombreado
  NODE_TEXT,        // Etiqueta de nodo
  ARC_TEXT,         // Etiqueta de arco
  TAG,              // Etiqueta
  WITHOUT_NODE,     // No dibujar nodo
  SHADOW_PATH,      // Camino sombreado a destacar entre dos nodos
  PATH,             // Camino a destacar entre dos nodos
  LEFT, RIGHT,      // Cadena a escribir en un arco (a su izquierda o derecha)
  HRADIO,           // radio horizontal de un nodo particular
  VRADIO,           // radio vertical  de un nodo particular
  STRING,           // Cadena a escribir en un nodo
                    // TAG options
  NORTH,      // N
  SOUTH,      // S
  EAST,       // E
  WEST,       // W
  NORTH_EAST, // NE
  NORTH_WEST, // NW
  SOUTH_EAST, // SE
  SOUTH_WEST, // SW
  END_FILE, 
  INVALID 
};


extern bool tiny_keys;

/* valores de distancias por omision */
double hr = 8;  // radio horizontal de la elipse
double vr = 8;  // radio vertical de la elipse

double h_size = 3000; /* longitud horizontal de picture */
double v_size = 3000; /* longitud vertical de picture */

double zoom_factor = 1;

double hd = 2*hr;  // diametro horizontal de la elipse
double vd = 2*vr;  // diametro vertical de la elipse
double x_offset = 0; /* offset horizontal etiqueta */
double y_offset = 0;
double x_picture_offset = 0; // offset horizontal para dibujo
double y_picture_offset = 0; // offset vertical para dibujo

long line_dimension = 100;

string command_line;
string input_file_name;
string output_file_name;

bool draw_node_mode = true;  // dibujar elipses

bool squarize = true; // ajuste automático de la escala del dibujo

bool latex_header = false; // envolver salida con un latex header
bool ellipses     = true; // por omisión dibujar elipses 
bool rectangles   = false; 
bool not_nodes    = false;


const char * parameters_file_name = "./.graphpic";


    inline
void print_parameters()
{
  cout
    << "Horizontal radius             -x   = " << hr << endl
    << "Vertical radius               -y   = " << vr << endl
    << "Horizontal diameter                = " << hd << endl
    << "Vertical diameter                  = " << vd << endl
    << "Resolution in mm              -l   = " << resolution << endl
    << "Horizontal size               -z   = " << h_size << endl
    << "Vertical size                 -u   = " << v_size << endl
    << "Horizontal offset for key     -X   = " << x_offset << endl
    << "Vertical offset for key       -Y   = " << y_offset << endl
    << "Horizontal offset for picture -O   = " << x_picture_offset << endl
    << "Vertical offset for picture   -P   = " << y_picture_offset << endl;
}


    inline
void save_parameters()
{
  ofstream output(parameters_file_name, ios::trunc);

  output << hr << " " << vr << " " << hd << " " << vd << " " << resolution 
	 << " " << h_size << " " << v_size << " " << x_offset
	 << " " << y_offset << " " << endl;
}


    inline
void read_parameters()
{
  ifstream input(parameters_file_name, ios::in);

  input >> hr >> vr >> hd >> vd >> resolution >> h_size 
	>> v_size >> x_offset >> y_offset >> x_picture_offset 
	>> y_picture_offset;
}


using namespace std;


typedef Token_Type Tag_Option;

typedef double Coord;

struct Tag_Data
{
  string tag;
  
  Token_Type sense;
  double xoffset;
  double yoffset;
};

struct Node_Data
{
  int number;
  string name; 
  double x, y; 
  double hr, vr;
  bool shadow;
  bool without;

  double xoffset, yoffset;

  DynDlist<Tag_Data> tag_list;

  Node_Data() :  hr(::hr), vr(::vr), shadow(false), without(false)
  {
    // empty
  }
};


# define X(p)       ((p)->get_info().x)
# define Y(p)       ((p)->get_info().y)
# define HR(p)      ((p)->get_info().hr)
# define VR(p)      ((p)->get_info().vr)
# define SHADOW(p)  ((p)->get_info().shadow)
# define WITHOUT(p) ((p)->get_info().without)
# define NUMBER(p)  ((p)->get_info().number)
# define STRING(p)  ((p)->get_info().name)
# define XOFFSET(p) ((p)->get_info().xoffset)
# define YOFFSET(p) ((p)->get_info().yoffset)
# define TAGS(p)    ((p)->get_info().tag_list)

# define DYNARRAY_APPEND(array, item) (array[array.size()] = item)


struct Arc_Data
{
  string data;
  double xoffset;
  double yoffset;
  bool shadow;
  bool dashed;
  bool curve;
  bool left;
  int  curve_mid;

  Arc_Data() 
    : xoffset(0), yoffset(0), 
      shadow(false), dashed(false), curve(false), left(false)
  {
    // empty
  }
};


# define STRING_ARC(a)   ((a)->get_info().data)
# define XOFFSET_ARC(a)  ((a)->get_info().xoffset)
# define YOFFSET_ARC(a)  ((a)->get_info().yoffset)
# define SHADOW_ARC(a)   ((a)->get_info().shadow)
# define DASHED_ARC(a)   ((a)->get_info().dashed)
# define CURVE_ARC(a)    ((a)->get_info().curve)
# define CURVE_LEFT(a)   ((a)->get_info().left)
# define CURVE_MID(a)    ((a)->get_info().curve_mid)


typedef Graph_Node<Node_Data> Node;

typedef Graph_Arc<Arc_Data> Arc;

typedef List_Graph<Node, Arc> Graph;

typedef List_Digraph<Node, Arc> Digraph;

DynTreapTree<string, Graph::Node*> node_table;

size_t num_nodes = 0;

DynArray<Graph::Node*> nodes;

DynArray<Graph::Arc*> arcs;


Token_Type get_token(ifstream & input_stream)
{
  char buffer[Buffer_Size];
  char * start_addr = buffer;
  char * end_addr   = buffer + Buffer_Size;

  int c;

  init_token_scanning();

  try 
    {
      skip_white_spaces(input_stream);
      c = read_char_from_stream(input_stream); 
    }
  catch (out_of_range) 
    {
      return END_FILE; 
    }

  if (c == EOF)
    return END_FILE;

  if (not isprint(c))
    return INVALID;

  if (c == '%')
    { /* comentario */
      do 
	c = read_char_from_stream(input_stream); 
      while (c != '\n' and c != EOF);
      return COMMENT;
    }

  do /* delimita una cadena de caracteres cualquiera delimitada por blancos */
    {
      put_char_in_buffer(start_addr, end_addr, c);
      c = read_char_from_stream(input_stream);
    }
  while (isgraph(c) and c != '%' and c != EOF);

  close_token_scanning(buffer, start_addr, end_addr);

  if (c == '%') /* Se encontró comentario */
        /* retroceda, así proxima llamada detectara comentario */
    input_stream.unget(); 

  if (strcasecmp(buffer, "NODE") == 0)
    return NODE;

  if (strcasecmp(buffer, "SHADOW-NODE") == 0)
    return SHADOW_NODE;

  if (strcasecmp(buffer, "WITHOUT-NODE") == 0)
    return WITHOUT_NODE;

  if (strcasecmp(buffer, "HRADIO") == 0)
    return HRADIO;

  if (strcasecmp(buffer, "VRADIO") == 0)
    return VRADIO;

  if (strcasecmp(buffer, "ARC") == 0)
    return ARC;

  if (strcasecmp(buffer, "NODE-TEXT") == 0)
    return NODE_TEXT;

  if (strcasecmp(buffer, "ARC-TEXT") == 0)
    return ARC_TEXT;

  if (strcasecmp(buffer, "DASHED-ARC") == 0)
    return DASHED_ARC;

  if (strcasecmp(buffer, "SHADOW-ARC") == 0)
    return SHADOW_ARC;

  if (strcasecmp(buffer, "CURVE-ARC") == 0)
    return CURVE_ARC;

  if (strcasecmp(buffer, "ARC-TEXT") == 0)
    return ARC_TEXT;
  
  if (strcasecmp(buffer, "SHADOW-CURVE-ARC") == 0)
    return SHADOW_CURVE_ARC;

  if (strcasecmp(buffer, "DASHED-CURVE-ARC") == 0)
    return DASHED_CURVE_ARC;

  if (strcasecmp(buffer, "SHADOW-PATH") == 0)
    return SHADOW_PATH;

  if (strcasecmp(buffer, "TAG") == 0)
    return TAG;

  if (strcasecmp(buffer, "N") == 0)
    return NORTH;

  if (strcasecmp(buffer, "S") == 0)
    return SOUTH;

  if (strcasecmp(buffer, "E") == 0)
    return EAST;

  if (strcasecmp(buffer, "L") == 0)
    return LEFT;

  if (strcasecmp(buffer, "R") == 0)
    return RIGHT;

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

  if (strcasecmp(buffer, "GRAPH") == 0)
    return GRAPH;

  if (strcasecmp(buffer, "DIGRAPH") == 0)
    return DIGRAPH;

  if (strcasecmp(buffer, "POLY-DIGRAPH") == 0)
    return POLY_DIGRAPH;

  if (strcasecmp(buffer, "POLY-GRAPH") == 0)
    return POLY_GRAPH;

  if (strcasecmp(buffer, "NET-DIGRAPH") == 0)
    return NET_DIGRAPH;

  if (strcasecmp(buffer, "NET-GRAPH") == 0)
    return NET_GRAPH;

  if (strcasecmp(buffer, "CROSS-NET-DIGRAPH") == 0)
    return CROSS_NET_DIGRAPH;

  if (strcasecmp(buffer, "CROSS-NET-GRAPH") == 0)
    return CROSS_NET_GRAPH;

  return STRING;
}


Graph * build_poly_graph(ifstream & input_stream, Graph * g)
{
  try
    {
      const int side_size = load_number(input_stream);

      const double arc = load_number(input_stream);

      const Regular_Polygon 
	poly(Point(side_size, side_size), side_size, num_nodes, arc);

      for (int i = 0; i < num_nodes; ++i)
	{
	  Graph::Node * p = nodes[i];

	  const Point pt = poly.get_vertex(i);

	  X(p) = pt.get_x().get_d();
	  Y(p) = pt.get_y().get_d();
	}
    }

  catch (domain_error)
    {
      AH_ERROR("Expecting for side-size or an arc");
    }

  return g;
}



    // NET-GRAPH num-nodes num-levels x-dist y-dist 
Graph * build_net_graph(ifstream & input_stream, Graph * g)
{
  try
    {
      const int num_levels = load_number(input_stream);

      const double xdist = load_number(input_stream);

      const double ydist = load_number(input_stream);

      const size_t & num_nodes = g->get_num_nodes();

      const int nodes_by_level = num_nodes/num_levels;
      
      double y = 0;
      for (int level = 0, i = 0; level < num_levels and i < num_nodes; ++level)
	{
	  double x = 0;

	  for (int j = 0; j < nodes_by_level and i < num_nodes; ++j, ++i)
	    {
	      Graph::Node * p = nodes[i];

	      X(p) = x;
	      Y(p) = y;

	      x += xdist;
	    }

	  y += ydist;
	}
    }
  
  catch (domain_error)
    {
      AH_ERROR("Expecting for num-of-levels or a distance");
    }
  
  return g;
}



    // CROSS-NET-GRAPH num-nodes nodes-by-level x-dist y-dist 
Graph * build_cross_net_graph(ifstream & input_stream, Graph * g)
{
  try
    {
      const int nodes_by_level = load_number(input_stream);

      const double xdist = load_number(input_stream);

      const double ydist = load_number(input_stream);

      const size_t & num_nodes = g->get_num_nodes();

      double y = 0;
      for (int i = 0; i < num_nodes; /* nothing */)
	{
	  double x = xdist/2;

	  for (int j = 0; j < nodes_by_level - 1 and i < num_nodes; ++j, ++i)
	    {
	      Graph::Node * p = nodes[i];

	      X(p) = x;
	      Y(p) = y;

	      x += xdist;
	    }

	  x = 0;
	  y += ydist;

	  for (int j = 0; j < nodes_by_level and i < num_nodes; ++j, ++i)
	    {
	      Graph::Node * p = nodes[i];

	      X(p) = x;
	      Y(p) = y;

	      x += xdist;
	    }

	  y += ydist;
	}
    }
  
  catch (domain_error)
    {
      AH_ERROR("Expecting for num-of-levels or a distance");
    }
  
  return g;
}


/*
  GRAPH num-nodes

  POLY-GRAPH num_nodes side-size rotation

  NET-GRAPH num-nodes num-levels x-dist y-dist 

  CROSS-NET-GRAPH num-nodes num-levels x-dist y-dist 
*/
Graph * parse_graph_definition(ifstream & input_stream)
{
  const Token_Type token = get_token(input_stream);

  if (token != GRAPH and token != DIGRAPH and
      token !=	POLY_DIGRAPH and token != POLY_GRAPH and 
      token != NET_GRAPH and token != NET_DIGRAPH and
      token != CROSS_NET_GRAPH and token != CROSS_NET_DIGRAPH)
    print_parse_error_and_exit("Input does not start with GRAPH definition");

  Graph * g = (token == GRAPH or token == POLY_GRAPH or 
	       token == NET_GRAPH or token == CROSS_NET_GRAPH) 
    ? new Graph : new Digraph;  

  try
    {
      num_nodes = load_number(input_stream); 
    }
  catch (...)
    {
      AH_ERROR("Expecting for number of nodes");
    }

  for (int i = 0; i < num_nodes; i++)
    {
      Graph::Node * p = new Graph::Node;
      nodes[i] = g->insert_node(p);
      string n = to_string(i);
      p->get_info().name = n;
      NUMBER(p) = i;
    }

  if (token == GRAPH or token == DIGRAPH)
    return g;

  if (token == POLY_GRAPH or token == POLY_DIGRAPH)
    return build_poly_graph(input_stream, g);

  if (token == NET_GRAPH or token == NET_DIGRAPH)
    return build_net_graph(input_stream, g);

    return build_cross_net_graph(input_stream, g);
}


Graph::Node * load_node(ifstream & input_stream)
{
  int node_number = load_number(input_stream);

  if (node_number >= num_nodes)
    AH_ERROR("Node number out of range (%d)", num_nodes);

  Graph::Node * p = nodes[node_number];

  return p;
}


/*
  Node i string x y
*/
void parse_node_definition(ifstream & input_stream)
{
  Graph::Node * p = load_node(input_stream);

  Node_Data & node_data = p->get_info();

  node_data.name = load_string(input_stream);

  node_table.insert(node_data.name, p);

  node_data.x = load_number(input_stream);

  node_data.y = load_number(input_stream);
}

    // TAG node-number string sentido xoffset yoffset
void parse_tag_definition(ifstream & input_stream)
{
  Graph::Node * p = load_node(input_stream);

  Tag_Data tag_data;

  tag_data.tag = load_string(input_stream);
  tag_data.sense = get_token(input_stream);

  if (tag_data.sense < NORTH or tag_data.sense > SOUTH_WEST)
    print_parse_error_and_exit("Invalid tag option found");

  tag_data.xoffset = load_number(input_stream); // xoffset
  tag_data.yoffset = load_number(input_stream); // yoffset

  TAGS(p).append(tag_data);
}

void parse_without_node_definition(ifstream & input_stream)
{
  Graph::Node * p = load_node(input_stream);

  WITHOUT(p) = true;
}


void parse_shadow_node_definition(ifstream & input_stream)
{
  Graph::Node * p = load_node(input_stream);

  SHADOW(p) = true;
}


void load_nodes(ifstream & input_stream, 
		Graph::Node *& src_node, 
		Graph::Node *& tgt_node)
{
  int num_src = load_number(input_stream);

  if (num_src >= num_nodes)
    AH_ERROR("source node %d out of range (%d)", num_src, num_nodes);

  int num_tgt = load_number(input_stream);

  if (num_tgt >= num_nodes)
    AH_ERROR("Target node %d out of range (%d)", num_tgt, num_nodes);

  src_node = nodes[num_src];
  tgt_node = nodes[num_tgt];
}


/*
  ARC number-node number-tgt-node 
*/
Graph::Arc * parse_arc_definition(ifstream & input_stream, Graph * g)
{
  Graph::Node * src_node = NULL;
  Graph::Node * tgt_node = NULL;

  load_nodes(input_stream, src_node, tgt_node);

  Graph::Arc * arc = search_arc(*g, src_node, tgt_node);

  if (arc == NULL)
    {
      Graph::Arc * arc = g->insert_arc(src_node, tgt_node, Arc_Data());

      DYNARRAY_APPEND(::arcs, arc);

      return arc;
    }

  return arc;
}


/*
  NODE-TEXT number-node Text xoffset yoffset
*/
Graph::Node * parse_node_text_definition(ifstream & input_stream)
{
  Graph::Node * p = load_node(input_stream);

  STRING(p)  = load_string(input_stream);
  XOFFSET(p) = load_number(input_stream);
  YOFFSET(p) = load_number(input_stream);

  return p;
}


/*
  ARC-TEXT number-node number-tgt-node Text xoffset yoffset
*/
Graph::Arc * parse_arc_text_definition(ifstream & input_stream, Graph * g)
{
  Graph::Node * src_node = NULL;
  Graph::Node * tgt_node = NULL;

  load_nodes(input_stream, src_node, tgt_node);

  Graph::Arc * a = search_arc(*g, src_node, tgt_node);

  if (a == NULL)
    AH_ERROR("Arc not found");

  STRING_ARC(a)  = load_string(input_stream);
  XOFFSET_ARC(a) = load_number(input_stream);
  YOFFSET_ARC(a) = load_number(input_stream);

  return a;
}


/*  CURVED-ARC src tgt mid-point sentido */
Graph::Arc * parse_curve_arc_definition(ifstream & input_stream, Graph * g)
{
  Graph::Arc * arc = parse_arc_definition(input_stream, g);

  CURVE_ARC(arc) = true;

  CURVE_MID(arc) = load_number(input_stream);

  const Token_Type token = get_token(input_stream);

  if (token == LEFT)
    CURVE_LEFT(arc) = true;
  else if (token == RIGHT)
    CURVE_LEFT(arc) = false;
  else
    AH_ERROR("Invalid sense in curved-arc definition");

  return arc;
}


Graph::Arc * parse_shadow_arc_definition(ifstream & input_stream, Graph * g)
{
  Graph::Arc * arc = parse_arc_definition(input_stream, g);

  SHADOW_ARC(arc) = true;

  return arc;
}


Graph::Arc * parse_dashed_arc_definition(ifstream & input_stream, Graph * g)
{
  Graph::Arc * arc = parse_arc_definition(input_stream, g);

  DASHED_ARC(arc) = true;

  return arc;
}


/*  SHADOW-CURVED-ARC src tgt mid-point sentido */
    Graph::Arc * 
parse_shadow_curve_arc_definition(ifstream & input_stream, Graph * g)
{
  Graph::Arc * arc = parse_curve_arc_definition(input_stream, g);

  SHADOW_ARC(arc) = true;

  return arc;
}


/*  DASHED-CURVED-ARC src tgt mid-point sentido */
    Graph::Arc * 
parse_dashed_curve_arc_definition(ifstream & input_stream, Graph * g)
{
  Graph::Arc * arc = parse_curve_arc_definition(input_stream, g);

  DASHED_ARC(arc) = true;

  return arc;
}


    // HRADIO node-number radio
void parse_hradio_definition(ifstream & input_stream)
{
  Graph::Node * p = load_node(input_stream);

  HR(p) = load_number(input_stream);
}


    // VRADIO node-number radio
void parse_vradio_definition(ifstream & input_stream)
{
  Graph::Node * p = load_node(input_stream);

  VR(p) = load_number(input_stream);
}


Graph * read_input_and_build_graph(ifstream & input_stream)
{
  Graph * g = parse_graph_definition(input_stream);

  try
    {
      while (true)
	switch (get_token(input_stream))
	  {
	  case END_FILE:
	    return g; 

	  case INVALID:
	    print_parse_error_and_exit("Unrecognized token");

	  case COMMENT: break;

	  case NODE:
	    parse_node_definition(input_stream);
	    break;

	  case TAG:
	    parse_tag_definition(input_stream);
	    break;

	  case WITHOUT_NODE:
	    parse_without_node_definition(input_stream);
	    break;

	  case SHADOW_NODE:
	    parse_shadow_node_definition(input_stream);
	    break;

	  case ARC:
	    parse_arc_definition(input_stream, g);
	    break;
	
	  case SHADOW_ARC:
	    parse_shadow_arc_definition(input_stream, g);
	    break;

	  case DASHED_ARC:
	    parse_dashed_arc_definition(input_stream, g);
	    break;

	  case NODE_TEXT:
	    parse_node_text_definition(input_stream);
	    break;

	  case ARC_TEXT:
	    parse_arc_text_definition(input_stream, g);
	    break;

	  case CURVE_ARC:
	    parse_curve_arc_definition(input_stream, g);
	    break;
	
	  case SHADOW_CURVE_ARC:
	    parse_shadow_curve_arc_definition(input_stream, g);
	    break;
	
	  case DASHED_CURVE_ARC:
	    parse_dashed_curve_arc_definition(input_stream, g);
	    break;
	
	  case HRADIO:
	    parse_hradio_definition(input_stream);
	    break;
	
	  case VRADIO:
	    parse_hradio_definition(input_stream);
	    break;
	
	  default:
	    print_parse_error_and_exit("Unknown token type");
	  }
    }
  catch (exception & e)
    {
      delete g;
      print_parse_error_and_exit(e.what());
    }
  
  return NULL; // nunca se alcanza
}


void generate_prologue(ofstream & output)
{
  time_t t;
  time(&t);
  output << endl
	 << "%      This LaTeX picture is a graph automatically" << endl
	 << "%      generated by graphpic program" << endl
	 << endl
	 << "% Copyright (C) 2007" << endl
	 << "% UNIVERSITY of LOS ANDES (ULA)" << endl
	 << "% Merida - REPUBLICA BOLIVARIANA DE VENEZUELA" << endl
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
	   << endl
	   << "\\usepackage{curves}"<< endl
	   << "\\usepackage{epic}" << endl
	   << "\\usepackage{eepic}" << endl
	   << endl
	   << "\\begin{document}" << endl
	   << "\\begin{center}" << endl;
}


    // retorna un segmento desde el nodo src hasta el nodo tgt. Los extremos
    // corresponden con la intersección de la linea con los bordes de la
    // elipse 
Segment arc_segment(Graph::Node * src, Graph::Node * tgt)
{
      // centros de las elipses de los nodos
  const Point src_center(X(src), Y(src));
  const Point tgt_center(X(tgt), Y(tgt));

  const Segment l(src_center, tgt_center); // segmento entre los centros

      // ellipses de nodos
  const Ellipse src_el(src_center, HR(src), VR(src));
  const Ellipse tgt_el(tgt_center, HR(tgt), VR(tgt));

      // calcula las intersecciones de la recta que pasa por los puntos
      // centrales de las elipses
  const Segment src_sg = src_el.intersection_with(l);
  const Segment tgt_sg = tgt_el.intersection_with(l);

      // selecciona como punto de intersección el que esté más cercano
      // al centro de la elipse opuesta
  const Point & src_point = src_sg.nearest_point(tgt_center);

      // selecciona el punto de intersección que sea parte del segmento l
      // entre los centros de la elipse
  const Point & tgt_point = tgt_sg.nearest_point(src_center);

  return Segment(src_point, tgt_point);
}


Polygon arc_trigon(Graph::Node *  src, 
		   Graph::Node *  tgt, 
		   const double & dist,
		   const bool &   left)
{
        // centros de las elipses de los nodos
  const Point src_center(X(src), Y(src));
  const Point tgt_center(X(tgt), Y(tgt));

  const Segment l(src_center, tgt_center); // segmento entre los centros

  const Segment perp = l.mid_perpendicular(dist); // perpendicular que
						  // pasa por el centro

      // selecciona según bandera left el punto medio de la curva
  const Point & mid = left ? perp.get_tgt_point() : perp.get_src_point();


		      // cálculo del punto origen -------

  const Ellipse src_el(src_center, HR(src), VR(src)); // elipse origen
  const Segment src_to_mid(src_center, mid); // segmento centro elipse--mid

      // calcular puntos de intersección entre elipse origen y segmento
      // hacia punto medio
  const Segment src_inter = src_el.intersection_with(src_to_mid); // intersecta 

  const Point & src_pt = src_inter.nearest_point(mid);


		      // cálculo del punto destino -------

  const Ellipse tgt_el(tgt_center, hr, vr); // elipse destino
  const Segment mid_to_tgt(mid, tgt_center); // segmento centro mid--elipse

      // calcular puntos de intersección entre segmento desde el punto
      // medio y la elipse origen 
  const Segment tgt_inter = tgt_el.intersection_with(mid_to_tgt); // intersecta 

  const Point & tgt_pt = tgt_inter.nearest_point(mid);
  
      // calculados los tres puntos, nos resta por construir el polígono
      // abierto 
  Polygon result;

  result.add_vertex(src_pt);
  result.add_vertex(mid);
  result.add_vertex(tgt_pt);

  return result;
}


void process_tag_node(Eepic_Plane & plane, Graph::Node * p)
{
  const double & xp = X(p);
  const double & yp = Y(p);


  for (DynDlist<Tag_Data>::Iterator it(TAGS(p)); it.has_curr(); it.next())
    {
      const Tag_Data & tag_data = it.get_curr();

      Point tag_point(xp, yp);

      tag_point +=  Point(tag_data.xoffset, tag_data.yoffset);

      switch (tag_data.sense)
	{
	case NORTH:
	  {
	    tag_point += Polar_Point(VR(p), PI_2);

	    put_in_plane(plane, Center_Text(tag_point, tag_data.tag));

	    break;
	  }

	case SOUTH:
	  {
	    tag_point += Polar_Point(VR(p), -PI_2);

	    put_in_plane(plane, Center_Text(tag_point, tag_data.tag));

	    break;
	  }

	case EAST: 
	  {
	    tag_point += Polar_Point(HR(p), 0);

	    put_in_plane(plane, Left_Text(tag_point, tag_data.tag));

	    break;
	  }

	case WEST: 
	  {
	    tag_point += Polar_Point(HR(p), PI);

	    put_in_plane(plane, Right_Text(tag_point, tag_data.tag));

	    break;
	  }

	case NORTH_EAST:
	  {
	    tag_point += Polar_Point(pitag(HR(p), VR(p)), PI_4);

	    put_in_plane(plane, Left_Text(tag_point, tag_data.tag));

	    break;
	  }

	case NORTH_WEST:
	  {
	    tag_point += Polar_Point(- pitag(HR(p), VR(p)), -PI_4);

	    put_in_plane(plane, Right_Text(tag_point, tag_data.tag));

	    break;
	  }

	case SOUTH_EAST:
	  {
	    tag_point += Polar_Point(pitag(HR(p), VR(p)), -PI_4);

	    put_in_plane(plane, Left_Text(tag_point, tag_data.tag));

	    break;
	  }

	case SOUTH_WEST:
	  {
	    tag_point += Polar_Point(- pitag(HR(p), VR(p)), PI_4);

	    put_in_plane(plane, Right_Text(tag_point, tag_data.tag));

	    break;
	  }

	default:
	  AH_ERROR("(internal) invalid tag sense option %ld", tag_data.sense);
	}
    }

}


void process_node(Eepic_Plane & plane, Graph::Node * p)
{
  if (draw_node_mode and not WITHOUT(p))
    {
      if (SHADOW(p))
	put_in_plane(plane, Thick_Ellipse(Point(X(p), Y(p)), HR(p), VR(p)));
      else 
	put_in_plane(plane, Ellipse(Point(X(p), Y(p)), HR(p), VR(p)));
    }

  put_in_plane(plane, Center_Text(Point(X(p), Y(p)), STRING(p)));

  process_tag_node(plane, p);
}


    void 
process_text_arc(Eepic_Plane & plane, Graph::Arc * a, const Segment & arc_sg)
{
  const string & text = STRING_ARC(a);

  if (text == "")
    return;

  const Point offset(XOFFSET_ARC(a), YOFFSET_ARC(a));

  const Point mid_point = arc_sg.mid_point() + offset;

  switch (arc_sg.sense())
    {
    case Segment::E:
      {
	const double ydiff = vr/2; 

	const Point pos = Point(0, ydiff);

	put_in_plane(plane, Center_Text(mid_point + pos, text));

	break;
      }

    case Segment::W:
      {
	const double ydiff = 0.6*vr; 

	const Point pos = Point(0, -ydiff);

	put_in_plane(plane, Center_Text(mid_point + pos, text));

	break;
      }

    case Segment::N:
      put_in_plane(plane, Left_Text(mid_point, text));

      break;

    case Segment::S:
      put_in_plane(plane, Right_Text(mid_point, text));

      break;

    case Segment::NE:
    case Segment::SE:
    case Segment::NW:
    case Segment::SW:
      {
	const double xdiff = vr/2; 

	const Point pos = Point(xdiff, 0);

	put_in_plane(plane, Left_Text(mid_point + pos, text));

	break;
      }

    default: AH_ERROR("(Internal) invalid sense option");
    }
}


void process_text_arc(Eepic_Plane & plane, Graph::Arc * a, Polygon & trigon)
{
  if (STRING_ARC(a) == "")
    return;

      // la idea es calcular un segmento paralelo al que conectaría los
      // nodos y luego invocar a la rutina anterior sobre ese segmento
      // paralelo 
  const Vertex & first_vt = trigon.get_first_vertex();

      // segmento que conectaría a los nodos si no hubiese un arco
  const Segment arc_sg(first_vt, trigon.get_last_vertex());

      // punto por donde debe pasar el segmento paralelo
  const Point & second_pt = first_vt.next_vertex();

  const Geom_Number dist = second_pt.distance_with(arc_sg.mid_point());

  const Segment par_sg(arc_sg, dist); // segmento paralelo

  process_text_arc(plane, a, par_sg);
}


void process_arc(Eepic_Plane & plane, Graph * g, Graph::Arc * a)
{
  Graph::Node * src_node = g->get_src_node(a);
  Graph::Node * tgt_node = g->get_tgt_node(a);

  if (CURVE_ARC(a)) // ¿es el arco una curva?
    {
      Polygon poly = // determine el trio de puntos que la definen
	arc_trigon(src_node, tgt_node, CURVE_MID(a), CURVE_LEFT(a));

      process_text_arc(plane, a, poly);

      if (g->is_digraph())
	{
	  if (SHADOW_ARC(a))
	    {
	      if (DASHED_ARC(a))
		put_in_plane(plane, Thick_Dash_Polygon_With_Arrow(poly));
	      else
		put_in_plane(plane, Thick_Spline_Arrow(poly));
	    }
	  else
	    {
	      if (DASHED_ARC(a))
		put_in_plane(plane, Dash_Polygon_With_Arrow(poly));
	      else
		put_in_plane(plane, Spline_Arrow(poly));
	    }
	}
      else
	{
	  if (SHADOW_ARC(a))
	    {
	      if (DASHED_ARC(a))
		put_in_plane(plane, Thick_Dash_Polygon(poly));
	      else
		put_in_plane(plane, Thick_Spline(poly));
	    }
	  else
	    {
	      if (DASHED_ARC(a))
		put_in_plane(plane, Dash_Polygon(poly));
	      else
		put_in_plane(plane, Spline(poly));
	    }
	}

      return;
    }
      
  const Segment sg = arc_segment(src_node, tgt_node);

  process_text_arc(plane, a, sg);
      
  if (g->is_digraph())
    {
      if (SHADOW_ARC(a))
	{
	  if (DASHED_ARC(a))
	    put_in_plane(plane, Thick_Dash_Arrow(sg));
	  else
	    put_in_plane(plane, Thick_Arrow(sg));
	}
      else
	{
	  if (DASHED_ARC(a))
	    put_in_plane(plane, Dash_Arrow(sg));
	  else
	    put_in_plane(plane, Arrow(sg));
	}
    }
  else
    {
      if (SHADOW_ARC(a))
	{
	  if (DASHED_ARC(a))
	    put_in_plane(plane, Thick_Dash_Segment(sg));
	  else
	    put_in_plane(plane, Thick_Segment(sg));
	}
      else
	{
	  if (DASHED_ARC(a))
	    put_in_plane(plane, Dash_Segment(sg));
	  else
	    put_in_plane(plane, sg);
	}
    }
}


void generate_picture(ofstream & output, Graph * g)
{
  Eepic_Plane plane(h_size, v_size, x_picture_offset, y_picture_offset); 

  for (Graph::Node_Iterator it(*g); it.has_curr(); it.next())
    process_node(plane, it.get_current_node());

  for (int i = 0; i < ::arcs.size(); ++i)
    process_arc(plane, g, ::arcs[i]);

  plane.zoom(zoom_factor);

  plane.draw(output, squarize);
}


void generate_epilogue(ofstream & output)  
{
  if (latex_header)
    output << endl
	   << "\\end{center}" << endl
	   << "\\end{document}" << endl;
}


void generate_graph(Graph * g, ofstream & output)
{
  generate_prologue(output);

  generate_picture(output, g);

  generate_epilogue(output);
}


const char *argp_program_version = 
"graphpic 1.0b\n"
"ALEPH drawer for graphs\n"
"Copyright (C) 2007 UNIVERSITY of LOS ANDES (ULA)\n"
"Merida - REPUBLICA BOLIVARIANA DE VENEZUELA\n"
"Center of Studies in Microelectronics & Distributed Systems (CEMISID)\n"
"ULA Computer Science Department\n"
"This is free software; There is NO warranty; not even for MERCHANTABILITY\n"
"or FITNESS FOR A PARTICULAR PURPOSE\n"
"\n";

const char *argp_program_bug_address = "lrleon@ula.ve";

static char doc[] = "graphpic -- Aleph drawer for graphs";

static char argDoc[] = "-f input-file\n";

static const char license_text [] = 
"Aleph drawer for graphs. License & Copyright Note\n"
"Copyright (C) 2007\n"
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
  {"radius", 'r', "radius", OPTION_ARG_OPTIONAL, "fit radius for circles", 0},
  {"h-rad", 'x', "hor-radius", OPTION_ARG_OPTIONAL, "horizontal radius", 0},
  {"width", 'W', "picture-width", OPTION_ARG_OPTIONAL, "picture width", 0},
  {"height", 'H', "picture-height", OPTION_ARG_OPTIONAL, "picture height", 0},
  {"v-radius", 'y', "vert-radius", OPTION_ARG_OPTIONAL, "vertical  radius", 0},
  {"resol", 'l', "resolution", OPTION_ARG_OPTIONAL, "resolution in mm", 0},
  {"latex", 'a', 0, OPTION_ARG_OPTIONAL, "add latex header", 0},
  {"without-node", 'N', 0, OPTION_ARG_OPTIONAL, "no draw nodes", 0},
  {"key-x-offset",'X',"offset", OPTION_ARG_OPTIONAL,"horizontal key offset", 0},
  {"key-y-offset",'Y', "offset", OPTION_ARG_OPTIONAL, "vertical key offset", 0},
  {"input-file", 'i', "input-file", 0, "input file", 0},
  {"input-file", 'f', "input-file", OPTION_ALIAS, 0, 0},
  {"output", 'o', "output-file", 0, "output file", 0},
  {"license", 'C', 0, OPTION_ARG_OPTIONAL, "print license", 0},
  {"x-pic-offset",'O',"x-pic-offset",OPTION_ARG_OPTIONAL,"x picture offset",0},
  {"y-pic-offset",'P',"y-pic-offset",OPTION_ARG_OPTIONAL,"y picture offset", 0},
  {"print", 'R', 0, OPTION_ARG_OPTIONAL, "print current parameters", 0},
  {"tiny-keys", 't', 0, OPTION_ARG_OPTIONAL, "with tiny keys", 0},
  {"version", 'V', 0, OPTION_ARG_OPTIONAL, "Print version information", 0},
  {"squarize", 'S', 0, OPTION_ARG_OPTIONAL, "fits picture without scalate", 0},
  {"zoom", 'Z', "zoom-factor", OPTION_ARG_OPTIONAL, "zoom picture", 0},
  { 0, 0, 0, 0, 0, 0 } 
}; 

static const char * hello = 
"ALEPH drawer of graphs\n"
"Copyright (C) 2007 University of Los Andes (ULA)\n"
"Merida - REPUBLICA BOLIVARIANA DE VENEZUELA\n"
"Center of Studies in Microelectronics & Distributed Systems (CEMISID)\n"
"ULA Computer Science Department\n"
"This is free software; There is NO warranty; not even for MERCHANTABILITY\n"
"or FITNESS FOR A PARTICULAR PURPOSE\n"
"\n";


static error_t parser_opt(int key, char *arg, struct argp_state *)
{
  switch (key)
    {
    case 'r': /* Especificacion de radio */
      if (arg == NULL)
	AH_ERROR("Waiting for radius in command line");

      hr = vr = atof(arg); hd = vd = 2*hr;

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

    case 'Z': /* radio horizontal de elipse */
      if (arg == NULL)
	AH_ERROR("Waiting for picture width in command line");
      
      zoom_factor = atof(arg);

      break;

    case 'W': /* radio horizontal de elipse */
      if (arg == NULL)
	AH_ERROR("Waiting for picture width in command line");
      
      h_size = atof(arg);

      break;

    case 'H': /* radio horizontal de elipse */
      if (arg == NULL)
	AH_ERROR("Waiting for picture height in command line");

      v_size = atof(arg);

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

    case 'S':
      squarize = false;
      break;

    case 'u':
      if (arg == NULL)
	AH_ERROR("Waiting for vertical size in command line");

      v_size = atof(arg);

      break;

    case 'n': /* coloca el radio minimo */
      hr = vr = resolution/2; hd = vd = resolution;  

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

    case 'v': /* verbose mode (no implementado) */ 
      break;

    case 'i': /* archivo de entrada */
    case 'f':
      {
	if (arg == NULL)
	  AH_ERROR("Waiting for input file name");

	input_file_name  = arg;
	int pos = input_file_name.rfind(".");

	if (pos == string::npos)
	  output_file_name = input_file_name + ".eepic";
	else
	  output_file_name = 
	    string(input_file_name).replace(pos, input_file_name.size() - pos, 
					    string(".eepic"));

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

    case 't': 
      tiny_keys = true;
      break;

    case 'R': /* imprime parametros */ 
      print_parameters();
      save_parameters();
      TERMINATE(0);

      break;

    case 'V': /* imprimir version */ 
      cout << argp_program_version;
      TERMINATE(0);

      break;
      
    default: return ARGP_ERR_UNKNOWN;
    }

  return 0;
}


static struct argp arg_defs = { options, parser_opt, argDoc, doc, 0, 0, 0 }; 


int main(int argc, char *argv[]) 
{
  command_line = command_line_to_string(argc, argv);

  read_parameters();

  argp_parse(&arg_defs, argc, argv, ARGP_IN_ORDER, 0, NULL);

  if (input_file_name.size() == 0)
    AH_ERROR("Input file not given");

  ifstream input_stream(input_file_name.c_str());

  if (not input_stream)
    AH_ERROR("%s file does not exist", input_file_name.c_str());

  cout << hello;

  cout << "input from " << input_file_name << " file " << endl;

  if (output_file_name.size() == 0)
    AH_ERROR("Output file not given");

  ofstream output_stream(output_file_name.c_str(), ios::out);

  cout << "output sent to " << output_file_name << " file " << endl << endl;

  Graph * g = read_input_and_build_graph(input_stream);

  generate_graph(g, output_stream);
  
  save_parameters();
}
