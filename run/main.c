#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include "../include/globals.h"
#include "../include/error.h"
#include "../include/quit.h"

extern void parser();

int saveOrigDir()
{
    if(!getcwd(ORIG_DIR, MAX_PATH_LENGTH))
    {
        return ErrorMsgs(FILESYSTEM_ERROR, print_flag);
    }

    return OK;
}

void sigIntHandler(int sig)
{
    printf("You have pressed Ctrl+C...\n");
    printf("Closing currently open database.\n");
    char *argv[] = {"quit"};
    Quit(1, argv);
}

void main()
{   
    printf("\n\nWelcome to MINIREL Database System\n\n");
    signal(SIGINT, sigIntHandler);
    if(saveOrigDir() == OK)
    {   
        parser();
    }
    printf("\nGoodbye from MINIREL\n\n\n");
}
