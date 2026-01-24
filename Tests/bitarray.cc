
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
 * @file bitarray.cc
 * @brief Tests for Bitarray
 */

#include <gtest/gtest.h>

#include <bitArray.H>

#include <cstdint>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

using namespace Aleph;
using namespace std;

namespace {

struct SavedBitArray
{
  size_t num_bytes = 0;
  size_t num_bits = 0;
  vector<int> bytes;
};

SavedBitArray save_and_parse(const BitArray &arr)
{
  ostringstream out;
  arr.save(out);

  istringstream in(out.str());
  SavedBitArray parsed;
  in >> parsed.num_bytes >> parsed.num_bits;
  parsed.bytes.reserve(parsed.num_bytes);
  for (size_t i = 0; i < parsed.num_bytes; ++i)
    {
      int value = 0;
      in >> value;
      parsed.bytes.push_back(value);
    }
  return parsed;
}

string bits_to_string(const vector<unsigned int> &bits)
{
  string out;
  out.reserve(bits.size());
  for (auto bit : bits)
    out.push_back(bit ? '1' : '0');
  return out;
}

template <typename U>
string to_bit_string(U value, const size_t num_bits)
{
  string out;
  out.reserve(num_bits);
  for (size_t i = 0; i < num_bits; ++i)
    {
      const size_t shift = num_bits - i - 1;
      const bool bit = ((value >> shift) & U(1)) != 0;
      out.push_back(bit ? '1' : '0');
    }
  return out;
}

template <typename T>
string expected_set_num_bits(const T value)
{
  using U = std::make_unsigned_t<T>;
  const U u = static_cast<U>(value);
  return to_bit_string(u, sizeof(T) * 8);
}

vector<unsigned int> random_bits(mt19937 &rng, const size_t n)
{
  bernoulli_distribution dist(0.5);
  vector<unsigned int> bits;
  bits.reserve(n);
  for (size_t i = 0; i < n; ++i)
    bits.push_back(dist(rng) ? 1u : 0u);
  return bits;
}

BitArray from_bits(const vector<unsigned int> &bits)
{
  BitArray arr(bits.size());
  for (size_t i = 0; i < bits.size(); ++i)
    if (bits[i])
      arr.write_bit(i, 1);
  return arr;
}

vector<unsigned int> or_bits(const vector<unsigned int> &a,
                             const vector<unsigned int> &b)
{
  const size_t n = max(a.size(), b.size());
  vector<unsigned int> out(n, 0);
  for (size_t i = 0; i < n; ++i)
    {
      const unsigned int lhs = i < a.size() ? a[i] : 0u;
      const unsigned int rhs = i < b.size() ? b[i] : 0u;
      out[i] = lhs | rhs;
    }
  return out;
}

vector<unsigned int> and_bits(const vector<unsigned int> &a,
                              const vector<unsigned int> &b)
{
  const size_t n = min(a.size(), b.size());
  vector<unsigned int> out(n, 0);
  for (size_t i = 0; i < n; ++i)
    out[i] = a[i] & b[i];
  return out;
}

TEST(BitArrayBasics, construction_size_and_defaults)
{
  BitArray empty;
  EXPECT_EQ(empty.size(), 0u);
  EXPECT_THROW((void) empty.read_bit(0), std::out_of_range);

  BitArray a(10);
  EXPECT_EQ(a.size(), 10u);
  for (size_t i = 0; i < a.size(); ++i)
    EXPECT_EQ(a.read_bit(i), 0);

  BitArray ones(10, 1);
  EXPECT_EQ(ones.size(), 10u);
  EXPECT_EQ(ones.count_ones(), 10);
  EXPECT_EQ(ones.count_zeros(), 0);
  EXPECT_EQ(ones.get_bit_str(), string(10, '1'));
}

TEST(ByteBasics, read_write_and_int_roundtrip)
{
  static_assert(sizeof(Byte) == 1);

  Byte b;
  EXPECT_EQ(b.get_int(), 0);
  EXPECT_EQ(b.count_ones(), 0);
  EXPECT_EQ(b.count_zeros(), 8);

  b.write_bit(0, 1);
  b.write_bit(2, 1);
  EXPECT_EQ(b.get_int(), 0b00000101);
  EXPECT_EQ(b.read_bit(0), 1u);
  EXPECT_EQ(b.read_bit(1), 0u);
  EXPECT_EQ(b.read_bit(2), 1u);

  b.set_int(0xA5); // 1010'0101
  EXPECT_EQ(b.get_int(), 0xA5);
  EXPECT_EQ(b.read_bit(0), 1u);
  EXPECT_EQ(b.read_bit(2), 1u);
  EXPECT_EQ(b.read_bit(5), 1u);
  EXPECT_EQ(b.read_bit(7), 1u);
  EXPECT_EQ(b.count_ones(), 4);
  EXPECT_EQ(b.count_zeros(), 4);
}

TEST(BitArrayProxy, writes_extend_size_and_copy_from_sparse_proxy)
{
  BitArray a;
  a[0] = 1;
  EXPECT_EQ(a.size(), 1u);
  EXPECT_EQ(a.read_bit(0), 1);

  a[10] = 1;
  EXPECT_EQ(a.size(), 11u);
  EXPECT_EQ(a.read_bit(0), 1);
  EXPECT_EQ(a.read_bit(9), 0);
  EXPECT_EQ(a.read_bit(10), 1);

  BitArray sparse(40'000); // sized, but blocks not allocated
  BitArray b;
  EXPECT_NO_THROW(b[0] = sparse[0]);
  EXPECT_EQ(b.size(), 1u);
  EXPECT_EQ(b.read_bit(0), 0);

  const BitArray c(1);
  EXPECT_THROW((void) c[1], std::out_of_range);
}

TEST(BitArraySizing, set_size_computes_byte_count_correctly)
{
  BitArray a;

  a.set_size(1);
  {
    const auto saved = save_and_parse(a);
    EXPECT_EQ(saved.num_bytes, 1u);
    EXPECT_EQ(saved.num_bits, 1u);
  }

  a.set_size(16);
  {
    const auto saved = save_and_parse(a);
    EXPECT_EQ(saved.num_bytes, 2u);
    EXPECT_EQ(saved.num_bits, 16u);
  }

  a.set_size(17);
  {
    const auto saved = save_and_parse(a);
    EXPECT_EQ(saved.num_bytes, 3u);
    EXPECT_EQ(saved.num_bits, 17u);
  }
}

TEST(BitArrayAccess, reserve_enables_fast_read_write)
{
  BitArray a;
  a.reserve(9);
  EXPECT_EQ(a.size(), 9u);

  a.write(8, 1);
  EXPECT_EQ(a.read(8), 1);
  EXPECT_EQ(a.read_bit(8), 1);
}

TEST(BitArrayAccess, reading_unallocated_bytes_returns_zero)
{
  BitArray a;
  a.write_bit(40'000, 1);
  EXPECT_EQ(a.size(), 40'001u);
  EXPECT_EQ(a.read_bit(0), 0);
  EXPECT_EQ(a.read_bit(1), 0);
  EXPECT_EQ(a.read_bit(39'999), 0);
  EXPECT_EQ(a.read_bit(40'000), 1);
}

TEST(BitArrayStackOps, push_pop_and_empty)
{
  BitArray a;
  EXPECT_THROW(a.pop(), std::underflow_error);

  a.push(1);
  a.push(0);
  a.push(1);
  EXPECT_EQ(a.size(), 3u);
  EXPECT_EQ(a.get_bit_str(), "101");

  a.pop();
  EXPECT_EQ(a.size(), 2u);
  EXPECT_EQ(a.get_bit_str(), "10");
  EXPECT_THROW((void) a.read_bit(2), std::out_of_range);

  a.empty();
  EXPECT_EQ(a.size(), 0u);
}

TEST(BitArrayPersistence, save_load_roundtrip)
{
  BitArray a;
  a.set_bit_str("10110011");
  a.write_bit(40'000, 1); // force sparse blocks

  ostringstream out;
  a.save(out);

  istringstream in(out.str());
  BitArray b;
  b.load(in);

  EXPECT_TRUE(a == b);
  EXPECT_EQ(b.size(), a.size());
  EXPECT_EQ(b.read_bit(40'000), 1);
}

TEST(BitArrayPersistence, load_from_array_of_chars_roundtrip)
{
  BitArray a;
  a.set_bit_str("10110011");

  const auto saved = save_and_parse(a);
  vector<unsigned char> bytes;
  bytes.reserve(saved.num_bytes);
  for (int value : saved.bytes)
    bytes.push_back(static_cast<unsigned char>(value));

  BitArray b;
  b.load_from_array_of_chars(bytes.data(), saved.num_bits);
  EXPECT_TRUE(a == b);
}

TEST(BitArrayShifts, fixed_and_dynamic_shifts)
{
  BitArray a;
  a.set_bit_str("10110011");

  BitArray left = a;
  left.left_shift(2);
  EXPECT_EQ(left.get_bit_str(), "11001100");

  BitArray right = a;
  right.right_shift(2);
  EXPECT_EQ(right.get_bit_str(), "00101100");

  BitArray dyn = a;
  dyn.dyn_left_shift(2);
  EXPECT_EQ(dyn.get_bit_str(), "1011001100");
  dyn.dyn_right_shift(2);
  EXPECT_EQ(dyn.get_bit_str(), a.get_bit_str());

  BitArray too_much = a;
  too_much.dyn_right_shift(1'000);
  EXPECT_EQ(too_much.size(), 1u);
  EXPECT_EQ(too_much.get_bit_str(), "0");
}

TEST(BitArrayShifts, circular_shifts)
{
  BitArray a;
  a.set_bit_str("10110011");

  BitArray left = a;
  left.circular_left_shift(10); // 10 % 8 == 2
  EXPECT_EQ(left.get_bit_str(), "11001110");

  BitArray right = a;
  right.circular_right_shift(10); // 10 % 8 == 2
  EXPECT_EQ(right.get_bit_str(), "11101100");

  BitArray empty;
  EXPECT_NO_THROW(empty.circular_left_shift(3));
  EXPECT_NO_THROW(empty.circular_right_shift(3));
}

TEST(BitArrayBitwiseOps, or_and_with_different_sizes)
{
  BitArray a;
  a.set_bit_str("10101");

  BitArray b;
  b.set_bit_str("00110011");

  BitArray c = a | b;
  EXPECT_EQ(c.get_bit_str(), "10111011");

  BitArray d = a & b;
  EXPECT_EQ(d.get_bit_str(), "00100");
}

TEST(BitArrayBitwiseOps, or_does_not_leak_rhs_unused_bits_after_shrink)
{
  BitArray rhs;
  rhs.set_size(16);
  for (size_t i = 9; i < 16; ++i)
    rhs.write_bit(i, 1);
  rhs.set_size(9); // last byte still exists, but bits 9..15 are out of range

  BitArray lhs;
  lhs.set_size(16);
  lhs |= rhs;

  for (size_t i = 9; i < 16; ++i)
    EXPECT_EQ(lhs.read_bit(i), 0);
}

TEST(BitArrayBitwiseOps, or_masks_rhs_unused_bits_from_load)
{
  // num_bits is not multiple of 8; extra bits in last byte must not leak.
  const unsigned char bytes[] = {0x00, 0xFF};
  BitArray rhs;
  rhs.load_from_array_of_chars(bytes, 9);

  BitArray lhs;
  lhs.set_size(16);
  lhs |= rhs;

  EXPECT_EQ(lhs.read_bit(8), 1);
  for (size_t i = 9; i < 16; ++i)
    EXPECT_EQ(lhs.read_bit(i), 0);
}

TEST(BitArraySizing, shrink_then_write_bit_growth_keeps_new_bits_zeroed)
{
  BitArray a;
  a.set_size(16);
  a.write_bit(8, 1);
  for (size_t i = 9; i < 16; ++i)
    a.write_bit(i, 1);

  a.set_size(9);
  EXPECT_EQ(a.read_bit(8), 1);

  // Extend without set_size(), so intermediate bits must already be cleared.
  a.write_bit(15, 0);
  EXPECT_EQ(a.size(), 16u);
  EXPECT_EQ(a.read_bit(8), 1);
  for (size_t i = 9; i < 15; ++i)
    EXPECT_EQ(a.read_bit(i), 0);
}

TEST(BitArrayFunctional, iterator_and_traverse)
{
  BitArray a;
  a.set_bit_str("101");

  auto it = a.get_it();
  ASSERT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_curr(), 1u);
  it.next();
  EXPECT_EQ(it.get_curr(), 0u);
  it.next();
  EXPECT_EQ(it.get_curr(), 1u);
  it.next();
  EXPECT_FALSE(it.has_curr());
  EXPECT_EQ(it.get_curr_ne(), 0u);

  struct StopOnOne
  {
    int visited = 0;
    bool operator()(int bit)
    {
      ++visited;
      return bit == 0;
    }
  };

  BitArray b;
  b.set_bit_str("000100");
  StopOnOne op;
  const bool completed = b.traverse(op);
  EXPECT_FALSE(completed);
  EXPECT_EQ(op.visited, 4);
}

TEST(BitArrayIterator, unbound_iterator_corner_cases)
{
  BitArray::Iterator it;
  EXPECT_FALSE(it.has_curr());
  EXPECT_EQ(it.get_curr_ne(), 0u);
  EXPECT_THROW((void) it.get_curr(), std::overflow_error);
  EXPECT_NO_THROW(it.end()); // should be a no-op and not crash
  EXPECT_THROW(it.next(), std::overflow_error);
  EXPECT_THROW(it.prev(), std::underflow_error);
}

TEST(BitArrayIterator, empty_array_iterator_corner_cases)
{
  BitArray empty;
  auto it = empty.get_it();
  EXPECT_FALSE(it.has_curr());
  EXPECT_EQ(it.get_curr_ne(), 0u);
  EXPECT_THROW((void) it.get_curr(), std::overflow_error);
  EXPECT_THROW(it.next(), std::overflow_error);

  it.reset_last();
  EXPECT_FALSE(it.has_curr());
  EXPECT_THROW(it.prev(), std::underflow_error);
  EXPECT_NO_THROW(it.end());
}

TEST(BitArrayNumbers, set_num_bit_strings_and_numeric_roundtrip)
{
  BitArray a;

  a.set_num<std::uint8_t>(0);
  EXPECT_EQ(a.size(), 8u);
  EXPECT_EQ(a.get_bit_str(), expected_set_num_bits<std::uint8_t>(0));
  EXPECT_EQ(a.get_unum(), 0ul);
  EXPECT_EQ(a.get_num(), 0l);

  a.set_num<std::uint8_t>(0xA5);
  EXPECT_EQ(a.get_bit_str(), expected_set_num_bits<std::uint8_t>(0xA5));
  EXPECT_EQ(a.get_unum(), 0xA5ul);
  EXPECT_EQ(a.get_num(), 0xA5l);

  a.set_num<std::int8_t>(-1);
  EXPECT_EQ(a.get_bit_str(), expected_set_num_bits<std::int8_t>(-1));
  EXPECT_EQ(a.get_unum(), 255ul);
  EXPECT_EQ(a.get_num(), 255l);

  a.set_num<std::int8_t>(-2);
  EXPECT_EQ(a.get_bit_str(), expected_set_num_bits<std::int8_t>(-2));
  EXPECT_EQ(a.get_unum(), 254ul);
  EXPECT_EQ(a.get_num(), 254l);

  a.set_num<std::int16_t>(-2);
  EXPECT_EQ(a.size(), 16u);
  EXPECT_EQ(a.get_bit_str(), expected_set_num_bits<std::int16_t>(-2));
  EXPECT_EQ(a.get_unum(), 65534ul);
  EXPECT_EQ(a.get_num(), 65534l);

  a.set_num<long>(-1);
  EXPECT_EQ(a.size(), sizeof(long) * 8);
  EXPECT_EQ(a.get_bit_str(), expected_set_num_bits<long>(-1));
  EXPECT_EQ(a.get_unum(), std::numeric_limits<unsigned long>::max());
}

TEST(BitArrayNumbers, get_unum_ignores_bits_beyond_unsigned_long_width)
{
  constexpr size_t ul_bits = sizeof(unsigned long) * 8;
  constexpr size_t extra = 5;

  BitArray a(ul_bits + extra);
  a.write_bit(0, 1);                    // outside get_unum() window
  a.write_bit(a.size() - 1, 1);         // LSB within get_unum() window
  EXPECT_EQ(a.get_unum(), 1ul);
}

TEST(BitArrayRandomized, matches_reference_or_and_counts)
{
  mt19937 rng(12345);

  for (int iter = 0; iter < 50; ++iter)
    {
      const size_t n1 = rng() % 200;
      const size_t n2 = rng() % 200;

      const auto bits1 = random_bits(rng, n1);
      const auto bits2 = random_bits(rng, n2);

      const BitArray a = from_bits(bits1);
      const BitArray b = from_bits(bits2);

      const auto expected_or = or_bits(bits1, bits2);
      const auto expected_and = and_bits(bits1, bits2);

      const BitArray got_or = a | b;
      const BitArray got_and = a & b;

      EXPECT_EQ(got_or.get_bit_str(), bits_to_string(expected_or));
      EXPECT_EQ(got_and.get_bit_str(), bits_to_string(expected_and));

      const int ones = got_or.count_ones();
      int expected_ones = 0;
      for (auto bit : expected_or)
        expected_ones += static_cast<int>(bit);
      EXPECT_EQ(ones, expected_ones);
      EXPECT_EQ(got_or.count_zeros(), static_cast<int>(got_or.size()) - ones);
    }
}

TEST(BitArrayCopyMove, copy_constructor_and_assignment)
{
  BitArray a;
  a.set_bit_str("10110011");

  // Copy constructor
  BitArray b(a);
  EXPECT_EQ(b.get_bit_str(), a.get_bit_str());
  EXPECT_TRUE(b == a);

  // Modify b, a should not change
  b.write_bit(0, 0);
  EXPECT_NE(b.get_bit_str(), a.get_bit_str());
  EXPECT_EQ(a.read_bit(0), 1);

  // Copy assignment
  BitArray c;
  c = a;
  EXPECT_EQ(c.get_bit_str(), a.get_bit_str());
  EXPECT_TRUE(c == a);

  // Self assignment
  c = c;
  EXPECT_EQ(c.get_bit_str(), a.get_bit_str());
}

TEST(BitArrayCopyMove, move_constructor_and_assignment)
{
  BitArray a;
  a.set_bit_str("10110011");
  const string original = a.get_bit_str();

  // Move constructor
  BitArray b(std::move(a));
  EXPECT_EQ(b.get_bit_str(), original);
  EXPECT_EQ(a.size(), 0u); // a should be empty after move

  // Move assignment
  BitArray c;
  c.set_bit_str("111");
  c = std::move(b);
  EXPECT_EQ(c.get_bit_str(), original);
  EXPECT_EQ(b.size(), 0u); // b should be empty after move
}

TEST(BitArrayCopyMove, swap)
{
  BitArray a;
  a.set_bit_str("10110011");

  BitArray b;
  b.set_bit_str("0101");

  const string a_str = a.get_bit_str();
  const string b_str = b.get_bit_str();

  a.swap(b);

  EXPECT_EQ(a.get_bit_str(), b_str);
  EXPECT_EQ(b.get_bit_str(), a_str);
}

TEST(BitArrayFunctions, bits_list)
{
  BitArray a;
  a.set_bit_str("10110");

  auto list = a.bits_list();
  EXPECT_EQ(list.size(), 5u);

  auto it = list.get_it();
  EXPECT_EQ(it.get_curr(), 1); it.next();
  EXPECT_EQ(it.get_curr(), 0); it.next();
  EXPECT_EQ(it.get_curr(), 1); it.next();
  EXPECT_EQ(it.get_curr(), 1); it.next();
  EXPECT_EQ(it.get_curr(), 0);
}

TEST(BitArrayFunctions, fast_read_write)
{
  BitArray a;
  a.reserve(16);

  // fast_write doesn't expand, so we need to reserve first
  a.fast_write(0, 1);
  a.fast_write(7, 1);
  a.fast_write(15, 1);

  EXPECT_EQ(a.fast_read(0), 1);
  EXPECT_EQ(a.fast_read(1), 0);
  EXPECT_EQ(a.fast_read(7), 1);
  EXPECT_EQ(a.fast_read(15), 1);
}

TEST(BitArrayFunctions, save_in_array_of_chars)
{
  BitArray a;
  a.set_bit_str("10110011");

  ostringstream out;
  a.save_in_array_of_chars("test_arr", out);
  const string output = out.str();

  EXPECT_NE(output.find("test_arr"), string::npos);
  EXPECT_NE(output.find("8 bits"), string::npos);
  EXPECT_NE(output.find("const unsigned char"), string::npos);
}

TEST(BitArrayFunctions, equality_operator)
{
  BitArray a;
  a.set_bit_str("10110011");

  BitArray b;
  b.set_bit_str("10110011");

  BitArray c;
  c.set_bit_str("10110010");

  BitArray d;
  d.set_bit_str("1011001");

  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a == c);  // Different last bit
  EXPECT_FALSE(a == d);  // Different size
}

TEST(BitArrayIterator, prev_and_bidirectional)
{
  BitArray a;
  a.set_bit_str("101");

  auto it = a.get_it();
  it.reset_last();
  EXPECT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_curr(), 1u);  // Last bit

  it.prev();
  EXPECT_EQ(it.get_curr(), 0u);  // Middle bit

  it.prev();
  EXPECT_EQ(it.get_curr(), 1u);  // First bit

  // At position 0, prev moves to -1 (before first, like rend())
  it.prev();
  EXPECT_EQ(it.get_pos(), -1);
  EXPECT_FALSE(it.has_curr());

  // Now at position -1, prev should throw
  EXPECT_THROW(it.prev(), std::underflow_error);
}

TEST(BitArrayIterator, end_and_get_pos)
{
  BitArray a;
  a.set_bit_str("101");

  auto it = a.get_it();
  EXPECT_EQ(it.get_pos(), 0);

  it.next();
  EXPECT_EQ(it.get_pos(), 1);

  it.end();
  EXPECT_EQ(it.get_pos(), 3);
  EXPECT_FALSE(it.has_curr());

  it.reset();
  EXPECT_EQ(it.get_pos(), 0);
  EXPECT_TRUE(it.has_curr());
}

} // namespace
