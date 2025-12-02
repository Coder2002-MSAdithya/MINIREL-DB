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

/**
 * Creates a new database. It creates the relation and attribute catalogs and loads them with
 * the appropriate initial information.
 *
 * @param argc
 * @param argv
 * @return OK or NOTOK
 */

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