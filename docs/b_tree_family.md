# B-Tree and B+ Tree in Aleph-w

Aleph-w now includes two page-oriented ordered-set containers:

- `Aleph::B_Tree<Key, Compare, MinDegree>`
- `Aleph::BPlus_Tree<Key, Compare, MinDegree>`
- `Aleph::File_B_Tree<Key, Compare, MinDegree, Codec>`
- `Aleph::File_BPlus_Tree<Key, Compare, MinDegree, Codec>`
- `Aleph::File_B_Map<Key, Value, Compare, MinDegree, KeyCodec, ValueCodec>`
- `Aleph::File_BPlus_Map<Key, Value, Compare, MinDegree, KeyCodec, ValueCodec>`

Both are header-only, store unique keys, use `Aleph::less<Key>` by default, and
expose a compact API centered on:

- `insert()`, `remove()`, `contains()`, `search()`
- `lower_bound()`, `upper_bound()`
- `min_key()`, `max_key()`
- `keys()`, `verify()`, `size()`, `height()`, `clear()`
- `get_it()` and `get_range_it()` on the B+ variants

The `File_*` variants add:

- constructor with file path
- `sync()` and `reload()`
- `auto_sync_enabled()` and `set_auto_sync()`
- `file_path()` / `get_file_path()`
- reusable `.lock` sidecars with shared/exclusive advisory locking
- sidecar `.wal` files for page-level redo recovery
- sidecar `.journal` files as a legacy full-image fallback path
- checksummed headers and pages for corruption detection
- current on-disk writes use a fixed-size portable little-endian codec
- custom fixed-size codecs can be supplied explicitly, including bounded
  strings through `Aleph::detail::Paged_Bounded_String_Codec<N>`

The `File_*_Map` variants add:

- unique key/value storage on the same paged persistent backends
- `find()`, `at()`, `insert_or_assign()`, `items()`, and `values()`
- `range()` for the B+ map variant
- `get_it()` and `get_range_it()` on `File_BPlus_Map`

## Which one should you use?

Use `B_Tree` when:

- you want a classic B-Tree where internal nodes also store user keys
- point lookups and updates are the primary workload
- you want a multiway tree but do not need leaf-linked scans

Use `BPlus_Tree` when:

- range scans are a first-class operation
- you want all user keys to live in leaves
- you want sequential access through linked leaves

`BPlus_Tree` also provides `range(first, last)` for inclusive range scans in
`O(log n + k)`.

Use `File_B_Tree` when:

- you want the same ordered-set behavior as `B_Tree`
- you need to say where the data lives on disk at construction time
- you want a real paged B-Tree file whose internal pages also store user keys
- exclusive ownership of the file by one live process is acceptable
- a page cache with explicit `sync()` / `reload()` is enough

Use `File_B_Map` when:

- you want `File_B_Tree` persistence and page layout
- you need mapped values instead of a pure ordered set
- point lookups by key dominate the workload

Use `File_BPlus_Tree` when:

- you want durable range scans with a file path API
- you want a real paged B+ Tree file with linked leaves
- page ids, splits, merges, and page reuse should exist on disk too
- exclusive ownership of the file by one live process is acceptable
- a page cache with explicit `sync()` / `reload()` is enough
- you want lazy ordered scans without materializing an `Array<Key>`

Use `File_BPlus_Map` when:

- you want durable key/value storage with inclusive range scans
- you want the B+ leaf chain to back ordered map traversals
- you want `lower_bound()`, `upper_bound()`, and `range()` over pairs
- you want lazy ordered pair scans through `get_it()` / `get_range_it()`

## Degree parameter

The third template parameter is the minimum degree `MinDegree`:

```cpp
Aleph::B_Tree<int, Aleph::less<int>, 4> bt;
Aleph::BPlus_Tree<int, Aleph::less<int>, 8> bpt;
Aleph::File_B_Tree<int, Aleph::less<int>, 4> fbt("catalog.idx");
Aleph::File_BPlus_Tree<int, Aleph::less<int>, 8> fbpt("ledger.idx");
```

Larger degrees mean:

- wider nodes
- shorter trees
- fewer levels during search
- more data moved during a single split or merge

For tests and examples, small degrees such as `3` or `4` are useful because
they trigger structural changes quickly. For page-oriented real workloads,
higher values are often more appropriate.

## Example

```cpp
#include <tpl_b_tree.H>
#include <tpl_bplus_tree.H>
#include <tpl_file_b_tree.H>
#include <tpl_file_bplus_tree.H>

Aleph::B_Tree<int, Aleph::less<int>, 4> bt = {40, 10, 90, 20, 70, 60, 30};
Aleph::BPlus_Tree<int, Aleph::less<int>, 4> bpt = {
    105, 110, 115, 120, 125, 130, 135, 140
};
Aleph::File_BPlus_Tree<int, Aleph::less<int>, 4> file_index("orders.idx");

bt.insert(50);
auto a = bt.lower_bound(55);      // 60

auto band = bpt.range(115, 130);  // 115, 120, 125, 130
auto b = bpt.upper_bound(130);    // 135

file_index.insert(200);
file_index.insert(220);
file_index.sync();
```

## Persistent wrappers

If what you really need is "where do I specify the file?", use
`File_B_Tree` or `File_BPlus_Tree`:

```cpp
#include <tpl_file_bplus_tree.H>

Aleph::File_BPlus_Tree<int, Aleph::less<int>, 4> index("orders.idx");
index.insert(105);
index.insert(110);
index.insert(115);

// Delay flushing dirty pages until sync().
index.set_auto_sync(false);
index.insert(120);
index.sync();

Aleph::File_BPlus_Tree<int, Aleph::less<int>, 4> reopened("orders.idx");
auto band = reopened.range(108, 120); // 110, 115, 120
```

Important scope note:

- `File_B_Tree` stores a real paged B-Tree file with a header, page ids, and
  a reusable free-list
- `File_BPlus_Tree` stores a real paged B+ Tree file with a header, page ids,
  linked leaves, and a reusable free-list
- reopening either file-backed tree reloads the paged structure from disk
- durable commits use a page-level `.wal` sidecar with a commit trailer and
  payload checksum for current portable version-5 stores
- legacy upgrades can still fall back to the older full-image `.journal` path
- opening the same file twice concurrently is rejected through a `.lock` sidecar
  that combines shared/exclusive advisory OS locks with a pid marker
- current writes use format version 5 with header/page checksums, codec
  metadata, and per-page checkpoint sequences, while readers still accept
  legacy native versions 1 through 4
- `Read_Only` handles are shared-lock snapshots: they reject recovery sidecars
  and throw if asked to mutate or `sync()`
- the shared/exclusive lock semantics are validated both within one process and
  across forked child processes
- current portable writes accept any key/value types that provide fixed-size
  codecs; the bundled codecs cover fixed-size arithmetic types, `std::array`,
  and bounded strings through `Aleph::detail::Paged_Bounded_String_Codec<N>`
- legacy native versions 1 through 4 still require trivially copyable keys to
  reopen because those layouts stored native object bytes directly

## Notes on invariants

- Both trees keep all leaves at the same depth.
- `B_Tree` stores user keys in every node.
- `BPlus_Tree` stores separators in internal nodes and user keys only in leaves.
- `File_B_Tree` stores pages and header metadata in one binary file.
- `File_BPlus_Tree` stores pages, linked leaves, and header metadata in one
  binary file.
- `verify()` is intended as a debugging and test aid to validate structural
  invariants after a sequence of operations.
