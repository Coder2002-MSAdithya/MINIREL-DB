#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include "../include/error.h"
#include "../include/defs.h"
#include "../include/globals.h"
#include "../include/insertrec.h"

/* Create a new relation with name relName with same attribute descriptors as relNum */
int CreateRel(char *relName, int relNum)
{
    RelCatRec new_rc = catcache[relNum].relcat_rec;
    new_rc.numPgs = new_rc.numRecs = 0;

    if(strlen(relName) >= RELNAME)
    {
        db_err_code = REL_LENGTH_EXCEEDED;
        return NOTOK;
    }

    AttrDesc *ptr = catcache[relNum].attrList;
    strncpy(new_rc.relName, relName, RELNAME);

    FILE *fptr = fopen(relName, "w");

    if(!fptr)
    {
        db_err_code = FILESYSTEM_ERROR;
        return NOTOK;
    }

    InsertRec(RELCAT_CACHE, &new_rc);

    for(;ptr;ptr=ptr->next)
    {
        AttrCatRec ac = ptr->attr;
        strncpy(ac.relName, relName, RELNAME);
        ac.hasIndex = 0;
        ac.nKeys = ac.nPages = 0;
        InsertRec(ATTRCAT_CACHE, &ac);
    }

    return OK;
}
