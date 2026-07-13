#include <cassert>
#include <iostream>
#include <string>

#include <tpl_persistent_hash_map.H>

using namespace Aleph;
using namespace std;

int main()
{
  cout << "Aleph-w PersistentHashMap Example" << endl;
  cout << "=================================" << endl;

  // Start with an empty immutable map.
  PersistentHashMap<string, int> m0;

  // Insert a few bindings.
  auto m1 = m0.insert("apple", 10).insert("orange", 20).insert("banana", 30);

  cout << "m1 has " << m1.size() << " bindings." << endl;

  assert(m1.contains("apple"));
  assert(*m1.find("orange") == 20);

  // m0 remains intact.
  assert(m0.is_empty());

  // insert() leaves existing bindings unchanged.
  auto duplicate = m1.insert("apple", 15);
  assert(*duplicate.find("apple") == 10);

  // insert_or_assign() updates a binding in the returned version.
  auto m2 = m1.insert_or_assign("apple", 15).insert("grape", 40);

  cout << "m2 has " << m2.size() << " bindings after update/insert." << endl;

  assert(*m2.find("apple") == 15);

  // m1 was not mutated.
  assert(*m1.find("apple") == 10);
  assert(!m1.contains("grape"));

  auto m3 = m2.erase("orange").erase("banana");

  cout << "m3 has " << m3.size() << " bindings after erasing." << endl;
  assert(!m3.contains("orange"));
  assert(m3.contains("apple"));

  assert(m2.contains("orange"));
  assert(m1.verify());
  assert(m2.verify());
  assert(m3.verify());

  cout << "All persistent structural-sharing invariants are valid." << endl;

  return 0;
}
