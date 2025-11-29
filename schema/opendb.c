/************************INCLUDES*******************************/
#include "../include/defs.h"
#include "../include/helpers.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/opencats.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <errno.h>


/*------------------------------------------------------------

FUNCTION OpenDB (argc, argv)

PARAMETER DESCRIPTION:
    argc → number of command line arguments (must be = 2)
    argv → argument vector  
            argv[0] = "opendb"
            argv[1] = database directory path to open
            argv[argc] = NIL

FUNCTION DESCRIPTION:
    Opens a database directory and initializes the MINIREL environment. 
    The function:
        - validates arguments.
        - verifies that no database is already open.
        - verifies that the provided path is syntactically valid.
        - changes the process working directory to the DB directory.
        - loads the system catalogs using OpenCats().
        - initializes DB_DIR and sets db_open = true.

ALGORITHM:
    1. Check argc for correct count.
    2. Reject operation if a DB is currently open.
    3. Copy DB path into DB_DIR and validate format.
    4. chdir() into DB_DIR. If this fails, report DB-not-exist or filesystem error.
    5. Call OpenCats() to load system catalogs.
    6. On any failure, revert directory to ORIG_DIR and restore db_open = false.
    7. On success, print confirmation message.

ERRORS REPORTED:
    ARGC_INSUFFICIENT
    TOO_MANY_ARGS
    DBNOTCLOSED
    DBPATHNOTVALID
    DBNOTEXIST
    FILESYSTEM_ERROR
    CAT_OPEN_ERROR

GLOBAL VARIABLES MODIFIED:
    DB_DIR
    db_open
    db_err_code
    catcache[]
    buffer[]

------------------------------------------------------------*/

int OpenDB(int argc, char **argv)
{
    if(argc<2)
    {
        db_err_code = ARGC_INSUFFICIENT;
        return ErrorMsgs(db_err_code, print_flag);
    }

    if(argc>2)
    {
        db_err_code = TOO_MANY_ARGS;
        return ErrorMsgs(db_err_code, print_flag);
    }

    if(db_open)
    {
        db_err_code = DBNOTCLOSED;
        return ErrorMsgs(db_err_code, print_flag);
    }

    strncpy(DB_DIR, argv[1], MAX_PATH_LENGTH);

    if(!isValidPath(DB_DIR))
    {
        db_err_code = DBPATHNOTVALID;
        return ErrorMsgs(db_err_code, print_flag);
    }

    if(chdir(DB_DIR) == NOTOK)
    {
        if(errno == ENOENT)
        {
            db_err_code = DBNOTEXIST;
            return ErrorMsgs(db_err_code, print_flag);
        }
        else
        {
            db_err_code = FILESYSTEM_ERROR;
            return ErrorMsgs(db_err_code, print_flag);
        }
    }

    db_open = true;

    if(OpenCats() == OK)
    {
        printf("Database %s has been opened successfully for use.\n", DB_DIR);
    }
    else
    {
        chdir(ORIG_DIR);
        db_open = false;
        db_err_code = CAT_OPEN_ERROR;
        return ErrorMsgs(db_err_code, print_flag);
    }

    return OK;
}
