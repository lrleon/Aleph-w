
# include <stdlib.h>
# include <stdio.h>
# include <stdarg.h>
# include <ahAssert.H>
# include <ahDefs.H>

/*
 These debug functions are only defined in basic.H if DEBUG flag is
 defined. However, since basic.o is destined to be a library and that
 is not frequently compiled, we do not use the preprocessor for
 conditioning the functions definitions. Because they are in library,
 they are only used if they are inquired. So, we minimize misuse
 e.g. if we forget to define DEBUG, we get a "undefined reference"
 message while linking symbol using DEBUG
*/ 

void _assert(const char *exp, const char* fileName, unsigned lineNumber)
{
  MESSAGE("Iion %s in line %u file %s violated\n", 
	  exp, lineNumber, fileName);
  abort();
}

void _assert(const char *exp, 
	     const char*  fileName, 
	     unsigned int lineNumber,
	     const char*  format,
	     ...)
{
  MESSAGE("Iion %s in line %u file %s violated\n", 
	  exp, lineNumber, fileName);
  
  va_list ap;
  va_start(ap, format);
  vprintf(format, ap);
  va_end(ap);

  abort();
}

void _Warning(const char *exp, const char* fileName, unsigned lineNumber)
{
  MESSAGE("Condition %s in line %u file %s has been violated\n"
	  "Programmer decided does not abort on that violation\n", 
	  exp, lineNumber, fileName);
}

void _Warning(const char *exp, 
	      const char*  fileName, 
	      unsigned int lineNumber,
	      const char*  format,
	      ...)
{
  MESSAGE("Warning %s in line %u file %s\n"
	 "(Programmer decided does not abort on that violation)\n",
	  exp, lineNumber, fileName);
  
  va_list ap;
  va_start(ap, format);
  vprintf(format, ap);
  va_end(ap);

  abort();
}

