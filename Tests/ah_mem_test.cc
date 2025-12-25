#include <gtest/gtest.h>

#include <ahMem.H>

#include <cstring>
#include <stdexcept>

namespace
{

constexpr std::size_t kBlockSize = 64;

} // namespace

TEST(AhMemTest, AllocateReturnsValidBuffer)
{
  void * ptr = Aleph::allocate(kBlockSize);
  ASSERT_NE(ptr, nullptr);
  std::memset(ptr, 0xAB, kBlockSize);
  Aleph::deallocate(ptr);
}

TEST(AhMemTest, AllocateRejectsZeroSize)
{
  EXPECT_THROW(static_cast<void>(Aleph::allocate(0)), std::invalid_argument);
}

TEST(AhMemTest, AllocateZeroedClearsMemory)
{
  void * ptr = Aleph::allocate_zeroed(kBlockSize);
  ASSERT_NE(ptr, nullptr);
  const unsigned char * bytes =
    static_cast<const unsigned char *>(ptr);
  for (std::size_t i = 0; i < kBlockSize; ++i)
    EXPECT_EQ(bytes[i], 0u);
  Aleph::deallocate(ptr);
}

TEST(AhMemTest, AllocateZeroedRejectsZeroSize)
{
  EXPECT_THROW(static_cast<void>(Aleph::allocate_zeroed(0)),
               std::invalid_argument);
}
