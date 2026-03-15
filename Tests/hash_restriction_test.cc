
#include <gtest/gtest.h>
#include <hash-fct.H>
#include <vector>
#include <type_traits>

using namespace Aleph;

// This test doesn't actually run anything, it just checks for compilation 
// behavior (via type traits and decltype if needed, but here we can just 
// use a compile-time check for the existence of the overload)

template<typename T, typename = void>
struct is_hashable : std::false_type {};

template<typename T>
struct is_hashable<T, std::void_t<decltype(fnv_hash(std::declval<T>()))>> : std::true_type {};

TEST(HashRestrictionTest, TriviallyCopyableTypesAreHashable)
{
  EXPECT_TRUE(is_hashable<int>::value);
  EXPECT_TRUE(is_hashable<double>::value);
  
  struct Simple { int a; char b; };
  EXPECT_TRUE(is_hashable<Simple>::value);
}

TEST(HashRestrictionTest, NonTriviallyCopyableTypesAreNotHashable)
{
  // std::vector is NOT trivially copyable
  EXPECT_FALSE(is_hashable<std::vector<int>>::value);
}

TEST(HashRestrictionTest, SpecialOverloadsStillWork)
{
  // std::string is NOT trivially copyable, but has a special overload
  EXPECT_TRUE(is_hashable<std::string>::value);
  
  // const char* is a pointer (trivially copyable) and has overloads
  EXPECT_TRUE(is_hashable<const char*>::value);
}
