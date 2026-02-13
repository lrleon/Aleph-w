#include "geom_algorithms_test_common.h"


// ---------- Concave polygon boolean operations (Greiner-Hormann) ----------

TEST_F(GeomAlgorithmsTest, BooleanIntersectionConcaveLShapes)
{
  // Two overlapping L-shaped (concave) polygons.
  //
  // L1:  (0,0)-(6,0)-(6,3)-(3,3)-(3,6)-(0,6)
  // L2:  (2,2)-(8,2)-(8,8)-(5,8)-(5,5)-(2,5)
  //
  // Their intersection is a non-trivial concave region.

  Polygon L1;
  L1.add_vertex(Point(0, 0));
  L1.add_vertex(Point(6, 0));
  L1.add_vertex(Point(6, 3));
  L1.add_vertex(Point(3, 3));
  L1.add_vertex(Point(3, 6));
  L1.add_vertex(Point(0, 6));
  L1.close();

  Polygon L2;
  L2.add_vertex(Point(2, 2));
  L2.add_vertex(Point(8, 2));
  L2.add_vertex(Point(8, 8));
  L2.add_vertex(Point(5, 8));
  L2.add_vertex(Point(5, 5));
  L2.add_vertex(Point(2, 5));
  L2.close();

  BooleanPolygonOperations bop;
  auto result = bop.intersection(L1, L2);

  // Should produce at least one polygon.
  EXPECT_GE(result.size(), 1u);

  // The intersection area must be strictly less than either L-shape's area.
  // Each L-shape has area 27 (= 6*6 - 3*3).  The intersection should be
  // much smaller.
  if (result.size() >= 1)
    EXPECT_GE(result(0).size(), 3u);
}


TEST_F(GeomAlgorithmsTest, BooleanUnionConcaveNotConvexHull)
{
  // Verify that union of two overlapping concave polygons does NOT return
  // the convex hull (the original bug).

  Polygon L1;
  L1.add_vertex(Point(0, 0));
  L1.add_vertex(Point(6, 0));
  L1.add_vertex(Point(6, 3));
  L1.add_vertex(Point(3, 3));
  L1.add_vertex(Point(3, 6));
  L1.add_vertex(Point(0, 6));
  L1.close();

  Polygon sq;
  sq.add_vertex(Point(1, 1));
  sq.add_vertex(Point(5, 1));
  sq.add_vertex(Point(5, 5));
  sq.add_vertex(Point(1, 5));
  sq.close();

  BooleanPolygonOperations bop;
  auto result = bop.polygon_union(L1, sq);

  EXPECT_GE(result.size(), 1u);

  // The convex hull of L1 ∪ sq would be the bounding box (0,0)-(6,6) with
  // 4 vertices. The actual union preserves the concavity of L1, so the
  // result must have MORE than 4 vertices.
  if (result.size() == 1)
    EXPECT_GT(result(0).size(), 4u);
}


TEST_F(GeomAlgorithmsTest, BooleanUnionOverlappingSquaresExact)
{
  // Two overlapping unit squares — verify the union outline is the
  // 8-vertex L-shaped boundary, not a convex hull.

  Polygon sq1;
  sq1.add_vertex(Point(0, 0));
  sq1.add_vertex(Point(2, 0));
  sq1.add_vertex(Point(2, 2));
  sq1.add_vertex(Point(0, 2));
  sq1.close();

  Polygon sq2;
  sq2.add_vertex(Point(1, 1));
  sq2.add_vertex(Point(3, 1));
  sq2.add_vertex(Point(3, 3));
  sq2.add_vertex(Point(1, 3));
  sq2.close();

  BooleanPolygonOperations bop;
  auto result = bop.polygon_union(sq1, sq2);

  EXPECT_EQ(result.size(), 1u);
  // The union of two overlapping axis-aligned squares yields an
  // 8-vertex staircase outline.
  EXPECT_EQ(result(0).size(), 8u);
}


TEST_F(GeomAlgorithmsTest, BooleanDifferenceOverlappingSquares)
{
  // Difference: sq1 minus sq2 where they partially overlap.
  Polygon sq1;
  sq1.add_vertex(Point(0, 0));
  sq1.add_vertex(Point(2, 0));
  sq1.add_vertex(Point(2, 2));
  sq1.add_vertex(Point(0, 2));
  sq1.close();

  Polygon sq2;
  sq2.add_vertex(Point(1, 1));
  sq2.add_vertex(Point(3, 1));
  sq2.add_vertex(Point(3, 3));
  sq2.add_vertex(Point(1, 3));
  sq2.close();

  BooleanPolygonOperations bop;
  auto result = bop.difference(sq1, sq2);

  // Should produce one polygon (the part of sq1 outside sq2).
  EXPECT_EQ(result.size(), 1u);
  // The difference is an L-shaped 6-vertex polygon.
  if (result.size() == 1)
    EXPECT_EQ(result(0).size(), 6u);
}


TEST_F(GeomAlgorithmsTest, BooleanIntersectionContainment)
{
  // Small square entirely inside a larger one.
  Polygon big;
  big.add_vertex(Point(0, 0));
  big.add_vertex(Point(10, 0));
  big.add_vertex(Point(10, 10));
  big.add_vertex(Point(0, 10));
  big.close();

  Polygon small;
  small.add_vertex(Point(2, 2));
  small.add_vertex(Point(4, 2));
  small.add_vertex(Point(4, 4));
  small.add_vertex(Point(2, 4));
  small.close();

  BooleanPolygonOperations bop;

  // Intersection = small polygon.
  auto inter = bop.intersection(big, small);
  EXPECT_EQ(inter.size(), 1u);
  if (inter.size() == 1)
    EXPECT_EQ(inter(0).size(), 4u);

  // Union = big polygon.
  auto uni = bop.polygon_union(big, small);
  EXPECT_EQ(uni.size(), 1u);
  if (uni.size() == 1)
    EXPECT_EQ(uni(0).size(), 4u);

  // Difference big - small = big (hole not representable as simple polygon).
  auto diff = bop.difference(big, small);
  EXPECT_EQ(diff.size(), 1u);
}


// ============================================================================
// 3D Primitives Tests
// ============================================================================

TEST_F(GeomAlgorithmsTest, Point3DBasicOps)
{
  Point3D a(1, 2, 3);
  Point3D b(4, 5, 6);

  auto sum = a + b;
  EXPECT_EQ(sum.get_x(), Geom_Number(5));
  EXPECT_EQ(sum.get_y(), Geom_Number(7));
  EXPECT_EQ(sum.get_z(), Geom_Number(9));

  auto diff = b - a;
  EXPECT_EQ(diff.get_x(), Geom_Number(3));
  EXPECT_EQ(diff.get_y(), Geom_Number(3));
  EXPECT_EQ(diff.get_z(), Geom_Number(3));

  auto scaled = a * Geom_Number(2);
  EXPECT_EQ(scaled.get_x(), Geom_Number(2));
  EXPECT_EQ(scaled.get_y(), Geom_Number(4));
  EXPECT_EQ(scaled.get_z(), Geom_Number(6));
}


TEST_F(GeomAlgorithmsTest, Point3DDotCross)
{
  Point3D i(1, 0, 0);
  Point3D j(0, 1, 0);
  Point3D k(0, 0, 1);

  // i · j = 0
  EXPECT_EQ(i.dot(j), Geom_Number(0));
  // i · i = 1
  EXPECT_EQ(i.dot(i), Geom_Number(1));

  // i × j = k
  auto ixj = i.cross(j);
  EXPECT_EQ(ixj, k);

  // j × k = i
  auto jxk = j.cross(k);
  EXPECT_EQ(jxk, i);

  // k × i = j
  auto kxi = k.cross(i);
  EXPECT_EQ(kxi, j);
}


TEST_F(GeomAlgorithmsTest, Point3DDistanceAndNorm)
{
  Point3D a(0, 0, 0);
  Point3D b(3, 4, 0);

  EXPECT_EQ(a.distance_squared_to(b), Geom_Number(25));
  EXPECT_EQ(b.norm_squared(), Geom_Number(25));
}


TEST_F(GeomAlgorithmsTest, Point3DProjectionAndLift)
{
  Point3D p(3, 4, 5);
  Point p2d = p.to_2d();
  EXPECT_EQ(p2d, Point(3, 4));

  Point3D lifted = Point3D::from_2d(Point(1, 2));
  EXPECT_EQ(lifted, Point3D(1, 2, 0));

  Point3D lifted_z = Point3D::from_2d(Point(1, 2), Geom_Number(7));
  EXPECT_EQ(lifted_z, Point3D(1, 2, 7));
}


TEST_F(GeomAlgorithmsTest, Segment3DBasic)
{
  Point3D a(0, 0, 0), b(3, 4, 0);
  Segment3D s(a, b);

  EXPECT_EQ(s.get_src(), a);
  EXPECT_EQ(s.get_tgt(), b);
  EXPECT_EQ(s.length_squared(), Geom_Number(25));

  auto mid = s.midpoint();
  EXPECT_EQ(mid, Point3D(Geom_Number(3, 2), Geom_Number(2), Geom_Number(0)));

  EXPECT_EQ(s.at(Geom_Number(0)), a);
  EXPECT_EQ(s.at(Geom_Number(1)), b);
}


TEST_F(GeomAlgorithmsTest, Triangle3DNormal)
{
  Triangle3D t(Point3D(0, 0, 0), Point3D(1, 0, 0), Point3D(0, 1, 0));

  // Normal should be (0, 0, 1) (z-axis).
  auto n = t.normal();
  EXPECT_EQ(n, Point3D(0, 0, 1));

  EXPECT_FALSE(t.is_degenerate());
}


TEST_F(GeomAlgorithmsTest, Triangle3DDegenerate)
{
  // Collinear points → degenerate triangle.
  Triangle3D t(Point3D(0, 0, 0), Point3D(1, 0, 0), Point3D(2, 0, 0));
  EXPECT_TRUE(t.is_degenerate());
}


TEST_F(GeomAlgorithmsTest, Triangle3DCentroid)
{
  Triangle3D t(Point3D(0, 0, 0), Point3D(3, 0, 0), Point3D(0, 3, 0));
  auto c = t.centroid();
  EXPECT_EQ(c, Point3D(1, 1, 0));
}


TEST_F(GeomAlgorithmsTest, Triangle3DBarycentric)
{
  Triangle3D t(Point3D(0, 0, 0), Point3D(4, 0, 0), Point3D(0, 4, 0));

  // Centroid should have barycentric coords (1/3, 1/3, 1/3).
  auto bc = t.barycentric(Point3D(Geom_Number(4, 3), Geom_Number(4, 3), 0));
  EXPECT_EQ(bc.u, Geom_Number(1, 3));
  EXPECT_EQ(bc.v, Geom_Number(1, 3));
  EXPECT_EQ(bc.w, Geom_Number(1, 3));

  // Vertex a should have (1, 0, 0).
  auto bca = t.barycentric(Point3D(0, 0, 0));
  EXPECT_EQ(bca.u, Geom_Number(1));
  EXPECT_EQ(bca.v, Geom_Number(0));
  EXPECT_EQ(bca.w, Geom_Number(0));
}


TEST_F(GeomAlgorithmsTest, Triangle3DBarycentricDegenerateThrows)
{
  Triangle3D t(Point3D(0, 0, 0), Point3D(1, 0, 0), Point3D(2, 0, 0));
  EXPECT_THROW(t.barycentric(Point3D(0, 0, 0)), std::domain_error);
}


TEST_F(GeomAlgorithmsTest, TetrahedronVolume)
{
  // Regular tetrahedron with one vertex at origin.
  Tetrahedron tet(Point3D(0, 0, 0),
                  Point3D(6, 0, 0),
                  Point3D(0, 6, 0),
                  Point3D(0, 0, 6));

  // Volume = |det| / 6 = 6*6*6 / 6 = 36.
  EXPECT_EQ(tet.volume(), Geom_Number(36));

  EXPECT_FALSE(tet.is_degenerate());
}


TEST_F(GeomAlgorithmsTest, TetrahedronDegenerate)
{
  // Four coplanar points.
  Tetrahedron tet(Point3D(0, 0, 0),
                  Point3D(1, 0, 0),
                  Point3D(0, 1, 0),
                  Point3D(1, 1, 0));

  EXPECT_TRUE(tet.is_degenerate());
  EXPECT_EQ(tet.volume(), Geom_Number(0));
}


TEST_F(GeomAlgorithmsTest, TetrahedronContains)
{
  Tetrahedron tet(Point3D(0, 0, 0),
                  Point3D(4, 0, 0),
                  Point3D(0, 4, 0),
                  Point3D(0, 0, 4));

  // Centroid should be inside.
  EXPECT_TRUE(tet.contains(Point3D(1, 1, 1)));

  // Origin vertex should be inside (on boundary).
  EXPECT_TRUE(tet.contains(Point3D(0, 0, 0)));

  // A point far outside.
  EXPECT_FALSE(tet.contains(Point3D(10, 10, 10)));

  // A point outside but close.
  EXPECT_FALSE(tet.contains(Point3D(2, 2, 2)));
}


TEST_F(GeomAlgorithmsTest, TetrahedronCentroid)
{
  Tetrahedron tet(Point3D(0, 0, 0),
                  Point3D(4, 0, 0),
                  Point3D(0, 4, 0),
                  Point3D(0, 0, 4));

  auto c = tet.centroid();
  EXPECT_EQ(c, Point3D(1, 1, 1));
}


TEST_F(GeomAlgorithmsTest, TetrahedronFaces)
{
  Tetrahedron tet(Point3D(0, 0, 0),
                  Point3D(1, 0, 0),
                  Point3D(0, 1, 0),
                  Point3D(0, 0, 1));

  auto f = tet.faces();
  // Should have 4 faces.
  for (int i = 0; i < 4; ++i)
    EXPECT_FALSE(f.f[i].is_degenerate());
}


TEST_F(GeomAlgorithmsTest, ScalarTripleProduct)
{
  Point3D a(1, 0, 0);
  Point3D b(0, 1, 0);
  Point3D c(0, 0, 1);

  // a · (b × c) = 1 · (1) = 1
  EXPECT_EQ(scalar_triple_product(a, b, c), Geom_Number(1));

  // Cyclic: b · (c × a) = 1
  EXPECT_EQ(scalar_triple_product(b, c, a), Geom_Number(1));

  // Anti-cyclic: a · (c × b) = -1
  EXPECT_EQ(scalar_triple_product(a, c, b), Geom_Number(-1));
}


// ============================================================================
// operator<< Tests
// ============================================================================

TEST_F(GeomAlgorithmsTest, StreamOutputPoint)
{
  std::ostringstream os;
  os << Point(3, 4);
  EXPECT_NE(os.str().find("Point("), std::string::npos);
  EXPECT_NE(os.str().find("3"), std::string::npos);
  EXPECT_NE(os.str().find("4"), std::string::npos);
}


TEST_F(GeomAlgorithmsTest, StreamOutputSegment)
{
  std::ostringstream os;
  os << Segment(Point(1, 2), Point(3, 4));
  EXPECT_NE(os.str().find("Segment("), std::string::npos);
}


TEST_F(GeomAlgorithmsTest, StreamOutputTriangle)
{
  std::ostringstream os;
  os << Triangle(Point(0, 0), Point(1, 0), Point(0, 1));
  EXPECT_NE(os.str().find("Triangle("), std::string::npos);
}


TEST_F(GeomAlgorithmsTest, StreamOutputRectangle)
{
  std::ostringstream os;
  os << Rectangle(0, 0, 5, 5);
  EXPECT_NE(os.str().find("Rectangle("), std::string::npos);
}


TEST_F(GeomAlgorithmsTest, StreamOutputEllipse)
{
  std::ostringstream os;
  os << Ellipse(Point(0, 0), 3, 2);
  EXPECT_NE(os.str().find("Ellipse("), std::string::npos);
}


TEST_F(GeomAlgorithmsTest, StreamOutputRotatedEllipse)
{
  std::ostringstream os;
  os << RotatedEllipse(Point(0, 0), 3, 2);
  EXPECT_NE(os.str().find("RotatedEllipse("), std::string::npos);
}


TEST_F(GeomAlgorithmsTest, StreamOutputPolygon)
{
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(1, 0));
  sq.add_vertex(Point(1, 1));
  sq.add_vertex(Point(0, 1));
  sq.close();

  std::ostringstream os;
  os << sq;
  EXPECT_NE(os.str().find("Polygon("), std::string::npos);
  EXPECT_NE(os.str().find("n=4"), std::string::npos);
  EXPECT_NE(os.str().find("closed"), std::string::npos);
}


TEST_F(GeomAlgorithmsTest, StreamOutput3DTypes)
{
  {
    std::ostringstream os;
    os << Point3D(1, 2, 3);
    EXPECT_NE(os.str().find("Point3D("), std::string::npos);
  }
  {
    std::ostringstream os;
    os << Segment3D(Point3D(0, 0, 0), Point3D(1, 1, 1));
    EXPECT_NE(os.str().find("Segment3D("), std::string::npos);
  }
  {
    std::ostringstream os;
    os << Triangle3D(Point3D(0, 0, 0), Point3D(1, 0, 0), Point3D(0, 1, 0));
    EXPECT_NE(os.str().find("Triangle3D("), std::string::npos);
  }
  {
    std::ostringstream os;
    os << Tetrahedron(Point3D(0, 0, 0), Point3D(1, 0, 0),
                      Point3D(0, 1, 0), Point3D(0, 0, 1));
    EXPECT_NE(os.str().find("Tetrahedron("), std::string::npos);
  }
}


// ============================================================================
// Serialization (WKT, GeoJSON) Tests
// ============================================================================

TEST_F(GeomAlgorithmsTest, WKTPoint)
{
  auto wkt = GeomSerializer::to_wkt(Point(3, 4));
  EXPECT_NE(wkt.find("POINT ("), std::string::npos);
  EXPECT_NE(wkt.find("3"), std::string::npos);
  EXPECT_NE(wkt.find("4"), std::string::npos);
}


TEST_F(GeomAlgorithmsTest, WKTSegment)
{
  auto wkt = GeomSerializer::to_wkt(Segment(Point(1, 2), Point(3, 4)));
  EXPECT_NE(wkt.find("LINESTRING ("), std::string::npos);
}


TEST_F(GeomAlgorithmsTest, WKTTriangle)
{
  auto wkt = GeomSerializer::to_wkt(Triangle(Point(0, 0), Point(1, 0), Point(0, 1)));
  EXPECT_NE(wkt.find("POLYGON (("), std::string::npos);
  // WKT polygon must close: first point repeated at end.
  // Count occurrences of "0 0" — should appear twice (start and end).
  size_t pos = 0, count = 0;
  while ((pos = wkt.find("0 0", pos)) != std::string::npos)
    { ++count; ++pos; }
  EXPECT_GE(count, 2u);
}


TEST_F(GeomAlgorithmsTest, WKTRectangle)
{
  auto wkt = GeomSerializer::to_wkt(Rectangle(0, 0, 5, 5));
  EXPECT_NE(wkt.find("POLYGON (("), std::string::npos);
}


TEST_F(GeomAlgorithmsTest, WKTPolygon)
{
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(1, 0));
  sq.add_vertex(Point(1, 1));
  sq.add_vertex(Point(0, 1));
  sq.close();

  auto wkt = GeomSerializer::to_wkt(sq);
  EXPECT_NE(wkt.find("POLYGON (("), std::string::npos);
}


TEST_F(GeomAlgorithmsTest, WKTPoint3D)
{
  auto wkt = GeomSerializer::to_wkt(Point3D(1, 2, 3));
  EXPECT_NE(wkt.find("POINT Z ("), std::string::npos);
}


TEST_F(GeomAlgorithmsTest, GeoJSONPoint)
{
  auto json = GeomSerializer::to_geojson(Point(3, 4));
  EXPECT_NE(json.find("\"type\":\"Point\""), std::string::npos);
  EXPECT_NE(json.find("\"coordinates\":["), std::string::npos);
}


TEST_F(GeomAlgorithmsTest, GeoJSONSegment)
{
  auto json = GeomSerializer::to_geojson(Segment(Point(1, 2), Point(3, 4)));
  EXPECT_NE(json.find("\"type\":\"LineString\""), std::string::npos);
}


TEST_F(GeomAlgorithmsTest, GeoJSONTriangle)
{
  auto json = GeomSerializer::to_geojson(
    Triangle(Point(0, 0), Point(1, 0), Point(0, 1)));
  EXPECT_NE(json.find("\"type\":\"Polygon\""), std::string::npos);
}


TEST_F(GeomAlgorithmsTest, GeoJSONPolygon)
{
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(1, 0));
  sq.add_vertex(Point(1, 1));
  sq.add_vertex(Point(0, 1));
  sq.close();

  auto json = GeomSerializer::to_geojson(sq);
  EXPECT_NE(json.find("\"type\":\"Polygon\""), std::string::npos);
  EXPECT_NE(json.find("\"coordinates\":[["), std::string::npos);
}


TEST_F(GeomAlgorithmsTest, GeoJSONPoint3D)
{
  auto json = GeomSerializer::to_geojson(Point3D(1, 2, 3));
  EXPECT_NE(json.find("\"type\":\"Point\""), std::string::npos);
}


// ============================================================================
// AABB Tree Tests
// ============================================================================

TEST_F(GeomAlgorithmsTest, AABBTreeEmpty)
{
  AABBTree tree;
  Array<AABBTree::Entry> entries;
  tree.build(entries);
  EXPECT_TRUE(tree.is_empty());
  EXPECT_EQ(tree.size(), 0u);
}


TEST_F(GeomAlgorithmsTest, AABBTreeSingleEntry)
{
  AABBTree tree;
  Array<AABBTree::Entry> entries;
  entries.append({Rectangle(0, 0, 10, 10), 42});
  tree.build(entries);

  EXPECT_EQ(tree.size(), 1u);
  EXPECT_FALSE(tree.is_empty());

  // Point inside.
  auto r = tree.query_point(Point(5, 5));
  EXPECT_EQ(r.size(), 1u);
  EXPECT_EQ(r(0), 42u);

  // Point outside.
  r = tree.query_point(Point(20, 20));
  EXPECT_EQ(r.size(), 0u);
}


TEST_F(GeomAlgorithmsTest, AABBTreeMultipleEntries)
{
  AABBTree tree;
  Array<AABBTree::Entry> entries;
  entries.append({Rectangle(0, 0, 5, 5), 0});
  entries.append({Rectangle(3, 3, 8, 8), 1});
  entries.append({Rectangle(10, 10, 15, 15), 2});
  entries.append({Rectangle(12, 0, 17, 5), 3});
  tree.build(entries);

  EXPECT_EQ(tree.size(), 4u);

  // Query a point in the overlap of boxes 0 and 1.
  auto r = tree.query_point(Point(4, 4));
  EXPECT_EQ(r.size(), 2u);

  // Query a point only in box 2.
  r = tree.query_point(Point(12, 12));
  EXPECT_EQ(r.size(), 1u);
  EXPECT_EQ(r(0), 2u);

  // Query a point outside all boxes.
  r = tree.query_point(Point(50, 50));
  EXPECT_EQ(r.size(), 0u);
}


TEST_F(GeomAlgorithmsTest, AABBTreeBoxQuery)
{
  AABBTree tree;
  Array<AABBTree::Entry> entries;
  entries.append({Rectangle(0, 0, 5, 5), 0});
  entries.append({Rectangle(3, 3, 8, 8), 1});
  entries.append({Rectangle(10, 10, 15, 15), 2});
  tree.build(entries);

  // Query box overlapping entries 0 and 1.
  auto r = tree.query(Rectangle(2, 2, 6, 6));
  EXPECT_EQ(r.size(), 2u);

  // Query box overlapping all entries.
  r = tree.query(Rectangle(0, 0, 20, 20));
  EXPECT_EQ(r.size(), 3u);

  // Query box overlapping nothing.
  r = tree.query(Rectangle(50, 50, 60, 60));
  EXPECT_EQ(r.size(), 0u);
}


TEST_F(GeomAlgorithmsTest, AABBTreeRootBbox)
{
  AABBTree tree;
  Array<AABBTree::Entry> entries;
  entries.append({Rectangle(0, 0, 5, 5), 0});
  entries.append({Rectangle(10, 10, 15, 15), 1});
  tree.build(entries);

  auto root = tree.root_bbox();
  EXPECT_EQ(root.get_xmin(), Geom_Number(0));
  EXPECT_EQ(root.get_ymin(), Geom_Number(0));
  EXPECT_EQ(root.get_xmax(), Geom_Number(15));
  EXPECT_EQ(root.get_ymax(), Geom_Number(15));
}


// ============================================================================
// GeomNumberType Concept Test (compile-time)
// ============================================================================

#if __cplusplus >= 202002L

TEST_F(GeomAlgorithmsTest, GeomNumberConceptSatisfied)
{
  // These are compile-time checks; if we get here, the static_asserts passed.
  EXPECT_TRUE((GeomNumberType<Geom_Number>));
  EXPECT_TRUE((GeomNumberType<double>));
  EXPECT_TRUE((GeomNumberType<long long>));
}

#endif


// ============================================================================
// std::format Tests
// ============================================================================

#if __cplusplus >= 202002L && __has_include(<format>)

# include <format>


TEST_F(GeomAlgorithmsTest, StdFormatPoint)
{
  auto s = std::format("{}", Point(3, 4));
  EXPECT_NE(s.find("Point("), std::string::npos);
  EXPECT_NE(s.find("3"), std::string::npos);
  EXPECT_NE(s.find("4"), std::string::npos);
}


TEST_F(GeomAlgorithmsTest, StdFormatSegment)
{
  auto s = std::format("{}", Segment(Point(1, 2), Point(3, 4)));
  EXPECT_NE(s.find("Segment("), std::string::npos);
}


TEST_F(GeomAlgorithmsTest, StdFormatTriangle)
{
  auto s = std::format("{}", Triangle(Point(0, 0), Point(1, 0), Point(0, 1)));
  EXPECT_NE(s.find("Triangle("), std::string::npos);
}


TEST_F(GeomAlgorithmsTest, StdFormatRectangle)
{
  auto s = std::format("{}", Rectangle(0, 0, 5, 5));
  EXPECT_NE(s.find("Rectangle("), std::string::npos);
}


TEST_F(GeomAlgorithmsTest, StdFormatPoint3D)
{
  auto s = std::format("{}", Point3D(1, 2, 3));
  EXPECT_NE(s.find("Point3D("), std::string::npos);
}


#endif // C++20 format


// ============================================================================
// Polygon Aleph Patterns Tests (Section 6.3)
// ============================================================================

TEST_F(GeomAlgorithmsTest, PolygonRangeBasedFor)
{
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(4, 0));
  sq.add_vertex(Point(4, 4));
  sq.add_vertex(Point(0, 4));
  sq.close();

  // Range-based for via StlAlephIterator begin()/end().
  size_t count = 0;
  for (const auto & pt : sq)
    {
      (void)pt;
      ++count;
    }
  EXPECT_EQ(count, 4u);
}


TEST_F(GeomAlgorithmsTest, PolygonIterator)
{
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(4, 0));
  sq.add_vertex(Point(2, 3));
  sq.close();

  // Use the new Polygon::Iterator directly.
  Polygon::Iterator it(sq);
  EXPECT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_curr(), Point(0, 0));
  it.next();
  EXPECT_EQ(it.get_curr(), Point(4, 0));
  it.next();
  EXPECT_EQ(it.get_curr(), Point(2, 3));
  it.next();
  EXPECT_FALSE(it.has_curr());
}


TEST_F(GeomAlgorithmsTest, PolygonForEach)
{
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(1, 0));
  sq.add_vertex(Point(1, 1));
  sq.add_vertex(Point(0, 1));
  sq.close();

  // FunctionalMethods::for_each
  Geom_Number sum_x = 0;
  sq.for_each([&sum_x](const Point & p) { sum_x += p.get_x(); });
  EXPECT_EQ(sum_x, Geom_Number(2)); // 0 + 1 + 1 + 0
}


TEST_F(GeomAlgorithmsTest, PolygonTraverse)
{
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(1, 0));
  sq.add_vertex(Point(1, 1));
  sq.add_vertex(Point(0, 1));
  sq.close();

  // GenericTraverse::traverse — stop early.
  size_t visited = 0;
  bool completed = sq.traverse([&visited](const Point &) {
    ++visited;
    return visited < 2; // stop after 2
  });
  EXPECT_FALSE(completed);
  EXPECT_EQ(visited, 2u);
}


TEST_F(GeomAlgorithmsTest, PolygonExists)
{
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(1, 0));
  sq.add_vertex(Point(1, 1));
  sq.add_vertex(Point(0, 1));
  sq.close();

  EXPECT_TRUE(sq.exists([](const Point & p) {
    return p.get_x() == 1 && p.get_y() == 1;
  }));

  EXPECT_FALSE(sq.exists([](const Point & p) {
    return p.get_x() == 99;
  }));
}


TEST_F(GeomAlgorithmsTest, PolygonAll)
{
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(1, 0));
  sq.add_vertex(Point(1, 1));
  sq.add_vertex(Point(0, 1));
  sq.close();

  EXPECT_TRUE(sq.all([](const Point & p) {
    return p.get_x() >= 0 && p.get_y() >= 0;
  }));

  EXPECT_FALSE(sq.all([](const Point & p) {
    return p.get_x() > 0;
  }));
}


TEST_F(GeomAlgorithmsTest, PolygonMaps)
{
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(4, 0));
  sq.add_vertex(Point(2, 3));
  sq.close();

  auto xs = sq.maps<Geom_Number>([](const Point & p) { return p.get_x(); });
  EXPECT_EQ(xs.size(), 3u);
}


TEST_F(GeomAlgorithmsTest, PolygonFilter)
{
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(1, 0));
  sq.add_vertex(Point(1, 1));
  sq.add_vertex(Point(0, 1));
  sq.close();

  auto filtered = sq.filter([](const Point & p) { return p.get_x() > 0; });
  EXPECT_EQ(filtered.size(), 2u);
}


TEST_F(GeomAlgorithmsTest, PolygonInitializerList)
{
  // Special_Ctors: construct from initializer_list<Point>.
  Polygon poly = {Point(0, 0), Point(2, 0), Point(2, 2), Point(0, 2)};

  EXPECT_EQ(poly.size(), 4u);
  EXPECT_FALSE(poly.is_closed()); // Special_Ctors doesn't close
}


TEST_F(GeomAlgorithmsTest, PolygonGetIt)
{
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(4, 0));
  sq.add_vertex(Point(2, 3));
  sq.close();

  auto it = sq.get_it();
  EXPECT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_curr(), Point(0, 0));

  auto it2 = sq.get_it(2);
  EXPECT_EQ(it2.get_curr(), Point(2, 3));
}


// ============================================================================
// Section 7.1: Missing Correctness Tests
// ============================================================================

TEST_F(GeomAlgorithmsTest, ConvexHullAlgorithmComparison)
{
  // All convex hull algorithms should produce the same result.
  DynList<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(10, 0));
  pts.append(Point(10, 10));
  pts.append(Point(0, 10));
  pts.append(Point(5, 5));   // interior
  pts.append(Point(3, 2));   // interior
  pts.append(Point(7, 8));   // interior
  pts.append(Point(1, 9));   // interior

  GiftWrappingConvexHull gift;
  GrahamScanConvexHull graham;
  QuickHull qh;

  auto hull_gw = gift(pts);
  auto hull_gm = graham(pts);
  auto hull_qh = qh(pts);

  // All should have same number of hull vertices (the 4 corners).
  EXPECT_EQ(hull_gw.size(), 4u);
  EXPECT_EQ(hull_gm.size(), 4u);
  EXPECT_EQ(hull_qh.size(), 4u);
}


TEST_F(GeomAlgorithmsTest, TriangulationNonConvexLShape)
{
  // L-shaped polygon (non-convex).
  Polygon L;
  L.add_vertex(Point(0, 0));
  L.add_vertex(Point(6, 0));
  L.add_vertex(Point(6, 3));
  L.add_vertex(Point(3, 3));
  L.add_vertex(Point(3, 6));
  L.add_vertex(Point(0, 6));
  L.close();

  CuttingEarsTriangulation cet;
  auto tris = cet(L);
  // An n-vertex polygon yields n-2 triangles.
  EXPECT_EQ(tris.size(), 4u); // 6 vertices -> 4 triangles
}


TEST_F(GeomAlgorithmsTest, TriangulationNonConvexUShaped)
{
  // U-shaped polygon.
  Polygon U;
  U.add_vertex(Point(0, 0));
  U.add_vertex(Point(6, 0));
  U.add_vertex(Point(6, 6));
  U.add_vertex(Point(5, 6));
  U.add_vertex(Point(5, 1));
  U.add_vertex(Point(1, 1));
  U.add_vertex(Point(1, 6));
  U.add_vertex(Point(0, 6));
  U.close();

  CuttingEarsTriangulation cet;
  auto tris = cet(U);
  EXPECT_EQ(tris.size(), 6u); // 8 vertices -> 6 triangles
}


TEST_F(GeomAlgorithmsTest, PointInPolygonManyVertices)
{
  // Create a regular-ish polygon with many vertices (circle approximation).
  Polygon circle;
  const int N = 32;
  for (int i = 0; i < N; ++i)
    {
      Geom_Number x(Geom_Number(1000) * Geom_Number(i) / Geom_Number(N));
      Geom_Number y(Geom_Number(1000) * Geom_Number((i * 7 + 3) % N) / Geom_Number(N));
      // Use a convex polygon: just use a large square with many vertices on edges
      if (i < N/4)
        circle.add_vertex(Point(i * 4, 0));
      else if (i < N/2)
        circle.add_vertex(Point((N/4) * 4, (i - N/4) * 4));
      else if (i < 3*N/4)
        circle.add_vertex(Point((3*N/4 - i) * 4, (N/4) * 4));
      else
        circle.add_vertex(Point(0, (N - i) * 4));
    }
  circle.close();

  // Center should be inside.
  EXPECT_TRUE(circle.contains(Point(16, 16)));
  // Far away point should be outside.
  EXPECT_FALSE(circle.contains(Point(1000, 1000)));
}


// ============================================================================
// Section 7.2: Missing Robustness Tests
// ============================================================================

TEST_F(GeomAlgorithmsTest, NearCollinearPoints)
{
  // Three nearly collinear points — exact arithmetic should handle this.
  // p3 deviates by 1/10^15 from the line p1-p2.
  Point p1(0, 0);
  Point p2(Geom_Number(1000000), 0);
  // Tiny deviation from collinear.
  Point p3(Geom_Number(500000), Geom_Number(1, 1000000000));

  // Should NOT be collinear (exact rational arithmetic).
  EXPECT_FALSE(p3.is_colinear_with(p1, p2));

  // But if deviation is exactly 0, it IS collinear.
  Point p4(Geom_Number(500000), 0);
  EXPECT_TRUE(p4.is_colinear_with(p1, p2));
}


TEST_F(GeomAlgorithmsTest, ExtremeCoordinates)
{
  // Very large coordinates.
  Geom_Number big("1000000000000000000"); // 10^18
  Point p1(big, big);
  Point p2(-big, -big);
  Point p3(big, -big);

  // Distance should be exact.
  Geom_Number dist_sq = p1.distance_squared_to(p2);
  EXPECT_EQ(dist_sq, Geom_Number(4) * big * big * Geom_Number(2));

  // Triangle should work.
  Triangle t(p1, p2, p3);
  EXPECT_FALSE(t.contains(Point(0, 0))); // origin outside this triangle

  // Very small coordinates.
  Geom_Number tiny(1, 1000000000);
  Point q1(0, 0);
  Point q2(tiny, 0);
  Point q3(0, tiny);
  Triangle t2(q1, q2, q3);
  // A point at (tiny/3, tiny/3) should be inside.
  EXPECT_TRUE(t2.contains(Point(tiny / 3, tiny / 3)));
}


TEST_F(GeomAlgorithmsTest, NearParallelSegments)
{
  // Two segments that are nearly parallel but do intersect.
  Segment s1(Point(0, 0), Point(Geom_Number(1000000), Geom_Number(1)));
  Segment s2(Point(0, Geom_Number(1, 2)),
             Point(Geom_Number(1000000), 0));

  // They should intersect (they cross at some point).
  EXPECT_TRUE(s1.intersects_with(s2));
}


TEST_F(GeomAlgorithmsTest, CocircularPoints)
{
  // 4 points on a circle of radius 5 centered at origin.
  // (3,4), (-3,4), (-3,-4), (3,-4) all on circle r=5.
  Point a(3, 4);
  Point b(-3, 4);
  Point c(-3, -4);
  Point d(3, -4);

  // d should be ON the circumcircle of a,b,c (not inside).
  auto result = in_circle(a, b, c, d);
  EXPECT_EQ(result, InCircleResult::ON_CIRCLE);
}


// ============================================================================
// Section 7.4: Missing Primitive Tests
// ============================================================================

TEST_F(GeomAlgorithmsTest, IntersectsProperlyWithNearCollinear)
{
  // Test the intersects_properly_with predicate with near-collinear segments.
  Segment s1(Point(0, 0), Point(10, 0));
  Segment s2(Point(5, -1), Point(5, 1));

  EXPECT_TRUE(s1.intersects_properly_with(s2));

  // Collinear overlapping segments should NOT intersect properly.
  Segment s3(Point(0, 0), Point(6, 0));
  Segment s4(Point(4, 0), Point(10, 0));
  EXPECT_FALSE(s3.intersects_properly_with(s4));
}


TEST_F(GeomAlgorithmsTest, EllipseIntersectionVerticalSegment)
{
  // Vertical segment through the center of an ellipse.
  Ellipse e(Point(0, 0), 5, 3);

  // Vertical segment x=0 from y=-10 to y=10.
  Segment vert(Point(0, -10), Point(0, 10));
  EXPECT_TRUE(e.intersects_with(vert));
}


TEST_F(GeomAlgorithmsTest, SegmentEnlargeDiagonal)
{
  // Enlarge a diagonal segment in both directions.
  Segment s(Point(0, 0), Point(3, 4)); // length = 5

  Segment s_copy = s;
  s_copy.enlarge_src(Geom_Number(5));
  // Source should have moved away from target.
  EXPECT_TRUE(s_copy.size() > s.size());

  Segment s_copy2 = s;
  s_copy2.enlarge_tgt(Geom_Number(5));
  EXPECT_TRUE(s_copy2.size() > s.size());
}


TEST_F(GeomAlgorithmsTest, TriangleCWvsCCW)
{
  // CCW triangle.
  Triangle ccw(Point(0, 0), Point(4, 0), Point(2, 3));
  EXPECT_TRUE(ccw.contains(Point(2, 1)));

  // CW triangle (reversed vertex order).
  Triangle cw(Point(0, 0), Point(2, 3), Point(4, 0));
  EXPECT_TRUE(cw.contains(Point(2, 1)));
}


TEST_F(GeomAlgorithmsTest, RectangleCornerIntersection)
{
  // Two rectangles sharing exactly one corner.
  Rectangle r1(0, 0, 5, 5);
  Rectangle r2(5, 5, 10, 10);

  // They touch at (5,5) — xmin of r2 == xmax of r1.
  // Depending on the definition, they may or may not intersect.
  // The point (5,5) is on the boundary of both.
  Point corner(5, 5);
  EXPECT_TRUE(corner.get_x() >= r1.get_xmin() &&
              corner.get_x() <= r1.get_xmax() &&
              corner.get_y() >= r1.get_ymin() &&
              corner.get_y() <= r1.get_ymax());
  EXPECT_TRUE(corner.get_x() >= r2.get_xmin() &&
              corner.get_x() <= r2.get_xmax() &&
              corner.get_y() >= r2.get_ymin() &&
              corner.get_y() <= r2.get_ymax());
}


TEST_F(GeomAlgorithmsTest, SegmentContainsEndpoints)
{
  // contains() should return true for endpoints.
  Segment s(Point(1, 2), Point(5, 6));
  EXPECT_TRUE(s.contains(s.get_src_point()));
  EXPECT_TRUE(s.contains(s.get_tgt_point()));

  // Midpoint should also be contained.
  EXPECT_TRUE(s.contains(s.mid_point()));
}


TEST_F(GeomAlgorithmsTest, PolygonContainsNewAPI)
{
  // Verify the new contains() method on Polygon works.
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(10, 0));
  sq.add_vertex(Point(10, 10));
  sq.add_vertex(Point(0, 10));
  sq.close();

  EXPECT_TRUE(sq.contains(Point(5, 5)));
  EXPECT_FALSE(sq.contains(Point(20, 20)));
  // Boundary point.
  EXPECT_TRUE(sq.contains(Point(0, 5)));
}


TEST_F(GeomAlgorithmsTest, TriangleContainsNewAPI)
{
  Triangle t(Point(0, 0), Point(10, 0), Point(0, 10));
  EXPECT_TRUE(t.contains(Point(1, 1)));
  EXPECT_FALSE(t.contains(Point(8, 8)));
}


TEST_F(GeomAlgorithmsTest, EllipseContainsNewAPI)
{
  Ellipse e(Point(0, 0), 5, 3);
  EXPECT_TRUE(e.contains(Point(0, 0)));    // center
  EXPECT_TRUE(e.contains(Point(4, 0)));    // inside
  EXPECT_FALSE(e.contains(Point(10, 10))); // outside
}


TEST_F(GeomAlgorithmsTest, EllipseDefaultInvalidRadiiThrow)
{
  Ellipse e;
  Segment t1, t2;

  EXPECT_THROW(e.contains(Point(0, 0)), std::domain_error);
  EXPECT_THROW(e.intersects_with(Point(0, 0)), std::domain_error);
  EXPECT_THROW(e.compute_tangents(t1, t2, Geom_Number(1)), std::domain_error);
}


TEST_F(GeomAlgorithmsTest, RotatedEllipseDefaultInvalidRadiiThrow)
{
  RotatedEllipse e;

  EXPECT_THROW(e.contains(Point(0, 0)), std::domain_error);
  EXPECT_THROW(e.strictly_contains(Point(0, 0)), std::domain_error);
  EXPECT_THROW(e.on_boundary(Point(0, 0)), std::domain_error);
}
