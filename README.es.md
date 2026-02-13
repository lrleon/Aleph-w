# Aleph-w

<div align="center">

Idioma: Español | [English](README.md)

```
     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / /
   / ___ \| |  __/ |_) | | | |_____\ V  V /
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/
                 |_|
```

**Una biblioteca C++20 completa de estructuras de datos y algoritmos**

[![CI](https://github.com/lrleon/Aleph-w/actions/workflows/ci.yml/badge.svg)](https://github.com/lrleon/Aleph-w/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![GCC](https://img.shields.io/badge/GCC-11%2B-orange.svg)](https://gcc.gnu.org/)
[![Clang](https://img.shields.io/badge/Clang-14%2B-orange.svg)](https://clang.llvm.org/)

</div>

---

<a id="readme-es-contenido"></a>
## Contenido

- [Resumen](#readme-es-resumen)
- [Requisitos](#readme-es-requisitos)
- [Instalación](#readme-es-instalacion)
- [Uso rápido](#readme-es-uso-rapido)
- [Pruebas](#readme-es-pruebas)
- [Contribuir](#readme-es-contribuir)
- [Licencia](#readme-es-licencia)
- [Agradecimientos](#readme-es-agradecimientos)

> Nota: `README.md` (en inglés) contiene el detalle completo (listados extensos de estructuras, algoritmos, tablas y ejemplos), incluyendo el flujo completo de Delaunay/Voronoi y los ejemplos:
> - `Examples/geom_example.C` (incluye selector `-s advanced`)
> - `Examples/voronoi_clipped_cells_example.cc`
> - `Examples/delaunay_voronoi_example.cc`
> - `Examples/point_in_polygon_example.cc`
> - `Examples/polygon_intersection_example.cc`
> - `Examples/halfplane_intersection_example.cc`
> - `Examples/convex_hull_comparison_example.cc`
> - `Examples/closest_pair_example.cc`
> - `Examples/rotating_calipers_example.cc`

---

<a id="readme-es-resumen"></a>
## Resumen

**Aleph-w** es una librería C++ (C++20) orientada a enseñanza y uso práctico: incluye implementaciones de estructuras de datos, algoritmos clásicos (especialmente de grafos) y una suite de pruebas amplia.

- **Distribución**: headers en el raíz del repo + librería estática `libAleph.a`.
- **Build**: CMake (con presets opcionales) y tests con GoogleTest.

---

<a id="readme-es-requisitos"></a>
## Requisitos

### Compiladores

- **GCC** 11+ (recomendado 13+)
- **Clang** 14+ (recomendado 16+)
- **MSVC**: no soportado oficialmente

### Dependencias (esta rama)

Esta rama enlaza contra estas librerías:

| Librería | Paquete (Ubuntu/Debian) | Uso | Requerida |
|---------|--------------------------|-----|----------|
| GMP + gmpxx | `libgmp-dev` | enteros/racionales multiprecisión | Sí |
| MPFR | `libmpfr-dev` | punto flotante multiprecisión | Sí |
| GSL (+ gslcblas) | `libgsl-dev` | RNG/estadística usada por estructuras/algoritmos aleatorizados | Sí |
| X11 | `libx11-dev` | dependencia de enlace | Sí |
| GoogleTest | `libgtest-dev` | tests | Solo si compilas tests (auto-fetch opcional) |

### Herramientas

- **CMake** 3.18+
- **CMake presets**: `CMakePresets.json` requiere CMake 3.21+ (opcional)
- **Ninja** (recomendado) o **Make**

### Instalación rápida (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build g++ clang \
  libgmp-dev libmpfr-dev libgsl-dev libx11-dev

# Opcional (si quieres tests sin auto-fetch, o si estás offline)
sudo apt-get install -y libgtest-dev
```

---

<a id="readme-es-instalacion"></a>
## Instalación

Guía detallada: `INSTALL.es.md`.

### Método 1: CMake (recomendado)

```bash
git clone https://github.com/lrleon/Aleph-w.git
cd Aleph-w

cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_EXAMPLES=OFF \
  -DBUILD_TESTS=ON

cmake --build build --parallel
ctest --test-dir build --output-on-failure

# Instalación (opcional)
sudo cmake --install build
```

Si GoogleTest no está instalado y no tienes red, desactiva el auto-fetch (`-DALEPH_FETCH_GTEST=OFF`) o desactiva tests (`-DBUILD_TESTS=OFF`), y/o instala `libgtest-dev`.

### Método 2: Compilación directa (sin instalar)

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF -DBUILD_EXAMPLES=OFF
cmake --build build --parallel

g++ -std=c++20 -I/path/to/Aleph-w your_code.cpp /path/to/Aleph-w/build/libAleph.a \
  -lgmpxx -lgmp -lmpfr -lgsl -lgslcblas -lpthread -lX11 -lm \
  -o your_program
```

### Método 3: CMake FetchContent (subproyecto)

```cmake
include(FetchContent)
FetchContent_Declare(
    aleph-w
    GIT_REPOSITORY https://github.com/lrleon/Aleph-w.git
    GIT_TAG v3
)
FetchContent_MakeAvailable(aleph-w)

target_link_libraries(your_target PRIVATE Aleph)
```

---

<a id="readme-es-uso-rapido"></a>
## Uso rápido

Los headers están en el raíz del repo (p.ej. `tpl_dynSetTree.H`, `tpl_graph.H`, etc.).

### Ejemplo 1: Set con árbol balanceado

```cpp
#include <tpl_dynSetTree.H>
#include <iostream>

int main() {
    DynSetTree<int, Avl_Tree> numbers;

    numbers.insert(50);
    numbers.insert(30);
    numbers.insert(70);
    numbers.insert(20);
    numbers.insert(40);

    if (numbers.contains(30))
        std::cout << "Found 30!\n";

    std::cout << "Sorted: ";
    numbers.for_each([](int x) { std::cout << x << " "; });

    return 0;
}
```

### Ejemplo 2: Grafo + Dijkstra

```cpp
#include <tpl_graph.H>
#include <Dijkstra.H>
#include <iostream>

int main() {
    using Node = Graph_Node<std::string>;
    using Arc = Graph_Arc<double>;
    using Graph = List_Graph<Node, Arc>;

    Graph city_map;

    auto* ny = city_map.insert_node("New York");
    auto* la = city_map.insert_node("Los Angeles");
    auto* ch = city_map.insert_node("Chicago");

    city_map.insert_arc(ny, ch, 790.0);
    city_map.insert_arc(ch, la, 2015.0);
    city_map.insert_arc(ny, la, 2790.0);

    Path<Graph> path(city_map);
    double distance = dijkstra_min_path(city_map, ny, la, path);

    std::cout << "Shortest distance: " << distance << "\n";
    return 0;
}
```

### Ejemplo 3: Fenwick Tree (sumas de prefijo y rangos)

```cpp
#include <tpl_fenwick_tree.H>
#include <iostream>

int main() {
    // Fenwick clásico: actualización puntual + consulta de rango
    Fenwick_Tree<int> ft = {3, 1, 4, 1, 5, 9};
    std::cout << "Suma [0..3]: " << ft.prefix(3) << "\n";   // 9
    std::cout << "Suma [2..4]: " << ft.query(2, 4) << "\n";  // 10

    ft.update(2, 6);  // a[2] += 6
    std::cout << "Suma [2..4] tras update: " << ft.query(2, 4) << "\n"; // 16

    // find_kth: menor índice cuyo prefijo acumulado >= k
    size_t idx = ft.find_kth(10);  // útil para order statistics

    // Range Fenwick: actualización y consulta por rangos
    Range_Fenwick_Tree<int> rft(8);
    rft.update(1, 4, 3);   // a[1..4] += 3
    rft.update(2, 6, 5);   // a[2..6] += 5
    std::cout << "Valor en pos 3: " << rft.get(3) << "\n";  // 8

    // Genérico sobre cualquier grupo abeliano (e.g. XOR)
    struct Xor { int operator()(int a, int b) const { return a ^ b; } };
    Gen_Fenwick_Tree<int, Xor, Xor> xft(10);
    xft.update(3, 0b1010);

    return 0;
}
```

### Ejemplo 4: Álgebra Lineal Sparse (matrices y vectores dispersos)

```cpp
#include <al-vector.H>
#include <al-matrix.H>
#include <iostream>

int main() {
    // Vector disperso con dominio de strings
    AlDomain<std::string> productos;
    productos.insert("Laptop");
    productos.insert("Teléfono");
    productos.insert("Tablet");

    Vector<std::string, double> inventario(productos);
    inventario.set_entry("Laptop", 150.0);
    inventario.set_entry("Teléfono", 450.0);
    // Tablet implícitamente 0 (no se almacena)

    std::cout << "Inventario Laptop: " << inventario["Laptop"] << "\n";

    // Matriz dispersa con indexación por dominio
    AlDomain<std::string> tiendas;
    tiendas.insert("Bogotá");
    tiendas.insert("Medellín");
    tiendas.insert("Cali");

    Matrix<std::string, std::string, int> ventas(productos, tiendas);
    ventas.set_entry("Laptop", "Bogotá", 25);
    ventas.set_entry("Teléfono", "Medellín", 80);
    // Otras entradas son cero implícitamente (no se almacenan)

    // Multiplicación matriz-vector
    Vector<std::string, int> total_por_producto = ventas * inventario;

    // Transpuesta
    auto ventas_t = ventas.transpose();

    // Ventaja: matriz 1000×1000 con 1000 no-ceros:
    //   Densa:  1,000,000 doubles = 8 MB
    //   Sparse:     1,000 entries = 8 KB (ahorro 1000×)

    return 0;
}
```

### Ejemplo 5: Voronoi recortado (celdas indexadas por sitio)

El ejemplo `Examples/voronoi_clipped_cells_example.cc` muestra cómo:

- construir Voronoi desde un conjunto de puntos,
- recortar todas las celdas contra un polígono convexo, y
- exportar celdas `ClippedCell` a CSV/WKT.

```bash
cmake --build build --target voronoi_clipped_cells_example
./build/Examples/voronoi_clipped_cells_example /tmp/voronoi_clipped_cells.csv
```

---

<a id="readme-es-pruebas"></a>
## Pruebas

```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure

# Ejecutar un binario específico
./build/Tests/dynlist
./build/Tests/test_dijkstra
./build/Tests/latex_floyd_test
```

Nota: algunos tests largos/de performance están marcados `Disabled/Skipped` intencionalmente; `ctest` los lista al final.

---

<a id="readme-es-contribuir"></a>
## Contribuir

PRs son bienvenidos.

- Mantén cambios pequeños y con tests cuando aplique.
- Si agregas algoritmos/estructuras, añade tests en `Tests/` y (idealmente) un ejemplo en `Examples/`.

---

<a id="readme-es-licencia"></a>
## Licencia

Aleph-w se distribuye bajo **licencia MIT**. Ver `LICENSE`.

---

<a id="readme-es-agradecimientos"></a>
## Agradecimientos

- **Universidad de Los Andes** (Mérida, Venezuela) - Cuna de Aleph-w
- **Miles de estudiantes** que aprendieron algoritmos con esta biblioteca
- **Comunidad de código abierto** por retroalimentación continua
- **[SYMYL RESEARCH](https://simylresearch.com/en/)** por apoyar el desarrollo y avance de Aleph-w

---
