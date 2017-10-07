
# include <iostream>
# include <fstream>
# include <string>
# include <tpl_dynArray.H>
# include <tpl_dynDlist.H>
# include <tpl_tree_node.H>

# include <argp.h>

# include "parse_utils.H"
# include "treepic_utils.H"

/*

  TODO:

  opcion connexion complementaria a dashed-connexion

  URGENTE: 
    -. Pasos de diferentes árboles
    -. Notación de Deway pura

  -. Mejorar heurística de cálculo de longitud string para que
     considere saltos de línea  
  -. Conversión en representación árbol binario
  -. Dibujar flecha
  -. Opcion que dibuje lineas punteadas cuando se ejecute opcion que
     dibuje reptresentacion con listas
*/


# define BLANK " "

extern bool tiny_keys;

    /* valores de distancias por omision */
long double hr = 70;  // radio horizontal de la elipse
long double vr = 70;  // radio vertical de la elipse
long double hd = 2*hr;  // diametro horizontal de la elipse
long double vd = 2*vr;  // diametro vertical de la elipse
long double xgap = 70;  // separacion horizontal entre nodos
long double ygap = 100; // separacion vertical entre nodos
long double tree_gap = 90; // separacion horizontal entre arboles
long double h_size = 0; /* longitud horizontal de picture */
double v_size = 0; /* longitud vertical de picture */
long double x_offset = 0; /* offset horizontal etiqueta */
long double y_offset = font_height()/2.0; /* offset vertical etiqueta */
long double x_picture_offset = 0; // offset horizontal para dibujo
long double y_picture_offset = 0; // offset vertical para dibujo

string command_line;
string input_file_name;
string output_file_name;


bool latex_header = false; // envolver salida con un latex header
bool ellipses = true; // por omisión dibujar elipses 
bool rectangles = false; 
bool draw_list_representation = false;
bool generate_binary_tree = false;
bool not_nodes = false;


using namespace std;

enum Token_Type 
  { 
    ROOT, NODE, STRING, WITHOUT_NODE, WITHOUT_ARC, XOFFSET, YOFFSET, 
    HRADIO, VRADIO, SHADOW, TAG, ARC, DASHED_ARC, DASHED_CONNEXION, ELLIPSE, 
    RECTANGLE, COMMENT, END_FILE, 
    NORTH,      // N
    SOUTH,      // S
    EAST,       // E
    WEST,       // W
    NORTH_EAST, // NE
    NORTH_WEST, // NW
    SOUTH_EAST, // SE
    SOUTH_WEST,  // SW
    LEFT,        // L
    RIGHT,       // R
    INVALID
  };


typedef Token_Type Tag_Option;

struct Tree_Data; /* declaración adelantada */

typedef Tree_Node<Tree_Data> EepicNode;

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


struct Arc_Data
{
  EepicNode * target_node;
  bool       is_dashed;

  Arc_Data() : target_node(NULL), is_dashed(false) { /* empty */ }
};


struct Connexion_Data
{
  EepicNode * target_node;
  bool        is_dashed;
  bool        is_left;

  Connexion_Data() : target_node(NULL), is_dashed(false) { /* empty */ }
};


struct Tree_Data
{
  long double x, y; /* coordenadas x,y definitivas del nodo */

  long double pre, mod; /* valores x temporales empleados por algoritmo
			   dibujado */

  long double sum_mod; /* acumulado de mod desde la raiz hasta el
			  nodo. Usado por el recorrido prefijo para colocar
			  los valores definitivos de x */

  long double xr, yr; /* radios de elipse */

  long double xd, yd; /* diametros de elipse */

  long double max_child_yr; /* máximo radio vertical de sus hijos. Usado
			       para reajustar coordenadas y de los hijos si
			       algunos de los diametros son variables */

  long double yr_gap; /* máximo radio entre los hermanos. Usado para
			 calcular separación vertical de los hijos de este
			 nodo de modo que queden alineados con el mayor
			 radio */

  string str; /* cadena asociada al nodo */

  long double xoffset, yoffset; /* offsets de posicion de cadena */

  bool ellipse;
  bool rectangle;

  bool shadow;
  bool without_node;
  bool with_arc;   /* indica si hay arco desde el nodo padre */
  bool dashed_arc; /* indica si el arco es punteado */

  int position; /* posicion infija del nodo como si fuera un arbol binario */

  DynDlist<Tag_Data> tag_list;

  DynDlist<Arc_Data> arc_list;
  
  DynDlist<Connexion_Data> connexion_list;

  int level; 
  int child_number; /* de izquierda a derecha */

  Tree_Data ()
    : x(0), y(0), pre(0), mod(0), sum_mod(0),
      xr(hr), yr(vr), xd(2*xr), yd(2*yr), max_child_yr(0),
      xoffset(0), yoffset(0),
      ellipse(ellipses), rectangle(rectangles),
      shadow(false), without_node(false), with_arc(true), dashed_arc(false)
  {
    assert(ellipse != rectangle);
  }
};



# define X(p)           ((p)->get_key().x)
# define Y(p)           ((p)->get_key().y)
# define PRE(p)         ((p)->get_key().pre)
# define MOD(p)         ((p)->get_key().mod)
# define SUMMOD(p)      ((p)->get_key().sum_mod)
# define STRING(p)      ((p)->get_key().str)
# define ISELLIPSE(p)   ((p)->get_key().ellipse)
# define ISRECTANGLE(p) ((p)->get_key().rectangle)
# define XOFFSET(p)     ((p)->get_key().xoffset)
# define YOFFSET(p)     ((p)->get_key().yoffset)
# define XR(p)          ((p)->get_key().xr)
# define YR(p)          ((p)->get_key().yr)
# define XD(p)          ((p)->get_key().xd)
# define YD(p)          ((p)->get_key().yd)
# define POS(p)         ((p)->get_key().position)
# define MAXCHILDYR(p)  ((p)->get_key().max_child_yr)
# define YRGAP(p)       ((p)->get_key().yr_gap)
# define LEVEL(p)       ((p)->get_key().level)
# define CHILDNUMBER(p) ((p)->get_key().child_number)
# define WIDTH(p)       ((p)->get_key().xd)
# define HEIGHT(p)      ((p)->get_key().yd)
# define SHADOW(p)      ((p)->get_key().shadow)
# define WITHOUTNODE(p) ((p)->get_key().without_node)
# define WITHARC(p)     ((p)->get_key().with_arc)
# define DASHEDARC(p)   ((p)->get_key().dashed_arc)
# define ARCS(p)        ((p)->get_key().arc_list)
# define CONNEXIONS(p)  ((p)->get_key().connexion_list)
# define TAGS(p)        ((p)->get_key().tag_list)


    inline
void load_deway_number(ifstream& input_stream, 
		       int deway_array[], size_t deway_array_size)
{
  init_token_scanning();
  skip_white_spaces(input_stream);

  int c;
  int deway_index = 0;
  char deway_string[Buffer_Size];
  char * start_addr = &deway_string[0];
  char * end_addr   = &deway_string[Buffer_Size];
  while (true)
    {
      c = read_char_from_stream(input_stream);

      if (isdigit(c))
	{
	  put_char_in_buffer(start_addr, end_addr, c);
	  continue;
	}

      if (c == '.' or isspace(c))
	{
	  put_char_in_buffer(start_addr, end_addr, '\0');

	  if (deway_index >= deway_array_size)
	    throw overflow_error("Deway number is too long");

	  deway_array[deway_index++] = atoi(deway_string);
	  start_addr = &deway_string[0];

	  if (isspace(c))
	    break;
	  else
	    continue;
	}

      input_stream.unget();
      throw invalid_argument("Unexpected character in deway number");
    }

  if (deway_index >= deway_array_size)
    throw overflow_error("Internal deway array size is not enough");
  deway_array[deway_index] = -1;
}


Token_Type get_token(ifstream& input_stream)
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

  if (strcasecmp(buffer, "ROOT") == 0)
    return ROOT;

  if (strcasecmp(buffer, "NODE") == 0)
    return NODE;

  if (strcasecmp(buffer, "TAG") == 0)
    return TAG;

  if (strcasecmp(buffer, "SHADOW") == 0)
    return SHADOW;

  if (strcasecmp(buffer, "WITHOUT-NODE") == 0)
    return WITHOUT_NODE;

  if (strcasecmp(buffer, "WITHOUT-ARC") == 0)
    return WITHOUT_ARC;

  if (strcasecmp(buffer, "ARC") == 0)
    return ARC;

  if (strcasecmp(buffer, "HRADIO") == 0)
    return HRADIO;

  if (strcasecmp(buffer, "VRADIO") == 0)
    return VRADIO;

  if (strcasecmp(buffer, "XOFFSET") == 0)
    return XOFFSET;

  if (strcasecmp(buffer, "YOFFSET") == 0)
    return YOFFSET;

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

  if (strcasecmp(buffer, "ARC") == 0)
    return ARC;

  if (strcasecmp(buffer, "DASHED-CONNEXION") == 0)
    return DASHED_CONNEXION;

  if (strcasecmp(buffer, "ELLIPSE") == 0)
    return ELLIPSE;

  if (strcasecmp(buffer, "RECTANGLE") == 0)
    return RECTANGLE;

  return STRING;
}

/*
  Aparta memoria para un nuevo nodo con string str
*/
EepicNode * allocate_node(const string & str)
{
  EepicNode * node = new EepicNode;
  STRING(node)     = str;
  return node;
}


/*
  Lee un numero de deway y retorna el correspondiente nodo dentro del
  árbol root. Retorna NULL si el nodo no existe
*/
EepicNode * parse_deway_number(ifstream& input_stream, EepicNode * root)
{
  int deway_array[Buffer_Size];

  load_deway_number(input_stream, deway_array, Buffer_Size);
  
  EepicNode * parent_node = deway_search(root, deway_array, Buffer_Size);

  if (parent_node == NULL)
    print_parse_error_and_exit("Deway number doesn't match an existing node");

  return parent_node;
}


EepicNode * parse_key_node_and_allocate(ifstream& input_stream)
{
  string str = load_string(input_stream);

  return allocate_node(str);
}

 
EepicNode * parse_first_root_definition(ifstream& input_stream)
{
  if (get_token(input_stream) != ROOT)
    print_parse_error_and_exit("Input file doesn't start with ROOT "
			       "definition");

  return parse_key_node_and_allocate(input_stream);
}


void parse_root_definition(ifstream& input_stream, EepicNode * root)
{
  EepicNode * new_tree = parse_key_node_and_allocate(input_stream);

  EepicNode * last_root = root->get_last_tree();
 
  last_root->insert_tree_to_right(new_tree);
}


void parse_node_definition(ifstream& input_stream, EepicNode * root)
{
  EepicNode * parent_node = parse_deway_number(input_stream, root);

  EepicNode * new_node = parse_key_node_and_allocate(input_stream);

  parent_node->insert_rightmost_child(new_node);
}

/*
  opcion HRADIO deway-number %-horizontal-radius-factor

  El radio horizontal actual de la elipse se multiplica por
  horizontal-radius-factor en porcentaje
*/
void parse_hradio(ifstream& input_stream, EepicNode * root)
{
  EepicNode * p = parse_deway_number(input_stream, root);

  XR(p) *= load_number(input_stream)/100.0;
  XD(p) = 2*XR(p);
}


/*
  opcion VRADIO deway-number %-vertical-radius-factor

  El radio vertical actual de la elipse se multiplica por
  vertical-radius-factor en porcentaje
*/
void parse_vradio(ifstream& input_stream, EepicNode * root)
{
  EepicNode * p = parse_deway_number(input_stream, root);

  YR(p) *= load_number(input_stream)/100.0;
  YD(p) = 2*YR(p);
}


/*
  opcion WITHOUT-NODE deway-number

  El radio vertical actual de la elipse se multiplica por
  vertical-radius-factor en porcentaje
*/
void parse_without_node(ifstream& input_stream, EepicNode * root)
{
  EepicNode * p = parse_deway_number(input_stream, root);

  WITHOUTNODE(p) = true;
}


/*
  opcion WITHOUT-NODE deway-number

  El radio vertical actual de la elipse se multiplica por
  vertical-radius-factor en porcentaje
*/
void parse_without_arc(ifstream& input_stream, EepicNode * root)
{
  EepicNode * src = parse_deway_number(input_stream, root);

  EepicNode * tgt = parse_deway_number(input_stream, root);

  if (tgt->get_parent() != src)
    print_parse_error_and_exit("target node does not match with parent in"
			       " WITHOUT-ARC"); 
  WITHARC(tgt) = false;
}


/*
  opcion XOFFSET deway-number offset
*/
void parse_xoffset(ifstream& input_stream, EepicNode * root)
{
  EepicNode * p = parse_deway_number(input_stream, root);

  XOFFSET(p) = load_number(input_stream);
}


/*
  opcion YOFFSET deway-number offset
*/
void parse_yoffset(ifstream& input_stream, EepicNode * root)
{
  EepicNode * p = parse_deway_number(input_stream, root);

  YOFFSET(p) = load_number(input_stream);
}

/*
  opcion SHADOW deway-number 
*/
void parse_shadow(ifstream& input_stream, EepicNode * root)
{
  EepicNode * p = parse_deway_number(input_stream, root);

  SHADOW(p) = true;
}

/*
  TAG deway-number string orientation xoffset yoffset
*/
void parse_tag(ifstream& input_stream, EepicNode * root) 
{
  EepicNode * p = parse_deway_number(input_stream, root);

  Tag_Data tag_data;

  tag_data.tag = load_string(input_stream);

  tag_data.tag_option = get_token(input_stream);

  if (tag_data.tag_option < NORTH or tag_data.tag_option > SOUTH_WEST)
    print_parse_error_and_exit("Invalid tag option");

  tag_data.x_offset = load_number(input_stream);
  tag_data.y_offset = load_number(input_stream);

  TAGS(p).append(tag_data);
}


void parse_arc(ifstream& input_stream, EepicNode * root)
{
  EepicNode * src = parse_deway_number(input_stream, root);
  EepicNode * tgt = parse_deway_number(input_stream, root);

  if (tgt == src) /* arco hacia si mismo */
    print_parse_error_and_exit("an arc to itself");

  if (tgt->get_parent() == src)
    { /* arco natural (padre a hijo). Imprimimos warning y nos vamos */
      print_parse_warning("declared an arc from parent to child");
      return; 
    }

  Arc_Data arc_data;

  arc_data.target_node = tgt;
  arc_data.is_dashed = false;

  ARCS(src).append(arc_data);
}


void parse_dashed_arc(ifstream& input_stream, EepicNode * root)
{
  EepicNode * src = parse_deway_number(input_stream, root);
  EepicNode * tgt = parse_deway_number(input_stream, root);

  if (tgt == src) /* arco hacia si mismo */
    print_parse_error_and_exit("an dashed arc to itself");

  Arc_Data arc_data;

  arc_data.target_node = tgt;
  arc_data.is_dashed = true;

  ARCS(src).append(arc_data);
}


void parse_connexion(ifstream& input_stream, EepicNode * root)
{
  EepicNode * src = parse_deway_number(input_stream, root);
  EepicNode * tgt = parse_deway_number(input_stream, root);

  Token_Type token = get_token(input_stream);

  if (token != LEFT and token != RIGHT)
    print_parse_error_and_exit("Expected L or R");

  Connexion_Data connexion_data;

  connexion_data.target_node = tgt;
  connexion_data.is_dashed   = true;
  connexion_data.is_left     = token == LEFT;

  CONNEXIONS(src).append(connexion_data);
}


void parse_ellipse(ifstream& input_stream, EepicNode * root)
{
  EepicNode * p = parse_deway_number(input_stream, root);

  ISELLIPSE(p)   = true; 
  ISRECTANGLE(p) = false;
}


void parse_rectangle(ifstream& input_stream, EepicNode * root)
{
  EepicNode * p = parse_deway_number(input_stream, root);

  ISELLIPSE(p)   = false; 
  ISRECTANGLE(p) = true;
}


EepicNode * read_input_and_build_tree(ifstream& input_stream)
{
  EepicNode * root = parse_first_root_definition(input_stream);

  try
    {
      while (true)
	switch (get_token(input_stream))
	  {
	  case ROOT: 
	    parse_root_definition(input_stream, root);
	    break;

	  case NODE: 
	    parse_node_definition(input_stream, root);
	    break;

	  case END_FILE:
	    return root; 

	  case INVALID:
	    print_parse_error_and_exit("Unrecognized token");

	  case COMMENT: break;

	  case HRADIO: 
	    parse_hradio(input_stream, root);
	    break;

	  case VRADIO: 
	    parse_vradio(input_stream, root);
	    break;

	  case WITHOUT_NODE: 
	    parse_without_node(input_stream, root);
	    break;

	  case WITHOUT_ARC: 
	    parse_without_arc(input_stream, root);
	    break;
	
	  case XOFFSET: 
	    parse_xoffset(input_stream, root);
	    break;
	
	  case YOFFSET: 
	    parse_yoffset(input_stream, root);
	    break;
	
	  case SHADOW: 
	    parse_shadow(input_stream, root);
	    break;
	
	  case TAG: 
	    parse_tag(input_stream, root);
	    break;
	
	  case ARC: 
	    parse_arc(input_stream, root);
	    break;
	
	  case DASHED_ARC: 
	    parse_dashed_arc(input_stream, root);
	    break;

	  case DASHED_CONNEXION: 
	    parse_connexion(input_stream, root);
	    break;

	  case ELLIPSE: 
	    parse_ellipse(input_stream, root);
	    break;
	
	  case RECTANGLE: 
	    parse_rectangle(input_stream, root);
	    break;
	
	  default:
	    print_parse_error_and_exit("Unknown token type");
	  }
    }
  catch (exception & e)
    {
      destroy_tree(root);
      print_parse_error_and_exit(e.what());
    }
  return NULL; // nunca se alcanza
}

/* ***************************************************************

   Las siguientes rutinas implantan la versión mejorada de Kozo
   Sugiyama del algoritmo de Walker

*/ 

/*
  Busca por profundidad en nodo más a la derecha en el nivel level a
  partir del árbol con raíz root. Retorna en sum el acumulado de MOD
  desde root hasta el padre del nodo encontrado.

  Retorna el nodo más a la derecha o NULL si no existe
*/
EepicNode * __advance_to_rightmost_in_level(EepicNode * root, int level,
					  long double & sum)
{
  sum += MOD(root);
  for (EepicNode * p = root->get_right_child(); p != NULL; 
       p = p->get_left_sibling())
    {
      if (LEVEL(p) == level)
	return p;

      EepicNode * q = __advance_to_rightmost_in_level(p, level, sum);

      if (q != NULL)
	return q;
    }
  
  sum -= MOD(root);

  return NULL;
}

/*
  wrapper de iniciación para llamada recursiva 
*/
EepicNode * advance_to_rightmost_in_level(EepicNode * root, int level,
					  long double & sum)
{
  sum = 0;
  if (LEVEL(root) == level)
    return root;

  return __advance_to_rightmost_in_level(root, level, sum);
}


/* 
   idem al anterior pero para el nodo más a la izquierda 
*/
EepicNode * __advance_to_leftmost_in_level(EepicNode * root, int level,
					 long double & sum)
{
  sum += MOD(root);
  for (EepicNode * p = root->get_left_child(); p != NULL; 
       p = p->get_right_sibling())
    {
      if (LEVEL(p) == level)
	return p;

      EepicNode * q = __advance_to_leftmost_in_level(p, level, sum);

      if (q != NULL)
	return q;
    }

  sum -= MOD(root);

  return NULL;
}

/*
  wrapper de iniciación para llamada recursiva 
*/
EepicNode * advance_to_leftmost_in_level(EepicNode * root, int level,
					 long double & sum)
{
  sum = 0;
  if (LEVEL(root) == level)
    return root;

  return __advance_to_leftmost_in_level(root, level, sum);
}

  
/*
  Esta rutina revisa los subarboles izquierdo de p y verifica que las
  separaciones de los hermanos izquierdos sean correctas. Por cada nivel,
  el algoritmo revisa la posicion de la hoja mas a la izquierda de p y la
  compara con la hoja mas a la derecha de su hermano izquierdo
  (left_sibling). Si no corresponden a la separacion minima, entonces se
  realiza los ajustes sobre PRE(p) y MOD(p).

  La rutina maneja el valor total de desplazamiento hacia la derecha
  denominado compensated.

  La rutina asume que los hermanos izquierdos de p y los descendientes
  de p ya fueron procesados. Esto es plausible porque los valores de
  PRE(p) y MOD(p) son calculados en sufijo.
*/
void adjust_minimal_separation_with_letf_sibling(EepicNode * p)
{
  assert(p->get_left_sibling() != NULL);
  assert(not p->is_leftmost());

  /* Procesar cada hermano izquierdo de p */
  for (EepicNode * left_sibling = p->get_left_sibling();
       left_sibling != NULL; 
       left_sibling = left_sibling->get_left_sibling())
    {
      assert(p->get_parent() == left_sibling->get_parent());
      assert(LEVEL(left_sibling) == LEVEL(p));
      /* 
	 Tenemos 2 subarboles en los cuales debemos verificar que su
	 separacion no se sobrelape. El subarbol izquierdo es left_sibling
	 y el subarbol derecho es p. Inspeccionaremos el extremo derecho de
	 left_sibling con el extremo derecho de p. Si no estan
	 adecuadamente separados, entonces alejamos MOD(p) de manera tal
	 que el ajuste final no los sobrelape. r será el extremo derecho de
	 left_sibling y l sera el extremo izquierdo en p. l y r siempre se
	 corresponderan con el mismo nivel. La revision y eventual ajuste
	 se realiza por cada nivel hasta que se alcance una hoja en
	 cualquiera de los dos subarboles
      */ 
          /* valores acumulados parciales de MOD por cada extremo */
      int level = LEVEL(p) + 1;
      long double r_sum_mod; /* acumulado de mod del extremo derecho */
      long double l_sum_mod; /* acumulado de mod del extremo izquierdo */

      /* 
	 Note que en posición r debe estar antes que l. Así que no
	 confundir el sentido
 
                                    root
                                  /      \
                                 /        \
                           left_sibling     p
                               \           /
                                \         /
                                 r       l
      */ 
  
      EepicNode * r = advance_to_rightmost_in_level(left_sibling, level, 
						    r_sum_mod);
      EepicNode * l = advance_to_leftmost_in_level(p, level, l_sum_mod);

      long double lx; /* coordenada x del extremo izquierdo  */
      long double rx; /* coordenada x del extremo derecho  */
      long double min_separation; /* distancia minima de separacion entre l
				     y r */ 
      long double current_separation;

      while (l != NULL and r != NULL)
      {
	    // calcula futuras coordenadas de los extremos
	rx = PRE(r) + r_sum_mod; // coordenada x de r
	lx = PRE(l) + l_sum_mod; // coordenada x de l

	current_separation = lx - rx;
	min_separation = tree_gap + XR(r) + XR(l);

	if (current_separation < min_separation)
	  { /* l y r se están superponiendo */
	    long double compensation = min_separation - current_separation;
	    PRE(p) += compensation;
	    MOD(p) += compensation;
	  }

	    /* avanza al siguiente nivel */	
	level++; /* proximo nivel a comparar */
	r = advance_to_rightmost_in_level(left_sibling, level, r_sum_mod);
	l = advance_to_leftmost_in_level(p, level, l_sum_mod);
      }
    }

  // TODO: Ajuste sugerido por Sigiyama causa colisiones entre nodos. Revisar
  return;
  /* 
     si p fue desplazado en el paso anterior, entonces es posible que se
     forme una brecha desprorporcionada entre p y sus hermanos
     izquierdos. Esta fase se encarga de dispersar la brecha de la forma
     más uniforme posible entre los hermanos izquierdos de p sin
     incluir el más a la izquierda.
  */
}


/*
  Esta funcion es invocada por el recorrido sufijo cada vez se visita
  un nodo. El rol de la funcion es calcular los valores pre y mod
  segun el algoritmo de Walker variante de Sugiyama
*/
void precompute_x_for_node(EepicNode * p, int level, int child)
{
  if (p->is_leaf() and p->is_leftmost()) // hoja mas a la izquierda 
    PRE(p) = MOD(p) = 0;
  else if (p->is_leaf() and p->is_root()) // p es un árbol de un solo nodo
    {
      PRE(p) = xgap + WIDTH(p)/2.0;
      MOD(p) = 0;
    } 
  else 
    {
      EepicNode * left_sibling = p->get_left_sibling();
    
      if (p->is_leaf()) // p es hoja y no es el mas izquierdo 
	{
	  PRE(p) = 
	    PRE(left_sibling) + xgap + (WIDTH(left_sibling) + WIDTH(p))/2.0;
	  MOD(p) = 0;
	}
      else // p no es hoja 
	{
	  EepicNode * left_child  = p->get_left_child();
	  EepicNode * right_child = p->get_right_child();
  
	  if (p->is_leftmost() or p->is_root())
	    {
	      PRE(p) = (PRE(left_child) + PRE(right_child))/2.0;
	      MOD(p) = 0;
	    }
	  else
	    {
	      PRE(p) = PRE(left_sibling) + xgap + 
		(WIDTH(left_sibling) + WIDTH(p))/2.0;
	      MOD(p) = PRE(p) - (PRE(left_child) + PRE(right_child))/2.0;
	    }
	} // fin p no es una hoja
    } // fin p es hoja y no es el mas izquierdo */

      // colocar numero de nivel e hijo 
  LEVEL(p) = level;
  CHILDNUMBER(p) = child;

  if (not p->is_root())
    { // guardar el mayor radio de elipse vertical 
      EepicNode * pp = p->get_parent();
      MAXCHILDYR(pp) = std::max(MAXCHILDYR(pp), YR(p));
    }
  
  if (not p->is_leftmost() and not p->is_root()) 
    adjust_minimal_separation_with_letf_sibling(p); 
}

/* 
   Estos valores son calculados para luego determinar el tama~o del
   dibujo -del ambiente picture 
*/
long double x_max = 0; /* maximo centro x visto */
long double y_max = 0; /* maximo centro y visto */


/*
  Esta funcion es invocada por el recorrido prefijo cada vez que se visita
  un nodo. Su rol es calcular las coordenadas definitivas x,y

  X(P) = suma de MOD desde raíz hasta padre de p + PRE(p)

  Y(p) = Y(pp) + YR(pp) + ygap + YR(p) excepto en raíz que es YR(root)
*/
    void 
compute_definitive_coordinates_for_node(EepicNode * p, int, int)
{
  EepicNode * pp = p->get_parent();
  
  long double parent_sum_mod; /* almacena sum MOD(ascendientes) */
  
  if (pp == NULL) /* p es la raiz */ 
    {
      parent_sum_mod = 0;
      YRGAP(p) = YR(p);
      Y(p) = YR(p);
    }
  else
    {
      parent_sum_mod = SUMMOD(pp);
      SUMMOD(p) += parent_sum_mod + MOD(p); /* acumulado para uso de los
					       hijos de p */
      YRGAP(p) = MAXCHILDYR(pp);
      Y(p) = Y(pp) + YRGAP(pp) + ygap + MAXCHILDYR(pp);
    }

  X(p) = PRE(p) + parent_sum_mod; /* posicion definitiva x */
  
      /* verificacion de maxima coordenada x */
  if (X(p) + XR(p) > x_max)
    x_max = X(p) + XR(p);

      /* verificacion de maxima coordenada y */
  if (Y(p) + YR(p) > y_max)
    y_max = Y(p) + YR(p);
}


    inline
void precompute_x_coordinates_for_tree(EepicNode * root)
{
  tree_postorder_traversal(root, &precompute_x_for_node);
}


void compute_coordinates_for_tree(EepicNode * root)
{
  x_max = 0;
  precompute_x_coordinates_for_tree(root);
  tree_preorder_traversal(root, &compute_definitive_coordinates_for_node); 
}


/*
  La siguiente rutina desplaza un árbol hacia la derecha en una
  distancia shift_size puntos
*/
long double shift_size = 0;

void shift_tree_to_right(EepicNode * p, int, int)
{
  X(p) += shift_size; 
}


void compute_coordinates_for_forest_and_set_picture_size(EepicNode * root)
{
      /* calcula coordenadas del primer árbol */
  compute_coordinates_for_tree(root);
  h_size = x_max;

      /* recorre el resto de arboles pertenecientes a la arborescencia */
  for (root = root->get_right_tree(); root != NULL; 
       root = root->get_right_tree())
    {
      /* Desplazamiento del siguiente arbol es respecto al ancho del
	 arbol anterior */ 
      long double increment = 
	XR(root->get_left_tree()) + x_max + tree_gap + XR(root);
      
          /* Desplazamiento es lo anterior mas incremento calculado */
      shift_size += increment; 

      // shift_size += XR(root->get_left_tree()) + x_max + tree_gap + XR(root);
      
      compute_coordinates_for_tree(root);

          /* desplaza todos los nodos del arbol en shift_size */
      tree_preorder_traversal(root, &shift_tree_to_right);

      h_size += increment; 
    }
  v_size = y_max; /* el mayor entre todos los árboles */ 
}


void infix_tree(EepicNode * root)
{
  static int counter = 0;

  if (root == NULL)
    return;

  infix_tree(root->get_left_child());
  POS(root) = counter++;
  infix_tree(root->get_right_sibling());
}


void generate_prefix_traversal(ofstream& output, EepicNode * root)
{
  if (root == NULL)
    return;

  output << POS(root) << " ";

  generate_prefix_traversal(output, root->get_left_child());
  generate_prefix_traversal(output, root->get_right_sibling());
}


void generate_infix_traversal(ofstream& output, EepicNode * root)
{
  if (root == NULL)
    return;

  generate_infix_traversal(output, root->get_left_child());

  output << "\"" << STRING(root) << "\" ";

  generate_infix_traversal(output, root->get_right_sibling());
}


void generate_bin_tree(ofstream& output, EepicNode * root)
{
  infix_tree(root);

  output << "start-prefix ";

  generate_prefix_traversal(output, root);

  output << endl
	 << endl
	 << "start-key ";

  generate_infix_traversal(output, root);

  output << endl;
}

void generate_prologue(ofstream& output)
{
  time_t t;
  time(&t);
  output << endl
	 << "%      This LaTeX picture is a tree automatically" << endl
	 << "%      generated by ntreepic program" << endl
	 << endl
	 << "% Copyright (C) 2002, 2003, 2004, 2007" << endl
	 << "% UNIVERSITY of LOS ANDES (ULA)" << endl
	 << "% Merida - REPUBLICA BOLIVARIANA DE VENEZUELA" << endl
	 << "% Center of Studies in Microelectronics & Distributed Systems"
	 << " (CEMISID)" << endl
	 << "% ULA Computer Science Department" << endl
	 << endl
	 << "% Created by Leandro Leon - lrleon@ula.ve" << endl
	 << endl
	 << "% This program uses the Sugiyama variation of Walker" << endl
	 << "% algorithm for general trees drawing" << endl
	 << endl
	 << "% You must use epic and eepic latex packages" << endl
	 << "% in your LaTeX application" << endl
	 << endl
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
    output << "\\documentclass[11pt]{article}" << endl
	   << endl
	   << "\\usepackage{epic}" << endl
	   << "\\usepackage{eepic}" << endl
	   << endl
	   << "\\begin{document}" << endl
	   << "\\begin{center}" << endl;

  output << endl
	 << "\\setlength{\\unitlength}{" << resolution << "mm}" << endl
	 << "\\filltype{" << fill_type << "}" << endl
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

int tree_number = 0;


/* 
   Hay cuatro casos

              x

   (1) src ------- tgt

   (2)
         tgt
     x  /
       /
      /
   src

   (3)
      src 
         \
          \ x
           \
            tgt

  (4) src
       |
       | x
       |
      tgt

  La rutina genera una curva src--x--tgt. El punto x se determina según
  que el parámetro left sea true o false. En los casos pictorizados
  left==true 

  La primera parte de la rutina determina src y tgt
     
*/
void generate_curve(ofstream &  output, 
		    EepicNode * p, 
		    EepicNode * q, 
		    const bool  left = false,
		    const bool  is_dashed = true)
{
  const long double & px = X(p); // coordenadas de p
  const long double & py = Y(p);

  const long double & qx = X(q); // coordenadas de q
  const long double & qy = Y(q);

  long double srcx, srcy, // punto origen
              mx, my,     // punto intermedio
              tgtx, tgty; // punto destino

  EepicNode * src, * tgt; // puntos origen y destino

      // determinación de puntos inicial y final
  if (px < qx)
    {
      src = p; tgt = q;
      srcx = px; srcy = py;
      tgtx = qx; tgty = qy; 
    }
  else
    {
      src = q; tgt = p;
      srcx = qx; srcy = qy;
      tgtx = px; tgty = py;
    }

      // determinación de punto medio
  if (fabsl(srcx - tgtx) < hr) // caso (4) ?
    {
      my = (srcy + tgty)/2.0;

      if (left)
	{
	  srcx -= XR(src); 
	  tgtx -= XR(tgt);
	  mx    = srcx - xgap/2.5;
	}
      else
	{
	  srcx += XR(src); 
	  tgtx += XR(tgt);
	  mx    = srcx + xgap/2.5;
	}
    }
  else if (fabsl(srcy - tgty) < vr) // caso (1)
    {
      mx    = (srcx + tgtx)/2.0;
      srcx += XR(src);
      tgtx -= XR(tgt);
      my    = srcy + (left ? -ygap : ygap)/2.0;
    }
  else 
    {    
      srcx += XR(src);
      tgtx -= XR(tgt);
      
      const long double xfourth = (tgtx - srcx)/4.0;

      if (tgty < srcy) // caso (2)
	{
	  const long double yfourth = (srcy - tgty)/4.0;

	  if (left)
	    {
	      mx = srcx + xfourth;
	      my = tgty + 3.0*yfourth;
	    }
	  else
	    {
	      mx = srcx + 3.0*xfourth;
	      my = tgty + yfourth;
	    }

	  assert(my <= srcy and my >= tgty);
	}
      else // caso (3)
	{
	  const long double yfourth = (tgty - srcy)/4.0;

	  if (left)
	    {
	      mx = srcx + 3.0*xfourth;
	      my = srcy + 3.0*yfourth;
	    }
	  else
	    {
	      mx = srcx + xfourth;
	      my = srcy + yfourth;
	    }

	  assert(my >= srcy and my <= tgty);
	}
      
      assert(mx >= srcx and mx <= tgtx);
    }

  output << "\\linethickness{0.05mm}" << endl;

  if (is_dashed)
    output << "\\curvedashes[1mm]{1,1}" << endl;

  output << "\\curve(" << srcx  << "," << YPIC(srcy) << ","
	 << mx << "," << YPIC(my) << ","
	 << tgtx << "," << YPIC(tgty) << ")";
    
}

void generate_tree(ofstream& output, EepicNode * p, int level, int child_index)
{
  if (p == NULL)
    return;

  if (p->is_root())
    output << endl
	   << "%   This the tree number " << tree_number++
	   << " inside a forest ";

  assert(LEVEL(p) == level and CHILDNUMBER(p) == child_index);

  long double x = X(p);
  long double y = Y(p);

      /* Imprimir comentario cabecera de nodo */
  output << endl 
	 << endl
	 << "%   Node at level " << level << ". It's the " 
	 << child_index << " child with key = " << STRING(p);

      /* dibujar nodo */
  if (not not_nodes and not WITHOUTNODE(p)) 
    {
      output << endl;
      if (ISELLIPSE(p))
	output << "%   Ellipse" << endl 
	       << "\\put(" << x << "," << YPIC(y) << ")"
	       << (SHADOW(p) ? "{\\ellipse*{" : "{\\ellipse{") << WIDTH(p) 
	       << "}{" << HEIGHT(p) << "}}";
      else
	output << "%   Rectangle" << endl
	       << "\\path(" << x - XR(p) << "," << YPIC(y - YR(p)) << ")"
	       << "(" << x + XR(p) << "," << YPIC(y - YR(p)) << ")"
	       << "(" << x + XR(p) << "," << YPIC(y + YR(p)) << ")"
	       << "(" << x - XR(p) << "," << YPIC(y + YR(p)) << ")"
	       << "(" << x - XR(p) << "," << YPIC(y - YR(p)) << ")";
    }

  { /* imprimir el contenido del nodo */
    long double dx = center_string(STRING(p), WIDTH(p));
    put_string(output, x - dx + XOFFSET(p), y + y_offset + YOFFSET(p), 
	       "Key text= " + STRING(p), STRING(p));
  }

      /* imprimir las etiquetas -TAGs- del nodo */
  if (not TAGS(p).is_empty())
    {
      long double tx, ty; /* coordenadas de la tag */
      Tag_Data tag_data;
      long double r = std::max(XR(p), YR(p)) + 2.0/resolution; // 2mm
      long double dr = sin_45*r; /* radius r at 45 degrees */
      long double dy = font_height();
      string comment;

      for (DynDlist<Tag_Data>::Iterator itor(TAGS(p)); 
	   itor.has_current(); itor.next())
	{
	  tag_data = itor.get_current();

	  switch (tag_data.tag_option)
	    {
	    case NORTH: 
	      comment = "North tag: ";
	      tx = x + tag_data.x_offset; 
	      ty = y - r + tag_data.y_offset; 
	      break;
	    case SOUTH: 
	      comment = "South tag: ";
	      tx = x + tag_data.x_offset; 
	      ty = y + r + tag_data.y_offset + dy; 
	      break;
	    case EAST: 
	      comment = "East tag: ";
	      tx = x + r + tag_data.x_offset; 
	      ty = y + tag_data.y_offset; 
	      break;
	    case WEST: 
	      comment = "West tag: ";
	      tx = x - r + tag_data.x_offset - string_width(tag_data.tag); 
	      ty = y + tag_data.y_offset; 
	      break;
	    case NORTH_EAST: 
	      comment = "Northeast tag: ";
	      tx = x + dr + tag_data.x_offset; 
	      ty = y - dr + tag_data.y_offset; 
	      break;
	    case NORTH_WEST: 
	      comment = "Northwest tag: ";
	      tx = x - dr + tag_data.x_offset - string_width(tag_data.tag); 
	      ty = y - dr + tag_data.y_offset; 
	      break; 
	    case SOUTH_EAST:
	      comment = "Southeast tag: ";
	      tx = x + dr + tag_data.x_offset; 
	      ty = y + dr + tag_data.y_offset; 
	      break;
	    case SOUTH_WEST: 
	      comment = "Southwest tag: ";
	      tx = x - dr + tag_data.x_offset - string_width(tag_data.tag); 
	      ty = y + dr + tag_data.y_offset; 
	      break;
	    default:
	      print_parse_error_and_exit("Fatal: unexpected tag option");
	    }
	  put_string(output, tx, ty, comment + tag_data.tag, tag_data.tag);
	}
    }

      /* Dibujar arcos adicionales declarados con opción ARC o DASHED-ARC */
  for (DynDlist<Arc_Data>::Iterator itor(ARCS(p)); itor.has_current(); 
       itor.next())
    {
      Arc_Data arc_data = itor.get_current();
      
      long double lx = X(arc_data.target_node);
      long double ly = Y(arc_data.target_node);
      long double src_dx, src_dy; // diferencias con nodo origen
      if (ISELLIPSE(p))
	intersection_ellipse_line(x, y, lx, ly, XR(p), YR(p), 
				  src_dx, src_dy);
      else
	intersection_rectangle_line(x, y, lx, ly, XR(p), YR(p), 
				    src_dx, src_dy);
      long double tgt_dx, tgt_dy; // diferencias con nodo destino
      if (ISELLIPSE(arc_data.target_node))
	intersection_ellipse_line(lx, ly, x, y, XR(arc_data.target_node),
				  YR(arc_data.target_node), tgt_dx, tgt_dy);
      else
	intersection_rectangle_line(lx, ly, x, y, XR(arc_data.target_node),
				    YR(arc_data.target_node), tgt_dx, tgt_dy);
      if (lx < x)
	src_dx = -src_dx;
      else
	tgt_dx = -tgt_dx;

          /* dibujar arco adicional */
      output << endl
	     << "%   Additional arc to child with key " << 
	STRING(arc_data.target_node) << endl;
      draw_arc(output, x + src_dx, y + src_dy, lx + tgt_dx, ly - tgt_dy, 
	       arc_data.is_dashed, with_arrow);
    }

  /// TODO: falta conexiones spline y dashes. esta parte tiene solo
  /// dashed por prueba

      // dibujar conexiones
  for (DynDlist<Connexion_Data>::Iterator itor(CONNEXIONS(p)); 
       itor.has_current(); itor.next())
    {
      Connexion_Data connexion_data = itor.get_current();

      generate_curve(output, p, connexion_data.target_node, 
		     connexion_data.is_left);

# ifdef tmp

      if (dash_threaded_trees)
	output << "\\curve(" << x - dx << "," << YPIC(y + dy) << "," 
	       << px + dx << "," << YPIC(y + h) << ","
	       << px + dx << "," << YPIC(py + dy) << ")";

# endif
    }


  if (not draw_list_representation)
    /* dibujar arcos correspondientes a hijos y visitarlos */
    for (EepicNode * c = p->get_left_child(); c != NULL;
	 c = c->get_right_sibling())
      {
	if (WITHARC(c))
	  { /* dibujar arco */
	    long double lx = X(c);
	    long double ly = Y(c);
	    long double src_dx, src_dy; // diferencias con nodo origen
	    if (ISELLIPSE(p))
	      intersection_ellipse_line(x, y, lx, ly, XR(p), YR(p), 
					src_dx, src_dy);
	    else
	      intersection_rectangle_line(x, y, lx, ly, XR(p), YR(p), 
					  src_dx, src_dy); 
	    long double tgt_dx, tgt_dy; // diferencias con nodo destino
	    if (ISELLIPSE(c))
	      intersection_ellipse_line(lx, ly, x, y, XR(c), YR(c), 
					tgt_dx, tgt_dy);
	    else
	      intersection_rectangle_line(lx, ly, x, y, XR(c), YR(c), 
					  tgt_dx, tgt_dy); 
	    if (lx < x)
	      src_dx = -src_dx;
	    else
	      tgt_dx = -tgt_dx;

	    output << endl
		   << "%   Arc to child " << CHILDNUMBER(c) 
		   << " with key " << STRING(c) << endl;
	    
	    draw_arc(output, x + src_dx, y + src_dy, lx + tgt_dx, ly - tgt_dy,
		     DASHEDARC(c), with_arrow);
	  }

	    /* ahora dibuje recursivamente el proximo nodo */
	generate_tree(output, c, level + 1, CHILDNUMBER(c));
      }
  else
    {
      EepicNode * c = p->get_left_child();
      if (c != NULL)
	{
	  long double lx = X(c);
	  long double ly = Y(c);
	  long double src_dx, src_dy; // diferencias con nodo origen
	  if (ISELLIPSE(p))
	    intersection_ellipse_line(x, y, lx, ly, XR(p), YR(p), 
				      src_dx, src_dy);
	  else
	    intersection_rectangle_line(x, y, lx, ly, XR(p), YR(p), 
					src_dx, src_dy); 
	  long double tgt_dx, tgt_dy; // diferencias con nodo destino
	  if (ISELLIPSE(c))
	    intersection_ellipse_line(lx, ly, x, y, XR(c), YR(c), 
				      tgt_dx, tgt_dy);
	  else
	    intersection_rectangle_line(lx, ly, x, y, XR(c), YR(c), 
					tgt_dx, tgt_dy); 
	  if (lx < x)
	    src_dx = -src_dx;
	  else
	    tgt_dx = -tgt_dx;

	  output << endl
		 << "%   link to leftmost child " << CHILDNUMBER(c) 
		 << " with key " << STRING(c) << endl;

	  draw_arc(output, x + src_dx, y + src_dy, lx + tgt_dx, ly - tgt_dy,
		   DASHEDARC(c), with_arrow);

	      /* ahora dibuje recursivamente el proximo nodo */
	  generate_tree(output, c, level + 1, CHILDNUMBER(c));
	}

      EepicNode * rs = p->get_right_sibling();
      if (rs != NULL)
	{
	  long double rx = X(rs);
	  long double ry = Y(rs);
	  long double src_dx, src_dy; // diferencias con nodo origen
	  if (ISELLIPSE(p))
	    intersection_ellipse_line(x, y, rx, ry, XR(p), YR(p), 
				      src_dx, src_dy);
	  else
	    intersection_rectangle_line(x, y, rx, ry, XR(p), YR(p), 
					src_dx, src_dy); 
	  long double tgt_dx, tgt_dy; // diferencias con nodo destino
	  if (ISELLIPSE(rs))
	    intersection_ellipse_line(rx, ry, x, y, XR(rs), YR(rs), 
				      tgt_dx, tgt_dy);
	  else
	    intersection_rectangle_line(rx, ry, x, y, XR(rs), YR(rs), 
					tgt_dx, tgt_dy); 
	  if (rx < x)
	    src_dx = -src_dx;
	  else
	    tgt_dx = -tgt_dx;

	  output << endl
		 << "%   link to right sibling " << child_index + 1
		 << " with key " << STRING(rs) << endl;

	  draw_arc(output, x + src_dx, y + src_dy, rx + tgt_dx, ry - tgt_dy,
		   DASHEDARC(rs), with_arrow);

	      /* ahora dibuje recursivamente el proximo nodo */
	  generate_tree(output, rs, level, CHILDNUMBER(rs));
	}
    }

  if (p->is_root()) 
    /* avance al proximo arbol dentro de la arborescencia */
    generate_tree(output, p->get_right_tree(), 0, 0);
}


void generate_forest(ofstream& output, EepicNode * root)
{
  generate_prologue(output);
  generate_tree(output, root, 0, 0);
  generate_epilogue(output);
}


const char *argp_program_version = 
"ntreepic 1.7 - ALEPH drawer for general rooted trees\n"
"Copyright (C) 2004, 2007 UNIVERSITY of LOS ANDES (ULA)\n"
"Merida - REPUBLICA BOLIVARIANA DE VENEZUELA\n"
"Center of Studies in Microelectronics & Distributed Systems (CEMISID)\n"
"ULA Computer Science Department\n"
"This is free software; There is NO warranty; not even for MERCHANTABILITY\n"
"or FITNESS FOR A PARTICULAR PURPOSE\n"
"\n";


const char *argp_program_bug_address = "lrleon@ula.ve";


static char doc[] = "ntreepic -- Aleph drawer for general rooted trees";


static char arg_doc[] = "-f input-file\n";


static char * hello = 
"ALEPH drawer for general rooted trees\n"
"Copyright (C) 2004, 2007 University of Los Andes (ULA)\n"
"Merida - REPUBLICA BOLIVARIANA DE VENEZUELA\n"
"Center of Studies in Microelectronics & Distributed Systems (CEMISID)\n"
"ULA Computer Science Department\n"
"This is free software; There is NO warranty; not even for MERCHANTABILITY\n"
"or FITNESS FOR A PARTICULAR PURPOSE\n"
"\n";


static const char license_text [] = 
"ALEPH drawer for general rooted trees. License & Copyright Note\n"
"Copyright (C) 2004, 2007\n"
"UNIVERSITY of LOS ANDES (ULA)\n"
"Merida - REPUBLICA BOLIVARIANA DE VENEZUELA\n"
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
  {"radius", 'r', "radius", OPTION_ARG_OPTIONAL, "fit radius for circles" , 0},
  {"x-gap", 'w', "sibling gap", OPTION_ARG_OPTIONAL, "sibling separation" , 0},
  {"y-gap", 'h', "children gap", OPTION_ARG_OPTIONAL, "child separation" , 0},
  {"t-gap", 't', "tree gap", OPTION_ARG_OPTIONAL, "subtree separation" , 0},
  {"bin-tree", 'b', "binary tree", OPTION_ARG_OPTIONAL, 
   "generate binary tree" , 0},
  {"h-radius", 'x', "horizontal-radius", OPTION_ARG_OPTIONAL, 
   "horizontal radius" , 0},
  {"v-radius", 'y', "vertical-radius", OPTION_ARG_OPTIONAL, 
   "vertical radius" , 0},
  {"resol", 'l', "resolution", OPTION_ARG_OPTIONAL, "resolution in mm" , 0},
  {"latex", 'a', 0, OPTION_ARG_OPTIONAL, "add latex header" , 0},
  {"no-node", 'n', 0, OPTION_ARG_OPTIONAL, "no draw nodes; only arcs", 0},
  {"key-x-offset", 'X', "offset", OPTION_ARG_OPTIONAL, 
   "horizontal offset for keys", 0},
  {"key-y-offset", 'Y', "offset", OPTION_ARG_OPTIONAL, 
   "vertical offset for keys", 0},
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
  {"version", 'V', 0, OPTION_ARG_OPTIONAL, 
   "Print version information and then exit", 0},
  {"black-fill", 'B', 0, OPTION_ARG_OPTIONAL, "Fill black ellipses", 0},
  {"shade-fill", 'S', 0, OPTION_ARG_OPTIONAL, "Fill shade ellipses", 0},
  {"ellipses", 'e', 0, OPTION_ARG_OPTIONAL, "draw ellipses as nodes", 0},
  {"rectangles", 'q', 0, OPTION_ARG_OPTIONAL, "draw rectangles as nodes", 0},
  {"draw-list", 'L', 0, OPTION_ARG_OPTIONAL, 
   "draw linked list representation", 0},
  {"draw-tree", 'T', 0, OPTION_ARG_OPTIONAL, 
   "draw normal tree representation (or turn off linked list form)", 0},
  {"arrow-len", 'K', "arrow lenght in points", OPTION_ARG_OPTIONAL, 
   "arrow lenght", 0},
  {"arrow-len", 'I', "arrow width in points", OPTION_ARG_OPTIONAL, 
   "arrow width", 0},
  {"arrows", 'A', 0, OPTION_ARG_OPTIONAL, "draw arcs with arrows", 0},
  {"vertical-flip", 'F', 0, OPTION_ARG_OPTIONAL, "Flip tree respect y axe", 0},
  { 0, 0, 0, 0, 0, 0 } 
}; 


# define TERMINATE(n) (save_parameters(), exit(n))



const char * parameters_file_name = "./.ntreepic";


    inline
void save_parameters()
{
  ofstream output(parameters_file_name, ios::trunc);

  output << hr << BLANK << vr << BLANK << hd << BLANK << vd << BLANK 
	 << xgap << BLANK << ygap << BLANK << tree_gap << BLANK 
	 << resolution << BLANK << x_offset << BLANK << y_offset << BLANK 
	 << x_picture_offset << BLANK << y_picture_offset; 
}


    inline
void read_parameters()
{
  ifstream input(parameters_file_name, ios::in);

  input >> hr >> vr >> hd >> vd >> xgap >> ygap >> tree_gap >> resolution
	>> x_offset >> y_offset >> x_picture_offset >> y_picture_offset;
}


    inline
void print_parameters()
{
  cout
    << "Global horizontal node radius    -x = " << hr << endl
    << "Global vertical node radius      -y = " << vr << endl
    << "Global horizontal node diameter     = " << hd << endl
    << "Global Vertical node diameter       = " << vd << endl
    << "Horizontal sibling separation    -w = " << xgap << endl
    << "Vertical children separation     -h = " << ygap << endl
    << "Subtree separation               -t = " << tree_gap << endl
    << "Resolution in mm                 -l = " << resolution << endl
    << "Horizontal global offset for key -X = " << x_offset << endl
    << "Vertical global offset for key   -Y = " << y_offset << endl
    << "Horizontal offset for picture    -O = " << x_picture_offset << endl
    << "Vertical offset for picture      -P = " << y_picture_offset << endl;
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
    case 'w': /* brecha entre hermanos */
      if (arg == NULL)
	AH_ERROR("Waiting for sibling gap in command line");
      xgap = atof(arg);
      break;
    case 'h': /* brecha entre hermanos */
      if (arg == NULL)
	AH_ERROR("Waiting for sibling gap in command line");
      ygap = atof(arg);
      break;
    case 't': /* brecha entre subárboles */
      if (arg == NULL)
	AH_ERROR("Waiting for tree gap in command line");
      tree_gap = atof(arg);
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
    case 'n': /* no dibuja nodos, solo arcos */ 
      hr = vr = resolution/2; hd = vd = resolution;  
      not_nodes = true;
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
    case 'P': /* offset vertical para dibujo */ 
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
	break;
      }
    case 'o': /* archivo de salida */
      if (arg == NULL)
	AH_ERROR("Waiting for output file name");
      output_file_name = arg;
      break;
    case 'b': /* generar árbol binario */
      generate_binary_tree = true;
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
    case 'e': /* draw ellipses as nodes */
      ellipses = true;
      rectangles = false;
      break;
    case 'q': /* draw ellipses as nodes */
      ellipses = false;
      rectangles = true;
      break;
    case 'L': /* draw linked list representation */
      draw_list_representation = true;
      with_arrow = true;
      break;
    case 'T': /* draw forest (no list representation) */
      draw_list_representation = false;
      break;
    case 'A':
      with_arrow = true;
      break;
    case 'K':
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
      break;
    default: return ARGP_ERR_UNKNOWN;
    }
  return 0;
}


static struct argp arg_defs = { options, parser_opt, arg_doc, doc, 0, 0, 0 }; 

int main(int argc, char *argv[]) 
{
  command_line = command_line_to_string(argc, argv);

  read_parameters();

  argp_parse(&arg_defs, argc, argv, ARGP_IN_ORDER, 0, NULL);

  if (input_file_name.size() == 0)
    AH_ERROR("Input file not given");

  ifstream input_stream(input_file_name.c_str(), ios::in);

  if (not input_stream)
    AH_ERROR("%s file does not exist", input_file_name.c_str());

  cout << hello;

  cout << "input from " << input_file_name << " file " << endl;

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

  ofstream output_stream(output_file_name.c_str(), ios::out);

  cout << "output sent to " << output_file_name << " file " << endl << endl;

  EepicNode * root = read_input_and_build_tree(input_stream);

  if (generate_binary_tree)
    generate_bin_tree(output_stream, root);
  else
    {
      compute_coordinates_for_forest_and_set_picture_size(root);

      generate_forest(output_stream, root);
    }

  destroy_tree(root);

  save_parameters();
}
