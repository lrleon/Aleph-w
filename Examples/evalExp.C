
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
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
# include <ctype.h>
# include <string.h>
# include <tpl_arrayStack.H>

enum Token_Type { Value, Operator, Lpar, Rpar, End, Error };

Token_Type lexer(char *& str, size_t & len)
{
  str += len;
  len = 1;
  while (isblank(*str))
    str++;
  switch (*str)
  {
   case '(':  return Lpar;
   case ')':  return Rpar;
   case '+':
   case '-':
   case '*':
   case '/':  return Operator;
   case '\0': return End;
  }
  if (not isdigit(*str)) 
    return Error;
  char* base = str + 1;
  while (isdigit(*base++))
    len++;
  return Value;
}

char * str_to_token(char * token_str, const size_t & len)
{
  static char buffer[256];
  strncpy(buffer, token_str, len); 
  buffer[len] = '\0';
  return buffer;
}

unsigned precedence(const char & op) // $ < ( < +- < */
{
  switch (op)
    {
    case '$': return 0;
    case '(': return 1;
    case '+':
    case '-': return 2;
    case '/':
    case '*': return 3;

    default: AH_ERROR("Invalid operator %c\n", op);
      return 0;
    }
}

void apply(ArrayStack<int>&  val_stack, ArrayStack<char>& op_stack)
{

  assert(op_stack.size() > 0);
  assert(val_stack.size() >= 2);

  const char the_operator = op_stack.pop(); 
  const int right_operand = val_stack.pop();
  const int left_operand  = val_stack.pop();
  int result;
  switch (the_operator)
    {
    case '+': result = left_operand + right_operand; break;
    case '-': result = left_operand - right_operand; break;
    case '*': result = left_operand * right_operand; break;
    case '/': result = left_operand / right_operand; break;

    default: AH_ERROR("Operator %c invalid", the_operator);

    }
  val_stack.push(result);
}

int eval(char* input)
{
  ArrayStack<int>  val_stack;
  ArrayStack<char> op_stack;
  Token_Type current_token;
  size_t token_len = 0;
  op_stack.push('$');
  while (true)
    {
      current_token = lexer(input, token_len);
      switch (current_token)
        {
        case Value:
          {
            const int operand = atoi(str_to_token(input, token_len));
            val_stack.push(operand);
            break;
          }
        case Lpar: 
          {
            op_stack.push(*input); // introducir parentesis en op_stack
            break;
          }
        case Operator: 
          {
            while (precedence(op_stack.top()) >= precedence(*input))
              apply(val_stack, op_stack);
            op_stack.push(*input); // introducir operador en op_stack
            break;
          }
        case Rpar: 
          {
            while (op_stack.top() != '(')
              apply(val_stack, op_stack);
            op_stack.pop(); /* saca el parentesis izquierdo */
            break;
          }
        case End:
          {
            while (op_stack.top() != '$')
              apply(val_stack, op_stack);
            op_stack.pop(); // debe ser '$'
            const int ret_val = val_stack.pop();

            if ((val_stack.size() != 0) or (op_stack.size() != 0))
              AH_ERROR("Bad expression");

            return ret_val;
          }

        case  Error:
        default: AH_ERROR("Bad token detected");

        } 
    }
}

int main(int, char** argc)
{
  printf("\t%d\n\n", eval(argc[1]));
}
