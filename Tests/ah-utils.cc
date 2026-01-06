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
 * @file ah-utils.cc
 * @brief Tests for Ah Utils
 */

#include <gtest/gtest.h>
#include "../ahUtils.H"
#include <vector>
#include <fstream>
#include <cstdio>

// Tests for is_even() and is_odd()
TEST(AhUtilsTest, EvenOdd) {
    EXPECT_TRUE(Aleph::is_even(0));
    EXPECT_FALSE(Aleph::is_odd(0));
    EXPECT_TRUE(Aleph::is_even(2));
    EXPECT_FALSE(Aleph::is_odd(2));
    EXPECT_FALSE(Aleph::is_even(1));
    EXPECT_TRUE(Aleph::is_odd(1));
    EXPECT_TRUE(Aleph::is_even(-4));
    EXPECT_FALSE(Aleph::is_odd(-4));
    EXPECT_FALSE(Aleph::is_even(-5));
    EXPECT_TRUE(Aleph::is_odd(-5));
}

// Tests for nibble_to_char() and char_to_nibble()
TEST(AhUtilsTest, NibbleConversion) {
    for (int i = 0; i < 16; ++i) {
        char c = Aleph::nibble_to_char(i);
        EXPECT_EQ(i, Aleph::char_to_nibble(c));
    }
    EXPECT_EQ(Aleph::nibble_to_char(10), 'A');
    EXPECT_EQ(Aleph::nibble_to_char(15), 'F');
    EXPECT_EQ(Aleph::char_to_nibble('9'), 9);
    EXPECT_EQ(Aleph::char_to_nibble('B'), 11);

    // Assertion test (only in debug)
    #ifndef NDEBUG
    EXPECT_DEATH(Aleph::nibble_to_char(16), "");
    EXPECT_DEATH(Aleph::char_to_nibble('G'), "");
    #endif
}

// Tests for is_power_of_2() and next_power_of_2()
TEST(AhUtilsTest, PowerOf2) {
    EXPECT_TRUE(Aleph::is_power_of_2(1));
    EXPECT_TRUE(Aleph::is_power_of_2(2));
    EXPECT_TRUE(Aleph::is_power_of_2(16));
    EXPECT_TRUE(Aleph::is_power_of_2(1024));
    EXPECT_FALSE(Aleph::is_power_of_2(0));
    EXPECT_FALSE(Aleph::is_power_of_2(3));
    EXPECT_FALSE(Aleph::is_power_of_2(15));

    EXPECT_EQ(Aleph::next_power_of_2(1), 1);
    EXPECT_EQ(Aleph::next_power_of_2(2), 2);
    EXPECT_EQ(Aleph::next_power_of_2(3), 4);
    EXPECT_EQ(Aleph::next_power_of_2(7), 8);
    EXPECT_EQ(Aleph::next_power_of_2(16), 16);
    EXPECT_EQ(Aleph::next_power_of_2(17), 32);
}

// Tests for median()
TEST(AhUtilsTest, Median) {
    Aleph::less<int> cmp;
    EXPECT_EQ(*Aleph::median(1, 2, 3, cmp), 2);
    EXPECT_EQ(*Aleph::median(3, 1, 2, cmp), 2);
    EXPECT_EQ(*Aleph::median(2, 3, 1, cmp), 2);
    EXPECT_EQ(*Aleph::median(1, 1, 2, cmp), 1);
    EXPECT_EQ(*Aleph::median(2, 1, 1, cmp), 1);
    EXPECT_EQ(*Aleph::median(1, 2, 1, cmp), 1);
    EXPECT_EQ(*Aleph::median(5, 5, 5, cmp), 5);

    // Test with temporary comparator (thanks to refactoring)
    EXPECT_EQ(*Aleph::median(10, 20, 15, Aleph::less<int>()), 15);
}

// Tests for interpolate() and extrapolate_*()
TEST(AhUtilsTest, Interpolation) {
    EXPECT_DOUBLE_EQ(Aleph::interpolate(0, 10, 0, 100, 5), 50.0);
    EXPECT_DOUBLE_EQ(Aleph::interpolate(1, 2, 10, 20, 1.5), 15.0);
    EXPECT_DOUBLE_EQ(Aleph::extrapolate_left(1, 2, 10, 20, 0), 0.0);
    EXPECT_DOUBLE_EQ(Aleph::extrapolate_right(1, 2, 10, 20, 3), 30.0);

    EXPECT_DOUBLE_EQ(Aleph::pow2(3.0), 9.0);
    EXPECT_DOUBLE_EQ(Aleph::pow3(2.0), 8.0);
}

// Tests for are_near()
TEST(AhUtilsTest, AreNear) {
    EXPECT_TRUE(Aleph::are_near(1.0, 1.000001, 1e-5));
    EXPECT_FALSE(Aleph::are_near(1.0, 1.0001, 1e-5));
    EXPECT_TRUE(Aleph::are_near(-1.0, -1.000001, 1e-5));
    EXPECT_FALSE(Aleph::are_near(-1.0, -1.0001, 1e-5));
}

// Tests for u_index and l_index
TEST(AhUtilsTest, Indices) {
    EXPECT_EQ(Aleph::u_index(10), 5);
    EXPECT_EQ(Aleph::u_index(11), 5);
    EXPECT_EQ(Aleph::l_index(5), 10);
}

// Tests for demangle and CLASSNAME_TO_STRING
struct MyTestStruct {};
TEST(AhUtilsTest, Demangle) {
    std::string name = Aleph::demangle(typeid(int).name());
    EXPECT_EQ(name, "int");

    MyTestStruct s;
    std::string className = CLASSNAME_TO_STRING(&s);
    // The exact name may vary depending on the compiler/namespace, but it must contain MyTestStruct
    EXPECT_TRUE(className.find("MyTestStruct") != std::string::npos);
}

// Tests for exists_file
TEST(AhUtilsTest, ExistsFile) {
    std::string filename = "test_exists_file.tmp";
    std::ofstream outfile(filename);
    outfile << "test";
    outfile.close();

    EXPECT_TRUE(Aleph::exists_file(filename));
    std::remove(filename.c_str());
    EXPECT_FALSE(Aleph::exists_file(filename));
}

// Tests for next_value, prev_value, is_normal_number
TEST(AhUtilsTest, FloatUtils) {
    double val = 1.0;
    EXPECT_GT(Aleph::next_value(val), val);
    EXPECT_LT(Aleph::prev_value(val), val);

    EXPECT_TRUE(Aleph::is_normal_number(1.0));
    EXPECT_TRUE(Aleph::is_normal_number(0.0));
    EXPECT_TRUE(Aleph::is_normal_number(-0.0));

    double inf = std::numeric_limits<double>::infinity();
    double nan = std::numeric_limits<double>::quiet_NaN();

    EXPECT_FALSE(Aleph::is_normal_number(inf));
    EXPECT_FALSE(Aleph::is_normal_number(nan));
}

// Tests for error_msg
TEST(AhUtilsTest, ErrorMsg) {
    EXPECT_DEATH(Aleph::error_msg("Test Error"), "Test Error");
}

// Mock container to test Rvector
struct MockContainer {
    std::vector<int> data;

    MockContainer(std::initializer_list<int> l) : data(l) {}

    const int& get_first() const { return data.front(); }
    const int& get_last() const { return data.back(); }

    struct Iterator {
        std::vector<int>::const_iterator it;
        std::vector<int>::const_iterator end;

        Iterator(const MockContainer& c) : it(c.data.begin()), end(c.data.end()) {}

        bool has_curr() const { return it != end; }
        const int& get_curr() const { return *it; }
        void next() { ++it; }
    };

    Iterator get_it() const { return Iterator(*this); }
    // For the second overload of Rvector that uses get_it(1)
    Iterator get_it(int) const { return Iterator(*this); }
};

TEST(AhUtilsTest, Rvector) {
    MockContainer c{1, 2, 3};

    std::string r1 = Aleph::Rvector("vec", c);
    EXPECT_EQ(r1, "vec <- c(1, 2, 3)");

    // Note: The second overload of Rvector uses c.get_first() as the name
    // and c.get_it(1) to iterate. In our mock, get_first returns int,
    // so the name will be "1".
    std::string r2 = Aleph::Rvector(c);
    EXPECT_EQ(r2, "1 <- c(1, 2, 3)");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
