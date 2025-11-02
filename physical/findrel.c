#include <stdio.h>
#include <stddef.h>
#include "../include/globals.h"
#include "../include/defs.h"
#include "../include/findrec.h"
#include "../include/helpers.h"

int FindRel(char *relName)
{
    Rid foundRid = INVALID_RID;
    RelCatRec rc;
    FindRec(RELCAT_CACHE, foundRid, &foundRid, &rc, 's', 
    RELNAME, offsetof(RelCatRec, relName), relName, CMP_EQ);
    return isValidRid(foundRid);
}