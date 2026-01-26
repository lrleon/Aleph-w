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

/**
 * @file map_arena_persistence_example.cc
 * @brief Premium MapArena example: persistence, offsets, and restart.
 *
 * This example focuses on a typical `Aleph::MapArena` use case (defined in
 * `ah-map-arena.H`): building a persistent, append-only log backed by a
 * memory-mapped file that can be recovered after reopening the file.
 *
 * ## What this example demonstrates
 *
 * 1. **Initialization and on-disk layout**:
 *    - We store a small header at the beginning of the file.
 *    - The header's first field is `end` (a `size_t`) holding the logical end
 *      offset (committed bytes).
 *
 * 2. **Persisting the logical end pointer**:
 *    - `MapArena` reads a `size_t` from the beginning of the file on open.
 *    - In this example, we update that value in the header after each append,
 *      so the state can be recovered after reopening.
 *
 * 3. **Record format**:
 *    - Each record is `[uint32_t len][len bytes payload]`.
 *    - This makes it straightforward to iterate and rebuild the log.
 *
 * 4. **Safety around `mremap()`**:
 *    - `reserve()` may trigger `mremap()` and move the region, invalidating
 *      previously returned pointers. Therefore, we use the pattern:
 *      reserve -> write -> commit -> (optional) sync, and we avoid keeping
 *      pointers for long periods.
 *    - For durable references, store **offsets** (integers) and rebuild a
 *      pointer as `base + offset`.
 *
 * ## Build
 *
 * This file is built as part of the examples (see `Examples/CMakeLists.txt`)
 * when `-DBUILD_EXAMPLES=ON`.
 *
 * ## Run
 *
 * The example creates a temporary file in `/tmp` and simulates a "restart" by
 * reopening the file multiple times within the same process.
 *
 * @see ah-map-arena.H
 * @ingroup Examples
 */

#include <ah-map-arena.H>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;
using namespace Aleph;
namespace fs = std::filesystem;

namespace
{

constexpr std::uint32_t kMagic = 0x414C5048U; // "ALPH"
constexpr std::uint32_t kVersion = 1;

struct FileHeader
{
  // Must be the first field: MapArena reads a size_t from file offset 0.
  size_t end;
  std::uint32_t magic;
  std::uint32_t version;
};

static_assert(offsetof(FileHeader, end) == 0);

FileHeader read_header(const MapArena & arena)
{
  FileHeader header{};
  if (!arena.is_initialized() || arena.capacity() < sizeof(FileHeader))
    return header;
  std::memcpy(&header, arena.begin(), sizeof(header));
  return header;
}

void write_header(MapArena & arena, const FileHeader & header)
{
  std::memcpy(arena.begin(), &header, sizeof(header));
}

bool header_is_valid(const FileHeader & header)
{
  return header.magic == kMagic && header.version == kVersion &&
         header.end >= sizeof(FileHeader);
}

void init_or_recover(MapArena & arena)
{
  if (const auto header = read_header(arena); header_is_valid(header))
    return;

  // New file: initialize a clean header.
  //
  // Note: if the file existed but is not in the expected format, `MapArena`
  // will have loaded an arbitrary `end_` (size()) from offset 0. In that case
  // it is not safe to "auto-repair" the file here; it's better to abort and
  // ask the user to delete the file.
  if (not arena.empty())
    throw std::runtime_error("existing file with unknown format; delete it and retry");
  FileHeader fresh{};
  fresh.end = sizeof(FileHeader);
  fresh.magic = kMagic;
  fresh.version = kVersion;

  char * header_write_ptr = arena.reserve(sizeof(FileHeader));
  if (header_write_ptr == nullptr)
    throw std::runtime_error("MapArena: reserve() failed while initializing the header");
  std::memcpy(header_write_ptr, &fresh, sizeof(fresh));
  arena.commit(sizeof(FileHeader));

  // Persist the offset so the next open() can recover end_ correctly.
  arena.sync();
}

// Writes a record as [len][payload] and updates header.end so it can be
// recovered after reopening the file.
void append_record(MapArena & arena, const std::string & payload)
{
  if (payload.size() > std::numeric_limits<std::uint32_t>::max())
    throw std::runtime_error("payload too large");

  const auto len = static_cast<std::uint32_t>(payload.size());

  // 1) Reserve and write len
  {
    char * record_len_write_ptr = arena.reserve(sizeof(len));
    if (record_len_write_ptr == nullptr)
      throw std::runtime_error("MapArena: reserve() failed (len)");
    std::memcpy(record_len_write_ptr, &len, sizeof(len));
    arena.commit(sizeof(len));
  }

  // 2) Reserve and write payload
  if (len > 0)
    {
      char * record_payload_write_ptr = arena.reserve(len);
      if (record_payload_write_ptr == nullptr)
        throw std::runtime_error("MapArena: reserve() failed (payload)");
      std::memcpy(record_payload_write_ptr, payload.data(), len);
      arena.commit(len);
    }

  // 3) Update header.end (persist the logical end pointer)
  auto header = read_header(arena);
  if (!header_is_valid(header))
    throw std::runtime_error("invalid header while appending");
  header.end = arena.size();
  write_header(arena, header);

  // For a demo we sync every append; in production you might batch + MS_ASYNC.
  arena.sync();
}

vector<string> read_all_records(const MapArena & arena)
{
  vector<string> out;

  const auto header = read_header(arena);
  if (!header_is_valid(header))
    return out;

  const char * cursor = arena.begin() + sizeof(FileHeader);
  const char * const end_ptr = arena.begin() + header.end;

  while (cursor < end_ptr)
    {
      if (static_cast<size_t>(end_ptr - cursor) < sizeof(std::uint32_t))
        throw std::runtime_error("truncated file: missing length field");

      std::uint32_t len = 0;
      std::memcpy(&len, cursor, sizeof(len));
      cursor += sizeof(len);

      if (static_cast<size_t>(end_ptr - cursor) < len)
        throw std::runtime_error("truncated file: incomplete payload");

      out.emplace_back(cursor, cursor + len);
      cursor += len;
    }

  return out;
}

void dump_records(const vector<string> & recs, const string & title)
{
  cout << "\n" << title << "\n";
  cout << string(title.size(), '-') << "\n";
  for (size_t index = 0; index < recs.size(); ++index)
    cout << "  [" << index << "] " << recs[index] << "\n";
}

} // namespace

int main()
{
  const string arena_file = "/tmp/aleph_map_arena_persist_demo.dat";
  fs::remove(arena_file);

  cout << "Aleph-w MapArena: persistence example\n";
  cout << "File: " << arena_file << "\n";

  // ----------------------------------------------------------------------------
  // Phase 1: create and append a few records
  // ----------------------------------------------------------------------------
  {
    MapArena arena(arena_file);
    init_or_recover(arena);

    append_record(arena, "hello");
    append_record(arena, "This is persistently in the file1");
    append_record(arena, "record #3");

    auto recs = read_all_records(arena);
    dump_records(recs, "Phase 1: records written");
  }

  // ----------------------------------------------------------------------------
  // Phase 2: "restart" (reopen) and read what was written before
  // ----------------------------------------------------------------------------
  {
    MapArena arena(arena_file);
    init_or_recover(arena);

    auto recs = read_all_records(arena);
    dump_records(recs, "Phase 2: records recovered after reopening");

    append_record(arena, "new record after reopening");
  }

  // ----------------------------------------------------------------------------
  // Phase 3: reopen again and validate there are more records now
  // ----------------------------------------------------------------------------
  {
    MapArena arena(arena_file);
    init_or_recover(arena);

    auto recs = read_all_records(arena);
    dump_records(recs, "Phase 3: records after second reopening");
  }

  fs::remove(arena_file);
  cout << "\nOK\n";
  return 0;
}
