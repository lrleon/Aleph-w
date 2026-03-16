/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library
*/

#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <cuckoo-filter.H>

using namespace std;
using namespace Aleph;

int main()
{
    cout << "Aleph-w Cuckoo Filter Example" << endl;
    cout << "=============================" << endl << endl;

    // Create a Cuckoo Filter for around 1000 items
    // Using 12 bits per fingerprint and 4 entries per bucket (default)
    Cuckoo_Filter<string, 12, 4> filter(1000);

    // 1. Basic insertions
    vector<string> items = {"apple", "banana", "cherry", "date", "elderberry"};
    
    cout << "Inserting items: ";
    for (const auto& item : items) {
        cout << item << " ";
        filter.insert(item);
    }
    cout << endl;

    cout << "Filter size: " << filter.size() << endl;
    cout << "Load factor: " << fixed << setprecision(4) << filter.load_factor() << endl << endl;

    // 2. Lookups
    cout << "Checking membership:" << endl;
    vector<string> queries = {"apple", "banana", "fig", "grape"};
    for (const auto& query : queries) {
        cout << "  - " << setw(10) << query << ": " 
             << (filter.contains(query) ? "LIKELY PRESENT" : "DEFINITELY ABSENT") << endl;
    }
    cout << endl;

    // 3. Deletions
    cout << "Removing 'banana'..." << endl;
    if (filter.remove("banana")) {
        cout << "'banana' was removed successfully." << endl;
    }
    
    cout << "Checking 'banana' again: " 
         << (filter.contains("banana") ? "LIKELY PRESENT" : "DEFINITELY ABSENT") << endl;
    cout << "Filter size after removal: " << filter.size() << endl << endl;

    // 4. Higher load example
    cout << "Filling the filter with more items..." << endl;
    for (int i = 0; i < 800; ++i) {
        filter.insert("item_" + to_string(i));
    }
    
    cout << "Final filter size: " << filter.size() << endl;
    cout << "Final load factor: " << fixed << setprecision(4) << filter.load_factor() << endl;
    
    if (filter.load_factor() > 0.8) {
        cout << "Notice high load factor supported by Cuckoo Filter!" << endl;
    }

    return 0;
}
