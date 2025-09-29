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

/*
 * Function:  DestroyDB()
 * ------------------------
 * deletes an existing database
 *
 * argv[0] = “destroydb”
 * argv[1] = database name
 * argv[argc] = NIL
 * 
 * returns: OK    upon successful deletion of directory.
 *          errorCode or NOTOK if directory CANNOT be deleted
 *
 * GLOBAL VARIABLES MODIFIED:
 *      <None>
 *
 * ERRORS REPORTED:
 *      ARGC_INSUFFICIENT
 *      DBNOTEXIST
 *      FILESYSTEM_ERROR
 *
 * ALGORITHM:
 *   1. Checks if the database is closed
 *   2. If closed, checks the existence of given database name
 *   3. If exists, deletes the directory and it's contents.
 *
 * IMPLEMENTATION NOTES:
 *      Uses only local functions except error reporting.
 *
 */

int DestroyDB(int argc, char **argv)
{
    if(argc < 2)
    {
        return ErrorMsgs(ARGC_INSUFFICIENT, print_flag);
    }

    if(!strcmp(DB_DIR, argv[1]))
    {
        char *argv2[] = {"closedb"};
        CloseDB(1, argv2);
    }

    if(!opendir(argv[1]))
    {
        return ErrorMsgs(DBNOTEXIST, print_flag);
    }

    if(remove_all_entry(argv[1]) == OK)
    {
        printf("Database %s destroyed.\n", argv[1]);
    }
    else
    {
        return ErrorMsgs(DBDESTROYERROR, print_flag);
    }

    return OK;
}
