/************************INCLUDES*******************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/closerel.h"


/*------------------------------------------------------------

FUNCTION CloseCats (argc, argv)

PARAMETER DESCRIPTION:
    None. Invoked internally during CloseDB.

FUNCTION DESCRIPTION:
    Closes all system catalog relations and any other open relations in the catalog cache. 
    Ensures that all catalog cache entries are properly flushed.

ALGORITHM:
    1. Verify that catalog cache entries for relcat and attrcat are valid.
       If either is invalid, return NOTOK.
    2. Iterate over catcache slots 2..MAXOPEN-1:
         - For each valid entry, call CloseRel(i).
    3. Close attrcat explicitly using CloseRel(ATTRCAT_CACHE).
    4. Close relcat explicitly using CloseRel(RELCAT_CACHE).
    5. Return OK.

ERRORS REPORTED:
    - Returns NOTOK when catalog cache[0] or cache[1] are invalid or when CloseRel retuens NOTOK.

GLOBAL VARIABLES MODIFIED:
    - catcache[] entries are modified indirectly through CloseRel().
    - Underlying file descriptors in catcache[] may be closed.

------------------------------------------------------------*/


int CloseCats() 
{
    if(!(catcache[0].status & VALID_MASK) || !(catcache[1].status & VALID_MASK))
    {
        return NOTOK;
    }

    for(int i=2;i<MAXOPEN;i++)
    {
        if(catcache[i].status & VALID_MASK)
        {
            if(CloseRel(i) == NOTOK)
            {
                return NOTOK;
            }
        }
    }

    if(CloseRel(ATTRCAT_CACHE) == OK && CloseRel(RELCAT_CACHE) == OK)
    {
        return OK;
    }

    return NOTOK;
}