#include <functional>
#include <optional>

#include <gtest/gtest.h>

#include <ahFunctional.H>
#include <htlist.H>
#include <tpl_dynArray.H>
#include <tpl_dynBinHeap.H>
#include <tpl_graph.H>
#include <tpl_sort_utils.H>

// Positive/negative checks for the constraints added in Phase 2 of
// aleph-concepts-plan.md: comparators on sorting and heaps, and callables on
// the functional layer (ah-dry.H, ah-dry-mixin.H, graph-dry.H, ahFunctional.H).
// Each constraint mirrors the call its function body makes, so the negative
// cases are exactly the ones that used to fail deep inside the library.

using namespace Aleph;

namespace
{
  struct IntLess
  {
    bool operator()(const int & a, const int & b) const { return a < b; }
  };

  struct NonConstLess
  {
    bool operator()(const int & a, const int & b) { return a < b; }
  };

  struct IsPositive
  {
    bool operator()(int x) const { return x > 0; }
  };

  struct ReturnsVoid
  {
    void operator()(int) const {}
  };

  struct MutatesInPlace
  {
    void operator()(int & x) const { ++x; }
  };

  struct MaybePositive
  {
    std::optional<int> operator()(int x) const
    {
      return x > 0 ? std::optional<int>(x) : std::nullopt;
    }
  };

  struct NeedsNothing
  {
    void operator()() const {}
  };

  struct Accumulate
  {
    int operator()(int acc, int x) const { return acc + x; }
  };

  struct NotAFold
  {
    void operator()(int, int) const {}
  };

  using Graph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;

  struct NodePred
  {
    bool operator()(Graph::Node *) const { return true; }
  };

  struct ArcPred
  {
    bool operator()(Graph::Arc *) const { return true; }
  };
} // namespace

// --- Hub concepts -----------------------------------------------------------

static_assert(CallableWith<IsPositive &, const int &>);
static_assert(CallableWith<ReturnsVoid &, int>);
static_assert(not CallableWith<IsPositive &>);
static_assert(not CallableWith<bool (std::optional<int>::*)() const, std::optional<int> &>);

static_assert(PredicateWith<IsPositive &, const int &>);
static_assert(not PredicateWith<ReturnsVoid &, const int &>);
// Contextual (not implicit) conversion to bool, as `if (op(x))` needs:
static_assert(PredicateWith<MaybePositive &, const int &>);
static_assert(not std::predicate<MaybePositive &, const int &>);

// --- Sorting ----------------------------------------------------------------

template <class Cmp>
concept can_introsort = requires(int * a, Cmp cmp) { Aleph::introsort(a, 3, cmp); };

static_assert(can_introsort<IntLess>);
static_assert(can_introsort<std::greater<int>>);
static_assert(can_introsort<bool (*)(const int &, const int &)>);
static_assert(not can_introsort<NonConstLess>);
static_assert(not can_introsort<ReturnsVoid>);

// --- Heaps ------------------------------------------------------------------

template <class Cmp>
concept can_name_heap = requires { typename DynBinHeap<int, Cmp>; };

static_assert(can_name_heap<IntLess>);
static_assert(not can_name_heap<NonConstLess>);

// --- Member functional layer (ah-dry.H) --------------------------------------

template <class Op>
concept can_filter = requires(const DynList<int> & l, Op op) { l.filter(op); };
template <class Op>
concept can_mutable_for_each = requires(DynList<int> & l, Op op) { l.mutable_for_each(op); };
template <class Op>
concept can_foldl = requires(const DynList<int> & l, Op op) { l.foldl(0, op); };
template <class Op>
concept can_partition = requires(const DynList<int> & l, Op op) { l.partition(op); };

static_assert(can_filter<IsPositive>);
static_assert(can_filter<MaybePositive>);
static_assert(not can_filter<ReturnsVoid>);
static_assert(can_mutable_for_each<MutatesInPlace>);
static_assert(can_foldl<Accumulate>);
static_assert(not can_foldl<NotAFold>);
static_assert(can_partition<IsPositive>);
// An int count is viable: it now reaches partition(size_t). Before the
// constraint, partition(Operation &&) won overload resolution with an exact
// match (Operation = int) and failed inside calling `op(item)`.
static_assert(can_partition<int>);

// --- Free functional layer (ahFunctional.H) ---------------------------------

template <class Op>
concept can_count_if = requires(const DynList<int> & l, Op op) { Aleph::count_if(l, op); };
template <class Op>
concept can_each = requires(Op op) { Aleph::each(size_t(0), size_t(1), op); };

static_assert(can_count_if<IsPositive>);
static_assert(not can_count_if<ReturnsVoid>);
static_assert(can_each<NeedsNothing>);
static_assert(not can_each<IsPositive>);

// --- Graph functional layer (graph-dry.H) -----------------------------------

template <class Op>
concept can_traverse_nodes = requires(const Graph & g, Op op) { g.traverse_nodes(op); };
template <class Op>
concept can_filter_arcs = requires(const Graph & g, Op op) { g.filter_arcs(op); };

static_assert(can_traverse_nodes<NodePred>);
static_assert(not can_traverse_nodes<ArcPred>);
static_assert(can_filter_arcs<ArcPred>);
static_assert(not can_filter_arcs<NodePred>);

// Former std::function parameters: the constraint is what std::function's
// constructor required (callable, result implicitly convertible to T).
template <class Op>
concept can_nodes_map = requires(const Graph & g, Op op) { g.nodes_map(op); };

static_assert(can_nodes_map<NodePred>);
static_assert(not can_nodes_map<ArcPred>);

TEST(AhConceptsFunctionalTest, ConstrainedCallsStillRun)
{
  DynList<int> l = {3, -1, 4};
  EXPECT_EQ(l.filter(IsPositive()).size(), 2u);
  EXPECT_EQ(l.filter(MaybePositive()).size(), 2u);
  EXPECT_EQ(l.foldl(0, Accumulate()), 6);
  EXPECT_EQ(l.partition(1).first.size(), 1u);
  EXPECT_EQ(Aleph::count_if(l, IsPositive()), 2u);

  int a[] = {3, 1, 2};
  Aleph::introsort(a, 3, IntLess());
  EXPECT_EQ(a[0], 1);

  Graph g;
  auto * n1 = g.insert_node(1);
  auto * n2 = g.insert_node(2);
  g.insert_arc(n1, n2, 7);
  EXPECT_TRUE(g.traverse_nodes(NodePred()));
  EXPECT_EQ(g.filter_arcs(ArcPred()).size(), 1u);
}

namespace
{
  int node_info(Graph::Node * p) { return p->get_info(); }
}

// These member functions used to take std::function<T(...)> with T in a
// deduced context, so passing a lambda without spelling T out did not compile
// (deduction of T from a lambda fails before the default could apply).
TEST(AhConceptsFunctionalTest, MemberMapsAndFoldsAcceptLambdasWithoutExplicitType)
{
  Graph g;
  auto * n1 = g.insert_node(1);
  auto * n2 = g.insert_node(2);
  g.insert_arc(n1, n2, 7);

  auto infos = g.nodes_map([](Graph::Node * p) { return p->get_info(); });
  EXPECT_EQ(infos.size(), 2u);
  EXPECT_EQ(g.nodes_map(&node_info).size(), 2u);
  EXPECT_EQ(g.arcs_map([](Graph::Arc * a) { return a->get_info(); }).get_first(), 7);
  EXPECT_EQ(g.arcs_map(n1, [](Graph::Arc * a) { return a->get_info(); }).get_first(), 7);
  EXPECT_EQ(g.foldl_nodes(0, [](const int & acc, Graph::Node * p) { return acc + p->get_info(); }), 3);
  EXPECT_EQ(g.foldl_arcs(0, [](const int & acc, Graph::Arc * a) { return acc + a->get_info(); }), 7);
  EXPECT_EQ(g.foldl_arcs(n1, 0, [](const int & acc, Graph::Arc * a) { return acc + a->get_info(); }), 7);

  // An explicit T still works and still converts the result.
  auto halves = g.template nodes_map<double>([](Graph::Node * p) { return p->get_info() / 2.0; });
  EXPECT_DOUBLE_EQ(halves.get_first() + halves.get_last(), 1.5);
}

// Uses an undirected List_Graph: a List_Digraph keeps each arc only in its
// source's list, so in-arcs are not visible from the target (in_degree 0).
TEST(AhConceptsFunctionalTest, MemberInOutArcMapsAndFoldsAcceptLambdas)
{
  Graph g;
  auto * a = g.insert_node(1);
  auto * b = g.insert_node(2);
  g.insert_arc(a, b, 5);

  auto arc_info = [](Graph::Arc * arc) { return arc->get_info(); };
  auto add_info = [](const int & acc, Graph::Arc * arc) { return acc + arc->get_info(); };

  EXPECT_EQ(g.template in_arcs_map<int>(b, arc_info).get_first(), 5);
  EXPECT_EQ(g.out_arcs_map(a, arc_info).get_first(), 5);
  EXPECT_EQ(g.foldl_in_arcs(b, 0, add_info), 5);
  EXPECT_EQ(g.foldl_out_arcs(a, 0, add_info), 5);
  EXPECT_TRUE(g.out_arcs_map(b, arc_info).is_empty());
}

// The 3-argument free overloads (g, op, filter) and (g, node, op) must stay
// unambiguous now that the operation is a template parameter.
TEST(AhConceptsFunctionalTest, FreeGraphOverloadsStayUnambiguous)
{
  Graph g;
  auto * n1 = g.insert_node(1);
  auto * n2 = g.insert_node(2);
  g.insert_arc(n1, n2, 7);

  int visits = 0;
  Aleph::for_each_arc(g, [&visits](Graph::Arc *) { ++visits; }, Dft_Show_Arc<Graph>());
  Aleph::for_each_arc(g, n1, [&visits](Graph::Arc *) { ++visits; });
  Aleph::for_each_arc(g, [&visits](Graph::Arc *) { ++visits; });
  EXPECT_EQ(visits, 3);
  EXPECT_TRUE(Aleph::forall_arc(g, ArcPred()));
  EXPECT_EQ((Aleph::nodes_map<Graph, long>(g, [](Graph::Node * p) { return p->get_info(); }).size()), 2u);
}

// nodes_map/arcs_map/map_in_arcs/map_out_arcs only accept a generic Op, so T
// (which appears only in the return type and the requires-clause, never in a
// parameter) is not deducible: an actual std::function<T(...)> argument, which
// the older std::function-parameter signature could deduce T from, used to
// need T spelled out explicitly. The compatibility overloads restore that.
TEST(AhConceptsFunctionalTest, MapHelpersDeduceTFromStdFunctionArguments)
{
  Graph g;
  auto * n1 = g.insert_node(1);
  auto * n2 = g.insert_node(2);
  auto * n3 = g.insert_node(3);
  g.insert_arc(n1, n2, 10);
  g.insert_arc(n1, n3, 20);
  g.insert_arc(n2, n3, 30);

  std::function<int(Graph::Node *)> nf = [](Graph::Node * p) { return p->get_info() * 100; };
  std::function<int(Graph::Arc *)> af = [](Graph::Arc * a) { return a->get_info() + 1; };

  EXPECT_EQ((Aleph::nodes_map<Graph>(g, nf).size()), 3u);
  EXPECT_EQ((Aleph::arcs_map<Graph>(g, af).size()), 3u);
  EXPECT_EQ((Aleph::arcs_map<Graph>(g, n1, af).size()), 2u);
  EXPECT_EQ((Aleph::map_in_arcs<Graph>(n3, af).size()), 2u);
  EXPECT_EQ((Aleph::map_out_arcs<Graph>(n1, af).size()), 2u);

  // The generic (lambda) and std::function overloads must both stay callable
  // with T explicit, without becoming ambiguous with each other.
  EXPECT_EQ((Aleph::nodes_map<Graph, int>(g, [](Graph::Node * p) { return p->get_info(); }).size()), 3u);
  EXPECT_EQ((Aleph::nodes_map<Graph, int>(g, nf).size()), 3u);
}
