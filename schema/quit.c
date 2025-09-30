#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/closedb.h"
#include <stdio.h>

/*
 * Function:  Quit()
 * ------------------------
 * Terminates the program after closing database, if open
 *
 * argv[0] = “quit”
 * argv[argc] = NIL
 * 
 * GLOBAL VARIABLES MODIFIED:
 *      <None>
 *
 * ERRORS REPORTED:
 *      <None>
 *
 * ALGORITHM:
 *   1. If database is not closed, call CloseDB
 *   2. Frees up allocated locations
 *
 * IMPLEMENTATION NOTES:
 *      Uses CloseDB from schema layer.
 */

int Quit(int argc, char **argv)
{
    if(db_open)
    {
        char *argv2[] = {"closedb"};
        CloseDB(1, argv2);
    }

    return OK;
}
