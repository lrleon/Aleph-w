/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

/** @file fmmd.H
 *  @brief File memory-mapped data utilities.
 *  @ingroup Utilities
 *  @author Leandro Rabindranath León
 */

# include<tpl_tree.H>
# include<tpl_binNode.H>
# include<tpl_dlist.H>
# include<fstream>


    template <class Key> 
BinNode<Key> * forest_to_bin(Tree_Node<Key> * root)
{
  if (root == nullptr)
    return nullptr;
  
  BinNode<Key> * result = new BinNode<Key> (root->get_data());
  
  result->getL() = forest_to_bin(root->get_left_child());
  result->getR() = forest_to_bin(root->get_right_sibling());
  
  return result;
}


    template <class Key> 
void insert_child(BinNode<Key> * bin_node, Tree_Node<Key> *& tree)
{
  if (bin_node == nullptr)
    return;
  
  Tree_Node<Key> * child = new Tree_Node<Key>(KEY(bin_node));
  
  tree->insert_leftmost_child(child);
}


    template <class Key> 
void insert_sibling(BinNode<Key> * bin_node, Tree_Node<Key> *& tree)
{
  if (bin_node == nullptr)
    return;
  
  Tree_Node<Key> * sibling = new Tree_Node<Key>(KEY(bin_node));
  
  tree->insert_right_sibling(sibling);
}


    template <class Key> 
void bin_to_tree(BinNode<Key> * bn, Tree_Node<Key> *& tree)
{
  if (bn == nullptr)
    return;
  
  insert_child(LLINK(bn), tree);
  Tree_Node<Key> * left_child =  tree->get_left_child();
  bin_to_tree(LLINK(bn), left_child);
  
  insert_sibling(RLINK(bn), tree);
  Tree_Node<Key> * right_sibling = tree->get_right_sibling();
  bin_to_tree(RLINK(bn), right_sibling);
}


    template <class Key> Tree_Node<Key> * 
bin_to_forest(BinNode<Key> * bn)
{
  if (bn == nullptr)
    return nullptr;
  
  Tree_Node<Key> * tree = new Tree_Node<Key> (KEY(bn));
  
  bin_to_tree(bn, tree);

  return tree;
}

struct Two_Nibbles
{
  unsigned int n1 : 4;
  unsigned int n2 : 4;
};

char nibble_to_char(unsigned char c)
{
  return c < 10 ? c+ '0' : 'A' + c -10;
}

char * stringficate(void *src_data, size_t src_size,char * result)
{
  Two_Nibbles two_nibbles;
  
  char * ret_val = result;
  char * src_str= static_cast<char *> (src_data);
  
  for(int i=0;i<src_size; i++)
    {
      two_nibbles= * reinterpret_cast<Two_Nibbles*>(src_str);
      src_str++;
            *result++ = nibble_to_char(two_nibbles.n2);
            *result++ = nibble_to_char(two_nibbles.n1);
    }
  *result = '\0';
  return ret_val;
}

int char_to_nibble(unsigned char c)
{
  return c < 58 ? c - '0' : c -'A' + 10;
}


void destringficate(void *result, size_t src_size, char *src_data){
  
  Two_Nibbles two_nibbles;
  char * result_str = static_cast<char*> (result);
  
  for(int i=0;i<src_size; i++)
    {
      two_nibbles.n2 = char_to_nibble(*src_data);
      src_data++;
      two_nibbles.n1 = char_to_nibble(*src_data);
      src_data++;
      char *character = reinterpret_cast<char*>(&two_nibbles);
      *result_str++ = *character;
    }
}

    template <class T>
struct pre_in{
  T data;
  int inorder_pos;
};
	
     template <class Node,class T> inline static
void get_pre_in_list(Node* node,Dlist<T> & l,int &pos)
{
  
  if (node == Node::NullPtr) 
    return;
  
  T pi;
  pi.data=KEY(node);
  Dnode<T> * lnodo = new Dnode<T>(pi);
  l.append(lnodo);
  
  get_pre_in_list(LLINK(node),l,pos);
  
  lnodo->get_data().inorder_pos=pos;
  pos++;
  
  get_pre_in_list(RLINK(node),l,pos);
}
 
	template <class Key> void
write_to_stream(BinNode<Key> * root, ofstream & output_stream)
{
  if( root == nullptr)
    return;
  Dlist<pre_in<Key> > l;
  int cardinality=0;
  char buffer[1024];
  size_t size= sizeof(KEY(root));
  
  get_pre_in_list(root,l,cardinality);
  output_stream<<cardinality<<endl;
  output_stream<<size<<endl;
  
  Dlist<pre_in<Key> >::Iterator a(l);
  for(a.reset_first(); a.has_curr(); a.next_ne())
    output_stream<<
      stringficate(reinterpret_cast<void *>(&(a.get_curr()->get_data().data)),size,buffer)
		 <<endl;
  
  for(a.reset_first();a.has_curr();a.next_ne())
    output_stream<<a.get_curr()->get_data().inorder_pos+1<<endl;
  
  
}

    template <class Key> 
BinNode<Key>* build_tree(DynArray<Key>& preorder, int inf_p, int sup_p,
                       DynArray<Key>& inorder,  int inf_i, int sup_i)
{
  
  if (inf_p > sup_p)
    {
    assert(inf_i > sup_i);
    return BinNode<Key>::NullPtr;
    }
  
  assert(sup_p - inf_p == sup_i - inf_i);
  
  BinNode<Key> * root = new BinNode<Key> (preorder[inf_p]);
  
  int i;
  for (int j = inf_i; j <= sup_i; j++)
    if (inorder[j] == preorder[inf_p])
      {
	i = j - inf_i;
	break;
      }
  
  assert(i <= sup_i);
  
  LLINK(root) = build_tree(preorder, inf_p + 1, inf_p + i, 
			   inorder, inf_i, inf_i + (i - 1));
  
  RLINK(root) = build_tree(preorder, inf_p + i + 1, sup_p,
			   inorder, inf_i + i + 1, sup_i);
  
  return root;
}

    template <class Key> 
BinNode<Key> * read_from_stream(ifstream& input_stream)
{
  int cardinality;
  size_t size;
  int pos;
  
  input_stream>>cardinality;
  input_stream>>size;
  
  char buffer[size*2+1];
  char str_data[size*2+1];
  
  DynArray<Key> preorder(cardinality);
  DynArray<Key> inorder(cardinality);
  
  for(int i = 0; i< cardinality ; i++){
    input_stream>>buffer;
    destringficate(reinterpret_cast<void *>(str_data),size,buffer);
    preorder[i] = *reinterpret_cast<Key *>(str_data);
  }
  
  for(int i = 0; i < cardinality; i++){
    input_stream>>pos;
    inorder[pos-1]=preorder[i];
  }
  
  return build_tree<Key>(preorder,0,cardinality-1,inorder,0,cardinality-1);
}

