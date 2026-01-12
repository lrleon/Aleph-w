/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

/**
 * @file map_arena_example.C
 * @brief Example demonstrating MapArena memory-mapped file allocator.
 *
 * This example shows how to use MapArena for memory allocation backed
 * by memory-mapped files.
 *
 * ## Features Demonstrated
 *
 * 1. Basic arena allocation (reserve/commit)
 * 2. Arena growth (automatic remapping)
 * 3. Iterating over allocated memory
 * 4. Storing structured data
 * 5. Move semantics
 *
 * ## Use Cases
 *
 * - Large data processing with mmap
 * - Bump allocator pattern
 * - Memory-efficient temporary storage
 *
 * @note The current MapArena implementation does not persist the allocation
 *       offset (`end_`) to disk automatically. For true persistence across
 *       program runs, you would need to store metadata separately or extend
 *       the class.
 *
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>
#include <filesystem>

#include <ah-map-arena.H>

using namespace std;
using namespace Aleph;
namespace fs = std::filesystem;

// ============================================================================
// Helper functions
// ============================================================================

void print_header(const string& title)
{
  cout << "\n";
  cout << "+" << string(70, '-') << "+" << endl;
  cout << "| " << left << setw(68) << title << " |" << endl;
  cout << "+" << string(70, '-') << "+" << endl;
}

void print_subheader(const string& subtitle)
{
  cout << "\n  " << subtitle << endl;
  cout << "  " << string(subtitle.length(), '-') << endl;
}

void print_arena_status(const MapArena& arena, const string& label)
{
  cout << "  " << label << ":" << endl;
  cout << "    Size (committed): " << arena.size() << " bytes" << endl;
  cout << "    Capacity (mapped): " << arena.capacity() << " bytes" << endl;
  cout << "    Available: " << arena.avail() << " bytes" << endl;
  cout << "    Is empty: " << (arena.empty() ? "yes" : "no") << endl;
}

// ============================================================================
// Example 1: Basic Arena Operations
// ============================================================================

void demo_basic_operations()
{
  print_header("Example 1: Basic Arena Operations");
  
  const string arena_file = "/tmp/aleph_arena_basic.dat";
  
  // Always start fresh
  fs::remove(arena_file);
  
  MapArena arena(arena_file);
  
  print_subheader("Initial state");
  print_arena_status(arena, "New arena");
  
  print_subheader("Reserve and commit memory");
  
  // Reserve space for a string
  const char* message = "Hola desde Colombia!";
  size_t msg_len = strlen(message) + 1;
  
  char* ptr = arena.reserve(msg_len);
  if (ptr) {
    memcpy(ptr, message, msg_len);
    arena.commit(msg_len);
    cout << "  Stored: \"" << message << "\" (" << msg_len << " bytes)" << endl;
  }
  
  // Store more data
  const char* cities[] = {"Bogota", "Medellin", "Cali", "Barranquilla"};
  for (const char* city : cities) {
    size_t len = strlen(city) + 1;
    char* p = arena.reserve(len);
    if (p) {
      memcpy(p, city, len);
      arena.commit(len);
    }
  }
  
  print_arena_status(arena, "After storing data");
  
  print_subheader("Reading stored strings");
  
  cout << "  Stored strings:" << endl;
  char* read_ptr = arena.begin();
  while (read_ptr < arena.end()) {
    cout << "    -> \"" << read_ptr << "\"" << endl;
    read_ptr += strlen(read_ptr) + 1;
  }
  
  // Sync to ensure persistence
  arena.sync();
  cout << "\n  Data synced to disk" << endl;
  
  // Cleanup
  fs::remove(arena_file);
}

// ============================================================================
// Example 2: Storing Structured Data
// ============================================================================

struct CityRecord {
  char name[32];
  int population;    // thousands
  double area;       // km²
  int altitude;      // meters
};

void demo_structured_data()
{
  print_header("Example 2: Storing Structured Data");
  
  const string arena_file = "/tmp/aleph_arena_struct.dat";
  fs::remove(arena_file);
  
  MapArena arena(arena_file);
  
  print_subheader("Store city records");
  
  // Colombian city data
  CityRecord cities[] = {
    {"Bogota", 8281, 1775.98, 2640},
    {"Medellin", 2569, 380.64, 1495},
    {"Cali", 2228, 564.33, 1018},
    {"Barranquilla", 1274, 154.00, 18},
    {"Cartagena", 1047, 609.10, 2}
  };
  
  size_t count = sizeof(cities) / sizeof(cities[0]);
  
  // Store the count first
  char* ptr = arena.reserve(sizeof(size_t));
  memcpy(ptr, &count, sizeof(size_t));
  arena.commit(sizeof(size_t));
  
  // Store each record
  for (const auto& city : cities) {
    ptr = arena.reserve(sizeof(CityRecord));
    memcpy(ptr, &city, sizeof(CityRecord));
    arena.commit(sizeof(CityRecord));
  }
  
  print_arena_status(arena, "After storing records");
  
  print_subheader("Read and display records");
  
  // Read count
  char* read_ptr = arena.begin();
  size_t stored_count;
  memcpy(&stored_count, read_ptr, sizeof(size_t));
  read_ptr += sizeof(size_t);
  
  cout << "\n  Retrieved " << stored_count << " city records:" << endl;
  cout << "  " << string(60, '-') << endl;
  cout << "  " << left << setw(15) << "City" 
       << right << setw(10) << "Pop (k)"
       << setw(12) << "Area (km2)"
       << setw(12) << "Alt (m)" << endl;
  cout << "  " << string(60, '-') << endl;
  
  for (size_t i = 0; i < stored_count; ++i) {
    CityRecord rec;
    memcpy(&rec, read_ptr, sizeof(CityRecord));
    read_ptr += sizeof(CityRecord);
    
    cout << "  " << left << setw(15) << rec.name
         << right << setw(10) << rec.population
         << setw(12) << fixed << setprecision(2) << rec.area
         << setw(12) << rec.altitude << endl;
  }
  
  fs::remove(arena_file);
}

// ============================================================================
// Example 3: Arena Growth
// ============================================================================

void demo_arena_growth()
{
  print_header("Example 3: Arena Growth (Automatic Remapping)");
  
  const string arena_file = "/tmp/aleph_arena_growth.dat";
  fs::remove(arena_file);
  
  MapArena arena(arena_file);
  
  cout << "\n  Initial capacity: " << arena.capacity() << " bytes" << endl;
  cout << "  (Initial region size: " << MapArena::initial_rgn_size << " bytes)" << endl;
  
  print_subheader("Allocate beyond initial capacity");
  
  // Keep track of capacity changes
  size_t prev_capacity = arena.capacity();
  int growth_count = 0;
  
  // Allocate chunks until we've grown several times
  const size_t chunk_size = 1000;
  
  for (int i = 0; i < 20; ++i) {
    char* ptr = arena.reserve(chunk_size);
    if (ptr) {
      // Fill with pattern
      memset(ptr, 'A' + (i % 26), chunk_size);
      arena.commit(chunk_size);
      
      if (arena.capacity() > prev_capacity) {
        growth_count++;
        cout << "  Growth #" << growth_count 
             << ": " << prev_capacity << " -> " << arena.capacity() 
             << " bytes (after " << arena.size() << " allocated)" << endl;
        prev_capacity = arena.capacity();
      }
    }
  }
  
  print_subheader("Final state");
  print_arena_status(arena, "Arena after growth");
  
  // Verify data integrity
  cout << "\n  Verifying data integrity..." << endl;
  char* ptr = arena.begin();
  bool valid = true;
  for (int i = 0; i < 20 && valid; ++i) {
    char expected = 'A' + (i % 26);
    for (size_t j = 0; j < chunk_size && valid; ++j) {
      if (ptr[j] != expected) {
        valid = false;
        cout << "  ERROR: Data corruption at chunk " << i << endl;
      }
    }
    ptr += chunk_size;
  }
  if (valid)
    cout << "  All data verified successfully!" << endl;
  
  fs::remove(arena_file);
}

// ============================================================================
// Example 4: Log Buffer Pattern
// ============================================================================

void demo_log_buffer()
{
  print_header("Example 4: Log Buffer Pattern");
  
  const string arena_file = "/tmp/aleph_arena_log.dat";
  fs::remove(arena_file);
  
  MapArena arena(arena_file);
  
  print_subheader("Write log entries");
  
  // Simulate log entries with timestamps
  const char* log_entries[] = {
    "[2024-01-15 08:00:00] Sistema iniciado en Bogota",
    "[2024-01-15 08:00:01] Conexion con servidor Medellin",
    "[2024-01-15 08:00:02] Usuario: Juan Perez",
    "[2024-01-15 08:00:05] Transaccion #1001: $150,000 COP",
    "[2024-01-15 08:00:07] Sincronizacion con Cali",
    "[2024-01-15 08:00:10] Backup iniciado",
    "[2024-01-15 08:00:15] Backup completado",
    "[2024-01-15 08:00:20] Alerta: Memoria al 75%"
  };
  
  size_t num_entries = sizeof(log_entries) / sizeof(log_entries[0]);
  
  for (const char* entry : log_entries) {
    size_t len = strlen(entry) + 1;
    char* ptr = arena.reserve(len);
    if (ptr) {
      memcpy(ptr, entry, len);
      arena.commit(len);
    }
  }
  
  cout << "  Written " << num_entries << " log entries" << endl;
  print_arena_status(arena, "Log buffer");
  
  print_subheader("Read log entries");
  
  cout << "\n  Log contents:" << endl;
  cout << "  " << string(55, '-') << endl;
  
  char* ptr = arena.begin();
  int count = 0;
  while (ptr < arena.end()) {
    cout << "  " << ptr << endl;
    ptr += strlen(ptr) + 1;
    count++;
  }
  
  cout << "  " << string(55, '-') << endl;
  cout << "  Total entries: " << count << endl;
  
  fs::remove(arena_file);
}

// ============================================================================
// Example 5: Move Semantics
// ============================================================================

void demo_move_semantics()
{
  print_header("Example 5: Move Semantics");
  
  const string arena_file = "/tmp/aleph_arena_move.dat";
  fs::remove(arena_file);
  
  print_subheader("Create and populate arena");
  
  MapArena arena1(arena_file);
  
  const char* data = "Datos importantes de Colombia";
  char* ptr = arena1.reserve(strlen(data) + 1);
  memcpy(ptr, data, strlen(data) + 1);
  arena1.commit(strlen(data) + 1);
  
  cout << "  arena1 size: " << arena1.size() << endl;
  cout << "  arena1 is_initialized: " << arena1.is_initialized() << endl;
  
  print_subheader("Move construction");
  
  MapArena arena2(std::move(arena1));
  
  cout << "  After move construction:" << endl;
  cout << "    arena1 is_initialized: " << arena1.is_initialized() << endl;
  cout << "    arena2 is_initialized: " << arena2.is_initialized() << endl;
  cout << "    arena2 size: " << arena2.size() << endl;
  cout << "    arena2 data: \"" << arena2.begin() << "\"" << endl;
  
  print_subheader("Move assignment");
  
  MapArena arena3;
  cout << "  arena3 (before): is_initialized = " << arena3.is_initialized() << endl;
  
  arena3 = std::move(arena2);
  
  cout << "  After move assignment:" << endl;
  cout << "    arena2 is_initialized: " << arena2.is_initialized() << endl;
  cout << "    arena3 is_initialized: " << arena3.is_initialized() << endl;
  cout << "    arena3 data: \"" << arena3.begin() << "\"" << endl;
  
  fs::remove(arena_file);
}

// ============================================================================
// Example 6: Memory Statistics
// ============================================================================

void demo_memory_stats()
{
  print_header("Example 6: Memory Statistics and Efficiency");
  
  const string arena_file = "/tmp/aleph_arena_stats.dat";
  fs::remove(arena_file);
  
  MapArena arena(arena_file);
  
  print_subheader("Arena efficiency analysis");
  
  // Simulate various allocation sizes
  int allocations[] = {10, 50, 100, 500, 1000, 2000};
  
  cout << "\n  Allocation pattern analysis:" << endl;
  cout << "  " << string(50, '-') << endl;
  cout << "  " << setw(10) << "Alloc Size" 
       << setw(12) << "Committed"
       << setw(12) << "Capacity"
       << setw(15) << "Utilization" << endl;
  cout << "  " << string(50, '-') << endl;
  
  for (int size : allocations) {
    char* ptr = arena.reserve(size);
    if (ptr) {
      memset(ptr, 'X', size);
      arena.commit(size);
      
      double utilization = 100.0 * arena.size() / arena.capacity();
      
      cout << "  " << setw(10) << size 
           << setw(12) << arena.size()
           << setw(12) << arena.capacity()
           << setw(14) << fixed << setprecision(1) << utilization << "%" << endl;
    }
  }
  
  print_subheader("Debug output");
  cout << arena << endl;
  
  fs::remove(arena_file);
}

// ============================================================================
// Main
// ============================================================================

int main()
{
  cout << "\n";
  cout << "========================================================================" << endl;
  cout << "        ALEPH-W MAP ARENA EXAMPLE" << endl;
  cout << "        Memory-Mapped File Arena Allocator" << endl;
  cout << "========================================================================" << endl;
  
  demo_basic_operations();
  demo_structured_data();
  demo_arena_growth();
  demo_log_buffer();
  demo_move_semantics();
  demo_memory_stats();
  
  cout << "\n";
  cout << "========================================================================" << endl;
  cout << "                    Example completed successfully!" << endl;
  cout << "========================================================================" << endl;
  cout << endl;
  
  return 0;
}
