/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
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

#include <ca-checkpoint-compress.H>

#include <algorithm>
#include <stdexcept>
#include <string>

#include <miniz.h>

#include <ah-errors.H>

namespace Aleph {
namespace CA {
namespace ca_checkpoint_detail {

std::vector<std::uint8_t>
deflate_bytes(const std::uint8_t *src, const std::size_t len, const int level)
{
  if (len == 0)
    return {};

  const int clamped = std::clamp(level, 1, 9);
  mz_ulong dest_cap = mz_compressBound(static_cast<mz_ulong>(len));
  std::vector<std::uint8_t> out(static_cast<std::size_t>(dest_cap));
  const int rc = mz_compress2(out.data(),
                              &dest_cap,
                              src,
                              static_cast<mz_ulong>(len),
                              clamped);
  ah_runtime_error_if(rc != MZ_OK)
    << "deflate_bytes: miniz mz_compress2 failed (rc=" << rc << ")";
  out.resize(static_cast<std::size_t>(dest_cap));
  return out;
}

std::vector<std::uint8_t>
inflate_bytes(const std::uint8_t *src,
              const std::size_t len,
              const std::size_t expected_uncompressed)
{
  std::vector<std::uint8_t> out(expected_uncompressed);
  if (expected_uncompressed == 0)
    return out;
  ah_runtime_error_if(len == 0)
    << "inflate_bytes: empty compressed payload but expected " << expected_uncompressed
    << " bytes of output";

  mz_ulong dest_len = static_cast<mz_ulong>(expected_uncompressed);
  const int rc = mz_uncompress(out.data(),
                               &dest_len,
                               src,
                               static_cast<mz_ulong>(len));
  ah_runtime_error_if(rc != MZ_OK)
    << "inflate_bytes: miniz mz_uncompress failed (rc=" << rc << ")";
  ah_runtime_error_if(dest_len != static_cast<mz_ulong>(expected_uncompressed))
    << "inflate_bytes: size mismatch (expected " << expected_uncompressed
    << ", got " << static_cast<std::size_t>(dest_len) << ")";
  return out;
}

std::vector<std::uint8_t>
inflate_bytes_up_to(const std::uint8_t *src,
                    const std::size_t len,
                    const std::size_t max_uncompressed)
{
  if (max_uncompressed == 0 or len == 0)
    return {};
  std::vector<std::uint8_t> out(max_uncompressed);
  mz_ulong dest_len = static_cast<mz_ulong>(max_uncompressed);
  const int rc = mz_uncompress(out.data(),
                               &dest_len,
                               src,
                               static_cast<mz_ulong>(len));
  ah_runtime_error_if(rc != MZ_OK)
    << "inflate_bytes_up_to: miniz mz_uncompress failed (rc=" << rc << ")";
  ah_runtime_error_if(static_cast<std::size_t>(dest_len) > max_uncompressed)
    << "inflate_bytes_up_to: produced " << static_cast<std::size_t>(dest_len)
    << " bytes, max=" << max_uncompressed;
  out.resize(static_cast<std::size_t>(dest_len));
  return out;
}

}  // namespace ca_checkpoint_detail
}  // namespace CA
}  // namespace Aleph