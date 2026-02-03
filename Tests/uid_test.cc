/**
 * @file uid_test.cc
 * @brief Unit tests for Uid class
 *
 * Tests construction, string conversion, equality, and exception handling
 * for the Uid class.
 */

#include <gtest/gtest.h>
#include <uid.H>
#include <string>
#include <cstring>
#include <vector>
#include <set>

using namespace Aleph;

// =============================================================================
// Basic Functionality Tests
// =============================================================================

TEST(UidTest, DefaultConstructor) {
    Uid id;
    // Default constructor leaves members uninitialized or zero-initialized depending on implementation details
    // but should be safe to call.
    // We can't assert specific values unless we know the implementation initializes them.
    // Based on the code, members are mutable but not initialized in default ctor.
    // However, it should be a valid object.
    (void)id;
}

TEST(UidTest, ComponentConstructor) {
    IPv4_Address ip = 3232235777; // 192.168.1.1 as uint32_t
    uint64_t counter = 1234567890123456789ULL;
    uint32_t port = 8080;

    Uid id(ip, counter, port);

    EXPECT_EQ(id.getIpAddr(), ip);
    EXPECT_EQ(id.get_counter(), counter);
    EXPECT_EQ(id.get_port_number(), port);

    // Random number should be initialized
    // We can't predict it, but we can check it's accessible
    (void)id.get_random_number();
}

TEST(UidTest, EqualityOperator) {
    IPv4_Address ip = 167772161; // 10.0.0.1 as uint32_t
    uint64_t counter = 42;
    uint32_t port = 1234;

    Uid id1(ip, counter, port);

    // Create a copy manually to ensure same random number
    // Since random number is generated in constructor, we can't just construct another one
    // with same parameters and expect equality (random numbers would differ).
    // We need to test copy construction or string round-trip.

    // Let's test self-equality first
    EXPECT_EQ(id1, id1);

    // Test copy
    Uid id2 = id1;
    EXPECT_EQ(id1, id2);

    // Test inequality (different counter)
    Uid id3(ip, counter + 1, port);
    EXPECT_FALSE(id1 == id3);
}

TEST(UidTest, StringConversionRoundTrip) {
    IPv4_Address ip = 2130706433; // 127.0.0.1 as uint32_t
    uint64_t counter = 987654321;
    uint32_t port = 5000;

    Uid original(ip, counter, port);

    char buffer[Uid::stringSize];
    char* result = original.getStringUid(buffer, sizeof(buffer));

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result, buffer);
    EXPECT_EQ(strlen(buffer), static_cast<size_t>(Uid::stringSize - 1));

    // Reconstruct from string
    Uid reconstructed(buffer);

    EXPECT_EQ(original, reconstructed);
    EXPECT_EQ(reconstructed.getIpAddr(), ip);
    EXPECT_EQ(reconstructed.get_counter(), counter);
    EXPECT_EQ(reconstructed.get_port_number(), port);
    EXPECT_EQ(reconstructed.get_random_number(), original.get_random_number());
}

// =============================================================================
// Exception Handling and Edge Cases
// =============================================================================

TEST(UidTest, StringBufferSizeCheck) {
    IPv4_Address ip = 2130706433; // 127.0.0.1
    Uid id(ip, 1, 1);

    char small_buffer[10]; // Too small

    // Should throw range_error (ah_range_error_if macro)
    EXPECT_THROW(id.getStringUid(small_buffer, sizeof(small_buffer)), std::range_error);
}

TEST(UidTest, Uniqueness) {
    // Verify that consecutive UIDs are different (due to random number)
    IPv4_Address ip = 2130706433; // 127.0.0.1
    uint64_t counter = 1;
    uint32_t port = 1;

    std::set<std::string> uids;
    const int num_uids = 100;

    for (int i = 0; i < num_uids; ++i) {
        Uid id(ip, counter, port); // Same params, different random number
        char buffer[Uid::stringSize];
        id.getStringUid(buffer, sizeof(buffer));
        uids.insert(std::string(buffer));
    }

    // All generated UIDs should be unique
    EXPECT_EQ(uids.size(), static_cast<size_t>(num_uids));
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
