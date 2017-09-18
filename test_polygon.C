# include <cassert>
# include <iostream>
# include <polygon.H>
# include <geom_algorithms.H>

int main()
{
  Polygon base;

  assert(not base.is_closed());
  assert(base.size() == 0);
  cout << "Default constructor ok!\n";

  base.add_vertex(0, 0);
  assert(not base.is_closed());
  assert(base.size() == 1);
  base.add_vertex(1, 1);
  assert(not base.is_closed());
  assert(base.size() == 2);
  base.add_vertex(-1, 1);
  assert(not base.is_closed());
  assert(base.size() == 3);
  cout << "add_vertex() ok!\n";

  base.close();
  assert(base.is_closed());
  cout << "close() ok!\n";

  Polygon cpy = base;

  assert(base.is_closed());
  assert(base.size() == 3);
  assert(cpy.is_closed());
  assert(cpy.size() == 3);

  Polygon::Vertex_Iterator it_b(base);
  Polygon::Vertex_Iterator it_c(cpy);

  for ( ; it_b.has_current() and it_c.has_current(); it_b.next(), it_c.next())
    assert(Point(it_b.get_current_vertex()) ==
	   Point(it_c.get_current_vertex()));

  assert(not it_b.has_curr() and not it_c.has_curr());

  cout << "Copy constructor ok!\n";

  Polygon another = move(cpy);

  assert(not cpy.is_closed());
  assert(cpy.size() == 0);
  assert(another.is_closed());
  assert(another.size() == 3);

  cout << "Move constructor ok!\n";

  Polygon p;

  p.add_vertex(0, 0);   // 0
  p.add_vertex(10, 7);  // 1
  p.add_vertex(12, 3);  // 2
  p.add_vertex(20, 8);  // 3
  p.add_vertex(13, 17); // 4
  p.add_vertex(10, 12); // 5
  p.add_vertex(12, 14); // 6
  p.add_vertex(14, 9);  // 7
  p.add_vertex(8, 10);  // 8
  p.add_vertex(6, 14);  // 9
  p.add_vertex(10, 15); // 10
  p.add_vertex(7, 18);  // 11
  p.add_vertex(0, 16);  // 12
  p.add_vertex(1, 13);  // 13
  p.add_vertex(3, 15);  // 14
  p.add_vertex(5, 8);   // 15
  p.add_vertex(-2, 9);  // 16
  p.add_vertex(5, 5);   // 17

  p.close();
  
  auto triangulation = CuttingEarsTriangulation()(p);

  assert(triangulation.size() == 16);

  cout << "Triangulation by cutting ears ok!\n";

  return 0;
}
