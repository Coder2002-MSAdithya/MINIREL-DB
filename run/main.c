#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include "../include/globals.h"
#include "../include/error.h"

extern void parser();

int saveOrigDir()
{
    if(!getcwd(ORIG_DIR, MAX_PATH_LENGTH))
    {
        return ErrorMsgs(FILESYSTEM_ERROR, print_flag);
    }

    return OK;
}

void main()
{
    printf("\n\nWelcome to MINIREL Database System\n\n");
    if(saveOrigDir() == OK)
    {
        parser();
    }
    printf("\nGoodbye from MINIREL\n\n\n");
}
