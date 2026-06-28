# 05 — Long-running simulations

Simulations that run for hours need to survive crashes and machine reboots.
Aleph-w checkpoints are **crash-safe**, optionally **compressed**, and can be
written **off the hot path**.

## Save and resume

```cpp
#include <ca-checkpoint.H>
#include <ca-engine-utils.H>

using namespace Aleph;
using namespace Aleph::CA;

auto eng = make_gol_engine(256, 256);
// ... seed and run for a while ...
eng.run(1000);

save_checkpoint(eng, "run.ckpt");        // atomic write to disk
```

To resume, build an engine **of the same shape** and load into it:

```cpp
auto resumed = make_gol_engine(256, 256);
Resume_Token token = load_checkpoint_into(resumed, "run.ckpt");

std::cout << "resumed at step " << token.header.step_count << '\n';
resumed.run(1000);                       // continue
```

`load_checkpoint_into` validates the engine type, state size, rank and grid
extents against the file before touching any data, and rejects mismatched or
corrupt checkpoints — it never trusts the header blindly.

## Crash safety

`save_checkpoint` writes to a temporary file, `fsync`s it, and only then
atomically renames it onto the target. A `kill -9` mid-write therefore leaves
any previous checkpoint **intact** rather than truncated. (`sync_dir = true`
also fsyncs the parent directory so the rename itself is durable.)

## Compression

For large or sparse grids, enable DEFLATE compression:

```cpp
Checkpoint_Options opts;
opts.compress = true;       // DEFLATE the frame payload (vendored miniz)
opts.level    = 6;          // 1..9, 0 falls back to 6
save_checkpoint(eng, "run.ckpt", opts);
```

Sparse binary grids typically shrink by 4× or more. The reader auto-detects
compression from the header.

## Periodic checkpoints during a run

`Periodic_Checkpoint_Observer` snapshots every *N* steps. The `{step}`
placeholder in the path is filled with the (zero-padded) generation number:

```cpp
Periodic_Checkpoint_Observer obs(eng, /*every=*/100, "snapshots/gen_{step}.ckpt");
attach_observer(eng, obs);
eng.run(1000);              // writes gen_000000, gen_000100, ... gen_001000
```

## Keeping I/O off the hot path

For high-throughput runs, route writes through an `Async_Checkpoint_Writer` so
`step()` never blocks on disk:

```cpp
Async_Checkpoint_Writer<decltype(eng)> writer;      // dedicated I/O thread

Checkpoint_Options opts;
opts.compress = true;
Periodic_Checkpoint_Observer obs(eng, 100, "snapshots/gen_{step}.ckpt",
                                 writer, opts);
attach_observer(eng, obs);

eng.run(100000);
writer.flush();             // wait for all queued writes before exiting
```

The writer's queue policy (block vs. drop-oldest) is configurable; see
`ca-checkpoint.H`.

## Delta snapshots

When successive frames differ in only a few cells, `save_delta_checkpoint` /
`apply_delta_checkpoint` persist just the changed `(index, value)` pairs
relative to a baseline — much smaller than a full frame.

## Next

[Chapter 06 — Scaling up](06_distributed.md): use every core, and what is on
the distributed roadmap.