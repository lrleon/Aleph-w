/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

/** @file Graph_Coloring.H
 *  @brief Algoritmos y heurísticas para el coloreado de grafos.
 *
 *  Este archivo proporciona una colección de algoritmos para el problema de
 *  coloreado de grafos (Vertex Coloring), donde se busca asignar un color a
 *  cada nodo de forma que no existan dos nodos adyacentes con el mismo color.
 *
 *  ## Algoritmos incluidos:
 *
 *  - **Greedy Coloring**: Procesa los nodos en su orden natural de iteración.
 *    Es rápido pero muy sensible al orden de los vértices.
 *  - **Welsh-Powell**: Una mejora del algoritmo ávido que ordena los nodos por
 *    grado descendente antes de colorear. Suele producir mejores resultados.
 *  - **DSatur (Degree of Saturation)**: Una heurística adaptativa que elige el
 *    siguiente nodo basándose en su "grado de saturación". Es una de las
 *    mejores heurísticas generales.
 *  - **Chromatic Number**: Calcula el número cromático exacto ($\chi(G)$) para
 *    grafos pequeños (hasta 64 nodos) usando backtracking y cotas de DSatur.
 *
 *  ## Detalles de Implementación
 *
 *  Todos los algoritmos utilizan el campo `NODE_COOKIE` de los nodos del grafo
 *  para almacenar temporalmente los colores de forma eficiente ($O(1)$).
 *  Para garantizar la seguridad ante excepciones y no destruir datos previos,
 *  se emplea @ref Cookie_Saver, que restaura automáticamente los cookies
 *  originales al finalizar la función.
 *
 *  Los resultados se devuelven en un @ref DynMapTree que mapea cada puntero a
 *  nodo con su índice de color (basado en 0).
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

#ifndef GRAPH_COLORING_H
#define GRAPH_COLORING_H

#include <tpl_graph.H>
#include <tpl_array.H>
#include <tpl_dynMapTree.H>
#include <tpl_dynSetTree.H>
#include <tpl_sort_utils.H>
#include <cookie_guard.H>
#include <ah-errors.H>

namespace Aleph {

/** @brief Detalles internos de implementación para el coloreado de grafos.
 *
 *  Contiene utilidades para la codificación de colores en los cookies y
 *  la búsqueda de colores disponibles.
 */
namespace graph_coloring_detail {

/** @brief Codifica un índice de color en un puntero para su almacenamiento en un cookie.
 *  @param[in] c Índice de color (desde 0).
 *  @return Puntero que representa el color `c`.
 *  @note Codificación interna: `(void*)(c + 1)`. `nullptr` representa "sin color".
 */
inline void *encode_color(size_t c) noexcept
{
  return reinterpret_cast<void *>(static_cast<uintptr_t>(c + 1));
}

/** @brief Verifica si un nodo tiene un color asignado basándose en su cookie.
 *  @param[in] cookie Valor del cookie del nodo.
 *  @return `true` si el nodo está coloreado, `false` en caso contrario.
 */
inline bool is_colored(void *cookie) noexcept
{
  return cookie != nullptr;
}

/** @brief Decodifica el índice de color almacenado en un cookie.
 *  @param[in] cookie Valor del cookie.
 *  @return El índice de color original.
 *  @pre El cookie debe representar un nodo coloreado (`is_colored(cookie)`).
 */
inline size_t decode_color(void *cookie) noexcept
{
  return static_cast<size_t>(reinterpret_cast<uintptr_t>(cookie)) - 1;
}

/** @brief Encuentra el número natural más pequeño (color) que no está en el conjunto `used`.
 *  @param[in] used Conjunto de colores ya utilizados por los vecinos.
 *  @return El color disponible más bajo.
 *  @note Complejidad: $O(k \log k)$ donde $k$ es el número de colores usados.
 */
inline size_t smallest_available(const DynSetTree<size_t> &used)
{
  size_t c = 0;
  while (used.search(c) != nullptr)
    ++c;
  return c;
}
}  // end namespace graph_coloring_detail

/** @brief Coloreado ávido (Greedy) siguiendo el orden de iteración.
 *
 *  Recorre los nodos del grafo en su orden natural y asigna a cada uno el
 *  primer color disponible que no esté siendo usado por sus vecinos ya coloreados.
 *
 *  @tparam GT Tipo de grafo.
 *  @tparam SA Filtro de arcos (por defecto @ref Dft_Show_Arc<GT>).
 *  @param[in] g Grafo a colorear.
 *  @param[out] colors Mapa donde se guardará el resultado (Nodo -> Color).
 *  @return El número total de colores utilizados.
 *
 *  @pre El grafo debe ser válido y `colors` debe estar listo para recibir datos.
 *  @post `is_valid_coloring(g, colors)` será `true`.
 *
 *  @note **Complejidad**: $O((V+E) \log \Delta)$, donde $\Delta$ es el grado máximo.
 *  @note El resultado depende críticamente del orden de iteración de los nodos.
 *
 *  @see welsh_powell_coloring, dsatur_coloring, is_valid_coloring
 *  @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>>
size_t greedy_coloring(const GT &g, DynMapTree<typename GT::Node *, size_t> &colors)
{
  using Node = typename GT::Node;
  using namespace graph_coloring_detail;

  colors = {};

  if (g.get_num_nodes() == 0)
    return 0;

  // Garantiza que los cookies previos se restauren al salir, incluso en caso de excepción.
  Cookie_Saver<GT> saver(g, true, false);

  // Inicializar cookies a nullptr (sin color)
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    NODE_COOKIE(it.get_curr()) = nullptr;

  size_t num_colors = 0;

  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    {
      Node              *v = it.get_curr();
      DynSetTree<size_t> used;

      // Recolectar colores de vecinos
      for (Node_Arc_Iterator<GT, SA> ait(v); ait.has_curr(); ait.next_ne())
        if (Node *w = ait.get_tgt_node(); is_colored(NODE_COOKIE(w)))
          used.insert(decode_color(NODE_COOKIE(w)));

      // Asignar el color disponible más pequeño
      size_t c       = smallest_available(used);
      NODE_COOKIE(v) = encode_color(c);
      colors.insert(v, c);

      if (c + 1 > num_colors)
        num_colors = c + 1;
    }

  return num_colors;
}

/** @brief Coloreado mediante la heurística de Welsh-Powell.
 *
 *  Ordena los nodos de mayor a menor grado y luego aplica el algoritmo ávido.
 *  Esta estrategia suele reducir el número de colores necesarios al procesar
 *  primero los nodos más "difíciles" (los de mayor grado).
 *
 *  @tparam GT Tipo de grafo.
 *  @tparam SA Filtro de arcos (por defecto @ref Dft_Show_Arc<GT>).
 *  @param[in] g Grafo a colorear.
 *  @param[out] colors Mapa con el resultado del coloreado.
 *  @return El número total de colores utilizados.
 *
 *  @note **Complejidad**: $O((V+E) \log \Delta + V \log V)$ debido al ordenamiento.
 *
 *  @see greedy_coloring, dsatur_coloring
 *  @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>>
size_t welsh_powell_coloring(const GT &g, DynMapTree<typename GT::Node *, size_t> &colors)
{
  using Node = typename GT::Node;
  using namespace graph_coloring_detail;

  colors = {};

  if (g.get_num_nodes() == 0)
    return 0;

  Cookie_Saver<GT> saver(g, true, false);

  // Recolectar nodos y limpiar cookies
  DynList<Node *> nodes;
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    {
      Node *p        = it.get_curr();
      NODE_COOKIE(p) = nullptr;
      nodes.append(p);
    }

  // Ordenar por grado descendente
  mergesort(nodes, [&g](Node *a, Node *b) { return g.get_num_arcs(a) > g.get_num_arcs(b); });

  size_t num_colors = 0;

  // Aplicar coloreado ávido en el orden ordenado
  for (auto nit = nodes.get_it(); nit.has_curr(); nit.next_ne())
    {
      Node              *v = nit.get_curr();
      DynSetTree<size_t> used;

      for (Node_Arc_Iterator<GT, SA> ait(v); ait.has_curr(); ait.next_ne())
        {
          Node *w = ait.get_tgt_node();
          if (is_colored(NODE_COOKIE(w)))
            used.insert(decode_color(NODE_COOKIE(w)));
        }

      size_t c       = smallest_available(used);
      NODE_COOKIE(v) = encode_color(c);
      colors.insert(v, c);

      if (c + 1 > num_colors)
        num_colors = c + 1;
    }

  return num_colors;
}

/** @brief Coloreado mediante la heurística DSatur (Degree of Saturation).
 *
 *  DSatur es un algoritmo adaptativo que, en cada paso, selecciona el nodo no
 *  coloreado con el mayor "grado de saturación" (número de colores distintos
 *  asignados a sus vecinos). Los empates se resuelven eligiendo el nodo con
 *  mayor grado en el grafo original.
 *
 *  Es ampliamente considerada como una de las mejores heurísticas para coloreado
 *  de grafos generales, superando frecuentemente a Welsh-Powell.
 *
 *  @tparam GT Tipo de grafo.
 *  @tparam SA Filtro de arcos (por defecto @ref Dft_Show_Arc<GT>).
 *  @param[in] g Grafo a colorear.
 *  @param[out] colors Mapa con el resultado.
 *  @return El número total de colores utilizados.
 *
 *  @note **Complejidad**: $O(V \cdot (V+E))$ en esta implementación.
 *
 *  @see chromatic_number, welsh_powell_coloring
 *  @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>>
size_t dsatur_coloring(const GT &g, DynMapTree<typename GT::Node *, size_t> &colors)
{
  using Node = typename GT::Node;
  using namespace graph_coloring_detail;

  colors = {};

  const size_t n = g.get_num_nodes();
  if (n == 0)
    return 0;

  Cookie_Saver<GT> saver(g, true, false);

  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    NODE_COOKIE(it.get_curr()) = nullptr;

  size_t num_colors    = 0;
  size_t colored_count = 0;

  while (colored_count < n)
    {
      // Paso adaptativo: encontrar el nodo no coloreado con mayor grado de saturación.
      Node  *best     = nullptr;
      size_t best_sat = 0;
      size_t best_deg = 0;

      for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
        {
          Node *v = it.get_curr();
          if (is_colored(NODE_COOKIE(v)))
            continue;

          // Grado de saturación: cantidad de colores distintos entre los vecinos
          DynSetTree<size_t> neighbor_colors;
          for (Node_Arc_Iterator<GT, SA> ait(v); ait.has_curr(); ait.next_ne())
            {
              Node *w = ait.get_tgt_node();
              if (is_colored(NODE_COOKIE(w)))
                neighbor_colors.insert(decode_color(NODE_COOKIE(w)));
            }

          size_t sat = neighbor_colors.size();
          size_t deg = g.get_num_arcs(v);

          // Prioridad: mayor saturación, luego mayor grado.
          if (best == nullptr or sat > best_sat or (sat == best_sat and deg > best_deg))
            {
              best     = v;
              best_sat = sat;
              best_deg = deg;
            }
        }

      // Colorear el nodo elegido
      DynSetTree<size_t> used;
      for (Node_Arc_Iterator<GT, SA> ait(best); ait.has_curr(); ait.next_ne())
        {
          Node *w = ait.get_tgt_node();
          if (is_colored(NODE_COOKIE(w)))
            used.insert(decode_color(NODE_COOKIE(w)));
        }

      size_t c          = smallest_available(used);
      NODE_COOKIE(best) = encode_color(c);
      colors.insert(best, c);
      ++colored_count;

      if (c + 1 > num_colors)
        num_colors = c + 1;
    }

  return num_colors;
}

/** @brief Valida si una asignación de colores para un grafo es correcta.
 *
 *  Una coloración es válida si:
 *  1. Todos los nodos del grafo tienen una entrada en el mapa `colors`.
 *  2. No existen dos nodos adyacentes que compartan el mismo color.
 *
 *  @tparam GT Tipo de grafo.
 *  @tparam SA Filtro de arcos (por defecto @ref Dft_Show_Arc<GT>).
 *  @param[in] g Grafo.
 *  @param[in] colors Mapa de colores a validar.
 *  @return `true` si la coloración es válida, `false` en caso contrario.
 *
 *  @note **Complejidad**: $O(V + E)$.
 *  @note Se omiten los bucles (self-loops) ya que harían imposible cualquier coloración.
 *
 *  @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>>
[[nodiscard]] bool is_valid_coloring(const GT &g, const DynMapTree<typename GT::Node *, size_t> &colors)
{
  using Node = typename GT::Node;

  // Cada nodo debe tener un color
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    if (colors.search(it.get_curr()) == nullptr)
      return false;

  // Ninguna arista debe conectar nodos con el mismo color
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    {
      Node  *v  = it.get_curr();
      size_t cv = colors.find(v);

      for (Node_Arc_Iterator<GT, SA> ait(v); ait.has_curr(); ait.next_ne())
        {
          Node *w = ait.get_tgt_node();
          if (w == v)
            continue;  // ignorar bucles

          size_t cw = colors.find(w);
          if (cv == cw)
            return false;
        }
    }

  return true;
}

/** @brief Calcula el número cromático exacto de un grafo pequeño.
 *
 *  El número cromático $\chi(G)$ es la mínima cantidad de colores necesaria para
 *  colorear el grafo. Este problema es NP-difícil.
 *
 *  La función utiliza un enfoque híbrido:
 *  1. Obtiene una cota superior usando DSatur.
 *  2. Realiza una búsqueda binaria sobre el número de colores $k \in [2, \text{cota}]$.
 *  3. Para cada $k$, usa un algoritmo de backtracking para determinar si el grafo
 *     es $k$-coloreable.
 *
 *  @tparam GT Tipo de grafo.
 *  @tparam SA Filtro de arcos (por defecto @ref Dft_Show_Arc<GT>).
 *  @param[in] g Grafo (máximo 64 nodos).
 *  @param[out] colors Mapa que se llenará con una coloración óptima.
 *  @return El número cromático exacto.
 *
 *  @throws std::domain_error Si el grafo tiene más de 64 nodos.
 *
 *  @note **Complejidad**: Exponencial en el peor caso. Se limita a $V \le 64$ para
 *        evitar tiempos de ejecución excesivos.
 *
 *  @see dsatur_coloring, is_valid_coloring
 *  @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>>
size_t chromatic_number(const GT &g, DynMapTree<typename GT::Node *, size_t> &colors)
{
  using Node = typename GT::Node;

  ah_domain_error_if(g.get_num_nodes() > 64)
    << "chromatic_number: graph has " << g.get_num_nodes() << " nodes (max 64)";

  colors = {};

  if (g.get_num_nodes() == 0)
    return 0;

  // Caso trivial: grafo sin arcos, número cromático es 1.
  if (g.get_num_arcs() == 0)
    {
      for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
        colors.insert(it.get_curr(), 0);
      return 1;
    }

  // Paso 1: Cota superior con DSatur
  DynMapTree<Node *, size_t> best_colors;
  size_t                     upper = dsatur_coloring<GT, SA>(g, best_colors);

  if (upper <= 2)
    {
      colors = std::move(best_colors);
      return upper;
    }

  // Paso 2: Búsqueda exacta mediante backtracking.
  // Preparamos una representación indexada para mayor velocidad.
  DynList<Node *> node_list;
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    node_list.append(it.get_curr());

  const size_t n     = node_list.size();
  Node       **nodes = new Node *[n];
  size_t       idx   = 0;
  for (auto it = node_list.get_it(); it.has_curr(); it.next_ne())
    nodes[idx++] = it.get_curr();

  // Lista de adyacencia basada en índices.
  Array<DynList<size_t>> adj(n, DynList<size_t>{});
  {
    Cookie_Saver<GT> saver(g, true, false);
    for (size_t i = 0; i < n; ++i)
      NODE_COOKIE(nodes[i]) = reinterpret_cast<void *>(static_cast<uintptr_t>(i));

    for (size_t i = 0; i < n; ++i)
      for (Node_Arc_Iterator<GT, SA> ait(nodes[i]); ait.has_curr(); ait.next_ne())
        {
          Node  *w = ait.get_tgt_node();
          size_t j = reinterpret_cast<uintptr_t>(NODE_COOKIE(w));
          if (j != i)
            adj[i].append(j);
        }
  }

  // Estado del coloreado para el backtracking
  Array<size_t> coloring(n, static_cast<size_t>(0));

  auto reset_coloring = [&]() {
    for (size_t i = 0; i < n; ++i)
      coloring[i] = 0;
  };

  // Función interna de backtracking para verificar k-colorabilidad.
  auto try_k_coloring = [&](size_t k, auto &self, size_t node_idx) -> bool {
    if (node_idx == n)
      return true;

    for (size_t c = 0; c < k; ++c)
      {
        bool feasible = true;
        for (auto jit = adj[node_idx].get_it(); jit.has_curr(); jit.next_ne())
          {
            size_t j = jit.get_curr();
            if (j < node_idx and coloring[j] == c)
              {
                feasible = false;
                break;
              }
          }

        if (feasible)
          {
            coloring[node_idx] = c;
            if (self(k, self, node_idx + 1))
              return true;
          }
      }

    coloring[node_idx] = 0;
    return false;
  };

  // Búsqueda binaria para encontrar el mínimo k en el rango [2, upper).
  size_t lo = 2;
  size_t hi = upper;

  while (lo < hi)
    {
      size_t mid = (lo + hi) / 2;
      reset_coloring();
      if (try_k_coloring(mid, try_k_coloring, 0))
        hi = mid;
      else
        lo = mid + 1;
    }

  if (lo < upper)
    {
      // El valor encontrado es óptimo; reconstruimos el mapa de colores.
      reset_coloring();
      try_k_coloring(lo, try_k_coloring, 0);

      colors = {};
      for (size_t i = 0; i < n; ++i)
        colors.insert(nodes[i], coloring[i]);
    }
  else
    colors = std::move(best_colors);

  delete[] nodes;
  return lo;
}

/** @brief Functor para el coloreado ávido (@ref greedy_coloring).
 *
 *  Permite usar el algoritmo en contextos donde se requiera un objeto función.
 *
 *  @tparam GT Tipo de grafo.
 *  @tparam SA Filtro de arcos.
 *  @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>> class Greedy_Coloring
{
public:
  /** @brief Ejecuta el coloreado ávido.
   *  @param[in] g Grafo.
   *  @param[out] colors Resultado.
   *  @return Número de colores.
   */
  size_t operator()(const GT &g, DynMapTree<typename GT::Node *, size_t> &colors) const
  {
    return greedy_coloring<GT, SA>(g, colors);
  }
};

/** @brief Functor para el coloreado de Welsh-Powell (@ref welsh_powell_coloring).
 *
 *  @tparam GT Tipo de grafo.
 *  @tparam SA Filtro de arcos.
 *  @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>> class Welsh_Powell_Coloring
{
public:
  /** @brief Ejecuta Welsh-Powell.
   *  @param[in] g Grafo.
   *  @param[out] colors Resultado.
   *  @return Número de colores.
   */
  size_t operator()(const GT &g, DynMapTree<typename GT::Node *, size_t> &colors) const
  {
    return welsh_powell_coloring<GT, SA>(g, colors);
  }
};

/** @brief Functor para el coloreado DSatur (@ref dsatur_coloring).
 *
 *  @tparam GT Tipo de grafo.
 *  @tparam SA Filtro de arcos.
 *  @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>> class DSatur_Coloring
{
public:
  /** @brief Ejecuta DSatur.
   *  @param[in] g Grafo.
   *  @param[out] colors Resultado.
   *  @return Número de colores.
   */
  size_t operator()(const GT &g, DynMapTree<typename GT::Node *, size_t> &colors) const
  {
    return dsatur_coloring<GT, SA>(g, colors);
  }
};

}  // end namespace Aleph

#endif  // GRAPH_COLORING_H
