# Array storage and queue compatibility notes

These changes address the Array, MemArray, and ArrayQueue defects reported
against Aleph 6.1.0. They incorporate the proposed patches 0001–0003 and extend
them to moved-from containers, constructor overloads, and out-of-source tests.

## Array construction

`Array` no longer accepts a single integer argument. Up to 6.x, `Array<T> a(n)`
meant an empty array with capacity for `n` items, except that a variadic
element constructor could win the overload resolution and build the
one-element array `{n}`, depending on `T` and on the exact argument type.
Readers familiar with `std::vector` expect `n` elements instead. The
constructor is now deleted, so every old use, including `Array<T> a = n;` and
the implicit conversion of an integer argument, fails to compile and must say
what it means:

```cpp
auto reserved = Aleph::Array<int>::create_reserved(10); // size() == 0, capacity() >= 10
Aleph::Array<int> repeated(10, 0);                      // ten zeroes
auto slots = Aleph::Array<int>::create(10);             // ten slots to assign
Aleph::Array<int> item{10};                             // one element whose value is 10
Aleph::Array<int> empty;                                // default capacity (32 slots)
```

Migration of the removed `Array<T> a(n)`:

| Old code | Replacement |
|---|---|
| `Array<T> a(n);` followed by `append()` | `auto a = Array<T>::create_reserved(n);` |
| `Array<T> a(n); a.putn(n);` | `auto a = Array<T>::create(n);` |
| `Array<T> a(n);` followed by `a(i) = ...` | `auto a = Array<T>::create(n);` or `Array<T> a(n, value);` (the old code wrote past `size()`) |
| member `Array<T> m = Array<T>(0);` | `Array<T> m = Array<T>::create_reserved(0);` |

`create(n)` makes the positions logical elements; assign scalar entries before
reading them. Class types retain their default-constructed values. A braced
list, even inside parentheses as in `Array<T>({x})`, is always an element list.

The old variadic item constructors of `Array` were removed because they could
silently reinterpret a capacity as a value. Migrate parenthesized element lists
to braces or to `Array<T>::build(...)`, which every container with the
functional mixin provides; unlike the braces, it moves rvalue items, so it also
stores move-only types (`build_array()` remains). Range construction now requires an input iterator
and a compatible sentinel, so integral count/value arguments cannot be mistaken
for iterators. The iterator-pair constructor that `Special_Ctors` generates for
other containers is constrained the same way: an integer pair never selects it.

## Moved-from storage

Move construction remains `noexcept` and performs no allocation. The source is
empty with zero capacity; it can be traversed, copied, moved again, reserved, or
reused for insertion. Growth from zero capacity starts at `MemArray::Min_Dim`.
`is_valid()` retains its existing meaning of having an allocated buffer and
therefore remains false until storage is allocated again.

Relocation transfers only logical elements and uses a temporary `MemArray` to
own the new allocation. It neither swaps uninitialized scalar slots nor reads
unused slots during contraction. If an element assignment throws, temporary
storage is released. Throwing moves can still leave earlier source elements in
a moved-from state; relocation does not promise a strong exception guarantee.

## Inserting an element of the same container

`put()`, `append()`, `push()` and `insert()` of `MemArray`, `Array`,
`ArrayStack` and `ArrayQueue`, and `insert()` of `DynArray`, accept an element
of the container itself, as in `a.append(a[0])` or `q.put(q.front())`, even
when the container is full and must grow. Before 7.0, growth freed that element
before reading it (a heap-use-after-free), and insertion at the front shifted
it before copying it, so `{10, 20, 30}` followed by `insert(a[1])` gave
`10 10 20 30` instead of `20 10 20 30`.

When a full array grows, the new item is assigned into the new buffer before
any element leaves the old one. A failed allocation or copy therefore leaves
the container unchanged, and a moved argument, as in `a.append(std::move(x))`,
keeps its value if the allocation fails. With free capacity, a front insertion
first copies or moves its argument to a temporary, because the shift
overwrites the slot the argument may occupy. Passing `std::move(a[i])` leaves
element `i` moved from.

## ArrayQueue

Copying and reserving a wrapped queue preserve FIFO order. `empty()`, `clear()`,
and `empty_and_release()` reset both circular indices. The latter may reduce
capacity to four slots, and the next insertion uses the new buffer correctly.
Moved-from queues can also be reused, including through `putn()`.

`ArrayQueue` now inherits from `MemArray` with protected access. Conversion to
`MemArray`, and physical-buffer methods such as `access()`, `get_ptr()`,
`get_dim()`, and `get_ne()`, are no longer public queue operations.

The familiar aliases have explicit queue semantics:

- `first()`, `get_first()`, and `top()` access the oldest item.
- `last()` and `get_last()` access the youngest item.
- `operator[]` and `operator()` index in FIFO order, starting at the oldest.
- `push()` enqueues at the rear; `pop()` and `remove_first()` dequeue the oldest.
- `remove_last()` removes the youngest item; `reverse()` reverses FIFO order.

Use `front(i)`, `rear(i)`, indexing, or iteration to inspect a queue. Do not
assume its logical sequence is a contiguous prefix of its physical allocation.

## Logical bounds and AddressSanitizer

`MemArray::operator()` and `Array::operator()` require `i < size()` and assert
that precondition in Debug. Use `operator[]` for an exception-based range check
in all build modes. A one-past-end pointer must be formed by pointer arithmetic
from a valid base, without first accessing the nonexistent element.

With `ALEPH_USE_SANITIZERS=ON`, CMake enables ASan container annotations by
default. `MemArray` poisons reserved slots beyond the logical size, so invalid
accesses through raw pointers and views are also detected. The circular storage
of `ArrayQueue` opts out of prefix annotations, including after move/reuse.

```bash
cmake -S . -B /tmp/aleph-asan -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DALEPH_USE_SANITIZERS=ON
cmake --build /tmp/aleph-asan --target memarray array arrayqueue arraystack
```

Set `ALEPH_ANNOTATE_CONTAINERS=OFF` in CMake to retain ASan/UBSan without
container annotations. Ordinary builds have neither the annotation calls nor
the extra annotation-state member.

The annotation macro affects layout when ASan is active. All translation units
must agree, including Aleph itself. The `Aleph` target publishes both the macro
and sanitizer compile/link options to its consumers and installed CMake target.
Consumers that bypass the target must apply the same settings manually and
rebuild their dependencies. With annotations enabled, `MemArray::access()` also
cannot read a slot outside the logical size; it remains unchecked otherwise.

## Tests outside the source tree

The planarity test target receives the absolute source directory from CMake.
Its external script checks no longer depend on walking from the process working
directory back into the repository. Direct builds without that definition retain
the existing directory-search fallback.

## Validation performed

The regression checks used fresh CMake/Ninja builds outside the source tree:

- GCC Debug: 1,101 passing tests, including Array/MemArray/ArrayQueue, stacks,
  flat containers, container edge cases, polynomial fitting/interpolation,
  matching, 2D trees, geometry, functional mixins, ranges, and planarity.
- Clang Release with ASan, UBSan, LeakSanitizer, and container annotations:
  600 passing tests. Undefined-behavior recovery was disabled at runtime.
- All 70 planarity tests passed in each build, including the external scripts.
- An installed-package consumer passed using only `Aleph::Aleph` to obtain
  sanitizer options and the annotation definition. It verified annotation
  state, moved-from reuse, and queue insertion after `empty_and_release()`.
- The `testQueue`, `testArrayQueue`, `test-memarray`, `tpl_2dtree_example`,
  `test_2dtree`, and `multi_polynomial_examples` targets compiled successfully.

These were targeted regressions and affected-consumer checks, not a run of the
entire repository suite. The pre-existing disabled performance benchmark was
not enabled. No additional dependencies were introduced.
