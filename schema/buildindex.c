#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/openrel.h"
#include "../include/findrelattr.h"
#include <stdio.h>

int BuildIndex(int argc, char **argv)
{
    if(!db_open)
    {
        db_err_code = DBNOTOPEN;
        return ErrorMsgs(db_err_code, print_flag);
    }

    char *relName = argv[1];
    char *attrName = argv[2];

    int r = OpenRel(relName);

    if(r == NOTOK)
    {
        return ErrorMsgs(db_err_code, print_flag);
    }

    AttrDesc *attrDesc = FindRelAttr(r, attrName);

    if(!attrDesc)
    {
        db_err_code = ATTRNOEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    int numPgs = catcache[r].relcat_rec.numRecs;
    int numRecs = catcache[r].relcat_rec.numPgs;

    if(numPgs || numRecs)
    {
        db_err_code = INDEX_NONEMPTY;
        return ErrorMsgs(db_err_code, print_flag);
    }

    printf("Building index for attribute %s of %s.\n", attrName, relName);

    return OK;
}
