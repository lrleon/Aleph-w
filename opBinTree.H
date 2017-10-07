
# ifndef OPBINTREE_H 
# define OPBINTREE_H

# include <tpl_dynArray.H>

namespace Aleph {

# define COST(i,j) (cost[(i)*(n+1) + (j)])
# define TREE(i,j) (tree[(i)*(n+1) + (j)])

static inline double sum_p(double p[], const size_t i, const size_t j) noexcept;

    static inline size_t 
min_index(DynArray <double>&, const size_t, const size_t, const size_t);

    static inline 
void compute_optimal_costs(DynArray <double> & cost, double p[], 
                           const size_t n, DynArray <int> & tree)
{
  for (size_t i = 1; i <= n; ++i)
    {
      COST(i, i - 1) = 0;
      TREE(i, i) = i;
    }
  for (size_t i = 0; i < n; ++i)
    for (size_t j = 1; j <= n - i; ++j)
      {
        size_t k   = j + i;
        TREE(j, k) = min_index(cost, j, k, n);
        COST(j, k) = sum_p(p, j, k) + 
                     COST(j, TREE(j, k) - 1) + COST(TREE(j, k) + 1, k);
      }
}

static inline double sum_p(double p[], const size_t i, const size_t j) noexcept
{
  double sum = 0.0;
  for (size_t k = i - 1; k < j; ++k)
    sum += p[k];
  return sum;
}

    static inline 
size_t min_index(DynArray <double>& cost, 
		 const size_t j, const size_t k, const size_t n)
{
  assert(j <= k);
  size_t ret_val = 0;
  double min_sum = numeric_limits<double>::max();
  for (size_t i = j; i <= k; ++i)
    {
      const double sum = COST(j, i - 1) + COST(i + 1, k);
      if (sum < min_sum)
        {
          min_sum = sum;
          ret_val = i;
        }
    }
  return ret_val;
}

    template<class Node, typename Key> static inline
Node * compute_tree(Key keys[], DynArray<int>& tree, 
                    const size_t n, const size_t i, const size_t j)
{
  if (i > j) 
    return Node::NullPtr;
  Node * root = new Node (keys[TREE(i, j) - 1]);
  LLINK(root) = compute_tree <Node, Key> (keys, tree, n, i, TREE(i, j) - 1);
  RLINK(root) = compute_tree <Node, Key> (keys, tree, n, TREE(i, j) + 1, j);
  return root;
}

    /** Construye un árbol binario de búsqueda óptimo según frecuencias
        estimadas de búsqueda.

        build_optimal_tree(keys,p,n) construye un árbol binario de
        búsqueda óptimo que contiene n claves almacenadas en el
        arreglo keys[], cada clave con frecuencia o probabilidad de
        búsqueda almacenada en el arreglo p[], paralelo a keys[].

        @param[in] keys arreglo contentivo de las claves.
        @param[in] p arreglos contentivo de las probabilidades de
        aparición o búsqueda.
        @param[in] n cantidad total de claves; esta es la dimensión de
        los arreglos.
        @return la raíz del árbol binario de búsqueda óptimo.
        @throw bad_alloc si no hay suficiente memoria.

        @ingroup Arboles
    */
    template <class Node, typename Key>
Node * build_optimal_tree(Key keys[], double p[], const size_t n)
{
  DynArray <int> tree((n + 1)*(n + 1));
  DynArray <double> cost((n + 1)*(n + 1));
  compute_optimal_costs(cost, p, n, tree);
  return compute_tree<Node, Key> (keys, tree, n, 1, n);
}


# undef COST
# undef TREE
} // end namespace Aleph
# endif // OPBINTREE_H

