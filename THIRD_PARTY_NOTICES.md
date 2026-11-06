# Third-Party Notices

This document records every third-party component vendored inside this
repository, the version pulled in, and the license under which it is
redistributed, and the libraries that Aleph-w links. Aleph-w itself
remains MIT (see `LICENSE`); each component below keeps its original
license terms.

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

## gmpfrxx — C++ interface for GMP and MPFR

- Location in this repository: `gmpfrxx.h`, `gmpfrxx.C` (`gmpfrxx.cpp`
  upstream), `mpfr_mul_d.h` and `mpfr_mul_d.c`
- Upstream: https://math.berkeley.edu/~wilken/code/gmpfrxx/, by Jon
  Wilkening: a rewrite of `gmpxx.h` of the GNU MP Library whose
  `mpfr_class`, for MPFR, takes the place of `mpf_class`
- Vendored release: of 2008-11-16 (`gmpfrxx.h` CVS version 1.18), with
  local changes in `gmpfrxx.h`, `gmpfrxx.C` and `mpfr_mul_d.h`
- License: LGPL-3.0-or-later, as gmpfrxx is distributed. See
  `LICENSES/LGPL-3.0-or-later.txt`; the LGPL version 3 incorporates the
  terms of the GNU GPL version 3, https://www.gnu.org/licenses/gpl-3.0.html.
  `gmpfrxx.h` keeps the notice of the `gmpxx.h` it derives from: Copyright
  2001, 2002, 2003, 2006 Free Software Foundation, Inc., under the GNU
  LGPL version 2.1 or any later version.

Used by:

- `point.H`, and so by the geometry of Aleph-w, which includes
  `gmpfrxx.h`. `libAleph` compiles `gmpfrxx.C` (except on Windows) and
  `mpfr_mul_d.c`, so a program that links it statically links them too.

## ran_array — Knuth's random number generator

- Location in this repository: `ran_array.c` and `ran_array.h`
- Upstream: `rng.c` of Donald E. Knuth,
  https://www-cs-faculty.stanford.edu/~knuth/programs/rng.c, explained in
  The Art of Computer Programming, Volume 2, Section 3.6
- Vendored release: one with the changes of the 9th printing of that
  volume (2002), with local changes, among them: the functions and the
  state that `ran_array.h` does not declare are `static`, the macro
  `ran_arr_next()` moves to `ran_array.h`, and the test program, `main()`,
  is left out
- License: Knuth's notice, kept in both files, puts `rng.c` in the public
  domain "as long as you make absolutely no changes". See
  `LICENSES/LicenseRef-Knuth-rng.txt`. This copy has changes.

Used by:

- Nothing of Aleph-w. `libAleph` compiles `ran_array.c` and installs
  `ran_array.h`; since nothing uses them, and this copy has changes,
  `ran_array.h` is deprecated since 7.4.0, and both go in 8.0.0.

## Libraries that Aleph-w links

These are not part of this repository. `Aleph::Aleph` passes them to the
programs that link it, which then link them too:

| Library | License | When |
|---------|---------|------|
| GMP (with its C++ interface, gmpxx) | LGPL-3.0-or-later or GPL-2.0-or-later | Always |
| MPFR | LGPL-3.0-or-later | Always |
| GSL (with gslcblas) | GPL-3.0-or-later | With `ALEPH_WITH_GSL` (ON by default until 8.0.0) |
| libX11 | MIT/X11-style licenses | With `ALEPH_BUILD_X11_VIEWER` (ON by default, except on Windows) |

Aleph-w uses nothing of GSL since 7.4.0: its random numbers, those of
`gsl_rng_mt19937`, come from `Gsl_Compatible_Rng` (`ah-random.H`). With
`ALEPH_WITH_GSL` set to OFF, neither Aleph-w nor the programs that link
it need GSL, and no GPL library is linked.
