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
    argc  → number of command-line arguments.
    argv  → pointer to an array of argument strings.

SPECIFICATIONS:
    argv[0] = "createdb"
    argv[1] = name (or path) of the database directory to create
    argv[argc] = NIL

FUNCTION DESCRIPTION:
    The routine creates a new database directory in the filesystem, initializes it, creates the system catalogs (relation catalog and attribute catalog), and populates them with their initial entries.
    Rules and requirements:
        • The specified path must be a valid filesystem path.
        • The database directory must not already exist.
        • The routine must create the directory and switch into it.
        • Catalog files must be created and initialized correctly.
        • Any failure in directory creation, path validation, or catalog creation must generate an appropriate error.
    A database is considered successfully created only if:
        (1) Directory creation succeeds.
        (2) Switching into the directory succeeds.
        (3) CreateCats() completes without error.

ALGORITHM:
    1) Extract the database path from argv[1].
    2) Validate the path using isValidPath():
        if invalid → return DBPATHNOTVALID.
    3) Attempt to create the directory using mkdir():
        if directory already exists → return DBEXISTS.
        if any other filesystem error → return FILESYSTEM_ERROR.
    4) Change directory (chdir) into the newly created DB directory:
        if this fails → return FILESYSTEM_ERROR.
    5) Call CreateCats() to build the relation and attribute catalogs:
        if catalog creation fails → return CAT_CREATE_ERROR.
    6) Return to the original working directory.
    7) If all operations succeed, print a success message.
    8) Return OK.

BUGS:
    None known.

ERRORS REPORTED:
      DBPATHNOTVALID     - The provided database path is invalid.
      DBEXISTS           - A database directory with the same name already exists.
      FILESYSTEM_ERROR   - mkdir() or chdir() failed due to OS-level issues.
      CAT_CREATE_ERROR   - Catalog creation failed internally.

GLOBAL VARIABLES MODIFIED:
    db_err_code
    ORIG_DIR (indirectly through chdir())
    print_flag (used by ErrorMsgs)

IMPLEMENTATION NOTES:
    • CreateCats() is responsible for constructing relcat and attrcat files.
    • On error, error messages are produced through ErrorMsgs().
    • After creating the DB, the working directory is restored to ORIG_DIR.

------------------------------------------------------------*/

int CreateDB(int argc, char **argv)
{
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