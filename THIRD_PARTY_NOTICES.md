# Third-Party Notices

This document records every third-party component vendored inside this
repository, the version pulled in, and the license under which it is
redistributed. Aleph-w itself remains MIT (see `LICENSE.md`); each
component below keeps its original license terms.

## miniz — DEFLATE/zip implementation

- Location in this repository: `third_party/miniz/`
- Upstream: https://github.com/richgel999/miniz
- Vendored release: **3.0.2** (2023-01-15)
- License: MIT (zlib-compatible). See
  `third_party/miniz/LICENSE`.

Used by:

- `ca-checkpoint.H` (Phase 17 — crash-safe checkpoints with optional
  DEFLATE compression of frame payloads). The C symbols are confined to
  the `aleph_miniz` static target and consumed exclusively by
  `ca-checkpoint-compress.C`; downstream users of `ca-checkpoint.H` do
  not see any `mz_*` symbols.

Copyright notice (reproduced from `third_party/miniz/LICENSE`):

```text
Copyright 2013-2014 RAD Game Tools and Valve Software
Copyright 2010-2014 Rich Geldreich and Tenacious Software LLC

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
```
