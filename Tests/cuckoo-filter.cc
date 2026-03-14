/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library
*/

#include <gtest/gtest.h>
#include <cuckoo-filter.H>
#include <string>
#include <vector>
#include <set>
#include <random>

using namespace Aleph;

// 1. Pruebas de Operaciones Básicas
TEST(CuckooFilterTest, BasicOperations) {
    Cuckoo_Filter<std::string, 12, 4> filter(100);
    
    EXPECT_TRUE(filter.insert("apple"));
    EXPECT_TRUE(filter.insert("banana"));
    EXPECT_TRUE(filter.insert("cherry"));
    
    EXPECT_TRUE(filter.contains("apple"));
    EXPECT_TRUE(filter.contains("banana"));
    EXPECT_TRUE(filter.contains("cherry"));
    EXPECT_FALSE(filter.contains("date"));
    
    EXPECT_EQ(filter.size(), 3);
}

// 2. Pruebas de Eliminación
TEST(CuckooFilterTest, Deletion) {
    Cuckoo_Filter<int, 16, 4> filter(1000);
    
    for (int i = 0; i < 100; ++i) {
        filter.insert(i);
    }
    
    EXPECT_EQ(filter.size(), 100);
    
    for (int i = 0; i < 50; ++i) {
        EXPECT_TRUE(filter.remove(i));
    }
    
    EXPECT_EQ(filter.size(), 50);
    
    for (int i = 0; i < 50; ++i) {
        EXPECT_FALSE(filter.contains(i));
    }
    
    for (int i = 50; i < 100; ++i) {
        EXPECT_TRUE(filter.contains(i));
    }
}

// 3. Prueba de Duplicados
TEST(CuckooFilterTest, Duplicates) {
    Cuckoo_Filter<std::string, 8, 2> filter(100);
    
    EXPECT_TRUE(filter.insert("same"));
    EXPECT_TRUE(filter.insert("same"));
    
    EXPECT_TRUE(filter.contains("same"));
    EXPECT_EQ(filter.size(), 2);
    
    EXPECT_TRUE(filter.remove("same"));
    EXPECT_TRUE(filter.contains("same")); 
    EXPECT_EQ(filter.size(), 1);
}

// 4. Prueba de Filtro Lleno (Saturación)
TEST(CuckooFilterTest, CapacityExhaustion) {
    Cuckoo_Filter<int, 8, 1> filter(4); 
    
    bool full = false;
    for (int i = 0; i < 100; ++i) {
        if (!filter.insert(i)) {
            full = true;
            break;
        }
    }
    EXPECT_TRUE(full); 
}

// 5. Prueba con Diferentes Tipos de Datos
TEST(CuckooFilterTest, TypeVariety) {
    Cuckoo_Filter<double, 12, 4> dbl_filter(100);
    EXPECT_TRUE(dbl_filter.insert(3.14159));
    EXPECT_TRUE(dbl_filter.contains(3.14159));
    EXPECT_FALSE(dbl_filter.contains(2.71828));
}

// 6. Prueba de Configuraciones de Plantilla
TEST(CuckooFilterTest, TemplateConfigurations) {
    Cuckoo_Filter<size_t, 16, 2> filter(1000);
    for (size_t i = 0; i < 500; ++i) EXPECT_TRUE(filter.insert(i));
    for (size_t i = 0; i < 500; ++i) EXPECT_TRUE(filter.contains(i));
}

// 7. Prueba de Estrés
TEST(CuckooFilterTest, StressTest) {
    const size_t num_ops = 1000; // Reducido para evitar saturación excesiva
    Cuckoo_Filter<int, 16, 4> filter(num_ops * 2); // Más margen para evitar fallos de kicking
    std::vector<int> inserted_list;
    
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 1000000);

    for (size_t i = 0; i < num_ops; ++i) {
        int val = dist(rng);
        if (filter.insert(val)) {
            inserted_list.push_back(val);
        }
    }

    // Todos los que reportaron éxito deben estar
    for (int val : inserted_list) {
        EXPECT_TRUE(filter.contains(val)) << "Value " << val << " not found";
    }

    // Eliminación selectiva
    size_t initial_size = inserted_list.size();
    size_t removed_count = 0;
    for (size_t i = 0; i < initial_size / 2; ++i) {
        int val = inserted_list.back();
        inserted_list.pop_back();
        if (filter.remove(val)) {
            removed_count++;
        }
    }

    EXPECT_EQ(filter.size(), initial_size - removed_count);

    // Los restantes deben seguir estando
    for (int val : inserted_list) {
        EXPECT_TRUE(filter.contains(val)) << "Remaining value " << val << " lost";
    }
}

// 8. Alta Carga con Capacidad Alineada
TEST(CuckooFilterTest, HighLoadRefined) {
    // 512 buckets * 4 slots = 2048 capacity
    const size_t capacity = 1800;
    Cuckoo_Filter<size_t, 12, 4> filter(capacity);

    // Record which keys were actually accepted — some may fail if the filter
    // reaches capacity, so we cannot assume insert(i) succeeded for all i < N.
    std::vector<size_t> inserted_keys;
    inserted_keys.reserve(capacity);
    for (size_t i = 0; i < capacity; ++i)
      if (filter.insert(i))
        inserted_keys.push_back(i);

    const double lf = filter.load_factor();
    EXPECT_GT(lf, 0.70);

    for (const size_t key : inserted_keys)
      ASSERT_TRUE(filter.contains(key)) << "Item " << key << " lost at LF " << lf;
}

TEST(CuckooFilterTest, Clear) {
    Cuckoo_Filter<int, 12, 4> filter(100);
    filter.insert(1);
    filter.insert(2);
    filter.clear();
    
    EXPECT_EQ(filter.size(), 0);
    EXPECT_FALSE(filter.contains(1));
    EXPECT_FALSE(filter.contains(2));
}
