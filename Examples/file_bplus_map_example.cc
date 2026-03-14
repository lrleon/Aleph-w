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

#include <tpl_file_bplus_map.H>

using Aleph::Array;
using Aleph::File_BPlus_Map;

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

  template <typename Map>
  void print_iter_items(const Map & map, const int first, const int last)
  {
    std::cout << "[";
    bool first_item = true;
    for (auto it = map.get_range_it(first, last); it.has_curr(); it.next_ne())
      {
        if (not first_item)
          std::cout << ", ";
        const auto item = it.get_curr();
        std::cout << "(" << item.first << " -> " << item.second << ")";
        first_item = false;
      }
    std::cout << "]";
  }
} // namespace

int main()
{
  const auto file_path =
    std::filesystem::temp_directory_path() / "aleph_file_bplus_map_example.idx";
  Cleanup cleanup{file_path};

  std::cout << "Persistent B+ Tree map example\n";
  std::cout << "==============================\n";
  std::cout << "file path     : " << file_path.string() << "\n";

  {
    File_BPlus_Map<int, int> prices(file_path.string(), false);
    prices.insert(101, 750);
    prices.insert(105, 920);
    prices.insert(110, 860);
    prices.insert(120, 990);
    prices.insert_or_assign(110, 875);
    prices.sync();

    std::cout << "first session\n";
    std::cout << "-------------\n";
    std::cout << "all items     : ";
    print_items(prices.items());
    std::cout << "\n";
    std::cout << "range 104..115: ";
    print_items(prices.range(104, 115));
    std::cout << "\n";
    std::cout << "iter 104..115 : ";
    print_iter_items(prices, 104, 115);
    std::cout << "\n";
  }

  File_BPlus_Map<int, int> reopened(file_path.string(),
                                    File_BPlus_Map<int, int>::Read_Only);
  std::cout << "\nafter reopen\n";
  std::cout << "------------\n";
  std::cout << "read only     : " << std::boolalpha << reopened.is_read_only() << "\n";
  std::cout << "all items     : ";
  print_items(reopened.items());
  std::cout << "\n";
  std::cout << "iter 104..115 : ";
  print_iter_items(reopened, 104, 115);
  std::cout << "\n";
  std::cout << "upper_bound(110): ";
  const auto ub = reopened.upper_bound(110);
  if (ub.has_value())
    std::cout << "(" << ub->first << " -> " << ub->second << ")";
  else
    std::cout << "(none)";
  std::cout << "\n";
  return 0;
}
