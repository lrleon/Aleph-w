
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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
 * @file test_al_vector.cc
 * @brief Tests for Test Al Vector
 */

/**
 * @file test_al_vector.cc
 * @brief Comprehensive Google Test suite for al-vector.H
 *
 * This test suite covers all major functionality of the Vector class including:
 * - Construction and initialization
 * - Entry access and modification
 * - Arithmetic operations
 * - Comparison operations
 * - Epsilon-based comparisons
 * - Proxy access patterns
 * - Edge cases and error conditions
 * - Iterator support
 * - Move and copy semantics
 */

#include <gtest/gtest.h>
#include <al-vector.H>
#include <al-domain.H>
#include <ah-errors.H>
#include <cmath>

using namespace Aleph;
using namespace testing;

// ============================================================================
// Test Fixture for Vector Tests
// ============================================================================

class VectorTest : public ::testing::Test {
protected:
    AlDomain<int> domain_5;      // Domain with 5 elements {0,1,2,3,4}
    AlDomain<int> domain_3;      // Domain with 3 elements {0,1,2}
    AlDomain<std::string> domain_str;  // String domain

    void SetUp() override {
        // Set up integer domains
        for (int i = 0; i < 5; ++i) {
            domain_5.insert(i);
        }

        for (int i = 0; i < 3; ++i) {
            domain_3.insert(i);
        }

        // Set up string domain
        domain_str.insert("x");
        domain_str.insert("y");
        domain_str.insert("z");
    }
};

// ============================================================================
// Construction and Initialization Tests
// ============================================================================

TEST_F(VectorTest, DefaultConstructor) {
    Vector<int, double> v(domain_5);

    // All entries should be zero by default
    for (int i = 0; i < 5; ++i) {
        EXPECT_DOUBLE_EQ(v.get_entry(i), 0.0);
    }
}

TEST_F(VectorTest, ConstructorWithCustomEpsilon) {
    Vector<int, double> v(domain_5, 1e-10);

    EXPECT_DOUBLE_EQ(v.get_epsilon(), 1e-10);
}

TEST_F(VectorTest, ConstructorFromDynList) {
    DynList<double> values;
    values.append(1.0);
    values.append(2.0);
    values.append(3.0);
    values.append(4.0);
    values.append(5.0);

    Vector<int, double> v(domain_5, values);

    for (int i = 0; i < 5; ++i) {
        EXPECT_DOUBLE_EQ(v.get_entry(i), static_cast<double>(i + 1));
    }
}

TEST_F(VectorTest, ConstructorFromDynListSizeMismatch) {
    DynList<double> values;
    values.append(1.0);
    values.append(2.0);

    // Should throw because domain has 5 elements but list has 2
    EXPECT_THROW((Vector<int, double>(domain_5, values)), std::exception);
}

TEST_F(VectorTest, CopyConstructor) {
    Vector<int, double> v1(domain_5);
    v1.set_entry(0, 1.5);
    v1.set_entry(2, 3.7);

    Vector<int, double> v2(v1);

    EXPECT_DOUBLE_EQ(v2.get_entry(0), 1.5);
    EXPECT_DOUBLE_EQ(v2.get_entry(2), 3.7);
    EXPECT_DOUBLE_EQ(v2.get_entry(1), 0.0);
}

TEST_F(VectorTest, MoveConstructor) {
    Vector<int, double> v1(domain_5);
    v1.set_entry(0, 1.5);
    v1.set_entry(2, 3.7);

    Vector<int, double> v2(std::move(v1));

    EXPECT_DOUBLE_EQ(v2.get_entry(0), 1.5);
    EXPECT_DOUBLE_EQ(v2.get_entry(2), 3.7);
}

TEST_F(VectorTest, CopyAssignment) {
    Vector<int, double> v1(domain_5);
    v1.set_entry(0, 1.5);

    Vector<int, double> v2(domain_5);
    v2 = v1;

    EXPECT_DOUBLE_EQ(v2.get_entry(0), 1.5);
}

TEST_F(VectorTest, CopyAssignmentSelfAssignment) {
    Vector<int, double> v1(domain_5);
    v1.set_entry(0, 1.5);

    v1 = v1;  // Self-assignment

    EXPECT_DOUBLE_EQ(v1.get_entry(0), 1.5);
}

TEST_F(VectorTest, MoveAssignment) {
    Vector<int, double> v1(domain_5);
    v1.set_entry(0, 1.5);

    Vector<int, double> v2(domain_5);
    v2 = std::move(v1);

    EXPECT_DOUBLE_EQ(v2.get_entry(0), 1.5);
}

// ============================================================================
// Entry Access and Modification Tests
// ============================================================================

TEST_F(VectorTest, SetAndGetEntry) {
    Vector<int, double> v(domain_5);

    v.set_entry(0, 1.5);
    v.set_entry(2, -3.2);
    v.set_entry(4, 5.7);

    EXPECT_DOUBLE_EQ(v.get_entry(0), 1.5);
    EXPECT_DOUBLE_EQ(v.get_entry(1), 0.0);
    EXPECT_DOUBLE_EQ(v.get_entry(2), -3.2);
    EXPECT_DOUBLE_EQ(v.get_entry(3), 0.0);
    EXPECT_DOUBLE_EQ(v.get_entry(4), 5.7);
}

TEST_F(VectorTest, SetEntryToZeroRemovesIt) {
    Vector<int, double> v(domain_5);

    v.set_entry(2, 5.0);
    EXPECT_DOUBLE_EQ(v.get_entry(2), 5.0);

    v.set_entry(2, 0.0);
    EXPECT_DOUBLE_EQ(v.get_entry(2), 0.0);

    // Entry should be removed from internal storage
    EXPECT_EQ(v.search_entry(2), nullptr);
}

TEST_F(VectorTest, SetEntriesWithInitializerLists) {
    Vector<int, double> v(domain_5);

    v.set_entries({0, 2, 4}, {1.0, 2.0, 3.0});

    EXPECT_DOUBLE_EQ(v.get_entry(0), 1.0);
    EXPECT_DOUBLE_EQ(v.get_entry(1), 0.0);
    EXPECT_DOUBLE_EQ(v.get_entry(2), 2.0);
    EXPECT_DOUBLE_EQ(v.get_entry(3), 0.0);
    EXPECT_DOUBLE_EQ(v.get_entry(4), 3.0);
}

TEST_F(VectorTest, SetEntriesSizeMismatch) {
    Vector<int, double> v(domain_5);

    EXPECT_THROW(v.set_entries({0, 1}, {1.0, 2.0, 3.0}), std::exception);
}

TEST_F(VectorTest, SearchEntry) {
    Vector<int, double> v(domain_5);

    v.set_entry(2, 3.14);

    double * ptr = v.search_entry(2);
    ASSERT_NE(ptr, nullptr);
    EXPECT_DOUBLE_EQ(*ptr, 3.14);

    double * null_ptr = v.search_entry(1);
    EXPECT_EQ(null_ptr, nullptr);
}

TEST_F(VectorTest, GetEntryConst) {
    Vector<int, double> v(domain_5);
    v.set_entry(2, 3.14);

    const Vector<int, double>& cv = v;
    EXPECT_DOUBLE_EQ(cv.get_entry(2), 3.14);
    EXPECT_DOUBLE_EQ(cv.get_entry(1), 0.0);
}

// ============================================================================
// Epsilon Tests
// ============================================================================

TEST_F(VectorTest, EpsilonDefaultValue) {
    Vector<int, double> v(domain_5);
    EXPECT_DOUBLE_EQ(v.get_epsilon(), 1e-7);
}

TEST_F(VectorTest, SetEpsilon) {
    Vector<int, double> v(domain_5);

    v.set_epsilon(1e-10);
    EXPECT_DOUBLE_EQ(v.get_epsilon(), 1e-10);
}

TEST_F(VectorTest, EpsilonAffectsZeroComparison) {
    Vector<int, double> v(domain_5, 1e-5);

    v.set_entry(0, 1e-6);  // Less than epsilon
    EXPECT_DOUBLE_EQ(v.get_entry(0), 0.0);  // Should be treated as zero

    v.set_entry(1, 1e-4);  // Greater than epsilon
    EXPECT_DOUBLE_EQ(v.get_entry(1), 1e-4);  // Should not be zero
}

TEST_F(VectorTest, AreEqual) {
    Vector<int, double> v(domain_5, 1e-5);

    EXPECT_TRUE(v.are_equal(1.0000001, 1.0000002));  // Within epsilon
    EXPECT_FALSE(v.are_equal(1.0, 1.0001));  // Beyond epsilon
}

// ============================================================================
// Arithmetic Operations Tests
// ============================================================================

TEST_F(VectorTest, VectorAddition) {
    Vector<int, double> v1(domain_5);
    v1.set_entry(0, 1.0);
    v1.set_entry(2, 3.0);

    Vector<int, double> v2(domain_5);
    v2.set_entry(1, 2.0);
    v2.set_entry(2, 4.0);

    Vector<int, double> v3 = v1 + v2;

    EXPECT_DOUBLE_EQ(v3.get_entry(0), 1.0);
    EXPECT_DOUBLE_EQ(v3.get_entry(1), 2.0);
    EXPECT_DOUBLE_EQ(v3.get_entry(2), 7.0);
    EXPECT_DOUBLE_EQ(v3.get_entry(3), 0.0);
}

TEST_F(VectorTest, VectorAdditionInPlace) {
    Vector<int, double> v1(domain_5);
    v1.set_entry(0, 1.0);
    v1.set_entry(2, 3.0);

    Vector<int, double> v2(domain_5);
    v2.set_entry(1, 2.0);
    v2.set_entry(2, 4.0);

    v1 += v2;

    EXPECT_DOUBLE_EQ(v1.get_entry(0), 1.0);
    EXPECT_DOUBLE_EQ(v1.get_entry(1), 2.0);
    EXPECT_DOUBLE_EQ(v1.get_entry(2), 7.0);
}

TEST_F(VectorTest, VectorSubtraction) {
    Vector<int, double> v1(domain_5);
    v1.set_entry(0, 5.0);
    v1.set_entry(2, 3.0);

    Vector<int, double> v2(domain_5);
    v2.set_entry(0, 2.0);
    v2.set_entry(2, 1.0);

    Vector<int, double> v3 = v1 - v2;

    EXPECT_DOUBLE_EQ(v3.get_entry(0), 3.0);
    EXPECT_DOUBLE_EQ(v3.get_entry(2), 2.0);
}

TEST_F(VectorTest, VectorSubtractionInPlace) {
    Vector<int, double> v1(domain_5);
    v1.set_entry(0, 5.0);
    v1.set_entry(2, 3.0);

    Vector<int, double> v2(domain_5);
    v2.set_entry(0, 2.0);
    v2.set_entry(2, 1.0);

    v1 -= v2;

    EXPECT_DOUBLE_EQ(v1.get_entry(0), 3.0);
    EXPECT_DOUBLE_EQ(v1.get_entry(2), 2.0);
}

TEST_F(VectorTest, ScalarMultiplication) {
    Vector<int, double> v(domain_5);
    v.set_entry(0, 2.0);
    v.set_entry(2, 3.0);

    Vector<int, double> v2 = v * 2.5;

    EXPECT_DOUBLE_EQ(v2.get_entry(0), 5.0);
    EXPECT_DOUBLE_EQ(v2.get_entry(2), 7.5);
}

TEST_F(VectorTest, ScalarMultiplicationFromLeft) {
    Vector<int, double> v(domain_5);
    v.set_entry(0, 2.0);
    v.set_entry(2, 3.0);

    Vector<int, double> v2 = 2.5 * v;

    EXPECT_DOUBLE_EQ(v2.get_entry(0), 5.0);
    EXPECT_DOUBLE_EQ(v2.get_entry(2), 7.5);
}

TEST_F(VectorTest, ScalarMultiplicationByZero) {
    Vector<int, double> v(domain_5);
    v.set_entry(0, 2.0);
    v.set_entry(2, 3.0);

    Vector<int, double> v2 = v * 0.0;

    // All entries should be zero
    for (int i = 0; i < 5; ++i) {
        EXPECT_DOUBLE_EQ(v2.get_entry(i), 0.0);
    }
}

TEST_F(VectorTest, ScalarMultiplicationByOne) {
    Vector<int, double> v(domain_5);
    v.set_entry(0, 2.0);
    v.set_entry(2, 3.0);

    Vector<int, double> v2 = v * 1.0;

    EXPECT_DOUBLE_EQ(v2.get_entry(0), 2.0);
    EXPECT_DOUBLE_EQ(v2.get_entry(2), 3.0);
}

TEST_F(VectorTest, ScalarDivision) {
    Vector<int, double> v(domain_5);
    v.set_entry(0, 10.0);
    v.set_entry(2, 5.0);

    Vector<int, double> v2 = v / 2.0;

    EXPECT_DOUBLE_EQ(v2.get_entry(0), 5.0);
    EXPECT_DOUBLE_EQ(v2.get_entry(2), 2.5);
}

TEST_F(VectorTest, ScalarDivisionByZero) {
    Vector<int, double> v(domain_5);
    v.set_entry(0, 10.0);

    EXPECT_THROW(v / 0.0, std::exception);
}

TEST_F(VectorTest, ScalarDivisionByOne) {
    Vector<int, double> v(domain_5);
    v.set_entry(0, 2.0);
    v.set_entry(2, 3.0);

    Vector<int, double> v2 = v / 1.0;

    EXPECT_DOUBLE_EQ(v2.get_entry(0), 2.0);
    EXPECT_DOUBLE_EQ(v2.get_entry(2), 3.0);
}

TEST_F(VectorTest, UnaryMinus) {
    Vector<int, double> v(domain_5);
    v.set_entry(0, 2.0);
    v.set_entry(2, -3.0);

    Vector<int, double> v2 = -v;

    EXPECT_DOUBLE_EQ(v2.get_entry(0), -2.0);
    EXPECT_DOUBLE_EQ(v2.get_entry(2), 3.0);
}

// ============================================================================
// Scalar Product (Dot Product) Tests
// ============================================================================

TEST_F(VectorTest, ScalarProduct) {
    Vector<int, double> v1(domain_5);
    v1.set_entry(0, 1.0);
    v1.set_entry(1, 2.0);
    v1.set_entry(2, 3.0);

    Vector<int, double> v2(domain_5);
    v2.set_entry(0, 4.0);
    v2.set_entry(1, 5.0);
    v2.set_entry(2, 6.0);

    // 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32
    double result = v1.scalar_product(v2);

    EXPECT_DOUBLE_EQ(result, 32.0);
}

TEST_F(VectorTest, ScalarProductOperator) {
    Vector<int, double> v1(domain_5);
    v1.set_entry(0, 1.0);
    v1.set_entry(1, 2.0);

    Vector<int, double> v2(domain_5);
    v2.set_entry(0, 3.0);
    v2.set_entry(1, 4.0);

    // 1*3 + 2*4 = 3 + 8 = 11
    double result = v1 * v2;

    EXPECT_DOUBLE_EQ(result, 11.0);
}

TEST_F(VectorTest, ScalarProductOrthogonalVectors) {
    Vector<int, double> v1(domain_5);
    v1.set_entry(0, 1.0);
    v1.set_entry(1, 0.0);

    Vector<int, double> v2(domain_5);
    v2.set_entry(0, 0.0);
    v2.set_entry(1, 1.0);

    double result = v1 * v2;

    EXPECT_DOUBLE_EQ(result, 0.0);
}

TEST_F(VectorTest, ScalarProductWithZeroVector) {
    Vector<int, double> v1(domain_5);
    v1.set_entry(0, 1.0);
    v1.set_entry(1, 2.0);

    Vector<int, double> v2(domain_5);  // Zero vector

    double result = v1 * v2;

    EXPECT_DOUBLE_EQ(result, 0.0);
}

// ============================================================================
// Comparison Tests
// ============================================================================

TEST_F(VectorTest, EqualityOperator) {
    Vector<int, double> v1(domain_5);
    v1.set_entry(0, 1.0);
    v1.set_entry(2, 3.0);

    Vector<int, double> v2(domain_5);
    v2.set_entry(0, 1.0);
    v2.set_entry(2, 3.0);

    EXPECT_TRUE(v1 == v2);
}

TEST_F(VectorTest, EqualityWithEpsilon) {
    Vector<int, double> v1(domain_5, 1e-5);
    v1.set_entry(0, 1.0);

    Vector<int, double> v2(domain_5, 1e-5);
    v2.set_entry(0, 1.0 + 1e-6);  // Within epsilon

    EXPECT_TRUE(v1 == v2);
}

TEST_F(VectorTest, InequalityOperator) {
    Vector<int, double> v1(domain_5);
    v1.set_entry(0, 1.0);

    Vector<int, double> v2(domain_5);
    v2.set_entry(0, 2.0);

    EXPECT_TRUE(v1 != v2);
}

TEST_F(VectorTest, EqualityZeroVectors) {
    Vector<int, double> v1(domain_5);
    Vector<int, double> v2(domain_5);

    EXPECT_TRUE(v1 == v2);
}

// ============================================================================
// Proxy Access Tests
// ============================================================================

TEST_F(VectorTest, ProxyReadAccess) {
    Vector<int, double> v(domain_5);
    v.set_entry(2, 3.14);

    double val = v[2];
    EXPECT_DOUBLE_EQ(val, 3.14);

    double val_zero = v[1];
    EXPECT_DOUBLE_EQ(val_zero, 0.0);
}

TEST_F(VectorTest, ProxyWriteAccess) {
    Vector<int, double> v(domain_5);

    v[2] = 3.14;

    EXPECT_DOUBLE_EQ(v.get_entry(2), 3.14);
}

TEST_F(VectorTest, ProxyWriteZero) {
    Vector<int, double> v(domain_5);
    v.set_entry(2, 5.0);

    v[2] = 0.0;

    EXPECT_DOUBLE_EQ(v.get_entry(2), 0.0);
    EXPECT_EQ(v.search_entry(2), nullptr);
}

TEST_F(VectorTest, ProxyParenthesesOperator) {
    Vector<int, double> v(domain_5);
    v(2) = 3.14;

    double val = v(2);
    EXPECT_DOUBLE_EQ(val, 3.14);
}

TEST_F(VectorTest, ProxyAssignmentBetweenEntries) {
    Vector<int, double> v(domain_5);
    v[0] = 3.14;
    v[1] = v[0];

    EXPECT_DOUBLE_EQ(v.get_entry(1), 3.14);
}

// ============================================================================
// Iterator Tests
// ============================================================================

TEST_F(VectorTest, IteratorBasic) {
    Vector<int, double> v(domain_5);
    v.set_entry(0, 1.0);
    v.set_entry(2, 3.0);
    v.set_entry(4, 5.0);

    auto it = v.get_it();

    int count = 0;
    while (it.has_curr()) {
        (void)it.get_curr();
        count++;
        it.next();
    }

    EXPECT_EQ(count, 3);  // Only 3 non-zero entries
}

// ============================================================================
// Conversion and Output Tests
// ============================================================================

TEST_F(VectorTest, ToList) {
    Vector<int, double> v(domain_3);
    v.set_entry(0, 1.0);
    v.set_entry(1, 2.0);
    v.set_entry(2, 3.0);

    DynList<double> list = v.to_list();

    EXPECT_EQ(list.size(), 3);
}

TEST_F(VectorTest, ToStr) {
    Vector<int, double> v(domain_3);
    v.set_entry(0, 1.0);
    v.set_entry(1, 2.0);
    v.set_entry(2, 3.0);

    std::string str = v.to_str();

    EXPECT_FALSE(str.empty());
}

// ============================================================================
// String Domain Tests
// ============================================================================

TEST_F(VectorTest, StringDomain) {
    Vector<std::string, double> v(domain_str);

    v.set_entry("x", 1.0);
    v.set_entry("y", 2.0);
    v.set_entry("z", 3.0);

    EXPECT_DOUBLE_EQ(v.get_entry("x"), 1.0);
    EXPECT_DOUBLE_EQ(v.get_entry("y"), 2.0);
    EXPECT_DOUBLE_EQ(v.get_entry("z"), 3.0);
}

TEST_F(VectorTest, StringDomainArithmetic) {
    Vector<std::string, double> v1(domain_str);
    v1.set_entry("x", 1.0);
    v1.set_entry("y", 2.0);

    Vector<std::string, double> v2(domain_str);
    v2.set_entry("x", 3.0);
    v2.set_entry("z", 4.0);

    Vector<std::string, double> v3 = v1 + v2;

    EXPECT_DOUBLE_EQ(v3.get_entry("x"), 4.0);
    EXPECT_DOUBLE_EQ(v3.get_entry("y"), 2.0);
    EXPECT_DOUBLE_EQ(v3.get_entry("z"), 4.0);
}

// ============================================================================
// Edge Cases and Stress Tests
// ============================================================================

TEST_F(VectorTest, VerySmallEpsilon) {
    Vector<int, double> v(domain_5, 1e-15);

    v.set_entry(0, 1e-14);
    EXPECT_DOUBLE_EQ(v.get_entry(0), 1e-14);

    v.set_entry(1, 1e-16);
    EXPECT_DOUBLE_EQ(v.get_entry(1), 0.0);  // Should be treated as zero
}

TEST_F(VectorTest, NegativeValues) {
    Vector<int, double> v(domain_5);
    v.set_entry(0, -1.5);
    v.set_entry(2, -3.7);

    EXPECT_DOUBLE_EQ(v.get_entry(0), -1.5);
    EXPECT_DOUBLE_EQ(v.get_entry(2), -3.7);
}

TEST_F(VectorTest, LargeValues) {
    Vector<int, double> v(domain_5);
    v.set_entry(0, 1e100);
    v.set_entry(2, -1e100);

    EXPECT_DOUBLE_EQ(v.get_entry(0), 1e100);
    EXPECT_DOUBLE_EQ(v.get_entry(2), -1e100);
}

TEST_F(VectorTest, MixedOperations) {
    Vector<int, double> v1(domain_5);
    v1.set_entry(0, 10.0);
    v1.set_entry(1, 20.0);

    Vector<int, double> v2(domain_5);
    v2.set_entry(0, 5.0);
    v2.set_entry(2, 15.0);

    // (v1 + v2) * 2 - v1
    Vector<int, double> result = (v1 + v2) * 2.0 - v1;

    EXPECT_DOUBLE_EQ(result.get_entry(0), 20.0);  // (10+5)*2 - 10 = 30 - 10 = 20
    EXPECT_DOUBLE_EQ(result.get_entry(1), 20.0);  // (20+0)*2 - 20 = 40 - 20 = 20
    EXPECT_DOUBLE_EQ(result.get_entry(2), 30.0);  // (0+15)*2 - 0 = 30
}

TEST_F(VectorTest, ChainedOperations) {
    Vector<int, double> v(domain_5);
    v.set_entry(0, 1.0);

    Vector<int, double> v2(domain_5);
    v2.set_entry(0, 2.0);

    v += v2;
    v = v * 2.0;
    v -= v2;

    EXPECT_DOUBLE_EQ(v.get_entry(0), 4.0);  // (1+2)*2 - 2 = 4
}

// ============================================================================
// Functional Methods Tests (if available)
// ============================================================================

TEST_F(VectorTest, MapAndFilter) {
    Vector<int, double> v(domain_5);
    v.set_entry(0, 1.0);
    v.set_entry(1, 2.0);
    v.set_entry(2, 3.0);
    v.set_entry(3, 4.0);
    v.set_entry(4, 5.0);

    // Test that we can iterate over non-zero entries
    int count = 0;
    auto it = v.get_it();
    while (it.has_curr()) {
        count++;
        it.next();
    }

    EXPECT_EQ(count, 5);
}

// ============================================================================
// Domain Tests
// ============================================================================

TEST_F(VectorTest, GetDomain) {
    Vector<int, double> v(domain_5);

    const auto& d = v.get_domain();
    EXPECT_EQ(d.size(), 5);
}

// ============================================================================
// Float Type Tests
// ============================================================================

TEST(VectorFloatTest, FloatType) {
    AlDomain<int> domain;
    domain.insert(0);
    domain.insert(1);
    domain.insert(2);

    Vector<int, float> v(domain);
    v.set_entry(0, 1.5f);
    v.set_entry(1, 2.5f);

    EXPECT_FLOAT_EQ(v.get_entry(0), 1.5f);
    EXPECT_FLOAT_EQ(v.get_entry(1), 2.5f);
}

TEST(VectorFloatTest, FloatArithmetic) {
    AlDomain<int> domain;
    domain.insert(0);
    domain.insert(1);

    Vector<int, float> v1(domain);
    v1.set_entry(0, 1.5f);

    Vector<int, float> v2(domain);
    v2.set_entry(0, 2.5f);

    Vector<int, float> v3 = v1 + v2;

    EXPECT_FLOAT_EQ(v3.get_entry(0), 4.0f);
}

// ============================================================================
// Comprehensive Integration Test
// ============================================================================

TEST_F(VectorTest, ComprehensiveIntegrationTest) {
    // Create two vectors
    Vector<int, double> v1(domain_5);
    v1.set_entries({0, 1, 2}, {1.0, 2.0, 3.0});

    Vector<int, double> v2(domain_5);
    v2.set_entries({1, 2, 3}, {1.0, 1.0, 1.0});

    // Test addition
    Vector<int, double> sum = v1 + v2;
    EXPECT_DOUBLE_EQ(sum.get_entry(0), 1.0);
    EXPECT_DOUBLE_EQ(sum.get_entry(1), 3.0);
    EXPECT_DOUBLE_EQ(sum.get_entry(2), 4.0);
    EXPECT_DOUBLE_EQ(sum.get_entry(3), 1.0);

    // Test scalar multiplication
    Vector<int, double> scaled = sum * 0.5;
    EXPECT_DOUBLE_EQ(scaled.get_entry(0), 0.5);
    EXPECT_DOUBLE_EQ(scaled.get_entry(1), 1.5);

    // Test scalar product
    double dot = v1 * v2;
    EXPECT_DOUBLE_EQ(dot, 2.0 + 3.0);  // 1*0 + 2*1 + 3*1 + 0*1 = 5

    // Test equality
    Vector<int, double> copy = v1;
    EXPECT_TRUE(copy == v1);

    // Test modification
    copy[0] = 10.0;
    EXPECT_FALSE(copy == v1);
    EXPECT_DOUBLE_EQ(copy.get_entry(0), 10.0);

    // Test negation
    Vector<int, double> neg = -v1;
    EXPECT_DOUBLE_EQ(neg.get_entry(0), -1.0);
    EXPECT_DOUBLE_EQ(neg.get_entry(1), -2.0);
    EXPECT_DOUBLE_EQ(neg.get_entry(2), -3.0);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
