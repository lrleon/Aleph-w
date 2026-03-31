# Auditoría Exhaustiva de la Rama `adversarial`

**Fecha**: 2026-03-20
**Rama**: adversarial
**Cambios**: ~13,300 líneas en 40 archivos (9 headers, 11 ejemplos, 5 docs, 5 tests)
**Auditoría realizada por**: Claude Code con 4 agentes especializados

## Resumen Ejecutivo

La rama introduce un framework completo de búsqueda en espacios de estados con arquitectura sólida y código de buena calidad general. Se encontraron **8 defectos corregidos** durante la auditoría, **3 defectos pendientes** (baja severidad), y **falencias significativas en cobertura de tests** que deben abordarse antes del merge.

### Estadísticas

| Categoría | Cantidad |
|-----------|----------|
| Defectos corregidos | 8 |
| Defectos pendientes | 3 |
| Gaps en cobertura de tests | 12+ |
| Ejemplos incompletos | 7 |
| APIs no documentadas | 8 |
| Temas documentales faltantes | 5 |

---

## 1. DEFECTOS DE CÓDIGO (Bugs)

### 1.1 Defectos Corregidos en Esta Sesión

#### IDA* Loop Infinito (`State_Search_IDA_Star.H:386-412`)
**Severidad**: CRÍTICA
**Descripción**: Cuando `stop_at_first_solution=false` y se encuentra una solución, `next_bound == threshold` (ambos iguales), causando que el siguiente loop use el mismo threshold → loop infinito encontrando la misma solución repetidamente.
**Causa raíz**: La lógica del DFS retorna `{true, threshold}` cuando encuentra un goal, sin incrementar el threshold.
**Solución**: Siempre hacer `break` cuando `found == true`, dejando que el IDA* termine después de encontrar soluciones en la profundidad actual.
**Estado**: ✅ CORREGIDO

#### IDA* Comentario Engañoso (`State_Search_IDA_Star.H:339-345`)
**Severidad**: MEDIA
**Descripción**: Documentación decía que IDA* busca "soluciones más baratas" en iteraciones posteriores, pero es imposible: con costos no-negativos, la primera solución encontrada es óptima. Incrementar el threshold solo encuentra soluciones más caras.
**Solución**: Actualizar doc para reflejar semántica correcta: IDA* garantiza optimalidad en la primera búsqueda exitosa.
**Estado**: ✅ CORREGIDO

#### IDA* Total Cost Inconsistencia (`State_Search_IDA_Star.H:155-157`)
**Severidad**: ALTA
**Descripción**: `result.total_cost = g` se asignaba incondicionalmente, incluso cuando `consider()` retornaba `false` (el incumbent no fue reemplazado). Inconsistencia entre `best_solution` y `total_cost`.
**Solución**: Asignar `total_cost` solo cuando `consider()` retorna `true`.
**Estado**: ✅ CORREGIDO

#### Backtracking Doc: Campo "time" No Existe (`Backtracking.H:142`)
**Severidad**: BAJA
**Descripción**: Doc del parámetro `limits` decía "depth, expansions, solutions, time, etc." pero `SearchLimits` no tiene campo de tiempo.
**Solución**: Actualizar a "such as depth, expansions and solutions".
**Estado**: ✅ CORREGIDO

#### Backtracking: Falta Exception Safety (`Backtracking.H:489-491, 566-568`)
**Severidad**: MEDIA
**Descripción**: Si `dfs()` o `dfs_visited()` lanzan excepción, `domain_.undo(state, move)` y `path.remove_last()` se saltan, dejando el estado corrupto.
**Solución**: Envolver en try/catch: si excepción en `dfs()`, hacer cleanup antes de re-lanzar.
**Estado**: ✅ CORREGIDO

#### Backtracking: Visited Stats Incorrectas (`Backtracking.H:515`)
**Severidad**: BAJA
**Descripción**: Cuando un estado ya visitado era encontrado nuevamente, se incrementaba `pruned_by_domain` en lugar de `pruned_by_visited`, confundiendo duplicate suppression con domain-side pruning.
**Solución**: Introducir campo `pruned_by_visited` en `SearchStats` e incrementar el campo correcto.
**Estado**: ✅ CORREGIDO

#### Backtracking: Depth-Blind Pruning en Visited (`Backtracking.H:522-564`)
**Severidad**: ALTA
**Descripción**: Visited-set plano (`VisitedStateSet`) impedía re-exploración a profundidad menor. Si estado S se explora a profundidad D1=5 (presupuesto bajo), el mismo S encontrado luego a D2=2 (más presupuesto) se poda incorrectamente, perdiendo soluciones válidas.
**Solución**: Cambiar a `VisitedBoundMap<Key, size_t>` que almacena la profundidad más somera vista. Prune solo si `stored_depth <= current_depth`.
**Estado**: ✅ CORREGIDO

#### CMakeLists.txt: Escaping de Semicolons (`Examples/CMakeLists.txt:65,75,85`)
**Severidad**: MEDIA
**Descripción**: `-DEXPECT_SUBSTRINGS=a;b;c` en CMake es parseado como lista, dividiéndose en múltiples argumentos.
**Solución**: Escapar como `-DEXPECT_SUBSTRINGS=a\;b\;c`.
**Estado**: ✅ CORREGIDO

#### Hash Backend Default Cambio (`tpl_hash.H:60,91`)
**Severidad**: MEDIA
**Descripción**: Default de `HashSet`/`HashMap` cambió de `ODhashTable`/`MapODhash` a `OLhashTable`/`MapOLhash` sin documentación explícita del cambio breaking.
**Solución**: Agregar test con `static_assert` que fija el default y documentar como cambio intencional.
**Estado**: ✅ CORREGIDO + TEST AGREGADO

### 1.2 Defectos Pendientes

| Archivo | Línea | Descripción | Severidad |
|---------|-------|-------------|-----------|
| `State_Search_IDA_Star.H` | 194 | `if (const bool can_expand = depth + 1 <= ...; not can_expand)` — doble negación difícil de leer. Mejor: `if (depth + 1 > ...) return out;` | Baja |
| `Negamax.H` | 982-984 | Detección de underflow usa `std::numeric_limits<Score>::lowest()` — frágil si dominio usa ese valor legítimamente | Baja |
| `Alpha_Beta.H` | 432 | `not(alpha < beta)` es doble negación. Mejor: `alpha >= beta` | Baja |

---

## 2. DEFECTOS DE DISEÑO DE API

### 2.1 Exception Safety Incompleta en Otros Motores

Los try/catch agregados en `Backtracking.H` (líneas 494-503, 584-593) solo cubren ese motor. El mismo patrón `apply → recurse → undo` existe sin protección en:

- **`State_Search_IDA_Star.H:206-212`** — `dfs()` del IDA*
- **`Branch_And_Bound.H`** — todas las variantes (`search_node`, `search_node_visited`)

**Recomendación**: Agregar try/catch idéntico en ambos ubicaciones para garantizar cleanup si `dfs()` o métodos del dominio lanzan.

### 2.2 Backtracking.H Incluye Transposition_Table.H Innecesariamente

`Backtracking.H` incluye `Transposition_Table.H` solo para acceder al concepto `VisitedBoundMap` (línea 509). Esto introduce una dependencia transitiva no deseada: Backtracking (búsqueda en estado) depende de Transposition_Table (característica de búsqueda adversarial).

**Solución recomendada**: Mover `VisitedStateSet`, `VisitedBoundMap`, y `SearchStateKeyProvider` de `Transposition_Table.H` a `state_search_common.H` donde pertenecen conceptualmente.

### 2.3 Alpha_Beta `search_with_window()` Sin Test

El API público `Alpha_Beta::search_with_window()` existe pero no tiene test directo en la suite.

---

## 3. FALENCIAS EN LOS TESTS

### 3.1 Cobertura Cero

| Componente | Descripción |
|------------|-------------|
| **`search_move_ordering.H`** | **SIN TESTS UNITARIOS**. `Killer_Move_Table<Move>` (líneas 138-191) y `History_Heuristic_Table<Key>` (líneas 225-270) solo se ejercitan indirectamente vía integración con Alpha-Beta. Nunca se testean en aislamiento. |
| **Alpha-Beta `search_with_window()`** | API pública sin ningún test directo |
| **Exception safety** | Ningún test verifica que `undo()` se llame si `apply()` o `dfs()` lanzan excepción |
| **Edge cases** | `max_depth=0`, `max_expansions=1`, `cost=0` en IDA*, estado raíz como goal — ninguno testeado |
| **Hooks opcionales** | Sin tests de dominios **sin** `is_terminal()`, **sin** `should_prune()`, **sin** `state_key()` (detectados vía `if constexpr` pero nunca ejercitados por ausencia) |

### 3.2 Calidad de Aserciones

**Principal Variation Nunca se Replaya** (`adversarial_search_test.cc`):
- Tests obtienen `result.principal_variation` pero **nunca la ejecutan** desde cero
- Falta: construir estado desde movimientos de PV y verificar que el valor resultante = valor reportado
- Bug que corrompa la PV pasaría los tests

**Stats No se Validan Cruzadamente**:
- Nunca se verifica `expanded_states <= visited_states`
- Nunca se verifica `solutions_found + terminal_states + pruned_by_domain + pruned_by_visited + ... == visited_states`
- Inconsistencias silenciosas podrían pasar

**IDA\* No Valida Historia de Iteraciones**:
- `result.iterations` no está vacío ✓
- Pero no se comprueba:
  - Monotonicidad de thresholds (cada iteración debe requerir threshold más alto)
  - Corrección de `visited_states` contados por iteración
  - Exactitud del cálculo de `next_threshold`

### 3.3 Dominios de Test Limitados

- **Árboles de decisión**: Siempre binarios, profundidad 2 → no ejercitan branching asimétrico
- **N-Queens**: Solo n=4 → falta n=1 (trivial), n=8 (realista)
- **Ciclos**: Ningún test de estado S reachable vía múltiples caminos de diferente profundidad
- **TicTacToe**: Solo desde tablero vacío → nunca desde posiciones de endgame donde la evaluación estática es crítica
- **Knapsack**: Problemas de 3-4 items → nunca problemas de 10+ items donde movimiento ordering importa más

### 3.4 Concepto Compliance Gaps

| Hook | Estado en tests |
|------|-----------------|
| `TerminalPredicate` (opcional) | Testeado solo **con** el hook; nunca sin él |
| `DomainPruner` (opcional) | Testeado solo **con** el hook; nunca sin él |
| `MoveKeyProvider` (opcional en adversarial) | Testeado solo en Connect-3 con el hook presente |

**Problema**: Si el motor incorrectamente asume que `should_prune()` existe cuando es `if constexpr`-ional, nunca lo detectaría porque todos los tests lo proveen.

---

## 4. EJEMPLOS

### 4.1 Inconsistencias de Estilo

| Archivo | Problema |
|---------|----------|
| `backtracking_subset_sum_example.cc` | Usa `Depth_First_Backtracking<>` directamente en vez de alias `Search::Backtracking` |
| `branch_and_bound_assignment_example.cc` | Usa `Branch_And_Bound<>` directamente vs alias |
| Todos los ejemplos | Namespace inconsistente: algunos `using namespace Aleph;` vs imports selectivos |
| `backtracking_subset_sum_example.cc` | Falta header `@file` de Doxygen |
| `branch_and_bound_assignment_example.cc` | Falta header `@file` de Doxygen |

### 4.2 Ejemplos Faltantes

| Caso de Uso | Status |
|-------------|--------|
| Transposition tables en backtracking (DFS no-adversarial) | **SIN EJEMPLO** |
| Custom `SearchSolutionVisitor` (en vez de `SearchSolutionCollector`) | **SIN EJEMPLO** |
| Iterative deepening en backtracking / B&B | **SIN EJEMPLO** |
| Problema de escala realista (8-queens, problema de 20+ items) | **SIN EJEMPLO** |
| Uso explícito de `VisitedBoundMap` (depth-aware visited) | **SIN EJEMPLO** |
| Move ordering effects (comparar con/sin killer+history) | **SIN EJEMPLO** |

### 4.3 Pedagogía

- **`negamax_tictactoe_example.cc` (308 líneas)**: Demasiado complejo como "getting started". Combina:
  - Negamax básico
  - Iterative deepening
  - Aspiration windows
  - Move ordering

  Sería mejor: ejemplo simple de negamax puro (50 líneas) + ejemplo separado de variantes avanzadas.

- **Sin guía de selección de algoritmo**: No hay documento que diga "usa DFS para esto, B&B para aquello, IDA* para lo otro".

---

## 5. DOCUMENTACIÓN

### 5.1 Precisión de Snippets

✅ Los snippets de código en `.md` **son generalmente correctos** y reflejan la API real.

⚠️ Excepciones menores:
- `transposition_table_framework.md`: Menciona `dft_hash_fct()` y `hash_combine()` sin mostrar ejemplo de uso
- `adversarial_search_framework.md`: No nombra `AdversarialSearchStats` al describir stats del resultado

### 5.2 APIs No Documentadas

| API | Archivo | Líneas Aprox. |
|-----|---------|-------------|
| `SearchStatus` enum (listado completo de valores) | `state_search_framework.md` | 40-80 |
| `SearchResult::found_solution()`, `::exhausted()` | idem | - |
| `SearchSolutionCollector::solutions()`, `::size()` | idem | - |
| `BranchAndBoundStats::pruned_by_bound`, `::incumbent_updates` | `branch_and_bound_framework.md` | 50-100 |
| `AdversarialSearchResult::principal_variation` (tipo) | `adversarial_search_framework.md` | 150-200 |
| `sort_ranked_moves()` (firma completa) | `move_ordering_framework.md` | 40-60 |
| `Killer_Move_Table<Move>` (constructor, métodos públicos) | idem | - |
| `History_Heuristic_Table<Key>` (idem) | idem | - |

### 5.3 Temas Documentales Faltantes

| Tema | Archivo | Impacto |
|------|---------|--------|
| Cuándo usar DFS vs B&B vs IDA* | Intro + selección | Alto — usuarios no saben qué elegir |
| Depth-First vs Best-First en B&B | `branch_and_bound_framework.md` | Medio — trade-offs no documentados |
| Aspiration windows (re-búsqueda en fail-low/high) | `adversarial_search_framework.md:214-226` | Medio — comportamiento no detallado |
| Presupuesto de memoria para TT | `transposition_table_framework.md` | Bajo-Medio — sizing guidance falta |
| Cuándo limpiar/resetear TT entre búsquedas | idem | Bajo |

---

## 6. CONCERNS TRANSVERSALES

### 6.1 Consistencia de Backends Hash ✅

**Estado: EXCELENTE**

Todos los aliases de storage `SearchStorageSet`, `SearchStorageMap`, `DynHashSet`, `DynHashMap`, `HashSet`, `HashMap` defaultean a **OL (linear probing)** de forma consistente.

El test `hash_backends_test.cc` (372 líneas) contiene `static_assert` en compile-time (líneas 62-78) que fija estos defaults y previene cambios silenciosos. El test también verifica equivalencia funcional entre OL y OD backends.

### 6.2 Grafo de Dependencias ✅

**Estado: LIMPIO — NO hay ciclos**

```
state_search_common.H (foundation)
    ↓
    ├── Transposition_Table.H
    │   ├── Branch_And_Bound.H
    │   └── Negamax.H  →  Alpha_Beta.H
    └── Backtracking.H  →  State_Search_IDA_Star.H

search_move_ordering.H (utilities, sin depender de motores)
```

Validación:
- ✅ No hay `#include` circular
- ✅ Todas las dependencias son explícitas (no transitivas no-documentadas)
- ✅ Orden topológico válido para compilación

### 6.3 Estilo Preprocessor ⚠️

Todos los headers nuevos usan `#ifndef`/`#define` **sin espacio** después de `#`.

CLAUDE.md convención: `# ifndef` / `# define` (con espacio)

**Archivos afectados**:
- state_search_common.H:49-50
- Backtracking.H:47-48
- Branch_And_Bound.H:47-48
- State_Search_IDA_Star.H:49-50
- Negamax.H:50-51
- Alpha_Beta.H:42-43
- search_move_ordering.H:49-50
- Transposition_Table.H:48-49
- State_Search.H:46-47

**Severidad**: Baja (estilo, no funcional)
**Recomendación**: Normalizar en pass de cleanup post-merge

### 6.4 Backward Compatibility ✅

El único cambio **breaking real**:
- Default de `HashSet`/`HashMap` de `ODhashTable`/`MapODhash` → `OLhashTable`/`MapOLhash`

**Mitigación**:
- ✅ Documentado en comentarios de `tpl_hash.H:135-136, 144-145`
- ✅ Test `static_assert` fija el default
- ✅ Funcionalidad (OL y OD) es idéntica; solo performance/iteración-order difiere

**Todos los demás cambios son backward-compatible**:
- Nuevo campo `SearchStats::pruned_by_visited` (default 0) ✅
- Cambio de `VisitedStateSet` → `VisitedBoundMap` está encapsulado en API search() ✅
- New files no modifican existentes ✅

---

## 7. RECOMENDACIONES PRIORIZADAS

### 🔴 CRÍTICAS (Antes de Merge)

1. **Agregar Exception Safety a State_Search_IDA_Star.H y Branch_And_Bound.H**
   - Aplicar el mismo patrón try/catch de Backtracking.H (lines 494-503)
   - Archivos: `State_Search_IDA_Star.H:206-212`, `Branch_And_Bound.H` (todas las variantes de DFS)
   - **Estimado**: 1 hora

2. **Tests Unitarios para Move Ordering**
   - `Killer_Move_Table<Move>`: insert/query/ensure_depth
   - `History_Heuristic_Table<Key>`: increment/query/reset
   - `sort_ranked_moves()`: verificar orden correcto con distintos comparadores
   - **Estimado**: 3-4 horas

3. **Tests para Alpha-Beta `search_with_window()`**
   - Ventana aspiration narrow + fail-low/high
   - Ventana muy ancha (equiv. a búsqueda sin ventana)
   - **Estimado**: 2 horas

### 🟠 IMPORTANTES (1-2 semanas post-merge)

4. **Reorganizar Conceptos**
   - Mover `VisitedStateSet`, `VisitedBoundMap`, `SearchStateKeyProvider` de `Transposition_Table.H` a `state_search_common.H`
   - **Impacto**: Reduce acoplamiento innecesario

5. **Ampliar Cobertura de Edge Cases**
   - Tests con `max_depth=0`, `max_expansions=1`
   - Estado raíz como goal
   - Ciclos en espacio de estados (DFS con visited-set)
   - **Estimado**: 3-4 horas

6. **Principal Variation Replay Verification**
   - Para cada test adversarial: ejecutar PV desde cero, verificar valor
   - **Estimado**: 2 horas

7. **Documentación de APIs Faltantes**
   - Completar todas las APIs listadas en §5.2
   - **Estimado**: 4-5 horas

### 🟡 DESEABLES (Mejora Continua)

8. **Separar Ejemplos Complejos**
   - `negamax_tictactoe_example.cc` → ejemplo simple (50 líneas) + avanzado
   - **Estimado**: 2 horas

9. **Unificar Estilo de Ejemplos**
   - Consistent namespace imports: todos con `namespace Search = ...;`
   - Agregar `@file` headers faltantes
   - **Estimado**: 1 hora

10. **Normalizar Estilo Preprocessor**
    - Cambiar `#ifndef` → `# ifndef`, `#define` → `# define` en 9 headers
    - **Estimado**: 0.5 horas

11. **Guía de Selección de Algoritmo**
    - Documento nuevo: "Cuándo usar DFS vs B&B vs IDA* vs Negamax vs Alpha-Beta"
    - **Estimado**: 3-4 horas

---

## 8. MATRIZ DE RIESGO

| Problema | Probabilidad | Impacto | Riesgo | Esfuerzo |
|----------|-------------|--------|--------|----------|
| IDA* loop infinito | Detectada | Crítico | Mitigado ✅ | 1h ✅ |
| Exception safety gaps | Baja | Alto | Alto | 1-2h |
| Move ordering 0% test | Alta | Medio | Medio | 3-4h |
| PV no verificada | Media | Medio | Medio | 2h |
| Stats correlación | Baja | Bajo | Bajo | 2h |
| Edge cases | Alta | Bajo | Bajo | 3h |
| Docs incompletas | Alta | Bajo | Bajo | 4h |

---

## 9. CONCLUSIONES

### Fortalezas

✅ Arquitectura bien diseñada (DAG limpio de dependencias)
✅ Código legible y bien comentado (en general)
✅ Concepts bien constrained (no over/under)
✅ Hash backend consistency excelente
✅ Test framework sólido (GTest + CMake GLOB auto-discovery)
✅ Documentación precisa en snippets de código

### Debilidades

❌ Cobertura de tests tiene gaps críticos (0% en move ordering)
❌ Exception safety incompleta en 2 motores
❌ Principal variation nunca se verifica correctamente
❌ Edge cases no explorados
❌ APIs importantes no documentadas
❌ Ejemplos con inconsistencias de estilo y pedagogía mediocre

### Recomendación Final

**✅ MERGE CON CONDICIONES**:

- Agregar exception safety a IDA* y Branch_And_Bound (bloqueante)
- Agregar tests de move ordering (bloqueante)
- Agregar test de `search_with_window()` (bloqueante)
- Post-merge: reorganizar conceptos, expandir documentación, mejorar ejemplos

**Timeline estimado**:
- Bloqueantes: 6-7 horas
- Post-merge importante: 15-20 horas (1-2 sprints)
- Post-merge deseable: 10-15 horas (mejora continua)

La rama está **production-ready** con las correcciones bloqueantes, pero se recomienda un follow-up cercano para completar los gaps identificados.

---

## 10. REFERENCIAS

### Archivos Auditados

**Headers** (9):
- `/home/lrleon/Aleph-w/state_search_common.H`
- `/home/lrleon/Aleph-w/Backtracking.H`
- `/home/lrleon/Aleph-w/Branch_And_Bound.H`
- `/home/lrleon/Aleph-w/State_Search_IDA_Star.H`
- `/home/lrleon/Aleph-w/Negamax.H`
- `/home/lrleon/Aleph-w/Alpha_Beta.H`
- `/home/lrleon/Aleph-w/search_move_ordering.H`
- `/home/lrleon/Aleph-w/Transposition_Table.H`
- `/home/lrleon/Aleph-w/State_Search.H`

**Tests** (5):
- `/home/lrleon/Aleph-w/Tests/state_search_framework_test.cc`
- `/home/lrleon/Aleph-w/Tests/branch_and_bound_test.cc`
- `/home/lrleon/Aleph-w/Tests/adversarial_search_test.cc`
- `/home/lrleon/Aleph-w/Tests/transposition_table_test.cc`
- `/home/lrleon/Aleph-w/Tests/hash_backends_test.cc`

**Ejemplos** (11):
- `/home/lrleon/Aleph-w/Examples/backtracking_nqueens_example.cc`
- `/home/lrleon/Aleph-w/Examples/backtracking_subset_sum_example.cc`
- `/home/lrleon/Aleph-w/Examples/branch_and_bound_knapsack_example.cc`
- `/home/lrleon/Aleph-w/Examples/branch_and_bound_assignment_example.cc`
- `/home/lrleon/Aleph-w/Examples/branch_and_bound_artificial_example.cc`
- `/home/lrleon/Aleph-w/Examples/negamax_tictactoe_example.cc`
- `/home/lrleon/Aleph-w/Examples/alpha_beta_connect3_example.cc`
- `/home/lrleon/Aleph-w/Examples/adversarial_artificial_example.cc`
- `/home/lrleon/Aleph-w/Examples/ida_star_grid_example.cc`
- `/home/lrleon/Aleph-w/Examples/state_search_framework_example.cc`
- `/home/lrleon/Aleph-w/Examples/state_search_benchmark.cc`

**Documentación** (5):
- `/home/lrleon/Aleph-w/docs/state_search_framework.md`
- `/home/lrleon/Aleph-w/docs/branch_and_bound_framework.md`
- `/home/lrleon/Aleph-w/docs/adversarial_search_framework.md`
- `/home/lrleon/Aleph-w/docs/move_ordering_framework.md`
- `/home/lrleon/Aleph-w/docs/transposition_table_framework.md`

**Configuración**:
- `/home/lrleon/Aleph-w/Tests/CMakeLists.txt`
- `/home/lrleon/Aleph-w/Examples/CMakeLists.txt`

---

*Auditoría realizada el 2026-03-20 por Claude Code con metodología multi-agente*