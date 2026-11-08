
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
#include <string>
#include <type_traits>
#include <vector>

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
// The proxy of v[k], to_list(), constant vectors and temporary domains
// ============================================================================

namespace
{
  // The values of v, as to_list() gives them
  std::vector<double> values_of(const Vector<int, double> & v)
  {
    std::vector<double> ret;
    v.to_list().for_each([&ret] (double x) { ret.push_back(x); });
    return ret;
  }
}

TEST(VectorProxy, AssigningAZeroEntryClearsTheTarget)
{
  // The assignment of a proxy of a zero entry did nothing
  const IntRange d(3);
  Vector<int, double> v(d);
  v.set_entry(0, 5);
  v[0] = v[1];
  EXPECT_EQ(v.get_entry(0), 0.0);
  EXPECT_EQ(v.search_entry(0), nullptr);

  v.set_entry(2, 4);
  v[1] = v[2];
  EXPECT_EQ(v.get_entry(1), 4.0);
}

TEST(VectorProxy, AssignsAnLvalue)
{
  // The assignment of an lvalue did not compile
  const IntRange d(3);
  Vector<int, double> v(d);
  double x = 3;
  v[1] = x;
  EXPECT_EQ(v.get_entry(1), 3.0);

  const double zero = 0;
  v[1] = zero;
  EXPECT_EQ(v.search_entry(1), nullptr);

  const double tiny = 1e-9; // within the epsilon of the vector
  v[2] = tiny;
  EXPECT_EQ(v.search_entry(2), nullptr);
}

TEST(VectorProxy, KeepsItsOwnKey)
{
  // The proxy pointed to the key, here a temporary that is gone when the
  // proxy is assigned
  const IntRange d(3);
  Vector<int, double> v(d);
  auto p = v[2];
  p = 7.0;
  EXPECT_EQ(v.get_entry(2), 7.0);

  const AlDomain<std::string> words = {"one", "two"};
  Vector<std::string, double> w(words);
  auto q = w[std::string("two")];
  q = 2.0;
  EXPECT_EQ(w.get_entry("two"), 2.0);
}

TEST(VectorProxy, ReadsWhatItWasAssigned)
{
  // The proxy kept the entry that it found when it was made
  const IntRange d(10);
  Vector<int, double> v(d);
  auto p = v[5];
  p = 9.0;
  EXPECT_EQ(static_cast<double>(p), 9.0);

  v[6] = v[7] = 4.0;
  EXPECT_EQ(v.get_entry(6), 4.0);
  EXPECT_EQ(v.get_entry(7), 4.0);
}

TEST(VectorProxy, SurvivesTheGrowthOfTheTable)
{
  // The proxy pointed to the entry, which an insertion that grows the
  // table moves
  const IntRange d(100000);
  Vector<int, double> v(d);
  v.set_entry(0, 1);
  auto p = v[0];
  for (int i = 1; i < 5000; ++i)
    v.set_entry(i, i);
  p = 2.0;
  EXPECT_EQ(v.get_entry(0), 2.0);
  EXPECT_EQ(static_cast<double>(p), 2.0);

  for (int i = 5000; i < 10000; ++i)
    v[i] = v[0];
  for (int i = 5000; i < 10000; ++i)
    ASSERT_EQ(v.get_entry(i), 2.0) << i;
}

TEST(Vector, IndexesAConstantVector)
{
  // v[k] and v(k) of a constant vector did not compile
  const IntRange d(3);
  Vector<int, double> v(d);
  v.set_entry(1, 2);
  const Vector<int, double> & cv = v;
  EXPECT_EQ(cv[1], 2.0);
  EXPECT_EQ(cv(1), 2.0);
  EXPECT_EQ(cv[0], 0.0);
  static_assert(std::is_same_v<decltype(cv[1]), double>);
}

TEST(Vector, ToListFollowsTheKeys)
{
  // to_list() sorted the values
  const IntRange d(3);
  const Vector<int, double> v(d, DynList<double>({3, 1, 2}));
  EXPECT_EQ(values_of(v), (std::vector<double>{3, 1, 2}));

  const IntRange e(5);
  Vector<int, double> w(e);
  w.set_entry(3, -1);
  w.set_entry(0, 8);
  EXPECT_EQ(values_of(w), (std::vector<double>{8, 0, 0, -1, 0}));
}

TEST(Vector, KeepsATemporaryDomain)
{
  // The vector pointed to the temporary domain, gone after the declaration
  Vector<int, double> v(IntRange(10));
  v.set_entry(3, 1.5);
  EXPECT_EQ(v.get_entry(3), 1.5);
  EXPECT_EQ(v.get_domain().size(), 10u);
  EXPECT_EQ(v.get_domain_ptr().use_count(), 1);

  const Vector<int, double> c = v;
  EXPECT_EQ(c.get_domain_ptr(), v.get_domain_ptr());
  EXPECT_EQ(v.get_domain_ptr().use_count(), 2);
  EXPECT_EQ((v + c).get_entry(3), 3.0);

  const Vector<int, double> w(IntRange(3), DynList<double>({1, 2, 3}));
  EXPECT_EQ(values_of(w), (std::vector<double>{1, 2, 3}));

  // A domain that the program keeps stays its own: the vector points to it
  const IntRange d(4);
  const Vector<int, double> u(d);
  EXPECT_EQ(&u.get_domain(), static_cast<const AlDomain<int> *>(&d));
}

TEST(Vector, NeedsADomain)
{
  // Vector(10) turned 10 into an empty temporary domain
  static_assert(not std::is_constructible_v<Vector<int, double>, int>);
  static_assert(not std::is_constructible_v<Vector<int, double>, size_t>);
  static_assert(not std::is_constructible_v<Vector<int, double>, int, double>);
  static_assert(not std::is_constructible_v<Vector<int, double>, int,
                                            const DynList<double> &>);
  static_assert(std::is_constructible_v<Vector<int, double>, IntRange>);
  static_assert(std::is_constructible_v<Vector<int, double>, const IntRange &>);
  static_assert(std::is_constructible_v<Vector<int, double>,
                                        std::shared_ptr<AlDomain<int>>>);
  SUCCEED();
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
