
/*
  This file is part of Aleph-w system

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014
  Leandro Rabindranath León
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met: 

  1. Redistributions of source code must retain the above copyright 
     notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright 
     notice, this list of conditions and the following disclaimer in the 
     documentation and/or other materials provided with the distribution.

  3. All advertising materials mentioning features or use of this software
     must display the following acknowledgement:

     Copyright (c) 2002-2014 Leandro Rabindranath León. See details of 
     licence.     

     This product includes software developed by the Hewlett-Packard
     Company, Free Software Foundation and Silicon Graphics Computer
     Systems, Inc. 

  4. Neither the name of the ULA nor the names of its contributors may
     be used to endorse or promote products derived from this software
     without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY Leandro Rabindranath León ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Aleph-w is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.

  I request users of this software to return to 

  Leandro Rabindranath Leon
  CEMISID 
  Ed La Hechicera 
  3er piso, ala sur
  Facultad de Ingenieria 
  Universidad de Los Andes 
  Merida - REPÚBLICA BOLIVARIANA DE VENEZUELA    or

  leandro.r.leon@gmail.com

  any improvements or extensions that they make and grant me the rights
  to redistribute these changes.  
*/

/**
  @file huffman_btreepic.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# include <string.h>

enum Offset { NO_OFFSET, LEFT, RIGHT };

struct Infix_Desc
{
  int pos;
  int level;
  
  Offset offset;

  Infix_Desc() : offset(NO_OFFSET) { /* Empty */ }

  Infix_Desc(int i, int l) : pos(i), level(l), offset(NO_OFFSET) 
  {
    /* Empty */ 
  }
};


DynMapTree<Freq_Node *, Infix_Desc, Treap> infix_table;

struct Level_Desc
{
  bool        is_left;
  Freq_Node * level_succ;

  Level_Desc() {}

  Level_Desc(bool il, Freq_Node * succ = NULL)
    : is_left(il), level_succ(succ)
  {
    /* Empty */
  }
};

typedef DynMapTree<Freq_Node *, Level_Desc, Treap>  Level_Table;

Level_Table level_table;

ofstream *output_ptr = NULL;

void save_infix_pos(Freq_Node * p, int level, int pos)
{
  infix_table.insert(p, Infix_Desc(pos, level));
}

/** 
  @brief Last to post guards first level and sense 
*/
void save_level_pos_1(Freq_Node * p, int, bool is_left)
{
  level_table.insert(p, Level_Desc(is_left));
}

Freq_Node * pred = NULL; /* P Predesor levels in tour */

/** 
  @brief Second run to save the successor to the level 
*/
void save_level_pos_2(Freq_Node * p, int pos, bool)
{
  if (pos == 0)
    {
      assert(pred == NULL);

      pred = p;

      return;
    }

  assert(pred != NULL and pred != p);

  Level_Desc & level_desc_pred = level_table.find(pred);

  level_desc_pred.level_succ = p;

  pred = p;
}


void write_prefix(Freq_Node * p, int, int)
{
  const Infix_Desc & infix_desc = infix_table[p];
  *output_ptr << (int) infix_desc.pos << " ";
}

void write_freq(Freq_Node * p, int, int) 
{
  const size_t & freq = p->get_key().second;

  *output_ptr << "\"" << freq << "\" ";
}


void write_leaves(Freq_Node * p, int, int pos) 
{
  if (not (LLINK(p) == NULL and RLINK(p) == NULL))
    return;

  const string & key =  p->get_key().first;

  *output_ptr << "TAG " << pos << " \"";

  if (key == "\n")
    *output_ptr << "$\\backslash$n"; /* ALL: write */
  else if (key == "")
    *output_ptr << "$\\neg$";
  else if (key == "$")
    *output_ptr << "\\$" ; 
  else if (key == "&")
    *output_ptr << "\\&" ;
  else if (key == "#")
    *output_ptr << "\\#" ;
  else if (key == "%")
    *output_ptr << "\\%" ;
  else if (key == "{")
    *output_ptr << "$\\{$" ;
  else if (key == "}")
    *output_ptr << "$\\}$" ;
  else if (key == "^")
    *output_ptr << "\\^" ;
  else if (key == "_")
    *output_ptr << "\\_" ;    
  else if (key == "\\")
    *output_ptr << "$\\backslash$" ; 
  else if (key == " ")
    *output_ptr << "$\\square$";
  else if (key == "\"")
    *output_ptr << "$\\prime\\prime$";
  else 
    *output_ptr << key;
  
  *output_ptr << "\" S 0 -20 " << endl; 
}


size_t num_digits(const int & n)
{
  return strlen(gnu::autosprintf("%d" ,n));
}


int minimal_gap = 4;


const long double offset[] = { 10, 15, 25, 40, 55, 65, 85, 90 };


void adjust_nodes(Freq_Node * p, int p_level, int p_infix_pos)
{
    /** It is the last node?
    YES? ==> return because no successor */
  if (p_infix_pos == level_table.size() - 1)
    return;

  Level_Desc & p_level_desc = level_table[p];

  Freq_Node *& p_succ = p_level_desc.level_succ;

    /** It is the last node on tour levels?
    YES? ==> Return  */
  if (p_succ == NULL) 
    return;

  Infix_Desc & p_succ_infix_desc = infix_table[p_succ];

    /** They are on the same level?
    NO? ==> there is nothing to do */
  if (p_level != p_succ_infix_desc.level) 
    return; 

  const int & p_succ_infix_pos = p_succ_infix_desc.pos;

/** 
  @brief Separation between the two nodes nodes 
*/
  const int diff_pos = p_succ_infix_pos - p_infix_pos;

  /** They are sufficiently separated?
  YES? ==> no adjustment is made; */
  if (diff_pos > minimal_gap) 
    return;

  Level_Desc & p_suc_level_desc = level_table[p_succ];
  
    /**They have the same sense?
    YES? ==> they have the same orientation and do adjustment */
  if (p_suc_level_desc.is_left == p_level_desc.is_left) 
    return;
  
  Infix_Desc & p_infix_desc = infix_table[p];

  const string & kp = p->get_key().first;
  const size_t kp_sz = kp.size();
  if (p_infix_desc.offset == NO_OFFSET) /* It was moved? */
    {
      *output_ptr << "xoffset " << p_infix_pos << " " << -offset[kp_sz] << endl;

      p_infix_desc.offset = LEFT;
    }

  assert(p_succ_infix_desc.offset == NO_OFFSET);

  const string & k_succ = p_succ->get_key().first;
  const size_t k_succ_sz = k_succ.size();
  /*  if (k_succ_sz > 1) */
    *output_ptr << "xoffset " << p_succ_infix_pos << " " 
	   << (int)(offset[k_succ_sz]/2) << endl;

  p_infix_desc.offset = RIGHT;
}

/** 
  @brief btreepic generates a specification for a Huffman tree.

  @ingroup Trees
*/
void huffman_to_btreepic(Freq_Node * p, 
			 const bool with_level_adjust = false)
{
  inOrderRec(p, save_infix_pos);

  *output_ptr << "start-prefix ";

  preOrderRec(p, write_prefix);

  *output_ptr << endl
	 << "start-key ";

  inOrderRec(p, write_freq);  

  *output_ptr << endl;

  inOrderRec(p, write_leaves);  

  *output_ptr << endl;

  if (not with_level_adjust)
    return;

  const size_t tree_size = infix_table.size();

  levelOrder(p, save_level_pos_1, tree_size);

  levelOrder(p, save_level_pos_2, tree_size);

/** 
  @brief Browse by infix and move labels
  same level that they are very attached 
*/
  inOrderRec(p, adjust_nodes);
}
