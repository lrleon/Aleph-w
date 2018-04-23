# Aleph-w

Aleph-w is a library of data structures and algorithms implemented in
and for C++.

It contains the most popular data structures: single and double lists,
special arrays (dynamic and of bits), many types of trees: multitrees,
binary search trees, avl, red-black, randomized, treaps, splay and
other interesting types.

Several types of hash tables are implemented: separated chaining, open
addressing with linear probing and double function hash; this last one
has a garbage collector mechanism that allows freeing deleted
entries. Dynamic linear hash tables are implemented too.

The main algorithms on graphs are implemented and with good
performance features. Network Flow (maximum and min cost), cut points,
topological sort, spanning trees, min paths, etc. The graphs are
generic in the sense that they can contain data independent of the
algorithm.

## Summary of Data Structures and Algorithms

Aleph-w export the most popular data structures but with three
distinctive principles:

  1. The implementation is not hidden. Each data structure claims how
     this is implemented. For example, the Aleph-w type `DynList`
     represents a dynamic list of items that is implemented with a
     single linked list. Consequently you can not delete in O(1) an
     item given its address. At the contrary, the data type
     `DynDlist`, that represents a list implemented with a double
     linked list, allows to delete any item in O(1) given its address
     because the item contains enough context for the deletion.
  
  2. End to end principle: most of components of Aleph-w are designed
     with a minimalist pretension, according to the end to end
     argument (Saltzer et Al). Consider for example, the `Dlink` type,
     which represents a double link to be used in a double list. At
     this level, all the functionality related to the links is
     handled, without regard of the rest of list context. In the next
     level, the class `Dnode<T>` handles a node belonging to a double
     list of nodes (not items) without memory management. Finally, the
     class `DynDlist<T>` handles a list of items, with full memory
     management.
	 
	

Thus,
     you could have something like this:
	 
    struct MyNode
	{
	  // here I put everyone that I want to put in an item
	  Dlink link_to_list1;
	  Dlink link_to_list2;
    };
	
	Here we have defined a node which belongs to two different lists
    called `list1` and `list2` through the double links
    `link_to_list1` and `link_to_list2` respectively. The `Dlink` type
    implements all that it is needed 
  
  3. Functional programming

### Sequences

### Trees

#### Binary search trees

#### General rooted trees

#### Graphs

#### Network Flows

#### Functional Programming 

## Integration



### Requirements

### Building


### Use in projects

## LICENSE

## License

See
[LICENSE](https://github.com/lrleon/Aleph-w/blob/master/LICENSE). Copyright
(c) 2002-2018 Leandro Rabindranath Leon and Alejandro Mujica
