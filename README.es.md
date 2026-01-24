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
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![GCC](https://img.shields.io/badge/GCC-11%2B-orange.svg)](https://gcc.gnu.org/)
[![Clang](https://img.shields.io/badge/Clang-14%2B-orange.svg)](https://clang.llvm.org/)

</div>

---

## Contenido

- [Resumen](#resumen)
- [Requisitos](#requisitos)
- [Instalación](#instalación)
- [Uso rápido](#uso-rápido)
- [Pruebas](#pruebas)
- [Contribuir](#contribuir)
- [Licencia](#licencia)

> Nota: `README.md` (en inglés) contiene el detalle completo (listados extensos de estructuras, algoritmos, tablas y ejemplos).

---

## Resumen

**Aleph-w** es una librería C++ (C++20) orientada a enseñanza y uso práctico: incluye implementaciones de estructuras de datos, algoritmos clásicos (especialmente de grafos) y una suite de pruebas amplia.

- **Distribución**: headers en el raíz del repo + librería estática `libAleph.a`.
- **Build**: CMake (con presets opcionales) y tests con GoogleTest.

---

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

---

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

## Contribuir

PRs son bienvenidos.

- Mantén cambios pequeños y con tests cuando aplique.
- Si agregas algoritmos/estructuras, añade tests en `Tests/` y (idealmente) un ejemplo en `Examples/`.

---

## Licencia

Aleph-w se distribuye bajo **GNU GPL v3.0**. Ver `LICENSE`.

