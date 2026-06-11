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

/** @file ca-checkpoint-compress.H
 *  @brief Internal compress / decompress helpers used by
 *         `ca-checkpoint.H` (Phase 17).
 *
 *  The vendored single-file DEFLATE implementation lives under
 *  `third_party/miniz/`. We keep its C symbols isolated to a dedicated
 *  translation unit (`ca-checkpoint-compress.C`) so downstream users of
 *  `ca-checkpoint.H` do not get the `mz_*` names dumped into their
 *  global namespace. This header exposes only a small, type-safe C++
 *  surface.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef CA_CHECKPOINT_COMPRESS_H
#define CA_CHECKPOINT_COMPRESS_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace Aleph {
namespace CA {
namespace ca_checkpoint_detail {

/** @brief Compress a raw byte buffer with DEFLATE (miniz backend).
 *
 *  @param[in] src pointer to the uncompressed bytes.
 *  @param[in] len number of bytes available at `src`.
 *  @param[in] level DEFLATE level in `[1, 9]`; clamped to that range.
 *  @return owning byte buffer holding the compressed stream.
 *  @throws std::runtime_error when miniz reports an error.
 */
[[nodiscard]] std::vector<std::uint8_t>
deflate_bytes(const std::uint8_t *src, std::size_t len, int level);

/** @brief Decompress a DEFLATE byte buffer back to its raw form.
 *
 *  @param[in] src pointer to the compressed bytes.
 *  @param[in] len number of bytes available at `src`.
 *  @param[in] expected_uncompressed expected size of the decompressed
 *             payload; the function pre-allocates the output buffer to
 *             this size and rejects the input if miniz returns a
 *             different length.
 *  @return owning byte buffer with the decompressed stream.
 *  @throws std::runtime_error on miniz failure or size mismatch.
 */
[[nodiscard]] std::vector<std::uint8_t>
inflate_bytes(const std::uint8_t *src,
              std::size_t len,
              std::size_t expected_uncompressed);

/** @brief Decompress a DEFLATE byte buffer with an upper bound.
 *
 *  Unlike `inflate_bytes`, this overload only requires that the actual
 *  uncompressed size is `<= max_uncompressed`. The returned buffer is
 *  resized to the actual decompressed length. Used by the delta
 *  reader, where the exact uncompressed size is not stored in the
 *  header.
 *
 *  @param[in] src pointer to the compressed bytes.
 *  @param[in] len number of bytes available at `src`.
 *  @param[in] max_uncompressed upper bound on the decompressed size.
 *  @return owning byte buffer trimmed to the actual decompressed length.
 *  @throws std::runtime_error on miniz failure or when the output
 *          would exceed `max_uncompressed`.
 */
[[nodiscard]] std::vector<std::uint8_t>
inflate_bytes_up_to(const std::uint8_t *src,
                    std::size_t len,
                    std::size_t max_uncompressed);

}  // namespace ca_checkpoint_detail
}  // namespace CA
}  // namespace Aleph

#endif  // CA_CHECKPOINT_COMPRESS_H