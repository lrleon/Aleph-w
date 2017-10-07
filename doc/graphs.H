

/** @defgroup Grafos Graphs
    @{ 

   Introduction
   ============

   Many combinatorial problems can be modeled and solved through
   `Aleph-w` (\f$\aleph_\omega\f$) graphs.

   There are three types of graphs in `Aleph-w` (\f$\aleph_\omega\f$):

   1. List_Digraph<Node, Arc>: graph represented with doubly linked
      lists. 

   2. List_SGraph<Node, Arc>:  graph represented with single linked
      lists. 

   3. Array_Graph<Node, Arc>: graph represented with dynamic arrays.

   All the three classes export the same interface. What differentiates
   them is their performance qualities in speed and space comsuption. 

   Historically, the first `Aleph-w` (\f$\aleph_\omega\f$) graph class
   was List_Graph. This class is very versatile and dynamic in the
   sense that it allow very fast topological modifications on the
   graph. By "topological modification" we intend to say that nodes
   and/or arc are inserted and/or removed. The dynamic sense consists on
   the possibility that these operations are done very frequently in
   \f$O(1)\f$. List_Graph in since the choice of fact for applications
   requering to build partial and/or many graph are very often. However,
   this speed pays its price in space consumption.

   List_SGraph was designed in order to reduce the space consumption of
   List_Graph. This is managed by using single lists instead of double
   ones, what reduces aproximatively 4 pointers by node and
   arc. However, the `remove_node()` method is slower \f$O(E)\f$, since
   it requires to traverse all the arcs ir order to remove the arcs
   pointed to the removed node. So, this class could be very time
   expensive for application that dinamically remove nodes. Although
   this class is fully functional and (we hope) correct, we are
   considering remove it or simply not support it longer in next library
   releases.

   Finally, the class Array_Graph models graphs whose internal
   representation uses dinamic arrays in order to save the adjacency
   lists. According our experience, we find this class as a very good
   trade off between performance and space consumption. The
   `remove_node()` operation is still \f$O(E)\f$, but the remaining
   topological operations are \f$O(1)\f$ and definitively this class
   occupies much less space than the list versions. We recommend to use
   this class unless that you require to remove nodes many times. There
   are a few of applications where you perform calculation by removing
   nodes from a graph. For this type of case, consider to use another
   graph.

   
   Specifiying a graph
   ===================

   In order to specify a graph and to do computations on it, the first
   thing that you must do is to define the types of data to store in the
   nodes and the arcs. Often the nodes and arcs are associated with many
   data. By example, a graph of cities could map the cities with the
   nodes and the distances and other sort of costs with the arcs. For
   each city (node) one could associate coordinates, the name, its
   population, etc. Now, in order to save space, it is very important
   that you only associate to the node the required data needed for the
   graph computations. Remember, the more fat is the data to associate
   to a node or arc, the more space the graph will use. So, use ids to
   the data, not all the data. In those case where the data resides in
   memory associate pointers to it. Of course, this remarks apply
   specially to the large scale graphs. If your graph is small, then
   perhaps you must not worry too much.

   Once you already have defined data types that you will store in the
   nodes and arcs, then you must declare the node and arc that you will
   use.

   The graphs interface requires the following specificaction convention:

   1. Node definition: in this phase you define the type of data to
      store in the nodes and then you declare the node.
   2. Arc definition: in the same way than for nodes, you define the
      type of data to store in the arcs. Then you declare the arc
   3. Graph definition: once defined the nodes and arcs structure, you
      declare the graph by passing it the nodes and arcs types
      previously defined.
     
   It is strongly recommended to do these phased by using aliases,
   `typedef` or `using`, to the node, arc and graph types.
   

   Specifiying the node
   --------------------

   Before to declare a `Aleph-w` (\f$\aleph_\omega\f$) graph, you must
   imperatively you must have declared the node. The node types are:

   1. Graph_Node<T>: for List_Graph
   2. Graph_Snode<T>: for List_SGraph
   3. Graph_Anode<T>: for Array_Graph

   If you want to use `Point` as type of nodes, then you could declare:

       using My_Node = Graph_Node<Point>;

   Here you have expressed that the nodes of your graph contain data of
   type `Point` and that you will use `List_Graph` as graph type (the
   `Aleph-w` (\f$\aleph_\omega\f$) graph whose representation is with
   doubly linked lists. 

   Specifiying the arc
   -------------------

   For the arcs you must also declare its type. There are three types of
   arcs according to the graph type:

   1. Graph_Arc<T>: for List_Graph
   2. Graph_Sarc<T>: for List_SGraph
   3. Graph_Aarc<T>: for Array_Graph

   By example, following the previous example, if the weights are
   euclidian distances between the points, ten you could declare an arc
   as follows:

       using My_Arc = Grap_Arc<double>;

   Specifying the Graph
   --------------------

   Once you have defined the node and the arc of your graph, you are
   ready for declare the graph. For the previous example, that would be
   as follows:

       using Euclidian_Graph = Graph_List<My_Node. My_Arc>;

   Now, you can instantiate graphs by using the alias `Euclidian_Graph`,
   some as:

       Euclidian_Graph g;

   Although you have already defined your node and arc with aliases, the
   interface exports two internal aliases that could be considered as
   subclasses:

   - `Euclidian_Graph::Node`, and
   - `Euclidian_Graph::Arc`

   You will need to know the types of nodes and arc in order to perform
   many operations on the graph. In our experience, it is more readable
   to use the these aliases instead of those that you declared when you
   was specifiying the graph. The following code snippet traverses all
   the arcs of a `Euclidian_Graph` graph:

       Euclidian_Graph g
       some operations; by example insert nodes and arcs ...
       for (auto it = g.get_arc_it(); it.has_curr(); it.next())
         {
	   Euclidian_Graph::Arc * arc = it.get_curr(); 
	   do something with the arc
	 }
   
   
@} */


