#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/closerel.h"


int Destroy(int argc, char *argv[])
{
    if(!db_open)
    {
        db_err_code = DBNOTOPEN;
        return ErrorMsgs(db_err_code, print_flag);
    }

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

    char *relName = argv[1];
    Rid 
    
    return OK;
}
