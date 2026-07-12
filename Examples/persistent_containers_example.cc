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
 * @file persistent_containers_example.cc
 * @brief Small example for Aleph persistent in-memory containers.
 *
 * Build with the examples target and run:
 *
 * @code
 * ./Examples/persistent_containers_example
 * @endcode
 */

#include <iostream>
#include <string>

#include <tpl_persistent_treap.H>
#include <tpl_persistent_vector.H>

int main()
{
  Aleph::PersistentTreapSet<int> ids;
  auto ids_v1 = ids.insert(10).insert(20);
  auto ids_v2 = ids_v1.insert(15).erase(10);

  std::cout << "ids_v1 contains 10: " << ids_v1.contains(10) << '\n';
  std::cout << "ids_v2 contains 10: " << ids_v2.contains(10) << '\n';

  Aleph::PersistentTreapMap<int, std::string> names;
  auto names_v1 = names.insert(1, std::string("Ada"));
  auto names_v2 = names_v1.insert_or_assign(1, std::string("Ada Lovelace"));

  std::cout << "old name: " << *names_v1.find(1) << '\n';
  std::cout << "new name: " << *names_v2.find(1) << '\n';

  Aleph::PersistentVector<std::string> log;
  auto log_v1 = log.push_back(std::string("created"));
  auto log_v2 = log_v1.push_back(std::string("reviewed"));
  auto log_v3 = log_v2.set(1, std::string("approved"));

  std::cout << "log_v2[1]: " << log_v2.get(1) << '\n';
  std::cout << "log_v3[1]: " << log_v3.get(1) << '\n';

  return ids_v1.contains(10) and not ids_v2.contains(10) and
    *names_v1.find(1) == "Ada" and *names_v2.find(1) == "Ada Lovelace" and
    log_v2.get(1) == "reviewed" and log_v3.get(1) == "approved" ? 0 : 1;
}
