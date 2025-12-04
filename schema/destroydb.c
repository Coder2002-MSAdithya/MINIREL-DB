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
    argc → number of command-line arguments.
    argv → pointer to array of command-line argument strings.

SPECIFICATIONS:
      argv[0] = "destroydb"
      argv[1] = database name (directory to delete)
      argv[argc] = NIL

FUNCTION DESCRIPTION:
    The routine implements the database destruction command.
    It deletes an existing database directory and all its contents.
    If the database is currently open, it is first closed.
    The directory must already exist and be accessible. 
    The routine must delete every file and subdirectory contained within it before removing the database directory itself.
    Typical usage requirements:
        • A database cannot be destroyed if it does not exist.
        • If the database being destroyed is the one currently open, it must be closed before deletion.
        • All internal filesystem errors must be reported.
        • remove_all_entry() is used to recursively delete contents.

ALGORITHM:
    1) If the database is currently open and the name provided matches the open database, call CloseDB() to close it.
    2) Attempt to open the directory using opendir():
        if this fails → directory does not exist → report DBNOTEXIST.
    3) Call remove_all_entry() to recursively delete all files and subdirectories:
        if this fails → report DBDESTROYERROR or FILESYSTEM_ERROR.
    4) If successful, print a confirmation message that the database was destroyed.
    5) Return OK.

BUGS:
    None known.

ERRORS REPORTED:
    DBNOTEXIST       - The database directory does not exist.
    DBDESTROYERROR   - Failed to delete some part of the directory tree.
    FILESYSTEM_ERROR - Filesystem-level failure encountered.

GLOBAL VARIABLES MODIFIED:
    db_err_code
    (db_open may be modified indirectly via CloseDB())

IMPLEMENTATION NOTES (IF ANY):
    • remove_all_entry() performs recursive deletion; DestroyDB() handles only checking and error reporting.
    • Uses CloseDB() only when destroying the currently open DB.
    • opendir() is only used as an existence check.

------------------------------------------------------------*/

int DestroyDB(int argc, char **argv)
{
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
