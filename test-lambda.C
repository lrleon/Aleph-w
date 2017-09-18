# include <tpl_binNodeUtils.H>

int main()
{
  BinNode<int> * p = nullptr;
  For_Each_Preorder<BinNode<int>>()(p, [] (BinNode<int> * p) {  ; });
  for_each_in_order(p, [] (BinNode<int> * p) { return ; });
  for_each_preorder(p, [] (BinNode<int> * p) { return ; });
}
