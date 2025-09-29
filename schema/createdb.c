#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/createdb.h"
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
    if(argc < 2)
    {
        return ErrorMsgs(ARGC_INSUFFICIENT, false);
    }

    char DB_PATH[MAX_PATH_LENGTH];

    strncpy(DB_PATH, argv[1], MAX_PATH_LENGTH);

    if(!isValidPath(DB_PATH))
    {
        return ErrorMsgs(DBPATHNOTVALID, print_flag);
    }
    
    if(mkdir(DB_PATH, 0777) == NOTOK)
    {
        if(errno == EEXIST)
        {
            return ErrorMsgs(DBEXISTS, print_flag);
        }
        else
        {
            return ErrorMsgs(FILESYSTEM_ERROR, print_flag);
        }
    }
    
    if(chdir(DB_PATH) == NOTOK)
    {
        return ErrorMsgs(FILESYSTEM_ERROR, print_flag);
    }

    int flag = CreateCats();

    chdir(ORIG_DIR);

    if(flag == OK)
    printf("DB %s successfully created.\n", DB_PATH);
    else
    printf("Error in creating DB.\n");

    return flag;
}