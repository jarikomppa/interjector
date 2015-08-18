/*
 * An example interject.h file. You can make the macros do anything
 * you like, naturally.
 */
 
static int __interject_call_depth = 0;
#define FUNCTION_ENTRY(x) if (gInterjectMode) printf("%-*s> %s\n", __interject_call_depth++, "", (x));
#define FUNCTION_LEAVE(x) if (gInterjectMode) printf("%-*s< %s\n", --__interject_call_depth, "", (x));