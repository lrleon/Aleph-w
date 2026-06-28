# Rust bindings (Phase 25)

Experimental Rust bindings to the Aleph-w cellular-automata C ABI
([`ca-c-api.h`](../../ca-c-api.h)).

| Crate | Role |
|-------|------|
| [`aleph-ca-sys`](aleph-ca-sys/) | raw `extern "C"` FFI declarations + link config |
| [`aleph-ca`](aleph-ca/) | safe, idiomatic wrapper (`GameOfLife`) |

## Prerequisites

Build the native shared library first (CMake target `alephca_c`):

```bash
cmake -S . -B build -G Ninja
cmake --build build --target alephca_c
```

## Build & test

Point the linker and loader at the built library:

```bash
ALEPH_CA_LIB_DIR=$PWD/build LD_LIBRARY_PATH=$PWD/build \
  cargo test --manifest-path bindings/rust/Cargo.toml
```

`ALEPH_CA_LIB_DIR` defaults to `<repo>/build` if unset.

## Example

```rust
use aleph_ca::GameOfLife;

let mut gol = GameOfLife::new(64, 64)?;
gol.set(2, 1, true)?;
gol.set(2, 2, true)?;
gol.set(2, 3, true)?;      // a blinker
gol.run(10)?;
let live = gol.frame()?.iter().filter(|&&v| v != 0).count();
println!("{live} live cells after 10 generations");
# Ok::<(), aleph_ca::Error>(())
```

## Stability

The C ABI is stable within its `_v1` major (`ALEPH_CA_API_VERSION_MAJOR`). The
Rust surface is experimental until it reaches 1.0; expect ergonomic changes.