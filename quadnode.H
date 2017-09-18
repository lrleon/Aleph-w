# ifndef QUADNODE_H
# define QUADNODE_H

# include <htlist.H>
# include <point.H>

# define PARENT(p) ((p)->get_parent())
# define NW_CHILD(p) ((p)->get_nw_child())
# define NE_CHILD(p) ((p)->get_ne_child())
# define SW_CHILD(p) ((p)->get_sw_child())
# define SE_CHILD(p) ((p)->get_se_child())
# define COLOR(p) ((p)->get_color())
# define LEVEL(p) ((p)->get_level())

/**
  * \brief Clase que modela un Nodo para Quad Tree.
  *
  * @author Alejandro Mujica
  */
class QuadNode
{
public:

  /**
    * \brief Colores para un Quad Node.
    *
    * White representa un nodo hoja desocupado.
    * Black representa un nodo hoja ocupado (al menos un punto).
    * Gray representa un nodo que no es hoja.
    *
    * @author Alejandro Mujica
    */
  enum class Color
  {
    White,
    Gray,
    Black,
    Num_Colors
  };

  /**
    * \brief Direcciones de los Quad.
    *
    * @author Alejandro Mujica
    */
  enum class Quad
  {
    NW, NE, SW, SE, Num_Quads
  };

  /**
    * \brief Lados hacia los que se puede ir desde un Quad cualquiera.
    *
    * @author Alejandro Mujica
    */
  enum class Side
  {
    North, South, East, West, Num_Sides
  };

private:
  DynList<Point> points;

  QuadNode * parent;

  QuadNode * nw_child;
  
  QuadNode * ne_child;

  QuadNode * sw_child;

  QuadNode * se_child;

  Color color;

  unsigned long level;

  // Datos para la región
  Geom_Number min_x;

  Geom_Number max_x;

  Geom_Number min_y;

  Geom_Number max_y;

  static QuadNode * get_north_neighbor(QuadNode * v)
  {
    if (PARENT(v) == nullptr)
      return nullptr;

    if (v->is_sw_child())
      return NW_CHILD(PARENT(v));

    if (v->is_se_child())
      return NE_CHILD(PARENT(v));

    QuadNode * u = get_north_neighbor(PARENT(v));

    if (u == nullptr or u->is_leaf())
      return u;
    else if (v->is_nw_child())
      return SW_CHILD(u);
    else
    return SE_CHILD(u);
  }

  static QuadNode * get_south_neighbor(QuadNode * v)
  {
    if (PARENT(v) == nullptr)
      return nullptr;

    if (v->is_nw_child())
      return SW_CHILD(PARENT(v));

    if (v->is_ne_child())
      return SE_CHILD(PARENT(v));

    QuadNode * u = get_south_neighbor(PARENT(v));

    if (u == nullptr or u->is_leaf())
      return u;
    else if (v->is_sw_child())
      return NW_CHILD(u);
    else
      return NE_CHILD(u);
  }

  static QuadNode * get_east_neighbor(QuadNode * v)
  {
    if (PARENT(v) == nullptr)
      return nullptr;

    if (v->is_nw_child())
      return NE_CHILD(PARENT(v));

    if (v->is_sw_child())
      return SE_CHILD(PARENT(v));

    QuadNode * u = get_east_neighbor(PARENT(v));

    if (u == nullptr or u->is_leaf())
      return u;
    else if (v->is_ne_child())
      return NW_CHILD(u);
    else
      return SW_CHILD(u);
  }

  static QuadNode * get_west_neighbor(QuadNode * v)
  {
    if (PARENT(v) == nullptr)
      return nullptr;

    if (v->is_ne_child())
      return NW_CHILD(PARENT(v));

    if (v->is_se_child())
      return SW_CHILD(PARENT(v));

    QuadNode * u = get_west_neighbor(PARENT(v));

    if (u == nullptr or u->is_leaf())
      return u;
    else if (v->is_nw_child())
      return NE_CHILD(u);
    else
      return SE_CHILD(u);
  }

  static void get_neighbors_by_side(QuadNode * node,
                                    const Side & side,
				    DynList<QuadNode *> & neighbors)
  {
    if (node == nullptr)
      return;

    if (node->is_leaf())
      {
        neighbors.append(node);
        return;
      }

    switch(side)
      {
      case Side::West:
        get_neighbors_by_side(NW_CHILD(node), side, neighbors);
        get_neighbors_by_side(SW_CHILD(node), side, neighbors);
        break;
      case Side::East:
        get_neighbors_by_side(NE_CHILD(node), side, neighbors);
        get_neighbors_by_side(SE_CHILD(node), side, neighbors);
        break;
      case Side::North:
        get_neighbors_by_side(NW_CHILD(node), side, neighbors);
        get_neighbors_by_side(NE_CHILD(node), side, neighbors);
        break;
      case Side::South:
        get_neighbors_by_side(SW_CHILD(node), side, neighbors);
        get_neighbors_by_side(SE_CHILD(node), side, neighbors);
        break;
      default:
        throw std::domain_error("Invalid side value");
    }
}

static size_t count_points(QuadNode * node)
{
  if (node == nullptr)
    return 0;

  if (node->is_leaf())
    return node->points.size();

  size_t num_points = 0;

  num_points += count_points(NW_CHILD(node));
  num_points += count_points(NE_CHILD(node));
  num_points += count_points(SW_CHILD(node));
  num_points += count_points(SE_CHILD(node));

  return num_points;
}
  
public:
  QuadNode()
    : points(), parent(nullptr), nw_child(nullptr), ne_child(nullptr),
      sw_child(nullptr), se_child(nullptr), color(Color::White), level(0),
      min_x(0), max_x(0), min_y(0), max_y(0)
  {
    // Empty
  }
  
  QuadNode(const Geom_Number & _min_x, const Geom_Number & _max_x,
	   const Geom_Number & _min_y, const Geom_Number & _max_y,
	   QuadNode * _parent = nullptr)
    : points(), parent(_parent), nw_child(nullptr), ne_child(nullptr),
      sw_child(nullptr), se_child(nullptr), color(Color::White), level(0),
      min_x(_min_x), max_x(_max_x), min_y(_min_y), max_y(_max_y)
  {
    // Empty
  }

  QuadNode(const QuadNode &) = delete;

  /// Asigna una región dada al nodo.
  void set_region(const Geom_Number & _min_x,
		  const Geom_Number & _max_x,
                  const Geom_Number & _min_y,
                  const Geom_Number & _max_y)
  {
    min_x = _min_x;
    max_x = _max_x;
    min_y = _min_y;
    max_y = _max_y;
  }

  /**
    * Puntero referencia al padre del nodo, se puede usar la macro
    * PARENT(p) tanto para lectura como para escritura.
    */
  QuadNode *& get_parent()
  {
    return parent;
  }

  /**
    * Puntero referencia al hijo noroeste del nodo, se puede usar la macro
    * NW_CHILD(p) tanto para lectura como para escritura.
    */
  QuadNode *& get_nw_child()
  {
    return nw_child;
  }

  /**
    * Puntero referencia al hijo noreste del nodo, se puede usar la macro
    * NE_CHILD(p) tanto para lectura como para escritura.
    */
  QuadNode *& get_ne_child()
  {
    return ne_child;
  }

  /**
    * Puntero referencia al hijo suroeste del nodo, se puede usar la macro
    * SW_CHILD(p) tanto para lectura como para escritura.
    */
  QuadNode *& get_sw_child()
  {
    return sw_child;
  }

  /**
    * Puntero referencia al hijo sureste del nodo, se puede usar la macro
    * SE_CHILD(p) tanto para lectura como para escritura.
    */
  QuadNode *& get_se_child()
  {
    return se_child;
  }

  /**
    * Referencia al color del nodo, se puede usar la macro COLOR(p) 
    * tanto para lectura como para escritura.
    */
  Color & get_color()
  {
    return color;
  }

  /**
    * Referencia al nivel en el &aacute;rbol del nodo, se puede usar la macro
    * LEVEL(p) tanto para lectura como para escritura.
    */
  unsigned long & get_level()
  {
    return level;
  }

  bool is_leaf() const
  {
    return color != Color::Gray;
  }

  /**
    * Dice si el nodo es hijo noroeste.
    */
  bool is_nw_child() const
  {
    if (parent == nullptr)
      return false;
    return NW_CHILD(parent) == this;
  }

  /**
    * Dice si el nodo es hijo noreste.
    */
  bool is_ne_child() const
  {
    if (parent == nullptr)
      return false;
    return NE_CHILD(parent) == this;
  }

  /**
    * Dice si el nodo es hijo suroeste.
    */
  bool is_sw_child() const
  {
    if (parent == nullptr)
      return false;
    return SW_CHILD(parent) == this;
  }

  /**
    * Dice si el nodo es hijo sureste.
    */
  bool is_se_child() const
  {
    if (parent == nullptr)
      return false;
    return SE_CHILD(parent) == this;
  }

  /**
    * Especifica si la región representada por el nodo contiene al punto
    * @param p El punto que se quiere probar
    * @return true si el punto está dentro de la región
    */
  bool contains(const Point & p) const
  {
    return p.get_x() >= min_x and p.get_x() < max_x and
           p.get_y() >= min_y and p.get_y() < max_y;
  }

  /** Dado un punto, retorna el nodo hijo que lo contiene
    *
    * @param p Punto que se quiere evaluar.
    * @return Nodo hijo que contiene a p.
    */
  QuadNode * get_child_to(const Point & p) const
  {
    assert(nw_child != nullptr);
    assert(ne_child != nullptr);
    assert(sw_child != nullptr);
    assert(se_child != nullptr);

    if (not contains(p))
      throw std::domain_error("This node does not contain that point");

    if (nw_child->contains(p))
      return nw_child;

    if (ne_child->contains(p))
      return ne_child;

    if (sw_child->contains(p))
      return sw_child;

    if (se_child->contains(p))
      return se_child;

    return nullptr; // Para evitar advertencias del compilador.
  }

  Point & add_point(const Point & p)
  {
    color = Color::Black;
    return points.append(p);
  }

  size_t get_num_points()
  {
    return count_points(this);
  }

  const Geom_Number & get_min_x() const
  {
    return min_x;
  }

  const Geom_Number & get_max_x() const
  {
    return max_x;
  }

  const Geom_Number & get_min_y() const
  {
    return min_y;
  }

  const Geom_Number & get_max_y() const
  {
    return max_y;
  }

  Geom_Number get_width() const
  {
    return max_x - min_x;
  }

  Geom_Number get_height() const
  {
    return max_y - min_y;
  }

  Geom_Number get_mid_x() const
  {
    return (min_x + max_x) / 2;
  }

  Geom_Number get_mid_y() const
  {
    return (min_y + max_y) / 2;
  }

  /** Busca el punto dado en el nodo.
   *
   * @param p punto que se quiere buscar.
   * @return puntero al punto encontrado o nullptr si no lo encuentra.
   */
  Point * search_point(const Point & p)
  {
    for (Point & point : points)
      {
        if (point == p)
          return &point;
      }
    return nullptr;
  }

  /** Elimina un punto dado del nodo.
   *
   * @param p punto que se desea eliminar.
   * @return true si el punto existía y lo eliminó, false en caso contrario.
   */
  bool remove_point(const Point & p)
  {
    for (DynList<Point>::iterator it(points); it.has_curr(); it.next())
      {
        const Point & curr = it.get_curr();
	
        if (curr == p)
          {
            it.del();
            return true;
          }
      }
    return false;
  }

  /// Vacía el conjunto de puntos del nodo.
  void empty()
  {
    points.empty();
    color = Color::White;
  }

  /**
    * Busca los nodos adyacentes al nodo this y los almacena en una lista.
    * @param list Lista donde quedan almacenados los punteros a los nodos
    * adyacentes.
    */
  DynList<QuadNode *> get_neighbors()
  {
    DynList<QuadNode *> neighbors;
    
    QuadNode * north_neighbor = get_north_neighbor(this);
    get_neighbors_by_side(north_neighbor, Side::South, neighbors);

    QuadNode * south_neighbor = get_south_neighbor(this);
    get_neighbors_by_side(south_neighbor, Side::North, neighbors);

    QuadNode * east_neighbor = get_east_neighbor(this);
    get_neighbors_by_side(east_neighbor, Side::West, neighbors);

    QuadNode * west_neighbor = get_west_neighbor(this);
    get_neighbors_by_side(west_neighbor, Side::East, neighbors);

    return neighbors;
  }

  DynList<Point> & get_points_set()
  {
    return points;
  }

  template <class Op>
  void for_each_point(Op & op)
  {
    for (Point & point : points)
      op(point);
  };

  template <class Op>
  void for_each_point(Op && op = Op())
  {
    for_each_point<Op>(op);
  }

  QuadNode & operator = (const QuadNode &) = delete;
};

# endif // QUADNODE_H

