
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

# include <aleph.H>
# include <slink.H>

# define NumItems 10

struct Record1 : public Slink
{
  unsigned n;

  Record1* get_next() { return static_cast<Record1*>(Slink::get_next()); }
};

struct Record2 
{
  unsigned n;

  Slink link;

  SLINK_TO_TYPE(Record2, link);
};
# include <iostream>


void* foo();

int main()
{
  {
    Record1 head;
    unsigned i;
    Record1 *node; 
    Record1 *aux;

    for (i = 0; i < NumItems; i++)
      {
	node = new Record1;

	node->n = i;
	head.insert_next(node);
      }

    for (node = head.get_next(); node not_eq &head; node = node->get_next())
      {
	printf("%u ", node->n);
      }

    while (not head.is_empty())
      {
	aux = static_cast<Record1*>(head.remove_next());
	delete aux;
      }
  }

  {
    Slink head;
    unsigned i;
    Record2 *node; 
    Slink *link;

    for (i = 0; i < NumItems; i++)
      {
	node = new Record2;

	node->n = i;
	head.insert_next(&node->link);
      }

    for (link = head.get_next(); link not_eq &head; link = link->get_next())
      {
	node = Record2::slink_to_type(link);
	printf("%u ", node->n);
      }

    while (not head.is_empty())
      {
	node = Record2::slink_to_type(head.remove_next());
	delete node;
      }
  }

  printf("Ended\n");
}
