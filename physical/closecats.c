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

FUNCTION CloseCats ()

PARAMETER DESCRIPTION:
    None

FUNCTION DESCRIPTION:
    Closes the system catalog relations (relcat and attrcat) along with any user relations that remain open in the catcache[].
    This routine is invoked during database shutdown (CloseDB) and must ensure that all buffered pages are flushed, metadata is written back, and file descriptors are closed.
    The routine proceeds by:
        • Verifying that both catalog cache entries (relcat and attrcat) are valid.
        • Closing all open user relations (slots 2 … MAXOPEN-1).
        • Closing attrcat (slot 1).
        • Closing relcat (slot 0).

ALGORITHM:
    1) Check that catcache[0] and catcache[1] both have VALID_MASK set.
        If either is invalid, return NOTOK.
    2) For each cache slot i = 2 … MAXOPEN-1:
        a) If catcache[i] is valid, call CloseRel(i).
        b) If CloseRel(i) fails, return NOTOK.
    3) Close attrcat by calling CloseRel(ATTRCAT_CACHE).
        If failure, return NOTOK.
    4) Close relcat by calling CloseRel(RELCAT_CACHE).
        If failure, return NOTOK.
    5) Return OK.

BUGS:
    None found.

ERRORS REPORTED:
    None directly.

GLOBAL VARIABLES MODIFIED:
    catcache[] entries’ statuses are cleared by CloseRel()
    buffer[] pages flushed by CloseRel()

------------------------------------------------------------*/

/* Close all system catalogs */
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

    /* Close relation attrcat */
    if(CloseRel(ATTRCAT_CACHE) == NOTOK)
    {
        return NOTOK;
    }

    /* Close relation relcat */
    if(CloseRel(RELCAT_CACHE) == NOTOK)
    {
        return NOTOK;
    }

    return OK;
}