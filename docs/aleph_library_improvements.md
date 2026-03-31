# Aleph-w — Auditoría general y plan de mejoras

> Revisión del 19 de marzo de 2026.
> Alcance: toda la librería (~330 headers, ~280 tests, ~280 ejemplos, build system, documentación).

---

## Tabla de contenidos

1. [Resumen ejecutivo](#1-resumen-ejecutivo)
2. [Lo que está bien](#2-lo-que-está-bien)
3. [Consistencia de API entre contenedores](#3-consistencia-de-api-entre-contenedores)
4. [Calidad de código y modernidad C++](#4-calidad-de-código-y-modernidad-c)
5. [Estructuras de datos faltantes](#5-estructuras-de-datos-faltantes)
6. [Algoritmos faltantes](#6-algoritmos-faltantes)
7. [Cobertura de tests](#7-cobertura-de-tests)
8. [Build system e infraestructura](#8-build-system-e-infraestructura)
9. [Documentación](#9-documentación)
10. [Módulos utilitarios faltantes](#10-módulos-utilitarios-faltantes)
11. [Plan de mejoras](#11-plan-de-mejoras)

---

## 1. Resumen ejecutivo

Aleph-w es una librería madura y amplia: ~150+ algoritmos, ~90+ estructuras de datos,
cobertura de grafos/geometría/strings/numéricos/DP/búsqueda, con soporte real de C++20
(concepts, constexpr, requires). El README es excelente (9/10), hay CI con 5 workflows,
Doxygen publicado, y un benchmark suite.

Las áreas principales de mejora son:

| Área | Impacto |
|------|---------|
| Inconsistencia de API entre contenedores | Alto — afecta la curva de aprendizaje |
| C-style casts masivos en código legacy | Medio — riesgo de bugs, dificulta auditoría |
| Cobertura de tests desigual (~10% en DP/combinatoria) | Alto — huecos de calidad |
| Build system incompleto (no hay find_package, presets faltantes) | Medio — limita adopción externa |
| Faltan estructuras para competir con Boost/LEDA | Bajo-Medio — oportunidad de diferenciación |

---

## 2. Lo que está bien

**Diseño y arquitectura**
- Namespace `Aleph` consistente en toda la librería.
- Patrón `Gen_<Name>` + typedef para parametrización de árboles/containers.
- CRTP mixins (`FunctionalMethods`, `LocateFunctions`, `SpecialCtors`, etc.) inyectan
  funcionalidad de forma limpia.
- Separación clara entre nodos internos (`Slinknc`, `Dlink`, `BinNode`) y wrappers
  de usuario (`DynList`, `DynSetTree`).

**Modernidad C++**
- Uso serio de concepts (~100 definiciones en 24 headers) y `requires` (~400 usos).
- `constexpr` extendido (~800 usos en 96+ headers).
- `[[nodiscard]]` en ~1700 sitios.
- Move semantics en todos los contenedores principales.
- `nullptr` universal (0 usos de `NULL`).
- Header guards tradicionales uniformes (0 `#pragma once`).

**Errores**
- Sistema centralizado `ah_*_error` (>1100 usos) con macros stream-able.
- Solo ~70 `throw` directos, concentrados en código de terceros y utilidades de
  concurrencia.

**Integración**
- Arena allocators reales (`ah-arena.H`, `ah-map-arena.H`, `tpl_nodePool.H`).
- Adaptadores de iteradores: zip, enumerate, transform, filter (`ah-zip.H`).
- String utils completas: split, join, trim, format (`ah-string-utils.H`).
- Estadísticos: mean, variance, median, percentiles, skewness, kurtosis (`stat_utils.H`).
- Filtros probabilísticos: Bloom, Cuckoo, Quotient, Count-Min, HyperLogLog, MinHash, SimHash.

**Documentación**
- README.md de ~4500 líneas: overview, instalación (3 métodos), quick-start, tabla de
  APIs, comparación STL/Boost.
- INSTALL.md detallado por distribución.
- Doxygen generado y publicado en GitHub Pages.
- CI con 5 workflows (Ubuntu 22.04/24.04 × GCC/Clang × Debug/Release).

---

## 3. Consistencia de API entre contenedores

### 3.1 El problema de `empty()` vs `clear()`

| Contenedor | Vaciar | Predicado "¿vacío?" |
|---|---|---|
| Array | `empty()` (verbo) | `is_empty()` |
| DynList | `empty()` (verbo) | `is_empty()` |
| DynSetTree | `empty()` (verbo) | deducido de `size() == 0` |
| DynArray | `empty()` (verbo) | `is_empty()` |
| DynHashSet | `empty()` (verbo) | sin `is_empty()` explícito |
| DynHashMap | `empty()` (verbo) | sin `is_empty()` explícito |

- En STL, `empty()` es predicado y `clear()` es verbo. En Aleph es al revés.
- Solo `DynListStack` tiene `clear()` como alias.
- **Recomendación**: añadir `clear()` como alias universal y `is_empty()` donde falte.

### 3.2 `contains()` ausente en secuenciales

| Contenedor | `contains()` | Alternativa actual |
|---|---|---|
| DynSetTree | Sí | — |
| DynHashSet | Sí | — |
| DynHashMap | Sí | — |
| **Array** | **No** | `find_ptr()` O(n) |
| **DynList** | **No** | `search()` O(n) |
| **DynArray** | **No** | `exist(i)` (por índice, no valor) |

- **Recomendación**: añadir `contains(const T&)` a contenedores secuenciales, posiblemente
  vía un mixin CRTP.

### 3.3 DynList carece de `operator==`

`DynList` no hereda `EqualSequenceMethod` ni define `operator==`. `Array` sí lo tiene.
Para una librería general, la comparación de igualdad entre listas es fundamental.

### 3.4 Retorno inconsistente de `insert()`

| Contenedor | `insert()` retorna |
|---|---|
| Array, DynList, DynArray | `T&` |
| DynSetTree, OLhashTable | `Key*` (nullptr si ya existe) |

Semánticamente correcto (secuencial vs asociativo), pero no documentado.

### 3.5 Sin `operator<<` / `to_string()` en ningún contenedor

Ningún contenedor principal define `operator<<` ni `to_string()`. Para debugging,
scripting y logging, esto es una carencia relevante.

### 3.6 `size()` O(n) en DynList

`DynList::size()` recorre toda la lista. Esto es coherente con el diseño (lista
enlazada sin contador), pero es una trampa para usuarios que asumen O(1).
Debería estar documentado de forma prominente.

---

## 4. Calidad de código y modernidad C++

### 4.1 C-style casts masivos

Miles de C-style casts `(Type)expr` en headers legacy, especialmente en árboles
(`tpl_rbRk.H`, `tpl_hRbTreeRk.H`, `tpl_avl.H`, etc.), FFT (`fft.H`) y nodos de
lista (`htlist.H`). Estos casts eluden las protecciones de `static_cast` y
`reinterpret_cast`.

**Estimación**: >2000 instancias.

**Recomendación**: migrar gradualmente a `static_cast`/`reinterpret_cast` donde
corresponda. Priorizar los headers más usados.

### 4.2 Features de C++20 sin adoptar

| Feature | Uso actual | Oportunidad |
|---|---|---|
| Spaceship operator (`<=>`) | 0 usos | Simplificaría comparadores en muchas clases |
| `consteval` | 1 uso | Poco impacto; oportunista |
| `std::format` | 1 archivo (`point.H`) | Podría reemplazar `std::ostringstream` en `to_string()` |
| C++20 modules | 0 | Largo plazo; el ecosistema aún no está maduro |
| `std::ranges` | 6 archivos | Poco impacto directo (Aleph tiene su propia capa funcional) |

### 4.3 `noexcept` inconsistente

Algunos contenedores lo usan bien (Array, DynList, graph-dry), otros lo omiten
en constructores por defecto, destructores y métodos triviales que no lanzan.

### 4.4 `throw` directo vs `ah_*_error`

~70 `throw` directos persisten en:
- `gmpfrxx.h` (19) — código de terceros, no tocar
- `parse_utils.H` (21) — debería migrar
- `stat_utils.H` (17) — debería migrar
- `thread_pool.H` (13) — aceptable para excepciones de hilo
- `ah-signal.H` (3) — menor

### 4.5 `using namespace Aleph` redundante

Varios headers tienen `using namespace Aleph` dentro de `namespace Aleph { ... }`,
lo cual es redundante y potencialmente confuso.

### 4.6 `using namespace std` en header

`ah-chronos-utils.H` tiene `using namespace std` dentro de un header público. Esto
contamina el namespace de cualquier inclusor.

---

## 5. Estructuras de datos faltantes

### Presentes y bien cubiertas
Listas, árboles binarios (AVL, RB, Splay, Treap, Cartesian), B-trees (memoria y
disco), hash tables (open/closed addressing, linear hashing), heaps (binary, array,
Fibonacci), grafos (lista/array/matrix de adyacencia, redes de flujo), segment trees,
Fenwick trees, interval trees, link-cut trees, skip lists, tries, filtros
probabilísticos (Bloom, Cuckoo, Quotient, HyperLogLog, Count-Min, MinHash, SimHash),
union-find, quadtrees, k-d trees, sparse tables, Mo's algorithm.

### Ausentes

| Estructura | Relevancia | Dificultad |
|---|---|---|
| **Ring buffer en memoria** | Alta (colas circulares, streaming) | Baja |
| **Rope** (string eficiente) | Media (editores de texto, strings largos) | Media |
| **Flat map / flat set** | Alta (cache-friendly, C++23 influence) | Media |
| **Small vector** (inline storage) | Alta (evitar allocation para N pequeños) | Media |
| **Radix tree / PATRICIA trie** | Media (routing, IP lookup) | Media |
| **R-tree / R\*-tree** | Media-Alta (bases de datos espaciales) | Alta |
| **Persistent data structures** | Media (functional programming, undo) | Alta |
| **Concurrent hash map** | Alta (multi-threading sin lock global) | Alta |
| **Lock-free queue** | Alta (producer-consumer sin mutex) | Alta |
| **Wavelet tree** | Baja-Media (compressed sequences) | Alta |
| **Suffix automaton** | Presente (`Suffix_Structures.H`) | — |

---

## 6. Algoritmos faltantes

### Bien cubiertos
Sorting (11 algoritmos), graph shortest paths (Dijkstra, Bellman-Ford, Floyd, Johnson,
A\*, Dial, 0-1 BFS, IDA\*), MST (Kruskal, Prim), max flow (Dinic, capacity scaling,
min-cost), SCC (Tarjan, Kosaraju), string matching (KMP, Z, Horspool, Rabin-Karp,
Aho-Corasick), suffix structures, convex hull (5 algoritmos), triangulación (Delaunay,
Ears), FFT/NTT, simplex, DP optimizations (D&C, Knuth, CHT, Li Chao).

### Huecos relevantes

| Algoritmo | Área | Relevancia |
|---|---|---|
| **Push-Relabel** (max flow) | Grafos | Alta (más rápido que Dinic en grafos densos) |
| **Hopcroft-Karp** (bipartite matching) | Grafos | Alta (O(E√V) vs O(VE)) |
| **Edmonds-Karp** explícito | Grafos | Media (BFS-based Ford-Fulkerson) |
| **SA-IS** (suffix array linear) | Strings | Media (reemplaza doubling O(n log n)) |
| **Burrows-Wheeler Transform** | Strings | Media (compresión, FM-index) |
| **Extended GCD** explícito | Numéricos | Media (fundamental en criptografía) |
| **Interpolation search** | Búsqueda | Baja (mejora sobre binary para datos uniformes) |
| **Generación de particiones** | Combinatoria | Baja (uso en DP y enumeración) |
| **3D convex hull** | Geometría | Media |
| Tests unitarios para Blossom, Hungarian, LCA, HLD, Dominators | Grafos | Alta (código existe pero sin tests) |

### Cobertura de tests por área

| Área | Cobertura estimada |
|---|---|
| Sorting / búsqueda | ~90% |
| State search / adversarial | ~95% |
| Geometría | ~85% |
| Graph (shortest path, MST, flow) | ~70% |
| Numéricos (FFT, NTT, polynomials) | ~60% |
| Strings (pattern matching) | ~40% |
| Estadística / probabilísticos | ~30% |
| DP / combinatoria | **~10%** |
| Grafos avanzados (Blossom, Hungarian, LCA, HLD) | **~5%** |

---

## 7. Cobertura de tests

### Cantidad
- ~280 archivos de test, ~271 ejecutables con GoogleTest.
- 1 archivo (`test_tpl_hash_macros.cc`) usa `main()` propia y no gtest, pero se
  enlaza con `GTest::gtest_main` (posible conflicto de doble `main`).

### Huecos críticos

| Módulo | Qué falta |
|---|---|
| **DP/Knapsack/LIS/Matrix Chain** | Tests unitarios para Knapsack.H, Subset_Sum.H, LIS.H, Matrix_Chain.H, Tree_DP.H, DP_Optimizations.H |
| **Grafos avanzados** | Tests para Blossom.H, Blossom_Weighted.H, Hungarian.H, Min_Cost_Matching.H, LCA.H, HLD.H, Dominators.H, Planarity_Test.H |
| **Strings** | Tests para Suffix_Structures.H, String_Palindromes.H, String_DP.H |
| **Numéricos** | Tests para primality.H, pollard_rho.H, modular_combinatorics.H, modular_linalg.H |
| **Filtros probabilísticos** | Tests formales para reservoir-sampling.H, minhash.H, hyperloglog.H, count-min-sketch.H, simhash.H |
| **Combinatoria** | Tests para ah-comb.H (permutaciones, combinaciones, producto cartesiano) |

### Recomendación
Priorizar tests de DP y grafos avanzados — son los módulos más usados en
competencia/producción y los menos probados.

---

## 8. Build system e infraestructura

### 8.1 Versión inconsistente

- `CMakeLists.txt`: `project(Aleph-w VERSION 1.0.0)`
- `Doxyfile`: `PROJECT_NUMBER = "3.0"`

Debería haber una sola fuente de verdad.

### 8.2 No hay `find_package` para consumidores

No existe `AlephConfig.cmake`, `AlephConfigVersion.cmake` ni `aleph.pc` (pkg-config).
Un proyecto externo no puede hacer `find_package(Aleph)` ni `pkg_check_modules(ALEPH)`.

El `install(EXPORT AlephTargets)` está incompleto: falta
`write_basic_package_version_file()` y `configure_package_config_file()`.

### 8.3 CMake presets faltantes

El README y scripts referencian presets (`default`, `clang-libcxx-ci`, `gcc-debug`),
pero no existen `CMakePresets.json` ni `CMakeUserPresets.json`.

### 8.4 `EXHAUSTIVE_TESTS` referenciada pero no definida

`Tests/CMakeLists.txt` usa `EXHAUSTIVE_TESTS` en lógica condicional, pero la variable
nunca se define. Código muerto o incompleto.

### 8.5 Sin `.clang-tidy`

Existe `.clang-format` pero no `.clang-tidy`. Un `.clang-tidy` con checks de
modernización (modernize-use-nullptr, modernize-use-override,
readability-redundant-casting) ayudaría a limpiar código legacy gradualmente.

### 8.6 Sin `.editorconfig`

No hay `.editorconfig` para garantizar consistencia de indentación/encoding entre
editores.

### 8.7 Sin package manager (Conan/vcpkg)

No hay `conanfile.py`, `vcpkg.json` ni similar. Dificulta la adopción en proyectos
que usan gestores de paquetes.

### 8.8 Sin CONTRIBUTING.md

No hay guía de contribución. Para un proyecto open-source, es fundamental.

### 8.9 Tests sin categorización

Todos los ~271 tests se ejecutan juntos. No hay etiquetas/labels de CTest para
separar unit tests rápidos de exhaustivos/lentos/benchmarks.

---

## 9. Documentación

### 9.1 Lo bueno

- README.md de altísima calidad (9/10).
- INSTALL.md detallado por plataforma.
- Doxygen generado y desplegado.
- CI verifica cobertura de Doxygen.

### 9.2 Lo que falta

| Elemento | Estado |
|---|---|
| **CONTRIBUTING.md** | Ausente |
| **CHANGELOG** versionado | Solo `CHANGELOG_v3.md` |
| **Migration guide** (de versiones anteriores) | Ausente |
| **Thread-safety policy** global | Disperso en Doxygen de cada clase |
| **Complexity guarantees** documento | Disperso; no hay tabla unificada |
| **Comparison table** (Aleph vs STL vs Boost) | Parcial en README |
| **Tutorials** por área | Los ejemplos son buenos pero no hay narrativa guiada |
| **Examples organizados** por dominio | Todo plano en `Examples/` |

### 9.3 Headers sin Doxygen

La mayoría de los headers de DP, combinatoria y grafos avanzados tienen documentación
mínima o parcial. Contrasta con los contenedores principales y geometría, que están
bien documentados.

---

## 10. Módulos utilitarios faltantes

| Módulo | Estado en Aleph | Recomendación |
|---|---|---|
| **Serialización genérica** (JSON/binary) | Solo ad-hoc para grafos y polinomios | Módulo genérico de serialización |
| **Date/time utilities** | Ninguno | Al menos wrapper sobre `<chrono>` con formateo |
| **Regex wrapper** | Ninguno | Bajo impacto; `std::regex` es suficiente |
| **File I/O utils** (paths, copy, temp) | Solo `load_file_contents()` y CSV | Wrapper básico sobre `<filesystem>` |
| **Logging framework** | `log.H` básico sin niveles | Añadir niveles (DEBUG/INFO/WARN/ERROR) |
| **`operator<<` para contenedores** | Ninguno | Mixin o funciones libres para debugging |

---

## 11. Plan de mejoras

### Fase α: Consistencia de API (impacto alto, esfuerzo medio)

| ID | Tarea | Archivos | Prioridad |
|---|---|---|---|
| α1 | Añadir `clear()` como alias de `empty()` en Array, DynList, DynSetTree, DynArray, DynHashSet, DynHashMap | 6 headers | P0 |
| α2 | Añadir `is_empty()` donde falte (DynHashSet, DynHashMap) | 2 headers | P0 |
| α3 | Añadir `contains(const T&)` a Array, DynList, DynArray (vía mixin o directo) | ah-dry.H + 3 headers | P1 |
| α4 | Añadir `EqualSequenceMethod` a DynList para `operator==` | htlist.H | P1 |
| α5 | Añadir `operator<<` genérico para contenedores (mixin o funciones libres) | ah-dry.H o nuevo header | P2 |
| α6 | Documentar `size()` O(n) en DynList de forma prominente | htlist.H | P0 |

### Fase β: Calidad de código (impacto medio, esfuerzo alto)

| ID | Tarea | Estimación |
|---|---|---|
| β1 | Migrar C-style casts a `static_cast`/`reinterpret_cast` en los 20 headers más usados | ~500 cambios |
| β2 | Eliminar `using namespace std` de `ah-chronos-utils.H` | 1 cambio |
| β3 | Eliminar `using namespace Aleph` redundante dentro de `namespace Aleph` | ~10 headers |
| β4 | Migrar `throw` directos a `ah_*_error` en `parse_utils.H` y `stat_utils.H` | ~40 cambios |
| β5 | Añadir `noexcept` a destructores, constructores triviales y swap en contenedores | ~100 sitios |
| β6 | Introducir spaceship operator (`<=>`) en clases con comparadores triviales | Oportunista |
| β7 | Crear `.clang-tidy` con checks de modernización | 1 archivo |
| β8 | Crear `.editorconfig` | 1 archivo |

### Fase γ: Build system (impacto medio, esfuerzo bajo-medio)

| ID | Tarea |
|---|---|
| γ1 | Crear `CMakePresets.json` con los presets referenciados en README y scripts |
| γ2 | Unificar versión: definir en un solo sitio y propagar a CMake y Doxyfile |
| γ3 | Completar install: añadir `AlephConfig.cmake`, `AlephConfigVersion.cmake` |
| γ4 | Generar `aleph.pc` (pkg-config) |
| γ5 | Definir o eliminar `EXHAUSTIVE_TESTS` |
| γ6 | Añadir labels de CTest para categorizar tests (unit, slow, benchmark) |
| γ7 | Corregir `test_tpl_hash_macros.cc` (conflicto de `main`) |
| γ8 | Evaluar `vcpkg.json` para facilitar adopción |

### Fase δ: Tests (impacto alto, esfuerzo alto)

| ID | Tarea | Tests a añadir |
|---|---|---|
| δ1 | DP: Knapsack.H, Subset_Sum.H, LIS.H, Matrix_Chain.H, Tree_DP.H, DP_Optimizations.H | ~6 test files |
| δ2 | Grafos avanzados: Blossom, Hungarian, LCA, HLD, Dominators, Planarity_Test | ~6 test files |
| δ3 | Strings: Suffix_Structures, String_Palindromes, String_DP | ~3 test files |
| δ4 | Numéricos: primality, pollard_rho, modular_combinatorics, modular_linalg | ~4 test files |
| δ5 | Filtros: reservoir-sampling, minhash, hyperloglog, count-min-sketch, simhash | ~5 test files |
| δ6 | Combinatoria: ah-comb.H (permutaciones, combinaciones, producto cartesiano) | ~1 test file |

### Fase ε: Documentación (impacto medio, esfuerzo medio)

| ID | Tarea |
|---|---|
| ε1 | Crear CONTRIBUTING.md |
| ε2 | Documentar thread-safety policy global |
| ε3 | Crear tabla de complejidades unificada para operaciones de contenedores |
| ε4 | Organizar `Examples/` en subcarpetas por dominio |
| ε5 | Completar Doxygen en headers de DP, combinatoria y grafos avanzados |
| ε6 | Escribir tutorials narrativos para los 3 dominios principales (grafos, contenedores, geometría) |

### Fase ζ: Estructuras y algoritmos nuevos (impacto medio-alto, esfuerzo alto)

| ID | Tarea | Prioridad |
|---|---|---|
| ζ1 | Ring buffer en memoria | P1 |
| ζ2 | Flat map / flat set (sobre Array ordenado) | P1 |
| ζ3 | Small vector (inline buffer + heap fallback) | P2 |
| ζ4 | Push-Relabel max flow | P1 |
| ζ5 | Hopcroft-Karp bipartite matching | P1 |
| ζ6 | SA-IS (suffix array lineal) | P2 |
| ζ7 | Concurrent hash map | P2 |
| ζ8 | Lock-free queue | P2 |

---

## 12. Orden de ejecución recomendado

```
Fase α (API consistency)      ← Primer paso: bajo riesgo, alto impacto en usabilidad
  ├─ α1, α2, α6              ← Triviales, hacer inmediatamente
  ├─ α3, α4                  ← Requieren diseño de mixin
  └─ α5                      ← Diseño de operator<<

Fase γ (Build system)         ← En paralelo con α
  ├─ γ1, γ2, γ5, γ7          ← Correcciones puntuales
  └─ γ3, γ4, γ6, γ8          ← Mejoras de empaquetado

Fase β (Code quality)         ← Segundo paso: gradual, por headers más usados
  ├─ β2, β3, β7, β8          ← Rápidos
  └─ β1, β4, β5, β6          ← Graduales

Fase δ (Tests)                ← Tercer paso: subir cobertura de DP y grafos
  ├─ δ1, δ6                  ← DP y combinatoria (10% → 70%)
  ├─ δ2                      ← Grafos avanzados
  └─ δ3, δ4, δ5              ← Strings, numéricos, filtros

Fase ε (Documentación)        ← En paralelo con δ
  ├─ ε1, ε2                  ← Piezas fundamentales
  └─ ε3, ε4, ε5, ε6          ← Mejoras incrementales

Fase ζ (Nuevas estructuras)   ← Último: requiere diseño y decisiones de API
  ├─ ζ1, ζ2                  ← Más pedidos / útiles
  ├─ ζ4, ζ5                  ← Algoritmos con código de referencia disponible
  └─ ζ3, ζ6, ζ7, ζ8          ← Más complejos
```

**Invariante global**: después de cada subfase, todos los tests existentes deben
seguir pasando y no debe haber warnings nuevos en Doxygen.

---

## 13. Métricas objetivo

| Métrica | Valor actual | Objetivo |
|---|---|---|
| C-style casts en headers principales | >2000 | <200 |
| `throw` directos en código propio | ~50 | <10 |
| Contenedores sin `clear()` | 6 | 0 |
| Contenedores sin `contains()` | 3 | 0 |
| Contenedores sin `operator==` | 1 (DynList) | 0 |
| Cobertura de tests en DP | ~10% | >70% |
| Cobertura de tests en grafos avanzados | ~5% | >60% |
| find_package disponible | No | Sí |
| CMakePresets.json | No | Sí |
| CONTRIBUTING.md | No | Sí |
