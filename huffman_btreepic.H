
# include <cstdlib>

enum Offset { NO_OFFSET, LEFT, RIGHT };

struct Infix_Desc
{
  int pos = 0;
  int level = 0;
  
  Offset offset;

  Infix_Desc() : offset(NO_OFFSET) { /* empty */ }

  Infix_Desc(int i, int l) : pos(i), level(l), offset(NO_OFFSET) 
  {
    /* empty */ 
  }
};


DynMapTree<Freq_Node *, Infix_Desc, Treap> infix_table;

struct Level_Desc
{
  bool        is_left;
  Freq_Node * level_succ;

  Level_Desc() {}

  Level_Desc(bool il, Freq_Node * succ = nullptr)
    : is_left(il), level_succ(succ)
  {
    // empty
  }
};

typedef DynMapTree<Freq_Node *, Level_Desc, Treap>  Level_Table;

Level_Table level_table;

ofstream *output_ptr = nullptr;

void save_infix_pos(Freq_Node * p, int level, int pos)
{
  infix_table.insert(p, Infix_Desc(pos, level));
}

    // primera pasada para guardas pos de nivel y sentido
void save_level_pos_1(Freq_Node * p, int, bool is_left)
{
  level_table.insert(p, Level_Desc(is_left));
}

Freq_Node * pred = nullptr; // Predesor de p en recorrido por niveles

// segunda pasada para guardar el sucesor en el nivel
void save_level_pos_2(Freq_Node * p, int pos, bool)
{
  if (pos == 0)
    {
      assert(pred == nullptr);

      pred = p;

      return;
    }

  assert(pred != nullptr and pred != p);

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
  if (not (LLINK(p) == nullptr and RLINK(p) == nullptr))
    return;

  const string & key =  p->get_key().first;

  *output_ptr << "TAG " << pos << " \"";

  if (key == "\n")
    *output_ptr << "$\\backslash$n"; // TODO: escribir 
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
  return to_string(n).size();
}


int minimal_gap = 4;


const long double offset[] = { 10, 15, 25, 40, 55, 65, 85, 90 };


void adjust_nodes(Freq_Node * p, int p_level, int p_infix_pos)
{
  if (p_infix_pos == level_table.size() - 1) // es el último nodo
    return; // retorne porque no tiene sucesor

  Level_Desc & p_level_desc = level_table[p];

  Freq_Node *& p_succ = p_level_desc.level_succ;

  if (p_succ == nullptr) // último nodo en recorrido por niveles?
    return; // sí

  Infix_Desc & p_succ_infix_desc = infix_table[p_succ];

  if (p_level != p_succ_infix_desc.level) // están en el mismo nivel?
    return; // No, nada que hacer

  const int & p_succ_infix_pos = p_succ_infix_desc.pos;

  // separación en nodos entre los dos nodos
  const int diff_pos = p_succ_infix_pos - p_infix_pos;

  if (diff_pos > minimal_gap) // están suficientemente separados?
    return; // si, no se hace ningún ajuste;

  Level_Desc & p_suc_level_desc = level_table[p_succ];
  
        // tienen el mismo sentido?
  if (p_suc_level_desc.is_left == p_level_desc.is_left) 
    return; // tienen la misma orientación ==> no hacer ajuste
  
  Infix_Desc & p_infix_desc = infix_table[p];

  const string & kp = p->get_key().first;
  const size_t kp_sz = kp.size();
  if (p_infix_desc.offset == NO_OFFSET) // ya fue desplazado?
    {
      *output_ptr << "xoffset " << p_infix_pos << " " << -offset[kp_sz] << endl;

      p_infix_desc.offset = LEFT;
    }

  assert(p_succ_infix_desc.offset == NO_OFFSET);

  const string & k_succ = p_succ->get_key().first;
  const size_t k_succ_sz = k_succ.size();
  //  if (k_succ_sz > 1)
    *output_ptr << "xoffset " << p_succ_infix_pos << " " 
	   << (int)(offset[k_succ_sz]/2) << endl;

  p_infix_desc.offset = RIGHT;
}

    /** Genera una especificación para btreepic de un árbol de Huffman.

       @ingroup Arboles
     */
void huffman_to_btreepic(Freq_Node * p, const bool with_level_adjust = false)
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

  levelOrder(p, save_level_pos_1);

  levelOrder(p, save_level_pos_2);

  // Recorrer por infijo y desplazar etiquetas del mismo nivel que
  // queden muy pegadas  
  inOrderRec(p, adjust_nodes);
}


