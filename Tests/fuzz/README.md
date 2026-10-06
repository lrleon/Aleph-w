# Fuzzing

libFuzzer targets for parsers, containers and sorting algorithms. They look
for memory-safety defects and compare selected operations with independent
reference models over generated inputs.

## Targets

| Target                   | API under test                           | Corpus dir               |
| ------------------------ | ---------------------------------------- | ------------------------ |
| `fuzz_rle_parser`        | `read_rle` (RLE pattern reader)          | `corpus/rle`             |
| `fuzz_life_parser`       | `read_life_105` / `read_life_106`        | `corpus/life`            |
| `fuzz_csv_reader`        | `read_csv_snapshot<int>`                 | `corpus/csv`             |
| `fuzz_checkpoint_loader` | `load_checkpoint_into` (binary + miniz)  | `corpus/checkpoint`      |
| `fuzz_dynarray`          | `DynArray` mutation sequences            | `corpus/dynarray`        |
| `fuzz_sort`              | `sort`, `in_place_sort`, `stable_argsort` | `corpus/sort`            |
| `fuzz_csv_utility`       | General CSV row reader/writer            | `corpus/csv_utility`     |
| `fuzz_compiler_lexer`    | Compiler lexer and source spans          | `corpus/compiler_lexer`  |
| `fuzz_compiler_parser`   | Compiler parser and error recovery       | `corpus/compiler_parser` |

Malformed parser input is expected to be **rejected via Aleph error macros**
(exceptions). The container, sorting, CSV round-trip and compiler targets also
abort when their checked invariants fail.

## Local battery

From the repository root, run every target for 30 seconds each:

```bash
ruby Tests/run_fuzz.rb
```

To select targets or change the budget:

```bash
ruby Tests/run_fuzz.rb --list
ruby Tests/run_fuzz.rb --seconds 120 --target fuzz_sort --target fuzz_dynarray
```

The script configures a Clang/Ninja build in `build-fuzz-local`, builds only
the selected targets, and runs them sequentially. It copies the committed seeds
into a persistent working corpus under `build-fuzz-local/Tests/fuzz/corpus/`.
New inputs and crash artifacts remain under that ignored build directory, so
normal local runs do not change the committed corpus. Use `--build-dir DIR` to
choose another build directory. A nonzero exit indicates a build or fuzzing
failure; the script prints the artifact directory for a failed target. The
newer targets have bounded input lengths to keep each iteration practical.

Prerequisites: Ruby, CMake, Ninja, Clang with libFuzzer/ASan/UBSan runtimes,
and the repository's normal GMP, MPFR and GSL development packages. GoogleTest
is not needed: the local runner disables GoogleTest downloads, and when
GoogleTest is absent CMake warns and configures only the fuzz targets. On
Ubuntu/Debian, install missing packages with:

```bash
sudo apt-get install ruby cmake ninja-build clang libclang-rt-dev \
  libgmp-dev libmpfr-dev libgsl-dev
```

## Building & running locally

Fuzzers are opt-in and Clang-only (`ALEPH_BUILD_FUZZERS`):

```bash
cmake -S . -B build-fuzz -G Ninja \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DBUILD_TESTS=ON -DALEPH_BUILD_FUZZERS=ON

cmake --build build-fuzz --target fuzz_rle_parser

# Run for 60 seconds, seeded with the committed corpus.
./build-fuzz/Tests/fuzz/fuzz_rle_parser -max_total_time=60 Tests/fuzz/corpus/rle
```

Each target is compiled with `-fsanitize=fuzzer,address,undefined` and
`-fno-sanitize-recover=undefined`. The header-only parsers are instrumented
directly into the fuzzer translation unit; the checkpoint loader additionally
exercises the bundled `miniz` through ASan's global allocator interceptor.

## Reproducing a crash

The local runner writes reproducers under
`build-fuzz-local/Tests/fuzz/artifacts/<target>/`. Replay one with its target:

```bash
./build-fuzz-local/Tests/fuzz/fuzz_compiler_parser \
  build-fuzz-local/Tests/fuzz/artifacts/fuzz_compiler_parser/crash-<sha1>
```

Manual libFuzzer runs without `-artifact_prefix` write the reproducer in the
current directory.

## CI

`.github/workflows/fuzz.yml` runs all nine targets weekly (and on demand) for
30 minutes each, in parallel, failing the job on any crash and uploading the
reproducer. The five newer targets use the same `-max_len` bounds as the local
runner.

## Findings

- **Checkpoint loader, unbounded allocation (CWE-789).** `read_raw_payload`
  allocated `std::vector<uint8_t>(payload_size)` straight from the header.
  A hostile `payload_size` drove a multi-gigabyte allocation. Fixed by
  bounding `payload_size` against the actual file size and validating
  `cell_count == product(extents)` in `inspect_checkpoint`
  (see `ca-checkpoint.H`; regression tests in
  `Tests/ca_checkpoint_safety_test.cc`).
