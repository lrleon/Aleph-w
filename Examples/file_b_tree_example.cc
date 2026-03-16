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
 * @file file_b_tree_example.cc
 * @brief Illustrative example for Aleph::File_B_Tree
 */

#include <exception>
#include <filesystem>
#include <iostream>
#include <system_error>

#include <ah-errors.H>
#include <tpl_file_b_tree.H>

using namespace Aleph;

namespace
{
  namespace fs = std::filesystem;

  template <typename T>
  void print_array(const char * label, const Array<T> & values)
  {
    std::cout << label << " [";
    for (size_t i = 0; i < values.size(); ++i)
      {
        if (i != 0)
          std::cout << ", ";
        std::cout << values[i];
      }
    std::cout << "]\n";
  }
}

int main()
{
  const fs::path file_path =
    fs::temp_directory_path() / "aleph_file_b_tree_example.idx";

  try
    {
      std::error_code ec;
      fs::remove(file_path, ec);
      fs::remove(file_path.string() + ".wal", ec);
      fs::remove(file_path.string() + ".wal.tmp", ec);
      fs::remove(file_path.string() + ".journal", ec);
      fs::remove(file_path.string() + ".journal.tmp", ec);
      fs::remove(file_path.string() + ".lock", ec);

      std::cout << "Persistent B-Tree example\n";
      std::cout << "=========================\n";
      std::cout << "file path     : " << file_path.string() << "\n";
      std::cout << "sidecars      : " << file_path.string() << ".lock, "
                << file_path.string() << ".wal, "
                << file_path.string() << ".wal.tmp, "
                << file_path.string() << ".journal, "
                << file_path.string() << ".journal.tmp\n\n";

      {
        File_B_Tree<int, Aleph::less<int>, 3> catalog(file_path.string(), false);
        for (int value : {40, 10, 90, 20, 70, 60, 30, 50, 80})
          catalog.insert(value);

        catalog.remove(20);
        catalog.insert(65);
        catalog.sync();

        ah_runtime_error_unless(catalog.verify())
          << "File_B_Tree example produced an invalid in-memory tree";

        std::cout << "first session\n";
        std::cout << "-------------\n";
        print_array("catalog keys  :", catalog.keys());
        std::cout << "height        : " << catalog.height() << "\n";
        std::cout << "page size     : " << catalog.page_size_bytes() << "\n";
        std::cout << "page count    : " << catalog.page_count() << "\n";
        std::cout << "auto sync     : " << std::boolalpha
                  << catalog.auto_sync_enabled() << "\n\n";
      }

      {
        File_B_Tree<int, Aleph::less<int>, 3> reopened(file_path.string());

        std::cout << "after reopen\n";
        std::cout << "------------\n";
        print_array("catalog keys  :", reopened.keys());
        std::cout << "page count    : " << reopened.page_count() << "\n";

        if (const auto lb = reopened.lower_bound(64); lb.has_value())
          std::cout << "lower_bound(64) -> " << *lb << "\n";

        if (const auto ub = reopened.upper_bound(70); ub.has_value())
          std::cout << "upper_bound(70) -> " << *ub << "\n";
      }

      fs::remove(file_path, ec);
      fs::remove(file_path.string() + ".wal", ec);
      fs::remove(file_path.string() + ".wal.tmp", ec);
      fs::remove(file_path.string() + ".journal", ec);
      fs::remove(file_path.string() + ".journal.tmp", ec);
      fs::remove(file_path.string() + ".lock", ec);
    }
  catch (const std::exception & ex)
    {
      std::cerr << "file_b_tree_example failed: " << ex.what() << '\n';
      return 1;
    }

  return 0;
}
