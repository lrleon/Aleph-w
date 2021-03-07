
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

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
#ifndef PREFIX_TREE_H
#define PREFIX_TREE_H

# include <ah-zip.H>
# include <tpl_dynArray.H>
# include <tpl_tree_node.H>
# include <generate_tree.H>

using namespace std;

class Cnode : public Tree_Node<char>
{
public:

  Cnode(const char c) { this->get_key() = c; }

  char symbol() const noexcept { return get_key(); }

  DynList<Cnode*> children() const 
  {
    DynList<Cnode*> r;
    for_each_child([&r] (Tree_Node<char> * p) { r.append((Cnode*) p); });
    return r;
  }

  string to_str() const 
  {
    string ret(1, symbol());
    children().for_each([&ret] (Cnode * p) { ret += "(" + p->to_str() + ")"; });
    return ret;
  }

private:

  struct W
  {
    const string operator () (Cnode * p) const
    {
      char c = p->symbol();
      return c != '$' ? string(1, p->symbol()) : string("\\$");
    }
  };

public:

  void ntreepic(ostream & out)
  {
    generate_tree<Cnode, W>(this, out);
  }

  bool is_end_word() const noexcept
  {
    Tree_Node * child = get_left_child();
    if (child == nullptr)
      return false;
    return child->get_key() == '$';
  }

  void mark_end_word()
  {
    assert(not is_end_word());
    insert_leftmost_child(new Cnode ('$'));
  }

  Cnode * search_child(const char c) const noexcept
  {
    for (Tree_Node * child = get_left_child(); child != nullptr; 
         child = child->get_right_sibling())
      if (child->get_key() == c)
        return (Cnode*) child;

    return nullptr;
  }

  Cnode * greater_child(const char c) const noexcept
  {
    for (Tree_Node * child = get_left_child(); child != nullptr; 
         child = child->get_right_sibling())
      if (child->get_key() > c)
        return (Cnode*) child;
    return nullptr;
  }

  // retorna el nodo situado al final del prefijo prefix
  tuple<const Cnode*, const char*> search_prefix(const char * prefix) const
  {
    assert(prefix != nullptr);

    if (*prefix == '\0')
      return make_tuple(this, "");

    Cnode * p = search_child(*prefix);
    if (p == nullptr) // encontró *prefix?
      return make_tuple(this, prefix); // No! ==> llegamos hasta aquí

    return p->search_prefix(prefix + 1);
  }

  const Cnode * search_word(const char * word) const
  {
    if (*word == '\0')
      return is_end_word() ? this : nullptr;

    Cnode * ptr = search_child(*word);
    if (ptr == nullptr)
      return nullptr;

    return ptr->search_word(word + 1);
  }

  bool contains(const string & word) const noexcept
  {
    return search_word(word.c_str());
  }

  Cnode * insert_child(Cnode * child)
  {
    assert(not children().exists([child] (Cnode * p) 
				 { return p->symbol() == child->symbol(); }));

    Cnode * sibling = greater_child(child->symbol());
    if (sibling == nullptr)
      this->insert_rightmost_child(child);
    else
      sibling->insert_left_sibling(child);

    return child;
  }

  bool insert_word(const string & word) 
  {
    DynList<Cnode*> list;
    try
      {
	tuple<const Cnode*, const char*> p = search_prefix(word.c_str());
	const Cnode * pp = get<0>(p);
	char * rem = const_cast<char*>(get<1>(p));

	if (*rem == '\0')
	  {
	    if (pp->is_end_word())
	      return false;
	    else
	      {
		const_cast<Cnode*>(pp)->mark_end_word();
		return true;
	      }
	  }

	for (char *ptr = rem; *ptr; ptr++)
	  list.append(new Cnode(*ptr));

	Cnode * child = 
	  const_cast<Cnode*>(pp)->insert_child(list.remove_first());
	while (not list.is_empty())
	  child = child->insert_child(list.remove_first());

	child->mark_end_word();

	return true;
      }
    catch (invalid_argument & e)
      {
	throw invalid_argument("word contains an invalid character");
      }
  }

  void destroy()
  {
    children().for_each([] (Cnode * p) { destroy_tree(p); });
  }

private:

  static string extract_word_from_stack(FixedStack<char> & stack)
  {
    string ret_val;
    ret_val.reserve(stack.size());

    char * ptr = &stack.base();
    char * last = &stack.top();
    while (ptr <= last)
      ret_val.append(1, *ptr++);

    return ret_val;
  }

  void words(FixedStack<char> & stack, DynArray<string> & l)
  {
    if (symbol() == '$')
      l.append(extract_word_from_stack(stack));

    stack.push(symbol());

    for (Tree_Node<char> * child = get_left_child(); child != nullptr;
	 child = child->get_right_sibling())
      static_cast<Cnode*>(child)->words(stack, l);
 
    stack.pop();
  }

public:

  DynArray<string> words() const
  {
    static FixedStack<char> stack(2048);
    assert(stack.is_empty());
    DynArray<string> ret_val;

    children().for_each([&] (Cnode * p) { p->words(stack, ret_val); });

    return ret_val;
  }

  void print_words() const
  {
    words().for_each([] (const string & w) { cout << w << endl; });
  }

  static void clone(Tree_Node<char> * src, Tree_Node<char> * tgt)
  {
    src->for_each_child([tgt] (auto s)
			{
			  tgt->insert_rightmost_child(new Cnode(s->get_key()));
			});
    for (auto it = get_pair_it(*src, *tgt); it.has_curr(); it.next_ne()) 
      {
	auto p = it.get_curr_ne();
	clone(p.first, p.second);
      }
  }

  Cnode * clone() const
  {
    Cnode * ret = new Cnode(symbol());
    clone(const_cast<Cnode*>(this), ret);
    return ret;
  }
};



#endif
