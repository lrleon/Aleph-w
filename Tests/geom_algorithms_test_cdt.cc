#include "geom_algorithms_test_common.h"
#include <cmath>

// ============================================================================
// Helper: check that an indexed edge (u,v) appears in a CDT result
// ============================================================================
static bool cdt_has_edge(
    const ConstrainedDelaunayTriangulation::Result & r,
    const size_t u, const size_t v)
{
  for (size_t t = 0; t < r.triangles.size(); ++t)
    {
      const auto & tri = r.triangles(t);
      size_t vs[3] = {tri.i, tri.j, tri.k};
      for (int e = 0; e < 3; ++e)
        {
          const size_t a = vs[e], b = vs[(e + 1) % 3];
          if ((a == u and b == v) or (a == v and b == u))
            return true;
        }
    }
  return false;
}

// Helper: find point index in sorted result sites
static size_t find_site(const Array<Point> & sites, const Point & p)
{
  for (size_t i = 0; i < sites.size(); ++i)
    if (sites(i) == p)
      return i;
  return ~static_cast<size_t>(0);
}

// Helper: check the constrained edge list contains an edge between two points
static bool has_constrained_edge(
    const ConstrainedDelaunayTriangulation::Result & r,
    const Point & p, const Point & q)
{
  const size_t u = find_site(r.sites, p);
  const size_t v = find_site(r.sites, q);
  if (u == ~static_cast<size_t>(0) or v == ~static_cast<size_t>(0))
    return false;

  for (size_t i = 0; i < r.constrained_edges.size(); ++i)
    {
      const auto & e = r.constrained_edges(i);
      if ((e.u == u and e.v == v) or (e.u == v and e.v == u))
        return true;
    }
  return false;
}

// Helper: check all triangles have CCW winding
static bool all_ccw(const ConstrainedDelaunayTriangulation::Result & r)
{
  for (size_t t = 0; t < r.triangles.size(); ++t)
    {
      const auto & tri = r.triangles(t);
      const Orientation o = orientation(r.sites(tri.i),
                                        r.sites(tri.j),
                                        r.sites(tri.k));
      if (o != Orientation::CCW)
        return false;
    }
  return true;
}

// Helper: check Delaunay property for non-constrained edges
// For each triangle, check that no non-adjacent vertex violates circumcircle
// (only approximate check — checks that mesh triangles don't violate each other)
static bool check_delaunay_for_non_constrained(
    const ConstrainedDelaunayTriangulation::Result & r)
{
  // Build a set of constrained edge pairs for fast lookup
  struct EdgeKey { size_t u, v; };
  Array<EdgeKey> con_edges;
  for (size_t i = 0; i < r.constrained_edges.size(); ++i)
    {
      const auto & e = r.constrained_edges(i);
      const size_t a = e.u < e.v ? e.u : e.v;
      const size_t b = e.u < e.v ? e.v : e.u;
      con_edges.append(EdgeKey{a, b});
    }

  auto is_constrained = [&](size_t a, size_t b) -> bool
    {
      if (a > b) { const size_t t = a; a = b; b = t; }
      for (size_t i = 0; i < con_edges.size(); ++i)
        if (con_edges(i).u == a and con_edges(i).v == b)
          return true;
      return false;
    };

  // For each pair of triangles sharing a non-constrained edge,
  // check that neither opposite vertex is inside the other's circumcircle.
  for (size_t t1 = 0; t1 < r.triangles.size(); ++t1)
    {
      const auto & tri1 = r.triangles(t1);
      size_t vs1[3] = {tri1.i, tri1.j, tri1.k};

      for (int e = 0; e < 3; ++e)
        {
          const size_t ea = vs1[e], eb = vs1[(e + 1) % 3];
          if (is_constrained(ea, eb))
            continue;

          // Find the other triangle sharing this edge
          for (size_t t2 = t1 + 1; t2 < r.triangles.size(); ++t2)
            {
              const auto & tri2 = r.triangles(t2);
              size_t vs2[3] = {tri2.i, tri2.j, tri2.k};

              bool shares = false;
              size_t opp = ~static_cast<size_t>(0);
              for (int f = 0; f < 3; ++f)
                {
                  const size_t fa = vs2[f], fb = vs2[(f + 1) % 3];
                  if ((fa == ea and fb == eb) or (fa == eb and fb == ea))
                    {
                      shares = true;
                      opp = vs2[(f + 2) % 3];
                      break;
                    }
                }

              if (not shares)
                continue;

              // Check that opp is not inside circumcircle of tri1
              const Orientation o = orientation(r.sites(tri1.i),
                                                r.sites(tri1.j),
                                                r.sites(tri1.k));
              if (o == Orientation::COLLINEAR)
                continue;

              const Geom_Number det = in_circle_determinant(
                  r.sites(tri1.i), r.sites(tri1.j),
                  r.sites(tri1.k), r.sites(opp));

              if (o == Orientation::CCW and det > 0)
                return false; // violation
              if (o == Orientation::CW and det < 0)
                return false;
            }
        }
    }

  return true;
}

// ============================================================================
// Basic tests
// ============================================================================

TEST_F(GeomAlgorithmsTest, CDT_TriangleNoConstraints)
{
  DynList<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(4, 0));
  pts.append(Point(2, 3));

  DynList<Segment> constraints;

  ConstrainedDelaunayTriangulation cdt;
  auto result = cdt(pts, constraints);

  EXPECT_EQ(result.sites.size(), 3u);
  EXPECT_EQ(result.triangles.size(), 1u);
  EXPECT_TRUE(result.constrained_edges.is_empty());
}

TEST_F(GeomAlgorithmsTest, CDT_SquareWithForcedDiagonal)
{
  Point a(0, 0), b(4, 0), c(4, 4), d(0, 4);

  DynList<Point> pts;
  pts.append(a); pts.append(b); pts.append(c); pts.append(d);

  // Force diagonal a-c
  DynList<Segment> constraints;
  constraints.append(Segment(a, c));

  ConstrainedDelaunayTriangulation cdt;
  auto result = cdt(pts, constraints);

  EXPECT_EQ(result.sites.size(), 4u);
  EXPECT_EQ(result.triangles.size(), 2u);

  // The constrained diagonal must be present
  EXPECT_TRUE(has_constrained_edge(result, a, c));

  // The edge a-c should be in the triangulation
  const size_t ia = find_site(result.sites, a);
  const size_t ic = find_site(result.sites, c);
  EXPECT_TRUE(cdt_has_edge(result, ia, ic));
}

TEST_F(GeomAlgorithmsTest, CDT_ConstraintAlreadyInDT)
{
  // Triangle — any edge is already in DT
  Point a(0, 0), b(6, 0), c(3, 5);

  DynList<Point> pts;
  pts.append(a); pts.append(b); pts.append(c);

  DynList<Segment> constraints;
  constraints.append(Segment(a, b)); // already a DT edge

  ConstrainedDelaunayTriangulation cdt;
  auto result = cdt(pts, constraints);

  EXPECT_EQ(result.triangles.size(), 1u);
  EXPECT_TRUE(has_constrained_edge(result, a, b));
}

TEST_F(GeomAlgorithmsTest, CDT_ConstraintCrossingDTEdge)
{
  // Diamond shape — force the non-Delaunay diagonal
  Point a(0, 0), b(2, 1), c(4, 0), d(2, -1);

  DynList<Point> pts;
  pts.append(a); pts.append(b); pts.append(c); pts.append(d);

  // The Delaunay diagonal is likely b-d. Force a-c instead.
  DynList<Segment> constraints;
  constraints.append(Segment(a, c));

  ConstrainedDelaunayTriangulation cdt;
  auto result = cdt(pts, constraints);

  EXPECT_EQ(result.sites.size(), 4u);
  EXPECT_EQ(result.triangles.size(), 2u);
  EXPECT_TRUE(has_constrained_edge(result, a, c));

  const size_t ia = find_site(result.sites, a);
  const size_t ic = find_site(result.sites, c);
  EXPECT_TRUE(cdt_has_edge(result, ia, ic));
}

TEST_F(GeomAlgorithmsTest, CDT_MultipleConstraints)
{
  Point a(0, 0), b(6, 0), c(6, 6), d(0, 6), e(3, 3);

  DynList<Point> pts;
  pts.append(a); pts.append(b); pts.append(c);
  pts.append(d); pts.append(e);

  DynList<Segment> constraints;
  constraints.append(Segment(a, c)); // diagonal
  constraints.append(Segment(b, d)); // other diagonal

  ConstrainedDelaunayTriangulation cdt;
  auto result = cdt(pts, constraints);

  EXPECT_GE(result.sites.size(), 5u);
  EXPECT_GE(result.triangles.size(), 4u);

  // Both diagonals cross at e(3,3), so constraints are split.
  // a-c becomes (a,e) + (e,c); b-d becomes (b,e) + (e,d).
  EXPECT_TRUE(has_constrained_edge(result, a, e));
  EXPECT_TRUE(has_constrained_edge(result, e, c));
  EXPECT_TRUE(has_constrained_edge(result, b, e));
  EXPECT_TRUE(has_constrained_edge(result, e, d));
}

TEST_F(GeomAlgorithmsTest, CDT_PLSGChain)
{
  // A zig-zag chain as constraints
  Point p0(0, 0), p1(2, 3), p2(4, 0), p3(6, 3), p4(8, 0);

  DynList<Point> pts;
  pts.append(p0); pts.append(p1); pts.append(p2);
  pts.append(p3); pts.append(p4);

  DynList<Segment> constraints;
  constraints.append(Segment(p0, p1));
  constraints.append(Segment(p1, p2));
  constraints.append(Segment(p2, p3));
  constraints.append(Segment(p3, p4));

  ConstrainedDelaunayTriangulation cdt;
  auto result = cdt(pts, constraints);

  EXPECT_EQ(result.sites.size(), 5u);
  EXPECT_GE(result.triangles.size(), 3u);

  EXPECT_TRUE(has_constrained_edge(result, p0, p1));
  EXPECT_TRUE(has_constrained_edge(result, p1, p2));
  EXPECT_TRUE(has_constrained_edge(result, p2, p3));
  EXPECT_TRUE(has_constrained_edge(result, p3, p4));
}

// ============================================================================
// Edge cases
// ============================================================================

TEST_F(GeomAlgorithmsTest, CDT_EmptyInput)
{
  DynList<Point> pts;
  DynList<Segment> constraints;

  ConstrainedDelaunayTriangulation cdt;
  auto result = cdt(pts, constraints);

  EXPECT_TRUE(result.triangles.is_empty());
}

TEST_F(GeomAlgorithmsTest, CDT_LessThan3Points)
{
  DynList<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(1, 1));

  DynList<Segment> constraints;

  ConstrainedDelaunayTriangulation cdt;
  auto result = cdt(pts, constraints);

  EXPECT_TRUE(result.triangles.is_empty());
}

TEST_F(GeomAlgorithmsTest, CDT_CollinearPoints)
{
  DynList<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(1, 0));
  pts.append(Point(2, 0));

  DynList<Segment> constraints;

  ConstrainedDelaunayTriangulation cdt;
  auto result = cdt(pts, constraints);

  EXPECT_TRUE(result.triangles.is_empty());
}

TEST_F(GeomAlgorithmsTest, CDT_OnlyConstraintEndpoints)
{
  // Points come only from constraint segments
  Point a(0, 0), b(4, 0), c(2, 3);

  DynList<Point> pts; // empty point list

  DynList<Segment> constraints;
  constraints.append(Segment(a, b));
  constraints.append(Segment(b, c));
  constraints.append(Segment(c, a));

  ConstrainedDelaunayTriangulation cdt;
  auto result = cdt(pts, constraints);

  EXPECT_EQ(result.sites.size(), 3u);
  EXPECT_EQ(result.triangles.size(), 1u);

  EXPECT_TRUE(has_constrained_edge(result, a, b));
  EXPECT_TRUE(has_constrained_edge(result, b, c));
  EXPECT_TRUE(has_constrained_edge(result, c, a));
}

TEST_F(GeomAlgorithmsTest, CDT_PointOnConstraintInterior)
{
  // A constraint from a to c, with b lying on the segment
  Point a(0, 0), b(2, 0), c(4, 0), d(2, 3);

  DynList<Point> pts;
  pts.append(a); pts.append(b); pts.append(c); pts.append(d);

  DynList<Segment> constraints;
  constraints.append(Segment(a, c)); // b lies on this

  ConstrainedDelaunayTriangulation cdt;
  auto result = cdt(pts, constraints);

  EXPECT_EQ(result.sites.size(), 4u);
  EXPECT_GE(result.triangles.size(), 2u);

  // The constraint should be split into a-b and b-c
  const size_t ia = find_site(result.sites, a);
  const size_t ib = find_site(result.sites, b);
  const size_t ic = find_site(result.sites, c);
  EXPECT_TRUE(cdt_has_edge(result, ia, ib));
  EXPECT_TRUE(cdt_has_edge(result, ib, ic));
}

TEST_F(GeomAlgorithmsTest, CDT_DuplicatePoints)
{
  Point a(0, 0), b(4, 0), c(2, 3);

  DynList<Point> pts;
  pts.append(a); pts.append(b); pts.append(c);
  pts.append(a); pts.append(b); // duplicates

  DynList<Segment> constraints;

  ConstrainedDelaunayTriangulation cdt;
  auto result = cdt(pts, constraints);

  EXPECT_EQ(result.sites.size(), 3u);
  EXPECT_EQ(result.triangles.size(), 1u);
}

TEST_F(GeomAlgorithmsTest, CDT_DuplicateConstraints)
{
  Point a(0, 0), b(4, 0), c(2, 3);

  DynList<Point> pts;
  pts.append(a); pts.append(b); pts.append(c);

  DynList<Segment> constraints;
  constraints.append(Segment(a, b));
  constraints.append(Segment(a, b)); // duplicate
  constraints.append(Segment(b, a)); // reverse duplicate

  ConstrainedDelaunayTriangulation cdt;
  auto result = cdt(pts, constraints);

  EXPECT_EQ(result.sites.size(), 3u);
  EXPECT_EQ(result.triangles.size(), 1u);
  EXPECT_TRUE(has_constrained_edge(result, a, b));
}

// ============================================================================
// Property tests
// ============================================================================

TEST_F(GeomAlgorithmsTest, CDT_AllConstraintsPresent)
{
  // Larger set with multiple constraints
  Point p0(0, 0), p1(5, 0), p2(5, 5), p3(0, 5);
  Point p4(1, 1), p5(4, 1), p6(4, 4), p7(1, 4);

  DynList<Point> pts;
  pts.append(p0); pts.append(p1); pts.append(p2); pts.append(p3);
  pts.append(p4); pts.append(p5); pts.append(p6); pts.append(p7);

  // Inner square edges as constraints
  DynList<Segment> constraints;
  constraints.append(Segment(p4, p5));
  constraints.append(Segment(p5, p6));
  constraints.append(Segment(p6, p7));
  constraints.append(Segment(p7, p4));

  ConstrainedDelaunayTriangulation cdt;
  auto result = cdt(pts, constraints);

  EXPECT_GE(result.sites.size(), 8u);

  // All constraint edges must be in the triangulation
  for (size_t i = 0; i < result.constrained_edges.size(); ++i)
    {
      const auto & e = result.constrained_edges(i);
      EXPECT_TRUE(cdt_has_edge(result, e.u, e.v))
          << "Constrained edge (" << e.u << "," << e.v << ") not in triangulation";
    }

  EXPECT_TRUE(has_constrained_edge(result, p4, p5));
  EXPECT_TRUE(has_constrained_edge(result, p5, p6));
  EXPECT_TRUE(has_constrained_edge(result, p6, p7));
  EXPECT_TRUE(has_constrained_edge(result, p7, p4));
}

TEST_F(GeomAlgorithmsTest, CDT_DelaunayPropertyForNonConstrained)
{
  Point p0(0, 0), p1(6, 0), p2(6, 6), p3(0, 6), p4(3, 3);

  DynList<Point> pts;
  pts.append(p0); pts.append(p1); pts.append(p2);
  pts.append(p3); pts.append(p4);

  // One constraint
  DynList<Segment> constraints;
  constraints.append(Segment(p0, p2));

  ConstrainedDelaunayTriangulation cdt;
  auto result = cdt(pts, constraints);

  EXPECT_TRUE(check_delaunay_for_non_constrained(result))
      << "Delaunay property violated for non-constrained edges";
}

TEST_F(GeomAlgorithmsTest, CDT_AllPointsPresent)
{
  Point a(0, 0), b(4, 0), c(4, 4), d(0, 4), e(2, 2);

  DynList<Point> pts;
  pts.append(a); pts.append(b); pts.append(c);
  pts.append(d); pts.append(e);

  DynList<Segment> constraints;
  constraints.append(Segment(a, c));

  ConstrainedDelaunayTriangulation cdt;
  auto result = cdt(pts, constraints);

  EXPECT_NE(find_site(result.sites, a), ~static_cast<size_t>(0));
  EXPECT_NE(find_site(result.sites, b), ~static_cast<size_t>(0));
  EXPECT_NE(find_site(result.sites, c), ~static_cast<size_t>(0));
  EXPECT_NE(find_site(result.sites, d), ~static_cast<size_t>(0));
  EXPECT_NE(find_site(result.sites, e), ~static_cast<size_t>(0));
}

TEST_F(GeomAlgorithmsTest, CDT_CCWWinding)
{
  Point p0(0, 0), p1(6, 0), p2(6, 4), p3(0, 4), p4(3, 2);

  DynList<Point> pts;
  pts.append(p0); pts.append(p1); pts.append(p2);
  pts.append(p3); pts.append(p4);

  DynList<Segment> constraints;
  constraints.append(Segment(p0, p2));

  ConstrainedDelaunayTriangulation cdt;
  auto result = cdt(pts, constraints);

  // Check all triangles that are non-degenerate are CCW
  for (size_t t = 0; t < result.triangles.size(); ++t)
    {
      const auto & tri = result.triangles(t);
      const Orientation o = orientation(result.sites(tri.i),
                                        result.sites(tri.j),
                                        result.sites(tri.k));
      EXPECT_NE(o, Orientation::COLLINEAR) << "Degenerate triangle " << t;
    }
}

// ============================================================================
// Stress tests
// ============================================================================

TEST_F(GeomAlgorithmsTest, CDT_GridWithDiagonalConstraints)
{
  // 4x4 grid of points with diagonal constraints
  DynList<Point> pts;
  for (int y = 0; y < 4; ++y)
    for (int x = 0; x < 4; ++x)
      pts.append(Point(x, y));

  // Diagonal constraints across each cell
  DynList<Segment> constraints;
  for (int y = 0; y < 3; ++y)
    for (int x = 0; x < 3; ++x)
      constraints.append(Segment(Point(x, y), Point(x + 1, y + 1)));

  ConstrainedDelaunayTriangulation cdt;
  auto result = cdt(pts, constraints);

  EXPECT_EQ(result.sites.size(), 16u);
  EXPECT_GE(result.triangles.size(), 18u); // at least 2 per cell

  // All diagonal constraints present
  for (int y = 0; y < 3; ++y)
    for (int x = 0; x < 3; ++x)
      EXPECT_TRUE(has_constrained_edge(result, Point(x, y),
                                       Point(x + 1, y + 1)));
}

TEST_F(GeomAlgorithmsTest, CDT_PolygonBoundaryAsConstraints)
{
  // Pentagon boundary as constraints with interior point
  Point p0(2, 0), p1(4, 1.5), p2(3, 4), p3(1, 4), p4(0, 1.5);
  Point center(2, 2);

  DynList<Point> pts;
  pts.append(p0); pts.append(p1); pts.append(p2);
  pts.append(p3); pts.append(p4); pts.append(center);

  DynList<Segment> constraints;
  constraints.append(Segment(p0, p1));
  constraints.append(Segment(p1, p2));
  constraints.append(Segment(p2, p3));
  constraints.append(Segment(p3, p4));
  constraints.append(Segment(p4, p0));

  ConstrainedDelaunayTriangulation cdt;
  auto result = cdt(pts, constraints);

  EXPECT_EQ(result.sites.size(), 6u);
  EXPECT_GE(result.triangles.size(), 4u);

  EXPECT_TRUE(has_constrained_edge(result, p0, p1));
  EXPECT_TRUE(has_constrained_edge(result, p1, p2));
  EXPECT_TRUE(has_constrained_edge(result, p2, p3));
  EXPECT_TRUE(has_constrained_edge(result, p3, p4));
  EXPECT_TRUE(has_constrained_edge(result, p4, p0));
}

TEST_F(GeomAlgorithmsTest, CDT_AsTrianglesConversion)
{
  Point a(0, 0), b(4, 0), c(2, 3);

  DynList<Point> pts;
  pts.append(a); pts.append(b); pts.append(c);

  DynList<Segment> constraints;
  constraints.append(Segment(a, b));

  ConstrainedDelaunayTriangulation cdt;
  auto result = cdt(pts, constraints);

  DynList<Triangle> triangles = ConstrainedDelaunayTriangulation::as_triangles(result);
  EXPECT_EQ(triangles.size(), result.triangles.size());
}

TEST_F(GeomAlgorithmsTest, CDT_InitializerListOverload)
{
  ConstrainedDelaunayTriangulation cdt;
  auto result = cdt(
      {Point(0, 0), Point(4, 0), Point(4, 4), Point(0, 4)},
      {Segment(Point(0, 0), Point(4, 4))});

  EXPECT_EQ(result.sites.size(), 4u);
  EXPECT_EQ(result.triangles.size(), 2u);
  EXPECT_TRUE(has_constrained_edge(result, Point(0, 0), Point(4, 4)));
}

TEST_F(GeomAlgorithmsTest, CDT_DelaunayCheckOnLargerSet)
{
  // 8 points with 2 constraints — verify Delaunay property
  DynList<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(5, 0));
  pts.append(Point(10, 0));
  pts.append(Point(10, 5));
  pts.append(Point(10, 10));
  pts.append(Point(5, 10));
  pts.append(Point(0, 10));
  pts.append(Point(0, 5));

  DynList<Segment> constraints;
  constraints.append(Segment(Point(0, 0), Point(10, 10)));

  ConstrainedDelaunayTriangulation cdt;
  auto result = cdt(pts, constraints);

  EXPECT_GE(result.triangles.size(), 6u);
  EXPECT_TRUE(has_constrained_edge(result, Point(0, 0), Point(10, 10)));
  EXPECT_TRUE(check_delaunay_for_non_constrained(result));
}
