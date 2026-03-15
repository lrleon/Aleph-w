
#include <gtest/gtest.h>
#include <hash-fct.H>
#include <string>

using namespace Aleph;

TEST(JSWInitTest, LazyInitialization)
{
  // Note: This test assumes it's the first one to run in its process
  // or that jsw was not initialized yet.
  // Since we cannot easily reset static state, we just check consistency.
  
  const std::string key = "test_key";
  
  // This should not throw anymore, even if init_jsw wasn't called
  size_t h1 = jsw_hash(key);
  EXPECT_TRUE(is_jsw_initialized());
  
  size_t h2 = jsw_hash(key);
  EXPECT_EQ(h1, h2);
}

TEST(JSWInitTest, Reinitialization)
{
  const std::string key = "test_key";
  
  init_jsw(12345);
  size_t h1 = jsw_hash(key);
  
  init_jsw(12345); // Same seed
  size_t h2 = jsw_hash(key);
  EXPECT_EQ(h1, h2);
  
  init_jsw(54321); // Different seed
  size_t h3 = jsw_hash(key);
  EXPECT_NE(h1, h3);
}
