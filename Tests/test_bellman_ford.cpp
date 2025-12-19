/**
 * Complete test suite for Bellman_Ford.H
 *
 * This file contains exhaustive tests to verify the correctness
 * of the Bellman-Ford algorithm implementation in Aleph-w.
 */

#include <Bellman_Ford.H>
#include <tpl_graph.H>
#include <cassert>
#include <iostream>
#include <limits>

using namespace Aleph;
using namespace std;

// Graph type for tests
using GT = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
using Node = GT::Node;
using Arc = GT::Arc;

// ========== TEST 1: Simple Graph without Negative Cycles ==========
void test_simple_graph_no_negative_cycles() {
  cout << "Test 1: Simple graph without negative cycles... ";

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  Arc* a01 = g.insert_arc(n0, n1, 1);
  Arc* a02 = g.insert_arc(n0, n2, 4);
  Arc* a12 = g.insert_arc(n1, n2, 2);
  Arc* a13 = g.insert_arc(n1, n3, 5);
  Arc* a23 = g.insert_arc(n2, n3, 1);

  Bellman_Ford<GT> bf(g);

  // No debe detectar ciclos negativos
  bool has_negative_cycle = bf.has_negative_cycle(n0);
  assert(!has_negative_cycle);

  cout << "OK" << endl;
}

// ========== TEST 2: Grafo con Ciclo Negativo ==========
void test_graph_with_negative_cycle() {
  cout << "Test 2: Grafo con ciclo negativo... ";

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  Arc* a01 = g.insert_arc(n0, n1, 1);
  Arc* a12 = g.insert_arc(n1, n2, -3);
  Arc* a20 = g.insert_arc(n2, n0, 1); // Ciclo: 0->1->2->0 con peso total -1

  Bellman_Ford<GT> bf(g);

  // Debe detectar ciclo negativo
  bool has_negative_cycle = bf.has_negative_cycle(n0);
  assert(has_negative_cycle);

  cout << "OK" << endl;
}

// ========== TEST 3: Árbol de Caminos Mínimos ==========
void test_spanning_tree() {
  cout << "Test 3: Generación de árbol de caminos mínimos... ";

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  Arc* a01 = g.insert_arc(n0, n1, 1);
  Arc* a02 = g.insert_arc(n0, n2, 4);
  Arc* a12 = g.insert_arc(n1, n2, 2);
  Arc* a13 = g.insert_arc(n1, n3, 5);
  Arc* a23 = g.insert_arc(n2, n3, 1);

  Bellman_Ford<GT> bf(g);
  bool negative_cycle = bf.paint_spanning_tree(n0);

  assert(!negative_cycle);

  // Verificar que los arcos del árbol estén marcados
  int painted_arcs = 0;
  for (Arc_Iterator<GT> it(g); it.has_curr(); it.next()) {
    if (IS_ARC_VISITED(it.get_curr(), Aleph::Spanning_Tree)) {
      painted_arcs++;
    }
  }

  // El árbol debe tener n-1 arcos
  assert(painted_arcs == 3);

  cout << "OK" << endl;
}

// ========== TEST 4: Versión Rápida del Algoritmo ==========
void test_faster_version() {
  cout << "Test 4: Versión rápida del algoritmo... ";

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);
  Node* n4 = g.insert_node(4);

  g.insert_arc(n0, n1, 6);
  g.insert_arc(n0, n2, 7);
  g.insert_arc(n1, n2, 8);
  g.insert_arc(n1, n3, 5);
  g.insert_arc(n1, n4, -4);
  g.insert_arc(n2, n3, -3);
  g.insert_arc(n2, n4, 9);
  g.insert_arc(n3, n1, -2);
  g.insert_arc(n4, n0, 2);
  g.insert_arc(n4, n3, 7);

  Bellman_Ford<GT> bf(g);
  bool negative_cycle = bf.faster_paint_spanning_tree(n0);

  assert(!negative_cycle);

  cout << "OK" << endl;
}

// ========== TEST 5: Detección de Ciclo Negativo (versión completa) ==========
void test_negative_cycle_detection() {
  cout << "Test 5: Detección completa de ciclo negativo... ";

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, -1);
  g.insert_arc(n2, n3, -1);
  g.insert_arc(n3, n1, -1); // Ciclo negativo: 1->2->3->1 con peso -3

  Bellman_Ford<GT> bf(g);
  Path<GT> cycle = bf.test_negative_cycle(n0);

  // El ciclo no debe estar vacío
  assert(!cycle.is_empty());

  cout << "OK" << endl;
}

// ========== TEST 6: Búsqueda de Ciclo Negativo ==========
void test_search_negative_cycle() {
  cout << "Test 6: Búsqueda de ciclo negativo con parámetros... ";

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, -2);
  g.insert_arc(n2, n3, -2);
  g.insert_arc(n3, n1, -1); // Ciclo negativo

  Bellman_Ford<GT> bf(g);
  auto result = bf.search_negative_cycle(n0, 0.5, 2);

  Path<GT>& cycle = get<0>(result);
  size_t iterations = get<1>(result);

  assert(!cycle.is_empty());
  assert(iterations > 0);

  cout << "OK" << endl;
}

// ========== TEST 7: Grafo Vacío ==========
void test_empty_graph() {
  cout << "Test 7: Grafo vacío... ";

  GT g;
  Node* n0 = g.insert_node(0);

  Bellman_Ford<GT> bf(g);
  bool has_negative_cycle = bf.has_negative_cycle(n0);

  assert(!has_negative_cycle);

  cout << "OK" << endl;
}

// ========== TEST 8: Grafo con Peso Cero ==========
void test_graph_with_zero_weights() {
  cout << "Test 8: Grafo con pesos cero... ";

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 0);
  g.insert_arc(n1, n2, 0);
  g.insert_arc(n2, n0, 0);

  Bellman_Ford<GT> bf(g);
  bool has_negative_cycle = bf.has_negative_cycle(n0);

  // Ciclo con peso cero no es negativo
  assert(!has_negative_cycle);

  cout << "OK" << endl;
}

// ========== TEST 9: Grafo Desconectado ==========
void test_disconnected_graph() {
  cout << "Test 9: Grafo desconectado... ";

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  // Componente 1
  g.insert_arc(n0, n1, 1);

  // Componente 2 (desconectada)
  g.insert_arc(n2, n3, 2);

  Bellman_Ford<GT> bf(g);
  bool has_negative_cycle = bf.has_negative_cycle(n0);

  assert(!has_negative_cycle);

  cout << "OK" << endl;
}

// ========== TEST 10: Pesos Negativos sin Ciclo Negativo ==========
void test_negative_weights_no_cycle() {
  cout << "Test 10: Pesos negativos sin ciclo negativo... ";

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, -1);
  g.insert_arc(n1, n2, -2);
  g.insert_arc(n2, n3, -3);
  // No hay ciclo

  Bellman_Ford<GT> bf(g);
  bool has_negative_cycle = bf.has_negative_cycle(n0);

  assert(!has_negative_cycle);

  cout << "OK" << endl;
}

// ========== TEST 11: Grafo Grande con Múltiples Caminos ==========
void test_large_graph() {
  cout << "Test 11: Grafo grande con múltiples caminos... ";

  GT g;
  const int num_nodes = 100;
  vector<Node*> nodes;

  for (int i = 0; i < num_nodes; ++i) {
    nodes.push_back(g.insert_node(i));
  }

  // Crear una cadena larga
  for (int i = 0; i < num_nodes - 1; ++i) {
    g.insert_arc(nodes[i], nodes[i+1], 1);
  }

  // Agregar algunos arcos adicionales
  for (int i = 0; i < num_nodes - 5; i += 5) {
    g.insert_arc(nodes[i], nodes[i+5], 2);
  }

  Bellman_Ford<GT> bf(g);
  bool has_negative_cycle = bf.has_negative_cycle(nodes[0]);

  assert(!has_negative_cycle);

  cout << "OK" << endl;
}

// ========== TEST 12: Ciclo Negativo Complejo ==========
void test_complex_negative_cycle() {
  cout << "Test 12: Ciclo negativo complejo... ";

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);
  Node* n4 = g.insert_node(4);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 3);
  g.insert_arc(n2, n3, -2);
  g.insert_arc(n3, n4, -1);
  g.insert_arc(n4, n2, -3); // Ciclo negativo: 2->3->4->2 con peso -6

  Bellman_Ford<GT> bf(g);
  Path<GT> cycle = bf.test_negative_cycle(n0);

  assert(!cycle.is_empty());

  cout << "OK" << endl;
}

// ========== TEST 13: test_negative_cycle con Output Parameter ==========
void test_negative_cycle_with_output_param() {
  cout << "Test 13: Detección de ciclo con parámetro de salida... ";

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, -3);
  g.insert_arc(n2, n0, 1);

  Bellman_Ford<GT> bf(g);
  Path<GT> cycle;
  bool has_cycle = bf.test_negative_cycle(n0, cycle);

  assert(has_cycle);
  assert(!cycle.is_empty());

  cout << "OK" << endl;
}

// ========== TEST 14: search_negative_cycle sin Nodo de Inicio ==========
void test_search_negative_cycle_no_start() {
  cout << "Test 14: Búsqueda de ciclo negativo en todo el grafo... ";

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  // Componente sin ciclo negativo
  g.insert_arc(n0, n1, 1);

  // Componente con ciclo negativo
  g.insert_arc(n1, n2, 1);
  g.insert_arc(n2, n3, -2);
  g.insert_arc(n3, n1, -1);

  Bellman_Ford<GT> bf(g);
  Path<GT> cycle = bf.search_negative_cycle();

  assert(!cycle.is_empty());

  cout << "OK" << endl;
}

// ========== TEST 15: Bellman_Ford_Negative_Cycle Functor ==========
void test_bellman_ford_negative_cycle_functor() {
  cout << "Test 15: Functor Bellman_Ford_Negative_Cycle... ";

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, -3);
  g.insert_arc(n2, n0, 1);

  Bellman_Ford_Negative_Cycle<GT> detector;
  Path<GT> cycle;
  bool has_cycle = detector(g, cycle);

  assert(has_cycle);
  assert(!cycle.is_empty());

  cout << "OK" << endl;
}

// ========== TEST 16: Grafo con Autoloop Negativo ==========
void test_negative_self_loop() {
  cout << "Test 16: Grafo con autoloop negativo... ";

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n1, -1); // Autoloop negativo

  Bellman_Ford<GT> bf(g);
  bool has_negative_cycle = bf.has_negative_cycle(n0);

  assert(has_negative_cycle);

  cout << "OK" << endl;
}

// ========== TEST 17: Todos los Pesos Negativos sin Ciclo ==========
void test_all_negative_weights_no_cycle() {
  cout << "Test 17: Todos los pesos negativos sin ciclo... ";

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, -1);
  g.insert_arc(n1, n2, -2);
  g.insert_arc(n2, n3, -3);
  g.insert_arc(n0, n2, -4);

  Bellman_Ford<GT> bf(g);
  bool has_negative_cycle = bf.has_negative_cycle(n0);

  assert(!has_negative_cycle);

  cout << "OK" << endl;
}

// ========== TEST 18: Nullptr Validation ==========
void test_nullptr_validation() {
  cout << "Test 18: Validación de nullptr... ";

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  g.insert_arc(n0, n1, 1);

  Bellman_Ford<GT> bf(g);

  bool caught_exception = false;
  try {
    bf.paint_spanning_tree(nullptr);
  } catch (const domain_error&) {
    caught_exception = true;
  }
  assert(caught_exception);

  caught_exception = false;
  try {
    bf.faster_paint_spanning_tree(nullptr);
  } catch (const domain_error&) {
    caught_exception = true;
  }
  assert(caught_exception);

  caught_exception = false;
  try {
    bf.has_negative_cycle(nullptr);
  } catch (const domain_error&) {
    caught_exception = true;
  }
  assert(caught_exception);

  cout << "OK" << endl;
}

// ========== TEST 19: Completely Empty Graph ==========
void test_completely_empty_graph() {
  cout << "Test 19: Grafo completamente vacío (0 nodos)... ";

  GT g;
  // No se insertan nodos

  Bellman_Ford<GT> bf(g);

  // has_negative_cycle() sin parámetros debe funcionar
  bool has_cycle = bf.has_negative_cycle();
  assert(!has_cycle);

  cout << "OK" << endl;
}

// ========== TEST 20: Build Tree ==========
void test_build_tree() {
  cout << "Test 20: Construcción de árbol de caminos mínimos... ";

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n0, n2, 4);
  g.insert_arc(n1, n2, 2);
  g.insert_arc(n1, n3, 5);
  g.insert_arc(n2, n3, 1);

  Bellman_Ford<GT> bf(g);
  bf.paint_spanning_tree(n0);

  GT tree;
  bf.build_tree(tree, false);

  // El árbol debe tener el mismo número de nodos
  assert(tree.get_num_nodes() == g.get_num_nodes());

  // El árbol debe tener n-1 arcos
  assert(tree.get_num_arcs() == g.get_num_nodes() - 1);

  cout << "OK" << endl;
}

// ========== TEST 21: Extract Min Spanning Tree ==========
void test_extract_min_spanning_tree() {
  cout << "Test 21: Extracción de árbol de caminos mínimos... ";

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n0, n2, 4);
  g.insert_arc(n1, n2, 2);
  g.insert_arc(n1, n3, 5);
  g.insert_arc(n2, n3, 1);

  Bellman_Ford<GT> bf(g);
  bf.paint_spanning_tree(n0);

  DynArray<Arc*> tree_arcs = bf.extract_min_spanning_tree();

  // Debe haber n-1 arcos (algunos pueden ser nullptr)
  int non_null_arcs = 0;
  for (size_t i = 0; i < tree_arcs.size(); ++i) {
    if (tree_arcs(i) != nullptr) {
      non_null_arcs++;
    }
  }

  assert(non_null_arcs == 3); // n-1 para 4 nodos

  cout << "OK" << endl;
}

// ========== TEST 22: Compute Nodes Weights (Johnson) ==========
void test_compute_nodes_weights() {
  cout << "Test 22: Cálculo de pesos de nodos (Johnson)... ";

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 2);
  g.insert_arc(n2, n0, 3);

  Bellman_Ford<GT> bf(g);

  auto weights = bf.compute_nodes_weights();

  // Debe haber un peso para cada nodo
  assert(weights.size() == g.get_num_nodes());

  cout << "OK" << endl;
}

// ========== TEST 23: Compute Nodes Weights with Negative Cycle ==========
void test_compute_nodes_weights_negative_cycle() {
  cout << "Test 23: Compute weights con ciclo negativo... ";

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, -3);
  g.insert_arc(n2, n0, 1); // Ciclo negativo

  Bellman_Ford<GT> bf(g);

  bool caught_exception = false;
  try {
    bf.compute_nodes_weights();
  } catch (const domain_error&) {
    caught_exception = true;
  }

  assert(caught_exception);

  cout << "OK" << endl;
}

// ========== TEST 24: has_negative_cycle Without Start Node ==========
void test_has_negative_cycle_no_start() {
  cout << "Test 24: has_negative_cycle sin nodo inicial... ";

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, -3);
  g.insert_arc(n2, n0, 1); // Ciclo negativo

  Bellman_Ford<GT> bf(g);
  bool has_cycle = bf.has_negative_cycle();

  assert(has_cycle);

  cout << "OK" << endl;
}

// ========== TEST 25: test_negative_cycle Without Parameters ==========
void test_negative_cycle_no_params() {
  cout << "Test 25: test_negative_cycle sin parámetros... ";

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, -3);
  g.insert_arc(n2, n0, 1);

  Bellman_Ford<GT> bf(g);
  Path<GT> cycle;
  bool has_cycle = bf.test_negative_cycle(cycle);

  assert(has_cycle);
  assert(!cycle.is_empty());

  cout << "OK" << endl;
}

// ========== MAIN ==========
int main() {
  cout << "==============================================\n";
  cout << "  Batería de Tests para Bellman_Ford.H\n";
  cout << "==============================================\n\n";

  try {
    test_simple_graph_no_negative_cycles();
    test_graph_with_negative_cycle();
    test_spanning_tree();
    test_faster_version();
    test_negative_cycle_detection();
    test_search_negative_cycle();
    test_empty_graph();
    test_graph_with_zero_weights();
    test_disconnected_graph();
    test_negative_weights_no_cycle();
    test_large_graph();
    test_complex_negative_cycle();
    test_negative_cycle_with_output_param();
    test_search_negative_cycle_no_start();
    test_bellman_ford_negative_cycle_functor();
    test_negative_self_loop();
    test_all_negative_weights_no_cycle();
    test_nullptr_validation();
    test_completely_empty_graph();
    test_build_tree();
    test_extract_min_spanning_tree();
    test_compute_nodes_weights();
    test_compute_nodes_weights_negative_cycle();
    test_has_negative_cycle_no_start();
    test_negative_cycle_no_params();

    cout << "\n==============================================\n";
    cout << "  ✓ Todos los 25 tests pasaron exitosamente\n";
    cout << "==============================================\n";

    return 0;
  } catch (const exception& e) {
    cerr << "\n✗ Error: " << e.what() << endl;
    return 1;
  }
}