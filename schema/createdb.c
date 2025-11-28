/************************INCLUDES*******************************/
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/createdb.h"
#include "../include/createcats.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>


/*------------------------------------------------------------

FUNCTION CreateDB (argc, argv)

PARAMETER DESCRIPTION:
    argc → Number of command-line arguments.
    argv → Argument list.
           argv[0] should be "createdb". 
           argv[1] is expected to be the database directory name to create. The function assumes argv contains valid, null-terminated strings.

FUNCTION DESCRIPTION:
    Creates a new MiniRel database directory and initializes all system catalogs inside it. 
    Performs path validation, attempts to create the directory, changes into it, and then calls the catalog-creation routine (CreateCats). 
    If catalog creation succeeds, the working directory is restored.

ALGORITHM:
    1. Validate that argc = 2.
    2. Copy database path from argv[1] into a local buffer.
    3. Validate directory path format.
    4. Attempt mkdir():
        - If EEXIST → report DBEXISTS.
        - If other error → report FILESYSTEM_ERROR.
    5. Change current directory to the new DB directory.
    6. Call CreateCats() to create:
        - relcat
        - attrcat
    7. Change back to ORIG_DIR.
    8. If success, print success message, else report CAT_CREATE_ERROR.

ERRORS REPORTED:
    ARGC_INSUFFICIENT → argc < 2.
    TOO_MANY_ARGS     → args > 2.
    DBPATHNOTVALID    → invalid database path.
    DBEXISTS          → database directory already exists.
    FILESYSTEM_ERROR  → mkdir/chdir failures.
    CAT_CREATE_ERROR  → failure in CreateCats().

GLOBAL VARIABLES MODIFIED:
    db_err_code

------------------------------------------------------------*/

int CreateDB(int argc, char **argv)
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

    char DB_PATH[MAX_PATH_LENGTH];

    strncpy(DB_PATH, argv[1], MAX_PATH_LENGTH);

    if(!isValidPath(DB_PATH))
    {
        db_err_code = DBPATHNOTVALID;
        return ErrorMsgs(db_err_code, print_flag);
    }
    
    if(mkdir(DB_PATH, 0777) == NOTOK)
    {
        if(errno == EEXIST)
        {
            db_err_code = DBEXISTS;
            return ErrorMsgs(db_err_code, print_flag);
        }
        else
        {
            db_err_code = FILESYSTEM_ERROR;
            return ErrorMsgs(db_err_code, print_flag);
        }
    }
    
    if(chdir(DB_PATH) == NOTOK)
    {
        db_err_code = FILESYSTEM_ERROR;
        return ErrorMsgs(db_err_code, print_flag);
    }

    int flag = CreateCats();

    chdir(ORIG_DIR);

    if(flag == OK)
    {
        printf("DB %s successfully created.\n", DB_PATH);
    }
    else
    {
        db_err_code = CAT_CREATE_ERROR;
        return ErrorMsgs(db_err_code, print_flag);
    }

    return flag;
}