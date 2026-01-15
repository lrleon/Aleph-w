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
 * @author Leandro Rabindranath León
 * @ingroup Examples
*/

/**
 * @file evalExp.C
 * @brief Arithmetic expression evaluator using operator precedence
 * 
 * This example demonstrates a classic expression evaluator that correctly
 * handles operator precedence and associativity. It uses two stacks:
 * one for operands (values) and one for operators, implementing a variant
 * of the shunting-yard algorithm. This is fundamental to understanding
 * how compilers and interpreters evaluate expressions.
 *
 * ## The Expression Evaluation Problem
 *
### Challenge
 *
 * Evaluate arithmetic expressions like `3 + 4 * 2` correctly:
 * - **Wrong**: (3 + 4) * 2 = 14
 * - **Correct**: 3 + (4 * 2) = 11
 *
 * **Problem**: Operators have different precedence levels
 *
### Solution: Two-Stack Algorithm
 *
 * Use two stacks to manage operator precedence:
 * - **Operand stack**: Stores numbers
 * - **Operator stack**: Stores operators
 * - **Precedence rules**: Determine evaluation order
 *
## How It Works
 *
### Algorithm Overview
 *
 * The evaluator processes the expression left-to-right:
 * ```
 * evaluate(expression):
 *   operand_stack = []
 *   operator_stack = []
 *   
 *   for each token in expression:
 *     if token is number:
 *       operand_stack.push(token)
 *     else if token is operator:
 *       while operator_stack not empty and
 *             precedence(operator_stack.top()) >= precedence(token):
 *         evaluate_top_operator()
 *       operator_stack.push(token)
 *     else if token is '(':
 *       operator_stack.push(token)
 *     else if token is ')':
 *       while operator_stack.top() != '(':
 *         evaluate_top_operator()
 *       operator_stack.pop()  // Remove '('
 *   
 *   while operator_stack not empty:
 *     evaluate_top_operator()
 *   
 *   return operand_stack.top()
 * ```
 *
### Step-by-Step Example
 *
 * Expression: `3 + 4 * 2`
 *
 * ```
 * Token: 3
 *   Operand stack: [3]
 *   Operator stack: []
 *
 * Token: +
 *   Operand stack: [3]
 *   Operator stack: [+]
 *
 * Token: 4
 *   Operand stack: [3, 4]
 *   Operator stack: [+]
 *
 * Token: *
 *   Precedence(*) > Precedence(+), so push
 *   Operand stack: [3, 4]
 *   Operator stack: [+, *]
 *
 * Token: 2
 *   Operand stack: [3, 4, 2]
 *   Operator stack: [+, *]
 *
 * End of expression:
 *   Evaluate *: 4 * 2 = 8
 *   Operand stack: [3, 8]
 *   Operator stack: [+]
 *
 *   Evaluate +: 3 + 8 = 11
 *   Operand stack: [11]
 *   Operator stack: []
 *
 * Result: 11
 * ```
 *
## Operator Precedence
 *
### Precedence Levels
 *
 * | Operator | Precedence | Associativity | Notes |
 * |----------|------------|---------------|-------|
 * | +, -     | 1          | Left-to-right | Addition, subtraction |
 * | *, /     | 2          | Left-to-right | Multiplication, division |
 *
### Precedence Rules
 *
 * - **Higher precedence**: Evaluated first
 * - **Same precedence**: Left-to-right (associativity)
 * - **Parentheses**: Override precedence
 *
### Examples
 *
 * ```
 * 3 + 4 * 2     → 3 + (4 * 2) = 11
 * 10 - 2 - 3    → (10 - 2) - 3 = 5  (left-to-right)
 * (3 + 4) * 2   → 7 * 2 = 14  (parentheses override)
 * ```
 *
## Supported Features
 *
### Operators
 * - **+**: Addition
 * - **-**: Subtraction
 * - **\***: Multiplication
 * - **/**: Division (integer division)
 *
### Parentheses
 * - **( )**: Grouping and precedence override
 * - **Nested**: Supports nested parentheses
 *
### Data Types
 * - **Integer arithmetic**: All operations on integers
 * - **No floating point**: Integer-only evaluation
 *
### Error Handling
 * - **Malformed expressions**: Detects syntax errors
 * - **Mismatched parentheses**: Detects parenthesis errors
 * - **Invalid operators**: Detects unknown operators
 *
## Usage Examples
 *
 * ```bash
 * # Basic arithmetic
 * evalExp "3 + 4 * 2"        # Result: 11 (multiplication first)
 *
 * # With parentheses
 * evalExp "(3 + 4) * 2"      # Result: 14 (addition first)
 *
 * # Complex expression
 * evalExp "10 - 2 * 3 + 4"   # Result: 8 (2*3=6, then 10-6+4=8)
 *
 * # Nested parentheses
 * evalExp "((1 + 2) * 3) - 4"  # Result: 5
 * ```
 *
## Algorithm Variants
 *
### Shunting-Yard Algorithm
 *
 * This implementation is similar to the shunting-yard algorithm:
 * - **Shunting-yard**: Converts infix to postfix, then evaluates
 * - **This variant**: Evaluates directly using two stacks
 * - **Advantage**: More efficient (single pass)
 *
### Recursive Descent
 *
 * Alternative approach:
 * - **Recursive**: Parse expression recursively
 * - **Grammar-based**: Follows expression grammar
 * - **More complex**: But more flexible
 *
## Educational Value
 *
### Concepts Demonstrated
 *
 * - **Stack data structures**: Using stacks for parsing
 * - **Operator precedence**: Handling different precedence levels
 * - **Parsing algorithms**: Expression parsing techniques
 * - **State management**: Tracking parsing state with stacks
 * - **Algorithm design**: Two-stack approach
 *
### Learning Outcomes
 *
 * After studying this example, you understand:
 * - How compilers evaluate expressions
 * - Why operator precedence matters
 * - How to implement expression evaluators
 * - Stack-based algorithm design
 *
## Complexity
 *
 * | Aspect | Complexity | Notes |
 * |--------|-----------|-------|
 * | Time | O(n) | Single pass through expression |
 * | Space | O(n) | Stacks store tokens |
 * | Operators | O(1) | Constant number of operators |
 *
## Extensions
 *
### Possible Enhancements
 *
 * - **Floating point**: Support decimal numbers
 * - **More operators**: Exponentiation, modulo
 * - **Functions**: sin, cos, log, etc.
 * - **Variables**: Support variable names
 * - **Assignment**: Support variable assignment
 *
## Applications
 *
### Compilers and Interpreters
 * - **Expression parsing**: How languages evaluate expressions
 * - **Syntax analysis**: Part of compiler frontend
 * - **Code generation**: Generate code for expressions
 *
### Calculators
 * - **Scientific calculators**: Evaluate complex expressions
 * - **Programming calculators**: Support operator precedence
 *
### Formula Evaluators
 * - **Spreadsheets**: Excel, Google Sheets formula evaluation
 * - **Mathematical software**: Evaluate mathematical expressions
 *
 * @see tpl_arrayStack.H Stack implementation used
 * @see linear_structures_example.C Stack basics
 * @author Leandro Rabindranath León
 * @ingroup Examples
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
