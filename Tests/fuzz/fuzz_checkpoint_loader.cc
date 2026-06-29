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
 * @file fuzz_checkpoint_loader.cc
 * @brief libFuzzer target for the checkpoint loader (`load_checkpoint_into`).
 *
 * The loader consumes a binary on-disk format (header + optionally
 * DEFLATE-compressed frame payload via miniz). It is the most security-
 * sensitive parser because it decodes lengths and offsets from untrusted
 * bytes. The fuzz buffer is written to a per-process temp file and loaded
 * into a canonical engine; malformed files must be rejected via exceptions,
 * never via memory corruption.
 */

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

#include <ca-checkpoint.H>
#include <ca-traits.H>
#include <tpl_ca_engine.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_rule.H>
#include <tpl_ca_storage.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace
{

using Grid = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;
using Engine = Synchronous_Engine<Grid, Game_Of_Life_Rule, Moore<2, 1>>;

// Per-process scratch file, created once and overwritten each iteration.
const std::filesystem::path &scratch_path()
{
  static const std::filesystem::path p =
      std::filesystem::temp_directory_path() /
      ("aleph_fuzz_ckpt_" + std::to_string(::getpid()) + ".bin");
  return p;
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  const auto &path = scratch_path();
  {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char *>(data),
              static_cast<std::streamsize>(size));
  }

  try
    {
      Engine engine(Grid({16, 16}, 0), make_game_of_life_rule(), Moore<2, 1>{});
      const Resume_Token token = load_checkpoint_into(engine, path);
      (void) token;
    }
  catch (...)
    {
      // Rejecting a malformed/incompatible checkpoint is correct.
    }

  return 0;
}