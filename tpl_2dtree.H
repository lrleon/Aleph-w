
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

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
# ifndef TPL_2DTREE_H
# define TPL_2DTREE_H

# include <point.H>
# include <htlist.H>
# include <tpl_nodePool.H>

/** Árbol Kd en versión 2d.

    Un kdtree es un árbol binario que almacena segmentos según un
    hiperplano de k dimensiones. En este caso sólo se maneja un plano.

   @ingroup Geometria
   @author Leandro Rabindranath León
 */
    template <typename T = Empty_Class>
class K2Tree
{
  struct Node 
  {
    Point point; // the point
    Rectangle rect; // the axis-aligned rectangle corresponding to this node
    Node * lb; // the left/bottom subtree
    Node * rt; // the right/top subtree

    Node(const Point & __point) : point(__point), lb(nullptr), rt(nullptr)
    { 
      // empty
    }

    void set_rect(const Geom_Number & xmin, const Geom_Number & ymin, 
		  const Geom_Number & xmax, const Geom_Number & ymax) 
    {
      rect.set_rect(xmin, ymin, xmax, ymax);
    }

    void set_rect(const Point & pmin, const Point & pmax)
    {
      rect.set_rect(pmin.get_x(), pmin.get_y(), pmax.get_x(), pmax.get_y());
    }

    const Geom_Number & xmin() const { return rect.get_xmin(); }
    const Geom_Number & ymin() const { return rect.get_ymin(); }
    const Geom_Number & xmax() const { return rect.get_xmax(); }
    const Geom_Number & ymax() const { return rect.get_ymax(); }
    const Geom_Number & x() const { return point.get_x(); }
    const Geom_Number & y() const { return point.get_y(); }
  };

  Point pmin;  // esquina inferior izquierda del rectángulo de referencia
  Point pmax;  // esquina superior derecha del rectángulo de referencia
  size_t N;    // número de puntos que maneja el árbol
  Node * root; // raíz del árbol

  static const size_t dim = 19;
  Node_Pool<Node> node_pool;

public:

  K2Tree() : pmin(0, 0), pmax(0, 0), node_pool(dim) { /* empty */ }

  K2Tree(const Point & __pmin, const Point & __pmax) 
    : pmin(__pmin), pmax(__pmax), node_pool(dim)
  {
    /* empty */ 
  }

  K2Tree(const Geom_Number & xmin, const Geom_Number & ymin, 
	 const Geom_Number & xmax, const Geom_Number & ymax) 
    : pmin(xmin, ymin), pmax(xmax, ymax), node_pool(dim)
  {
    /* empty */ 
  }

      /// retorna true si el árbol no contiene ningún punto
  bool is_empty() const { return N == 0; }

      /// retorna el número de puntos que contiene el árbol
  size_t size() const { return N; }

private:

  static Node * lr_insert(Node * root, const Point & p) 
  {
    if (root == nullptr)
      return new Point (p);

    Node * ret_val = nullptr;
    if (p.get_x() == root->x()) 
      {
	if (p.get_y() == root->y())
	  return nullptr; // duplicated point

	ret_val = bu_insert(root->rt, p);
	if (ret_val != nullptr) 
	  {
	    root->rt = ret_val; // node was inserted
	    root->rt->set_rect(root->x(), root->ymin(), 
			       root->xmax(), root->ymax());

	    return root;
	  }

	return nullptr;
      }

    if (p.get_x() < root->x())
      {
	ret_val = bu_insert(root->lb, p);
	if (ret_val != nullptr) 
	  {
	    root->lb = ret_val;
	    root->lb->set_rect(root->xmin(), root->ymin(), 
			       root->x(), root->ymax());

	    return root;
	  } 
	else
	  return nullptr;
      }

    ret_val = bu_insert(root->rt, p);
    if (ret_val != nullptr) 
      {
	root->rt = ret_val;
	root->rt->set_rect(root->x(), root->ymin(), 
			   root->xmax(), root->ymax());

	return root;
      }

    return nullptr;
  }

  static Node * bu_insert(Node * root, const Point & p) 
  {
    if (root == nullptr)
      return new Point(p);

    Node * ret_val = nullptr;
    if (p.get_y() == root->y()) 
      {
	if (p.get_x() == root->x())
	  return nullptr; // duplicated point

	ret_val = lr_insert(root->rt, p);
	if (ret_val != nullptr) 
	  {
	    root->rt = ret_val; // node was inserted
	    root->rt->set_rect(root->xmin(), root->y(), 
			       root->xmax(), root->ymax());

	    return root;
	  }
	
	return nullptr;
      }

    if (p.get_y() < root->y()) 
      {
	ret_val = lr_insert(root->lb, p);
	if (ret_val != nullptr) 
	  {
	    root->lb = ret_val;
	    root->lb->set_rect(root->xmin(), root->ymin(), 
			       root->xmax(), root->y());
	    return root;
	  } 
	else
	  return nullptr;
      }

    ret_val = lr_insert(root->rt, p);
    if (ret_val != nullptr) 
      {
	root->rt = ret_val;
	root->rt->set_rect(root->xmin(), root->y(), 
			   root->xmax(), root->ymax());
	
	return root;
    }

    return nullptr;
  }

public:

  /** Inserta un punto en el árbol. 

      @param[in] point el punto a insertar
      @return Puntero al punto insertado si éste no se encuentra en el
      árbol; nullptr de lo contrario
   */
  Point * insert(const Point & p) 
  {
    if (root == nullptr) 
      {
	root = new Node (p);
	root->set_rect(pmin, pmax);
	N++;

	return &root->point;
      }

    Node * ret = lr_insert(root, p);
    if (ret == nullptr)
      return nullptr;

    N++;

    return nullptr;
  }

private:

  static Node * bu_search(Node * root, const Point & p) 
  {
    if (root == nullptr)
      return nullptr;

    if (root->y() == p.get_y()) 
      {
	if (root->x() == p.get_x())
	  return root;

	return lr_search(root->rt, p);
      }

    if (p.get_y() < root->y())
      return lr_search(root->lb, p);
    else
      return lr_search(root->rt, p);
  }

  static Node * lr_search(Node * root, const Point & p) 
  {
    if (root == nullptr)
      return nullptr;

    if (root->x() == p.get_x()) 
      {
	if (root->y() == p.get_y())
	  return root;

	return bu_search(root->rt, p);
      }

    if (p.get_x() < root->x())
      return bu_search(root->lb, p);
    else
      return bu_search(root->rt, p);
  }

public:

      /// Retorna true si el árbol contiene exactamente al punto p
  bool contains(const Point & p) 
  {
    return lr_search(root, p) != nullptr;
  }

private:

  static void range(Node * root, const Rectangle & rect, DynList<Point> * q) 
  {
    if (root == nullptr)
      return;
    
    if (not root->rect.intersects(rect))
      return;
    
    const Point & point = root->point;
    if (rect.contains(point))
        q->append(point);
    
    range(root->lb, rect, q);
    range(root->rt, rect, q);
  }

public:

  /** Retorna una lista con todos los puntos contenidos en el árbol y
      que están circunscritos al rectángulo rect

      @param[in] rect el rectángulo de referencia
      @param[out] l la lista donde almacenar los puntos encontrados
  */
  void range(const Rectangle & rect, DynList<Point> * l)
  { 
    if (N == 0)
      return;
    
    range(root, rect, l);
  }

private:

  Node * min_node;
  Geom_Number min_dist2;

  void lr_nearest(Node * root, const Point & p) 
  {
    if (root == nullptr)
      return;
    
    if (root->rect.distance_squared_to(p) > min_dist2)
      return;
    
    Geom_Number d2 = root->point.distance_squared_to(p); // es con el rectángulo
    if (d2 < min_dist2) 
      {
	min_dist2 = d2;
	min_node = root;
      }
    
    if (p.get_x() < root->x()) // is p to left of this
      { 
	bu_nearest(root->lb, p);
	bu_nearest(root->rt, p);
      } 
    else 
      {
	bu_nearest(root->rt, p);
	bu_nearest(root->lb, p);
      }
  }

  void bu_nearest(Node * root, const Point & p) 
  {
    if (root == nullptr)
      return;
    
    if (root->rect.distance_squared_to(p) > min_dist2)
      return;
    
    Geom_Number d2 = root->point.distance_squared_to(p);
    if (d2 < min_dist2) 
      {   
	min_dist2 = d2;
	min_node = root;
      }
        
    if (p.get_y() < root->y())  // is p to below this?
      {
	lr_nearest(root->lb, p);
	lr_nearest(root->rt, p);
    } 
    else 
      {
	lr_nearest(root->rt, p);
	lr_nearest(root->lb, p);
      }
  }

public:

      /// Retorna el punto más cercano al punto p
  Point nearest(const Point & p) 
  { 
    if (N == 0)
      return NullPoint;
    
    min_dist2 = 1.0;
    lr_nearest(root, p);
    
    return min_node->point;
  }
};

# endif // TPL_2DTREE_H
