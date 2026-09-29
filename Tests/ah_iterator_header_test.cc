#include <ah-iterator.H>

#include <gtest/gtest.h>

// Regression test for aleph-concepts.md §2.1: ah-iterator.H must compile
// when included first, on its own. extract_from_stl_container() names
// Aleph::DynList in a non-dependent qualified-id, which needs a forward
// declaration even though the function body is never instantiated here.

// AlephRandomAccessItor lives in the global namespace, not Aleph::, since
// ah-iterator.H never opens `namespace Aleph`.
struct NoRandomAccess {};
static_assert(not AlephRandomAccessItor<NoRandomAccess>);

TEST(AhIteratorHeaderTest, IsSelfContained)
{
  SUCCEED();
}
