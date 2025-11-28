/************************INCLUDES*******************************/
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/closedb.h"
#include "../include/helpers.h"
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>


/*------------------------------------------------------------

FUNCTION DestroyDB (argc, argv)

PARAMETER DESCRIPTION:
    argc → Number of command-line arguments.
    argv → Argument list. 
           argv[0] should be "destroydb".
           argv[1] is expected to be the name of the database directory to destroy.

FUNCTION DESCRIPTION:
    Deletes an existing MiniRel database directory and all of its contents. 
    If the database being destroyed is currently open, the function implicitly invokes CloseDB() to safely close all catalogs and relations before deletion.

ALGORITHM:
    1. Validate that argc = 2.
    2. If a database is currently open AND its name matches argv[1]:
        - call CloseDB() to close system catalogs and flush buffers.
    3. Attempt to open the directory using opendir():
        - If it fails → the database does not exist → report DBNOTEXIST.
    4. Call remove_all_entry() to recursively delete the directory and its contents.
    5. If deletion succeeds, print success message, else report DBDESTROYERROR.

ERRORS REPORTED:
    ARGC_INSUFFICIENT → argc < 2.
    TOO_MANY_ARGS     → args > 2.
    DBNOTEXIST        → directory does not exist.
    FILESYSTEM_ERROR  → lower-level file system failures.
    DBDESTROYERROR    → recursive deletion failed.

GLOBAL VARIABLES MODIFIED:
    - db_open (indirectly through CloseDB)
    - db_err_code

------------------------------------------------------------*/

int DestroyDB(int argc, char **argv)
{
    if(argc < 2)
    {
        db_err_code = ARGC_INSUFFICIENT;
        return ErrorMsgs(db_err_code, print_flag);
    }

    if(argc > 2)
    {
        db_err_code = TOO_MANY_ARGS;
        return ErrorMsgs(db_err_code, print_flag);
    }

    if(db_open && !strcmp(DB_DIR, argv[1]))
    {
        char *argv2[] = {"closedb"};
        CloseDB(1, argv2);
    }

    if(!opendir(argv[1]))
    {
        db_err_code = DBNOTEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    if(remove_all_entry(argv[1]) == OK)
    {
        printf("Database %s destroyed.\n", argv[1]);
    }
    else
    {
        db_err_code = DBDESTROYERROR;
        return ErrorMsgs(db_err_code, print_flag);
    }

    return OK;
}
