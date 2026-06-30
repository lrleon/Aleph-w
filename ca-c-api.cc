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

/**
 * @file ca-c-api.cc
 * @brief Phase 25 — implementation of the flat C ABI declared in ca-c-api.h.
 *
 * Each entry point wraps the C++ engine in a try/catch so that no C++
 * exception ever crosses the `extern "C"` boundary; failures are reported
 * through thread-local error state instead.
 */

#define ALEPH_CA_C_API_BUILD 1
#include "ca-c-api.h"

#include <new>
#include <string>

#include <ca-engine-utils.H>
#include <tpl_ca_engine.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_rule.H>
#include <tpl_ca_storage.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace
{

// Thread-local last-error state.
thread_local int g_last_error = ALEPH_CA_OK;
thread_local std::string g_last_message = "";

void set_error(int code, const char *msg) noexcept
{
  g_last_error = code;
  try
    {
      g_last_message = msg;
    }
  catch (...)
    {
      // If even copying the message throws (OOM), leave it empty.
    }
}

void clear_error() noexcept
{
  g_last_error = ALEPH_CA_OK;
  g_last_message.clear();
}

}  // namespace

// The opaque engine handle owns one Game-of-Life engine.
struct aleph_ca_engine
{
  Game_Of_Life_Engine engine;
};

extern "C"
{

uint32_t aleph_ca_api_version_v1(void)
{
  return (static_cast<uint32_t>(ALEPH_CA_API_VERSION_MAJOR) << 16)
         | static_cast<uint32_t>(ALEPH_CA_API_VERSION_MINOR);
}

int aleph_ca_last_error_v1(void)
{
  return g_last_error;
}

const char *aleph_ca_error_message_v1(void)
{
  return g_last_message.c_str();
}

aleph_ca_engine_t *aleph_ca_create_gol_engine_v1(size_t rows, size_t cols)
{
  clear_error();
  if (rows == 0 or cols == 0)
    {
      set_error(ALEPH_CA_ERR_RANGE, "create_gol_engine: rows and cols must be > 0");
      return nullptr;
    }
  try
    {
      auto *handle = new aleph_ca_engine{
          make_gol_engine(static_cast<ca_size_t>(rows),
                          static_cast<ca_size_t>(cols))};
      return handle;
    }
  catch (const std::bad_alloc &)
    {
      set_error(ALEPH_CA_ERR_ALLOC, "create_gol_engine: out of memory");
    }
  catch (const std::exception &e)
    {
      set_error(ALEPH_CA_ERR_INTERNAL, e.what());
    }
  catch (...)
    {
      set_error(ALEPH_CA_ERR_INTERNAL, "create_gol_engine: unknown error");
    }
  return nullptr;
}

void aleph_ca_destroy_engine_v1(aleph_ca_engine_t *engine)
{
  delete engine;  // delete nullptr is a no-op
}

int aleph_ca_extents_v1(const aleph_ca_engine_t *engine, size_t *rows,
                        size_t *cols)
{
  clear_error();
  if (engine == nullptr or rows == nullptr or cols == nullptr)
    {
      set_error(ALEPH_CA_ERR_NULL, "extents: NULL argument");
      return ALEPH_CA_ERR_NULL;
    }
  *rows = static_cast<size_t>(engine->engine.frame().size(0));
  *cols = static_cast<size_t>(engine->engine.frame().size(1));
  return ALEPH_CA_OK;
}

int aleph_ca_set_cell_v1(aleph_ca_engine_t *engine, size_t row, size_t col,
                         int32_t value)
{
  clear_error();
  if (engine == nullptr)
    {
      set_error(ALEPH_CA_ERR_NULL, "set_cell: NULL engine");
      return ALEPH_CA_ERR_NULL;
    }
  auto &frame = engine->engine.write_lattice();
  if (row >= static_cast<size_t>(frame.size(0))
      or col >= static_cast<size_t>(frame.size(1)))
    {
      set_error(ALEPH_CA_ERR_RANGE, "set_cell: coordinate out of range");
      return ALEPH_CA_ERR_RANGE;
    }
  frame.set({static_cast<ca_index_t>(row), static_cast<ca_index_t>(col)},
            value != 0 ? 1 : 0);
  return ALEPH_CA_OK;
}

int aleph_ca_get_cell_v1(const aleph_ca_engine_t *engine, size_t row,
                         size_t col, int32_t *out)
{
  clear_error();
  if (engine == nullptr or out == nullptr)
    {
      set_error(ALEPH_CA_ERR_NULL, "get_cell: NULL argument");
      return ALEPH_CA_ERR_NULL;
    }
  const auto &frame = engine->engine.frame();
  if (row >= static_cast<size_t>(frame.size(0))
      or col >= static_cast<size_t>(frame.size(1)))
    {
      set_error(ALEPH_CA_ERR_RANGE, "get_cell: coordinate out of range");
      return ALEPH_CA_ERR_RANGE;
    }
  *out = static_cast<int32_t>(
      frame.at({static_cast<ca_index_t>(row), static_cast<ca_index_t>(col)}));
  return ALEPH_CA_OK;
}

int aleph_ca_step_v1(aleph_ca_engine_t *engine)
{
  clear_error();
  if (engine == nullptr)
    {
      set_error(ALEPH_CA_ERR_NULL, "step: NULL engine");
      return ALEPH_CA_ERR_NULL;
    }
  try
    {
      engine->engine.step();
      return ALEPH_CA_OK;
    }
  catch (const std::exception &e)
    {
      set_error(ALEPH_CA_ERR_INTERNAL, e.what());
    }
  catch (...)
    {
      set_error(ALEPH_CA_ERR_INTERNAL, "step: unknown error");
    }
  return ALEPH_CA_ERR_INTERNAL;
}

int aleph_ca_run_v1(aleph_ca_engine_t *engine, size_t steps)
{
  clear_error();
  if (engine == nullptr)
    {
      set_error(ALEPH_CA_ERR_NULL, "run: NULL engine");
      return ALEPH_CA_ERR_NULL;
    }
  try
    {
      engine->engine.run(static_cast<std::size_t>(steps));
      return ALEPH_CA_OK;
    }
  catch (const std::exception &e)
    {
      set_error(ALEPH_CA_ERR_INTERNAL, e.what());
    }
  catch (...)
    {
      set_error(ALEPH_CA_ERR_INTERNAL, "run: unknown error");
    }
  return ALEPH_CA_ERR_INTERNAL;
}

int aleph_ca_copy_frame_v1(const aleph_ca_engine_t *engine, int32_t *out,
                           size_t out_len)
{
  clear_error();
  if (engine == nullptr or out == nullptr)
    {
      set_error(ALEPH_CA_ERR_NULL, "copy_frame: NULL argument");
      return ALEPH_CA_ERR_NULL;
    }
  const auto &frame = engine->engine.frame();
  const size_t rows = static_cast<size_t>(frame.size(0));
  const size_t cols = static_cast<size_t>(frame.size(1));
  if (out_len < rows * cols)
    {
      set_error(ALEPH_CA_ERR_RANGE, "copy_frame: out buffer too small");
      return ALEPH_CA_ERR_RANGE;
    }
  size_t k = 0;
  for (size_t i = 0; i < rows; ++i)
    for (size_t j = 0; j < cols; ++j)
      out[k++] = static_cast<int32_t>(
          frame.at({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)}));
  return ALEPH_CA_OK;
}

size_t aleph_ca_steps_run_v1(const aleph_ca_engine_t *engine)
{
  if (engine == nullptr)
    return 0;
  return static_cast<size_t>(engine->engine.steps_run());
}

}  // extern "C"