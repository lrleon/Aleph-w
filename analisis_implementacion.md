# Análisis de Implementación Aleph-w (Febrero 2026)

Este documento presenta un contraste entre el backlog de algoritmos faltantes (`falta.md`) y el estado real de la librería tras una inspección exhaustiva de los archivos de cabecera (`.H`) y fuentes (`.C`).

---

## 📊 Resumen de Estado por Categoría

| Categoría | Estado según `falta.md` | Estado Real (Inspección) | Observaciones |
| :--- | :--- | :--- | :--- |
| **1) Strings** | Pendiente | ❌ **No implementado** | Faltan KMP, Z, Aho-Corasick, Suffix Array/Tree. |
| **2) Range Queries** | ✅ Listo (BIT, SegTree, etc.) | ✅ **Confirmado** | Suite muy completa en `tpl_segment_tree.H`, `tpl_fenwick_tree.H`, etc. |
| **3) Programación Dinámica** | Pendiente | ❌ **No implementado** | Faltan clásicos (Knapsack, LIS) y optimizaciones (CHT, Li Chao). |
| **4) Teoría de Números** | Pendiente | ⚠️ **Básico** | `primes.H` solo tiene trial division. Se usa `std::gcd`. Faltan Miller-Rabin, Pollard Rho, CRT y FFT. |
| **5) Geometría Computacional** | "Zona que cojea" | ✅ **Muy Robusto** | **Discrepancia detectada**: `geom_algorithms.H` es una suite de +13k líneas con algoritmos avanzados. |
| **6) Grafos (Avanzado)** | Parcial | ✅ **Avanzado** | Hopcroft-Karp, Hungarian y Blossom están presentes. Faltan HLD y Centroid Decomp. |
| **7) Utilitarios** | Parcial | ✅ **Parcial** | Buen soporte de sorting/searching. Falta IDA* y BFS 0-1. |
| **8) Probabilísticos** | Parcial | ⚠️ **Básico** | Bloom Filter listo. Faltan Count-Min Sketch e HyperLogLog. |

---

## 🔍 Hallazgos Detallados

### 1. Geometría Computacional (La gran sorpresa)
Contrario a lo que indica el backlog, Aleph-w tiene una de las secciones de geometría más completas para una librería de propósito general:
- **Triangulación:** Ear-cutting (`O(n²)`) y Bowyer-Watson para Delaunay.
- **Convex Hull:** Andrew, Graham, QuickHull, Gift Wrapping.
- **Diagramas:** Voronoi (dual de Delaunay).
- **Intersección:** Polígonos convexos (Sutherland-Hodgman) y Half-plane intersection (`O(n log n)`).
- **MEC:** Círculo mínimo envolvente via Welzl (`O(n)` esperado).
- **Estructuras Espaciales:** AABBTree y 2D-Tree (en `tpl_2dtree.H`).
- **Simplificación:** Douglas-Peucker y Visvalingam-Whyatt.

### 2. Grafos: Matching y Conectividad
La suite de grafos es muy potente y supera el estado "parcial" sugerido:
- **Hopcroft-Karp:** Implementado en `tpl_bipartite.H` (`hopcroft_karp_matching`).
- **Matching General:** Algoritmos de Blossom y Blossom Weighted presentes.
- **Asignación:** Algoritmo Húngaro implementado en `Hungarian.H`.
- **Conectividad:** Detección de puentes y puntos de articulación en `tpl_cut_nodes.H`.

### 3. Teoría de Números y Álgebra
- **Primalidad:** `primes.H` es rudimentario. No hay Miller-Rabin determinista.
- **MCD:** No hay implementación propia; los tests y ejemplos dependen de `std::gcd` (C++17/20).
- **Faltantes Críticos:** Pollard Rho (factorización rápida) y FFT/NTT (convolución de polinomios).

### 4. Strings: El área más débil
No se han encontrado implementaciones de algoritmos de búsqueda de patrones. Es el área con mayor prioridad de desarrollo según la demanda actual de algoritmos competitivos y de procesamiento de texto.

---

## 🚀 Recomendaciones Próximas

1.  **Actualizar `falta.md`:** Mover toda la sección de Geometría a "Completado" y reflejar el estado real de los algoritmos de Matching.
2.  **Prioridad 1 (Strings):** Implementar KMP y Z-algorithm (son la base de muchos otros).
3.  **Prioridad 2 (Teoría de Números):** Implementar Miller-Rabin y Pollard Rho para elevar el perfil de `primes.H`.
4.  **Prioridad 3 (DP):** Agregar LIS (O(n log n)) y Knapsack como funciones utilitarias en una nueva cabecera `ah-dp.H`.
