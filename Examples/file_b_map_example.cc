/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w
*/

#include <filesystem>
#include <iostream>
#include <string>
#include <utility>

#include <tpl_file_b_map.H>

using Aleph::Array;
using Aleph::File_B_Map;

namespace
{
  template <typename K, typename V>
  void print_items(const Array<std::pair<K, V>> & items)
  {
    std::cout << "[";
    for (size_t i = 0; i < items.size(); ++i)
      {
        if (i != 0)
          std::cout << ", ";
        std::cout << "(" << items[i].first << " -> " << items[i].second << ")";
      }
    std::cout << "]";
  }

  struct Cleanup
  {
    std::filesystem::path path;

    ~Cleanup()
    {
      std::error_code ec;
      std::filesystem::remove(path, ec);
      std::filesystem::remove(path.string() + ".lock", ec);
      std::filesystem::remove(path.string() + ".wal", ec);
      std::filesystem::remove(path.string() + ".wal.tmp", ec);
      std::filesystem::remove(path.string() + ".journal", ec);
      std::filesystem::remove(path.string() + ".journal.tmp", ec);
    }
  };
} // namespace

int main()
{
  const auto file_path =
    std::filesystem::temp_directory_path() / "aleph_file_b_map_example.idx";
  Cleanup cleanup{file_path};

  std::cout << "Persistent B-Tree map example\n";
  std::cout << "=============================\n";
  std::cout << "file path     : " << file_path.string() << "\n";

  {
    File_B_Map<int, int> inventory(file_path.string(), false);
    inventory.insert(10, 3);
    inventory.insert(20, 8);
    inventory.insert(30, 5);
    inventory.insert_or_assign(20, 11);
    inventory.sync();

    std::cout << "first session\n";
    std::cout << "-------------\n";
    std::cout << "inventory     : ";
    print_items(inventory.items());
    std::cout << "\n";
    std::cout << "page count    : " << inventory.page_count() << "\n";
  }

  File_B_Map<int, int> reopened(file_path.string(), File_B_Map<int, int>::Read_Only);
  std::cout << "\nafter reopen\n";
  std::cout << "------------\n";
  std::cout << "read only     : " << std::boolalpha << reopened.is_read_only() << "\n";
  std::cout << "inventory     : ";
  print_items(reopened.items());
  std::cout << "\n";
  std::cout << "lower_bound(15): ";
  const auto lb = reopened.lower_bound(15);
  if (lb.has_value())
    std::cout << "(" << lb->first << " -> " << lb->second << ")";
  else
    std::cout << "(none)";
  std::cout << "\n";
  return 0;
}
