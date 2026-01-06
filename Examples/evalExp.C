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

/**
 * @file evalExp.C
 * @brief Arithmetic expression evaluator using operator precedence
 * 
 * This example demonstrates a classic expression evaluator using two stacks:
 * one for operands and one for operators. It implements the shunting-yard
 * algorithm for handling operator precedence.
 * 
 * Supported operators: +, -, *, /
 * Supports parentheses for grouping.
 * 
 * Usage: evalExp "expression"
 * Example: evalExp "3 + 4 * 2"
 *          evalExp "(3 + 4) * 2"
 */

# include <cctype>
# include <cstring>
# include <cstdlib>
# include <iostream>
# include <tclap/CmdLine.h>
# include <tpl_arrayStack.H>

using namespace std;
using namespace Aleph;

enum Token_Type { Value, Operator, Lpar, Rpar, End, Error };

/**
 * @brief Lexer - extracts next token from input string
 * 
 * @param str Pointer to current position in string (updated on return)
 * @param len Length of extracted token (updated on return)
 * @return Token_Type The type of token found
 */
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

/**
 * @brief Convert token string to null-terminated string
 */
char * str_to_token(const char * token_str, const size_t & len)
{
  static char buffer[256];
  strncpy(buffer, token_str, len); 
  buffer[len] = '\0';
  return buffer;
}

/**
 * @brief Get operator precedence
 * 
 * Precedence levels: $ < ( < +- < /
 */
unsigned precedence(const char & op)
{
  switch (op)
    {
    case '$': return 0;  // Stack bottom marker
    case '(': return 1;
    case '+':
    case '-': return 2;
    case '/':
    case '*': return 3;
    default: 
      cerr << "Invalid operator: " << op << endl;
      exit(1);
    }
}

/**
 * @brief Apply operator to top two values on stack
 */
void apply(ArrayStack<int>& val_stack, ArrayStack<char>& op_stack)
{
  if (op_stack.size() == 0)
    {
      cerr << "Error: operator stack empty" << endl;
      exit(1);
    }
  if (val_stack.size() < 2)
    {
      cerr << "Error: not enough operands" << endl;
      exit(1);
    }

  const char the_operator = op_stack.pop(); 
  const int right_operand = val_stack.pop();
  const int left_operand  = val_stack.pop();
  int result;
  
  switch (the_operator)
    {
    case '+': result = left_operand + right_operand; break;
    case '-': result = left_operand - right_operand; break;
    case '*': result = left_operand * right_operand; break;
    case '/': 
      if (right_operand == 0)
        {
          cerr << "Error: division by zero" << endl;
          exit(1);
        }
      result = left_operand / right_operand; 
      break;
    default: 
      cerr << "Invalid operator: " << the_operator << endl;
      exit(1);
    }
    
  val_stack.push(result);
}

/**
 * @brief Evaluate arithmetic expression
 * 
 * @param input The expression string to evaluate
 * @return int The result of the evaluation
 */
int eval(char* input)
{
  ArrayStack<int>  val_stack;
  ArrayStack<char> op_stack;
  size_t token_len = 0;
  
  op_stack.push('$');  // Stack bottom marker
  
  while (true)
    {
      switch (lexer(input, token_len))
        {
        case Value:
          {
            const int operand = atoi(str_to_token(input, token_len));
            val_stack.push(operand);
            break;
          }
          
        case Lpar: 
          op_stack.push(*input);
          break;
          
        case Operator: 
          while (precedence(op_stack.top()) >= precedence(*input))
            apply(val_stack, op_stack);
          op_stack.push(*input);
          break;
          
        case Rpar: 
          while (op_stack.top() != '(')
            {
              if (op_stack.top() == '$')
                {
                  cerr << "Error: mismatched parentheses" << endl;
                  exit(1);
                }
              apply(val_stack, op_stack);
            }
          op_stack.pop();  // Remove the '('
          break;
          
        case End:
          while (op_stack.top() != '$')
            apply(val_stack, op_stack);
          op_stack.pop();  // Remove '$'
          
          if (val_stack.size() != 1 or op_stack.size() != 0)
            {
              cerr << "Error: malformed expression" << endl;
              exit(1);
            }
          return val_stack.pop();

        case Error:
        default: 
          cerr << "Error: invalid token at position" << endl;
          exit(1);
        } 
    }
}

int main(int argc, char** argv)
{
  try
    {
      TCLAP::CmdLine cmd("Arithmetic expression evaluator", ' ', "1.0");

      TCLAP::UnlabeledValueArg<string> exprArg("expression",
                                                "Arithmetic expression to evaluate (use quotes)",
                                                true, "", "expression");
      cmd.add(exprArg);

      cmd.parse(argc, argv);

      string expr = exprArg.getValue();
      
      cout << "Expression Evaluator" << endl;
      cout << "====================" << endl;
      cout << "Input: " << expr << endl;
      
      // Create mutable copy for lexer
      char* input = new char[expr.size() + 1];
      strcpy(input, expr.c_str());
      
      int result = eval(input);
      
      cout << "Result: " << result << endl;
      
      delete[] input;
    }
  catch (TCLAP::ArgException &e)
    {
      cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
      return 1;
    }

  return 0;
}
