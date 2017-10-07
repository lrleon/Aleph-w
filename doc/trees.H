/**
   @defgroup Arboles Trees
   @{ 

   `Aleph-w` (\f$\aleph_\omega\f$) mamages several types of trees.

   General Trees and Forests
   -------------------------

   General rooted trees and forests can be specified through the class
   `Tree_Node<T>`, which defines nodes containing a generic key of type
   `T`.

   `Tree_Node<T>` is relativelly simple but very powerful and useful for
   modeling problems based in general trees and resident in the memory.


   Binary search trees
   -------------------

   `Aleph-w` (\f$\aleph_\omega\f$) support an extense variety of binary
   search tre types designed for managing operation on sets in \f$O(\lg
   n)\f$ complexity.

   The main types are `Avl_Tree`, `Splay_Tree`, `Rand_Tree`, `Treap`,
   `Treap_Rk`, `Rb_Tree` and others.

   These classes operate in function of "nodes" not of the data
   contained in the nodes, which allows a more powerful and functional
   interface, although certainly more complicated for using because the
   memory management is delegated to the user.

   Containers classes
   ------------------

   Managing of sets and maps based on the binary search trees metioned
   above could be implemented with memory management through of two
   general classes
   
   1. `DynSetTree`: implement sets
   2. `DynMapTree`: implement maps

   For both classes you could specify the binary search tree type as
   template parameter and decide its performance characteristics. 

   Heaps
   -----

   Heaps are indispensable for the good performance of many
   algorithms. `Aleph-w` (\f$\aleph_\omega\f$) supports several types of
   heaps, whose usage is according the algorithms needs.

   1. `BinHeap`: a binary heap oriented to nodes containing data. The
      heap type is ideal for application managing dynamic data that can
      belong to several heaps. The great advantage of this type is its
      scalability. 

   2. `DynBinHeap`: a version of above heap where the memory management
      is already done.

   3. `ArrayHeap`: a heap based on a contiguous array representation. This
      is the fastest heap, but once decided the array size, the scale is
      limited. 

   4. `DynArrayHeap`: similar to above but uses `DynArray`, what gives a
      good trade of between performance and memory consumption.


   @author Leandro Rabindranath León (lrleon en ula punto ve)
   @}
*/
