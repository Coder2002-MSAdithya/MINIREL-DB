#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/openrel.h"
#include "../include/findrelattr.h"
#include "../include/writerec.h"
#include <stdio.h>

int removeIndex(AttrDesc *attrPtr)
{
    attrPtr->attr.hasIndex = 0;

    if(WriteRec(ATTRCAT_CACHE, &(attrPtr->attr), attrPtr->attrCatRid) != OK)
    {
        return NOTOK;
    }

    return OK;
}

int DropIndex(int argc, char **argv)
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

        if(!attrPtr->attr.hasIndex)
        {
            db_err_code = IDXNOEXIST;
            return ErrorMsgs(db_err_code, print_flag);
        }
    }

    if(attrPtr)
    {
        removeIndex(attrPtr);
        printf("Destroyed index successfully on attribute %s of relation %s\n", 
        attrName, relName);
    }
    else
    {
        attrPtr = catcache[r].attrList;

        for(;attrPtr;attrPtr=attrPtr->next)
        {
            if(attrPtr->attr.hasIndex)
            removeIndex(attrPtr);
        }

        printf("Destroyed index successfully on all attributes of relation %s\n", 
        relName);
    }
}