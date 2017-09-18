# include <tpl_avl.H>


    template <class Node>
struct Preorder_Nodes
{
  ofstream & output;

  Preorder_Nodes(ofstream & out) 
     : output(out)
  {
    output << "start-prefix ";
  }

  ~Preorder_Nodes()
  {
    output << endl
	   << endl;
  }

  void operator () (Node * p)
  {
    output << (long) p << " ";
  }
};

    template <class Node>
struct Inorder_Nodes
{
  ofstream & output;

  Inorder_Nodes(ofstream & out) : output(out) 
  {
    output << "start-infix ";
  }

  ~Inorder_Nodes()
  {
    output << endl
	   << endl;
  }

  void operator () (Node * p)
  {
    output << (long) p << " ";
  }
};

    template <class Node, class Key>
struct Inorder_Keys
{
  ofstream & output;

  Inorder_Keys(ofstream & out) : output(out) 
  {
    output << "start-key ";
  }

  ~Inorder_Keys()
  {
    output << endl
	   << endl;
  }

  void operator () (Node * p)
  {
    output << Key () (KEY(p)) << " ";
  }
};

    template <class Node>
struct Inorder_Diff
{
  ofstream & output;

  Inorder_Diff(ofstream & out) : output(out) 
  {
    output << "start-aux ";
  }

  ~Inorder_Diff()
  {
    output << endl
	   << endl;
  }

  void operator () (Node * p)
  {
    output << (int) DIFF(p) << " ";
  }
};



    template <class Node, class Key_To_Text>
void avl_to_btreepic(Node * root, ofstream & output)
{
  {
    Preorder_Nodes<Node> preorder (output);
    For_Each_Preorder<Node> () (root, preorder);
  }

  {
    Inorder_Nodes<Node> inorder (output);

    For_Each_In_Order<Node> () (root, inorder);
  }

  {
    Inorder_Diff<Node> diffs (output);
    For_Each_In_Order<Node> () (root, diffs);
  }

  {
    Inorder_Keys<Node, Key_To_Text> inorder (output);
    For_Each_In_Order<Node> () (root, inorder);
  }
}

