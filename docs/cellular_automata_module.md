# Módulo de autómatas celulares — arquitectura

> Documento de arquitectura, glosario y diagrama de capas para el módulo
> `Aleph::CA`. Acompaña al plan por fases del módulo de autómatas
> celulares. Esta versión refleja **Fase 0** (concepts y skeleton). Las
> capas concretas se materializan a partir de Fase 1.

---

## 1. Visión general

El módulo `Aleph::CA` ofrece un marco genérico para construir, ejecutar y
analizar autómatas celulares en Aleph-w. Su diseño está guiado por tres
principios:

1. **Separación estricta de capas** — cada capa tiene una responsabilidad
   única y se conecta con las demás únicamente a través de los *concepts*
   definidos en `tpl_ca_concepts.H`. Así, un cambio de storage no afecta a
   la regla, y al revés.

2. **Header-dominant** — toda la API pública vive en headers `.H` bajo
   `namespace Aleph::CA`. No se introducen nuevas dependencias externas
   obligatorias.

3. **Reuso de Aleph-w** — siempre que sea posible, se construye sobre
   piezas existentes (`tpl_dynArray.H`, `bitArray.H`, `thread_pool.H`,
   `ah-parallel.H`, `ah-errors.H`, …).

---

## 2. Diagrama de capas

```text
            +----------------------------------------------------+
            |                  Observers / Métricas              |
            |   (densidad, actividad, entropía, snapshots, ...)   |
            +----------------------------------------------------+
                                  ▲
                                  │  callbacks por step
                                  │
            +----------------------------------------------------+
            |                       Engine                        |
            |   Synchronous_Engine, Parallel_Synchronous_Engine,  |
            |   Hashlife_Engine ...                               |
            +----------------------------------------------------+
                                  ▲
            invoca regla por celda │  estado actual + Neighbor_View
                                  │
            +----------------------------------------------------+
            |                        Rule                         |
            |   Totalistic_Rule, Outer_Totalistic_Rule,           |
            |   Lookup_Rule, Probabilistic_Rule ...               |
            +----------------------------------------------------+
                                  ▲
                                  │  enumera vecinos en orden canónico
                                  │
            +----------------------------------------------------+
            |                    Neighborhood                     |
            |   Moore<N,r>, Von_Neumann<N,r>, Hex, Custom, ...    |
            +----------------------------------------------------+
                                  ▲
                                  │  consulta y muta cells
                                  │
            +----------------------------------------------------+
            |                       Lattice                       |
            |   Lattice<Storage, Boundary>, Ghost_Lattice, ...    |
            +----------------------------------------------------+
                                  ▲
                                  │  contiguous memory
                                  │
            +----------------------------------------------------+
            |                        Storage                      |
            |   Dense_Cell_Storage<T,N>, Bit_Cell_Storage<N>      |
            +----------------------------------------------------+
```

Cada capa solo conoce las inferiores y publica un *concept* hacia las
superiores. La frontera entre dos capas adyacentes está completamente
descrita por el *concept* correspondiente.

---

## 3. Glosario

- **Cell / Celda** — unidad atómica de información en el autómata. Su
  tipo cumple `CellState`.
- **Coord** — coordenada N-dimensional. Por defecto un
  `std::array<ca_index_t, N>`. Cumple `Coord<C, N>`.
- **Lattice / Retícula** — combinación de storage y topología. Conoce
  sus dimensiones y aplica una *boundary policy* en los bordes.
- **Neighborhood / Vecindario** — patrón de conectividad que enumera
  vecinos relevantes para una celda dada (Moore, Von Neumann, hex,
  custom).
- **Neighbor_View** — vista ligera (no propietaria) sobre los valores de
  los vecinos de una celda en un orden canónico (Fase 2).
- **Rule / Regla** — función pura `(state, NeighborView) → state`.
- **Engine / Motor** — orquesta la iteración: aplica la regla a cada
  celda y maneja el doble buffer (`current` / `next`).
- **Observer** — callback opcional que recibe eventos por paso para
  calcular métricas, registrar frames o detectar estados estacionarios.
- **Boundary policy** — política para los accesos fuera del rango
  válido: `OpenBoundary`, `ToroidalBoundary`, `ReflectiveBoundary`,
  `ConstantBoundary<v>`.
- **Ghost layer / Halo** — anillo de celdas extra alrededor del subgrid
  activo, refrescado en `pre_step` por la *boundary policy* (Fase 4).

---

## 4. Concepts (Fase 0)

Los concepts están definidos en `tpl_ca_concepts.H`. La intención es que
sean intencionalmente laxos: describen el mínimo viable para que el
engine de Fase 3 pueda operar sobre cualquier combinación razonable.

### `CellState<T>`

Un tipo válido como contenido de una celda. Es regular, default-
construible y comparable por igualdad. Cubre `bool`, integrales,
enums y POD pequeños.

### `Coord<C, N>`

Coordenada N-dimensional. Soporta `operator[]` y un `size()` constante
en tiempo de compilación. La especialización canónica en Aleph-w es
`Coord_Vec<N>` (alias de `std::array<ca_index_t, N>`).

### `LatticeLike<L>`

Storage y topología combinados. Expone:

- `state_type`, `coord_type` — alias.
- `dimension()` — número de ejes.
- `size(axis)` — extent del eje dado.
- `at(coord)` — lectura.
- `set(coord, value)` — escritura.

### `NeighborhoodLike<H>`

Patrón de conectividad. Expone `radius()` y `size()`. La enumeración
real (`apply` / `Neighbor_View`) se cierra en Fase 2 con un buffer en
stack para no asignar memoria.

### `RuleLike<R, L>`

Función local invocable con el estado actual de una celda; produce un
valor convertible al `state_type` del lattice. La extensión completa
con `Neighbor_View` se introduce en Fase 2.

---

## 5. Boundary policies (`ca-traits.H`)

- **`OpenBoundary`** — fuera del lattice no hay nada. La política
  concreta (cero, undefined, ...) la decide cada Lattice.
- **`ToroidalBoundary`** — wrap-around en todos los ejes.
- **`ReflectiveBoundary`** — los accesos fuera de rango espejean.
- **`ConstantBoundary<T, V>`** — fuera del rango el valor es `V`.

Las políticas son tags vacíos (zero-cost). Cada Lattice las consulta
mediante `at_safe(coord)` (Fase 1) o resolviendo el halo en
`pre_step` (Fase 4).

---

## 6. Iteración

`RowMajor` / `ColumnMajor` son tags que declaran el orden de visita.
Por defecto el engine usa `RowMajor`, que coincide con el layout C de
los storages densos. Variantes en tiles (Fase 5) viven en una capa
aparte (`ca-tiling.H`).

---

## 7. Estado actual del módulo

| Capa            | Estado    | Header(s)                                     |
|-----------------|-----------|-----------------------------------------------|
| Concepts        | Fase 0 ✔ | `tpl_ca_concepts.H`                            |
| Tag types       | Fase 0 ✔ | `ca-traits.H`                                  |
| Storage         | Fase 1 ✔ | `tpl_ca_storage.H`, `tpl_ca_bit_storage.H`     |
| Lattice         | Fase 1 ✔ | `tpl_ca_lattice.H`                             |
| Neighborhood    | Fase 2 ✔ | `tpl_ca_neighborhood.H`                        |
| Rule            | Fase 2 ✔ | `tpl_ca_rule.H`                                |
| Engine síncrono | Fase 3 ✔  | `tpl_ca_engine.H`                              |
| Ghost / halo    | pendiente | `tpl_ca_ghost_lattice.H`                       |
| Paralelo        | pendiente | `tpl_ca_parallel_engine.H`                     |
| Topologías      | pendiente | `tpl_ca_hex_lattice.H`, `tpl_ca_graph_automaton.H` |
| Observers       | pendiente | `ca-observer.H`, `ca-metrics.H`                |
| Estocásticos    | pendiente | `ca-rng.H`                                     |
| Continuos       | Fase 9 ✔ | `ca-kernels.H`, `tpl_ca_continuous_rules.H`    |
| Hashlife        | pendiente | `CA_Hashlife.H`                                |
| IO / visualización | Fase 11.5 ✔ | `ca-io.H`, `ca-tikz.H`, `ca-frame-stream.H`, `ca-png.H`, `ca-gif.H`, `ca-svg.H`, `ca-npy.H`, `ca-vtk.H`, `ca-html-player.H`, `ca-ffmpeg-sink.H`, `ca-dot.H`, `ca-ascii.H`, `ca-x11-viewer.H` |

Fase 11.5 añade un contrato común `Frame_Sink`, composición de sinks y
backends de salida para PNG, GIF, SVG, NPY, VTK, HTML autocontenido,
GraphViz DOT, terminal ASCII/ANSI, ffmpeg en runtime y visor X11 en vivo.
Los backends son headers independientes y no agregan dependencias de build.

El estado de cada fase se mantiene además en
[`cell-automata.md`](../cell-automata.md) con checkboxes.

---

## 8. Convenciones recordatorias

- Namespace: `Aleph::CA`.
- Headers: `tpl_ca_*.H` (plantillas), `ca-*.H` (utilidades),
  `CA_<Algoritmo>.H` (algoritmos con nombre propio).
- Estilo: 2 espacios, llaves GNU, `and / or / not`, guards tradicionales.
- Errores: `ah-errors.H` exclusivamente.
- Doxygen completo en inglés en cada API pública.

---

## 9. Próximos pasos

1. **Fase 12** — preparar documentación de release y revisar warnings
   Doxygen del módulo completo.
2. **Fase 13** — añadir esquemas de actualización asíncronos y por
   bloques.
3. **Fase 14** — modelar autómatas multi-campo con layouts AoS/SoA.
