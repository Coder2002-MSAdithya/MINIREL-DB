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
    argc  → number of command line arguments
    argv  → command line arguments
            argv[0] = "closedb"
            argv[argc] = NIL

FUNCTION DESCRIPTION:
    Closes the currently open database. This involves:
        - verifying that a database is open.
        - closing all system catalogs through CloseCats().
        - resetting the process working directory to the original directory from which MINIREL was invoked.
        - clearing global DB state.

ALGORITHM:
    1. If no database is open, report DBNOTOPEN.
    2. Call CloseCats() to flush buffers, close relations, free attribute lists, and close catalog files.
    3. Change directory back to ORIG_DIR.
    4. Mark db_open = false.
    5. Print confirmation message.

ERRORS REPORTED:
    DBNOTOPEN
    FILESYSTEM_ERROR

GLOBAL VARIABLES MODIFIED:
    db_open
    db_err_code
    catcache[]
    buffer[]
    DB_DIR (implicitly no longer relevant)

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