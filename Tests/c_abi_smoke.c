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
 * c_abi_smoke.c — Phase 25: smoke test for the flat C ABI.
 *
 * Compiled as C99 (no C++), linked against libalephca_c. Drives a Game of
 * Life blinker through the C API and checks it oscillates with period 2,
 * proving the ABI is usable from a pure-C consumer.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "ca-c-api.h"

#define CHECK(cond, msg)                                                     \
  do {                                                                       \
    if (!(cond)) {                                                           \
      fprintf(stderr, "FAIL: %s (line %d): %s\n", msg, __LINE__,            \
              aleph_ca_error_message_v1());                                  \
      return 1;                                                              \
    }                                                                        \
  } while (0)

static int cell(const aleph_ca_engine_t *e, size_t r, size_t c)
{
  int32_t v = -1;
  int rc = aleph_ca_get_cell_v1(e, r, c, &v);
  if (rc != ALEPH_CA_OK)
    return -1;
  return (int) v;
}

int main(void)
{
  /* Version check. */
  uint32_t ver = aleph_ca_api_version_v1();
  CHECK((ver >> 16) == 1u, "major version is 1");

  /* 5x5 toroidal Game of Life. */
  aleph_ca_engine_t *e = aleph_ca_create_gol_engine_v1(5, 5);
  CHECK(e != NULL, "engine created");

  size_t rows = 0, cols = 0;
  CHECK(aleph_ca_extents_v1(e, &rows, &cols) == ALEPH_CA_OK, "extents");
  CHECK(rows == 5 && cols == 5, "extents are 5x5");

  /* Seed a horizontal blinker on row 2. */
  CHECK(aleph_ca_set_cell_v1(e, 2, 1, 1) == ALEPH_CA_OK, "set (2,1)");
  CHECK(aleph_ca_set_cell_v1(e, 2, 2, 1) == ALEPH_CA_OK, "set (2,2)");
  CHECK(aleph_ca_set_cell_v1(e, 2, 3, 1) == ALEPH_CA_OK, "set (2,3)");

  /* One step: the blinker must become vertical on column 2. */
  CHECK(aleph_ca_step_v1(e) == ALEPH_CA_OK, "step 1");
  CHECK(aleph_ca_steps_run_v1(e) == 1, "one step run");
  CHECK(cell(e, 1, 2) == 1 && cell(e, 2, 2) == 1 && cell(e, 3, 2) == 1,
        "vertical blinker after 1 step");
  CHECK(cell(e, 2, 1) == 0 && cell(e, 2, 3) == 0,
        "horizontal arms cleared after 1 step");

  /* Another step: back to horizontal (period 2). */
  CHECK(aleph_ca_step_v1(e) == ALEPH_CA_OK, "step 2");
  CHECK(cell(e, 2, 1) == 1 && cell(e, 2, 2) == 1 && cell(e, 2, 3) == 1,
        "horizontal blinker after 2 steps");

  /* copy_frame round-trip: exactly three live cells. */
  int32_t buf[25];
  CHECK(aleph_ca_copy_frame_v1(e, buf, 25) == ALEPH_CA_OK, "copy_frame");
  int live = 0;
  for (int i = 0; i < 25; ++i)
    live += (buf[i] != 0);
  CHECK(live == 3, "three live cells in frame");

  /* Error path: a too-small buffer must be rejected, not crash. */
  CHECK(aleph_ca_copy_frame_v1(e, buf, 4) == ALEPH_CA_ERR_RANGE,
        "small buffer rejected");
  CHECK(aleph_ca_last_error_v1() == ALEPH_CA_ERR_RANGE, "last_error set");

  /* Error path: out-of-range coordinate. */
  CHECK(aleph_ca_set_cell_v1(e, 99, 0, 1) == ALEPH_CA_ERR_RANGE,
        "out-of-range set rejected");

  aleph_ca_destroy_engine_v1(e);
  aleph_ca_destroy_engine_v1(NULL); /* must be a no-op */

  printf("c_abi_smoke: OK\n");
  return 0;
}