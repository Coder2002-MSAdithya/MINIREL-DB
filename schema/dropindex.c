#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/openrel.h"
#include "../include/findrelattr.h"
#include <stdio.h>


int DropIndex (int argc, char **argv)
{
    if(!db_open)
    {
        db_err_code = DBNOTOPEN;
        return ErrorMsgs(db_err_code, print_flag);
    }

    char *relName = argv[1];
    char *attrName = argc == 3 ? argv[2] : NULL;

    AttrDesc *attrPtr = NULL;

    int r = OpenRel(relName);

    if(r == NOTOK)
    {
        return ErrorMsgs(db_err_code, print_flag);
    }

    if(argc == 3)
    {
        attrPtr = FindRelAttr(r, attrName);

        if(!attrPtr)
        {
            db_err_code = ATTRNOEXIST;
            return ErrorMsgs(db_err_code, print_flag);
        }
    }

    if(attrPtr)
    {
        // Remove ONLY index on that attribute ...
    }
    else
    {
        // Remove all indices on relation r ...
    }
}
