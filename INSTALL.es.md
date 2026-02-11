# Guía de instalación de Aleph-w

Idioma: Español | [English](INSTALL.md)

Esta guía cubre la instalación de Aleph-w desde el código fuente usando CMake.

---
<a id="install-es-contenido"></a>
## Contenido

- [Prerrequisitos](#install-es-prerrequisitos)
  - [Requisitos de compilador](#install-es-requisitos-de-compilador)
  - [Herramientas de build](#install-es-herramientas-de-build)
  - [Librerías requeridas](#install-es-librerias-requeridas)
  - [Librerías opcionales](#install-es-librerias-opcionales)
- [Instalación en Linux](#install-es-instalacion-en-linux)
  - [Ubuntu/Debian](#install-es-ubuntudebian)
  - [Fedora/RHEL](#install-es-fedorarhel)
  - [Arch Linux](#install-es-arch-linux)
- [Compilación desde el código fuente](#install-es-compilacion-desde-el-codigo-fuente)
  - [Build básico](#install-es-build-basico)
  - [Presets de CMake (opcional)](#install-es-presets-de-cmake-opcional)
  - [Opciones de build](#install-es-opciones-de-build)
  - [Tipos de build](#install-es-tipos-de-build)
- [Configuración avanzada](#install-es-configuracion-avanzada)
- [Verificación](#install-es-verificacion)
- [Solución de problemas](#install-es-solucion-de-problemas)

---
<a id="install-es-prerrequisitos"></a>
## Prerrequisitos

<a id="install-es-requisitos-de-compilador"></a>
### Requisitos de compilador

Aleph-w requiere un compilador moderno con soporte completo de C++20 (C++17 y C++23 también están soportados):

| Compilador | Versión mínima | Versiones probadas | Estado |
|-----------|-----------------|-------------------|--------|
| **GCC** | 11.0 | 11, 12, 13 | ✅ Soportado |
| **Clang** | 14.0 | 14, 15, 16 | ✅ Soportado |
| **MSVC** | - | - | ❌ No soportado |

<a id="install-es-herramientas-de-build"></a>
### Herramientas de build

- **CMake** 3.18+ (3.21+ si usas `CMakePresets.json`)
- **Make** o **Ninja** (Ninja recomendado)

<a id="install-es-librerias-requeridas"></a>
### Librerías requeridas

Esta rama enlaza contra:

| Librería | Paquete (Ubuntu/Debian) | Propósito |
|---------|--------------------------|-----------|
| **GMP + gmpxx** | `libgmp-dev` | enteros/racionales multiprecisión (incluye bindings C++) |
| **MPFR** | `libmpfr-dev` | punto flotante multiprecisión |
| **GSL (+ gslcblas)** | `libgsl-dev` | RNG/estadística usada por estructuras/algoritmos aleatorizados |
| **X11** | `libx11-dev` | dependencia de enlace |

Threads (pthreads) son requeridos y normalmente vienen con la libc del sistema (no suele hacer falta un paquete aparte).

<a id="install-es-librerias-opcionales"></a>
### Librerías opcionales

| Librería | Paquete | Propósito |
|---------|---------|-----------|
| **GoogleTest** | `libgtest-dev` | tests (auto-fetch si no está instalado) |

Auto-fetch de GoogleTest requiere acceso a red; usa `-DALEPH_FETCH_GTEST=OFF` para desactivarlo (y/o `-DBUILD_TESTS=OFF`).

**Nota:** Las siguientes dependencias **ya no son necesarias**:
- ~~`autosprintf` / `gettext`~~ (removidas)
- ~~`nana`~~ (removida)
- ~~`m4`~~ (removida)
- ~~`xutils-dev`~~ (ya no se usa Imakefile/xmkmf)

---

<a id="install-es-instalacion-en-linux"></a>
## Instalación en Linux

<a id="install-es-ubuntudebian"></a>
### Ubuntu/Debian

```bash
sudo apt-get update

# Compilador (elige uno)
sudo apt-get install -y g++      # GCC
sudo apt-get install -y clang    # Clang (alternativa)

# Herramientas
sudo apt-get install -y cmake ninja-build

# Librerías requeridas
sudo apt-get install -y \
  libgmp-dev \
  libmpfr-dev \
  libgsl-dev \
  libx11-dev

# Opcional: GoogleTest (si no quieres auto-fetch o estás offline)
sudo apt-get install -y libgtest-dev
```

<a id="install-es-fedorarhel"></a>
### Fedora/RHEL

```bash
sudo dnf install -y gcc-c++      # GCC
# sudo dnf install -y clang      # Clang (alternativa)

sudo dnf install -y cmake ninja-build

sudo dnf install -y \
  gmp-devel \
  mpfr-devel \
  gsl-devel \
  libX11-devel

sudo dnf install -y gtest-devel
```

<a id="install-es-arch-linux"></a>
### Arch Linux

```bash
sudo pacman -S gcc
# sudo pacman -S clang

sudo pacman -S cmake ninja

sudo pacman -S gmp mpfr gsl libx11
sudo pacman -S gtest
```

---

<a id="install-es-compilacion-desde-el-codigo-fuente"></a>
## Compilación desde el código fuente

<a id="install-es-build-basico"></a>
### Build básico

```bash
git clone https://github.com/lrleon/Aleph-w.git
cd Aleph-w

cmake -S . -B build -G Ninja
cmake --build build --parallel

# Tests (opcional pero recomendado)
ctest --test-dir build --output-on-failure

# Instalación (opcional)
sudo cmake --install build
```

<a id="install-es-presets-de-cmake-opcional"></a>
### Presets de CMake (opcional)

Si tienes CMake 3.21+, puedes usar `CMakePresets.json`:

```bash
cmake --preset default
cmake --build --preset default
ctest --preset default
```

**Alternativa usando Make:**

```bash
cmake -S . -B build
cmake --build build -j"$(nproc)"
```

<a id="install-es-opciones-de-build"></a>
### Opciones de build

| Opción | Default | Descripción |
|-------|---------|-------------|
| `CMAKE_BUILD_TYPE` | `Debug` | `Debug`, `Release`, `RelWithDebInfo`, `MinSizeRel` |
| `ALEPH_CXX_STANDARD` | `20` | estándar C++: `17`, `20`, `23` |
| `BUILD_EXAMPLES` | `ON` | compila ejemplos |
| `BUILD_TESTS` | `ON` | compila suite de tests |
| `BUILD_OPTIMIZED` | `OFF` | conveniencia: si no defines `CMAKE_BUILD_TYPE`, usa `Release` |

Ejemplo:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DALEPH_CXX_STANDARD=20 \
  -DBUILD_EXAMPLES=ON \
  -DBUILD_TESTS=ON

cmake --build build --parallel
```

<a id="install-es-tipos-de-build"></a>
### Tipos de build

#### Debug (default)

```bash
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug --parallel
```

#### Release

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --parallel
```

**Importante:** `-fno-strict-aliasing` es necesario (ver comentario en `CMakeLists.txt`).

#### Release con símbolos (RelWithDebInfo)

```bash
cmake -S . -B build-relwithdebinfo -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-relwithdebinfo --parallel
```

#### “Optimized” (conveniencia)

```bash
cmake -S . -B build-optimized -DBUILD_OPTIMIZED=ON
cmake --build build-optimized --parallel
```

Si quieres `-Ofast`, pásalo explícitamente vía `CMAKE_CXX_FLAGS_RELEASE` (con cuidado).

---

<a id="install-es-configuracion-avanzada"></a>
## Configuración avanzada

### Seleccionar estándar C++

```bash
cmake -S . -B build -DALEPH_CXX_STANDARD=17
cmake -S . -B build -DALEPH_CXX_STANDARD=20
cmake -S . -B build -DALEPH_CXX_STANDARD=23
```

### Seleccionar compilador

```bash
cmake -S . -B build -DCMAKE_CXX_COMPILER=g++ -DCMAKE_C_COMPILER=gcc
cmake -S . -B build -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang
```

### Prefijo de instalación

```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/opt/aleph
cmake --build build --parallel
sudo cmake --install build
```

### Compilar solo la librería (sin ejemplos/tests)

```bash
cmake -S . -B build -DBUILD_EXAMPLES=OFF -DBUILD_TESTS=OFF
cmake --build build --parallel
```

### Out-of-source build

Recomendado evitar `cmake .` en el directorio fuente:

```bash
cmake -S . -B build     # ✅
cmake .                # ❌
```

---

<a id="install-es-verificacion"></a>
## Verificación

### Ejecutar tests

```bash
ctest --test-dir build --output-on-failure
ctest --test-dir build -V
ctest --test-dir build -j"$(nproc)"

# Ejecutar binarios específicos
./build/Tests/dynlist
./build/Tests/test_dijkstra
./build/Tests/latex_floyd_test
```

Nota: algunos tests largos/de performance están marcados `Disabled/Skipped` intencionalmente; `ctest` los lista al final.

### Sanitizers (ASan/UBSan)

Opción 1: pasar flags globales (instrumenta también `libAleph.a`):

```bash
cmake -S . -B build-asan -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTS=ON \
  -DBUILD_EXAMPLES=OFF \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -g" \
  -DCMAKE_C_FLAGS="-fsanitize=address,undefined -g"

cmake --build build-asan --parallel
ctest --test-dir build-asan --output-on-failure
```

### Verificar instalación

```bash
ls /usr/local/lib/libAleph.a
ls /usr/local/include/aleph/
```

Ejemplo mínimo:

```bash
cat > test.cpp << 'EOF'
#include <tpl_dynList.H>
#include <iostream>

int main() {
    Aleph::DynList<int> list = {1, 2, 3, 4, 5};
    list.for_each([](int x) { std::cout << x << " "; });
    std::cout << "\n";
    return 0;
}
EOF

g++ -std=c++20 -I/usr/local/include/aleph test.cpp \
  -L/usr/local/lib -lAleph \
  -lgmpxx -lgmp -lmpfr -lgsl -lgslcblas -lpthread -lX11 -lm \
  -o test

./test
```

---

<a id="install-es-solucion-de-problemas"></a>
## Solución de problemas

### 1) CMake muy viejo

Si tu distro trae CMake < 3.18, actualiza CMake (p.ej. Kitware, backports, etc.).

### 2) GMP/MPFR/GSL no encontrados

Instala los paquetes *-dev/*-devel correspondientes (según la distro).

### 3) X11 no encontrado

Error típico:

```
Could not find X11
```

Instala `libx11-dev` (Ubuntu/Debian) o `libX11-devel` (Fedora/RHEL).

### 4) Tests fallan o GoogleTest no está disponible

```bash
cmake -S . -B build -DBUILD_TESTS=ON -DALEPH_FETCH_GTEST=OFF
```

Si estás offline y no tienes `libgtest-dev`, desactiva tests:

```bash
cmake -S . -B build -DBUILD_TESTS=OFF
```

### 5) Compilador demasiado viejo

Verifica versiones:

```bash
g++ --version
clang++ --version
```

---

## Compilar ejemplos

```bash
cmake -S . -B build -DBUILD_EXAMPLES=ON
cmake --build build --parallel

./build/Examples/dijkstra_example
./build/Examples/mst_example
./build/Examples/network_flow_example
```

---

## Usar Aleph-w en tu proyecto

### Método 1: instalación system-wide

```cmake
find_library(ALEPH_LIB Aleph)
find_path(ALEPH_INCLUDE_DIR tpl_dynList.H PATHS /usr/local/include/aleph)

find_package(Threads REQUIRED)
find_package(X11 REQUIRED)

target_include_directories(your_target PRIVATE ${ALEPH_INCLUDE_DIR})
target_link_libraries(your_target PRIVATE
  ${ALEPH_LIB}
  gmpxx gmp mpfr gsl gslcblas m
  Threads::Threads
  ${X11_LIBRARIES}
)
```

### Método 2: FetchContent

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

### Método 3: add_subdirectory

```cmake
add_subdirectory(path/to/Aleph-w)
target_link_libraries(your_target PRIVATE Aleph)
```

---

## Obtener ayuda

1. Revisa los issues: https://github.com/lrleon/Aleph-w/issues
2. Incluye en el reporte: OS + versión, compilador + versión, CMake + versión, y el log completo del error.
