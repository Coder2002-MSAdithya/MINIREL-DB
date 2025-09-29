#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/closecats.h"
#include "../include/closedb.h"
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

/**
 * Closes the currently opened database and changes
 * the working directory to the original directory from which MINIREL was invoked
 * argv[0] - "closedb"
 * argv[argc] - NIL
 *
 * @param argc
 * @param argv
 * @return OK or errorCode according to the type of error
 *
 * GLOBAL VARIABLES MODIFIED
 *      g_DBOpenFlag
 *
 * ERRORS REPORTED
 *      DB_NOT_OPEN
 *
 * ALGORITHM
 *      1. Check if some db is opened
 *      2. Call CloseCats() to close everything
 *      3. Change the directory to the invoked directory
 *
 * IMPLEMENTATION NOTES:
 *      Uses CloseCats()
 *
 */


int CloseDB(int argc, char **argv)
{
    if(!db_open)
    {
        return ErrorMsgs(DBNOTOPEN, print_flag);
    }

    if(CloseCats() == OK)
    {
        db_open = false;
        chdir(ORIG_DIR);
        printf("Database %s CLOSED.\n", DB_DIR);
        return OK;
    }
    else
    {
        return ErrorMsgs(FILESYSTEM_ERROR, print_flag);
    }
}