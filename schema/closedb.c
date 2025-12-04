/************************INCLUDES*******************************/

#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/closecats.h"
#include "../include/closedb.h"
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>


/*------------------------------------------------------------

FUNCTION CloseDB (argc, argv)

PARAMETER DESCRIPTION:
    argc → number of command-line arguments.
    argv → array of pointers to the argument strings.

SPECIFICATIONS:
    argv[0] = "closedb"
    argv[argc] = NIL

FUNCTION DESCRIPTION:
    This routine implements the database close command.
    It closes the currently open database by shutting down all catalog structures, releasing associated resources, and restoring the working directory back to the original invocation directory of MINIREL.
    A database must be open before this operation can be executed. 
    Any attempt to close when no database is active should result in an error.

ALGORITHM:
    1) Check whether a database is currently open.
        If not, report DBNOTOPEN.
    2) Invoke CloseCats() to close the relation and attribute catalogs and release global memory.
    3) Change directory back to ORIG_DIR.
    4) Mark db_open = FALSE.
    5) Report success.

BUGS:
    None known.

ERRORS REPORTED:
    DBNOTOPEN         – Attempted to close when no DB is open.
    FILESYSTEM_ERROR  – A filesystem-level error occurred while cleaning up or restoring directory state.

GLOBAL VARIABLES MODIFIED:
       db_open        – set to FALSE once the DB is closed.
       db_err_code    – updated when an error occurs.

IMPLEMENTATION NOTES (IF ANY):
       • Uses CloseCats() from the physical layer to release
         all catalog-related structures.
       • Assumes ORIG_DIR contains a valid path to return to.
       • No partial-state rollback is required.

------------------------------------------------------------*/

int CloseDB(int argc, char **argv)
{
    if(!db_open)
    {
        db_err_code = DBNOTOPEN;
        return ErrorMsgs(db_err_code, print_flag);
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
        db_err_code = FILESYSTEM_ERROR;
        return ErrorMsgs(db_err_code, print_flag);
    }
}