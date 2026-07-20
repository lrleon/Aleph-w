#include <polygon.H>

using namespace Aleph;

/** @brief Read every observable scalar in the default polygon under MSan. */
int main()
{
  const Regular_Polygon polygon;
  if (polygon.size() != 0 or polygon.get_side_size() != 0 or
      polygon.radius() != 0 or polygon.is_closed())
    return 1;
  return 0;
}
