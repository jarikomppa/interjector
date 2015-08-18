/* Interjector 1.0
* C code instrumentation tool
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global options

// Verbose: print out all sorts of debug information
int gVerboseMode = 0;
// Quiet: don't print out anything unless it's an error.
int gQuietMode = 0;
// Test mode: Whether to write stuff to disk at all
int gTestMode = 0;

// Mode for testing the interjection on this source
int gInterjectMode = 0;

// Other global gSourceData

// Source file name: needed to print out meaningful error message
char *gSourceFilename = NULL;
// Data: the source gSourceData loaded from disk
char *gSourceData = NULL;
// Data length: the length of gSourceData loaded from disk
int gSourceDataLength = 0;

// Target file for writing
FILE *gOutputFile = NULL;



// Utility functions

// Write a single byte if file is opened
void writeByte(int aByte)
{
    if (gOutputFile)
    {
        fputc(aByte, gOutputFile);
    }
}


// Write a asciiz string using writeByte
void writeString(char *aString)
{
    char *t = aString;
    while (*t)
    {
        writeByte(*t);
        t++;
    }
}


// Check if string is 'return', guard against 'returnValue' or some such
int checkForReturn(char *aDataPointer)
{
    char match[] = "return\0";
    char *m = match;
    while (*m && *aDataPointer && *m == *aDataPointer)
    {
        m++;
        aDataPointer++;
    }

    if (*m == 0)
    {
        // guard against 'returnValue' or some such variables..
        // (assuming legal characters for a variable are a-z,A-Z,0-9 and _)
        if (*aDataPointer == '_' || 
            (*aDataPointer >= '0' && *aDataPointer <= '9') || 
            (*aDataPointer >= 'a' && *aDataPointer <= 'z') || 
            (*aDataPointer >= 'A' && *aDataPointer <= 'Z'))
        {
            return 0; // No match: string continues
        }
        return 1; // Match
    }
    return 0; // No match
}


// Do the actual work
void scan()
{
    int commentBlock = 0; // Are we within multi-line /* */ - comment block
    int lineComment = 0;  // Are we within rest-of-the-line // - comment block 
    int quoteBlock = 0;   // Are we within literal string "" - block
    int codeBlock = 0;    // Code block depth {}
    int position = 0;     // Position in source
    int lastFunctionNamePosition = 0; // Position of the last function name
    int function = 0;     // Within function
    int functionReturned = 0; // Has function returned (for void funcs and others without last-line return)
    int preprocessor = 0; // Preprocessor directive
    int returnBlock = 0;  // Within return block (return ... ;)

    // prefix the include file
    writeString("#include \"interject.h\"\r\n");

    while (position < gSourceDataLength)
    {
        if (commentBlock)
        {
            // In a comment block - look for the end of the comment block..
            if (gSourceData[position] == '/' && gSourceData[position - 1] == '*')
            {
                commentBlock = 0;
            }
        }
        else
        {
            if (lineComment)
            {
                // In a line comment block.. ends when line ends
                if (gSourceData[position] == '\n' || gSourceData[position] == '\r')
                {
                    lineComment = 0;
                }
            }
            else
            {
                if (quoteBlock)
                {
                    // Quite block ends at another non-escaped quote character.
                    if (gSourceData[position] == '\"'  && gSourceData[position - 1] != '\\')
                    {
                        quoteBlock = 0;
                    }
                }
                else
                {
                    // Otherwise we're parsing code.
                    switch (gSourceData[position])
                    {
                    case ' ':
                    case '\t':
                    case '\n':
                    case '\r':
                        if (!function)
                        {
                            // If we're not in a function and we've seen whitespace, the function
                            // name may possibly follow.
                            lastFunctionNamePosition = position;
                        }
                        if (gSourceData[position] == '\n' ||
                            gSourceData[position] == '\r')
                        {
                            // check for line-continuation stuff related to preprocessor macros to 
                            // make sure that the macro has really ended..

                            if (position > 0 && gSourceData[position - 1] == '\n' || gSourceData[position - 1] == '\r')
                            {
                                if (position > 1 && gSourceData[position - 2] != '\\')
                                {
                                    preprocessor = 0;
                                }
                            }
                            else
                            {
                                if (position > 0 && gSourceData[position - 1] != '\\')
                                {
                                    preprocessor = 0;
                                }
                            }
                        }

                        if (function)
                        {
                            // If we're in a function, look for exit points.
                            if (gSourceData[position + 1] == 'r')
                            {
                                if (checkForReturn(&gSourceData[position + 1]))
                                {
                                    returnBlock = 1;
                                    functionReturned = 1;
                                    writeString("{ FUNCTION_LEAVE(\"");
                                    int i = 1;
                                    while (gSourceData[lastFunctionNamePosition + i] != '(')
                                    {
                                        writeByte(gSourceData[lastFunctionNamePosition + i]);
                                        i++;
                                    }
                                    writeString("\");");
                                }
                            }
                        }
                        break;
                    case ';':
                        if (function && codeBlock == 0)
                        {
                            // Not a function but a prototype.
                            function = 0;
                        }
                        break;
                    case '#':
                        // Start of a preprocessor directive.
                        preprocessor = 1;
                        break;
                    case '(':
                        // If at global level, and not parsing a preprocessor directive,
                        // we're hit a function.
                        if (!preprocessor && codeBlock == 0)
                        {
                            function = 1;
                            functionReturned = 0;
                        }
                        break;                
                    case '{':
                        // Deeper code block
                        codeBlock++;
                        break;
                    case '}':
                        // End of a code block
                        codeBlock--;
                        break;
                    case '/':
                        // Did we hit a line comment?
                        if (position > 0 && gSourceData[position - 1] == '/')
                        {
                            lineComment = 1;
                        }
                        break;
                    case '*':
                        // Did we hit a comment block?
                        if (position > 0 && gSourceData[position - 1] == '/')
                        {
                            commentBlock = 1;
                        }
                        break;
                    case '\"':
                        // Did we hit a literal text block?
                        if (position == 0 || gSourceData[position - 1] != '\\')
                        {
                            quoteBlock = 1;
                        }
                        break;
                    default:
                        if (functionReturned && !returnBlock && !preprocessor)
                        {
                            // function's return wasn't the last thing in function.
                            functionReturned = 0;
                        }
                        break;                    
                    }
                }
            }
        }

        if (gSourceData[position] == '}' && codeBlock == 0 && function)
        {
            // If function ends, we need to check if we already handled the return case, and
            // if not, write a leave macro here.
            function = 0;
            if (!functionReturned)
            {
                writeString("FUNCTION_LEAVE(\"");
                int i = 1;
                while (gSourceData[lastFunctionNamePosition + i] != '(')
                {
                    writeByte(gSourceData[lastFunctionNamePosition + i]);
                    i++;
                }
                writeString("\");\r\n");
            }
        }

        // Write source out.
        writeByte(gSourceData[position]);

        // Did we hit the end of a return block?
        if (gSourceData[position] == ';' && returnBlock)
        {
            writeString("}");
            returnBlock = 0;
        }

        // Did we find the start of a function?
        if (gSourceData[position] == '{' && codeBlock == 1 && function)
        {
            writeString("\r\nFUNCTION_ENTRY(\"");
            int i = 1;
            while (gSourceData[lastFunctionNamePosition+i] != '(')
            {
                switch (gSourceData[lastFunctionNamePosition + i])
                {
                case ' ':
                case '\t':
                case '\n':
                case '\r':
                    if (gQuietMode)
                    {
                        printf("%s:", gSourceFilename);
                    }
                    printf("Parsing got confused (detecting function name); instrumentation aborted.\n");
                    exit(0);                        
                }
                writeByte(gSourceData[lastFunctionNamePosition + i]);
                i++;
            }
            writeString("\");\r\n");


        }
        position++;  
    }

    // Error check - at end of file, was any of the blocks still open?
    if (commentBlock || lineComment || quoteBlock || codeBlock || returnBlock)    
    {
        if (gQuietMode)
        {
            printf("%s:", gSourceFilename);
        }

        printf("Parsing got confused at some point; instrumentation aborted\n");

        if (commentBlock)
        {
            printf("- Comment block left open (/* .. */)\n");
        }

        if (lineComment)
        {
            printf("- Line comment left open (// .. EOL)\n");
        }

        if (quoteBlock)
        {
            printf("- Quote block left open (\" .. \")\n");
        }

        if (codeBlock)
        {
            printf("- Code block left open ({ .. })\n");
        }

        if (returnBlock)
        {
            printf("- Return block left open (return .. ;)\n");
        }

        exit(-1);
    }
}


// Print out program header
void header()
{
    printf("Interjector v1.0 Copyright (c) 2007 Jari Komppa\nhttp://iki.fi/sol/\n");
}


// Entry point
void main(int parc, char **pars)
{
    int i;
    FILE *f;
    char *targetFilename = NULL;
    
    // Handle commandline parameters
    for (i = 1; i < parc; i++)
    {
        if (pars[i][0] == '-')
        {
            switch(pars[i][1])
            {
            case 'i':
            case 'I': gInterjectMode = 1;
#ifndef FUNCTION_ENTRY
                printf("\nNote: Interject test mode is useless unless"
                       "\ninterjector has been run through interjector.\n\n");
#endif
                /* FALLTHROUGH */
            case 'q':
            case 'Q': gQuietMode = 1;
                break;
            case 't':
            case 'T': gTestMode = 1;
                /* FALLTHROUGH */
            case 'v':
            case 'V': gVerboseMode = 1;
                break;
            default:
                printf("Unknown flag \"%s\". Run without parameters for help.\n", pars[i]);
                exit(-1);
            }
        }
        else
        {
            // First non-flag is the source
            if (gSourceFilename == NULL)
            {
                gSourceFilename = pars[i];
            }
            else
            {
                // and the second is (optional) target name
                if (targetFilename == NULL)
                {
                    targetFilename = pars[i];
                }
                else
                {
                    // if there's a third, that's an error
                    printf("Parameter error. Run without parameters for help.\n");
                    exit(-1);
                }            
            }
        }
    }

    // If no source filename was given, toss out help and quit
    if (gSourceFilename == NULL)
    {        
        header();
        printf("\nUsage:"
            "\ninst [options] sourcefile.cpp [targetfile.cpp]"
            "\nIf no targetfile is specified, _i is postfixed."
            "\n"
            "\nIt is legal to use the same name for input and output;"
            "\nthis will overwrite the original file."
            "\nOptions:"
            "\n-q - Quiet mode - only print out on errors"
            "\n-v - Verbose mode (for debugging)"
            "\n-t - Test - Do all the work but don't actually write to disk"
            "\n     (Implies verbose)"
            "\n-i - Interject printout (testing, implies quiet mode)"
            "\n");
        return;
    }

    // If we're supposed to be quiet, be wewy wewy quiet
    if (!gQuietMode)
    {
        header();
    }

    // Open source..
    f = fopen(gSourceFilename, "rb");

    if (f == NULL)
    {
        printf("Error: '%s' not found\n", gSourceFilename);
        return;
    }

    if (gVerboseMode)
    {
        printf("%s\n", gSourceFilename);
    }
    else
    {
        if (!gQuietMode)
        {
            printf("%s ", gSourceFilename);
        }
    }

    // find out length, allocate memory, and read the whole file
    fseek(f, 0, SEEK_END);
    gSourceDataLength = ftell(f);
    fseek(f, 0, SEEK_SET);
    gSourceData = new char[gSourceDataLength + 1];
    memset(gSourceData, 0, gSourceDataLength + 1);
    fread(gSourceData, 1, gSourceDataLength, f);
    fclose(f);

    if (gVerboseMode)
    {
        printf("%d bytes read\n", gSourceDataLength);
    }

    // Do a first pass without writing to disk. We do this in case something
    // goes wrong, we don't end up with a partial bad file.
    scan();

    if (gVerboseMode)
    {
        printf("pre-scanning ok, file will most likely be instrumented correctly\n");
    }

    if (gTestMode)
    {
        // All done, return
        return;
    }

    // Now we're ready to do it again for real.

    char *fn;
    // Do we need to generate target filename?
    if (targetFilename == NULL)
    {
        fn = new char[strlen(gSourceFilename + 2)];
        int i = 0;
        while (gSourceFilename[i] != '.' && gSourceFilename[i])
        {
            fn[i] = gSourceFilename[i];
            i++;
        }
        fn[i] = '_'; i++;
        fn[i] = 'i'; i++;
        while (gSourceFilename[i - 2])
        {
            fn[i] = gSourceFilename[i-2];
            i++;
        }
        fn[i] = 0;             
    }
    else
    {
        fn = targetFilename;
    }

    gOutputFile = fopen(fn, "wb");

    // Do another pass with the target file open
    scan();

    fclose(gOutputFile);

    // And all done.

    if (gVerboseMode)
    {
        printf("'%s' written.\n", fn);
    }
    else
    {
        if (!gQuietMode)
        {
            printf("ok\n");  
        }
    }
}