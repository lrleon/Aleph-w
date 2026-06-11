# `std::vector` whitelist — CA module

The CA module follows the project convention *"ALWAYS prefer Aleph
containers over STL"* (see `CLAUDE.md`). After the phase 27.5 migration
(`std::vector` → `Aleph::Array`), the linter check is:

```sh
grep -rn "std::vector" ca-*.H tpl_ca_*.H
```

Every hit must be listed here with a reason. Anything not listed is a
convention violation and should be migrated.

## Whitelisted sites

### `tpl_ca_parallel_engine.H` — `std::vector<std::future<void>>`

`std::async` returns `std::future`; the container only stages futures
created and consumed by the standard library inside one function. This
is STL-native interop and stays as `std::vector`.

### `ca-checkpoint-compress.H` / `ca-checkpoint-compress.C`

`deflate_bytes`, `inflate_bytes` and `inflate_bytes_up_to` return
`std::vector<std::uint8_t>`. They form the compiled boundary around the
vendored **miniz** C library (`third_party/miniz`): buffers are sized
with `resize`, filled through raw `mz_compress2`/`mz_uncompress` calls
on `.data()`, and cross a translation-unit boundary (`.C` file). This
is an STL/C interop surface in the sense of the phase 27.5.B
exceptions.

### `ca-checkpoint.H`

All `std::vector<std::uint8_t>` occurrences in this header carry binary
checkpoint payloads into and out of the `ca-checkpoint-compress`
functions above (`build_payload`, `read_raw_payload`,
`snapshot_frame_bytes`, `restore_frame_from_bytes`, delta encode and
decode). Migrating them would force conversions at every call into the
miniz boundary without removing the whitelisted type from the module.
If the compression layer is ever rewritten against `Aleph::Array`,
these sites should migrate with it.

## Non-whitelisted leftovers

None. As of phase 27.5.B the grep above returns only the sites listed
in this document.