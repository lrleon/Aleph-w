
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
# include <iostream>
# include <htlist.H>
# include <tpl_dynDlist.H>
# include <tpl_dynArray.H>
# include <tpl_dynSetTree.H>
# include <tpl_dynSetHash.H>
# include <tpl_dynBinHeap.H>
# include <tpl_dynArrayHeap.H>
# include <tpl_graph.H>
# include <tpl_sgraph.H>
# include <tpl_agraph.H>
# include <tpl_net.H>
# include <tpl_netcost.H>

using namespace std;
using namespace Aleph;

struct Foo
{
  int f1 = -1;
  string f2 = "hello";
  float f3 = 0;

  Foo() {}

  Foo(int __f1) : f1(__f1) {}

  Foo(string __f2) : f2(__f2) {}

  Foo(int __f1, const string & __f2) : f1(__f1), f2(__f2) {}

  Foo(int __f1, const string & __f2, float __f3) 
    : f1(__f1), f2(__f2), f3(__f3) {}

  friend ostream & operator << (ostream & s, const Foo & foo)
  {
    return s << "(" << foo.f1 << "," << foo.f2 << "," << foo.f3 << ")";
  }
};

template <template <typename> class C>
void test()
{
  int i = 7;
  string str = "salut";
  float f = 10e6;
  C<Foo> c;
  c.emplace(2);
  c.emplace(3, "hola");
  c.emplace(4, "adios", -1.0);
  c.emplace(5, str);
  c.emplace(6, str, f);
  c.emplace(i, str, 2.5);

  c.for_each([] (auto foo) { cout << foo; });
  cout << endl;
}

void test_graph()
{
  int i = 7;
  string str = "salut";
  float f = 10e6;

  {
    Array_Graph<Graph_Anode<Foo>, Graph_Aarc<Foo>> g;

    auto p1 = g.emplace_node(2);
    auto p2 = g.emplace_node(3, "hola");
    auto p3 = g.emplace_node(4, "adios", -1.0);
    auto p4 = g.emplace_node(5, str);
    auto p5 = g.emplace_node(6, str, f);
    auto p6 = g.emplace_node(i, str, 2.5);

    g.emplace_arc(p1, p2, 0);
    g.emplace_arc(p3, p4, i, str, f);
    g.emplace_arc(p5, p6, 0, "soyuz");
  }

  cout << endl
       << endl;

  {
    Net_Graph<Net_Node<Foo>, Net_Arc<Foo>> g;
    auto p1 = g.emplace_node(2);
    auto p2 = g.emplace_node(3, "hola");
    auto p3 = g.emplace_node(4, "adios", -1.0);
    auto p4 = g.emplace_node(5, str);
    auto p5 = g.emplace_node(6, str, f);
    auto p6 = g.emplace_node(i, str, 2.5);

    g.insert_arc(p1, p2, 0);
    g.emplace_arc(p3, p4, 0, 0, i, str, f);
    g.insert_arc(p5, p6, 0);
  }

  {
    Net_Cost_Graph<Net_Cost_Node<Foo>, Net_Cost_Arc<Foo>> g;
    auto p1 = g.emplace_node(2);
    auto p2 = g.emplace_node(3, "hola");
    auto p3 = g.emplace_node(4, "adios", -1.0);
    auto p4 = g.emplace_node(5, str);
    auto p5 = g.emplace_node(6, str, f);
    auto p6 = g.emplace_node(i, str, 2.5);

    g.insert_arc(p1, p2, 10, 10);
    g.emplace_arc(p3, p4, 0, 0, i, str, f);
    g.insert_arc(p5, p6, 20, 20);
 }
}

int main()
{
  test<DynList>();
  test<DynDlist>();
  test<DynArray>();


  test_graph();
}
