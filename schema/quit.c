/************************INCLUDES*******************************/

#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/closedb.h"
#include <stdio.h>
#include <stdlib.h>


/*------------------------------------------------------------

FUNCTION Quit (argc, argv)

PARAMETER DESCRIPTION:
    argc → number of command arguments.
    argv → argument vector.
        
SPECIFICATIONS:
    argv[0] = "quit"
    argv[argc] = NIL

FUNCTION DESCRIPTION:
    Terminates the MINIREL system. 
    If a database is currently open, it is first cleanly closed using CloseDB(), ensuring that catalog entries, buffers, and file handles are flushed and released. 
    After cleanup, the routine exits the process with status OK.

ALGORITHM:
    1) Check whether a database is currently open.
    2) If so, construct a minimal argument list and invoke CloseDB() to properly close catalogs, relations, and global state.
    3) Terminate the process by calling exit(OK).

BUGS:
    None known.

ERRORS REPORTED:
    None.

GLOBAL VARIABLES MODIFIED:
    None.

------------------------------------------------------------*/


int Quit(int argc, char **argv)
{
    if(db_open)
    {
        char *argv2[] = {"closedb"};
        CloseDB(1, argv2);
    }

    exit(OK);  
    return OK;
}
