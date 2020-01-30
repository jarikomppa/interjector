/*
 * An example interject.h file. You can make the macros do anything
 * you like, naturally.
 */
 
static int __interject_call_depth = 0;
#define INTERJECTOR_FUNCTION_ENTRY(x) if (gInterjectMode) printf("%-*s> %s\n", __interject_call_depth++, "", (x));
#define INTERJECTOR_FUNCTION_LEAVE(x) if (gInterjectMode) printf("%-*s< %s\n", --__interject_call_depth, "", (x));
// Only if interjector is run with -l
#define INTERJECTOR_FUNCTION_LEAVE_LEVELS(x,y) if (gInterjectMode) printf("codeblock:%d\n", (y));
