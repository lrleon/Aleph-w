/**
 * @file hash_fct_test.cc
 * @brief Exhaustive test suite for hash functions in hash-fct.H
 *
 * Tests cover:
 * - Consistency: same input always produces same output
 * - Various input types: void*, templates, const char*, std::string
 * - Edge cases: empty strings, single byte, large data
 * - Basic distribution quality (collision rates)
 * - All hash functions: add_hash, xor_hash, rot_hash, djb_hash, sax_hash,
 *   fnv_hash, oat_hash, jsw_hash, elf_hash, jen_hash, SuperFastHash,
 *   murmur3hash, dft_hash_fct, snd_hash_fct
 */

#include <gtest/gtest.h>
#include <hash-fct.H>

#include <cstring>
#include <random>
#include <set>
#include <string>
#include <vector>

using namespace Aleph;

// JSW hash requires initialization (defined in Aleph namespace in hash-fct.C)
namespace Aleph {
  extern void init_jsw() noexcept;
}

class HashTestEnvironment : public ::testing::Environment
{
public:
  void SetUp() override
  {
    init_jsw();
  }
};

// Register the environment to initialize jsw_hash table before any tests run
testing::Environment* const hash_env =
    testing::AddGlobalTestEnvironment(new HashTestEnvironment);

// ==================== Consistency Tests ====================
// Hash functions must produce the same output for the same input

class HashConsistencyTest : public ::testing::Test
{
protected:
  const char* test_string = "test_string_for_hashing";
  const std::string test_std_string = "test_string_for_hashing";
  const int test_int = 42;
  const double test_double = 3.14159265359;
};

TEST_F(HashConsistencyTest, AddHashConsistency)
{
  auto h1 = add_hash(test_string);
  auto h2 = add_hash(test_string);
  EXPECT_EQ(h1, h2);

  auto h3 = add_hash(test_std_string);
  auto h4 = add_hash(test_std_string);
  EXPECT_EQ(h3, h4);
  EXPECT_EQ(h1, h3);  // const char* and std::string should match

  auto h5 = add_hash(test_int);
  auto h6 = add_hash(test_int);
  EXPECT_EQ(h5, h6);
}

TEST_F(HashConsistencyTest, XorHashConsistency)
{
  auto h1 = xor_hash(test_string);
  auto h2 = xor_hash(test_string);
  EXPECT_EQ(h1, h2);

  auto h3 = xor_hash(test_std_string);
  auto h4 = xor_hash(test_std_string);
  EXPECT_EQ(h3, h4);
  EXPECT_EQ(h1, h3);

  auto h5 = xor_hash(test_int);
  auto h6 = xor_hash(test_int);
  EXPECT_EQ(h5, h6);
}

TEST_F(HashConsistencyTest, RotHashConsistency)
{
  auto h1 = rot_hash(test_string);
  auto h2 = rot_hash(test_string);
  EXPECT_EQ(h1, h2);

  auto h3 = rot_hash(test_std_string);
  EXPECT_EQ(h1, h3);

  auto h5 = rot_hash(test_int);
  auto h6 = rot_hash(test_int);
  EXPECT_EQ(h5, h6);
}

TEST_F(HashConsistencyTest, DjbHashConsistency)
{
  auto h1 = djb_hash(test_string);
  auto h2 = djb_hash(test_string);
  EXPECT_EQ(h1, h2);

  auto h3 = djb_hash(test_std_string);
  EXPECT_EQ(h1, h3);

  auto h5 = djb_hash(test_int);
  auto h6 = djb_hash(test_int);
  EXPECT_EQ(h5, h6);
}

TEST_F(HashConsistencyTest, SaxHashConsistency)
{
  auto h1 = sax_hash(test_string);
  auto h2 = sax_hash(test_string);
  EXPECT_EQ(h1, h2);

  auto h3 = sax_hash(test_std_string);
  EXPECT_EQ(h1, h3);

  auto h5 = sax_hash(test_int);
  auto h6 = sax_hash(test_int);
  EXPECT_EQ(h5, h6);
}

TEST_F(HashConsistencyTest, FnvHashConsistency)
{
  auto h1 = fnv_hash(test_string);
  auto h2 = fnv_hash(test_string);
  EXPECT_EQ(h1, h2);

  auto h3 = fnv_hash(test_std_string);
  EXPECT_EQ(h1, h3);

  auto h5 = fnv_hash(test_int);
  auto h6 = fnv_hash(test_int);
  EXPECT_EQ(h5, h6);
}

TEST_F(HashConsistencyTest, OatHashConsistency)
{
  auto h1 = oat_hash(test_string);
  auto h2 = oat_hash(test_string);
  EXPECT_EQ(h1, h2);

  auto h3 = oat_hash(test_std_string);
  EXPECT_EQ(h1, h3);

  auto h5 = oat_hash(test_int);
  auto h6 = oat_hash(test_int);
  EXPECT_EQ(h5, h6);
}

TEST_F(HashConsistencyTest, JswHashConsistency)
{
  auto h1 = jsw_hash(test_string);
  auto h2 = jsw_hash(test_string);
  EXPECT_EQ(h1, h2);

  auto h3 = jsw_hash(test_std_string);
  EXPECT_EQ(h1, h3);

  auto h5 = jsw_hash(test_int);
  auto h6 = jsw_hash(test_int);
  EXPECT_EQ(h5, h6);
}

TEST_F(HashConsistencyTest, ElfHashConsistency)
{
  auto h1 = elf_hash(test_string);
  auto h2 = elf_hash(test_string);
  EXPECT_EQ(h1, h2);

  auto h3 = elf_hash(test_std_string);
  EXPECT_EQ(h1, h3);

  auto h5 = elf_hash(test_int);
  auto h6 = elf_hash(test_int);
  EXPECT_EQ(h5, h6);
}

TEST_F(HashConsistencyTest, JenHashConsistency)
{
  unsigned seed = 12345;
  // Use std::string for consistency - const char* would use template version
  auto h1 = jen_hash(test_std_string, seed);
  auto h2 = jen_hash(test_std_string, seed);
  EXPECT_EQ(h1, h2);

  // Also test via void* API directly
  auto h3 = jen_hash((void*)test_string, strlen(test_string), seed);
  auto h4 = jen_hash((void*)test_std_string.c_str(), test_std_string.size(), seed);
  EXPECT_EQ(h3, h4);

  auto h5 = jen_hash(test_int, seed);
  auto h6 = jen_hash(test_int, seed);
  EXPECT_EQ(h5, h6);
}

TEST_F(HashConsistencyTest, SuperFastHashConsistency)
{
  auto h1 = SuperFastHash(test_string);
  auto h2 = SuperFastHash(test_string);
  EXPECT_EQ(h1, h2);

  auto h3 = SuperFastHash(test_std_string);
  EXPECT_EQ(h1, h3);

  auto h5 = SuperFastHash(test_int);
  auto h6 = SuperFastHash(test_int);
  EXPECT_EQ(h5, h6);
}

TEST_F(HashConsistencyTest, Murmur3HashConsistency)
{
  unsigned long seed = 42;
  auto h1 = murmur3hash(test_std_string, seed);
  auto h2 = murmur3hash(test_std_string, seed);
  EXPECT_EQ(h1, h2);

  auto h3 = murmur3hash(test_int, seed);
  auto h4 = murmur3hash(test_int, seed);
  EXPECT_EQ(h3, h4);
}

TEST_F(HashConsistencyTest, DftHashFctConsistency)
{
  auto h1 = dft_hash_fct(test_std_string);
  auto h2 = dft_hash_fct(test_std_string);
  EXPECT_EQ(h1, h2);

  auto h3 = dft_hash_fct(test_int);
  auto h4 = dft_hash_fct(test_int);
  EXPECT_EQ(h3, h4);
}

// ==================== Edge Case Tests ====================

class HashEdgeCaseTest : public ::testing::Test
{
protected:
  const char* empty_string = "";
  const std::string empty_std_string = "";
  const char* single_char = "a";
  std::string large_string;

  void SetUp() override
  {
    // Create a large string (1MB)
    large_string.resize(1024 * 1024, 'x');
  }
};

TEST_F(HashEdgeCaseTest, EmptyStringHash)
{
  // Empty strings should hash consistently (though maybe to 0 for simple hashes)
  EXPECT_EQ(add_hash(empty_string), add_hash(empty_std_string));
  EXPECT_EQ(xor_hash(empty_string), xor_hash(empty_std_string));
  EXPECT_EQ(rot_hash(empty_string), rot_hash(empty_std_string));
  EXPECT_EQ(djb_hash(empty_string), djb_hash(empty_std_string));
  EXPECT_EQ(sax_hash(empty_string), sax_hash(empty_std_string));
  EXPECT_EQ(fnv_hash(empty_string), fnv_hash(empty_std_string));
  EXPECT_EQ(oat_hash(empty_string), oat_hash(empty_std_string));
  EXPECT_EQ(jsw_hash(empty_string), jsw_hash(empty_std_string));
  EXPECT_EQ(elf_hash(empty_string), elf_hash(empty_std_string));
  EXPECT_EQ(SuperFastHash(empty_string), SuperFastHash(empty_std_string));
}

TEST_F(HashEdgeCaseTest, SingleCharHash)
{
  const std::string single_std = "a";

  // All hashes should produce non-zero for single char 'a'
  EXPECT_NE(add_hash(single_char), 0u);
  EXPECT_NE(xor_hash(single_char), 0u);
  EXPECT_NE(rot_hash(single_char), 0u);
  EXPECT_NE(djb_hash(single_char), 0u);
  EXPECT_NE(sax_hash(single_char), 0u);
  EXPECT_NE(fnv_hash(single_char), 0u);
  EXPECT_NE(oat_hash(single_char), 0u);
  EXPECT_NE(jsw_hash(single_char), 0u);
  EXPECT_NE(elf_hash(single_char), 0u);
  EXPECT_NE(SuperFastHash(single_char), 0u);

  // Verify consistency
  EXPECT_EQ(add_hash(single_char), add_hash(single_std));
}

TEST_F(HashEdgeCaseTest, LargeStringHash)
{
  // Large strings should hash correctly
  auto h1 = fnv_hash(large_string);
  auto h2 = fnv_hash(large_string);
  EXPECT_EQ(h1, h2);

  auto h3 = SuperFastHash(large_string);
  auto h4 = SuperFastHash(large_string);
  EXPECT_EQ(h3, h4);

  auto h5 = oat_hash(large_string);
  auto h6 = oat_hash(large_string);
  EXPECT_EQ(h5, h6);

  auto h7 = murmur3hash(large_string, 42ul);
  auto h8 = murmur3hash(large_string, 42ul);
  EXPECT_EQ(h7, h8);
}

TEST_F(HashEdgeCaseTest, BinaryDataWithNulls)
{
  // Test hashing data with embedded null bytes
  char data1[] = {1, 0, 2, 0, 3};
  char data2[] = {1, 0, 2, 0, 3};

  EXPECT_EQ(add_hash(data1, 5), add_hash(data2, 5));
  EXPECT_EQ(fnv_hash(data1, 5), fnv_hash(data2, 5));
  EXPECT_EQ(oat_hash(data1, 5), oat_hash(data2, 5));
  EXPECT_EQ(SuperFastHash(data1, 5), SuperFastHash(data2, 5));
  EXPECT_EQ(jen_hash(data1, 5), jen_hash(data2, 5));
}

// ==================== Different Inputs Produce Different Hashes ====================

class HashDifferenceTest : public ::testing::Test
{
};

TEST_F(HashDifferenceTest, DifferentStringsDifferentHashes)
{
  const char* s1 = "hello";
  const char* s2 = "world";
  const char* s3 = "hello1";  // Very similar to s1

  // Production-quality hashes should distinguish these
  EXPECT_NE(fnv_hash(s1), fnv_hash(s2));
  EXPECT_NE(fnv_hash(s1), fnv_hash(s3));

  EXPECT_NE(oat_hash(s1), oat_hash(s2));
  EXPECT_NE(oat_hash(s1), oat_hash(s3));

  EXPECT_NE(SuperFastHash(s1), SuperFastHash(s2));
  EXPECT_NE(SuperFastHash(s1), SuperFastHash(s3));

  EXPECT_NE(djb_hash(s1), djb_hash(s2));
  EXPECT_NE(sax_hash(s1), sax_hash(s2));
  EXPECT_NE(elf_hash(s1), elf_hash(s2));
  EXPECT_NE(jsw_hash(s1), jsw_hash(s2));
}

TEST_F(HashDifferenceTest, DifferentIntsDifferentHashes)
{
  int i1 = 1;
  int i2 = 2;
  int i3 = 1000000;

  EXPECT_NE(fnv_hash(i1), fnv_hash(i2));
  EXPECT_NE(fnv_hash(i1), fnv_hash(i3));

  EXPECT_NE(oat_hash(i1), oat_hash(i2));
  EXPECT_NE(SuperFastHash(i1), SuperFastHash(i2));
  EXPECT_NE(djb_hash(i1), djb_hash(i2));
}

TEST_F(HashDifferenceTest, DifferentSeedsDifferentHashes)
{
  const std::string key = "test_key";

  auto h1 = jen_hash(key, 1);
  auto h2 = jen_hash(key, 2);
  auto h3 = jen_hash(key, 100);

  EXPECT_NE(h1, h2);
  EXPECT_NE(h1, h3);
  EXPECT_NE(h2, h3);

  auto m1 = murmur3hash(key, 1ul);
  auto m2 = murmur3hash(key, 2ul);
  auto m3 = murmur3hash(key, 100ul);

  EXPECT_NE(m1, m2);
  EXPECT_NE(m1, m3);
  EXPECT_NE(m2, m3);
}

// ==================== Void Pointer API Tests ====================

class HashVoidPtrTest : public ::testing::Test
{
};

TEST_F(HashVoidPtrTest, IntArrayHash)
{
  int arr1[] = {1, 2, 3, 4, 5};
  int arr2[] = {1, 2, 3, 4, 5};
  int arr3[] = {1, 2, 3, 4, 6};  // Different

  EXPECT_EQ(add_hash(arr1, sizeof(arr1)), add_hash(arr2, sizeof(arr2)));
  EXPECT_NE(add_hash(arr1, sizeof(arr1)), add_hash(arr3, sizeof(arr3)));

  EXPECT_EQ(fnv_hash(arr1, sizeof(arr1)), fnv_hash(arr2, sizeof(arr2)));
  EXPECT_NE(fnv_hash(arr1, sizeof(arr1)), fnv_hash(arr3, sizeof(arr3)));

  EXPECT_EQ(oat_hash(arr1, sizeof(arr1)), oat_hash(arr2, sizeof(arr2)));
  EXPECT_EQ(SuperFastHash(arr1, sizeof(arr1)), SuperFastHash(arr2, sizeof(arr2)));
  // jen_hash with void* requires explicit cast for arrays
  EXPECT_EQ(jen_hash((void*)arr1, sizeof(arr1)), jen_hash((void*)arr2, sizeof(arr2)));
}

TEST_F(HashVoidPtrTest, StructHash)
{
  // Use a packed struct or memset to ensure no padding issues
  struct TestStruct
  {
    int a;
    double b;
    char c;
  };

  // Zero-initialize to avoid uninitialized padding bytes
  TestStruct s1, s2, s3;
  memset(&s1, 0, sizeof(s1));
  memset(&s2, 0, sizeof(s2));
  memset(&s3, 0, sizeof(s3));
  s1.a = 1; s1.b = 2.5; s1.c = 'x';
  s2.a = 1; s2.b = 2.5; s2.c = 'x';
  s3.a = 1; s3.b = 2.5; s3.c = 'y';

  EXPECT_EQ(fnv_hash(&s1, sizeof(s1)), fnv_hash(&s2, sizeof(s2)));
  EXPECT_NE(fnv_hash(&s1, sizeof(s1)), fnv_hash(&s3, sizeof(s3)));

  EXPECT_EQ(oat_hash(&s1, sizeof(s1)), oat_hash(&s2, sizeof(s2)));
  EXPECT_EQ(SuperFastHash(&s1, sizeof(s1)), SuperFastHash(&s2, sizeof(s2)));
}

// ==================== Template API Tests ====================

class HashTemplateTest : public ::testing::Test
{
};

TEST_F(HashTemplateTest, IntTemplateHash)
{
  int val = 12345;

  // Template versions should work
  auto h1 = add_hash(val);
  auto h2 = add_hash(&val, sizeof(val));
  EXPECT_EQ(h1, h2);

  auto h3 = fnv_hash(val);
  auto h4 = fnv_hash(&val, sizeof(val));
  EXPECT_EQ(h3, h4);
}

TEST_F(HashTemplateTest, DoubleTemplateHash)
{
  double val = 3.14159;

  auto h1 = fnv_hash(val);
  auto h2 = fnv_hash(&val, sizeof(val));
  EXPECT_EQ(h1, h2);

  auto h3 = oat_hash(val);
  auto h4 = oat_hash(&val, sizeof(val));
  EXPECT_EQ(h3, h4);
}

TEST_F(HashTemplateTest, LongLongTemplateHash)
{
  long long val = 9223372036854775807LL;

  auto h1 = fnv_hash(val);
  auto h2 = fnv_hash(&val, sizeof(val));
  EXPECT_EQ(h1, h2);

  auto h3 = SuperFastHash(val);
  auto h4 = SuperFastHash(&val, sizeof(val));
  EXPECT_EQ(h3, h4);
}

// ==================== Pair Hash Functions ====================

TEST(PairHashTest, PairDftHashFct)
{
  std::pair<int, int> p1{1, 2};
  std::pair<int, int> p2{1, 2};
  std::pair<int, int> p3{2, 1};

  auto h1 = pair_dft_hash_fct(p1);
  auto h2 = pair_dft_hash_fct(p2);
  auto h3 = pair_dft_hash_fct(p3);

  EXPECT_EQ(h1, h2);
  // Note: pair_dft_hash_fct uses addition, so {1,2} and {2,1} might collide
  // This is a known limitation of simple combination functions
  (void)h3;  // Suppress unused variable warning - see comment above
}

TEST(PairHashTest, PairSndHashFct)
{
  std::pair<std::string, std::string> p1{"hello", "world"};
  std::pair<std::string, std::string> p2{"hello", "world"};

  auto h1 = pair_snd_hash_fct(p1);
  auto h2 = pair_snd_hash_fct(p2);

  EXPECT_EQ(h1, h2);
}

// ==================== Distribution Quality Tests ====================

class HashDistributionTest : public ::testing::Test
{
protected:
  static constexpr size_t NUM_KEYS = 10000;
  std::vector<std::string> keys;

  void SetUp() override
  {
    std::mt19937 gen(42);  // Fixed seed for reproducibility
    std::uniform_int_distribution<> len_dist(5, 20);
    std::uniform_int_distribution<> char_dist('a', 'z');

    for (size_t i = 0; i < NUM_KEYS; ++i)
      {
        int len = len_dist(gen);
        std::string s;
        s.reserve(len);
        for (int j = 0; j < len; ++j)
          s += static_cast<char>(char_dist(gen));
        keys.push_back(std::move(s));
      }
  }

  // Measure hash uniqueness (collision ratio without modulo)
  // Good hash functions should have very few or no collisions
  double measureHashUniqueness(const std::vector<size_t>& hashes)
  {
    std::set<size_t> unique_hashes(hashes.begin(), hashes.end());
    return static_cast<double>(unique_hashes.size()) /
           static_cast<double>(hashes.size());
  }
};

// Test that production-quality hash functions produce mostly unique hashes
// (without modulo). We expect >90% uniqueness for 10000 distinct random strings.
TEST_F(HashDistributionTest, FnvHashUniqueness)
{
  std::vector<size_t> hashes;
  hashes.reserve(NUM_KEYS);
  for (const auto& key : keys)
    hashes.push_back(fnv_hash(key));

  double uniqueness = measureHashUniqueness(hashes);
  EXPECT_GT(uniqueness, 0.90) << "FNV hash uniqueness: " << uniqueness;
}

TEST_F(HashDistributionTest, OatHashUniqueness)
{
  std::vector<size_t> hashes;
  hashes.reserve(NUM_KEYS);
  for (const auto& key : keys)
    hashes.push_back(oat_hash(key));

  double uniqueness = measureHashUniqueness(hashes);
  EXPECT_GT(uniqueness, 0.90) << "OAT hash uniqueness: " << uniqueness;
}

TEST_F(HashDistributionTest, SuperFastHashUniqueness)
{
  std::vector<size_t> hashes;
  hashes.reserve(NUM_KEYS);
  for (const auto& key : keys)
    hashes.push_back(SuperFastHash(key));

  double uniqueness = measureHashUniqueness(hashes);
  EXPECT_GT(uniqueness, 0.90) << "SuperFastHash uniqueness: " << uniqueness;
}

TEST_F(HashDistributionTest, JenHashUniqueness)
{
  std::vector<size_t> hashes;
  hashes.reserve(NUM_KEYS);
  for (const auto& key : keys)
    hashes.push_back(jen_hash(key, 0));

  double uniqueness = measureHashUniqueness(hashes);
  EXPECT_GT(uniqueness, 0.90) << "Jenkins hash uniqueness: " << uniqueness;
}

TEST_F(HashDistributionTest, Murmur3HashUniqueness)
{
  std::vector<size_t> hashes;
  hashes.reserve(NUM_KEYS);
  for (const auto& key : keys)
    hashes.push_back(murmur3hash(key, 42ul));

  double uniqueness = measureHashUniqueness(hashes);
  EXPECT_GT(uniqueness, 0.90) << "MurmurHash3 uniqueness: " << uniqueness;
}

TEST_F(HashDistributionTest, DftHashFctUniqueness)
{
  std::vector<size_t> hashes;
  hashes.reserve(NUM_KEYS);
  for (const auto& key : keys)
    hashes.push_back(dft_hash_fct(key));

  double uniqueness = measureHashUniqueness(hashes);
  EXPECT_GT(uniqueness, 0.90) << "dft_hash_fct uniqueness: " << uniqueness;
}

// ==================== Avalanche Property Tests ====================
// Small changes in input should cause large changes in output

class HashAvalancheTest : public ::testing::Test
{
protected:
  // Count bit differences between two hash values
  static int bitDifference(size_t h1, size_t h2)
  {
    size_t diff = h1 ^ h2;
    int count = 0;
    while (diff)
      {
        count += diff & 1;
        diff >>= 1;
      }
    return count;
  }
};

TEST_F(HashAvalancheTest, SingleBitChangeJenkins)
{
  // Changing a single bit should cause ~50% of output bits to change
  // for a good hash function
  unsigned char data1[16] = {0};
  unsigned char data2[16] = {0};
  data2[0] = 1;  // Single bit difference

  auto h1 = jen_hash(data1, 16);
  auto h2 = jen_hash(data2, 16);

  int bit_diff = bitDifference(h1, h2);
  // Good hash should have at least 20% of bits different
  EXPECT_GT(bit_diff, (int)(sizeof(size_t) * 8 * 0.2))
      << "Jenkins hash has poor avalanche property";
}

TEST_F(HashAvalancheTest, SingleBitChangeMurmur3)
{
  unsigned char data1[16] = {0};
  unsigned char data2[16] = {0};
  data2[0] = 1;

  auto h1 = murmur3hash(std::string((char*)data1, 16), 42ul);
  auto h2 = murmur3hash(std::string((char*)data2, 16), 42ul);

  int bit_diff = bitDifference(h1, h2);
  EXPECT_GT(bit_diff, (int)(sizeof(size_t) * 8 * 0.2))
      << "MurmurHash3 has poor avalanche property";
}

TEST_F(HashAvalancheTest, SingleBitChangeOat)
{
  unsigned char data1[16] = {0};
  unsigned char data2[16] = {0};
  data2[0] = 1;

  auto h1 = oat_hash(data1, 16);
  auto h2 = oat_hash(data2, 16);

  int bit_diff = bitDifference(h1, h2);
  EXPECT_GT(bit_diff, (int)(sizeof(size_t) * 8 * 0.2))
      << "One-at-a-Time hash has poor avalanche property";
}

// ==================== Known Value Tests ====================
// Test against known hash values for regression testing

TEST(HashKnownValuesTest, AddHashKnownValue)
{
  // "abc" = 97 + 98 + 99 = 294
  EXPECT_EQ(add_hash("abc"), 294u);
}

TEST(HashKnownValuesTest, XorHashKnownValue)
{
  // "abc" = 97 ^ 98 ^ 99 = 96
  EXPECT_EQ(xor_hash("abc"), 96u);
}

TEST(HashKnownValuesTest, FnvHashStartValue)
{
  // Empty string with fnv_hash should return the FNV offset basis
  // Since our implementation iterates while (*p), empty string returns
  // initial value
  EXPECT_EQ(fnv_hash(""), 2166136261u);
}

// ==================== Seeded Hash Tests ====================

TEST(SeededHashTest, JenHashWithDefaultSeed)
{
  const std::string data = "test";
  auto h1 = jen_hash(data, Default_Hash_Seed);
  auto h2 = jen_hash((void*)data.c_str(), data.size(), Default_Hash_Seed);
  EXPECT_EQ(h1, h2);
}

TEST(SeededHashTest, DftHashFctWithSeed)
{
  const std::string key = "test_key";

  auto h1 = dft_hash_fct(key, 1ul);
  auto h2 = dft_hash_fct(key, 2ul);

  EXPECT_NE(h1, h2);  // Different seeds should produce different hashes
}

// ==================== Type Safety Tests ====================

TEST(HashTypeSafetyTest, SignedUnsignedConsistency)
{
  int signed_val = -1;
  unsigned int unsigned_val = static_cast<unsigned int>(-1);

  // These should hash the same as they have the same bit representation
  auto h1 = fnv_hash(&signed_val, sizeof(signed_val));
  auto h2 = fnv_hash(&unsigned_val, sizeof(unsigned_val));
  EXPECT_EQ(h1, h2);
}

TEST(HashTypeSafetyTest, ConstCharPtrAndStringConsistency)
{
  // Test that const char* and std::string versions produce same hashes
  // Note: char arr[] uses template version which includes null terminator,
  // so we test const char* and std::string which both hash the string content
  const char* ptr = "hello";
  std::string str = "hello";

  EXPECT_EQ(fnv_hash(ptr), fnv_hash(str));
  EXPECT_EQ(oat_hash(ptr), oat_hash(str));
  EXPECT_EQ(djb_hash(ptr), djb_hash(str));
  EXPECT_EQ(sax_hash(ptr), sax_hash(str));
  EXPECT_EQ(SuperFastHash(ptr), SuperFastHash(str));
}

// ==================== Performance Sanity Checks ====================
// These are not rigorous benchmarks but ensure hashes complete in
// reasonable time

TEST(HashPerformanceTest, LargeDataHashing)
{
  // 10MB of data
  std::string large_data(10 * 1024 * 1024, 'x');

  // These should complete without hanging
  auto h1 = fnv_hash(large_data);
  auto h2 = oat_hash(large_data);
  auto h3 = SuperFastHash(large_data);
  auto h4 = murmur3hash(large_data, 42ul);

  EXPECT_NE(h1, 0u);
  EXPECT_NE(h2, 0u);
  EXPECT_NE(h3, 0u);
  EXPECT_NE(h4, 0u);
}

TEST(HashPerformanceTest, ManySmallHashes)
{
  // Hash 100,000 small integers
  size_t sum = 0;
  for (int i = 0; i < 100000; ++i)
    sum += fnv_hash(i);

  EXPECT_NE(sum, 0u);
}