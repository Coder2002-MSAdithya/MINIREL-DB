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
        return ErrorMsgs(DBNOTOPEN, print_flag);
    }

    char *relName = argv[1];
    char *attrName = argv[2];

    int r = OpenRel(relName);

    if(r == NOTOK)
    {
        return ErrorMsgs(RELNOEXIST, print_flag);
    }

    AttrDesc *attrDesc = FindRelAttr(r, attrName);

    if(!attrDesc)
    {
        return ErrorMsgs(ATTRNOEXIST, print_flag);
    }

    int numPgs = catcache[r].relcat_rec.numRecs;
    int numRecs = catcache[r].relcat_rec.numPgs;

    if(numPgs || numRecs)
    {
        return ErrorMsgs(INDEX_NONEMPTY, print_flag);
    }

    printf("Building index for attribute %s of %s.\n", attrName, relName);

    return OK;
}
