/*
                          Aleph_w

  Data structures & Algorithms
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

/*
 * ca-c-api.h — Phase 25: flat C ABI for the cellular-automata module.
 *
 * A pure C (C99) header exposing a minimal, stable surface so that any
 * language with a C FFI (Rust, Julia, Go, .NET, Python/ctypes, ...) can drive
 * an Aleph-w cellular automaton without touching C++.
 *
 * STABILITY: experimental until v2.1 of the module. Every function carries a
 * `_v1` suffix so a future incompatible revision can coexist (`_v2`).
 *
 * THREADING: error state is thread-local. Distinct engine handles may be used
 * from distinct threads concurrently; a single handle is not thread-safe.
 *
 * OWNERSHIP: the caller owns every handle returned by a `create_*` function
 * and must release it with the matching `destroy` function.
 */

#ifndef ALEPH_CA_C_API_H
#define ALEPH_CA_C_API_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#  if defined(ALEPH_CA_C_API_BUILD)
#    define ALEPH_CA_API __declspec(dllexport)
#  else
#    define ALEPH_CA_API __declspec(dllimport)
#  endif
#else
#  define ALEPH_CA_API __attribute__((visibility("default")))
#endif

/* ABI version. Bump MINOR for additive changes, MAJOR for breaking ones. */
#define ALEPH_CA_API_VERSION_MAJOR 1
#define ALEPH_CA_API_VERSION_MINOR 0

/* Error codes returned by the integer-returning functions. */
typedef enum aleph_ca_status
{
  ALEPH_CA_OK = 0,            /* success */
  ALEPH_CA_ERR_NULL = 1,      /* a required pointer argument was NULL */
  ALEPH_CA_ERR_RANGE = 2,     /* coordinate or size out of range */
  ALEPH_CA_ERR_ALLOC = 3,     /* allocation / construction failed */
  ALEPH_CA_ERR_INTERNAL = 4   /* a C++ exception crossed the boundary */
} aleph_ca_status;

/* Opaque handles. */
typedef struct aleph_ca_engine aleph_ca_engine_t;

/* ------------------------------------------------------------------ */
/* Version & error reporting                                           */
/* ------------------------------------------------------------------ */

/** Return the packed ABI version: (MAJOR << 16) | MINOR. */
ALEPH_CA_API uint32_t aleph_ca_api_version_v1(void);

/** Return the status code of the most recent failing call on this thread. */
ALEPH_CA_API int aleph_ca_last_error_v1(void);

/** Return a human-readable message for the most recent failing call on this
 *  thread. The pointer is valid until the next API call on the same thread. */
ALEPH_CA_API const char *aleph_ca_error_message_v1(void);

/* ------------------------------------------------------------------ */
/* Game of Life engine                                                 */
/* ------------------------------------------------------------------ */

/** Create a Conway Game-of-Life engine on a `rows x cols` toroidal grid,
 *  initially all-dead. Returns NULL on failure (see aleph_ca_last_error_v1). */
ALEPH_CA_API aleph_ca_engine_t *aleph_ca_create_gol_engine_v1(size_t rows,
                                                              size_t cols);

/** Destroy an engine created by a `create_*` function. NULL is a no-op. */
ALEPH_CA_API void aleph_ca_destroy_engine_v1(aleph_ca_engine_t *engine);

/** Write the grid extents into *rows and *cols. */
ALEPH_CA_API int aleph_ca_extents_v1(const aleph_ca_engine_t *engine,
                                     size_t *rows, size_t *cols);

/** Set a single cell. `value` is 0 (dead) or non-zero (alive). */
ALEPH_CA_API int aleph_ca_set_cell_v1(aleph_ca_engine_t *engine, size_t row,
                                      size_t col, int32_t value);

/** Read a single cell into *out. */
ALEPH_CA_API int aleph_ca_get_cell_v1(const aleph_ca_engine_t *engine,
                                      size_t row, size_t col, int32_t *out);

/** Advance one generation. */
ALEPH_CA_API int aleph_ca_step_v1(aleph_ca_engine_t *engine);

/** Advance `steps` generations. */
ALEPH_CA_API int aleph_ca_run_v1(aleph_ca_engine_t *engine, size_t steps);

/** Copy the current frame into `out` (row-major, length `out_len` int32s).
 *  `out_len` must be at least rows*cols. */
ALEPH_CA_API int aleph_ca_copy_frame_v1(const aleph_ca_engine_t *engine,
                                        int32_t *out, size_t out_len);

/** Return the number of completed generations. */
ALEPH_CA_API size_t aleph_ca_steps_run_v1(const aleph_ca_engine_t *engine);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* ALEPH_CA_C_API_H */