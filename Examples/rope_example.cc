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
 * @file rope_example.cc
 * @brief Basic operations, structural sharing, and text-editing usage of
 *        `Aleph::Rope`.
 *
 * Usage:
 *   ./rope_example
 */

#include <print_rule.H>
#include <tpl_rope.H>

#include <iostream>
#include <string>
#include <string_view>

using namespace Aleph;

namespace
{
void demo_basic_usage()
{
  std::cout << "[1] Construction, concat, substr, at, flatten\n";
  print_rule();

  Rope<char> greeting{std::string_view("Hello, ")};
  Rope<char> name{std::string_view("world")};
  Rope<char> punctuation{std::string_view("!")};

  // concat is O(1) plus the occasional rebalance: it wraps existing
  // trees in a new node, it never copies character data.
  Rope<char> message = greeting.concat(name).concat(punctuation);

  std::cout << "message: \"" << message.to_string() << "\"\n";
  std::cout << "size: " << message.size() << "\n";
  std::cout << "char at 7: '" << message.at(7) << "' (expect 'w')\n";

  // substr shares whole subtrees with the source rope where possible;
  // it is typically O(log size()), but ranges that must be rejoined can
  // cost closer to the number of leaves in the extracted range.
  Rope<char> just_name = message.substr(7, 5);
  std::cout << "substr(7, 5): \"" << just_name.to_string() << "\" (expect \"world\")\n\n";
}

void demo_structural_sharing()
{
  std::cout << "[2] Structural sharing: a copy is O(1) and independent\n";
  print_rule();

  Rope<char> original{std::string_view("immutable")};
  Rope<char> copy = original;  // O(1): shares the same tree, no data copied.

  // Deriving a new rope from `copy` never touches `original`'s tree.
  Rope<char> derived = copy.concat(Rope<char>{std::string_view(" and shared")});

  std::cout << "original: \"" << original.to_string() << "\"\n";
  std::cout << "copy:     \"" << copy.to_string() << "\"\n";
  std::cout << "derived:  \"" << derived.to_string() << "\"\n";
  std::cout << "original == copy (same content): " << std::boolalpha
             << (original == copy) << "\n\n";
}

void demo_text_editing()
{
  std::cout << "[3] Text-editing style usage: insert and erase\n";
  print_rule();

  Rope<char> doc{std::string_view("The fox jumps over the dog.")};
  std::cout << "original: \"" << doc.to_string() << "\"\n";

  // insert(pos, other) is substr(0,pos) + other + substr(pos, rest),
  // so it usually touches logarithmically many nodes plus boundary leaves,
  // instead of shifting the whole suffix like std::string::insert.
  Rope<char> with_adjective = doc.insert(4, Rope<char>{std::string_view("quick brown ")});
  std::cout << "after insert: \"" << with_adjective.to_string() << "\"\n";

  // erase(pos, len) removes [pos, pos+len) the same way, via two substr
  // calls plus one concat.
  Rope<char> without_adjective = with_adjective.erase(4, 12);
  std::cout << "after erase:  \"" << without_adjective.to_string()
             << "\" (back to the original text)\n";
  std::cout << "doc unchanged by either edit: \"" << doc.to_string() << "\"\n\n";
}
}  // namespace

int main()
{
  std::cout << "\n=== Aleph::Rope: immutable, structurally-shared string ===\n\n";

  demo_basic_usage();
  demo_structural_sharing();
  demo_text_editing();

  std::cout << "Done.\n";
  return 0;
}
