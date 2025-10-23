#include "../include/defs.h"
#include "../include/helpers.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/opencats.h"
#include "../include/readpage.h"
#include "../include/openrel.h"
#include "../include/closerel.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

/*
 * Function: OpenDB()
 * ------------------------
 * changes the working directory to be the database directory, opens the catalogs and initializes 
 * the various global data structures
 *
 * argv[0] = “opendb”
 * argv[1] = database name
 * argv[argc] = NIL
 *
 *  returns: OK    upon opening database
 *           NOTOK or errorCode upon failure
 *
 * GLOBAL VARIABLES MODIFIED:
 *      DB_DIR is assigned with invoked location of minirel.
 *      db_open is set to true
 *
 * ERRORS REPORTED:
 *      ARGC_INSUFFICIENT
 *      DBNOTCLOSED
 *      DBNOTEXIST
 *
 * ALGORITHM:
 *   1. Checks for Errors.
 *   2. changes to given database directory and then OpenCats.
 *   
 * IMPLEMENTATION NOTES:
 *      Uses OpenCats from physical layer.
 *
 */

int OpenDB(int argc, char **argv)
{
    if(argc < 2)
    {
        return ErrorMsgs(ARGC_INSUFFICIENT, print_flag);
    }

    if(db_open)
    {
        return ErrorMsgs(DBNOTCLOSED, print_flag);
    }

    strncpy(DB_DIR, argv[1], MAX_PATH_LENGTH);

    if(!isValidPath(DB_DIR))
    {
        return ErrorMsgs(DBPATHNOTVALID, print_flag);
    }

    if(chdir(DB_DIR) == NOTOK)
    {
        if(errno == ENOENT)
        {
            return ErrorMsgs(DBNOTEXIST, print_flag);
        }
        else
        {
            return ErrorMsgs(FILESYSTEM_ERROR, print_flag);
        }
    }

    db_open = true;

    //OpenCats also initializes various global data structures
    if(OpenCats() == OK)
    {
        printf("Database %s has been opened successfully for use.\n", DB_DIR);
    }
    else
    {
        chdir(ORIG_DIR);
        db_open = false;
        return ErrorMsgs(CAT_OPEN_ERROR, print_flag);
    }

    return OK;
}
