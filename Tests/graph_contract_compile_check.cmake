# The contract matrix of the graph templates, graph_contract_matrix_*_test.cc,
# instantiates and calls every public template of the graph headers that
# takes an arc filter, on every kind of graph it supports, with every filter
# of graph_filter_catalog.H and Dft_Show_Arc, on a const graph where it takes
# one. This script checks the combinations that it cannot hold:
#
# - known_failure: a combination that the documentation or the ArcFilter
#   concept promise and that does not compile yet. It must still fail, with
#   the first error that its defect gives: in the header that has it, or at
#   the call when what is missing is the overload or the constructor that
#   takes the filter. When a fix lands it compiles, and the check fails
#   asking to move it to the matrix.
# - must_not_compile: a combination that must never compile, such as a
#   filter that is not a predicate on arcs, which the ArcFilter concept must
#   reject where it is named.
# - must_compile: the controls, with the same headers and the calls that
#   work, so that a known failure fails for its defect and not for the unit.
#
# Each unit is compiled alone, with -std=c++20 -fsyntax-only, by the
# compiler of the build.
#
# Run with: cmake -DCXX_COMPILER=<compiler> -DSOURCE_DIR=<repository>
#                 -DWORK_DIR=<directory> -P graph_contract_compile_check.cmake

foreach(var CXX_COMPILER SOURCE_DIR WORK_DIR)
  if(NOT DEFINED ${var})
    message(FATAL_ERROR "${var} is required")
  endif()
endforeach()

set(FLAGS "-std=c++20" "-fsyntax-only" "-I${SOURCE_DIR}" "-I${SOURCE_DIR}/Tests")

# On macOS, Homebrew installs GSL and GMP outside the default search path
# of the compiler, as in ah_errors_else_check.cmake
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
  execute_process(
    COMMAND brew --prefix
    OUTPUT_VARIABLE BREW_PREFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE BREW_RC
    ERROR_QUIET
  )
  if(BREW_RC EQUAL 0 AND EXISTS "${BREW_PREFIX}/include")
    list(APPEND FLAGS "-I${BREW_PREFIX}/include")
  endif()
endif()

file(MAKE_DIRECTORY "${WORK_DIR}")
set_property(GLOBAL PROPERTY CONTRACT_FAILURES "")
set_property(GLOBAL PROPERTY CONTRACT_CHECKED 0)

# Compiles code as WORK_DIR/<id>.cc, and gives back the result of the
# compiler and its first error line
function(contract_compile id code out_result out_error)
  set(source "${WORK_DIR}/${id}.cc")
  file(WRITE "${source}" "${code}")
  execute_process(
    COMMAND "${CXX_COMPILER}" ${FLAGS} "${source}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE output)
  string(REGEX MATCH "[^\n]*: error:[^\n]*" first_error "${output}")
  set(${out_result} "${result}" PARENT_SCOPE)
  set(${out_error} "${first_error}" PARENT_SCOPE)
  get_property(n GLOBAL PROPERTY CONTRACT_CHECKED)
  math(EXPR n "${n} + 1")
  set_property(GLOBAL PROPERTY CONTRACT_CHECKED ${n})
endfunction()

# A combination that the documentation promises and that does not compile
# yet. It must fail, with a first error that matches the regular
# expression error; once it compiles, its fix has landed, and it goes to
# the matrix of graph_contract_matrix_*_test.cc.
function(known_failure id what error code)
  contract_compile(${id} "${code}" result first_error)
  if(result EQUAL 0)
    set_property(GLOBAL APPEND PROPERTY CONTRACT_FAILURES
      "${id} (${what}) compiles now: move it to the matrix of graph_contract_matrix_*_test.cc and remove it from here")
  elseif(NOT first_error MATCHES "${error}")
    set_property(GLOBAL APPEND PROPERTY CONTRACT_FAILURES
      "${id} (${what}) fails with another error than the known one (${error}): ${first_error}")
  endif()
endfunction()

# A combination that must never compile, with a first error that matches
# the regular expression error
function(must_not_compile id what error code)
  contract_compile(${id} "${code}" result first_error)
  if(result EQUAL 0)
    set_property(GLOBAL APPEND PROPERTY CONTRACT_FAILURES
      "${id} (${what}) compiles, and it must not")
  elseif(NOT first_error MATCHES "${error}")
    set_property(GLOBAL APPEND PROPERTY CONTRACT_FAILURES
      "${id} (${what}) fails with another error than the expected one (${error}): ${first_error}")
  endif()
endfunction()

# A unit that must compile: the control of the ones above, with the same
# headers and the calls that work
function(must_compile id what code)
  contract_compile(${id} "${code}" result first_error)
  if(NOT result EQUAL 0)
    set_property(GLOBAL APPEND PROPERTY CONTRACT_FAILURES
      "${id} (${what}) does not compile: ${first_error}")
  endif()
endfunction()

# ---------------------------------------------------------------------------
# The iterators and functions of tpl_graph.H and filter_iterator.H,
# graph-traverse.H, tpl_graph_utils.H, io_graph.H, generate_graph.H,
# generate_spanning_tree_picture.H, generate_df_tree.H, graph_to_tree.H,
# single_graph.H, warshall.H, eulerian.H and hamiltonian.H
# (graph_contract_matrix_core_test.cc)
# ---------------------------------------------------------------------------

# ---- The filtered iterators of tpl_graph.H and filter_iterator.H

# Filter_Iterator::set_filter() is only for a filter that can be assigned,
# so that the iterators can be instantiated with one that cannot: a lambda
# with captures, a reference or a const member
must_compile(filter_iterator_with_a_filter_that_cannot_be_assigned
  "Filter_Iterator, the base of Node_Arc_Iterator and Arc_Iterator, explicitly instantiated with a filter that cannot be assigned"
  [=[
#include <tpl_graph.H>
#include "graph_filter_catalog.H"
using GT = Aleph::List_Graph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>;
template class Aleph::Filter_Iterator<GT, GT::Arc_Iterator, graph_filters::Const_Member<GT>>;
]=])

must_compile(filter_iterator_with_filters_that_cannot_be_assigned
  "Filter_Iterator with an assignable filter, and the iterators used with one that is not"
  [=[
#include <tpl_graph.H>
#include "graph_filter_catalog.H"
using GT = Aleph::List_Graph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>;
template class Aleph::Filter_Iterator<GT, GT::Arc_Iterator, graph_filters::Hide_Mask<GT>>;
template class Aleph::Filter_Iterator<GT::Node *, GT::Node_Arc_Iterator, graph_filters::Hide_Mask<GT>>;
size_t f(const GT & g, GT::Node * p)
{
  using SA = graph_filters::Const_Member<GT>;
  Aleph::Arc_Iterator<GT, SA> it(g, SA(4));
  Aleph::Node_Arc_Iterator<GT, SA> jt(p, SA(4));
  it.reset_last();
  it.prev();
  jt.reset_last();
  jt.prev_ne();
  return it.count() + jt.count();
}
]=])

# The iterators of List_SGraph and List_SDigraph go only forward: the
# iterator of the singly linked list of the arcs of a node, HTList::Iterator,
# has its prev as a private member, and that of the set of arcs has none.
# So Filter_Iterator has prev(), prev_ne() and reset_last() only where the
# iterator under it moves back, and so the iterators that forward to them,
# Digraph_Iterator, Out_Iterator, In_Iterator and those of the graph classes:
# the error is at the call
must_not_compile(sgraph_iterator_prev
  "prev() of Node_Arc_Iterator on List_SGraph, which goes only forward"
  "sgraph_iterator_prev\\.cc:[0-9]+:[0-9]+: error"
  [=[
#include <tpl_sgraph.H>
using GT = Aleph::List_SGraph<Aleph::Graph_Snode<int>, Aleph::Graph_Sarc<int>>;
void f(GT::Node * p)
{
  Aleph::Node_Arc_Iterator<GT> it(p);
  it.prev();
}
]=])

must_not_compile(sgraph_iterator_reset_last
  "reset_last() of Arc_Iterator on List_SGraph, which goes only forward"
  "sgraph_iterator_reset_last\\.cc:[0-9]+:[0-9]+: error"
  [=[
#include <tpl_sgraph.H>
using GT = Aleph::List_SGraph<Aleph::Graph_Snode<int>, Aleph::Graph_Sarc<int>>;
void f(const GT & g)
{
  Aleph::Arc_Iterator<GT> it(g);
  it.reset_last();
}
]=])

must_not_compile(sgraph_out_iterator_prev
  "prev() of Out_Iterator on List_SDigraph, which forwards to Digraph_Iterator"
  "sgraph_out_iterator_prev\\.cc:[0-9]+:[0-9]+: error"
  [=[
#include <tpl_sgraph.H>
using GT = Aleph::List_SDigraph<Aleph::Graph_Snode<int>, Aleph::Graph_Sarc<int>>;
void f(GT::Node * p)
{
  Aleph::Out_Iterator<GT> it(p);
  it.prev();
}
]=])

must_not_compile(sgraph_out_iterator_end
  "end() of Out_Iterator on List_SGraph, which needs reset_last()"
  "sgraph_out_iterator_end\\.cc:[0-9]+:[0-9]+: error"
  [=[
#include <tpl_sgraph.H>
using GT = Aleph::List_SGraph<Aleph::Graph_Snode<int>, Aleph::Graph_Sarc<int>>;
void f(GT::Node * p)
{
  Aleph::Out_Iterator<GT> it(p);
  it.end();
}
]=])

must_not_compile(sgraph_in_iterator_reset_last
  "reset_last() of In_Iterator(g, p) on List_SDigraph, which walks the arcs of the graph"
  "sgraph_in_iterator_reset_last\\.cc:[0-9]+:[0-9]+: error"
  [=[
#include <tpl_sgraph.H>
using GT = Aleph::List_SDigraph<Aleph::Graph_Snode<int>, Aleph::Graph_Sarc<int>>;
void f(const GT & g, GT::Node * p)
{
  Aleph::In_Iterator<GT> it(g, p);
  it.reset_last();
}
]=])

must_not_compile(sgraph_member_iterator_prev
  "prev() of the Out_Iterator of List_SGraph"
  "sgraph_member_iterator_prev\\.cc:[0-9]+:[0-9]+: error"
  [=[
#include <tpl_sgraph.H>
using GT = Aleph::List_SGraph<Aleph::Graph_Snode<int>, Aleph::Graph_Sarc<int>>;
void f(GT::Node * p)
{
  GT::Out_Iterator it(p);
  it.prev();
}
]=])

must_compile(iterator_backward_moves
  "the backward moves of the iterators on List_Graph and List_Digraph, the control of the six above"
  [=[
#include <tpl_graph.H>
#include "graph_filter_catalog.H"
using GT = Aleph::List_Graph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>;
using DG = Aleph::List_Digraph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>;
using SA = graph_filters::Hide_Mask<GT>;
bool f(const GT & g, GT::Node * p, const DG & dg, DG::Node * q)
{
  Aleph::Node_Arc_Iterator<GT, SA> it(p, SA{4});
  it.prev();
  it.prev_ne();
  it.reset_last();
  Aleph::Arc_Iterator<GT, SA> jt(g, SA{4});
  jt.reset_last();
  jt.prev();
  Aleph::Out_Iterator<GT, SA> out(p, SA{4});
  out.reset_last();
  out.prev();
  out.end();
  Aleph::In_Iterator<GT, SA> in(p, SA{4});
  in.reset_last();
  in.prev();
  in.end();
  Aleph::In_Iterator<DG> din(dg, q);
  din.reset_last();
  din.prev();
  din.end();
  GT::Out_Iterator mout(p);
  mout.reset_last();
  mout.prev();
  mout.prev_ne();
  DG::In_Iterator min(dg, q);
  min.reset_last();
  min.prev();
  min.prev_ne();
  return it.has_curr() or jt.has_curr() or out.has_curr() or in.has_curr() or din.has_curr()
    or mout.has_curr() or min.has_curr();
}
]=])

must_compile(sgraph_iterator_forward
  "the forward moves of the iterators of List_SGraph and List_SDigraph, control of the ones above"
  [=[
#include <tpl_sgraph.H>
#include "graph_filter_catalog.H"
using GT = Aleph::List_SDigraph<Aleph::Graph_Snode<int>, Aleph::Graph_Sarc<int>>;
using SA = graph_filters::Hide_Mask<GT>;
size_t f(const GT & g, GT::Node * p)
{
  Aleph::Arc_Iterator<GT, SA> it(g, SA{4});
  Aleph::Node_Arc_Iterator<GT, SA> jt(p, SA{4});
  Aleph::Out_Iterator<GT, SA> out(p, SA{4});
  Aleph::In_Iterator<GT, SA> in(g, p, SA{4});
  it.next_ne();
  it.reset_first();
  jt.reset_first();
  out.next_ne();
  out.reset_first();
  in.reset_first();
  in.end();
  return it.count() + jt.count() + out.has_curr() + in.has_curr();
}
]=])

# The Arc_Iterator of List_SGraph and List_SDigraph declares a default
# constructor that its base, the iterator of a DynSetTree, deletes, so that
# the filtered Arc_Iterator cannot be built empty as on the other kinds
known_failure(sgraph_arc_iterator_default_constructor
  "the default constructor of Arc_Iterator on List_SGraph and List_SDigraph"
  "filter_iterator\\.H:[0-9]+:[0-9]+: error"
  [=[
#include <tpl_sgraph.H>
using GT = Aleph::List_SGraph<Aleph::Graph_Snode<int>, Aleph::Graph_Sarc<int>>;
bool f()
{
  Aleph::Arc_Iterator<GT> it;
  return it.has_container();
}
]=])

# Out_Iterator() = default and In_Iterator() = default are deleted:
# Digraph_Iterator, their base, has no default constructor
known_failure(digraph_iterator_default_constructors
  "the default constructors of Out_Iterator and In_Iterator, on every kind"
  "digraph_iterator_default_constructors\\.cc:[0-9]+:[0-9]+: error: .*deleted"
  [=[
#include <tpl_graph.H>
using GT = Aleph::List_Digraph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>;
bool f()
{
  Aleph::Out_Iterator<GT> it;
  return it.has_curr();
}
]=])

must_compile(iterator_default_constructors
  "the default constructors of Node_Arc_Iterator on every kind and of Arc_Iterator on List_Graph, the control of the two above"
  [=[
#include <tpl_graph.H>
#include <tpl_sgraph.H>
#include "graph_filter_catalog.H"
using LG = Aleph::List_Digraph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>;
using SG = Aleph::List_SGraph<Aleph::Graph_Snode<int>, Aleph::Graph_Sarc<int>>;
bool f(LG::Node * p)
{
  Aleph::Node_Arc_Iterator<SG, graph_filters::Hide_Mask<SG>> it;
  Aleph::Arc_Iterator<LG, graph_filters::Hide_Mask<LG>> jt;
  Aleph::Out_Iterator<LG> out(p);
  return it.has_container() or jt.has_container() or out.has_curr();
}
]=])

# ---- The arcs into a node

# A digraph keeps each arc only in the list of its source, so that the
# forms that read the arcs into a node from its list do not compile on a
# digraph type, a class built with the Digraph template (DigraphType): the
# first error is the static_assert of the form, whose message leads to the
# form that takes the graph. Every form on List_Digraph, and, with forms
# SOME, In_Iterator(p) and in_degree(p) on the other digraph types, which
# take the same static_asserts
function(in_forms_of_the_node_on_a_digraph kind header type forms)
  string(CONCAT head "#include <${header}>\nusing GT = ${type};\n"
                     "using Arc = GT::Arc;\nvoid f(GT::Node * p)\n{\n")
  set(ok "[] (Arc *) { return true; }")
  set(same "[] (const int & n, Arc *) { return n; }")
  set(failed "error: static.assert.*")
  set(rest "does not see the arcs into p on a digraph")
  must_not_compile(in_iterator_of_the_node_on_${kind} "In_Iterator(p) on ${kind}"
    "${failed}In_Iterator\\(p\\) ${rest}"
    "${head}  Aleph::In_Iterator<GT> it(p);\n  (void) it.has_curr();\n}\n")
  if(forms STREQUAL "SOME")
    must_not_compile(in_degree_of_the_node_on_${kind} "in_degree(p) on ${kind}"
      "${failed}in_degree\\(p\\) ${rest}"
      "${head}  (void) Aleph::in_degree<GT>(p);\n}\n")
  else()
    must_not_compile(member_in_iterator_of_the_node_on_${kind} "GT::In_Iterator(p) on ${kind}"
      "${failed}In_Iterator\\(p\\) ${rest}"
      "${head}  GT::In_Iterator it(p);\n  (void) it.has_curr();\n}\n")
    must_not_compile(in_degree_of_the_node_on_${kind} "in_degree(p) on ${kind}"
      "${failed}in_degree\\(p\\) ${rest}"
      "${head}  (void) Aleph::in_degree<GT>(p);\n}\n")
    must_not_compile(in_nodes_of_the_node_on_${kind} "in_nodes(p) on ${kind}"
      "${failed}in_nodes\\(p\\) ${rest}"
      "${head}  (void) Aleph::in_nodes<GT>(p);\n}\n")
    must_not_compile(in_arcs_of_the_node_on_${kind} "in_arcs(p) on ${kind}"
      "${failed}in_arcs\\(p\\) ${rest}"
      "${head}  (void) Aleph::in_arcs<GT>(p);\n}\n")
    must_not_compile(in_pairs_of_the_node_on_${kind} "in_pairs(p) on ${kind}"
      "${failed}in_pairs\\(p\\) ${rest}"
      "${head}  (void) Aleph::in_pairs<GT>(p);\n}\n")
    must_not_compile(traverse_in_arcs_of_the_node_on_${kind} "traverse_in_arcs(p, op) on ${kind}"
      "${failed}traverse_in_arcs\\(p, op\\) ${rest}"
      "${head}  (void) Aleph::traverse_in_arcs<GT>(p, ${ok});\n}\n")
    must_not_compile(for_each_in_arc_of_the_node_on_${kind} "for_each_in_arc(p, op) on ${kind}"
      "${failed}for_each_in_arc\\(p, op\\) ${rest}"
      "${head}  Aleph::for_each_in_arc<GT>(p, [] (Arc *) {});\n}\n")
    must_not_compile(all_in_arc_of_the_node_on_${kind} "all_in_arc(p, op) on ${kind}"
      "${failed}all_in_arc\\(p, op\\) ${rest}"
      "${head}  (void) Aleph::all_in_arc<GT>(p, ${ok});\n}\n")
    must_not_compile(exists_in_arc_of_the_node_on_${kind} "exists_in_arc(p, op) on ${kind}"
      "${failed}exists_in_arc\\(p, op\\) ${rest}"
      "${head}  (void) Aleph::exists_in_arc<GT>(p, ${ok});\n}\n")
    must_not_compile(search_in_arc_of_the_node_on_${kind} "search_in_arc(p, op) on ${kind}"
      "${failed}search_in_arc\\(p, op\\) ${rest}"
      "${head}  (void) Aleph::search_in_arc<GT>(p, ${ok});\n}\n")
    must_not_compile(map_in_arcs_of_the_node_on_${kind} "map_in_arcs(p, op) on ${kind}"
      "${failed}map_in_arcs\\(p, op\\) ${rest}"
      "${head}  (void) Aleph::map_in_arcs<GT, int>(p, [] (Arc *) { return 1; });\n}\n")
    must_not_compile(foldl_in_arcs_of_the_node_on_${kind} "foldl_in_arcs(p, init, op) on ${kind}"
      "${failed}foldl_in_arcs\\(p, init, op\\) ${rest}"
      "${head}  (void) Aleph::foldl_in_arcs<GT, int>(p, 0, ${same});\n}\n")
    must_not_compile(filter_in_arcs_of_the_node_on_${kind} "filter_in_arcs(p, cond) on ${kind}"
      "${failed}filter_in_arcs\\(p, cond\\) ${rest}"
      "${head}  (void) Aleph::filter_in_arcs<GT>(p, ${ok});\n}\n")
  endif()

  # The control: the forms that take the graph, on the same digraph type
  must_compile(in_forms_of_the_graph_on_${kind} "the forms that take the graph on ${kind}" "
#include <${header}>
#include <functional>
using GT = ${type};
using Arc = GT::Arc;
size_t f(const GT & g, GT::Node * p)
{
  auto ok = ${ok};
  size_t n = 0;
  for (Aleph::In_Iterator<GT> it(g, p); it.has_curr(); it.next_ne())
    ++n;
  for (GT::In_Iterator it(g, p); it.has_curr(); it.next_ne())
    ++n;
  for (auto it = g.get_in_it(p); it.has_curr(); it.next_ne())
    ++n;
  n += Aleph::in_degree(g, p) + Aleph::in_nodes(g, p).size() + Aleph::in_arcs(g, p).size()
       + Aleph::in_pairs(g, p).size();
  n += Aleph::traverse_in_arcs(g, p, ok) + Aleph::all_in_arc(g, p, ok)
       + Aleph::exists_in_arc(g, p, ok) + (Aleph::search_in_arc(g, p, ok) != nullptr);
  Aleph::for_each_in_arc(g, p, [&n] (Arc *) { ++n; });
  std::function<int (Arc *)> one = [] (Arc *) { return 1; };
  n += Aleph::map_in_arcs<GT, int>(g, p, one).size() + Aleph::map_in_arcs(g, p, one).size();
  n += size_t(Aleph::foldl_in_arcs<GT, int>(g, p, 0, [] (const int & k, Arc *) { return k + 1; }));
  n += Aleph::filter_in_arcs(g, p, ok).size();
  const Aleph::In_Arcs_Index<GT> index(g);
  for (auto it = index.get_it(p); it.has_curr(); it.next_ne())
    ++n;
  return n + index.in_degree(p) + g.in_degree(p) + g.in_arcs(p).size();
}
")
endfunction()

in_forms_of_the_node_on_a_digraph(List_Digraph tpl_graph.H
  "Aleph::List_Digraph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>" ALL)
in_forms_of_the_node_on_a_digraph(List_SDigraph tpl_sgraph.H
  "Aleph::List_SDigraph<Aleph::Graph_Snode<int>, Aleph::Graph_Sarc<int>>" SOME)
in_forms_of_the_node_on_a_digraph(Array_Digraph tpl_agraph.H
  "Aleph::Array_Digraph<Aleph::Graph_Anode<int>, Aleph::Graph_Aarc<int>>" SOME)
in_forms_of_the_node_on_a_digraph(MCF_Graph tpl_multicommodity.H "Aleph::MCF_Graph<>" SOME)

# The controls: the forms that take only the node keep compiling on the
# types that are not directed, and on the networks, which keep each arc in
# the lists of both ends
function(in_forms_of_the_node_compile kind header type)
  must_compile(in_forms_of_the_node_on_${kind} "the forms that take only the node on ${kind}" "
#include <${header}>
#include <functional>
using GT = ${type};
using Arc = GT::Arc;
size_t f(const GT & g, GT::Node * p)
{
  auto ok = [] (Arc *) { return true; };
  size_t n = 0;
  for (Aleph::In_Iterator<GT> it(p); it.has_curr(); it.next_ne())
    ++n;
  for (Aleph::In_Iterator<GT> it(g, p); it.has_curr(); it.next_ne())
    ++n;
  for (GT::In_Iterator it(p); it.has_curr(); it.next_ne())
    ++n;
  for (Aleph::_In_Iterator<GT> it(p); it.has_curr(); it.next_ne())
    ++n;
  n += Aleph::in_degree<GT>(p) + Aleph::in_nodes<GT>(p).size() + Aleph::in_arcs<GT>(p).size()
       + Aleph::in_pairs<GT>(p).size() + Aleph::in_degree(g, p);
  n += Aleph::traverse_in_arcs<GT>(p, ok) + Aleph::all_in_arc<GT>(p, ok)
       + Aleph::exists_in_arc<GT>(p, ok) + (Aleph::search_in_arc<GT>(p, ok) != nullptr);
  Aleph::for_each_in_arc<GT>(p, [&n] (Arc *) { ++n; });
  std::function<int (Arc *)> one = [] (Arc *) { return 1; };
  n += Aleph::map_in_arcs<GT, int>(p, [] (Arc *) { return 1; }).size()
       + Aleph::map_in_arcs<GT>(p, one).size();
  n += size_t(Aleph::foldl_in_arcs<GT, int>(p, 0, [] (const int & k, Arc *) { return k + 1; }));
  return n + Aleph::filter_in_arcs<GT>(p, ok).size() + Aleph::In_Arcs_Index<GT>(g).in_degree(p);
}
")
endfunction()

in_forms_of_the_node_compile(List_Graph tpl_graph.H
  "Aleph::List_Graph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>")
in_forms_of_the_node_compile(List_SGraph tpl_sgraph.H
  "Aleph::List_SGraph<Aleph::Graph_Snode<int>, Aleph::Graph_Sarc<int>>")
in_forms_of_the_node_compile(Array_Graph tpl_agraph.H
  "Aleph::Array_Graph<Aleph::Graph_Anode<int>, Aleph::Graph_Aarc<int>>")
in_forms_of_the_node_compile(Net_Graph tpl_net.H "Aleph::Net_Graph<>")

# ---- The functions of tpl_graph.H

must_not_compile(arc_filter_of_for_each_arc
  "for_each_arc() with a filter that is not a predicate on an arc"
  "arc_filter_of_for_each_arc\\.cc:[0-9]+:[0-9]+: error"
  [=[
#include <tpl_graph.H>
using GT = Aleph::List_Graph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>;
struct On_Nodes { bool operator () (GT::Node *) const { return true; } };
void f(const GT & g)
{
  Aleph::for_each_arc(g, [] (GT::Arc *) {}, On_Nodes());
}
]=])

must_not_compile(arc_filter_of_copy_graph
  "Copy_Graph with a filter that is not a predicate on an arc"
  "arc_filter_of_copy_graph\\.cc:[0-9]+:[0-9]+: error"
  [=[
#include <tpl_graph.H>
using GT = Aleph::List_Graph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>;
struct On_Nodes { bool operator () (GT::Node *) const { return true; } };
void f(GT & tgt, GT & src)
{
  Aleph::Copy_Graph<GT, Aleph::Dft_Show_Node<GT>, On_Nodes> copy;
  copy(tgt, src);
}
]=])

must_not_compile(copy_graph_into_a_const_graph
  "Copy_Graph into a const graph"
  "copy_graph_into_a_const_graph\\.cc:[0-9]+:[0-9]+: error"
  [=[
#include <tpl_graph.H>
using GT = Aleph::List_Graph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>;
void f(const GT & tgt, GT & src)
{
  Aleph::Copy_Graph<GT> copy;
  copy(tgt, src);
}
]=])

must_compile(copy_graph_with_the_filters
  "Copy_Graph with each kind of filter, the control of the two above"
  [=[
#include <tpl_graph.H>
#include "graph_filter_catalog.H"
using GT = Aleph::List_Graph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>;
void f(GT & tgt, GT & src)
{
  const unsigned long mask = 4;
  using RM = graph_filters::Reference_Member<GT>;
  Aleph::Copy_Graph<GT, Aleph::Dft_Show_Node<GT>, RM> by_reference{RM(mask)};
  by_reference(tgt, src);
  auto lambda = graph_filters::make_capturing_lambda<GT>(mask);
  Aleph::Copy_Graph<GT, Aleph::Dft_Show_Node<GT>, decltype(lambda)> by_lambda{lambda};
  by_lambda(tgt, src);
}
]=])

# ---- tpl_graph_utils.H and tpl_dfs_stack.H

must_not_compile(arc_filter_of_depth_first_traversal
  "Depth_First_Traversal with a filter that is not a predicate on an arc"
  "arc_filter_of_depth_first_traversal\\.cc:[0-9]+:[0-9]+: error"
  [=[
#include <tpl_graph_utils.H>
using GT = Aleph::List_Graph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>;
struct On_Nodes { bool operator () (GT::Node *) const { return true; } };
Aleph::Depth_First_Traversal<GT, Aleph::Default_Visit_Op<GT>, On_Nodes> dft;
]=])

# ---- io_graph.H

# IO_Graph(g) builds the arc filter with AF(), and set_arc_filter() assigns
# it: a filter without a default constructor, or one that cannot be
# assigned, goes to the constructor that takes it, and the others fail at
# the call
must_not_compile(io_graph_needs_default_filter
  "IO_Graph(g) with a filter without a default constructor"
  "io_graph_needs_default_filter\\.cc:[0-9]+:[0-9]+: error"
  [=[
#include <io_graph.H>
#include "graph_filter_catalog.H"
using GT = Aleph::List_Graph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>;
using SA = graph_filters::Reference_Member<GT>;
void f(GT & g)
{
  Aleph::IO_Graph<GT, Aleph::Dft_Load_Node<GT>, Aleph::Dft_Store_Node<GT>, Aleph::Dft_Load_Arc<GT>,
                  Aleph::Dft_Store_Arc<GT>, Aleph::Dft_Show_Node<GT>, SA> io(g);
}
]=])

must_not_compile(io_graph_set_arc_filter_needs_assignment
  "IO_Graph::set_arc_filter(), which assigns, with a filter that cannot be assigned"
  "io_graph_set_arc_filter_needs_assignment\\.cc:[0-9]+:[0-9]+: error"
  [=[
#include <io_graph.H>
#include "graph_filter_catalog.H"
using GT = Aleph::List_Graph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>;
using SA = graph_filters::Const_Member<GT>;
void f(GT & g)
{
  Aleph::IO_Graph<GT, Aleph::Dft_Load_Node<GT>, Aleph::Dft_Store_Node<GT>, Aleph::Dft_Load_Arc<GT>,
                  Aleph::Dft_Store_Arc<GT>, Aleph::Dft_Show_Node<GT>, SA> io(g);
  io.set_arc_filter(SA(4));
}
]=])

must_compile(io_graph_with_an_assignable_filter
  "IO_Graph with a filter that has a default constructor and can be assigned, and with one in the constructor, the control of the two above"
  [=[
#include <sstream>
#include <io_graph.H>
#include "graph_filter_catalog.H"
using GT = Aleph::List_Graph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>;
using SA = graph_filters::Hide_Mask<GT>;
void f(GT & g)
{
  Aleph::IO_Graph<GT, Aleph::Dft_Load_Node<GT>, Aleph::Dft_Store_Node<GT>, Aleph::Dft_Load_Arc<GT>,
                  Aleph::Dft_Store_Arc<GT>, Aleph::Dft_Show_Node<GT>, SA> io(g);
  io.set_arc_filter(SA{4});
  std::ostringstream out;
  io.save_in_text_mode(out);
}
void h(GT & g)
{
  using RM = graph_filters::Reference_Member<GT>;
  const unsigned long mask = 4;
  Aleph::IO_Graph<GT, Aleph::Dft_Load_Node<GT>, Aleph::Dft_Store_Node<GT>, Aleph::Dft_Load_Arc<GT>,
                  Aleph::Dft_Store_Arc<GT>, Aleph::Dft_Show_Node<GT>, RM> io(g, RM(mask));
  std::ostringstream out;
  io.save_in_text_mode(out);
}
]=])

# ---- graph_to_tree.H

# ---- warshall.H

# ---- generate_spanning_tree_picture.H ("net" is a layout of graphpic)

# ---- eulerian.H

must_not_compile(arc_filter_of_test_eulerian
  "Test_Eulerian with a filter that is not a predicate on an arc"
  "arc_filter_of_test_eulerian\\.cc:[0-9]+:[0-9]+: error"
  [=[
#include <eulerian.H>
using GT = Aleph::List_Graph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>;
struct On_Nodes { bool operator () (GT::Node *) const { return true; } };
Aleph::Test_Eulerian<GT, Aleph::Dft_Show_Node<GT>, On_Nodes> test;
]=])

# ---- The entry points that also take the filter only as a type

# The drawings of generate_graph.H and generate_spanning_tree_picture.H,
# warshall_compute_transitive_clausure(), through Bit_Mat_Graph, and
# graph_to_tree_node() take the filter object as their last parameter, or
# their classes in their constructors, which the matrix calls with every
# filter; without it they build SA(), as here.
must_compile(entry_points_with_a_filter_type
  "the drawings, Warshall and graph_to_tree_node() with Hide_Mask as a type, which they build with SA()"
  [=[
#include <fstream>
#include <sstream>
#include <string>
#include <generate_graph.H>
#include <generate_spanning_tree_picture.H>
#include <warshall.H>
#include <graph_to_tree.H>
#include "graph_filter_catalog.H"
using GT = Aleph::List_Digraph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>;
using HM = graph_filters::Hide_Mask<GT>;
struct Text { template <class T> std::string operator () (T *) const { return ""; } };
struct To_Key
{
  void operator () (GT::Node * p, Aleph::Tree_Node<int> * t) const { t->get_key() = p->get_info(); }
};
using NA = Aleph::Dft_Node_Attr<GT>;
using AA = Aleph::Dft_Arc_Attr<GT>;
using SN = Aleph::Dft_Show_Node<GT>;
using D = Aleph::Dummy_Attr<GT>;
void f(GT & g, GT::Node * p)
{
  std::ostringstream out;
  Aleph::generate_graphpic<GT, Text, Text, Text, Text, HM>(g, 1.0, 1.0, out);
  Aleph::generate_graphviz<GT, Text, Text, D, D, D, D, HM, SN>(g, out);
  Aleph::generate_graphviz<GT, NA, AA, SN, HM>(g, out);
  Aleph::digraph_graphviz<GT, NA, AA, SN, HM>(g, out);
  (void) Aleph::rank_graphviz<GT, NA, AA, SN, HM>(g, out);
  Aleph::To_Graphviz<GT, NA, AA, SN, HM> dot;
  dot(g, out);
  dot.digraph(g, out);
  dot.ranks(g, out);
  Aleph::Generate_Graphviz<GT, Text, Text, D, D, D, D, HM>()(g, out);
  Aleph::generate_cross_graph<GT, Text, Text, Text, Text, HM>(g, 2, 1.0, 1.0, out);
  Aleph::generate_cross_graph<GT, Text, Text, HM>(g, 2, 1.0, 1.0, out);
  Aleph::generate_net_graph<GT, Text, Text, Text, Text, HM>(g, 2, 1.0, 1.0, out);
  Aleph::generate_net_graph<GT, Text, Text, HM>(g, 2, 1.0, 1.0, out);
  std::ofstream file;
  generate_cross_spanning_tree<GT, Text, Text, HM>(g, 2, 1.0, 1.0, file);
  Aleph::Bit_Mat_Graph<GT, HM> mat(g);
  Aleph::warshall_compute_transitive_clausure(g, mat);
  Aleph::destroy_tree(Aleph::graph_to_tree_node<GT, int, To_Key, HM>(g, p));
}
]=])

# ---------------------------------------------------------------------------
# The searches: tpl_find_path.H, Bidirectional_BFS.H, tpl_components.H,
# tpl_cut_nodes.H, tpl_spanning_tree.H, tpl_test_acyclique.H,
# tpl_test_connectivity.H, tpl_test_cycle.H and tpl_test_path.H
# (graph_contract_matrix_search_test.cc)
# ---------------------------------------------------------------------------

# The ArcFilter concept of ah-graph-concepts.H keeps out a filter that is
# not a predicate on an arc, here one on a node
must_not_compile(find_path_depth_first_node_predicate
  "Find_Path_Depth_First with a predicate on nodes for a filter"
  "error: .*constraint"
  [=[
#include <tpl_find_path.H>
using GT = Aleph::List_Graph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>;
struct On_Nodes
{
  bool operator () (GT::Node *) const { return true; }
};
Aleph::Find_Path_Depth_First<GT, Aleph::Node_Arc_Iterator, On_Nodes> * p = nullptr;
]=])

# ArcFilter also asks a filter to be copy constructible, since each iterator
# and each frame of a search keeps a copy of it: a filter that can only be
# moved is rejected where the concept is named
must_not_compile(find_path_depth_first_move_only_filter
  "Find_Path_Depth_First with a filter that can be moved and not copied"
  "error: .*constraint"
  [=[
#include <tpl_find_path.H>
using GT = Aleph::List_Graph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>;
struct Move_Only
{
  Move_Only() = default;
  Move_Only(Move_Only &&) = default;
  Move_Only(const Move_Only &) = delete;
  bool operator () (GT::Arc *) const { return true; }
};
Aleph::Find_Path_Depth_First<GT, Aleph::Node_Arc_Iterator, Move_Only> * p = nullptr;
]=])

must_not_compile(test_connectivity_node_predicate
  "Test_Connectivity with a predicate on nodes for a filter"
  "error: .*constraint"
  [=[
#include <tpl_test_connectivity.H>
using GT = Aleph::List_Graph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>;
struct On_Nodes
{
  bool operator () (GT::Node *) const { return true; }
};
Test_Connectivity<GT, On_Nodes> * p = nullptr;
]=])

# ---------------------------------------------------------------------------
# The structure: Tarjan.H, topological_sort.H, Dominators.H, tpl_kgraph.H,
# Karger.H, Stoer_Wagner.H, Planarity_Test.H and Graph_Coloring.H
# (graph_contract_matrix_structure_test.cc)
# ---------------------------------------------------------------------------

must_compile(structure_controls
  "the structure templates with filters as types, which they build with SA(), and as objects"
  [=[
#include <Tarjan.H>
#include <tpl_kgraph.H>
#include <Karger.H>
#include <Stoer_Wagner.H>
#include <Graph_Coloring.H>
#include "graph_filter_catalog.H"

using GT = Aleph::List_Graph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>;
using DG = Aleph::List_Digraph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>;

bool tarjan(const DG & g)
{
  using SA = graph_filters::Hide_Mask<DG>;
  return Aleph::Tarjan_Connected_Components<DG, Aleph::Out_Iterator, SA>(SA{4}).has_cycle(g);
}

long kgraph(GT & g)
{
  using SA = graph_filters::Hide_Mask<GT>;
  return Aleph::edge_connectivity<GT, Aleph::Random_Preflow_Maximum_Flow, SA>(g);
}

size_t karger(GT & g)
{
  using SA = graph_filters::Hide_Mask<GT>;
  return Aleph::Karger_Min_Cut<GT, SA>(1, SA{4}).compute_min_cut_size(g);
}

template class Aleph::Karger_Min_Cut<GT, graph_filters::Hide_Mask<GT>>;

int stoer_wagner(GT & g)
{
  Aleph::Stoer_Wagner_Min_Cut<GT, Aleph::Dft_Dist<GT>, graph_filters::Const_Member<GT>> sw;
  return sw.min_cut_weight(g);
}

size_t coloring(const GT & g, Aleph::DynMapTree<GT::Node *, size_t> & colors)
{
  return Aleph::greedy_coloring<GT, graph_filters::Throwing_Copy<GT>>(g, colors);
}
]=])

must_not_compile(tarjan_with_a_filter_that_is_not_a_predicate
  "Tarjan_Connected_Components with an int as the filter, which ArcFilter rejects"
  "error: .*constraint"
  [=[
#include <Tarjan.H>

using GT = Aleph::List_Digraph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>;

bool f(const GT & g)
{
  return Aleph::Tarjan_Connected_Components<GT, Aleph::Out_Iterator, int>().has_cycle(g);
}
]=])

# ---------------------------------------------------------------------------
# The paths: shortest_path_common.H, Dijkstra.H, AStar.H, IDA_Star.H,
# Dial.H, Zero_One_BFS.H, Bellman_Ford.H, Johnson.H, Floyd_Warshall.H,
# K_Shortest_Paths.H, Min_Mean_Cycle.H, Howard_Min_Mean_Cycle.H,
# Negative_Cycles.H, Kruskal.H and Prim.H
# (graph_contract_matrix_paths_*_test.cc)
# ---------------------------------------------------------------------------

# A filter of nodes is not a filter of arcs: the ArcFilter constraint of
# the class and of the function templates rejects it at the call
must_not_compile(paths_node_filter_as_arc_filter_class
  "Dijkstra_Min_Paths with a predicate on nodes as its arc filter"
  "\\.cc:[0-9]+:[0-9]+: error: [^\n]*constraint"
  [=[
#include <Dijkstra.H>
using G = Aleph::List_Graph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>;
struct On_Nodes
{
  bool operator()(G::Node *) const { return true; }
};
Aleph::Dijkstra_Min_Paths<G, Aleph::Dft_Dist<G>, Aleph::Node_Arc_Iterator, On_Nodes> * p = nullptr;
]=])

must_not_compile(paths_node_filter_as_arc_filter_function
  "karp_minimum_mean_cycle() with a predicate on nodes as its arc filter"
  "\\.cc:[0-9]+:[0-9]+: error: no matching function"
  [=[
#include <Min_Mean_Cycle.H>
using G = Aleph::List_Digraph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>;
struct On_Nodes
{
  bool operator()(G::Node *) const { return true; }
};
void f(const G & g)
{
  Aleph::karp_minimum_mean_cycle(g, Aleph::Dft_Dist<G>(), On_Nodes());
}
]=])

# ---------------------------------------------------------------------------
# The matchings and the trees: tpl_bipartite.H, Blossom.H,
# Blossom_Weighted.H, Min_Cost_Matching.H, LCA.H, HLD.H,
# Tree_Decomposition.H and Tree_DP.H
# (graph_contract_matrix_matching_test.cc, graph_contract_matrix_trees_test.cc)
# ---------------------------------------------------------------------------

# A predicate on nodes is not an arc filter: the matchings and the tree
# classes, which constrain their filter with ArcFilter, must reject it at
# the call, not inside their bodies
must_not_compile(general_matching_node_filter
  "compute_maximum_cardinality_general_matching() with a predicate on nodes as its arc filter"
  "no matching function for call to"
  [=[
#include <tpl_graph.H>
#include <Blossom.H>

using G = Aleph::List_Graph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>;

void f(const G & g)
{
  Aleph::DynDlist<G::Arc *> m;
  Aleph::compute_maximum_cardinality_general_matching(g, m, [] (G::Node *) { return true; });
}
]=])

must_not_compile(hopcroft_karp_node_filter
  "hopcroft_karp_matching() with a predicate on nodes as its arc filter type"
  "no matching function for call to"
  [=[
#include <tpl_graph.H>
#include <tpl_bipartite.H>

using G = Aleph::List_Graph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>;

struct Node_Predicate
{
  bool operator () (G::Node *) const { return true; }
};

void f(const G & g)
{
  Aleph::DynDlist<G::Arc *> m;
  Aleph::hopcroft_karp_matching<G, Node_Predicate>(g, m);
}
]=])

must_not_compile(lca_node_filter
  "Gen_Binary_Lifting_LCA with a predicate on nodes as its arc filter"
  "constraint"
  [=[
#include <tpl_graph.H>
#include <LCA.H>

using G = Aleph::List_Graph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>;

struct Node_Predicate
{
  bool operator () (G::Node *) const { return true; }
};

void f(const G & g)
{
  Aleph::Gen_Binary_Lifting_LCA<G, Node_Predicate> lca(g, Node_Predicate());
}
]=])

# The control of the three above: the same calls with a predicate on arcs
must_compile(matching_and_lca_arc_filter
  "compute_maximum_cardinality_general_matching(), hopcroft_karp_matching() and Gen_Binary_Lifting_LCA with a predicate on arcs"
  [=[
#include <tpl_graph.H>
#include <tpl_bipartite.H>
#include <Blossom.H>
#include <LCA.H>

using G = Aleph::List_Graph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>;

struct Arc_Predicate
{
  bool operator () (G::Arc *) const { return true; }
};

void f(const G & g)
{
  Aleph::DynDlist<G::Arc *> m;
  Aleph::compute_maximum_cardinality_general_matching(g, m, [] (G::Arc *) { return true; });
  Aleph::hopcroft_karp_matching<G, Arc_Predicate>(g, m);
  Aleph::Gen_Binary_Lifting_LCA<G, Arc_Predicate> lca(g, Arc_Predicate());
}
]=])

# ---------------------------------------------------------------------------
# The views: tpl_const_graph_view.H
# ---------------------------------------------------------------------------

# The ends of an arc are public void *, so that from a const arc of a
# Const_Graph_View a static_cast gives a node that can be modified. That way
# out is documented as a limit of the view; without the cast there is none,
# and there must never be one
must_not_compile(const_view_arc_gives_no_modifiable_node_without_a_cast
  "an Out_Iterator over the graph from the source of a const arc of its view, without a cast"
  "const_view_arc_gives_no_modifiable_node_without_a_cast\\.cc:[0-9]+:[0-9]+: error"
  [=[
#include <tpl_graph.H>
#include <tpl_const_graph_view.H>
using GT = Aleph::List_Digraph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>;
void f(const GT & g)
{
  const auto view = Aleph::const_view(g);
  const GT::Arc * arc = view.get_first_arc();
  Aleph::Out_Iterator<GT> it(arc->src_node);
  it.get_curr()->get_info() = 1;
}
]=])

must_compile(const_view_arc_reads_its_source
  "the control of the one above: the source of a const arc, read through the view"
  [=[
#include <tpl_graph.H>
#include <tpl_const_graph_view.H>
using GT = Aleph::List_Digraph<Aleph::Graph_Node<int>, Aleph::Graph_Arc<int>>;
int f(const GT & g)
{
  const auto view = Aleph::const_view(g);
  const GT::Arc * arc = view.get_first_arc();
  return arc->get_info() + int(view.get_src_node(arc) != nullptr);
}
]=])

get_property(failures GLOBAL PROPERTY CONTRACT_FAILURES)
get_property(checked GLOBAL PROPERTY CONTRACT_CHECKED)
if(failures)
  string(REPLACE ";" "\n" failures "${failures}")
  message(FATAL_ERROR "${failures}")
endif()
message(STATUS "${checked} units checked")
