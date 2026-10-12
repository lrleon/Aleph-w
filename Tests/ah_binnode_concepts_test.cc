#include <gtest/gtest.h>

#include <tpl_avl.H>
#include <tpl_avlRk.H>
#include <tpl_binNode.H>
#include <tpl_binNodeUtils.H>
#include <tpl_binNodeXt.H>
#include <tpl_rand_tree.H>
#include <tpl_rb_tree.H>
#include <tpl_treap.H>
#include <tpl_treapRk.H>

// Positive/negative checks for BinNodeLike / RankedBinNodeLike (Phase 3 of
// aleph-concepts-plan.md) and for the node utilities they now constrain.

using namespace Aleph;

static_assert(BinNodeLike<BinNode<int>> and BinNodeLike<BinNodeVtl<int>>);
static_assert(BinNodeLike<AvlNode<int>> and BinNodeLike<RbNode<int>> and BinNodeLike<TreapNode<int>>);
static_assert(BinNodeLike<RandNode<int>> and BinNodeLike<BinNodeXt<int>> and BinNodeLike<AvlNodeRk<int>>);
// Utilities are also instantiated with const nodes, whose getL() returns by value.
static_assert(BinNodeLike<const BinNode<int>>);
static_assert(not BinNodeLike<int> and not BinNodeLike<DynList<int>>);

static_assert(RankedBinNodeLike<BinNodeXt<int>> and RankedBinNodeLike<AvlNodeRk<int>>);
static_assert(RankedBinNodeLike<Treap_Rk_Node<int>> and RankedBinNodeLike<RandNode<int>>);
static_assert(not RankedBinNodeLike<BinNode<int>> and not RankedBinNodeLike<AvlNode<int>>);

// select() on a node without subtree sizes used to fail inside
// tpl_binNodeXt.H ("no member named getCount"); now the call is not viable.
template <class Node>
concept can_select = requires(Node * r) { Aleph::select(r, size_t(0)); };
template <class Node>
concept can_compute_height = requires(Node * r) { computeHeightRec(r); };

static_assert(can_select<BinNodeXt<int>> and not can_select<BinNode<int>>);
static_assert(can_compute_height<BinNode<int>> and not can_compute_height<int>);

TEST(AhBinNodeConceptsTest, ConstrainedUtilitiesStillRun)
{
  using Node = BinNodeXt<int>;
  Node * root = Node::NullPtr;
  for (int k : {5, 2, 8, 1, 9})
    ASSERT_NE(insert_by_key_xt(root, new Node(k)), Node::NullPtr);

  EXPECT_EQ(KEY(Aleph::select(root, 0)), 1);
  EXPECT_EQ(KEY(Aleph::select(root, 4)), 9);
  EXPECT_EQ(computeHeightRec(root), 3u);
  destroyRec(root);
}

// Without a sentinel, nullptr is the empty tree and stays valid.
TEST(AhBinNodeConceptsTest, NullptrIsTheEmptyTreeWithoutSentinel)
{
  BinNode<int> * root = nullptr;
  EXPECT_EQ(searchInBinTree(root, 1), nullptr);
}

#ifndef NDEBUG
// With a sentinel (BinNodeXt), a nullptr root is a corrupted tree: debug
// builds report it instead of dereferencing it.
TEST(AhBinNodeConceptsDeathTest, NullptrRootInSentinelTreeIsReported)
{
  using Node = BinNodeXt<int>;
  Node * root = nullptr;
  EXPECT_DEATH((void) searchInBinTree(root, 1), "use Node::NullPtr");
}
#endif
