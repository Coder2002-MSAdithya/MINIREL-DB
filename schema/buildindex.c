#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/openrel.h"
#include "../include/findrelattr.h"
#include "../include/writerec.h"
#include <stdio.h>
#include <string.h>

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

    if(strncmp(relName, RELCAT, RELNAME) == OK || 
    strncmp(relName, ATTRCAT, RELNAME) == OK)
    {
        db_err_code = METADATA_SECURITY;
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

    if(attrDesc->attr.hasIndex)
    {
        db_err_code = IDXEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    attrDesc->attr.hasIndex = 1;
    if(WriteRec(ATTRCAT_CACHE, &(attrDesc->attr), attrDesc->attrCatRid) == NOTOK)
    {
        return ErrorMsgs(db_err_code, print_flag);
    }

    printf("Built index successfully on attribute %s of relation %s\n", 
    attrName, relName);

    return OK;
}