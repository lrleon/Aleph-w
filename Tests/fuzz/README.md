# Fuzzing (Phase 26)

libFuzzer targets for the cellular-automata parsers that consume untrusted
input. The goal is to catch memory-safety defects (out-of-bounds, use-after-
free, integer overflow leading to OOB, excessive allocation) that
case-designed unit tests miss.

## Targets

| Target                    | Parser under test                         | Corpus dir            |
| ------------------------- | ----------------------------------------- | --------------------- |
| `fuzz_rle_parser`         | `read_rle` (RLE pattern reader)           | `corpus/rle`          |
| `fuzz_life_parser`        | `read_life_105` / `read_life_106`         | `corpus/life`         |
| `fuzz_csv_reader`         | `read_csv_snapshot<int>`                  | `corpus/csv`          |
| `fuzz_checkpoint_loader`  | `load_checkpoint_into` (binary + miniz)   | `corpus/checkpoint`   |

Malformed input is expected to be **rejected via Aleph error macros**
(exceptions); only sanitizer-detected memory errors are real bugs.

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

libFuzzer writes a reproducer to `crash-<sha1>` (or `artifacts/` in CI). Replay
it deterministically with:

```bash
./build-fuzz/Tests/fuzz/fuzz_checkpoint_loader artifacts/crash-<sha1>
```

## CI

`.github/workflows/fuzz.yml` runs every target weekly (and on demand) for
30 minutes each, in parallel, failing the job on any crash and uploading the
reproducer.

## Findings

- **Checkpoint loader, unbounded allocation (CWE-789).** `read_raw_payload`
  allocated `std::vector<uint8_t>(payload_size)` straight from the header.
  A hostile `payload_size` drove a multi-gigabyte allocation. Fixed by
  bounding `payload_size` against the actual file size and validating
  `cell_count == product(extents)` in `inspect_checkpoint`
  (see `ca-checkpoint.H`; regression tests in
  `Tests/ca_checkpoint_safety_test.cc`).