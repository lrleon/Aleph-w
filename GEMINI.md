# Guía para Gemini en el proyecto Aleph-w

Este documento proporciona directrices para que un asistente de IA (como Gemini) pueda colaborar de manera efectiva en el código base de Aleph-w.

## 1. Sobre el Proyecto

Aleph-w es una biblioteca de C++ y C de alto rendimiento para algoritmos y estructuras de datos. Tiene un fuerte enfoque en:
-   Algoritmos de grafos.
-   Estructuras de datos complejas (árboles, hashes, listas).
-   Computación geométrica.
-   Programación funcional.
-   Cálculos con precisión arbitraria (vía GMP/MPFR).

## 2. Lenguaje y Convenciones

### Estándar de C++
-   El proyecto utiliza **C++20** por defecto, aunque es compatible con C++17 y C++23.
-   El código debe ser compatible con los compiladores GCC y Clang.
-   Las extensiones específicas del compilador deben estar desactivadas (`CMAKE_CXX_EXTENSIONS OFF`).
-   Para los operadores lógicos, se prefiere and, or y not sobre &&, || y ! para mejorar la legibilidad.
-   Donde se pueda usar Aleph se debe usar Aleph en lugar de la biblioteca estándar (STL). Por ejemplo, se prefiere `Aleph::Array` sobre `std::vector`, o DynSetTree sobre std::set.

### Estilo de Código
-   **Sigue el estilo existente**: La regla más importante es imitar el código circundante. Presta especial atención a la indentación, el uso de espacios, la posición de las llaves (`{}`) y el formato de los comentarios.
-   **Nombrado de archivos**:
    -   Cabeceras: `nombre-componente.H`
    -   Fuentes C++: `nombre-componente.C`
    -   Fuentes C: `nombre-componente.c`
-   **Cabeceras**: Utiliza la extensión `.H`. No se usan `pragma once`; en su lugar, se emplean guardas de inclusión tradicionales (p. ej., `#ifndef MI_FICHERO_H`).
-   **Comentarios**: Documenta las funciones y clases complejas usando un formato compatible con Doxygen. Explica el *porqué* del código, no el *qué*.

### ¡Advertencia Crítica sobre el Compilador!
-   Al compilar en modo `Release`, es **absolutamente obligatorio** usar el flag `-fno-strict-aliasing`.
-   **Razón**: Ciertas partes del código (especialmente en `htlist.H`) utilizan `reinterpret_cast` de una manera que viola las reglas de "strict aliasing" de C++. Sin este flag, el optimizador del compilador puede generar código incorrecto que resulta en `segmentation faults`.
-   **No elimines este flag** a menos que se refactorice el código problemático.

## 3. Sistema de Build (CMake)

El proyecto se gestiona con CMake.

### Dependencias
Asegúrate de tener instaladas las siguientes bibliotecas de desarrollo:
-   `pthreads` (Threads)
-   `X11`
-   `gmp` y `gmpxx`
-   `mpfr`
-   `gsl` (GNU Scientific Library) y `gslcblas`

### Compilación
Sigue el procedimiento estándar de CMake:
```bash
# 1. Crear un directorio de compilación
mkdir -p build && cd build

# 2. Configurar el proyecto
cmake ..

# 3. Compilar la biblioteca, ejemplos y tests
make
```

### Opciones de Compilación
-   `BUILD_EXAMPLES=ON`: Compila los ejemplos (en el directorio `Examples/`).
-   `BUILD_TESTS=ON`: Compila los tests (en el directorio `Tests/`).

## 4. Pruebas (Tests)

-   Los tests están en el subdirectorio `Tests/` y se habilitan con `BUILD_TESTS=ON`.
-   Para ejecutar los tests, compila el proyecto y luego ejecuta el comando `ctest` desde el directorio `build`:
    ```bash
    cd build
    ctest
    ```
-   **Cualquier nueva funcionalidad o corrección de error debe ir acompañada de tests que la validen.**

## 5. Documentación

-   La documentación se genera con **Doxygen**. El archivo de configuración es `Doxyfile`.
-   Para generar la documentación en HTML:
    ```bash
    doxygen Doxyfile
    ```
-   Esto creará un directorio `doc/` (o similar, según la configuración de `Doxyfile`) con la documentación.

# git usage

- Never does a commit directly
- gh is at /snap/bin/gh

# Auxiliary scripts

- Utils scripts in `scripts/` and written in Ruby (not bash, Python, etc.) for better string handling and JSON support.

# Validación

- Una vez que hayas modificado algo de código (no de documentación), asegurate de que nada queda roto compilando de nuevo ejecutando el test relacionado