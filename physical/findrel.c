/************************INCLUDES*******************************/

#include <stdio.h>
#include <stddef.h>
#include "../include/globals.h"
#include "../include/defs.h"
#include "../include/findrec.h"
#include "../include/helpers.h"


/*--------------------------------------------------------------

FUNCTION FindRel(relName):

PARAMETER DESCRIPTION:
    relName  → Name of the relation whose existence is to be verified.

FUNCTION DESCRIPTION:
    This routine checks whether a relation exists in the system by searching the RelCat (system catalog of relations).  
    It uses FindRec() to scan for a tuple in RELCAT whose relName attribute matches the supplied relName.  
    If such a tuple exists, FindRel() returns 1. Otherwise, it returns 0.

ALGORITHM:
    1) Initialize a Rid variable with INVALID_RID to begin scanning.
    2) Call FindRec() on RELCAT_CACHE with comparison on the relName attribute, using CMP_EQ.
    3) If FindRec() sets foundRid to a valid RID, return 1 (relation exists), else return 0.

GLOBAL VARIABLES MODIFIED:
    None.

ERRORS REPORTED:
    None.

BUGS:
    None known.

---------------------------------------------------------------*/

int FindRel(char *relName)
{
    Rid foundRid = INVALID_RID;
    RelCatRec rc;
    FindRec(RELCAT_CACHE, foundRid, &foundRid, &rc, 's', 
    RELNAME, offsetof(RelCatRec, relName), relName, CMP_EQ);
    return (int)isValidRid(foundRid);
}