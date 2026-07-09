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
 * @file patricia_trie_example.cc
 * @brief Fixed-width unsigned integer lookup with `Aleph::PatriciaSet` and
 *        `Aleph::PatriciaMap`.
 *
 * Usage:
 *   ./patricia_trie_example
 */

#include <print_rule.H>
#include <tpl_patricia_trie.H>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using namespace Aleph;

namespace
{
template <typename UInt>
std::vector<UInt> sorted_keys(const Array<UInt> & keys)
{
  std::vector<UInt> result;
  result.reserve(keys.size());
  for (const auto key : keys)
    result.push_back(key);
  std::sort(result.begin(), result.end());
  return result;
}

void demo_set_membership()
{
  std::cout << "[1] PatriciaSet: integer-key membership\n";
  print_rule();

  PatriciaSet<std::uint32_t> ids;
  for (const auto id : {7U, 42U, 128U, 255U})
    ids.insert(id);

  std::cout << "contains 42: " << std::boolalpha << ids.contains(42)
            << " (expect true)\n";
  std::cout << "contains 99: " << ids.contains(99) << " (expect false)\n";

  ids.erase(128);
  std::cout << "contains 128 after erase: " << ids.contains(128)
            << " (expect false)\n";

  std::cout << "stored ids: ";
  for (const auto id : sorted_keys(ids.keys()))
    std::cout << id << " ";
  std::cout << "\n\n";
}

void demo_map_lookup()
{
  std::cout << "[2] PatriciaMap: integer-key dictionary\n";
  print_rule();

  PatriciaMap<std::uint32_t, std::string> routes;
  routes.insert(10, "default-lan");
  routes.insert(42, "service-net");
  routes.insert_or_assign(255, "broadcast");
  routes.insert_or_assign(42, "service-net-v2");

  if (const auto * value = routes.find(42); value != nullptr)
    std::cout << "route 42: " << *value << "\n";

  std::cout << "contains 255: " << routes.contains(255) << "\n";
  std::cout << "contains 11: " << routes.contains(11) << "\n";

  std::cout << "stored route ids: ";
  for (const auto id : sorted_keys(routes.keys()))
    std::cout << id << " ";
  std::cout << "\n\n";
}

void demo_fixed_width_property()
{
  std::cout << "[3] Fixed-width bitwise keys\n";
  print_rule();

  PatriciaSet<std::uint8_t> bytes;
  bytes.insert(0);
  bytes.insert(1);
  bytes.insert(128);
  bytes.insert(255);

  std::cout << "PatriciaSet<uint8_t>::bit_width = "
            << PatriciaSet<std::uint8_t>::bit_width << "\n";
  std::cout << "byte keys: ";
  for (const auto key : sorted_keys(bytes.keys()))
    std::cout << static_cast<unsigned>(key) << " ";
  std::cout << "\n\n";
}
}  // namespace

int main()
{
  std::cout << "\n=== Aleph::PatriciaSet / PatriciaMap ===\n\n";

  demo_set_membership();
  demo_map_lookup();
  demo_fixed_width_property();

  std::cout << "Done.\n";
  return 0;
}
