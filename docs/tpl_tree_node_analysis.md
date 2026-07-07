# Análisis intensivo de `tpl_tree_node.H`

**Fecha:** 2026-07-05
**Alcance:** `tpl_tree_node.H` (1299 líneas, clase `Tree_Node<T>` — árbol n-ario con representación primer-hijo/hermano-siguiente sobre listas circulares doblemente enlazadas `Dlink`) y sus consumidores en el resto del repositorio.

**Metodología:** lectura completa del fichero, reconstrucción del esquema interno de enlaces (no documentado en el propio código), y verificación empírica de cada hallazgo mediante programas de prueba independientes compilados con `-fsanitize=address,undefined`. Todos los bugs reportados aquí están confirmados con una reproducción reducida, no son solo sospechas de lectura.

---

## Resumen ejecutivo

| # | Hallazgo | Severidad | Estado |
|---|----------|-----------|--------|
| 1 | `bin_to_forest`/`bin_to_tree` corrompen `is_root()` en todos los árboles del bosque salvo el primero | **Crítico** | Confirmado con crash reproducible |
| 2 | `insert_right_sibling` no valida `is_root()` (asimetría con `insert_left_sibling`) | **Alto** | Confirmado con crash reproducible |
| 3 | Ejemplo de documentación en `random_tree.H` no compilaría (`destroyRec` en vez de `destroy_tree`) | Medio | Confirmado por inspección de firmas |
| 4 | Macro `IS_UNIQUE_SIBLING` siempre evalúa falso (código muerto, inofensivo) | Bajo | Confirmado empíricamente |
| 5 | El mecanismo interno de "espina compartida" (child-list como enlace ascendente) no está documentado en ningún lugar | Alto (mantenibilidad) | Confirmado por ingeniería inversa |
| 6 | Banderas `is_root/is_leaf/is_leftmost/is_rightmost` mantenidas a mano, no derivadas | Medio (diseño) | Observación |
| 7 | Huecos de test: `get_parent()`, rama compleja de `insert_leftmost_child`, `bin_to_forest` | Alto (proceso) | Confirmado por cobertura |
| 8 | `Tree_Node_Vtl` y `Children_Set` son código muerto | Bajo | Confirmado por grep |
| 9 | API pública sin documentar (`get_child_list`, `set_is_root`, etc.) | Bajo | Confirmado por inspección |

El hallazgo #1 es el más importante: es un bug real, con consecuencias observables (crash de aserción / comportamiento indefinido en Release), que además **ya era conocido** — hay un workaround manual escrito en `Examples/deway.C` en vez de corregirse en la librería.

---

## 1. Bugs confirmados

### 1.1 `bin_to_forest`/`bin_to_tree` corrompen `is_root()` en los árboles del bosque (CRÍTICO)

**Ubicación:** `tpl_tree_node.H:1241-1249` (`insert_sibling`), usado por `bin_to_tree` (líneas 1251-1266) y `bin_to_forest` (líneas 1285-1294).

```cpp
template <class TNode, class BNode> inline static
void insert_sibling(BNode * rnode, TNode * tree_node)
{
  if (rnode == BNode::NullPtr)
    return;

  auto * sibling = new TNode(KEY(rnode));
  tree_node->insert_right_sibling(sibling);   // <-- aquí está el bug
}
```

`bin_to_tree` usa el subárbol derecho de un `BinNode` para representar los "hermanos" en un **bosque** (la transformación clásica árbol-binario ⇄ bosque). Pero para enlazar esos hermanos usa `insert_right_sibling`, que es la función pensada para insertar un hermano **dentro de la lista de hijos de un padre común** — y por eso, correctamente, hace `p->set_is_root(false)` (líneas 268-269), ya que ese nuevo hermano deja de ser raíz al convertirse en hijo de alguien.

El problema es que, en el contexto de `bin_to_tree`, el "hermano" insertado es en realidad **la raíz de otro árbol del bosque**, y debería conservar `is_root() == true`, tal como sí hace correctamente la función hermana `insert_tree_to_right` (líneas 453-471), que **nunca** toca `is_root()` del árbol insertado.

**Reproducción** (repo local, `/tmp/.../probe5.cc`):

```cpp
BinNode<int> n1(1), n2(2), n3(3);
RLINK(&n1) = &n2;
RLINK(&n2) = &n3;
Tree_Node<int> *forest = bin_to_forest<Tree_Node<int>, BinNode<int>>(&n1);

for (auto *t = forest; t != nullptr; t = t->get_right_sibling())
  std::cout << "key=" << t->get_key() << " is_root()=" << t->is_root() << "\n";
```

Salida real:
```
key=1 is_root()=1   (correcto)
key=2 is_root()=0   (INCORRECTO: debería ser 1)
key=3 is_root()=0   (INCORRECTO: debería ser 1)
```

**Consecuencia observable:** cualquier código que confíe en `is_root()` para saber si un nodo es la cabeza de un árbol se rompe. En particular, `get_parent()` deja de hacer *short-circuit* y termina violando su propia aserción interna:

```cpp
Tree_Node<int> *second = forest->get_right_sibling();
second->get_parent();
// tpl_tree_node.H:246: Assertion `not ISROOT(p)' failed.  (abort)
```

En una build de Release (`NDEBUG`, sin asserts) esto no aborta, sino que **devuelve un puntero incorrecto** en vez de `nullptr` — es decir, comportamiento indefinido silencioso, peor que un crash.

**Ya es un problema conocido, no corregido:** `Examples/deway.C:246-249` documenta exactamente este bug con un workaround manual en el código cliente, en vez de arreglarlo en la librería:

```cpp
// bin_to_forest creates a forest where sibling trees are linked,
// but the is_root flag is not set correctly for siblings.
// We fix this by setting is_root=true for all sibling trees.
for (auto* t = tree; t != nullptr; t = t->get_right_sibling())
  t->set_is_root(true);
```

**Por qué nadie lo detectó en CI:** no existe ningún test (`Tests/`) que llame a `bin_to_forest`/`bin_to_tree` y verifique `is_root()` del resultado. El único consumidor de prueba es `Examples/test_bin_to_tree.C` (un ejemplo, no un test de GTest con aserciones), y aplica su propio workaround también.

**Recomendación:** en `insert_sibling` (línea 1247), reemplazar `tree_node->insert_right_sibling(sibling)` por una llamada equivalente a `insert_tree_to_right` (o replicar su lógica), preservando `is_root() == true` en el árbol insertado. Agregar un test de GTest que compruebe `is_root()` de cada árbol tras `bin_to_forest`. Eliminar el workaround de `Examples/deway.C` una vez corregido.

---

### 1.2 `insert_right_sibling` no valida `is_root()` — asimetría con `insert_left_sibling`

**Ubicación:** `tpl_tree_node.H:258-285` (`insert_right_sibling`) vs. `293-342` (`insert_left_sibling`).

`insert_left_sibling` valida explícitamente su precondición con una macro de error real (activa también en Release):

```cpp
void insert_left_sibling(Tree_Node * p)
{
  if (p == nullptr) return;
  ah_domain_error_if(this->is_root())
    << "Cannot insert sibling of a root";
  ...
```

`insert_right_sibling`, que tiene exactamente la misma precondición semántica (`this` no puede ser una raíz de árbol suelta si se le va a insertar un hermano "hijo de mi mismo padre"), **no tiene ningún chequeo equivalente** — ni con `ah_domain_error_if` ni siquiera con `assert`.

**Reproducción:**

```cpp
Tree_Node<int> root(0);
auto *child = new Tree_Node<int>(1);
root.insert_leftmost_child(child);

auto *p = new Tree_Node<int>(9);
root.insert_right_sibling(p);   // mal uso: `root` es una raíz real de árbol

p->get_parent();
// tpl_tree_node.H:246: Assertion `not ISROOT(p)' failed.  (abort)
```

Al igual que en el hallazgo anterior, en Release esto no aborta: se propaga como corrupción de estado silenciosa (dos nodos con banderas `is_root`/`is_leftmost` mutuamente inconsistentes), que luego afecta a cualquier navegación posterior (`get_parent()`, y transitivamente cualquier algoritmo que dependa de él, como `Gen_Mo_On_Tree_Node::get_parent()` en `tpl_mo_on_trees.H:799`).

**Recomendación compatible:** no hacer que este caso empiece a lanzar sin más,
porque hay código existente que usa `insert_right_sibling` para enlazar raíces de
un bosque. En su lugar, si `this->is_root()`, delegar en la semántica correcta
de bosque (`insert_tree_to_right`) para preservar `is_root()` en el árbol
insertado; si `this` no es raíz, conservar la semántica actual de hermano dentro
de la lista de hijos. Agregar un test de regresión que cubra ambos usos.

---

### 1.3 Ejemplo de documentación roto en `random_tree.H`

**Ubicación:** `random_tree.H:60` y `random_tree.H:181` (comentarios `@par Example`), función `operator()` en línea 184-193.

El ejemplo de uso documentado dice:

```cpp
/**
 * RandTree<int> gen(42);
 * Tree_Node<int>* tree = gen(100);
 * // ... use tree ...
 * destroyRec(tree);  // Don't forget to clean up!
 */
```

Pero `destroyRec` (definida en `tpl_binNodeUtils.H:521-530`) es genérica sobre nodos de **árbol binario tipo `BinNode`**, y requiere que el tipo tenga un miembro estático `Node::NullPtr` y las macros `LLINK`/`RLINK`:

```cpp
template <class Node> inline
void destroyRec(Node *& root) noexcept
{
  if (root == Node::NullPtr)   // Tree_Node<T> no tiene NullPtr
    return;
  destroyRec(LLINK(root));     // Tree_Node<T> no tiene LLINK
  destroyRec(RLINK(root));
  delete root;
  root = Node::NullPtr;
}
```

`Tree_Node<T>` no define `NullPtr` (usa `nullptr` directamente) ni `LLINK`/`RLINK`. Si alguien copia literalmente el ejemplo de la documentación, **no compila** (`error: no member named 'NullPtr' in 'Aleph::Tree_Node<int>'`). La función correcta para liberar un `Tree_Node<T>*` es `destroy_tree` (definida en el propio `tpl_tree_node.H:1002-1023`), curiosamente ya usada correctamente dentro del mismo `random_tree.H` en otro punto para el `BinNode` temporal (`destroyRec(l);`, línea 192, donde `l` sí es un `BinNode<T>*`) — el autor mezcló las dos funciones al escribir el ejemplo.

**Recomendación:** corregir ambos comentarios de ejemplo (`random_tree.H:60` y `:181`) para usar `destroy_tree(tree)` en vez de `destroyRec(tree)`.

---

### 1.4 Macro `IS_UNIQUE_SIBLING` siempre falsa (código muerto, inofensivo)

**Ubicación:** `tpl_tree_node.H:70` (definición), `:1008` (único uso, en `destroy_tree`).

```cpp
# define RSIBLING(p)     ((p)->get_right_sibling())
# define IS_UNIQUE_SIBLING(p) (RSIBLING(p) == (p))
```

La intención es detectar si `p` es el único miembro de su lista de hermanos (para saltarse un `del()` innecesario). Pero `get_right_sibling()` está definida para devolver `nullptr` cuando `p` es el último de la lista (`is_rightmost()`), **nunca** el propio puntero `p` — por lo tanto `RSIBLING(p) == p` no puede ser cierto jamás, sin importar la forma del árbol.

**Verificación:**

```cpp
Tree_Node<int> lone(42);   // sin hermanos: leftmost y rightmost a la vez
// RSIBLING(&lone) == &lone  →  false
// RSIBLING(&lone) == nullptr →  true
// IS_UNIQUE_SIBLING(&lone)  →  false   (debería ser true)
```

**Impacto real:** ninguno funcionalmente grave — en `destroy_tree` (línea 1008), la rama `if (not IS_UNIQUE_SIBLING(root)) SIBLING_LIST(root)->del();` **siempre** se ejecuta, incluso cuando sería innecesaria (nodo sin hermanos). Llamar a `Dlink::del()` sobre una lista ya vacía/auto-referenciada es inofensivo (es un no-op efectivo dado cómo está implementado `Dlink::del()`), así que esto es únicamente trabajo redundante, no corrupción. Lo mismo ocurre de forma anidada en `destroy_forest` (línea 1050), que ya desenlaza manualmente antes de llamar a `destroy_tree`, haciendo el `del()` interno doblemente redundante.

**Recomendación:** o bien arreglar la macro para comparar contra el header real de la lista (o usar `Dlink::is_unitarian_or_empty()`/`is_empty()`, ya disponibles en `dlink.H:246-252`), o eliminarla si no aporta valor tras el arreglo. Es una limpieza de bajo riesgo.

---

## 2. Deuda de diseño y mantenibilidad

### 2.1 El mecanismo de "espina compartida" no está documentado

Este es, en mi opinión, el hallazgo estructural más importante del análisis, aunque no sea un "bug" en sí mismo.

`Tree_Node` usa **un único campo `Dlink child` por nodo** tanto para (a) encabezar la lista circular de los propios hijos del nodo, como (b) — cuando el nodo es el hijo más a la izquierda de su padre — enlazarse con el campo `child` del padre, formando una **lista circular compartida que atraviesa toda la "espina izquierda"** de un árbol, desde la raíz hasta la hoja más profunda por el lado izquierdo. Esto es lo que permite que `get_parent()` (líneas 237-250) funcione en O(1) sin almacenar un puntero explícito al padre, y es también lo que exige la intrincada danza de `cut_list`/`insert` que se ve en la rama `else` de `insert_leftmost_child` (líneas 364-380) y `insert_left_sibling` (líneas 315-338).

**No hay ni un comentario en todo el fichero que explique esto.** Reconstruir el esquema exigió trazar `upper_link()`/`lower_link()` (líneas 109-117), leer `dlink.H` para entender `cut_list`/`insert`/`del` con precisión, y verificar empíricamente con programas de prueba que la reconstrucción era correcta (lo cual, afortunadamente, lo era — ver más abajo).

**Riesgo concreto:** cualquier futura modificación a `insert_leftmost_child`, `insert_rightmost_child`, `insert_left_sibling`, `insert_right_sibling`, `join`, `insert_tree_to_right` o `get_parent()` sin conocer este esquema puede romper la propiedad "el campo `child` del hijo izquierdo apunta de vuelta al padre" de forma sutil y silenciosa — exactamente el tipo de bug que ya ocurrió en el hallazgo 1.1.

**Recomendación:** agregar un bloque de comentario a nivel de clase (o de fichero) explicando el esquema de enlace: qué representa `child`/`sibling` en cada caso, cómo `upper_link()`/`lower_link()` navegan la "espina compartida", y qué invariante debe preservar cualquier función que la manipule (`p->child` de un hijo izquierdo siempre debe apuntar, vía `get_prev()`, al `child` de su padre).

### 2.2 Banderas booleanas mantenidas a mano en vez de derivadas

```cpp
struct Flags
{
  unsigned int is_root      : 1;
  unsigned int is_leaf      : 1;
  unsigned int is_leftmost  : 1;
  unsigned int is_rightmost : 1;
  ...
};
```

`is_leftmost`/`is_rightmost` son necesariamente banderas explícitas dado que las listas son circulares y sin nodo de cabecera dedicado (un ahorro de memoria deliberado; no hay forma barata de saber "soy el primero/último" sin ello). Pero `is_leaf()` parece derivable directamente de `child.is_empty()` (`Dlink::is_empty()`, ya disponible en O(1) en `dlink.H:246`): en todos los sitios donde se hace `set_is_leaf(false)` (líneas 361, 402, 427), inmediatamente antes o después se hace `CHILD_LIST(this)->insert(...)` o `->splice(...)`, es decir, el campo `child` deja de estar vacío exactamente en el mismo momento en que `is_leaf` pasa a `false`. Si esto se confirma con más profundidad, `is_leaf()` podría implementarse como `return child.is_empty();` eliminando un bit de estado que hay que recordar sincronizar manualmente en cada función nueva que se agregue — precisamente la clase de "olvido" que causó el hallazgo 1.1 con `is_root`.

Esto es una recomendación de diseño, no un bug: el cambio tiene riesgo (afecta la representación en memoria y el contrato `noexcept` de varias funciones) y debería hacerse como una refactorización deliberada con su propia batería de tests, no como parte de un parche puntual.

### 2.3 Inconsistencia entre `assert()` y `ah_*_error_if` para precondiciones similares

CLAUDE.md establece usar las macros `ah_*_error_if` (siempre activas) en vez de `throw` crudo, pero el fichero mezcla ese estilo con `assert()` puro (compilado fuera en Release/`NDEBUG`) para precondiciones de igual criticidad:

- `insert_leftmost_child`/`insert_rightmost_child`/`insert_right_sibling` validan "`p` debe ser un nodo nuevo y aislado" (`CHILD_LIST(p)->is_empty()`, `SIBLING_LIST(p)->is_empty()`, `p->is_root() and p->is_leaf() and ...`) **solo** con `assert()` — en Release, pasar un nodo ya usado en otro árbol corrompe silenciosamente ambos árboles.
- `insert_left_sibling` sí usa `ah_domain_error_if` para su precondición "`this` no puede ser root", pero el resto de sus asserts (líneas 301-304) son también asserts puros.
- `destroy_forest`/`forest_preorder_traversal`/`forest_postorder_traversal` sí usan `ah_domain_error_if` consistentemente para sus precondiciones de "raíz"/"leftmost".

No es necesariamente incorrecto tener una mezcla (algunos asserts son genuinamente solo para depuración interna, más baratos de omitir en Release), pero la línea entre "esto se degrada silenciosamente en Release" y "esto siempre lanza" no parece trazada según un criterio explícito, sino más bien accidental. Vale la pena revisar cuáles de estas precondiciones (en particular las de "`p` debe estar aislado" en las cuatro funciones de inserción) merecen promoverse a `ah_domain_error_if` dado que su violación produce corrupción de datos, no solo un resultado incorrecto localizado.

---

## 3. Huecos de cobertura de tests

La suite dedicada `Tests/tree-node.cc` tiene 13 `TEST`/`TEST_F`. Todos los árboles de prueba (`sample_tree`, en `Tests/tree-node-common.H:52-65`) se construyen **exclusivamente con `insert_rightmost_child`**, lo cual deja sin ejercitar directamente:

1. **`get_parent()` sin aserción directa en ningún test** — se usa indirectamente dentro de `insert_left_sibling`, pero ningún `TEST` hace `ASSERT_EQ(nodo.get_parent(), ...)`.
2. **La rama compleja de `insert_leftmost_child`** (líneas 364-380, cuando el destino ya tiene hijos) — no se llama nunca con un destino no-hoja en la suite de tests. **Verificado que funciona correctamente** mediante una prueba independiente (ver Apéndice, `probe1.cc`), pero sin ningún test de regresión que lo proteja en el repositorio.
3. **La rama compleja de `insert_left_sibling`** (líneas 315-338, cuando `this` es el actual hermano más a la izquierda) — tampoco se ejercita en `Tests/tree-node.cc`. Sí se ejercita **indirectamente** en `Tests/prefix_tree_test.cc` (test `ChildrenSortedOrder`, que inserta `'c'` y luego `'a'`, forzando a `'a'` a convertirse en el nuevo primer hijo) y ese test pasa — lo cual es tranquilizador, pero significa que la única red de seguridad de este mecanismo delicado de `tpl_tree_node.H` vive en el test de un consumidor distinto, no en el propio fichero.
4. **`bin_to_forest`/`bin_to_tree` sin ningún test que revise `is_root()`** — exactamente el vacío que permitió que el bug 1.1 pasara desapercibido.
5. **El mal uso de `insert_right_sibling` sobre una raíz** (bug 1.2) tampoco tiene un test negativo que lo capture.

**Recomendación:** agregar a `Tests/tree-node.cc` (o un nuevo fichero de test dedicado):
- Un test que construya un árbol con `insert_leftmost_child` sobre un nodo con hijos existentes y verifique `get_parent()` en varios niveles.
- Un test que llame `insert_left_sibling` sobre el hermano actualmente más a la izquierda y verifique la nueva estructura (independientemente de la cobertura indirecta vía `prefix_tree_test.cc`).
- Un test de `bin_to_forest`/`bin_to_tree` que verifique `is_root() == true` para **todos** los árboles del bosque resultante, no solo el primero.
- Un test negativo para `insert_right_sibling(p)` llamado sobre una raíz genuina, una vez se agregue la validación del hallazgo 1.2.

---

## 4. Código muerto / limpieza menor

### 4.1 `Tree_Node_Vtl` nunca se usa

```cpp
template <typename T>
struct Tree_Node_Vtl : public Tree_Node<T>
{
  virtual ~Tree_Node_Vtl() = default;
};
```

(línea 770-774). Ni esta clase ni ningún tipo derivado de ella aparecen en ningún otro lugar del repositorio (`grep` exhaustivo sobre `.H`/`.cc`/`.C`). Es plausiblemente una utilidad "por si acaso" para poder destruir polimórficamente a través de un puntero `Tree_Node<T>*`, pero al día de hoy es código sin usar ni probar.

**Recomendación:** o se documenta su caso de uso previsto con un ejemplo/test, o se elimina para reducir superficie de API sin mantenimiento.

### 4.2 `Children_Set` nunca se usa

```cpp
struct Children_Set // trick to use Pair_Iterator
{
  Children_Set(const Tree_Node &&) {}
  Children_Set(const Tree_Node &) {}
  struct Iterator : public Children_Iterator
  {
    using Children_Iterator::Children_Iterator;
  };
};
```

(línea 651-659). El propio comentario ("trick to use Pair_Iterator") promete una integración con `Pair_Iterator` que no aparece ejercitada en ningún sitio del repositorio. Es, de nuevo, código muerto: compila, pero nadie lo usa ni lo prueba.

**Recomendación:** igual que el anterior — documentar el caso de uso con un ejemplo real que use `Pair_Iterator<Children_Set>`, o eliminarlo.

### 4.3 `const_cast` innecesario en `Children_Iterator`

```cpp
Children_Iterator(const Children_Iterator & it) noexcept
  : curr(const_cast<Children_Iterator&>(it).curr) {}
```

(línea 624-625). `curr` es un simple `Tree_Node * curr` (línea 611); copiar su valor desde una referencia `const` no requiere ningún `const_cast` — `curr(it.curr)` es exactamente equivalente y no tiene ningún efecto secundario de mutar `it`. El `const_cast` aquí no cumple ninguna función real; es ruido que puede hacer pensar a un lector que hay algo más profundo ocurriendo.

**Recomendación:** simplificar a `curr(it.curr)`.

---

## 5. Documentación pública incompleta

Las siguientes funciones públicas, con uso confirmado **fuera** de `tpl_tree_node.H` (no son solo detalles internos), no tienen ningún comentario Doxygen, incumpliendo la política del proyecto ("Public APIs require Doxygen in English"):

| Función | Línea | Usada externamente en |
|---|---|---|
| `get_child_list()` | 146 | `Tests/tree-node.cc:57` |
| `get_sibling_list()` | 148 | `Tests/tree-node.cc:58` |
| `set_is_root(bool)` | 162 | `Examples/deway.C:250` (el workaround del bug 1.1) |
| `set_is_leaf(bool)` | 164 | — (solo interno hoy) |
| `set_is_leftmost(bool)` | 166 | — (solo interno hoy) |
| `set_is_rightmost(bool)` | 168 | — (solo interno hoy) |

Los cuatro `set_is_*` son particularmente delicados de dejar sin documentar: dado el esquema de la sección 2.1, mutar estas banderas sin entender sus invariantes es exactamente lo que causó (o al menos permitió trabajar alrededor de) el bug 1.1. Como mínimo merecen una nota de advertencia explicando qué otras estructuras deben mantenerse consistentes al usarlas manualmente.

---

## 6. Revisión de usos (consumidores)

| Fichero | Cómo usa `Tree_Node` | Observación |
|---|---|---|
| `prefix-tree.H` (`Cnode`) | `insert_rightmost_child`, `insert_left_sibling` según orden alfabético | Uso correcto y no trivial: es el único consumidor de producción que ejercita la rama compleja de `insert_left_sibling` (ver sección 3, punto 3). Vale la pena promover sus propios tests (`Tests/prefix_tree_test.cc`) como referencia al escribir los tests directos que faltan en `tpl_tree_node.H`. |
| `tpl_link_cut_tree.H` (`export_to_tree_node`) | Solo `insert_rightmost_child` | Uso simple y seguro; no expuesto a ninguno de los bugs de esta lista. |
| `graph_to_tree.H` (`build_tree_dfs`/`build_tree_bfs`) | Solo `insert_rightmost_child` | Igual que el anterior. |
| `tpl_mo_on_trees.H` (`Gen_Mo_On_Tree_Node`) | `get_parent()` (línea 799) | Depende de la corrección de `get_parent()`, que a su vez depende de que `is_root()` esté siempre bien mantenido — se beneficiaría directamente de corregir el hallazgo 1.1/1.2 si alguna vez se le pasa un árbol construido vía `bin_to_forest` o construido con `insert_right_sibling` mal usado. |
| `random_tree.H` (`RandTree::operator()`) | Construye el árbol vía `bin_to_tree` internamente | Su ejemplo de documentación está roto (hallazgo 1.3); además, indirectamente, cualquier árbol que produzca a través de `bin_to_tree` con más de un "hermano de nivel superior" heredaría el bug 1.1 si el código se extendiera para generar bosques en vez de árboles únicos (hoy genera un único árbol, por lo que no dispara el bug en su forma actual). |
| `Examples/deway.C` | Workaround manual del bug 1.1 | Confirma que el bug es conocido desde hace tiempo y nunca se centralizó el arreglo en la librería. |
| `generate_df_tree.H` / `generate_tree.H` | Solo tipos/plantillas de escritura (`Write_Low`, conversión desde grafo) | Sin llamadas directas a las funciones de inserción problemáticas; no se identificaron issues adicionales en la revisión. |

---

## 7. Recomendaciones priorizadas

1. **(Crítico)** Corregir `insert_sibling` (línea 1247) para que `bin_to_tree`/`bin_to_forest` preserven `is_root()` correctamente en cada árbol del bosque resultante. Agregar test de regresión. Eliminar el workaround de `Examples/deway.C` una vez corregido.
2. **(Alto)** Hacer que `insert_right_sibling` sea seguro sobre una raíz sin romper compatibilidad: si `this` es raíz, debe enlazar otro árbol del bosque preservando `is_root()`; si no es raíz, debe conservar la semántica de hermano-hijo. Agregar test de regresión.
3. **(Alto)** Documentar el esquema de "espina compartida" (`child` como enlace ascendente para el hijo más a la izquierda) en un comentario a nivel de clase — el activo de mantenibilidad más valioso que puede salir de este análisis.
4. **(Medio)** Agregar tests directos en `Tests/tree-node.cc` para `get_parent()`, la rama compleja de `insert_leftmost_child`, y `bin_to_forest`/`is_root()`.
5. **(Medio)** Corregir el ejemplo de documentación en `random_tree.H` (`destroyRec` → `destroy_tree`).
6. **(Bajo)** Arreglar o eliminar `IS_UNIQUE_SIBLING`; documentar o eliminar `Tree_Node_Vtl`/`Children_Set`; quitar el `const_cast` superfluo en `Children_Iterator`; documentar `get_child_list`/`get_sibling_list`/`set_is_*`.
7. **(Exploratorio, no urgente)** Evaluar derivar `is_leaf()` de `child.is_empty()` en vez de mantenerlo como bandera separada, como refactorización propia con su propia batería de tests.

---

## 8. Plan de trabajo para esta rama

El objetivo de esta rama debería ser estabilizar `tpl_tree_node.H` sin cambiar
su modelo público ni intentar una refactorización profunda. La estrategia
recomendada es: primero blindar con tests los invariantes delicados, luego
corregir los bugs confirmados, después documentar el modelo interno y dejar las
limpiezas de bajo riesgo para el final.

### Etapa 0: Línea base y alcance

**Objetivo:** confirmar el estado actual antes de tocar código.

Tareas:

1. Ejecutar la suite relevante existente:
   - `cmake --build build --target tree-node`
   - `ctest --test-dir build -R "tree-node|prefix_tree|random_tree" --output-on-failure`
2. Confirmar si `Examples/deway.C` y `Examples/test_bin_to_tree.C` compilan en
   la configuración local.
3. Revisar si hay cambios no relacionados en `tpl_tree_node.H`,
   `random_tree.H`, `Tests/tree-node.cc` o ejemplos antes de editar.

Criterio de salida:

- Se conoce si la rama parte de tests verdes o de fallos preexistentes.
- No se mezclan correcciones de `Tree_Node` con cambios ajenos.

### Etapa 1: Tests de caracterización antes de corregir

**Objetivo:** capturar el comportamiento actual, incluyendo los fallos
confirmados, para que los arreglos posteriores sean pequeños y verificables.

Tareas:

1. Agregar tests directos en `Tests/tree-node.cc` para:
   - `get_parent()` en nodos de varios niveles.
   - `insert_leftmost_child()` cuando el padre ya tiene hijos.
   - `insert_left_sibling()` cuando el receptor es el hijo más a la izquierda.
2. Agregar un test de regresión para `bin_to_forest` que construya una espina
   derecha de `BinNode` y verifique que todos los árboles resultantes del bosque
   son raíces (`is_root() == true`).
3. Agregar un test negativo para `insert_right_sibling()` sobre una raíz, pero
   dejarlo preparado para esperar excepción solo después de aplicar la Etapa 2.

Criterio de salida:

- Los tests nuevos que describen comportamiento correcto de rutas ya sanas
  pasan.
- El test de `bin_to_forest` falla antes del arreglo, demostrando que captura
  el bug real.
- El test negativo de `insert_right_sibling()` queda alineado con la semántica
  que se va a introducir.

Nota práctica: si el proyecto evita dejar tests fallando en commits
intermedios, la Etapa 1 y la Etapa 2 pueden ir en el mismo cambio, pero el orden
mental debe seguir siendo test primero, fix después.

### Etapa 2: Correcciones críticas de comportamiento

**Objetivo:** corregir solo los bugs confirmados que pueden producir corrupción
de árbol o comportamiento indefinido silencioso.

Tareas:

1. Corregir `insert_sibling()` para que `bin_to_tree`/`bin_to_forest` preserven
   `is_root()` en cada árbol de un bosque.
   - Preferir reutilizar la lógica de `insert_tree_to_right()` si es viable.
   - No usar `insert_right_sibling()` para enlazar raíces de árboles distintos.
2. Hacer que `insert_right_sibling()` preserve compatibilidad con usos
   existentes:
   - si `this->is_root()`, tratar `p` como árbol de bosque y delegar en
     `insert_tree_to_right(p)`;
   - si `this` no es raíz, mantener la semántica actual de insertar un hermano
     dentro de la lista de hijos del mismo padre.
3. Ejecutar los tests de la Etapa 1 y verificar que ahora pasan.

Criterio de salida:

- `bin_to_forest` produce un bosque cuyas raíces reportan `is_root() == true`.
- `get_parent()` sobre cualquier raíz del bosque retorna `nullptr` o respeta la
  semántica documentada sin violar asserts internos.
- `insert_right_sibling()` usado sobre una raíz enlaza árboles de un bosque sin
  demover el árbol insertado a no-raíz.

Validación mínima:

- `cmake --build build --target tree-node`
- `ctest --test-dir build -R "tree-node" --output-on-failure`

### Etapa 3: Ajustar consumidores y ejemplos

**Objetivo:** eliminar workarounds o ejemplos incorrectos que ya no deben
existir después de corregir la librería.

Tareas:

1. Eliminar el workaround de `Examples/deway.C` que fuerza
   `set_is_root(true)` sobre los árboles del bosque, si el ejemplo sigue siendo
   correcto sin él.
2. Revisar `Examples/test_bin_to_tree.C` por workarounds equivalentes.
3. Corregir los ejemplos Doxygen de `random_tree.H` para usar
   `destroy_tree(tree)` en vez de `destroyRec(tree)` cuando el puntero sea
   `Tree_Node<T>*`.
4. Compilar los ejemplos afectados si tienen target disponible.

Criterio de salida:

- Los consumidores dejan de compensar manualmente el bug de `bin_to_forest`.
- La documentación de `random_tree.H` ya no sugiere código que no compila.

Validación recomendada:

- `cmake --build build --target deway`
- `cmake --build build --target test_bin_to_tree`
- `ctest --test-dir build -R "random_tree|tree-node" --output-on-failure`

Si alguno de esos targets no existe en la configuración local, registrar el
target ausente y validar al menos los tests que sí existan.

### Etapa 4: Documentar invariantes internos

**Objetivo:** reducir el riesgo de futuros cambios en `Tree_Node` documentando
el mecanismo que hoy solo se entiende por ingeniería inversa.

Tareas:

1. Agregar un comentario de diseño en `tpl_tree_node.H`, cerca de la definición
   de `Tree_Node`, explicando:
   - el rol dual de `child`;
   - cómo se representa la lista de hijos;
   - cómo el hijo más a la izquierda mantiene el enlace ascendente hacia el
     padre mediante la "espina compartida";
   - qué invariantes deben preservar `insert_leftmost_child`,
     `insert_rightmost_child`, `insert_left_sibling`, `insert_right_sibling`,
     `join` e `insert_tree_to_right`.
2. Agregar Doxygen breve para APIs públicas delicadas:
   - `get_child_list()`
   - `get_sibling_list()`
   - `set_is_root(bool)`
   - `set_is_leaf(bool)`
   - `set_is_leftmost(bool)`
   - `set_is_rightmost(bool)`
3. En los setters de banderas, documentar que son primitivas de bajo nivel y
   que usarlas directamente puede romper invariantes estructurales.

Criterio de salida:

- Un lector puede entender por qué `get_parent()` funciona sin puntero explícito
  al padre.
- Las APIs públicas peligrosas tienen advertencia suficiente para no parecer
  setters ordinarios.

### Etapa 5: Limpiezas de bajo riesgo

**Objetivo:** retirar ruido o código muerto sin mezclarlo con los fixes
críticos.

Tareas:

1. Quitar el `const_cast` innecesario en el constructor copia de
   `Children_Iterator`.
2. Arreglar o eliminar `IS_UNIQUE_SIBLING`.
   - Si se arregla, agregar una microprueba que cubra nodo sin hermanos.
   - Si se elimina, confirmar que `destroy_tree`/`destroy_forest` siguen
     pasando bajo ASAN/UBSAN.
3. Decidir qué hacer con `Tree_Node_Vtl` y `Children_Set`:
   - documentarlos con ejemplo/test si se conservan;
   - o dejarlos explícitamente como deuda si eliminarlos implica riesgo de API.

Criterio de salida:

- Las limpiezas no cambian semántica pública observable.
- Los tests de `tree-node`, `prefix_tree` y consumidores relevantes siguen
  verdes.

### Etapa 6: Validación final de rama

**Objetivo:** cerrar la rama con confianza suficiente para PR/revisión.

Validación recomendada:

1. Build completo:
   - `cmake --build build`
2. Tests focalizados:
   - `ctest --test-dir build -R "tree-node|prefix_tree|random_tree" --output-on-failure`
3. Tests completos:
   - `ctest --test-dir build --output-on-failure`
4. Si se tocaron rutas de destrucción o enlaces internos, ejecutar al menos los
   tests focalizados con ASAN/UBSAN en una build separada.

Criterio de salida:

- Tests focalizados verdes.
- CTest completo verde, o cualquier fallo residual documentado como
  preexistente y no relacionado.
- El reporte final de la rama explica:
  - qué bug se corrigió;
  - qué invariantes quedaron documentados;
  - qué tests nuevos protegen el cambio;
  - qué deuda queda fuera de alcance.

Resultado de ejecución en esta rama:

- `cmake --build build`: verde; Ninja no encontró trabajo pendiente.
- `ctest --test-dir build -R 'Tree_Node\.|Simple_Tree\.|Three_Trees\.|PrefixTreeTest\.|PrefixTreeDestroyTest\.|RandomTreeTest\.|ForestTest\.' --output-on-failure`:
  82/82 tests verdes.
- `ctest --test-dir build --output-on-failure`: verde; 12097 tests ejecutados,
  0 fallos. Los tests no ejecutados fueron skips/disabled esperados de
  performance, guards ambientales o tests deshabilitados explícitamente.
- `ASAN_OPTIONS=detect_leaks=0 ./build-tree-node-asan/Tests/tree-node`:
  19/19 tests verdes con AddressSanitizer/UBSan. LeakSanitizer se dejó
  desactivado porque el entorno de ejecución reporta que no funciona bajo
  `ptrace`, lo cual impide obtener un código de salida útil aunque los tests
  pasen.
- `git diff --check -- tpl_tree_node.H Tests/tree-node.cc`: sin problemas de
  whitespace.

Con esto, la etapa 6 queda cerrada: no hay fallos residuales conocidos
atribuibles a los cambios de `tpl_tree_node.H`.

### Adenda: hallazgo posterior en `destroy_tree()` (revisión de bot en PR #75)

Tras el merge de las correcciones anteriores, dos comentarios de
`chatgpt-codex-connector` sobre `destroy_tree()` (líneas 1155-1157 del diff)
señalaron que, al desenlazar `root` de su lista de hermanos, la bandera
`is_rightmost()`/`is_leftmost()` del hermano que pasa a ocupar el extremo no
se actualizaba — dejando `get_right_sibling()`/`get_left_sibling()` sin su
cortocircuito a `nullptr` y provocando que envuelvan la lista circular
restante. Verificado empíricamente con ASan: destruir el hijo más a la
derecha de tres (`root -> a, b, c`) dejaba a `b` con `is_rightmost()==false`,
de modo que `b->get_right_sibling()` devolvía `a` en vez de `nullptr`, y
`for_each_child()` entraba en bucle infinito real (`a -> b -> a -> b -> ...`).

**Corrección aplicada**: al desenlazar `root`, si era `is_rightmost()`/
`is_leftmost()`, se promueve la bandera correspondiente en el nuevo hermano
extremo (`get_left_sibling()`/`get_right_sibling()` de `root`, capturados
antes de `SIBLING_LIST(root)->del()`).

**Hallazgo adicional, más profundo, encontrado durante la verificación**: el
caso `is_leftmost()` no es solo una bandera — el campo `child` de un hijo
más a la izquierda está enlazado (vía el truco de "espina compartida",
sección de esta misma nota más arriba) al campo `child` de su padre. Al
promover la bandera sin también transferir esa espina, `get_parent()` sobre
el hermano promovido viola su propia aserción (`assert(not
CHILD_LIST(p)->is_empty())`), o —si `root` no tenía hermanos vivos aún sin
banderas correctas— entra en el mismo bucle infinito por el lado de
`is_leftmost`.

Se añadió una transferencia de la espina vía `Dlink::swap()`, pero
**solo quedó habilitada cuando el hermano promovido es una hoja**
(`new_first->is_leaf()`): en ese caso `swap()` simplemente reubica el par
`root<->padre` para que sea `promovido<->padre`, lo cual es correcto y está
verificado con ASan (incluyendo el caso de raíces de bosque, donde no hay
espina que transferir en absoluto, gracias al guard `not root->is_root()`).

**Se descubrió, y se dejó explícitamente fuera de alcance, un bug preexistente
más profundo y no relacionado con estos 2 hallazgos**: si el hermano
promovido *ya tiene hijos propios* (su propio `child` ya es la cabeza de su
propia espina), `Dlink::swap()` no sirve — mezclaría la espina del padre
directamente con el extremo *opuesto* de la espina del promovido, dejando a
sus hijos reales huérfanos. Pero esto resultó ser síntoma de un problema
**mucho más general y ya presente antes de cualquiera de estos cambios**:
`destroy_tree()` no soporta correctamente borrar un nodo intermedio que
tenga hijos propios mientras su padre sobrevive — reproducido con ASan
incluso revirtiendo *todos* los cambios de esta rama (`git stash`), con un
árbol trivial de 3 niveles (`G -> P -> D`, `P` único hijo de `G`, `D` único
hijo de `P`) y `destroy_tree(P)`: produce un *stack-overflow* por recursión
infinita real. Arreglarlo requeriría una cirugía de `cut_list`/`insert`
sobre la espina compartida análoga a la rama compleja de
`insert_leftmost_child()`, de mayor alcance que esta corrección puntual.
Queda documentado aquí para una futura rama dedicada.

### Fuera de alcance para esta rama

No conviene incluir en esta misma rama:

- derivar `is_leaf()` de `child.is_empty()`;
- rediseñar la representación interna de `Tree_Node`;
- convertir todos los `assert()` de precondiciones en `ah_*_error_if`;
- eliminar APIs públicas potencialmente usadas externamente sin una revisión de
  compatibilidad más amplia.

Esos puntos pueden ser valiosos, pero son refactorizaciones de mayor radio de
impacto. Esta rama debería enfocarse en corregir bugs confirmados, blindar
tests y documentar invariantes.

---

## Apéndice: reproducciones usadas para verificar los hallazgos

Todas las pruebas se compilaron con `g++ -std=c++20 -I. -fsanitize=address,undefined` y se ejecutaron con `ASAN_OPTIONS=detect_leaks=0` (las fugas de memoria de las propias pruebas, al no llamar `destroy_tree`, son intencionales y no relevantes para lo que se estaba verificando).

- **`probe1.cc`** — `insert_leftmost_child` sobre un nodo con hijos existentes: confirma que la rama compleja funciona correctamente (no es un bug, sí es un hueco de test — sección 3).
- **`probe2.cc`** — `insert_left_sibling` sobre el hermano actualmente más a la izquierda: confirma que la rama compleja funciona correctamente (no es un bug, sí es un hueco de test — sección 3).
- **`probe3.cc`** — `insert_right_sibling` llamado sobre una raíz real: reproduce el crash de aserción del hallazgo 1.2.
- **`probe4.cc`** — evalúa `IS_UNIQUE_SIBLING` sobre un nodo sin hermanos: confirma que la macro nunca es verdadera (hallazgo 1.4).
- **`probe5.cc` / `probe5b.cc`** — `bin_to_forest` sobre un `BinNode` con espina derecha de 3 nodos: confirma `is_root() == false` en el 2º y 3er árbol del bosque, y que `get_parent()` sobre ellos viola su propia aserción interna (hallazgo 1.1, el más importante de este informe).
