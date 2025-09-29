#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/closedb.h"
#include <stdio.h>


int Quit (int argc, char **argv)
{
    if(db_open)
    {
        char *argv2[] = {"closedb"};
        CloseDB(1, argv2);
    }
}
