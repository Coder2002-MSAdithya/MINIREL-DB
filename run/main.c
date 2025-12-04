/************************INCLUDES*******************************/

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include "../include/globals.h"
#include "../include/error.h"
#include "../include/quit.h"

extern void parser();


/*------------------------------------------------------------

FUNCTION saveOrigDir()

PARAMETER DESCRIPTION:
    None.

FUNCTION DESCRIPTION:
    Saves the directory from which MINIREL was launched into the global buffer ORIG_DIR. This directory is later used by CloseDB() and Quit() to restore the execution environment.

ALGORITHM:
    1) Call getcwd() to retrieve the current working directory.
    2) If successful, store it into ORIG_DIR, and return OK.
    3) If getcwd() fails, set db_err_code and report the error.

GLOBAL VARIABLES MODIFIED:
    ORIG_DIR

ERRORS REPORTED:
    FILESYSTEM_ERROR

IMPLEMENTATION NOTES:
    - Called before parser() starts.
    - Required for leaving the DB system gracefully.
    - No memory allocation is performed.

------------------------------------------------------------*/

int saveOrigDir()
{
    if(!getcwd(ORIG_DIR, MAX_PATH_LENGTH))
    {
        return ErrorMsgs(FILESYSTEM_ERROR, print_flag);
    }

    return OK;
}


/*------------------------------------------------------------

FUNCTION sigIntHandler(sig)

PARAMETER DESCRIPTION:
    sig → Signal number (SIGINT).

FUNCTION DESCRIPTION:
    Handles Ctrl+C interrupts.  
    Ensures MINIREL shuts down gracefully instead of terminating abruptly.
    The handler:
        - Notifies the user.
        - Invokes Quit() to close any open database.
        - Exits the program cleanly.

ALGORITHM:
    1) Print a message informing the user that Ctrl+C was pressed.
    2) Call Quit() to safely close the database if open.
    3) Terminate execution.

GLOBAL VARIABLES MODIFIED:
    None directly (Quit() performs the cleanup).

ERRORS REPORTED:
    None directly.

IMPLEMENTATION NOTES:
    - Ensures database files, buffers, and catalogs are not left in inconsistent states.

------------------------------------------------------------*/

void sigIntHandler(int sig)
{
    printf("You have pressed Ctrl+C...\n");
    printf("Closing currently open database.\n");
    char *argv[] = {"quit"};
    Quit(1, argv);
}


/*------------------------------------------------------------

FUNCTION sigSegvHandler(sig)

PARAMETER DESCRIPTION:
    sig → Signal number (SIGSEGV).

FUNCTION DESCRIPTION:
    Handles unexpected segmentation faults within the DBMS.
    Allows the system to shut down gracefully rather than crashing.
    The handler:
        - Warns the user about the memory error.
        - Calls Quit() to ensure the DB is closed safely.

ALGORITHM:
    1) Print an error message indicating a memory fault.
    2) Call Quit() to shut down MINIREL safely.

GLOBAL VARIABLES MODIFIED:
    None directly.

ERRORS REPORTED:
    None directly.

IMPLEMENTATION NOTES:
    - Prevents partial writes, corrupted catalog metadata, or orphaned open-file descriptors caused by abrupt termination.

------------------------------------------------------------*/

void sigSegvHandler(int sig)
{
    printf("There was a memory error with the DB system.\n");
    printf("Closing currently open database.\n");
    char *argv[] = {"quit"};
    Quit(1, argv);
}


/*------------------------------------------------------------

FUNCTION main()

PARAMETER DESCRIPTION:
    None.

FUNCTION DESCRIPTION:
    Entry point of the MINIREL system.
    Initializes signal handlers, saves original working directory, and invokes the parser loop that processes user commands.

PROCESS:
    1) Print welcome banner.
    2) Register SIGINT (Ctrl+C) and SIGSEGV handlers.
    3) Save original working directory.
    4) If successful, start parser() to accept user commands.
    5) Print goodbye message upon exit.

ALGORITHM:
    1) Print welcome message.
    2) Install signal handlers with signal().
    3) Call saveOrigDir(); if OK → call parser().
    4) When parser exits, print closing message.

GLOBAL VARIABLES MODIFIED:
    ORIG_DIR (through saveOrigDir)

ERRORS REPORTED:
    Only those propagated from saveOrigDir().

IMPLEMENTATION NOTES:
    - Program termination may occur through signal handlers or Quit function.

------------------------------------------------------------*/


void main()
{   
    printf("\n\nWelcome to MINIREL Database System\n\n");
    signal(SIGINT, sigIntHandler);
    signal(SIGSEGV, sigSegvHandler);
    if(saveOrigDir() == OK)
    {   
        parser();
    }
    printf("\nGoodbye from MINIREL\n\n\n");
}
