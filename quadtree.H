# ifndef QUADTREE_H
# define QUADTREE_H

# include <quadnode.H>

/**
 * \brief Clase que modela un Quad Tree.
 *
 * @author Alejandro Mujica
 */
class QuadTree 
{
public:
  using Node = QuadNode;

private:
  Node * root;

  size_t max_num_points_per_node;

  /** Crea la instancia de cada uno de los cuatro hijos.
   *
   * Instancia cada uno de los hijos y reparte el conjunto de puntos entre
   * éstos. Se aplica sólo sobre nodo hoja.
   *
   * @param node nodo que se va a picar.
   */
  void split(Node * node)
  {
    assert(node->is_leaf()); // Sólo se aplica sobre nodo hoja

    size_t next_level = LEVEL(node) + 1;

    NW_CHILD(node) =
      new QuadNode(node->get_min_x(), node->get_mid_x(),
                    node->get_min_y(), node->get_mid_y(), node);
    LEVEL(NW_CHILD(node)) = next_level; 

    NE_CHILD(node) =
      new QuadNode(node->get_mid_x(), node->get_max_x(),
                    node->get_min_y(), node->get_mid_y(), node);
    LEVEL(NE_CHILD(node)) = next_level;

    SW_CHILD(node) =
      new QuadNode(node->get_min_x(), node->get_mid_x(),
                    node->get_mid_y(), node->get_max_y(), node);
    LEVEL(SW_CHILD(node)) = next_level;

    SE_CHILD(node) =
      new QuadNode(node->get_mid_x(), node->get_max_x(),
                    node->get_mid_y(), node->get_max_y(), node);
    LEVEL(SE_CHILD(node)) = next_level;

    COLOR(node) = Node::Color::Gray;

    DynList<Point> & points = node->get_points_set();
    
    while (not points.is_empty())
      {
        Point point = points.remove_first();

        QuadNode * child = node->get_child_to(point);

        child->add_point(point);
      }
  }

  /** Elimina los cuatro hijos.
   *
   * Elimina los cuatro hijos y el conjunto de puntos de cada uno de éstos
   * pasan a this. Se aplica sobre nodos no hojas con hijos que son hojas.
   *
   * @param node nodo que se va a unir.
   */
  void join(Node * node)
  {
    assert(not node->is_leaf()); // node no es hoja
    // Los cuatro hijos de node son hoja
    assert(NW_CHILD(node)->is_leaf());
    assert(NE_CHILD(node)->is_leaf());
    assert(SW_CHILD(node)->is_leaf());
    assert(SE_CHILD(node)->is_leaf());

    DynList<Point> & points = node->get_points_set();

    DynList<Point> & nw_points = NW_CHILD(node)->get_points_set();
 
    while(not nw_points.is_empty())
      points.append(nw_points.remove_first());

    DynList<Point> & ne_points = NE_CHILD(node)->get_points_set();
 
    while(not ne_points.is_empty())
      points.append(ne_points.remove_first());

    DynList<Point> & sw_points = SW_CHILD(node)->get_points_set();
 
    while(not sw_points.is_empty())
      points.append(sw_points.remove_first());

    DynList<Point> & se_points = SE_CHILD(node)->get_points_set();
 
    while(not se_points.is_empty())
      points.append(se_points.remove_first());

    delete NW_CHILD(node);
    NW_CHILD(node) = nullptr;
  
    delete NE_CHILD(node);
    NE_CHILD(node) = nullptr;

    delete SW_CHILD(node);
    SW_CHILD(node) = nullptr;

    delete SE_CHILD(node);
    SE_CHILD(node) = nullptr;

    COLOR(node) = points.is_empty() ? Node::Color::White : Node::Color::Black;
  }

  Point * insert(Node *& r, const Point & p)
  {
    assert(r->contains(p));

    if (r->is_leaf())
      {
        if (r->get_num_points() < max_num_points_per_node)
          return &r->add_point(p);
        else
          split(r);
      }
	
    Node * node = r->get_child_to(p);

    return insert(node, p);
  }

  void empty(Node *& r)
  {
    if (r == nullptr)
      return;

    empty(NW_CHILD(r));
    empty(NE_CHILD(r));
    empty(SW_CHILD(r));
    empty(SE_CHILD(r));

    delete r;
    r = nullptr;
  }
 
  template <class Op>
  void operate_on_nodes(Node * r, Op & op)
  {
    if (r == nullptr)
      return;

    op(r);

    operate_on_nodes<Op> (NW_CHILD(r), op);
    operate_on_nodes<Op> (NE_CHILD(r), op);
    operate_on_nodes<Op> (SW_CHILD(r), op);
    operate_on_nodes<Op> (SE_CHILD(r), op);
  }

  void copy_tree(QuadNode * src, QuadNode *& tgt,
                 QuadNode * tgt_parent = nullptr)
  {
    if (src == nullptr)
      return;

    tgt = new QuadNode(src->get_min_x(), src->get_max_x(),
                        src->get_min_y(), src->get_max_y(), tgt_parent);

    tgt->get_points_set() = src->get_points_set();

    COLOR(tgt) = COLOR(src);
    LEVEL(tgt) = LEVEL(src);

    copy_tree(NW_CHILD(src), NW_CHILD(tgt), tgt);
    copy_tree(NE_CHILD(src), NE_CHILD(tgt), tgt);
    copy_tree(SW_CHILD(src), SW_CHILD(tgt), tgt);
    copy_tree(SE_CHILD(src), SE_CHILD(tgt), tgt);
  }

public:
  QuadTree()
    : root(new Node), max_num_points_per_node(0)
  {
    // Empty
  }
  
  QuadTree(const Geom_Number & min_x, const Geom_Number & max_x,
	   const Geom_Number & min_y, const Geom_Number & max_y,
	   const size_t & _max_num_points_per_node = 1)
    : root(new Node(min_x, max_x, min_y, max_y)),
      max_num_points_per_node(_max_num_points_per_node)
  {
    // Empty
  }
  
  QuadTree(const QuadTree & tree)
    : root(nullptr), max_num_points_per_node(tree.max_num_points_per_node)
  {
    copy_tree(tree.root, root);
  }

  ~QuadTree()
  {
    empty(root);
  }

  Node * get_root()
  {
    return root;
  }

  void set_max_num_points_per_node(const size_t & _max_num_points_per_node)
  {
    max_num_points_per_node = _max_num_points_per_node;
  }

  /** Determina si un punto dado pertenece o no al Quad Tree.
   *
   *  @param p Punto que se quiere verificar.
   *  @return true si el punto pertenece al área sobre la cual trabaja el
   *  Quad Tree y false en caso contrario.
   */
  bool contains(const Point & p)
  {
    return root->contains(p);
  }

  /**
   * Inserta un punto en el QuadTree
   * @param p Punto que se quiere insertar
   * @return Puntero al puntoº insertado.
   */
  Point * insert(const Point & p)
  {
    if (not root->contains(p))
      return nullptr;

    return insert(root, p);
  }

  Point * insert(const Geom_Number & x, const Geom_Number & y)
  {
    return insert(Point(x, y));
  }
  
  /**
   * Busca un punto en el QuadTree
   * @param p Punto que se quiere buscar
   * @return Puntero al punto encontrado o nullptr si no existe.
   */
  Point * search(const Point & p)
  {
    if (not root->contains(p))
      return nullptr;

    Node * aux = root;

    while (not aux->is_leaf())
      aux = aux->get_child_to(p);

    return aux->search_point(p);
  }

  /** Busca el nodo hoja que contiene al punto dado.
   *
   * @param p punto que se quiere consultar.
   * @return puntero al nodo hoja que lo contiene o nullptr si el punto no
   * existe en la hoja contenedora.
   */
  Node * search_container_node(const Point & p)
  {
    if (not root->contains(p))
      return nullptr;

    Node * aux = root;

    while (not aux->is_leaf())
      aux = aux->get_child_to(p);

    if (aux->search_point(p) == nullptr)
      return nullptr;

    return aux;
  }

  void remove(const Point & p)
  {
    Node * aux = root;

    while (not aux->is_leaf())
      aux = aux->get_child_to(p);

    if (not aux->remove_point(p))
      return;

    Node * parent = PARENT(aux);

    if (parent == nullptr)
      return;

    if (NW_CHILD(parent)->get_num_points() +
        NE_CHILD(parent)->get_num_points() +
        SW_CHILD(parent)->get_num_points() +
        SE_CHILD(parent)->get_num_points() <= max_num_points_per_node)
      join(parent);
  }

  /**
   * Elimina todos los nodos del QuadTree y deja la raiz
   */
  void empty()
  {
    empty(NW_CHILD(root));
    empty(NE_CHILD(root));
    empty(SW_CHILD(root));
    empty(SE_CHILD(root));
    root->empty();
  }

  template <class Op>
  void for_each(Op & op)
  {
    operate_on_nodes <Op> (root, op);
  }

  template <class Op>
  void for_each(Op && op = Op())
  { 
    operate_on_nodes <Op> (root, op);
  }
};

# endif // QUAD_TREE_H

