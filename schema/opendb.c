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
    argc → number of command-line arguments.
    argv → array of pointers to argument strings.

SPECIFICATIONS:
    argv[0] = "opendb"
    argv[1] = database name (directory to open)
    argv[argc] = NIL

FUNCTION DESCRIPTION:
    This routine implements the database open command.  
    It switches the process working directory into the specified database directory, opens the catalog files, and initializes all global data structures required for further relational operations.
    The database must already exist on the filesystem, and no database should currently be open. 
    The routine validates the database path, attempts to move into it, and then calls OpenCats() to load relcat and attrcat.
    Any failure is reported appropriately.

    ALGORITHM:
        1) Check whether a database is already open; if so, report an error (DBNOTCLOSED).
        2) Copy and validate the provided database path.
        3) Attempt chdir() into the database directory:
            - If directory does not exist → DBNOTEXIST.
            - If filesystem error occurs → FILESYSTEM_ERROR.
        4) Call OpenCats() to load catalog metadata and initialize global structures.
            - If it fails, revert to original directory and report CAT_OPEN_ERROR.
        5) Set db_open to TRUE and return OK.

BUGS:
    None known.

ERRORS REPORTED:
    DBNOTCLOSED        – A database is already open.
    DBPATHNOTVALID     – The supplied path contains invalid characters.
    DBNOTEXIST         – The specified database directory does not exist.
    FILESYSTEM_ERROR   – A filesystem-level failure occurred.
    CAT_OPEN_ERROR     – Opening relation/attribute catalogs failed.

GLOBAL VARIABLES MODIFIED:
    DB_DIR       – stores the path of the opened database.
    db_open      – set to TRUE upon successful database opening.
    db_err_code  – set appropriately depending on failure.

IMPLEMENTATION NOTES:
    • chdir() determines existence and accessibility of database.
    • OpenCats() performs initialization of all catalog caches.

------------------------------------------------------------*/

int OpenDB(int argc, char **argv)
{
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

    //OpenCats also initializes various global data structures
    if(OpenCats() == OK)
    {
        printf("Database %s has been opened successfully for use.\n", DB_DIR);
    }
    else
    {
        chdir(ORIG_DIR);
        return ErrorMsgs(CAT_OPEN_ERROR, print_flag);
    }
    
    db_open = true;

    return OK;
}