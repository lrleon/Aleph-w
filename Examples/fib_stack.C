# define P1 1 
# define P2 2

struct Activation_Record
{
  int n;
  int f1;
  int result;
  char return_point;
};
# define NUM(p)          ((p)->n) 
# define F1(p)           ((p)->f1)
# define RESULT(p)       ((p)->result)
# define RETURN_POINT(p) ((p)->return_point)
int fib_st(const int & n) 
{
  ArrayStack<Activation_Record> stack;
  Activation_Record * caller_ar  = &stack.pushn();
  Activation_Record * current_ar = &stack.pushn();
  NUM(current_ar) = n;
  start:
   if (NUM(current_ar) <= 1)
     {
       RESULT(caller_ar) = 1;
       goto return_from_fib;
     }
   RETURN_POINT(current_ar) = P1;
   NUM(&stack.pushn()) = NUM(current_ar) - 1; // crea reg. act.
   caller_ar  = &stack.top(1);
   current_ar = &stack.top();
   goto start;
  p1: /* punto de retorno de fib(n - 1) */
   F1(current_ar) = RESULT(current_ar);
     NUM(&stack.pushn()) = NUM(current_ar) - 2; // crea reg. act. 
   RETURN_POINT(current_ar) = P2;
   caller_ar  = &stack.top(1);
   current_ar = &stack.top();
   goto start;
  p2: /* punto de retorno de fib(n - 2) */
   RESULT(caller_ar) = F1(current_ar) + RESULT(current_ar);
  return_from_fib:
   stack.pop(); /* cae en el registro del invocante */
   if (stack.size() == 1) 
     return RESULT(caller_ar);

   caller_ar  = &stack.top(1);
   current_ar = &stack.top();

   switch (RETURN_POINT(current_ar))
     {
     case P1: goto p1;
     case P2: goto p2;
     default: AH_ERROR("Invalid return point case");
     } 

   return 0; // never reached!
}
