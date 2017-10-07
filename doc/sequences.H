/**
     @defgroup Secuencias Sequences
     @{ 
   
     In `Aleph-w` (\f$\aleph_\omega\f$) all data structure based on a
     linked list or an array is considered a sequence.

     Sequences are probably the most used data structures. In `Aleph-w`
     (\f$\aleph_\omega\f$) the sequences very ofted are used in
     association with functional methods. These functional features are
     implemented through of iterators exported by each class
     representing a sequence. You could so traverse a sequence via an
     iterator or via a functional method

     These are the main `Aleph-w` (\f$\aleph_\omega\f$) sequences:

     1. Arrays: dynamic arrays `DynArray` and bit arrays ` y arreglos de bits
        BitArray.

	Some special containers oriented to flow are also implemented
	with arrays. These are `DynArrayQueue` and `DynArrayStack`.

     2. Linked lists: the `Aleph-w` (\f$\aleph_\omega\f$) doctrine is to
        export three levels of implementation:

	1. Link management: in this level all the problems that requires
	   to handle the links but not to know the data are solved. For
	   example, for concatenating a list to another list is not
	   necessary to know the data contained into the nodes. The only
	   knowledge is the link of extremes of lists. The classes
	   Slinknc and Dlink implement operation on single and double
	   linked list respectively.

	   This level is very useful for maintaining data related to
	   several lists. For example, suppose a graphical window
	   manager that handles two list: a list of showed windows on
	   the screen and another of hided windows. When a window must
	   be showed, the this windows must be deleted form the list of
	   hided and inserted into the list of showed. This implies that
	   a window must be deleted form a list and then to be inserted
	   into another. With this approach this can be done very
	   efficiently without destroying the window when it is removed
	   from a list and then reconstrunting in order to insert it in
	   the another list.

	   Another example could be a record that belongs simultaneously
	   to several lists.

	2. Data management: when it is necessary to know the data
	   contained on a node, then the operation is put in this
	   level. The most simple and common situation is when given a
	   node belonging to a linked list we want to obtain its
	   data. In fact, the main work of this level is to cast pointer
	   to links of previous level to pointers to nodes containing
	   data.

	   In this level we have to main classed `Dnode` and `Snodenc`.

	   In the previous level the memory management is not concerned.

	3. Memory management: the last level does not operate in
	   function of link or nodes. It is the more common and works in
	   function of data types, not of nodes or links. This level
	   manages transparentely the memory.

	   The main classes of this level are `DynList`, `DynDlist`,
	   `DynListQueue` and `DynListStack`.

     @author Leandro Rabindranath León (lrleon en ula punto ve)
     @} 
  */ 
